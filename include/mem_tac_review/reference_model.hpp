#pragma once

#include <string>
#include <vector>
#include <optional>
#include <ostream>

namespace mem_tac_review {

enum class Player {
    P1,  // Usually 'X'
    P2   // Usually 'O'
};

inline std::string player_to_string(Player p) {
    return (p == Player::P1) ? "P1" : "P2";
}

inline char player_to_mark(Player p) {
    return (p == Player::P1) ? 'X' : 'O';
}

inline Player opponent_of(Player p) {
    return (p == Player::P1) ? Player::P2 : Player::P1;
}

enum class GameOutcome {
    IN_PROGRESS,
    P1_WINS,
    P2_WINS,
    DRAW
};

inline std::string outcome_to_string(GameOutcome o) {
    switch (o) {
        case GameOutcome::IN_PROGRESS: return "In progress";
        case GameOutcome::P1_WINS: return "P1 (X) wins";
        case GameOutcome::P2_WINS: return "P2 (O) wins";
        case GameOutcome::DRAW: return "Draw";
    }
    return "Unknown";
}

struct MoveRecord {
    Player player;
    int row;
    int col;
    bool valid;
};

class ReferenceModel {
public:
    explicit ReferenceModel(int grid_size = 3, Player starting_player = Player::P1);

    int grid_size() const noexcept { return grid_size_; }
    Player current_player() const noexcept { return current_player_; }
    Player starting_player() const noexcept { return starting_player_; }
    GameOutcome outcome() const noexcept { return outcome_; }
    bool is_game_over() const noexcept { return outcome_ != GameOutcome::IN_PROGRESS; }
    int move_count() const noexcept { return move_count_; }
    const std::vector<MoveRecord>& move_history() const noexcept { return move_history_; }

    char cell(int r, int c) const;
    bool is_valid_move(Player p, int r, int c) const;

    // Attempts to apply a move.
    // If valid: updates the cell, switches current_player, updates outcome, increments move_count.
    // If invalid: leaves board, current_player, and outcome unchanged; records move as invalid.
    // Returns true if valid, false if invalid.
    bool apply_move(Player p, int r, int c);

    // Monospace multi-line board rendering (e.g. "X O .\n. X .\n. . .")
    std::string format_board() const;

    // Single line board summary for compact display
    std::string format_board_compact() const;

    // Full state summary including next player and outcome
    std::string format_summary() const;

    void reset();

private:
    void evaluate_outcome();

    int grid_size_;
    Player starting_player_;
    Player current_player_;
    GameOutcome outcome_;
    int move_count_;
    std::vector<char> board_;  // grid_size_ * grid_size_
    std::vector<MoveRecord> move_history_;
};

} // namespace mem_tac_review
