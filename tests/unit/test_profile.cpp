#include "mem_tac_review/profile.hpp"

#include <cassert>
#include <iostream>

using namespace mem_tac_review;

void test_profile_parsing() {
    std::string yaml = R"(
name: "student_submission_a"
build_cmd: "make clean && make"
work_dir: "./submission"
topology: "dual"
launch_p1: ["./player1", "{N}"]
launch_p2: ["./player2"]
creator: "p1"
coord_base: 1
coord_order: "col_row"
input_format: "{r},{c}\n"
turn_prompt: "Enter move:"
settling_ms: 150
timeout_sec: 12
session_timeout_sec: 90
)";

    Profile p = Profile::from_yaml_string(yaml);
    auto val = p.validate();
    assert(val.ok);

    assert(p.name() == "student_submission_a");
    assert(p.build_cmd() == "make clean && make");
    assert(p.work_dir() == "./submission");
    assert(p.topology() == Topology::DUAL);
    assert(p.launch_p1().size() == 2);
    assert(p.launch_p1()[1] == "{N}");
    assert(p.creator() == CreatorRole::P1);
    assert(p.coord_base() == 1);
    assert(p.coord_order() == CoordOrder::COL_ROW);
    assert(p.turn_prompt() == "Enter move:");
    assert(p.settling_ms() == 150);
    assert(p.timeout_sec() == 12);
    assert(p.session_timeout_sec() == 90);
}

void test_coordinate_translation() {
    // Case 1: 0-based, row_col, single line "{r} {c}\n"
    {
        Profile p;
        p.set_coord_base(0);
        p.set_coord_order(CoordOrder::ROW_COL);
        p.set_input_format("{r} {c}\n");

        auto inputs = p.format_move_inputs(1, 2);
        assert(inputs.size() == 1);
        assert(inputs[0] == "1 2\n");
    }

    // Case 2: 1-based, row_col, "{r},{c}\n"
    {
        Profile p;
        p.set_coord_base(1);
        p.set_coord_order(CoordOrder::ROW_COL);
        p.set_input_format("{r},{c}\n");

        auto inputs = p.format_move_inputs(0, 0);
        assert(inputs.size() == 1);
        assert(inputs[0] == "1,1\n");

        auto inputs2 = p.format_move_inputs(2, 1);
        assert(inputs2[0] == "3,2\n");
    }

    // Case 3: 1-based, col_row, multi-prompt ["{c}\n", "{r}\n"] or ["{first}\n", "{second}\n"]
    {
        Profile p;
        p.set_coord_base(1);
        p.set_coord_order(CoordOrder::COL_ROW);
        p.set_input_format_prompts({"{first}\n", "{second}\n"});

        // Logical (row=1, col=2) -> 1-based row=2, col=3.
        // col_row: first is col (3), second is row (2)
        auto inputs = p.format_move_inputs(1, 2);
        assert(inputs.size() == 2);
        assert(inputs[0] == "3\n");
        assert(inputs[1] == "2\n");
    }
}

void test_launch_args_substitution() {
    Profile p;
    p.set_launch_p1({"./player1", "-n", "{N}", "--verbose"});
    auto args = p.format_launch_args(p.launch_p1(), 4);
    assert(args.size() == 4);
    assert(args[0] == "./player1");
    assert(args[1] == "-n");
    assert(args[2] == "4");
    assert(args[3] == "--verbose");
}

void test_single_topology() {
    std::string yaml = R"(
name: "fork_submission"
topology: "single"
launch_p1: ["./launcher", "{N}"]
creator: "shared"
)";

    Profile p = Profile::from_yaml_string(yaml);
    auto val = p.validate();
    assert(val.ok);
    assert(p.topology() == Topology::SINGLE);
    assert(p.creator() == CreatorRole::SHARED);
}

int main() {
    test_profile_parsing();
    test_coordinate_translation();
    test_launch_args_substitution();
    test_single_topology();

    std::cout << "All Profile tests passed!" << std::endl;
    return 0;
}
