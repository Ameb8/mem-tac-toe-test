#include "mem_tac_review/reference_model.hpp"

#include <sstream>
#include <stdexcept>
#include <algorithm>

namespace mem_tac_review {

ReferenceModel::ReferenceModel(int grid_size, Player starting_player)
    : grid_size_(grid_size),
      starting_player_(starting_player),
      current_player_(starting_player),
      outcome_(GameOutcome::IN_PROGRESS),
      move_count_(0),
      board_(grid_size > 0 ? grid_size * grid_size : 0, '.') {
    if (grid_size_ <= 0) {
        throw std::invalid_argument("Grid size must be positive");
    }
}

void ReferenceModel::reset() {
    current_player_ = starting_player_;
    outcome_ = GameOutcome::IN_PROGRESS;
    move_count_ = 0;
    std::fill(board_.begin(), board_.end(), '.');
    move_history_.clear();
}

char ReferenceModel::cell(int r, int c) const {
    if (r < 0 || r >= grid_size_ || c < 0 || c >= grid_size_) {
        return '?';
    }
    return board_[r * grid_size_ + c];
}

bool ReferenceModel::is_valid_move(Player p, int r, int c) const {
    if (outcome_ != GameOutcome::IN_PROGRESS) {
        return false;
    }
    if (p != current_player_) {
        return false;
    }
    if (r < 0 || r >= grid_size_ || c < 0 || c >= grid_size_) {
        return false;
    }
    if (board_[r * grid_size_ + c] != '.') {
        return false;
    }
    return true;
}

bool ReferenceModel::apply_move(Player p, int r, int c) {
    if (!is_valid_move(p, r, c)) {
        move_history_.push_back({p, r, c, false});
        return false;
    }

    board_[r * grid_size_ + c] = player_to_mark(p);
    move_count_++;
    move_history_.push_back({p, r, c, true});

    evaluate_outcome();

    if (outcome_ == GameOutcome::IN_PROGRESS) {
        current_player_ = opponent_of(current_player_);
    }

    return true;
}

void ReferenceModel::evaluate_outcome() {
    auto check_line = [this](char mark, auto cell_getter) -> bool {
        for (int i = 0; i < grid_size_; ++i) {
            if (cell_getter(i) != mark) {
                return false;
            }
        }
        return true;
    };

    auto check_winner = [this, &check_line](char mark) -> bool {
        // Rows
        for (int r = 0; r < grid_size_; ++r) {
            if (check_line(mark, [this, r](int c) { return board_[r * grid_size_ + c]; })) {
                return true;
            }
        }
        // Columns
        for (int c = 0; c < grid_size_; ++c) {
            if (check_line(mark, [this, c](int r) { return board_[r * grid_size_ + c]; })) {
                return true;
            }
        }
        // Main diagonal
        if (check_line(mark, [this](int i) { return board_[i * grid_size_ + i]; })) {
            return true;
        }
        // Anti diagonal
        if (check_line(mark, [this](int i) { return board_[i * grid_size_ + (grid_size_ - 1 - i)]; })) {
            return true;
        }
        return false;
    };

    if (check_winner('X')) {
        outcome_ = GameOutcome::P1_WINS;
        return;
    }
    if (check_winner('O')) {
        outcome_ = GameOutcome::P2_WINS;
        return;
    }

    // Check for full board
    bool full = true;
    for (char ch : board_) {
        if (ch == '.') {
            full = false;
            break;
        }
    }

    if (full) {
        outcome_ = GameOutcome::DRAW;
    } else {
        outcome_ = GameOutcome::IN_PROGRESS;
    }
}

std::string ReferenceModel::format_board() const {
    std::ostringstream oss;
    for (int r = 0; r < grid_size_; ++r) {
        for (int c = 0; c < grid_size_; ++c) {
            if (c > 0) oss << ' ';
            oss << board_[r * grid_size_ + c];
        }
        if (r + 1 < grid_size_) {
            oss << '\n';
        }
    }
    return oss.str();
}

std::string ReferenceModel::format_board_compact() const {
    std::ostringstream oss;
    for (int r = 0; r < grid_size_; ++r) {
        if (r > 0) oss << " / ";
        for (int c = 0; c < grid_size_; ++c) {
            oss << board_[r * grid_size_ + c];
        }
    }
    return oss.str();
}

std::string ReferenceModel::format_summary() const {
    std::ostringstream oss;
    oss << format_board() << "\n\n";
    if (outcome_ == GameOutcome::IN_PROGRESS) {
        oss << "Expected next player: " << player_to_mark(current_player_) << "\n";
    }
    oss << "Expected outcome: " << outcome_to_string(outcome_);
    return oss.str();
}

} // namespace mem_tac_review
