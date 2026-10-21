"""Unit tests for ReferenceModel and scenario validation."""

import pytest

from mem_tac_review.models import (
    Action,
    ActionType,
    Coordinate,
    GameOutcome,
    PlayerMark,
    Scenario,
)
from mem_tac_review.reference import Board, ReferenceModel


def test_board_initialization_and_display() -> None:
    board = Board(3)
    assert not board.is_full()
    assert board.is_empty(0, 0)
    assert board.to_display_string() == ". . .\n. . .\n. . ."

    board.set(0, 0, "X")
    board.set(1, 1, "O")
    assert board.get(0, 0) == "X"
    assert board.get(1, 1) == "O"
    assert not board.is_empty(0, 0)
    assert board.to_display_string() == "X . .\n. O .\n. . ."


def test_reference_model_row_win() -> None:
    # Top row win: (0,0)X, (1,0)O, (0,1)X, (1,1)O, (0,2)X
    model = ReferenceModel(3, "X")
    assert model.apply_move("X", 0, 0)
    assert model.apply_move("O", 1, 0)
    assert model.apply_move("X", 0, 1)
    assert model.apply_move("O", 1, 1)
    assert model.outcome == GameOutcome.IN_PROGRESS
    assert model.apply_move("X", 0, 2)
    assert model.outcome == GameOutcome.X_WINS
    assert model.winning_line == "row 0"

    # Further move should fail
    assert not model.apply_move("O", 2, 2)


def test_reference_model_col_win() -> None:
    # Left column win: (0,0)X, (0,1)O, (1,0)X, (1,1)O, (2,0)X
    model = ReferenceModel(3, "X")
    model.apply_move("X", 0, 0)
    model.apply_move("O", 0, 1)
    model.apply_move("X", 1, 0)
    model.apply_move("O", 1, 1)
    model.apply_move("X", 2, 0)
    assert model.outcome == GameOutcome.X_WINS
    assert model.winning_line == "column 0"


def test_reference_model_diagonals() -> None:
    # Main diagonal: (0,0)X, (0,1)O, (1,1)X, (0,2)O, (2,2)X
    model = ReferenceModel(3, "X")
    model.apply_move("X", 0, 0)
    model.apply_move("O", 0, 1)
    model.apply_move("X", 1, 1)
    model.apply_move("O", 0, 2)
    model.apply_move("X", 2, 2)
    assert model.outcome == GameOutcome.X_WINS
    assert model.winning_line == "main diagonal"

    # Other diagonal: (0,2)X, (0,0)O, (1,1)X, (0,1)O, (2,0)X
    model2 = ReferenceModel(3, "X")
    model2.apply_move("X", 0, 2)
    model2.apply_move("O", 0, 0)
    model2.apply_move("X", 1, 1)
    model2.apply_move("O", 0, 1)
    model2.apply_move("X", 2, 0)
    assert model2.outcome == GameOutcome.X_WINS
    assert model2.winning_line == "other diagonal"


def test_reference_model_draw() -> None:
    # G10: (0,0)X (0,1)O (0,2)X (1,2)O (1,0)X (2,0)O (1,1)X (2,2)O (2,1)X
    # Board:
    # X O X
    # X X O
    # O X O
    moves = [
        ("X", 0, 0), ("O", 0, 1), ("X", 0, 2),
        ("O", 1, 2), ("X", 1, 0), ("O", 2, 0),
        ("X", 1, 1), ("O", 2, 2), ("X", 2, 1),
    ]
    model = ReferenceModel(3, "X")
    for player, r, c in moves:
        assert model.apply_move(player, r, c)
    assert model.outcome == GameOutcome.DRAW


def test_reference_model_o_wins() -> None:
    # G09: (0,0)X (1,1)O (1,0)X (0,1)O (0,2)X (2,1)O
    # Board:
    # X O X
    # X O .
    # . O .
    moves = [
        ("X", 0, 0), ("O", 1, 1), ("X", 1, 0),
        ("O", 0, 1), ("X", 0, 2), ("O", 2, 1),
    ]
    model = ReferenceModel(3, "X")
    for player, r, c in moves:
        assert model.apply_move(player, r, c)
    assert model.outcome == GameOutcome.O_WINS
    assert model.winning_line == "column 1"


def test_larger_grid_nxn() -> None:
    # 4x4 requires 4 in a row
    model = ReferenceModel(4, "X")
    for k in range(3):
        assert model.apply_move("X", 0, k)
        assert model.apply_move("O", 1, k)
    # 3 in a row for X is not a win on 4x4!
    assert model.outcome == GameOutcome.IN_PROGRESS
    assert model.apply_move("X", 0, 3)
    assert model.outcome == GameOutcome.X_WINS
    assert model.winning_line == "row 0"


def test_scenario_validation_errors() -> None:
    # Accidental move to occupied cell
    bad_scenario = Scenario(
        id="bad1",
        name="Bad occupied",
        description="",
        grid_size=3,
        actions=[
            Action(ActionType.MOVE, "X", Coordinate(0, 0)),
            Action(ActionType.MOVE, "O", Coordinate(0, 0)),  # occupied!
        ]
    )
    errors = ReferenceModel.validate_scenario(bad_scenario)
    assert len(errors) == 1
    assert "already occupied" in errors[0]

    # Intentional occupied cell should NOT trigger validation error
    good_invalid_scenario = Scenario(
        id="good_invalid",
        name="Intentional invalid",
        description="",
        grid_size=3,
        actions=[
            Action(ActionType.MOVE, "X", Coordinate(0, 0)),
            Action(ActionType.MOVE, "O", Coordinate(0, 0), is_intentional_invalid=True),
            Action(ActionType.MOVE, "O", Coordinate(1, 1)),
        ]
    )
    assert len(ReferenceModel.validate_scenario(good_invalid_scenario)) == 0


def test_scenario_validation_move_after_win() -> None:
    # Win on move 5, extra move on move 6
    scenario = Scenario(
        id="post_win",
        name="Post win",
        description="",
        grid_size=3,
        actions=[
            Action(ActionType.MOVE, "X", Coordinate(0, 0)),
            Action(ActionType.MOVE, "O", Coordinate(1, 0)),
            Action(ActionType.MOVE, "X", Coordinate(0, 1)),
            Action(ActionType.MOVE, "O", Coordinate(1, 1)),
            Action(ActionType.MOVE, "X", Coordinate(0, 2)),  # Win!
            Action(ActionType.MOVE, "O", Coordinate(2, 2)),  # After win!
        ]
    )
    errors = ReferenceModel.validate_scenario(scenario)
    assert len(errors) == 1
    assert "after game already ended" in errors[0]
