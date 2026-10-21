"""Unit and integration tests for Session, Submission, and Batch runners."""

import sys
from pathlib import Path
import pytest

from mem_tac_review.models import (
    Action,
    ActionType,
    Coordinate,
    Profile,
    Scenario,
    SessionStatus,
)
from mem_tac_review.runner import BatchRunner, SessionRunner, SubmissionRunner


def test_submission_build_failure(tmp_path: Path) -> None:
    # Profile with a failing build command
    profile = Profile(
        name="failing_build",
        build_cmd="false",
        work_dir=str(tmp_path),
    )
    runner = SubmissionRunner(profile)
    scenarios = [
        Scenario(id="S1", name="Scen 1", description=""),
        Scenario(id="S2", name="Scen 2", description=""),
    ]
    sub_result = runner.run_scenarios(scenarios)

    assert sub_result.build_result is not None
    assert sub_result.build_result.success is False
    assert len(sub_result.scenario_results) == 2
    for s_res in sub_result.scenario_results:
        assert s_res.status == SessionStatus.BUILD_ERROR
        assert "Build failed" in s_res.status_reason


def test_cli_error_scenario_execution(tmp_path: Path) -> None:
    # A script that expects an argument and fails with exit code 2 if omitted
    script = tmp_path / "cli_test.py"
    script.write_text(
        "import sys\n"
        "if len(sys.argv) < 2:\n"
        "    print('Usage: player <size>', file=sys.stderr)\n"
        "    sys.exit(2)\n"
        "print(f'Size: {sys.argv[1]}')\n",
        encoding="utf-8",
    )

    profile = Profile(
        name="cli_submission",
        topology="single",
        launch_p1=[sys.executable, str(script), "{N}"],
        creator="p1",
        work_dir=str(tmp_path),
    )

    scenario = Scenario(
        id="E01",
        name="Missing size",
        description="Omit size",
        grid_size=3,
        is_cli_scenario=True,
        cli_args_override=[],
        expected_cli_behavior="Usage error on missing arg",
    )

    runner = SessionRunner(profile, scenario)
    result = runner.run()

    assert result.status == SessionStatus.COMPLETED
    assert len(result.checkpoints) == 1
    assert "Usage: player <size>" in result.transcripts["p1"]
    assert result.processes[0].exit_code == 2


def test_interactive_game_scenario_with_mock_player(tmp_path: Path) -> None:
    # Create a small interactive script that prints a board and reads a move
    player_script = tmp_path / "player.py"
    player_script.write_text(
        "import sys\n"
        "print('=== Board ===', flush=True)\n"
        "print('. . .\\n. . .\\n. . .', flush=True)\n"
        "print('Enter move: ', end='', flush=True)\n"
        "move = sys.stdin.readline().strip()\n"
        "print(f'Received: {move}', flush=True)\n"
        "print('X . .\\n. . .\\n. . .', flush=True)\n",
        encoding="utf-8",
    )

    profile = Profile(
        name="mock_player",
        topology="single",
        launch_p1=[sys.executable, str(player_script), "{N}"],
        creator="p1",
        turn_prompt="Enter move:",
        settling_ms=100,
        work_dir=str(tmp_path),
    )

    scenario = Scenario(
        id="quick_move",
        name="Quick single move",
        description="",
        grid_size=3,
        actions=[
            Action(ActionType.MOVE, "X", Coordinate(0, 0), description="First move (0, 0)"),
        ],
    )

    runner = SessionRunner(profile, scenario)
    result = runner.run()

    assert result.status == SessionStatus.COMPLETED
    assert len(result.checkpoints) == 1
    cp = result.checkpoints[0]
    assert cp.submitted_text == "0 0\n"
    assert "X . ." in cp.expected_board_str
    assert "Enter move:" in cp.observed_output["p1"]
    assert cp.extracted_board_str is not None
    assert "X . ." in cp.extracted_board_str


def test_batch_runner(tmp_path: Path) -> None:
    p1 = Profile(name="sub1", build_cmd="true", work_dir=str(tmp_path))
    p2 = Profile(name="sub2", build_cmd="false", work_dir=str(tmp_path))
    scenarios = [Scenario(id="G01", name="Top row", description="", grid_size=3)]

    batch = BatchRunner([p1, p2], scenarios, suite_name="test_suite")
    batch_res = batch.run()

    assert len(batch_res.submissions) == 2
    assert batch_res.submissions[0].submission_name == "sub1"
    assert batch_res.submissions[1].submission_name == "sub2"
    assert batch_res.submissions[1].build_result.success is False
