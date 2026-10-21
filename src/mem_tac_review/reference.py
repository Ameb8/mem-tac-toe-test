"""Reference Tic-Tac-Toe model and scenario validation."""

from __future__ import annotations

import copy
from typing import Optional

from mem_tac_review.models import (
    Action,
    ActionType,
    Coordinate,
    GameOutcome,
    PlayerMark,
    Scenario,
)


class Board:
    """N x N Tic-Tac-Toe board."""

    def __init__(self, size: int, grid: Optional[list[list[str]]] = None) -> None:
        if size < 1:
            raise ValueError(f"Grid size must be >= 1, got {size}")
        self.size = size
        if grid is not None:
            self._grid = [row[:] for row in grid]
        else:
            self._grid = [[PlayerMark.EMPTY.value for _ in range(size)] for _ in range(size)]

    def copy(self) -> Board:
        return Board(self.size, self._grid)

    def is_valid_coord(self, row: int, col: int) -> bool:
        return 0 <= row < self.size and 0 <= col < self.size

    def get(self, row: int, col: int) -> str:
        if not self.is_valid_coord(row, col):
            raise IndexError(f"Coordinate ({row}, {col}) out of bounds for {self.size}x{self.size} board")
        return self._grid[row][col]

    def set(self, row: int, col: int, mark: str) -> None:
        if not self.is_valid_coord(row, col):
            raise IndexError(f"Coordinate ({row}, {col}) out of bounds for {self.size}x{self.size} board")
        self._grid[row][col] = mark

    def is_empty(self, row: int, col: int) -> bool:
        return self.get(row, col) == PlayerMark.EMPTY.value

    def is_full(self) -> bool:
        return all(self._grid[r][c] != PlayerMark.EMPTY.value for r in range(self.size) for c in range(self.size))

    def to_display_string(self) -> str:
        """Format board into monospace grid representation."""
        lines: list[str] = []
        for r in range(self.size):
            lines.append(" ".join(self._grid[r]))
        return "\n".join(lines)

    def __str__(self) -> str:
        return self.to_display_string()

    def __eq__(self, other: object) -> bool:
        if not isinstance(other, Board):
            return False
        return self.size == other.size and self._grid == other._grid


class ReferenceModel:
    """Calculates expected game state independently from any submission.

    Rules:
    - N x N board.
    - An entire row, column, or diagonal of N identical marks constitutes a win.
    - A full board without a win constitutes a draw.
    - Deliberately invalid moves leave expected board and turn unchanged.
    """

    def __init__(self, size: int = 3, starting_player: str = PlayerMark.X.value) -> None:
        self.size = size
        self.board = Board(size)
        self.current_player: str = starting_player
        self.outcome: GameOutcome = GameOutcome.IN_PROGRESS
        self.winning_line: Optional[str] = None
        self.move_history: list[tuple[str, int, int]] = []

    def copy(self) -> ReferenceModel:
        clone = ReferenceModel(self.size, self.current_player)
        clone.board = self.board.copy()
        clone.outcome = self.outcome
        clone.winning_line = self.winning_line
        clone.move_history = list(self.move_history)
        return clone

    def apply_move(self, player: str, row: int, col: int) -> bool:
        """Attempt to apply a move.

        Returns True if the move was legal and applied.
        Returns False if illegal (out of bounds, cell occupied, wrong turn, or game already ended).
        On False, state remains completely unchanged.
        """
        # If game is already over, no further moves are accepted
        if self.outcome != GameOutcome.IN_PROGRESS:
            return False

        # Check turn
        if player != self.current_player:
            return False

        # Check coordinates
        if not self.board.is_valid_coord(row, col):
            return False

        # Check cell is empty
        if not self.board.is_empty(row, col):
            return False

        # Apply move
        self.board.set(row, col, player)
        self.move_history.append((player, row, col))

        # Check win or draw
        if self._check_win(player, row, col):
            self.outcome = GameOutcome.X_WINS if player == PlayerMark.X.value else GameOutcome.O_WINS
        elif self.board.is_full():
            self.outcome = GameOutcome.DRAW
        else:
            # Advance turn
            self.current_player = PlayerMark.opponent(self.current_player)

        return True

    def _check_win(self, mark: str, last_r: int, last_c: int) -> bool:
        n = self.size

        # Check row
        if all(self.board.get(last_r, c) == mark for c in range(n)):
            self.winning_line = f"row {last_r}"
            return True

        # Check column
        if all(self.board.get(r, last_c) == mark for r in range(n)):
            self.winning_line = f"column {last_c}"
            return True

        # Check main diagonal
        if last_r == last_c:
            if all(self.board.get(i, i) == mark for i in range(n)):
                self.winning_line = "main diagonal"
                return True

        # Check other (anti) diagonal
        if last_r + last_c == n - 1:
            if all(self.board.get(i, n - 1 - i) == mark for i in range(n)):
                self.winning_line = "other diagonal"
                return True

        return False

    @classmethod
    def validate_scenario(cls, scenario: Scenario) -> list[str]:
        """Validate scenario consistency before execution.

        Checks:
        - Accidental occupied-cell move
        - Accidental out-of-bounds move
        - Move after game is already terminal
        - Accidental turn order discrepancy
        Intentional invalid actions are skipped from triggering consistency errors.
        """
        errors: list[str] = []
        if scenario.is_cli_scenario:
            return errors

        if scenario.grid_size < 1:
            errors.append(f"Scenario grid size {scenario.grid_size} must be >= 1")
            return errors

        model = cls(scenario.grid_size, scenario.starting_player)

        for idx, action in enumerate(scenario.actions):
            if action.type != ActionType.MOVE:
                continue

            if action.coord is None:
                errors.append(f"Action {idx}: Move action missing coordinates")
                continue

            r, c = action.coord.row, action.coord.col

            if action.is_intentional_invalid:
                # Intentional invalid moves do not mutate expected board
                continue

            # Check if game was already over
            if model.outcome != GameOutcome.IN_PROGRESS:
                errors.append(
                    f"Action {idx}: Attempted valid move {action.coord} by {action.player} after game already ended with {model.outcome.value}"
                )
                continue

            # Check turn
            if action.player != model.current_player:
                errors.append(
                    f"Action {idx}: Move by {action.player} but expected turn is {model.current_player}"
                )
                continue

            # Check coordinate bounds
            if not model.board.is_valid_coord(r, c):
                errors.append(
                    f"Action {idx}: Move coordinate ({r}, {c}) is out of bounds for {model.size}x{model.size} grid without intentional invalid flag"
                )
                continue

            # Check occupied
            if not model.board.is_empty(r, c):
                errors.append(
                    f"Action {idx}: Move coordinate ({r}, {c}) is already occupied by {model.board.get(r, c)} without intentional invalid flag"
                )
                continue

            # Apply move
            legal = model.apply_move(action.player, r, c)
            if not legal:
                errors.append(f"Action {idx}: Unexpected rejection of move ({r}, {c}) by {action.player}")

        return errors
