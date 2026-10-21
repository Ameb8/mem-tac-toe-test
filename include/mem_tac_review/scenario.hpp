#pragma once

#include "mem_tac_review/reference_model.hpp"

#include <string>
#include <vector>
#include <optional>
#include <memory>

namespace mem_tac_review {

enum class ActionType {
    MOVE,             // Standard valid logical (row, col)
    INVALID_COORD,    // Deliberately out of bounds (e.g. (-1, 0), (N, 0))
    OCCUPIED_CELL,    // Deliberately re-using an already marked cell
    MALFORMED_INPUT,  // Malformed raw text (e.g. "abc\n", "1\n")
    EOF_INPUT,        // Close standard input for target player
    PAUSE             // Wait a specified duration
};

inline std::string action_type_to_string(ActionType t) {
    switch (t) {
        case ActionType::MOVE: return "MOVE";
        case ActionType::INVALID_COORD: return "INVALID_COORD";
        case ActionType::OCCUPIED_CELL: return "OCCUPIED_CELL";
        case ActionType::MALFORMED_INPUT: return "MALFORMED_INPUT";
        case ActionType::EOF_INPUT: return "EOF";
        case ActionType::PAUSE: return "PAUSE";
    }
    return "UNKNOWN";
}

struct ScenarioAction {
    ActionType type = ActionType::MOVE;
    Player target_player = Player::P1;
    int row = 0;
    int col = 0;
    std::string raw_input = "";         // Raw string override if non-empty
    int pause_ms = 0;                  // Used when type == PAUSE
    bool is_intentional_invalid = false;
    std::string description = "";

    static ScenarioAction move(Player p, int r, int c, const std::string& desc = "") {
        ScenarioAction a;
        a.type = ActionType::MOVE;
        a.target_player = p;
        a.row = r;
        a.col = c;
        a.description = desc;
        return a;
    }

    static ScenarioAction invalid_coord(Player p, int r, int c, const std::string& desc = "") {
        ScenarioAction a;
        a.type = ActionType::INVALID_COORD;
        a.target_player = p;
        a.row = r;
        a.col = c;
        a.is_intentional_invalid = true;
        a.description = desc;
        return a;
    }

    static ScenarioAction occupied_cell(Player p, int r, int c, const std::string& desc = "") {
        ScenarioAction a;
        a.type = ActionType::OCCUPIED_CELL;
        a.target_player = p;
        a.row = r;
        a.col = c;
        a.is_intentional_invalid = true;
        a.description = desc;
        return a;
    }

    static ScenarioAction malformed_input(Player p, const std::string& text, const std::string& desc = "") {
        ScenarioAction a;
        a.type = ActionType::MALFORMED_INPUT;
        a.target_player = p;
        a.raw_input = text;
        a.is_intentional_invalid = true;
        a.description = desc;
        return a;
    }

    static ScenarioAction eof(Player p, const std::string& desc = "") {
        ScenarioAction a;
        a.type = ActionType::EOF_INPUT;
        a.target_player = p;
        a.description = desc;
        return a;
    }

    static ScenarioAction pause(int ms, const std::string& desc = "") {
        ScenarioAction a;
        a.type = ActionType::PAUSE;
        a.pause_ms = ms;
        a.description = desc;
        return a;
    }
};

enum class ScenarioType {
    GAME,
    CLI_ERROR
};

inline std::string scenario_type_to_string(ScenarioType t) {
    return (t == ScenarioType::GAME) ? "GAME" : "CLI_ERROR";
}

struct ValidationResult {
    bool ok = true;
    std::vector<std::string> errors;

    void add_error(const std::string& err) {
        ok = false;
        errors.push_back(err);
    }
};

class Scenario {
public:
    Scenario() = default;
    Scenario(std::string id, std::string name, std::string desc, int grid_size = 3, Player starting_player = Player::P1);

    const std::string& id() const noexcept { return id_; }
    const std::string& name() const noexcept { return name_; }
    const std::string& description() const noexcept { return description_; }
    int grid_size() const noexcept { return grid_size_; }
    Player starting_player() const noexcept { return starting_player_; }
    ScenarioType type() const noexcept { return type_; }
    const std::vector<std::string>& suites() const noexcept { return suites_; }
    const std::vector<ScenarioAction>& actions() const noexcept { return actions_; }
    const std::vector<std::string>& cli_args_p1() const noexcept { return cli_args_p1_; }
    const std::vector<std::string>& cli_args_p2() const noexcept { return cli_args_p2_; }

    void set_type(ScenarioType t) { type_ = t; }
    void add_suite(const std::string& suite) { suites_.push_back(suite); }
    void set_suites(const std::vector<std::string>& suites) { suites_ = suites; }
    void add_action(const ScenarioAction& action) { actions_.push_back(action); }
    void set_cli_args_p1(const std::vector<std::string>& args) { cli_args_p1_ = args; }
    void set_cli_args_p2(const std::vector<std::string>& args) { cli_args_p2_ = args; }

    bool in_suite(const std::string& suite) const;

    // Validates scenario consistency against the reference game rules
    ValidationResult validate() const;

    static Scenario from_yaml_string(const std::string& yaml_str);
    static std::string to_yaml_string(const Scenario& scenario);

private:
    std::string id_;
    std::string name_;
    std::string description_;
    int grid_size_ = 3;
    Player starting_player_ = Player::P1;
    ScenarioType type_ = ScenarioType::GAME;
    std::vector<std::string> suites_;
    std::vector<ScenarioAction> actions_;
    std::vector<std::string> cli_args_p1_;
    std::vector<std::string> cli_args_p2_;
};

} // namespace mem_tac_review
