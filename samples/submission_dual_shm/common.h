#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define MAX_N 10
#define SHM_KEY 0x470001

struct GameState {
    int n;
    int turn;       // 1 for P1, 2 for P2, 0 for ended
    int winner;     // 0: in progress, 1: P1, 2: P2, 3: draw
    int p1_ready;
    int p2_ready;
    char board[MAX_N][MAX_N];
};

static inline void print_board(const struct GameState *game) {
    printf("Current Board:\n");
    for (int r = 0; r < game->n; r++) {
        for (int c = 0; c < game->n; c++) {
            printf("%c%s", game->board[r][c], (c == game->n - 1) ? "" : " ");
        }
        printf("\n");
    }
    fflush(stdout);
}

static inline int check_winner(struct GameState *game, char mark) {
    int n = game->n;
    // Check rows
    for (int r = 0; r < n; r++) {
        int win = 1;
        for (int c = 0; c < n; c++) {
            if (game->board[r][c] != mark) { win = 0; break; }
        }
        if (win) return 1;
    }
    // Check columns
    for (int c = 0; c < n; c++) {
        int win = 1;
        for (int r = 0; r < n; r++) {
            if (game->board[r][c] != mark) { win = 0; break; }
        }
        if (win) return 1;
    }
    // Check main diagonal
    int win_diag1 = 1;
    for (int i = 0; i < n; i++) {
        if (game->board[i][i] != mark) { win_diag1 = 0; break; }
    }
    if (win_diag1) return 1;
    // Check other diagonal
    int win_diag2 = 1;
    for (int i = 0; i < n; i++) {
        if (game->board[i][n - 1 - i] != mark) { win_diag2 = 0; break; }
    }
    if (win_diag2) return 1;

    return 0;
}

static inline int is_board_full(const struct GameState *game) {
    for (int r = 0; r < game->n; r++) {
        for (int c = 0; c < game->n; c++) {
            if (game->board[r][c] == '.') return 0;
        }
    }
    return 1;
}

#endif
