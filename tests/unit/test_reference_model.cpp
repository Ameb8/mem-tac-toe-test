#include "mem_tac_review/reference_model.hpp"

#include <cassert>
#include <iostream>
#include <vector>

using namespace mem_tac_review;

void test_initial_state() {
    ReferenceModel rm(3);
    assert(rm.grid_size() == 3);
    assert(rm.current_player() == Player::P1);
    assert(rm.starting_player() == Player::P1);
    assert(rm.outcome() == GameOutcome::IN_PROGRESS);
    assert(!rm.is_game_over());
    assert(rm.move_count() == 0);
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            assert(rm.cell(r, c) == '.');
        }
    }
    assert(rm.cell(3, 3) == '?');
    assert(rm.cell(-1, 0) == '?');
}

void test_g01_top_row_win() {
    // G01: (0,0) (1,0) (0,1) (1,1) (0,2) -> X wins on move 5
    ReferenceModel rm(3);
    assert(rm.apply_move(Player::P1, 0, 0));
    assert(rm.apply_move(Player::P2, 1, 0));
    assert(rm.apply_move(Player::P1, 0, 1));
    assert(rm.apply_move(Player::P2, 1, 1));
    assert(rm.apply_move(Player::P1, 0, 2));

    assert(rm.is_game_over());
    assert(rm.outcome() == GameOutcome::P1_WINS);
    assert(rm.move_count() == 5);
    assert(rm.format_board_compact() == "XXX / OO. / ...");
}

void test_g07_main_diagonal_win() {
    // G07: (0,0) (0,1) (1,1) (0,2) (2,2) -> X wins on move 5
    ReferenceModel rm(3);
    assert(rm.apply_move(Player::P1, 0, 0));
    assert(rm.apply_move(Player::P2, 0, 1));
    assert(rm.apply_move(Player::P1, 1, 1));
    assert(rm.apply_move(Player::P2, 0, 2));
    assert(rm.apply_move(Player::P1, 2, 2));

    assert(rm.outcome() == GameOutcome::P1_WINS);
    assert(rm.format_board_compact() == "XOO / .X. / ..X");
}

void test_g08_anti_diagonal_win() {
    // G08: (0,2) (0,0) (1,1) (0,1) (2,0) -> X wins on move 5
    ReferenceModel rm(3);
    assert(rm.apply_move(Player::P1, 0, 2));
    assert(rm.apply_move(Player::P2, 0, 0));
    assert(rm.apply_move(Player::P1, 1, 1));
    assert(rm.apply_move(Player::P2, 0, 1));
    assert(rm.apply_move(Player::P1, 2, 0));

    assert(rm.outcome() == GameOutcome::P1_WINS);
    assert(rm.format_board_compact() == "OOX / .X. / X..");
}

void test_g09_p2_wins() {
    // G09: (0,0) (1,1) (1,0) (0,1) (0,2) (2,1) -> O wins on move 6
    ReferenceModel rm(3);
    assert(rm.apply_move(Player::P1, 0, 0));
    assert(rm.apply_move(Player::P2, 1, 1));
    assert(rm.apply_move(Player::P1, 1, 0));
    assert(rm.apply_move(Player::P2, 0, 1));
    assert(rm.apply_move(Player::P1, 0, 2));
    assert(rm.apply_move(Player::P2, 2, 1));

    assert(rm.outcome() == GameOutcome::P2_WINS);
    assert(rm.format_board_compact() == "XOX / XO. / .O.");
}

void test_g10_draw() {
    // G10: (0,0) (0,1) (0,2) (1,2) (1,0) (2,0) (1,1) (2,2) (2,1) -> draw on move 9
    ReferenceModel rm(3);
    std::vector<std::pair<int, int>> moves = {
        {0, 0}, {0, 1}, {0, 2}, {1, 2}, {1, 0}, {2, 0}, {1, 1}, {2, 2}, {2, 1}
    };
    Player p = Player::P1;
    for (auto [r, c] : moves) {
        assert(rm.apply_move(p, r, c));
        p = opponent_of(p);
    }
    assert(rm.outcome() == GameOutcome::DRAW);
    assert(rm.format_board_compact() == "XOX / XXO / OXO");
}

void test_g11_win_on_final_empty_cell() {
    // G11: (0,0) (0,1) (0,2) (1,0) (1,1) (1,2) (2,1) (2,0) (2,2) -> X wins on move 9, NOT a draw
    ReferenceModel rm(3);
    std::vector<std::pair<int, int>> moves = {
        {0, 0}, {0, 1}, {0, 2}, {1, 0}, {1, 1}, {1, 2}, {2, 1}, {2, 0}, {2, 2}
    };
    Player p = Player::P1;
    for (auto [r, c] : moves) {
        assert(rm.apply_move(p, r, c));
        p = opponent_of(p);
    }
    assert(rm.outcome() == GameOutcome::P1_WINS);
    assert(rm.format_board_compact() == "XOX / OXO / OXX");
}

void test_4x4_and_5x5_behavior() {
    // 4x4: row win requires 4 marks, 3 marks is still in progress
    ReferenceModel rm4(4);
    assert(rm4.apply_move(Player::P1, 0, 0));
    assert(rm4.apply_move(Player::P2, 1, 0));
    assert(rm4.apply_move(Player::P1, 0, 1));
    assert(rm4.apply_move(Player::P2, 1, 1));
    assert(rm4.apply_move(Player::P1, 0, 2));
    assert(rm4.apply_move(Player::P2, 1, 2));

    // 3 aligned X marks on 4x4 must NOT win!
    assert(rm4.outcome() == GameOutcome::IN_PROGRESS);

    // 4th X mark completes the row
    assert(rm4.apply_move(Player::P1, 0, 3));
    assert(rm4.outcome() == GameOutcome::P1_WINS);

    // 5x5: main diagonal win requires 5 marks
    ReferenceModel rm5(5);
    for (int k = 0; k < 4; ++k) {
        assert(rm5.apply_move(Player::P1, k, k));
        assert(rm5.apply_move(Player::P2, k, k + 1));
    }
    // 4 marks on 5x5: still in progress
    assert(rm5.outcome() == GameOutcome::IN_PROGRESS);

    // 5th mark: (4, 4)
    assert(rm5.apply_move(Player::P1, 4, 4));
    assert(rm5.outcome() == GameOutcome::P1_WINS);
}

void test_invalid_moves() {
    ReferenceModel rm(3);
    // Wrong player
    assert(!rm.apply_move(Player::P2, 0, 0));
    assert(rm.current_player() == Player::P1);
    assert(rm.move_count() == 0);

    // Out of bounds
    assert(!rm.apply_move(Player::P1, -1, 0));
    assert(!rm.apply_move(Player::P1, 3, 0));
    assert(!rm.apply_move(Player::P1, 0, -1));
    assert(!rm.apply_move(Player::P1, 0, 3));
    assert(rm.current_player() == Player::P1);

    // Valid move
    assert(rm.apply_move(Player::P1, 0, 0));
    assert(rm.current_player() == Player::P2);

    // Occupied cell by P1
    assert(!rm.apply_move(Player::P2, 0, 0));
    assert(rm.current_player() == Player::P2);

    // Valid move for P2
    assert(rm.apply_move(Player::P2, 0, 1));

    // Occupied cell by P2
    assert(!rm.apply_move(Player::P1, 0, 1));
    assert(rm.current_player() == Player::P1);
}

int main() {
    test_initial_state();
    test_g01_top_row_win();
    test_g07_main_diagonal_win();
    test_g08_anti_diagonal_win();
    test_g09_p2_wins();
    test_g01_top_row_win();
    test_g10_draw();
    test_g11_win_on_final_empty_cell();
    test_4x4_and_5x5_behavior();
    test_invalid_moves();

    std::cout << "All ReferenceModel tests passed!" << std::endl;
    return 0;
}
