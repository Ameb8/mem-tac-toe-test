"""Session, Submission, and Batch execution runners."""

from __future__ import annotations

from datetime import datetime, timezone
import os
from pathlib import Path
import platform
import subprocess
import time
from typing import Optional

from mem_tac_review.extractor import BoardExtractor
from mem_tac_review.ipc import IpcInspector
from mem_tac_review.models import (
    Action,
    ActionType,
    BatchResult,
    BuildResult,
    Checkpoint,
    GameOutcome,
    ProcessInfo,
    Profile,
    Scenario,
    SessionResult,
    SessionStatus,
    SubmissionResult,
)
from mem_tac_review.profiles import ProfileAdapter, load_profile_from_file
from mem_tac_review.pty_process import PtyProcess, PtySession
from mem_tac_review.reference import ReferenceModel


class SessionRunner:
    """Executes a single scenario for a given submission profile."""

    def __init__(self, profile: Profile, scenario: Scenario) -> None:
        self.profile = profile
        self.scenario = scenario
        self.adapter = ProfileAdapter(profile)

    def run(self) -> SessionResult:
        """Run the scenario and return structured review evidence."""
        start_time = time.monotonic()
        ipc_before = IpcInspector.get_sysv_shm_snapshot()

        # Handle CLI error scenarios
        if self.scenario.is_cli_scenario:
            return self._run_cli_scenario(ipc_before, start_time)

        # Game scenario execution
        return self._run_game_scenario(ipc_before, start_time)

    def _run_cli_scenario(self, ipc_before: dict[str, str], start_time: float) -> SessionResult:
        """Run a command-line argument test scenario."""
        launch_args = self.adapter.format_launch_args(
            role=self.profile.creator,
            grid_size=self.scenario.grid_size,
            cli_args_override=self.scenario.cli_args_override,
        )

        proc_info: list[ProcessInfo] = []
        status = SessionStatus.COMPLETED
        status_reason = ""
        transcripts: dict[str, str] = {}
        checkpoints: list[Checkpoint] = []

        try:
            pty_proc = PtyProcess(
                command=launch_args,
                work_dir=self.profile.work_dir,
                env=self.profile.env,
                role=self.profile.creator,
            )
            # Wait for exit or short timeout (max 3 seconds)
            wait_start = time.monotonic()
            exit_code = None
            while time.monotonic() - wait_start < 3.0:
                pty_proc.read_available()
                exit_code = pty_proc.poll()
                if exit_code is not None:
                    break
                time.sleep(0.05)

            if exit_code is None:
                # Still running, terminate
                exit_code = pty_proc.terminate(timeout_sec=1.0)
                status_reason = "Process did not terminate after invalid CLI argument within 3.0s"
            else:
                status_reason = f"Process exited with code {exit_code}"

            output = pty_proc.accumulated_output
            transcripts[self.profile.creator] = output
            proc_info.append(
                ProcessInfo(
                    role=self.profile.creator,
                    pid=pty_proc.pid,
                    exit_code=exit_code,
                    stdout_transcript=output,
                )
            )

            # Record a single checkpoint showing diagnostic output
            checkpoints.append(
                Checkpoint(
                    action_index=1,
                    target_player="CLI",
                    submitted_text=f"Launch with arguments: {' '.join(launch_args)}",
                    action_summary=f"CLI Test: {self.scenario.expected_cli_behavior}",
                    expected_board_str="[N/A - Command-line test scenario]",
                    expected_next_player=None,
                    expected_outcome=self.scenario.expected_cli_behavior,
                    observed_output={self.profile.creator: output},
                    extracted_board_str=None,
                    boundary_reason="process exit or 3s timeout",
                    timestamp_iso=datetime.now(timezone.utc).isoformat(),
                )
            )
        except Exception as e:
            status = SessionStatus.LAUNCH_ERROR
            status_reason = f"Failed to launch command {launch_args}: {e}"

        duration = time.monotonic() - start_time
        ipc_after = IpcInspector.get_sysv_shm_snapshot()
        new_ipc, _ = IpcInspector.format_snapshot_diff(ipc_before, ipc_after)

        return SessionResult(
            scenario_id=self.scenario.id,
            scenario_name=self.scenario.name,
            status=status,
            status_reason=status_reason,
            duration_sec=duration,
            checkpoints=checkpoints,
            unexecuted_actions=[],
            transcripts=transcripts,
            processes=proc_info,
            ipc_state_before=str(ipc_before),
            ipc_state_after=str(ipc_after),
            ipc_leaks_detected=new_ipc,
        )

    def _run_game_scenario(self, ipc_before: dict[str, str], start_time: float) -> SessionResult:
        """Run an interactive game scenario with PTY process management."""
        grid_size = self.scenario.grid_size
        reference_model = ReferenceModel(grid_size, self.scenario.starting_player)

        processes: dict[str, PtyProcess] = {}
        checkpoints: list[Checkpoint] = []
        unexecuted_actions: list[Action] = []
        status = SessionStatus.COMPLETED
        status_reason = ""
        warnings: list[str] = []

        # Determine launch order: creator starts first to setup shared memory
        creator = self.profile.creator
        joiner = "p2" if creator == "p1" else "p1"

        try:
            # 1. Launch creator
            creator_args = self.adapter.format_launch_args(creator, grid_size)
            processes[creator] = PtyProcess(
                command=creator_args,
                work_dir=self.profile.work_dir,
                env=self.profile.env,
                role=creator,
            )

            # Give creator a small settling interval (up to settling_ms or 150ms) to create shared memory
            time.sleep(max(self.profile.settling_ms / 1000.0, 0.15))

            # 2. Launch joiner if dual topology
            if self.profile.topology == "dual":
                joiner_args = self.adapter.format_launch_args(joiner, grid_size)
                processes[joiner] = PtyProcess(
                    command=joiner_args,
                    work_dir=self.profile.work_dir,
                    env=self.profile.env,
                    role=joiner,
                )

        except Exception as e:
            # Launch failure
            for p in processes.values():
                p.terminate()
            duration = time.monotonic() - start_time
            return SessionResult(
                scenario_id=self.scenario.id,
                scenario_name=self.scenario.name,
                status=SessionStatus.LAUNCH_ERROR,
                status_reason=f"Failed to launch processes: {e}",
                duration_sec=duration,
                unexecuted_actions=list(self.scenario.actions),
            )

        session = PtySession(processes)

        # Wait for startup output / initial prompt
        first_role = self.adapter.resolve_target_terminal(self.scenario.starting_player)
        session.wait_for_boundary(
            target_role=first_role,
            turn_prompt=self.profile.turn_prompt,
            settling_ms=self.profile.settling_ms,
            timeout_sec=min(self.profile.timeout_sec, 5.0),
        )

        # Drain startup banner/boards so they appear in initial output
        initial_output = session.drain_all()

        # Action execution loop
        for act_idx, action in enumerate(self.scenario.actions):
            # Check overall session timeout
            if time.monotonic() - start_time > self.profile.session_timeout_sec:
                status = SessionStatus.TIMED_OUT
                status_reason = f"Session exceeded maximum duration of {self.profile.session_timeout_sec}s"
                unexecuted_actions = self.scenario.actions[act_idx:]
                break

            # Check if all processes died prematurely
            if session.are_all_dead():
                status = SessionStatus.INTERRUPTED
                status_reason = f"Processes terminated early at action {act_idx + 1}"
                unexecuted_actions = self.scenario.actions[act_idx:]
                break

            # Update reference model
            if action.type == ActionType.MOVE and action.coord is not None:
                if not action.is_intentional_invalid:
                    reference_model.apply_move(action.player, action.coord.row, action.coord.col)
                # If intentional invalid, reference model state remains unchanged

            # Prepare submitted text
            submitted_text = ""
            target_role = self.adapter.resolve_target_terminal(action.player)
            target_proc = processes.get(target_role)

            if target_proc and target_proc.is_alive():
                if action.type == ActionType.MOVE:
                    inputs = self.adapter.format_action_inputs(action)
                    submitted_text = "".join(inputs)
                    for inp in inputs:
                        target_proc.write(inp)
                elif action.type == ActionType.RAW_INPUT:
                    submitted_text = action.raw_text or ""
                    target_proc.write(submitted_text)
                elif action.type == ActionType.CLOSE_INPUT:
                    submitted_text = "[EOF / stdin closed]"
                    target_proc.close_stdin()
                elif action.type == ActionType.WAIT:
                    submitted_text = f"[Wait {action.delay_ms}ms]"
                    time.sleep(action.delay_ms / 1000.0)

            # Pace interaction and wait for boundary
            boundary_ok, boundary_reason = session.wait_for_boundary(
                target_role=target_role,
                turn_prompt=self.profile.turn_prompt,
                settling_ms=self.profile.settling_ms,
                timeout_sec=self.profile.timeout_sec,
            )

            # Drain newly observed output
            drained_output = session.drain_all()

            # If this is the first checkpoint and initial_output had text, prepend it
            if act_idx == 0:
                for r, txt in initial_output.items():
                    if txt:
                        drained_output[r] = txt + drained_output.get(r, "")

            # Attempt board extraction from creator output or target role output
            creator_output = drained_output.get(self.profile.creator, "")
            extracted_board = BoardExtractor.extract_board(
                creator_output, grid_size, self.profile.board_extraction
            )
            if not extracted_board and target_role in drained_output:
                extracted_board = BoardExtractor.extract_board(
                    drained_output[target_role], grid_size, self.profile.board_extraction
                )

            # Record Checkpoint
            if action.expect_checkpoint:
                checkpoints.append(
                    Checkpoint(
                        action_index=act_idx + 1,
                        target_player=action.player,
                        submitted_text=submitted_text,
                        action_summary=action.description or f"Send {action.player} move {action.coord}",
                        expected_board_str=reference_model.board.to_display_string(),
                        expected_next_player=(
                            reference_model.current_player
                            if reference_model.outcome == GameOutcome.IN_PROGRESS
                            else None
                        ),
                        expected_outcome=reference_model.outcome.value,
                        observed_output=drained_output,
                        extracted_board_str=extracted_board,
                        boundary_reason=boundary_reason,
                        timestamp_iso=datetime.now(timezone.utc).isoformat(),
                    )
                )

            if not boundary_ok:
                warnings.append(f"Action {act_idx + 1} ({action.description}): {boundary_reason}")
                # If timed out on a move, mark session timed out and stop
                if "timed out" in boundary_reason:
                    status = SessionStatus.TIMED_OUT
                    status_reason = f"Timed out waiting for response to action {act_idx + 1}"
                    unexecuted_actions = self.scenario.actions[act_idx + 1 :]
                    break

        # Post-execution cleanup: terminate and harvest transcripts
        transcripts = session.get_transcripts()
        proc_infos: list[ProcessInfo] = []
        for role, proc in processes.items():
            exit_code = proc.terminate(timeout_sec=2.0)
            proc_infos.append(
                ProcessInfo(
                    role=role,
                    pid=proc.pid,
                    exit_code=exit_code,
                    stdout_transcript=proc.accumulated_output,
                )
            )

        duration = time.monotonic() - start_time
        ipc_after = IpcInspector.get_sysv_shm_snapshot()
        new_ipc, _ = IpcInspector.format_snapshot_diff(ipc_before, ipc_after)

        return SessionResult(
            scenario_id=self.scenario.id,
            scenario_name=self.scenario.name,
            status=status,
            status_reason=status_reason,
            duration_sec=duration,
            checkpoints=checkpoints,
            unexecuted_actions=unexecuted_actions,
            transcripts=transcripts,
            processes=proc_infos,
            ipc_state_before=str(ipc_before),
            ipc_state_after=str(ipc_after),
            ipc_leaks_detected=new_ipc,
            warnings=warnings,
        )


class SubmissionRunner:
    """Manages building a submission and running a suite or list of scenarios."""

    def __init__(self, profile: Profile) -> None:
        self.profile = profile

    def build(self, timeout_sec: float = 60.0) -> BuildResult:
        """Execute build command if configured."""
        if not self.profile.build_cmd:
            return BuildResult(
                command=None,
                success=True,
                exit_code=0,
                output="No build command specified.",
                duration_sec=0.0,
            )

        start = time.monotonic()
        try:
            res = subprocess.run(
                self.profile.build_cmd,
                shell=True,
                cwd=self.profile.work_dir,
                env=dict(os.environ, **self.profile.env),
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                timeout=timeout_sec,
                check=False,
            )
            duration = time.monotonic() - start
            return BuildResult(
                command=self.profile.build_cmd,
                success=(res.returncode == 0),
                exit_code=res.returncode,
                output=res.stdout,
                duration_sec=duration,
            )
        except subprocess.TimeoutExpired as e:
            duration = time.monotonic() - start
            output = e.stdout if isinstance(e.stdout, str) else (e.stdout.decode("utf-8") if e.stdout else "")
            return BuildResult(
                command=self.profile.build_cmd,
                success=False,
                exit_code=-1,
                output=output + f"\nBuild timed out after {timeout_sec}s",
                duration_sec=duration,
            )
        except Exception as e:
            duration = time.monotonic() - start
            return BuildResult(
                command=self.profile.build_cmd,
                success=False,
                exit_code=-1,
                output=f"Build failed to execute: {e}",
                duration_sec=duration,
            )

    def run_scenarios(self, scenarios: list[Scenario]) -> SubmissionResult:
        """Build submission and execute scenarios, returning aggregated SubmissionResult."""
        build_result = self.build()
        scenario_results: list[SessionResult] = []

        if not build_result.success:
            # Build failed: mark all scenarios as BUILD_ERROR and do not attempt execution
            for scen in scenarios:
                scenario_results.append(
                    SessionResult(
                        scenario_id=scen.id,
                        scenario_name=scen.name,
                        status=SessionStatus.BUILD_ERROR,
                        status_reason=f"Build failed with exit code {build_result.exit_code}",
                        duration_sec=0.0,
                        unexecuted_actions=list(scen.actions),
                    )
                )
        else:
            # Build succeeded: run scenarios sequentially
            for scen in scenarios:
                runner = SessionRunner(self.profile, scen)
                result = runner.run()
                scenario_results.append(result)

        return SubmissionResult(
            submission_name=self.profile.name,
            profile_name=self.profile.name,
            profile_data={
                "topology": self.profile.topology,
                "launch_p1": self.profile.launch_p1,
                "launch_p2": self.profile.launch_p2,
                "creator": self.profile.creator,
                "coord_base": self.profile.coord_base,
                "coord_order": self.profile.coord_order,
                "input_format": str(self.profile.input_format),
                "work_dir": self.profile.work_dir,
            },
            timestamp_iso=datetime.now(timezone.utc).isoformat(),
            platform_info=f"{platform.system()} {platform.release()} ({platform.machine()})",
            build_result=build_result,
            scenario_results=scenario_results,
        )


class BatchRunner:
    """Runs a suite of scenarios across multiple submission profiles."""

    def __init__(self, profiles: list[Profile], scenarios: list[Scenario], suite_name: str = "batch") -> None:
        self.profiles = profiles
        self.scenarios = scenarios
        self.suite_name = suite_name

    def run(self) -> BatchResult:
        """Execute all submissions sequentially, continuing through timeouts and build errors."""
        submission_results: list[SubmissionResult] = []

        for prof in self.profiles:
            sub_runner = SubmissionRunner(prof)
            result = sub_runner.run_scenarios(self.scenarios)
            submission_results.append(result)

        return BatchResult(
            timestamp_iso=datetime.now(timezone.utc).isoformat(),
            suite_name=self.suite_name,
            submissions=submission_results,
        )
