"""Unit tests for Scenario Registry and Suites."""

import pytest

from mem_tac_review.models import ActionType
from mem_tac_review.scenarios import ScenarioRegistry, load_scenario_from_file


def test_registry_initialization() -> None:
    registry = ScenarioRegistry()
    scenarios = registry.list_scenarios()
    assert len(scenarios) >= 20

    # Check G01 through G12 exist
    for i in range(1, 13):
        sid = f"G{i:02d}"
        s = registry.get(sid)
        assert s is not None
        assert s.grid_size == 3
        assert len(s.actions) > 0

    # Check suites exist
    suites = registry.list_suites()
    assert "lab-2" in suites
    assert "games-3x3" in suites
    assert "games-nxn" in suites
    assert "errors" in suites
    assert "all" in suites


def test_lab2_suite_scenarios() -> None:
    registry = ScenarioRegistry()
    lab2_scenarios = registry.get_suite("lab-2")
    assert len(lab2_scenarios) > 0
    ids = [s.id for s in lab2_scenarios]
    assert "G01" in ids
    assert "G07" in ids
    assert "G10" in ids
    assert "N01_4" in ids
    assert "E01" in ids


def test_nxn_scenarios() -> None:
    registry = ScenarioRegistry()
    s4 = registry.get("N01_4")
    assert s4 is not None
    assert s4.grid_size == 4
    # moves for 4x4 row win: (4-1)*2 + 1 = 7 actions
    assert len(s4.actions) == 7

    s5 = registry.get("N03_5")
    assert s5 is not None
    assert s5.grid_size == 5
    # moves for 5x5 diagonal win: (5-1)*2 + 1 = 9 actions
    assert len(s5.actions) == 9


def test_load_scenario_from_yaml(tmp_path) -> None:
    yaml_content = """
id: custom1
name: "Custom Test"
description: "A custom test scenario"
grid_size: 3
starting_player: "X"
actions:
  - type: move
    player: "X"
    coord: [0, 0]
    description: "Move 1"
  - type: raw_input
    player: "O"
    raw_text: "bad\\n"
    is_intentional_invalid: true
"""
    f = tmp_path / "custom.yaml"
    f.write_text(yaml_content, encoding="utf-8")

    scen = load_scenario_from_file(f)
    assert scen.id == "custom1"
    assert scen.grid_size == 3
    assert len(scen.actions) == 2
    assert scen.actions[0].type == ActionType.MOVE
    assert scen.actions[0].coord.row == 0
    assert scen.actions[0].coord.col == 0
    assert scen.actions[1].type == ActionType.RAW_INPUT
    assert scen.actions[1].is_intentional_invalid is True
