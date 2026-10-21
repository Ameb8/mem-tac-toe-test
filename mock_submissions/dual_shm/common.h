#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define SHM_KEY 0x544143 // "TAC"

struct GameState {
    int size;
    int current_turn; // 1 = P1 (X), 2 = P2 (O), 0 = Game Over, -1 = Uninitialized
    int winner;       // 0 = In progress, 1 = P1 wins, 2 = P2 wins, 3 = Draw
    int step_count;
    char board[64];   // Max 8x8
};

static inline void print_grid(const struct GameState* state) {
    int N = state->size;
    printf("\n--- Current Board (Step %d) ---\n", state->step_count);
    for (int r = 0; r < N; ++r) {
        for (int c = 0; c < N; ++c) {
            printf("%c ", state->board[r * N + c]);
        }
        printf("\n");
    }
    fflush(stdout);
}

static inline int check_winner(struct GameState* state) {
    int N = state->size;
    char* b = state->board;

    // Check rows
    for (int r = 0; r < N; ++r) {
        char first = b[r * N + 0];
        if (first == '.') continue;
        int win = 1;
        for (int c = 1; c < N; ++c) {
            if (b[r * N + c] != first) { win = 0; break; }
        }
        if (win) return (first == 'X') ? 1 : 2;
    }

    // Check cols
    for (int c = 0; c < N; ++c) {
        char first = b[0 * N + c];
        if (first == '.') continue;
        int win = 1;
        for (int r = 1; r < N; ++r) {
            if (b[r * N + c] != first) { win = 0; break; }
        }
        if (win) return (first == 'X') ? 1 : 2;
    }

    // Main diag
    char diag1 = b[0];
    if (diag1 != '.') {
        int win = 1;
        for (int i = 1; i < N; ++i) {
            if (b[i * N + i] != diag1) { win = 0; break; }
        }
        if (win) return (diag1 == 'X') ? 1 : 2;
    }

    // Anti diag
    char diag2 = b[0 * N + (N - 1)];
    if (diag2 != '.') {
        int win = 1;
        for (int i = 1; i < N; ++i) {
            if (b[i * N + (N - 1 - i)] != diag2) { win = 0; break; }
        }
        if (win) return (diag2 == 'X') ? 1 : 2;
    }

    // Check full
    int full = 1;
    for (int i = 0; i < N * N; ++i) {
        if (b[i] == '.') { full = 0; break; }
    }
    if (full) return 3; // Draw

    return 0; // In progress
}

#endif
