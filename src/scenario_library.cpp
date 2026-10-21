#include "mem_tac_review/scenario_library.hpp"

#include <yaml-cpp/yaml.h>
#include <fstream>
#include <set>
#include <iostream>

namespace mem_tac_review {

ScenarioLibrary::ScenarioLibrary() {
    init_canonical_scenarios();
}

void ScenarioLibrary::register_scenario(Scenario scenario) {
    std::string id = scenario.id();
    scenarios_[id] = std::move(scenario);
}

const Scenario* ScenarioLibrary::get(const std::string& id) const {
    auto alias_it = aliases_.find(id);
    std::string target_id = (alias_it != aliases_.end()) ? alias_it->second : id;

    auto it = scenarios_.find(target_id);
    if (it != scenarios_.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<const Scenario*> ScenarioLibrary::get_by_suite(const std::string& suite_name) const {
    std::vector<const Scenario*> result;
    for (const auto& [_, sc] : scenarios_) {
        if (sc.in_suite(suite_name)) {
            result.push_back(&sc);
        }
    }
    return result;
}

std::vector<const Scenario*> ScenarioLibrary::list_all() const {
    std::vector<const Scenario*> result;
    for (const auto& [_, sc] : scenarios_) {
        result.push_back(&sc);
    }
    return result;
}

std::vector<std::string> ScenarioLibrary::list_suites() const {
    std::set<std::string> suites;
    for (const auto& [_, sc] : scenarios_) {
        for (const auto& s : sc.suites()) {
            suites.insert(s);
        }
    }
    return std::vector<std::string>(suites.begin(), suites.end());
}

static Scenario build_game_scenario(
    const std::string& id,
    const std::string& name,
    const std::string& desc,
    const std::vector<std::string>& suites,
    int grid_size,
    const std::vector<std::pair<int, int>>& moves) {

    Scenario sc(id, name, desc, grid_size, Player::P1);
    sc.set_suites(suites);

    Player p = Player::P1;
    for (size_t i = 0; i < moves.size(); ++i) {
        auto [r, c] = moves[i];
        std::string move_desc = "Move " + std::to_string(i + 1) + ": " +
                                player_to_string(p) + " plays (" +
                                std::to_string(r) + "," + std::to_string(c) + ")";
        sc.add_action(ScenarioAction::move(p, r, c, move_desc));
        p = opponent_of(p);
    }
    return sc;
}

void ScenarioLibrary::init_canonical_scenarios() {
    // G01: Top row win
    {
        auto sc = build_game_scenario(
            "g01_top_row", "First player wins on top row",
            "G01: (0,0) (1,0) (0,1) (1,1) (0,2) -> X wins on move 5",
            {"lab-2", "p1"}, 3,
            {{0, 0}, {1, 0}, {0, 1}, {1, 1}, {0, 2}}
        );
        register_scenario(sc);
        aliases_["top-row"] = "g01_top_row";
    }

    // G02: Middle row win
    {
        auto sc = build_game_scenario(
            "g02_mid_row", "First player wins on middle row",
            "G02: (1,0) (0,0) (1,1) (0,1) (1,2) -> X wins on move 5",
            {"p1"}, 3,
            {{1, 0}, {0, 0}, {1, 1}, {0, 1}, {1, 2}}
        );
        register_scenario(sc);
    }

    // G03: Bottom row win
    {
        auto sc = build_game_scenario(
            "g03_bot_row", "First player wins on bottom row",
            "G03: (2,0) (0,0) (2,1) (0,1) (2,2) -> X wins on move 5",
            {"p1"}, 3,
            {{2, 0}, {0, 0}, {2, 1}, {0, 1}, {2, 2}}
        );
        register_scenario(sc);
    }

    // G04: Left column win
    {
        auto sc = build_game_scenario(
            "g04_left_col", "First player wins on left column",
            "G04: (0,0) (0,1) (1,0) (1,1) (2,0) -> X wins on move 5",
            {"p1"}, 3,
            {{0, 0}, {0, 1}, {1, 0}, {1, 1}, {2, 0}}
        );
        register_scenario(sc);
    }

    // G05: Middle column win
    {
        auto sc = build_game_scenario(
            "g05_mid_col", "First player wins on middle column",
            "G05: (0,1) (0,0) (1,1) (1,0) (2,1) -> X wins on move 5",
            {"p1"}, 3,
            {{0, 1}, {0, 0}, {1, 1}, {1, 0}, {2, 1}}
        );
        register_scenario(sc);
    }

    // G06: Right column win
    {
        auto sc = build_game_scenario(
            "g06_right_col", "First player wins on right column",
            "G06: (0,2) (0,0) (1,2) (1,0) (2,2) -> X wins on move 5",
            {"p1"}, 3,
            {{0, 2}, {0, 0}, {1, 2}, {1, 0}, {2, 2}}
        );
        register_scenario(sc);
    }

    // G07: Main diagonal win (diagonal-win)
    {
        auto sc = build_game_scenario(
            "g07_main_diag", "First player wins on main diagonal",
            "G07: (0,0) (0,1) (1,1) (0,2) (2,2) -> X wins on move 5",
            {"lab-2", "p1"}, 3,
            {{0, 0}, {0, 1}, {1, 1}, {0, 2}, {2, 2}}
        );
        register_scenario(sc);
        aliases_["diagonal-win"] = "g07_main_diag";
    }

    // G08: Anti diagonal win
    {
        auto sc = build_game_scenario(
            "g08_anti_diag", "First player wins on anti-diagonal",
            "G08: (0,2) (0,0) (1,1) (0,1) (2,0) -> X wins on move 5",
            {"p1"}, 3,
            {{0, 2}, {0, 0}, {1, 1}, {0, 1}, {2, 0}}
        );
        register_scenario(sc);
    }

    // G09: Second player wins
    {
        auto sc = build_game_scenario(
            "g09_p2_win", "Second player wins",
            "G09: (0,0) (1,1) (1,0) (0,1) (0,2) (2,1) -> O wins on move 6",
            {"lab-2", "p1"}, 3,
            {{0, 0}, {1, 1}, {1, 0}, {0, 1}, {0, 2}, {2, 1}}
        );
        register_scenario(sc);
        aliases_["p2-win"] = "g09_p2_win";
    }

    // G10: Full board draw
    {
        auto sc = build_game_scenario(
            "g10_draw", "Full-board draw",
            "G10: Full 3x3 board without a winner -> Draw on move 9",
            {"lab-2", "p1"}, 3,
            {{0, 0}, {0, 1}, {0, 2}, {1, 2}, {1, 0}, {2, 0}, {1, 1}, {2, 2}, {2, 1}}
        );
        register_scenario(sc);
        aliases_["draw"] = "g10_draw";
    }

    // G11: Win on final cell
    {
        auto sc = build_game_scenario(
            "g11_win_final_cell", "Win on the final empty cell",
            "G11: (0,0) (0,1) (0,2) (1,0) (1,1) (1,2) (2,1) (2,0) (2,2) -> X wins on move 9, not a draw",
            {"p1"}, 3,
            {{0, 0}, {0, 1}, {0, 2}, {1, 0}, {1, 1}, {1, 2}, {2, 1}, {2, 0}, {2, 2}}
        );
        register_scenario(sc);
    }

    // G12: Mixed lines do not win
    {
        auto sc = build_game_scenario(
            "g12_mixed_lines", "Mixed lines do not win",
            "G12: Moves create mixed rows and columns before finishing in a draw",
            {"p1"}, 3,
            {{0, 0}, {0, 1}, {0, 2}, {1, 1}, {1, 0}, {2, 0}, {2, 1}, {1, 2}, {2, 2}}
        );
        register_scenario(sc);
    }

    // N01-4x4: Row win on 4x4
    {
        std::vector<std::pair<int, int>> moves;
        for (int k = 0; k < 3; ++k) {
            moves.push_back({0, k});
            moves.push_back({1, k});
        }
        moves.push_back({0, 3});
        auto sc = build_game_scenario(
            "n01_4x4_row", "4x4 top row win",
            "N01: 4x4 grid, X completes top row on move 7",
            {"lab-2", "p1"}, 4, moves
        );
        register_scenario(sc);
    }

    // N03-4x4: Main diagonal win on 4x4
    {
        std::vector<std::pair<int, int>> moves;
        for (int k = 0; k < 3; ++k) {
            moves.push_back({k, k});
            moves.push_back({k, k + 1});
        }
        moves.push_back({3, 3});
        auto sc = build_game_scenario(
            "n03_4x4_diag", "4x4 main diagonal win",
            "N03: 4x4 grid, X completes main diagonal on move 7 (checkpoint with 3 marks)",
            {"lab-2", "p1"}, 4, moves
        );
        register_scenario(sc);
    }

    // N01-5x5: Row win on 5x5
    {
        std::vector<std::pair<int, int>> moves;
        for (int k = 0; k < 4; ++k) {
            moves.push_back({0, k});
            moves.push_back({1, k});
        }
        moves.push_back({0, 4});
        auto sc = build_game_scenario(
            "n01_5x5_row", "5x5 top row win",
            "N01: 5x5 grid, X completes top row on move 9",
            {"p1"}, 5, moves
        );
        register_scenario(sc);
    }

    // N03-5x5: Main diagonal win on 5x5
    {
        std::vector<std::pair<int, int>> moves;
        for (int k = 0; k < 4; ++k) {
            moves.push_back({k, k});
            moves.push_back({k, k + 1});
        }
        moves.push_back({4, 4});
        auto sc = build_game_scenario(
            "n03_5x5_diag", "5x5 main diagonal win",
            "N03: 5x5 grid, X completes main diagonal on move 9 (checkpoint with 3 and 4 marks)",
            {"p1"}, 5, moves
        );
        register_scenario(sc);
    }

    // E01: CLI missing grid size argument
    {
        Scenario sc("e01_cli_missing_size", "CLI error: missing grid size",
                    "E01: Omits required grid-size argument on P1 invocation", 3);
        sc.set_type(ScenarioType::CLI_ERROR);
        sc.set_suites({"lab-2", "p1"});
        sc.set_cli_args_p1({}); // Empty args override: drops {N} argument
        register_scenario(sc);
    }

    // E02: CLI invalid size argument ("abc")
    {
        Scenario sc("e02_cli_invalid_size", "CLI error: non-numeric grid size",
                    "E02: Supplies non-numeric 'abc' as grid size argument", 3);
        sc.set_type(ScenarioType::CLI_ERROR);
        sc.set_suites({"p1"});
        sc.set_cli_args_p1({"abc"});
        register_scenario(sc);
    }

    // E03: Invalid coordinates (out-of-bounds) followed by valid moves
    {
        Scenario sc("e03_invalid_coords", "Invalid coordinates error and recovery",
                    "E03: Tests negative and out-of-range coordinates on both turns followed by recovery", 3);
        sc.set_suites({"lab-2", "p1"});
        // Out of bounds for P1
        sc.add_action(ScenarioAction::invalid_coord(Player::P1, -1, 0, "Intentional out-of-bounds (-1, 0) for P1"));
        sc.add_action(ScenarioAction::invalid_coord(Player::P1, 3, 0, "Intentional out-of-bounds (3, 0) for P1"));
        // Valid move for P1
        sc.add_action(ScenarioAction::move(Player::P1, 0, 0, "Valid recovery move (0, 0) for P1"));
        // Out of bounds for P2
        sc.add_action(ScenarioAction::invalid_coord(Player::P2, 0, -1, "Intentional out-of-bounds (0, -1) for P2"));
        sc.add_action(ScenarioAction::invalid_coord(Player::P2, 0, 3, "Intentional out-of-bounds (0, 3) for P2"));
        // Valid move for P2
        sc.add_action(ScenarioAction::move(Player::P2, 1, 0, "Valid recovery move (1, 0) for P2"));
        // Remaining moves to finish G01
        sc.add_action(ScenarioAction::move(Player::P1, 0, 1, "P1 move (0, 1)"));
        sc.add_action(ScenarioAction::move(Player::P2, 1, 1, "P2 move (1, 1)"));
        sc.add_action(ScenarioAction::move(Player::P1, 0, 2, "P1 move (0, 2) winning top row"));
        register_scenario(sc);
    }

    // E04: Occupied cells reuse followed by valid moves
    {
        Scenario sc("e04_occupied_cells", "Occupied cell reuse error and recovery",
                    "E04: Attempts to overwrite own mark and opponent mark followed by valid moves", 3);
        sc.set_suites({"lab-2", "p1"});
        sc.add_action(ScenarioAction::move(Player::P1, 0, 0, "P1 plays (0, 0)"));
        // P2 tries to overwrite P1's mark at (0, 0)
        sc.add_action(ScenarioAction::occupied_cell(Player::P2, 0, 0, "P2 attempts to overwrite (0, 0)"));
        // P2 makes valid move at (1, 0)
        sc.add_action(ScenarioAction::move(Player::P2, 1, 0, "P2 valid move at (1, 0)"));
        // P1 tries to overwrite P2's mark at (1, 0)
        sc.add_action(ScenarioAction::occupied_cell(Player::P1, 1, 0, "P1 attempts to overwrite (1, 0)"));
        // P1 tries to overwrite own mark at (0, 0)
        sc.add_action(ScenarioAction::occupied_cell(Player::P1, 0, 0, "P1 attempts to overwrite own (0, 0)"));
        // P1 makes valid move
        sc.add_action(ScenarioAction::move(Player::P1, 0, 1, "P1 valid move (0, 1)"));
        sc.add_action(ScenarioAction::move(Player::P2, 1, 1, "P2 valid move (1, 1)"));
        sc.add_action(ScenarioAction::move(Player::P1, 0, 2, "P1 winning move (0, 2)"));
        register_scenario(sc);
    }

    // E05: Malformed input (non-numeric) and recovery
    {
        Scenario sc("e05_malformed_input", "Malformed non-numeric input",
                    "E05: Delivers 'abc\\n' as move input, followed by recovery with valid move", 3);
        sc.set_suites({"p1"});
        sc.add_action(ScenarioAction::malformed_input(Player::P1, "abc\n", "P1 sends non-numeric 'abc\\n'"));
        sc.add_action(ScenarioAction::move(Player::P1, 0, 0, "P1 recovery move (0, 0)"));
        sc.add_action(ScenarioAction::move(Player::P2, 1, 0, "P2 move (1, 0)"));
        sc.add_action(ScenarioAction::move(Player::P1, 0, 1, "P1 move (0, 1)"));
        sc.add_action(ScenarioAction::move(Player::P2, 1, 1, "P2 move (1, 1)"));
        sc.add_action(ScenarioAction::move(Player::P1, 0, 2, "P1 winning move (0, 2)"));
        register_scenario(sc);
    }

    // E06: Incomplete coordinate input
    {
        Scenario sc("e06_incomplete_input", "Incomplete coordinate input",
                    "E06: Sends single coordinate token or blank line before full coordinate", 3);
        sc.set_suites({"p1"});
        sc.add_action(ScenarioAction::malformed_input(Player::P1, "\n", "P1 sends blank line"));
        sc.add_action(ScenarioAction::move(Player::P1, 0, 0, "P1 valid move (0, 0)"));
        sc.add_action(ScenarioAction::move(Player::P2, 1, 0, "P2 move (1, 0)"));
        sc.add_action(ScenarioAction::move(Player::P1, 0, 1, "P1 move (0, 1)"));
        sc.add_action(ScenarioAction::move(Player::P2, 1, 1, "P2 move (1, 1)"));
        sc.add_action(ScenarioAction::move(Player::P1, 0, 2, "P1 winning move (0, 2)"));
        register_scenario(sc);
    }

    // W02: EOF handling
    {
        Scenario sc("w02_eof", "Standard input EOF handling",
                    "W02: Closes standard input before or during game to observe peer reaction", 3);
        sc.set_suites({"p2"});
        sc.add_action(ScenarioAction::move(Player::P1, 0, 0, "P1 initial move (0, 0)"));
        sc.add_action(ScenarioAction::eof(Player::P2, "P2 receives EOF on standard input"));
        register_scenario(sc);
    }
}

Scenario Scenario::from_yaml_string(const std::string& yaml_str) {
    YAML::Node node = YAML::Load(yaml_str);
    std::string id = node["id"] ? node["id"].as<std::string>() : "unnamed";
    std::string name = node["name"] ? node["name"].as<std::string>() : id;
    std::string desc = node["description"] ? node["description"].as<std::string>() : "";
    int grid_size = node["grid_size"] ? node["grid_size"].as<int>() : 3;
    Player starting_p = (node["starting_player"] && node["starting_player"].as<std::string>() == "p2")
                            ? Player::P2 : Player::P1;

    Scenario sc(id, name, desc, grid_size, starting_p);

    if (node["type"] && node["type"].as<std::string>() == "cli_error") {
        sc.set_type(ScenarioType::CLI_ERROR);
    }

    if (node["suites"]) {
        for (const auto& s : node["suites"]) {
            sc.add_suite(s.as<std::string>());
        }
    }

    if (node["cli_args_p1"]) {
        std::vector<std::string> args;
        for (const auto& a : node["cli_args_p1"]) {
            args.push_back(a.as<std::string>());
        }
        sc.set_cli_args_p1(args);
    }
    if (node["cli_args_p2"]) {
        std::vector<std::string> args;
        for (const auto& a : node["cli_args_p2"]) {
            args.push_back(a.as<std::string>());
        }
        sc.set_cli_args_p2(args);
    }

    if (node["actions"]) {
        for (const auto& a_node : node["actions"]) {
            std::string type_str = a_node["type"] ? a_node["type"].as<std::string>() : "move";
            std::transform(type_str.begin(), type_str.end(), type_str.begin(), [](unsigned char c) { return std::tolower(c); });
            Player p = (a_node["player"] && a_node["player"].as<std::string>() == "p2")
                           ? Player::P2 : Player::P1;
            std::string desc_act = a_node["description"] ? a_node["description"].as<std::string>() : "";

            if (type_str == "move") {
                int r = a_node["r"].as<int>();
                int c = a_node["c"].as<int>();
                sc.add_action(ScenarioAction::move(p, r, c, desc_act));
            } else if (type_str == "invalid_coord") {
                int r = a_node["r"].as<int>();
                int c = a_node["c"].as<int>();
                sc.add_action(ScenarioAction::invalid_coord(p, r, c, desc_act));
            } else if (type_str == "occupied_cell") {
                int r = a_node["r"].as<int>();
                int c = a_node["c"].as<int>();
                sc.add_action(ScenarioAction::occupied_cell(p, r, c, desc_act));
            } else if (type_str == "malformed_input") {
                std::string input = a_node["input"].as<std::string>();
                sc.add_action(ScenarioAction::malformed_input(p, input, desc_act));
            } else if (type_str == "eof") {
                sc.add_action(ScenarioAction::eof(p, desc_act));
            } else if (type_str == "pause") {
                int ms = a_node["ms"].as<int>();
                sc.add_action(ScenarioAction::pause(ms, desc_act));
            }
        }
    }

    return sc;
}

std::string Scenario::to_yaml_string(const Scenario& sc) {
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "id" << YAML::Value << sc.id();
    out << YAML::Key << "name" << YAML::Value << sc.name();
    out << YAML::Key << "description" << YAML::Value << sc.description();
    out << YAML::Key << "grid_size" << YAML::Value << sc.grid_size();
    out << YAML::Key << "starting_player" << YAML::Value << (sc.starting_player() == Player::P1 ? "p1" : "p2");
    out << YAML::Key << "type" << YAML::Value << (sc.type() == ScenarioType::GAME ? "game" : "cli_error");

    out << YAML::Key << "suites" << YAML::Value << YAML::BeginSeq;
    for (const auto& s : sc.suites()) {
        out << s;
    }
    out << YAML::EndSeq;

    out << YAML::Key << "actions" << YAML::Value << YAML::BeginSeq;
    for (const auto& a : sc.actions()) {
        out << YAML::BeginMap;
        out << YAML::Key << "type" << YAML::Value << action_type_to_string(a.type);
        out << YAML::Key << "player" << YAML::Value << (a.target_player == Player::P1 ? "p1" : "p2");
        if (a.type == ActionType::MOVE || a.type == ActionType::INVALID_COORD || a.type == ActionType::OCCUPIED_CELL) {
            out << YAML::Key << "r" << YAML::Value << a.row;
            out << YAML::Key << "c" << YAML::Value << a.col;
        } else if (a.type == ActionType::MALFORMED_INPUT) {
            out << YAML::Key << "input" << YAML::Value << a.raw_input;
        } else if (a.type == ActionType::PAUSE) {
            out << YAML::Key << "ms" << YAML::Value << a.pause_ms;
        }
        if (!a.description.empty()) {
            out << YAML::Key << "description" << YAML::Value << a.description;
        }
        out << YAML::EndMap;
    }
    out << YAML::EndSeq;

    out << YAML::EndMap;
    return out.c_str();
}

bool ScenarioLibrary::load_file(const std::filesystem::path& path, std::string* error_msg) {
    try {
        std::ifstream fin(path);
        if (!fin.is_open()) {
            if (error_msg) *error_msg = "Could not open file: " + path.string();
            return false;
        }
        std::stringstream buffer;
        buffer << fin.rdbuf();
        Scenario sc = Scenario::from_yaml_string(buffer.str());
        auto val = sc.validate();
        if (!val.ok) {
            if (error_msg) {
                *error_msg = "Scenario validation failed for " + path.string() + ": ";
                for (const auto& e : val.errors) *error_msg += e + "; ";
            }
            return false;
        }
        register_scenario(sc);
        return true;
    } catch (const std::exception& e) {
        if (error_msg) *error_msg = e.what();
        return false;
    }
}

size_t ScenarioLibrary::load_directory(const std::filesystem::path& dir, std::string* error_msg) {
    size_t count = 0;
    try {
        if (!std::filesystem::exists(dir) || !std::filesystem::is_directory(dir)) {
            if (error_msg) *error_msg = "Directory does not exist: " + dir.string();
            return 0;
        }
        for (const auto& entry : std::filesystem::directory_iterator(dir)) {
            if (entry.is_regular_file()) {
                auto ext = entry.path().extension();
                if (ext == ".yaml" || ext == ".yml" || ext == ".json") {
                    std::string err;
                    if (load_file(entry.path(), &err)) {
                        count++;
                    } else if (error_msg) {
                        *error_msg += err + "\n";
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        if (error_msg) *error_msg = e.what();
    }
    return count;
}

bool ScenarioLibrary::save_file(const Scenario& sc, const std::filesystem::path& path, std::string* error_msg) const {
    try {
        std::ofstream fout(path);
        if (!fout.is_open()) {
            if (error_msg) *error_msg = "Could not open file for writing: " + path.string();
            return false;
        }
        fout << Scenario::to_yaml_string(sc);
        return true;
    } catch (const std::exception& e) {
        if (error_msg) *error_msg = e.what();
        return false;
    }
}

} // namespace mem_tac_review
