"""Unit tests for submission profile loading and adapter formatting."""

import pytest

from mem_tac_review.models import Action, ActionType, Coordinate, Profile
from mem_tac_review.profiles import (
    ProfileAdapter,
    ProfileError,
    load_profile_from_file,
    parse_profile_dict,
)


def test_parse_valid_profile_dict() -> None:
    data = {
        "name": "test_submission",
        "build_cmd": "make clean && make",
        "work_dir": ".",
        "topology": "dual",
        "launch_p1": ["./p1", "{N}"],
        "launch_p2": ["./p2"],
        "creator": "p1",
        "coord_base": 0,
        "coord_order": "row_col",
        "input_format": "{r} {c}\n",
        "turn_prompt": "Enter move:",
        "settling_ms": 150,
        "timeout_sec": 5.0,
    }
    p = parse_profile_dict(data)
    assert p.name == "test_submission"
    assert p.topology == "dual"
    assert p.coord_base == 0
    assert p.settling_ms == 150


def test_parse_invalid_profile_dict() -> None:
    with pytest.raises(ProfileError, match="name"):
        parse_profile_dict({})

    with pytest.raises(ProfileError, match="topology"):
        parse_profile_dict({"name": "test", "topology": "triple"})

    with pytest.raises(ProfileError, match="coord_base"):
        parse_profile_dict({"name": "test", "coord_base": 2})


def test_load_profile_from_yaml(tmp_path) -> None:
    yaml_content = """
name: student_yaml
topology: single
launch_p1: ["./game", "{N}"]
coord_base: 1
coord_order: "col_row"
input_format: "{r},{c}\\n"
turn_prompt: "Move:"
"""
    f = tmp_path / "profile.yaml"
    f.write_text(yaml_content, encoding="utf-8")

    profile = load_profile_from_file(f)
    assert profile.name == "student_yaml"
    assert profile.topology == "single"
    assert profile.coord_base == 1
    assert profile.coord_order == "col_row"
    assert profile.input_format == "{r},{c}\n"


def test_profile_adapter_coord_and_input_translation() -> None:
    # 0-base profile
    p0 = Profile(
        name="p0",
        coord_base=0,
        input_format="{r} {c}\n",
    )
    adapter0 = ProfileAdapter(p0)
    act = Action(ActionType.MOVE, "X", Coordinate(1, 2))
    inputs0 = adapter0.format_action_inputs(act)
    assert inputs0 == ["1 2\n"]

    # 1-base profile
    p1 = Profile(
        name="p1",
        coord_base=1,
        input_format="{r},{c}\n",
    )
    adapter1 = ProfileAdapter(p1)
    inputs1 = adapter1.format_action_inputs(act)
    assert inputs1 == ["2,3\n"]

    # multi-prompt profile
    pm = Profile(
        name="pm",
        coord_base=0,
        input_format=["{r}\n", "{c}\n"],
    )
    adapter_m = ProfileAdapter(pm)
    inputs_m = adapter_m.format_action_inputs(act)
    assert inputs_m == ["1\n", "2\n"]


def test_profile_adapter_launch_args() -> None:
    p = Profile(
        name="p",
        launch_p1=["./p1", "-size", "{N}"],
        launch_p2=["./p2"],
        creator="p1",
    )
    adapter = ProfileAdapter(p)
    args_p1 = adapter.format_launch_args("p1", 4)
    assert args_p1 == ["./p1", "-size", "4"]

    args_p2 = adapter.format_launch_args("p2", 4)
    assert args_p2 == ["./p2"]

    # CLI override on creator
    override_args = adapter.format_launch_args("p1", 4, cli_args_override=["bad_arg"])
    assert override_args == ["./p1", "bad_arg"]


def test_profile_adapter_terminal_target() -> None:
    # Dual topology
    p_dual = Profile(name="dual", topology="dual")
    adapter_dual = ProfileAdapter(p_dual)
    assert adapter_dual.resolve_target_terminal("X") == "p1"
    assert adapter_dual.resolve_target_terminal("O") == "p2"

    # Single topology
    p_single = Profile(name="single", topology="single")
    adapter_single = ProfileAdapter(p_single)
    assert adapter_single.resolve_target_terminal("X") == "p1"
    assert adapter_single.resolve_target_terminal("O") == "p1"
