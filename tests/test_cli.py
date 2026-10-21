"""Unit tests for the CLI commands."""

from pathlib import Path
import sys
import pytest

from mem_tac_review.cli import main


def test_cli_help(capsys) -> None:
    with pytest.raises(SystemExit) as exc:
        main(["--help"])
    assert exc.value.code == 0
    captured = capsys.readouterr()
    assert "MemTacToe" in captured.out


def test_cli_list_scenarios(capsys) -> None:
    code = main(["list-scenarios"])
    assert code == 0
    captured = capsys.readouterr()
    assert "G01" in captured.out
    assert "N01_4" in captured.out
    assert "E01" in captured.out


def test_cli_list_suites(capsys) -> None:
    code = main(["list-suites"])
    assert code == 0
    captured = capsys.readouterr()
    assert "lab-2" in captured.out
    assert "games-3x3" in captured.out


def test_cli_show_scenario(capsys) -> None:
    code = main(["show-scenario", "G01"])
    assert code == 0
    captured = capsys.readouterr()
    assert "Top row win" in captured.out
    assert "Grid Size: 3x3" in captured.out


def test_cli_validate_profile(tmp_path: Path, capsys) -> None:
    # Valid profile
    valid_p = tmp_path / "valid.yaml"
    valid_p.write_text("name: valid_test\nlaunch_p1: ['./p1']\n", encoding="utf-8")
    assert main(["validate-profile", str(valid_p)]) == 0
    captured = capsys.readouterr()
    assert "is valid" in captured.out

    # Invalid profile
    invalid_p = tmp_path / "invalid.yaml"
    invalid_p.write_text("topology: invalid_topo\n", encoding="utf-8")
    assert main(["validate-profile", str(invalid_p)]) == 1
    err = capsys.readouterr()
    assert "validation failed" in err.err


def test_cli_run_command(tmp_path: Path, capsys) -> None:
    script = tmp_path / "mock.py"
    script.write_text("print('Mock output')\n", encoding="utf-8")

    profile_path = tmp_path / "profile.yaml"
    profile_path.write_text(
        f"name: cli_test\ntopology: single\nlaunch_p1: ['{sys.executable}', '{script}']\ncreator: p1\n",
        encoding="utf-8",
    )

    out_dir = tmp_path / "report_out"
    code = main([
        "run",
        "--profile", str(profile_path),
        "--scenario", "E01",
        "--output", str(out_dir),
    ])
    assert code == 0
    assert (out_dir / "report.html").is_file()
    assert (out_dir / "report.json").is_file()


def test_cli_batch_command(tmp_path: Path, capsys) -> None:
    profiles_dir = tmp_path / "profiles"
    profiles_dir.mkdir()

    p1 = profiles_dir / "p1.yaml"
    p1.write_text("name: sub1\nlaunch_p1: ['echo', '1']\n", encoding="utf-8")

    p2 = profiles_dir / "p2.yaml"
    p2.write_text("name: sub2\nlaunch_p1: ['echo', '2']\n", encoding="utf-8")

    out_dir = tmp_path / "batch_out"
    code = main([
        "batch",
        "--profiles", str(profiles_dir),
        "--scenario", "E01",
        "--output", str(out_dir),
    ])
    assert code == 0
    assert (out_dir / "index.html").is_file()
    assert (out_dir / "batch_report.json").is_file()
    assert (out_dir / "submissions" / "sub1" / "report.html").is_file()
    assert (out_dir / "submissions" / "sub2" / "report.html").is_file()
