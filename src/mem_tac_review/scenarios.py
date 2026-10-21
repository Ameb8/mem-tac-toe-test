"""Scenario library, suites, and loader for MemTacToe review harness."""

from __future__ import annotations

import json
from pathlib import Path
from typing import Any, Optional
import yaml

from mem_tac_review.models import (
    Action,
    ActionType,
    Coordinate,
    Scenario,
)
from mem_tac_review.reference import ReferenceModel


def _make_game_scenario(
    scenario_id: str,
    name: str,
    description: str,
    size: int,
    moves: list[tuple[str, int, int]],
    tags: Optional[list[str]] = None,
) -> Scenario:
    """Helper to create a standard game scenario from move tuples."""
    actions: list[Action] = []
    for player, r, c in moves:
        actions.append(
            Action(
                type=ActionType.MOVE,
                player=player,
                coord=Coordinate(r, c),
                description=f"Send {player}'s move ({r}, {c})",
            )
        )
    return Scenario(
        id=scenario_id,
        name=name,
        description=description,
        grid_size=size,
        starting_player="X",
        actions=actions,
        tags=tags or ["game", "p1"],
    )


def _generate_nxn_pattern(pattern: str, n: int) -> list[tuple[str, int, int]]:
    """Generate moves for N x N games according to docs/test-cases.md."""
    moves: list[tuple[str, int, int]] = []
    for k in range(n - 1):
        if pattern == "row":
            moves.append(("X", 0, k))
            moves.append(("O", 1, k))
        elif pattern == "col":
            moves.append(("X", k, 0))
            moves.append(("O", k, 1))
        elif pattern == "main_diag":
            moves.append(("X", k, k))
            moves.append(("O", k, k + 1))
        elif pattern == "other_diag":
            moves.append(("X", k, n - 1 - k))
            moves.append(("O", k, n - 2 - k))

    # Last X move
    if pattern == "row":
        moves.append(("X", 0, n - 1))
    elif pattern == "col":
        moves.append(("X", n - 1, 0))
    elif pattern == "main_diag":
        moves.append(("X", n - 1, n - 1))
    elif pattern == "other_diag":
        moves.append(("X", n - 1, 0))

    return moves


class ScenarioRegistry:
    """Built-in library of canonical MemTacToe test scenarios and suites."""

    def __init__(self) -> None:
        self._scenarios: dict[str, Scenario] = {}
        self._suites: dict[str, list[str]] = {}
        self._populate_builtins()

    def register(self, scenario: Scenario) -> None:
        errors = ReferenceModel.validate_scenario(scenario)
        if errors:
            raise ValueError(f"Invalid scenario '{scenario.id}': {'; '.join(errors)}")
        self._scenarios[scenario.id] = scenario

    def get(self, scenario_id: str) -> Optional[Scenario]:
        return self._scenarios.get(scenario_id)

    def list_scenarios(self) -> list[Scenario]:
        return list(self._scenarios.values())

    def get_suite(self, suite_name: str) -> list[Scenario]:
        if suite_name not in self._suites:
            raise KeyError(f"Unknown suite '{suite_name}'. Available suites: {list(self._suites.keys())}")
        scenario_ids = self._suites[suite_name]
        return [self._scenarios[sid] for sid in scenario_ids if sid in self._scenarios]

    def list_suites(self) -> dict[str, list[str]]:
        return dict(self._suites)

    def _populate_builtins(self) -> None:
        # G01-G12: 3x3 games
        self.register(
            _make_game_scenario(
                "G01",
                "Top row win",
                "First player wins on the top row (row 0)",
                3,
                [("X", 0, 0), ("O", 1, 0), ("X", 0, 1), ("O", 1, 1), ("X", 0, 2)],
                ["3x3", "p1", "win"],
            )
        )
        self.register(
            _make_game_scenario(
                "G02",
                "Middle row win",
                "First player wins on the middle row (row 1)",
                3,
                [("X", 1, 0), ("O", 0, 0), ("X", 1, 1), ("O", 0, 1), ("X", 1, 2)],
                ["3x3", "p1", "win"],
            )
        )
        self.register(
            _make_game_scenario(
                "G03",
                "Bottom row win",
                "First player wins on the bottom row (row 2)",
                3,
                [("X", 2, 0), ("O", 0, 0), ("X", 2, 1), ("O", 0, 1), ("X", 2, 2)],
                ["3x3", "p1", "win"],
            )
        )
        self.register(
            _make_game_scenario(
                "G04",
                "Left column win",
                "First player wins on the left column (col 0)",
                3,
                [("X", 0, 0), ("O", 0, 1), ("X", 1, 0), ("O", 1, 1), ("X", 2, 0)],
                ["3x3", "p1", "win"],
            )
        )
        self.register(
            _make_game_scenario(
                "G05",
                "Middle column win",
                "First player wins on the middle column (col 1)",
                3,
                [("X", 0, 1), ("O", 0, 0), ("X", 1, 1), ("O", 1, 0), ("X", 2, 1)],
                ["3x3", "p1", "win"],
            )
        )
        self.register(
            _make_game_scenario(
                "G06",
                "Right column win",
                "First player wins on the right column (col 2)",
                3,
                [("X", 0, 2), ("O", 0, 0), ("X", 1, 2), ("O", 1, 0), ("X", 2, 2)],
                ["3x3", "p1", "win"],
            )
        )
        self.register(
            _make_game_scenario(
                "G07",
                "Main diagonal win",
                "First player wins on the main diagonal (top-left to bottom-right)",
                3,
                [("X", 0, 0), ("O", 0, 1), ("X", 1, 1), ("O", 0, 2), ("X", 2, 2)],
                ["3x3", "p1", "win", "diagonal-win"],
            )
        )
        self.register(
            _make_game_scenario(
                "G08",
                "Other diagonal win",
                "First player wins on the other diagonal (top-right to bottom-left)",
                3,
                [("X", 0, 2), ("O", 0, 0), ("X", 1, 1), ("O", 0, 1), ("X", 2, 0)],
                ["3x3", "p1", "win"],
            )
        )
        self.register(
            _make_game_scenario(
                "G09",
                "Second player wins",
                "Second player (O) wins on column 1",
                3,
                [("X", 0, 0), ("O", 1, 1), ("X", 1, 0), ("O", 0, 1), ("X", 0, 2), ("O", 2, 1)],
                ["3x3", "p1", "win", "p2-win"],
            )
        )
        self.register(
            _make_game_scenario(
                "G10",
                "Full-board draw",
                "Full 3x3 board without a winner",
                3,
                [
                    ("X", 0, 0),
                    ("O", 0, 1),
                    ("X", 0, 2),
                    ("O", 1, 2),
                    ("X", 1, 0),
                    ("O", 2, 0),
                    ("X", 1, 1),
                    ("O", 2, 2),
                    ("X", 2, 1),
                ],
                ["3x3", "p1", "draw"],
            )
        )
        self.register(
            _make_game_scenario(
                "G11",
                "Win on final empty cell",
                "First player wins on move 9; must be recognized as win, not draw",
                3,
                [
                    ("X", 0, 0),
                    ("O", 0, 1),
                    ("X", 0, 2),
                    ("O", 1, 0),
                    ("X", 1, 1),
                    ("O", 1, 2),
                    ("X", 2, 1),
                    ("O", 2, 0),
                    ("X", 2, 2),
                ],
                ["3x3", "p1", "win"],
            )
        )
        self.register(
            _make_game_scenario(
                "G12",
                "Mixed lines are not wins",
                "Mixed marks along lines do not trigger win; game proceeds to draw",
                3,
                [
                    ("X", 0, 0),
                    ("O", 0, 1),
                    ("X", 0, 2),
                    ("O", 1, 1),
                    ("X", 1, 0),
                    ("O", 2, 0),
                    ("X", 2, 1),
                    ("O", 1, 2),
                    ("X", 2, 2),
                ],
                ["3x3", "p1", "draw"],
            )
        )

        # NxN Scenarios for N=4 and N=5
        for n in (4, 5):
            self.register(
                _make_game_scenario(
                    f"N01_{n}",
                    f"{n}x{n} row win",
                    f"X fills top row on {n}x{n} grid",
                    n,
                    _generate_nxn_pattern("row", n),
                    ["nxn", f"{n}x{n}", "p1", "win"],
                )
            )
            self.register(
                _make_game_scenario(
                    f"N02_{n}",
                    f"{n}x{n} column win",
                    f"X fills left column on {n}x{n} grid",
                    n,
                    _generate_nxn_pattern("col", n),
                    ["nxn", f"{n}x{n}", "p1", "win"],
                )
            )
            self.register(
                _make_game_scenario(
                    f"N03_{n}",
                    f"{n}x{n} main diagonal win",
                    f"X fills main diagonal on {n}x{n} grid",
                    n,
                    _generate_nxn_pattern("main_diag", n),
                    ["nxn", f"{n}x{n}", "p1", "win"],
                )
            )
            self.register(
                _make_game_scenario(
                    f"N04_{n}",
                    f"{n}x{n} other diagonal win",
                    f"X fills other diagonal on {n}x{n} grid",
                    n,
                    _generate_nxn_pattern("other_diag", n),
                    ["nxn", f"{n}x{n}", "p1", "win"],
                )
            )

        # CLI Error Handling Scenarios
        self.register(
            Scenario(
                id="E01",
                name="CLI missing size argument",
                description="Omit grid size argument on launch",
                grid_size=3,
                is_cli_scenario=True,
                cli_args_override=[],
                expected_cli_behavior="Diagnostic usage or controlled exit when size argument is omitted",
                tags=["cli", "error", "p1"],
            )
        )
        self.register(
            Scenario(
                id="E02_abc",
                name="CLI non-numeric size argument",
                description="Supply 'abc' as grid size argument",
                grid_size=3,
                is_cli_scenario=True,
                cli_args_override=["abc"],
                expected_cli_behavior="Safely reject non-numeric grid size",
                tags=["cli", "error", "p1"],
            )
        )
        self.register(
            Scenario(
                id="E02_zero",
                name="CLI zero size argument",
                description="Supply '0' as grid size argument",
                grid_size=3,
                is_cli_scenario=True,
                cli_args_override=["0"],
                expected_cli_behavior="Safely reject grid size of 0",
                tags=["cli", "error", "p1"],
            )
        )
        self.register(
            Scenario(
                id="E02_neg",
                name="CLI negative size argument",
                description="Supply '-1' as grid size argument",
                grid_size=3,
                is_cli_scenario=True,
                cli_args_override=["-1"],
                expected_cli_behavior="Safely reject negative grid size",
                tags=["cli", "error", "p1"],
            )
        )

        # In-game Error Handling Scenarios
        # E03: Invalid coordinates (out-of-bounds)
        self.register(
            Scenario(
                id="E03",
                name="Invalid coordinates out-of-bounds",
                description="Negative coordinates and coordinates >= N, followed by valid moves",
                grid_size=3,
                starting_player="X",
                actions=[
                    Action(ActionType.MOVE, "X", Coordinate(-1, 0), description="Negative row", is_intentional_invalid=True),
                    Action(ActionType.MOVE, "X", Coordinate(0, -1), description="Negative column", is_intentional_invalid=True),
                    Action(ActionType.MOVE, "X", Coordinate(3, 0), description="Row N out of bounds", is_intentional_invalid=True),
                    Action(ActionType.MOVE, "X", Coordinate(0, 3), description="Column N out of bounds", is_intentional_invalid=True),
                    Action(ActionType.MOVE, "X", Coordinate(0, 0), description="Valid recovery move (0, 0)"),
                    Action(ActionType.MOVE, "O", Coordinate(1, 0), description="Valid move (1, 0)"),
                    Action(ActionType.MOVE, "X", Coordinate(0, 1), description="Valid move (0, 1)"),
                    Action(ActionType.MOVE, "O", Coordinate(1, 1), description="Valid move (1, 1)"),
                    Action(ActionType.MOVE, "X", Coordinate(0, 2), description="Winning move (0, 2)"),
                ],
                tags=["error", "p1", "bounds"],
            )
        )

        # E04: Occupied cells
        self.register(
            Scenario(
                id="E04",
                name="Occupied cell reuse",
                description="Attempts to reuse cell occupied by either player, followed by valid moves",
                grid_size=3,
                starting_player="X",
                actions=[
                    Action(ActionType.MOVE, "X", Coordinate(0, 0), description="X plays (0, 0)"),
                    Action(ActionType.MOVE, "O", Coordinate(0, 0), description="O attempts occupied (0, 0)", is_intentional_invalid=True),
                    Action(ActionType.MOVE, "O", Coordinate(1, 0), description="O valid move (1, 0)"),
                    Action(ActionType.MOVE, "X", Coordinate(0, 0), description="X attempts own occupied (0, 0)", is_intentional_invalid=True),
                    Action(ActionType.MOVE, "X", Coordinate(0, 1), description="X valid move (0, 1)"),
                    Action(ActionType.MOVE, "O", Coordinate(1, 1), description="O valid move (1, 1)"),
                    Action(ActionType.MOVE, "X", Coordinate(0, 2), description="X winning move (0, 2)"),
                ],
                tags=["error", "p1", "occupied"],
            )
        )

        # E05: Malformed non-numeric input
        self.register(
            Scenario(
                id="E05",
                name="Malformed input recovery",
                description="Supply nonnumeric move input 'abc', then recover with valid move",
                grid_size=3,
                starting_player="X",
                actions=[
                    Action(
                        ActionType.RAW_INPUT,
                        "X",
                        raw_text="abc\n",
                        description="Non-numeric string 'abc'",
                        is_intentional_invalid=True,
                    ),
                    Action(ActionType.MOVE, "X", Coordinate(0, 0), description="Valid recovery move (0, 0)"),
                    Action(ActionType.MOVE, "O", Coordinate(1, 0), description="Valid move (1, 0)"),
                    Action(ActionType.MOVE, "X", Coordinate(0, 1), description="Valid move (0, 1)"),
                    Action(ActionType.MOVE, "O", Coordinate(1, 1), description="Valid move (1, 1)"),
                    Action(ActionType.MOVE, "X", Coordinate(0, 2), description="Winning move (0, 2)"),
                ],
                tags=["error", "p1", "malformed"],
            )
        )

        # E06: Incomplete input
        self.register(
            Scenario(
                id="E06",
                name="Incomplete input and empty line",
                description="Supply empty line, then recover with valid move",
                grid_size=3,
                starting_player="X",
                actions=[
                    Action(
                        ActionType.RAW_INPUT,
                        "X",
                        raw_text="\n",
                        description="Empty line",
                        is_intentional_invalid=True,
                    ),
                    Action(ActionType.MOVE, "X", Coordinate(0, 0), description="Valid recovery move (0, 0)"),
                    Action(ActionType.MOVE, "O", Coordinate(1, 0), description="Valid move (1, 0)"),
                    Action(ActionType.MOVE, "X", Coordinate(0, 1), description="Valid move (0, 1)"),
                    Action(ActionType.MOVE, "O", Coordinate(1, 1), description="Valid move (1, 1)"),
                    Action(ActionType.MOVE, "X", Coordinate(0, 2), description="Winning move (0, 2)"),
                ],
                tags=["error", "p1", "incomplete"],
            )
        )

        # Robustness / Workability Scenarios
        self.register(
            Scenario(
                id="W02_p1",
                name="EOF on Player 1 standard input",
                description="Close standard input on Player 1 to observe controlled exit",
                grid_size=3,
                starting_player="X",
                actions=[
                    Action(ActionType.CLOSE_INPUT, "X", description="Close stdin for Player 1"),
                ],
                tags=["workability", "p2", "eof"],
            )
        )

        # Suites
        # lab-2 canonical suite
        self._suites["lab-2"] = [
            "G01", "G04", "G07", "G08", "G09", "G10",
            "N01_4", "N03_4", "N01_5",
            "E01", "E02_abc", "E02_zero", "E03", "E04", "E05",
        ]
        self._suites["games-3x3"] = [f"G{i:02d}" for i in range(1, 13)]
        self._suites["games-nxn"] = [
            f"N0{p}_{n}" for n in (4, 5) for p in (1, 2, 3, 4)
        ]
        self._suites["errors"] = [
            "E01", "E02_abc", "E02_zero", "E02_neg", "E03", "E04", "E05", "E06"
        ]
        self._suites["cli"] = ["E01", "E02_abc", "E02_zero", "E02_neg"]
        self._suites["all"] = list(self._scenarios.keys())


def load_scenario_from_file(file_path: Path | str) -> Scenario:
    """Load a custom scenario from a YAML or JSON file."""
    path = Path(file_path)
    content = path.read_text(encoding="utf-8")
    if path.suffix in (".yaml", ".yml"):
        data = yaml.safe_load(content)
    else:
        data = json.loads(content)

    actions: list[Action] = []
    for a_data in data.get("actions", []):
        act_type = ActionType(a_data.get("type", "move"))
        coord = None
        if "coord" in a_data and a_data["coord"] is not None:
            coord = Coordinate(a_data["coord"][0], a_data["coord"][1])
        actions.append(
            Action(
                type=act_type,
                player=a_data.get("player", "X"),
                coord=coord,
                raw_text=a_data.get("raw_text"),
                delay_ms=a_data.get("delay_ms", 0),
                description=a_data.get("description", ""),
                is_intentional_invalid=a_data.get("is_intentional_invalid", False),
                expect_checkpoint=a_data.get("expect_checkpoint", True),
            )
        )

    return Scenario(
        id=data.get("id", path.stem),
        name=data.get("name", path.stem),
        description=data.get("description", ""),
        grid_size=data.get("grid_size", 3),
        starting_player=data.get("starting_player", "X"),
        actions=actions,
        tags=data.get("tags", []),
        is_cli_scenario=data.get("is_cli_scenario", False),
        cli_args_override=data.get("cli_args_override"),
        expected_cli_behavior=data.get("expected_cli_behavior", ""),
    )
