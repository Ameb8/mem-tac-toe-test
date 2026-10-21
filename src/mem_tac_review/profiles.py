"""Submission profile loading, validation, and interaction formatting."""

from __future__ import annotations

import json
from pathlib import Path
from typing import Any, Optional, Union
import yaml

from mem_tac_review.models import Action, ActionType, Coordinate, Profile


class ProfileError(Exception):
    """Raised when profile validation or translation fails."""


def load_profile_from_file(file_path: Path | str) -> Profile:
    """Load and validate a submission profile from YAML or JSON."""
    path = Path(file_path).resolve()
    if not path.is_file():
        raise ProfileError(f"Profile file not found: {path}")

    content = path.read_text(encoding="utf-8")
    if path.suffix in (".yaml", ".yml"):
        data = yaml.safe_load(content) or {}
    else:
        data = json.loads(content)

    return parse_profile_dict(data, base_dir=path.parent)


def parse_profile_dict(data: dict[str, Any], base_dir: Optional[Path] = None) -> Profile:
    """Parse and validate profile dictionary."""
    name = data.get("name")
    if not name:
        raise ProfileError("Profile must have a 'name' field")

    topology = data.get("topology", "dual").lower()
    if topology not in ("dual", "single"):
        raise ProfileError(f"Invalid topology '{topology}'. Must be 'dual' or 'single'")

    creator = data.get("creator", "p1").lower()
    if creator not in ("p1", "p2", "shared"):
        raise ProfileError(f"Invalid creator '{creator}'. Must be 'p1', 'p2', or 'shared'")

    starting_player = data.get("starting_player", "p1").lower()
    if starting_player not in ("p1", "p2"):
        raise ProfileError(f"Invalid starting_player '{starting_player}'. Must be 'p1' or 'p2'")

    coord_base = int(data.get("coord_base", 0))
    if coord_base not in (0, 1):
        raise ProfileError(f"Invalid coord_base '{coord_base}'. Must be 0 or 1")

    coord_order = data.get("coord_order", "row_col").lower()
    if coord_order not in ("row_col", "col_row"):
        raise ProfileError(f"Invalid coord_order '{coord_order}'. Must be 'row_col' or 'col_row'")

    work_dir = data.get("work_dir", ".")
    if base_dir and not Path(work_dir).is_absolute():
        work_dir = str((base_dir / work_dir).resolve())

    launch_p1 = data.get("launch_p1", ["./player1", "{N}"])
    if not isinstance(launch_p1, list):
        raise ProfileError("'launch_p1' must be a list of command arguments")

    launch_p2 = data.get("launch_p2", ["./player2"])
    if not isinstance(launch_p2, list):
        raise ProfileError("'launch_p2' must be a list of command arguments")

    return Profile(
        name=name,
        build_cmd=data.get("build_cmd"),
        work_dir=work_dir,
        topology=topology,
        launch_p1=launch_p1,
        launch_p2=launch_p2,
        creator=creator,
        starting_player=starting_player,
        p1_mark=data.get("p1_mark", "X"),
        p2_mark=data.get("p2_mark", "O"),
        coord_base=coord_base,
        coord_order=coord_order,
        input_format=data.get("input_format", "{r} {c}\n"),
        turn_prompt=data.get("turn_prompt"),
        settling_ms=int(data.get("settling_ms", 100)),
        timeout_sec=float(data.get("timeout_sec", 10.0)),
        session_timeout_sec=float(data.get("session_timeout_sec", 60.0)),
        env=data.get("env", {}),
        board_extraction=data.get("board_extraction"),
    )


class ProfileAdapter:
    """Adapts scenario commands and moves to submission profile conventions."""

    def __init__(self, profile: Profile) -> None:
        self.profile = profile

    def resolve_target_terminal(self, logical_player: str) -> str:
        """Map logical player ('X', 'O', 'p1', 'p2') to terminal connection role."""
        if self.profile.topology == "single":
            return "p1"  # Both players share the single terminal connection

        if logical_player in ("p1", "p2"):
            return logical_player

        # Logical player is "X" or "O"
        if logical_player == self.profile.p1_mark:
            return "p1"
        elif logical_player == self.profile.p2_mark:
            return "p2"
        else:
            # Fallback
            return "p1" if logical_player == "X" else "p2"

    def format_action_inputs(self, action: Action) -> list[str]:
        """Convert a scenario Action into one or more concrete input strings for stdin."""
        if action.type == ActionType.RAW_INPUT:
            return [action.raw_text or ""]

        if action.type == ActionType.CLOSE_INPUT or action.type == ActionType.WAIT:
            return []

        if action.coord is None:
            return []

        # Convert coordinates
        r = action.coord.row + self.profile.coord_base
        c = action.coord.col + self.profile.coord_base

        # Map according to coord_order if formatted generically
        # If input_format contains {r} and {c}, format them directly
        fmt = self.profile.input_format
        if isinstance(fmt, list):
            # Sequence of prompts e.g. ["{r}\n", "{c}\n"]
            inputs: list[str] = []
            for item in fmt:
                inputs.append(item.format(r=r, c=c))
            return inputs

        # String format
        # If coord_order is col_row and user format has positional/unnamed or if order is inverted
        if self.profile.coord_order == "col_row" and "{r}" not in fmt and "{c}" not in fmt:
            # Positional format e.g. "%d %d\n"
            return [fmt.format(c, r)]

        return [fmt.format(r=r, c=c)]

    def format_launch_args(self, role: str, grid_size: int, cli_args_override: Optional[list[str]] = None) -> list[str]:
        """Format launch arguments, substituting {N} or applying CLI overrides."""
        raw_args = self.profile.launch_p1 if role == "p1" else self.profile.launch_p2
        formatted: list[str] = []

        if cli_args_override is not None and role == self.profile.creator:
            has_placeholder = any("{N}" in arg for arg in raw_args)
            if has_placeholder:
                for arg in raw_args:
                    if "{N}" in arg:
                        for override_item in cli_args_override:
                            formatted.append(arg.replace("{N}", override_item))
                    else:
                        formatted.append(arg)
                return formatted
            else:
                return list(raw_args) + list(cli_args_override)

        for arg in raw_args:
            if "{N}" in arg:
                formatted.append(arg.replace("{N}", str(grid_size)))
            else:
                formatted.append(arg)

        return formatted
