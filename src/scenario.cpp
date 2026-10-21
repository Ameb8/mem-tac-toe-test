#include "mem_tac_review/scenario.hpp"

#include <algorithm>
#include <sstream>

namespace mem_tac_review {

Scenario::Scenario(std::string id, std::string name, std::string desc, int grid_size, Player starting_player)
    : id_(std::move(id)),
      name_(std::move(name)),
      description_(std::move(desc)),
      grid_size_(grid_size),
      starting_player_(starting_player) {}

bool Scenario::in_suite(const std::string& suite) const {
    if (suite == "all" || suite == "*") return true;
    for (const auto& s : suites_) {
        if (s == suite) return true;
    }
    return false;
}

ValidationResult Scenario::validate() const {
    ValidationResult res;

    if (id_.empty()) {
        res.add_error("Scenario ID cannot be empty");
    }
    if (grid_size_ <= 0) {
        res.add_error("Grid size must be greater than 0");
    }

    if (type_ == ScenarioType::CLI_ERROR) {
        // CLI error scenarios don't require move validation
        return res;
    }

    ReferenceModel model(grid_size_, starting_player_);

    for (size_t i = 0; i < actions_.size(); ++i) {
        const auto& act = actions_[i];
        std::string act_prefix = "Action " + std::to_string(i + 1) + ": ";

        switch (act.type) {
            case ActionType::MOVE: {
                if (model.is_game_over() && !act.is_intentional_invalid) {
                    res.add_error(act_prefix + "Move attempted after game is already over (outcome: " +
                                  outcome_to_string(model.outcome()) + ")");
                    break;
                }
                if (model.current_player() != act.target_player && !act.is_intentional_invalid) {
                    res.add_error(act_prefix + "Move sent for " + player_to_string(act.target_player) +
                                  " but current turn belongs to " + player_to_string(model.current_player()));
                    break;
                }
                if (!model.is_valid_move(act.target_player, act.row, act.col)) {
                    if (!act.is_intentional_invalid) {
                        res.add_error(act_prefix + "Accidental invalid move (" + std::to_string(act.row) +
                                      "," + std::to_string(act.col) + ") for player " +
                                      player_to_string(act.target_player) + " - cell occupied or out of bounds");
                        break;
                    }
                } else {
                    model.apply_move(act.target_player, act.row, act.col);
                }
                break;
            }
            case ActionType::INVALID_COORD: {
                // Must be marked intentional or out-of-bounds
                if (act.row >= 0 && act.row < grid_size_ && act.col >= 0 && act.col < grid_size_) {
                    res.add_error(act_prefix + "INVALID_COORD specified but (" + std::to_string(act.row) +
                                  "," + std::to_string(act.col) + ") is inside the grid boundaries");
                }
                break;
            }
            case ActionType::OCCUPIED_CELL: {
                if (act.row < 0 || act.row >= grid_size_ || act.col < 0 || act.col >= grid_size_) {
                    res.add_error(act_prefix + "OCCUPIED_CELL specified but coordinates are out of bounds");
                } else if (model.cell(act.row, act.col) == '.') {
                    res.add_error(act_prefix + "OCCUPIED_CELL specified for cell (" +
                                  std::to_string(act.row) + "," + std::to_string(act.col) +
                                  ") which is currently empty");
                }
                break;
            }
            case ActionType::MALFORMED_INPUT:
            case ActionType::EOF_INPUT:
            case ActionType::PAUSE:
                break;
        }
    }

    return res;
}

} // namespace mem_tac_review
