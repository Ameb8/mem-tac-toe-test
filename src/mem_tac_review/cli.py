"""Command-line interface for MemTacToe review harness."""

from __future__ import annotations

import argparse
from datetime import datetime, timezone
from pathlib import Path
import sys
from typing import Optional

from mem_tac_review.models import Profile, Scenario
from mem_tac_review.profiles import ProfileError, load_profile_from_file
from mem_tac_review.report import ReportGenerator
from mem_tac_review.runner import BatchRunner, SubmissionRunner
from mem_tac_review.scenarios import ScenarioRegistry, load_scenario_from_file


def create_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog="mem-tac-review",
        description="MemTacToe Lab 2 testing and review harness (CWU CS470).",
    )
    subparsers = parser.add_subparsers(dest="command", help="Subcommand to execute")

    # Command: run
    run_parser = subparsers.add_parser("run", help="Run scenarios for a single submission profile")
    run_parser.add_argument("--profile", "-p", required=True, help="Path to submission profile (YAML or JSON)")
    group = run_parser.add_mutually_exclusive_group()
    group.add_argument("--scenario", "-s", help="Run a specific scenario by ID or path to scenario file")
    group.add_argument("--suite", default="lab-2", help="Run a named suite (default: 'lab-2')")
    run_parser.add_argument("--output", "-o", help="Output directory for generated report (default: reports/<profile_name>)")
    run_parser.add_argument("--timeout", type=float, help="Override per-action response timeout (seconds)")
    run_parser.add_argument("--settling-ms", type=int, help="Override quiet-window settling interval (ms)")
    run_parser.add_argument("--session-timeout", type=float, help="Override maximum session duration (seconds)")
    run_parser.add_argument("--verbose", "-v", action="store_true", help="Print verbose progress messages")

    # Command: batch
    batch_parser = subparsers.add_parser("batch", help="Run scenarios for a batch of submission profiles")
    batch_parser.add_argument("--profiles", "-p", required=True, help="Directory containing profile files or path pattern")
    b_group = batch_parser.add_mutually_exclusive_group()
    b_group.add_argument("--suite", default="lab-2", help="Run a named suite (default: 'lab-2')")
    b_group.add_argument("--scenario", "-s", help="Run a single scenario by ID across all profiles")
    batch_parser.add_argument("--output", "-o", help="Output directory for batch report (default: reports/batch_<timestamp>)")
    batch_parser.add_argument("--timeout", type=float, help="Override per-action response timeout (seconds)")
    batch_parser.add_argument("--settling-ms", type=int, help="Override quiet-window settling interval (ms)")
    batch_parser.add_argument("--session-timeout", type=float, help="Override maximum session duration (seconds)")
    batch_parser.add_argument("--verbose", "-v", action="store_true", help="Print verbose progress messages")

    # Command: list-scenarios
    subparsers.add_parser("list-scenarios", help="List all available built-in scenarios")

    # Command: list-suites
    subparsers.add_parser("list-suites", help="List all available scenario suites")

    # Command: show-scenario
    show_parser = subparsers.add_parser("show-scenario", help="Display details of a specific scenario")
    show_parser.add_argument("scenario_id", help="Scenario ID (e.g. G01, N01_4, E03)")

    # Command: validate-profile
    val_parser = subparsers.add_parser("validate-profile", help="Validate a submission profile configuration")
    val_parser.add_argument("profile_path", help="Path to profile file")

    return parser


def _apply_overrides(profile: Profile, args: argparse.Namespace) -> None:
    if getattr(args, "timeout", None) is not None:
        profile.timeout_sec = args.timeout
    if getattr(args, "settling_ms", None) is not None:
        profile.settling_ms = args.settling_ms
    if getattr(args, "session_timeout", None) is not None:
        profile.session_timeout_sec = args.session_timeout


def _resolve_scenarios(registry: ScenarioRegistry, scenario_arg: Optional[str], suite_arg: Optional[str]) -> list[Scenario]:
    if scenario_arg:
        # Check if file
        scen_path = Path(scenario_arg)
        if scen_path.is_file():
            return [load_scenario_from_file(scen_path)]
        scen = registry.get(scenario_arg)
        if not scen:
            raise ValueError(f"Scenario '{scenario_arg}' not found in registry.")
        return [scen]

    suite_name = suite_arg or "lab-2"
    return registry.get_suite(suite_name)


def handle_run(args: argparse.Namespace) -> int:
    registry = ScenarioRegistry()
    try:
        profile = load_profile_from_file(args.profile)
    except Exception as e:
        sys.stderr.write(f"Operational error loading profile: {e}\n")
        return 1

    _apply_overrides(profile, args)

    try:
        scenarios = _resolve_scenarios(registry, args.scenario, args.suite)
    except Exception as e:
        sys.stderr.write(f"Operational error resolving scenarios: {e}\n")
        return 1

    output_dir = Path(args.output or f"reports/{profile.name}")

    if args.verbose:
        print(f"Loaded profile: {profile.name} (topology={profile.topology}, creator={profile.creator})")
        print(f"Executing {len(scenarios)} scenario(s) into {output_dir}...")

    runner = SubmissionRunner(profile)
    submission_result = runner.run_scenarios(scenarios)

    report_path = ReportGenerator.save_submission_report(submission_result, output_dir)
    print(f"Report generated: {report_path}")
    print(f"JSON evidence: {output_dir / 'report.json'}")

    completed_scenarios = sum(1 for s in submission_result.scenario_results if s.status.value == "completed")
    print(f"Execution finished: {completed_scenarios}/{len(scenarios)} scenarios completed interaction.")
    return 0


def handle_batch(args: argparse.Namespace) -> int:
    registry = ScenarioRegistry()
    profiles_dir = Path(args.profiles)

    profile_paths: list[Path] = []
    if profiles_dir.is_dir():
        for ext in ("*.yaml", "*.yml", "*.json"):
            profile_paths.extend(profiles_dir.glob(ext))
    elif profiles_dir.is_file():
        profile_paths.append(profiles_dir)
    else:
        sys.stderr.write(f"Operational error: Profiles path '{args.profiles}' not found.\n")
        return 1

    if not profile_paths:
        sys.stderr.write(f"Operational error: No profile files found in '{args.profiles}'.\n")
        return 1

    profiles: list[Profile] = []
    for p_path in sorted(profile_paths):
        try:
            prof = load_profile_from_file(p_path)
            _apply_overrides(prof, args)
            profiles.append(prof)
        except Exception as e:
            sys.stderr.write(f"Warning: Skipping invalid profile {p_path}: {e}\n")

    if not profiles:
        sys.stderr.write("Operational error: No valid profiles to execute in batch.\n")
        return 1

    try:
        scenarios = _resolve_scenarios(registry, args.scenario, args.suite)
    except Exception as e:
        sys.stderr.write(f"Operational error resolving scenarios: {e}\n")
        return 1

    ts = datetime.now(timezone.utc).strftime("%Y%m%d_%H%M%S")
    output_dir = Path(args.output or f"reports/batch_{ts}")

    if args.verbose:
        print(f"Batch execution: {len(profiles)} profile(s), {len(scenarios)} scenario(s)")
        print(f"Output directory: {output_dir}")

    suite_name = args.scenario if args.scenario else (args.suite or "lab-2")
    batch_runner = BatchRunner(profiles, scenarios, suite_name=suite_name)
    batch_result = batch_runner.run()

    report_path = ReportGenerator.save_batch_report(batch_result, output_dir)
    print(f"Batch report generated: {report_path}")
    print(f"Batch JSON evidence: {output_dir / 'batch_report.json'}")
    return 0


def handle_list_scenarios() -> int:
    registry = ScenarioRegistry()
    print("Available Scenarios:")
    print("-------------------")
    for s in registry.list_scenarios():
        tags = f" [{', '.join(s.tags)}]" if s.tags else ""
        print(f"  {s.id:<10} (N={s.grid_size}) {s.name}{tags}")
        if s.description:
            print(f"             {s.description}")
    return 0


def handle_list_suites() -> int:
    registry = ScenarioRegistry()
    print("Available Scenario Suites:")
    print("-------------------------")
    for name, sids in registry.list_suites().items():
        print(f"  {name:<12} ({len(sids)} scenarios): {', '.join(sids[:6])}{'...' if len(sids) > 6 else ''}")
    return 0


def handle_show_scenario(scenario_id: str) -> int:
    registry = ScenarioRegistry()
    scen = registry.get(scenario_id)
    if not scen:
        sys.stderr.write(f"Scenario '{scenario_id}' not found.\n")
        return 1

    print(f"Scenario: {scen.id} - {scen.name}")
    print(f"Description: {scen.description}")
    print(f"Grid Size: {scen.grid_size}x{scen.grid_size} | Starting Player: {scen.starting_player}")
    if scen.is_cli_scenario:
        print(f"Type: CLI Argument Test | Arguments Override: {scen.cli_args_override}")
        print(f"Expected Behavior: {scen.expected_cli_behavior}")
    else:
        print(f"Actions ({len(scen.actions)} total):")
        for idx, act in enumerate(scen.actions, 1):
            coord_str = f" at {act.coord}" if act.coord else ""
            raw_str = f" text={repr(act.raw_text)}" if act.raw_text else ""
            invalid_tag = " [Intentional Invalid]" if act.is_intentional_invalid else ""
            print(f"  {idx}. [{act.type.value}] Player {act.player}{coord_str}{raw_str}{invalid_tag}: {act.description}")
    return 0


def handle_validate_profile(path_str: str) -> int:
    try:
        p = load_profile_from_file(path_str)
        print(f"Profile '{p.name}' is valid.")
        print(f"  Topology: {p.topology}")
        print(f"  Creator: {p.creator}")
        print(f"  Coord base: {p.coord_base} | Coord order: {p.coord_order}")
        print(f"  Input format: {p.input_format}")
        print(f"  P1 launch: {p.launch_p1}")
        print(f"  P2 launch: {p.launch_p2}")
        return 0
    except Exception as e:
        sys.stderr.write(f"Profile validation failed: {e}\n")
        return 1


def main(argv: Optional[list[str]] = None) -> int:
    parser = create_parser()
    args = parser.parse_args(argv)

    if not args.command:
        parser.print_help()
        return 0

    if args.command == "run":
        return handle_run(args)
    elif args.command == "batch":
        return handle_batch(args)
    elif args.command == "list-scenarios":
        return handle_list_scenarios()
    elif args.command == "list-suites":
        return handle_list_suites()
    elif args.command == "show-scenario":
        return handle_show_scenario(args.scenario_id)
    elif args.command == "validate-profile":
        return handle_validate_profile(args.profile_path)

    return 0


if __name__ == "__main__":
    sys.exit(main())
