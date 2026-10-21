#include "mem_tac_review/scenario_library.hpp"

#include <cassert>
#include <iostream>

using namespace mem_tac_review;

void test_canonical_scenarios_validation() {
    ScenarioLibrary lib;
    auto all_scenarios = lib.list_all();
    assert(!all_scenarios.empty());

    for (const auto* sc : all_scenarios) {
        auto val = sc->validate();
        if (!val.ok) {
            std::cerr << "Validation failed for scenario: " << sc->id() << std::endl;
            for (const auto& err : val.errors) {
                std::cerr << "  - " << err << std::endl;
            }
        }
        assert(val.ok);
    }
    std::cout << "Validated " << all_scenarios.size() << " canonical scenarios successfully." << std::endl;
}

void test_suite_filtering() {
    ScenarioLibrary lib;
    auto lab2_scenarios = lib.get_by_suite("lab-2");
    assert(!lab2_scenarios.empty());

    auto p1_scenarios = lib.get_by_suite("p1");
    assert(p1_scenarios.size() >= lab2_scenarios.size());

    // Test alias resolution
    const Scenario* diag = lib.get("diagonal-win");
    assert(diag != nullptr);
    assert(diag->id() == "g07_main_diag");

    const Scenario* draw = lib.get("draw");
    assert(draw != nullptr);
    assert(draw->id() == "g10_draw");
}

void test_accidental_invalid_move_detection() {
    // Scenario with accidental move after game over
    Scenario sc("accidental_win_extra", "Extra move after win", "", 3);
    sc.add_action(ScenarioAction::move(Player::P1, 0, 0));
    sc.add_action(ScenarioAction::move(Player::P2, 1, 0));
    sc.add_action(ScenarioAction::move(Player::P1, 0, 1));
    sc.add_action(ScenarioAction::move(Player::P2, 1, 1));
    sc.add_action(ScenarioAction::move(Player::P1, 0, 2)); // P1 wins here!
    sc.add_action(ScenarioAction::move(Player::P2, 2, 2)); // Accidental extra move!

    auto val = sc.validate();
    assert(!val.ok);
    assert(!val.errors.empty());

    // Scenario with accidental occupied cell move
    Scenario sc2("accidental_occupied", "Occupied cell without flag", "", 3);
    sc2.add_action(ScenarioAction::move(Player::P1, 0, 0));
    sc2.add_action(ScenarioAction::move(Player::P2, 0, 0)); // Occupied!

    auto val2 = sc2.validate();
    assert(!val2.ok);

    // Scenario with accidental out-of-bounds move
    Scenario sc3("accidental_oob", "OOB without flag", "", 3);
    sc3.add_action(ScenarioAction::move(Player::P1, -1, 0)); // OOB!

    auto val3 = sc3.validate();
    assert(!val3.ok);
}

void test_yaml_serialization() {
    Scenario sc("test_yaml", "Test YAML scenario", "A test description", 3);
    sc.add_suite("test_suite");
    sc.add_action(ScenarioAction::move(Player::P1, 0, 0, "P1 move"));
    sc.add_action(ScenarioAction::invalid_coord(Player::P2, -1, 2, "P2 oob"));

    std::string yaml = Scenario::to_yaml_string(sc);
    assert(!yaml.empty());

    Scenario loaded = Scenario::from_yaml_string(yaml);
    assert(loaded.id() == "test_yaml");
    assert(loaded.grid_size() == 3);
    assert(loaded.actions().size() == 2);
    assert(loaded.actions()[0].type == ActionType::MOVE);
    assert(loaded.actions()[1].type == ActionType::INVALID_COORD);
}

int main() {
    test_canonical_scenarios_validation();
    test_suite_filtering();
    test_accidental_invalid_move_detection();
    test_yaml_serialization();

    std::cout << "All Scenario and ScenarioLibrary tests passed!" << std::endl;
    return 0;
}
