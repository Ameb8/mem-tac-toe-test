#include "common.h"

int main() {
    int shmid = -1;
    for (int attempts = 0; attempts < 30; ++attempts) {
        shmid = shmget(SHM_KEY, sizeof(struct GameState), 0666);
        if (shmid >= 0) break;
        usleep(50000);
    }

    if (shmid < 0) {
        fprintf(stderr, "Player 2: Failed to connect to shared memory after retries.\n");
        return 1;
    }

    struct GameState* state = (struct GameState*)shmat(shmid, NULL, 0);
    if (state == (void*)-1) {
        perror("Player 2: shmat failed");
        return 1;
    }

    int N = state->size;
    printf("=== Tic-Tac-Toe Player 2 ===\n");
    printf("Joined game of size %dx%d. You are playing as 'O'.\n", N, N);
    fflush(stdout);

    while (state->winner == 0) {
        if (state->current_turn == 2) {
            printf("Player 2 (O) Enter move (row col): \n");
            fflush(stdout);

            int r, c;
            int ret = scanf("%d %d", &r, &c);
            if (ret == EOF) {
                printf("Player 2 received EOF. Exiting.\n");
                break;
            }
            if (ret != 2) {
                printf("Invalid non-numeric input. Try again.\n");
                fflush(stdout);
                int ch;
                while ((ch = getchar()) != '\n' && ch != EOF);
                continue;
            }

            if (r < 0 || r >= N || c < 0 || c >= N) {
                printf("Coordinates (%d, %d) out of bounds! Must be between 0 and %d.\n", r, c, N - 1);
                fflush(stdout);
                continue;
            }

            if (state->board[r * N + c] != '.') {
                printf("Cell (%d, %d) is already occupied! Try another cell.\n", r, c);
                fflush(stdout);
                continue;
            }

            // Apply move
            state->board[r * N + c] = 'O';
            state->step_count++;

            state->winner = check_winner(state);
            if (state->winner != 0) {
                state->current_turn = 0;
                break;
            }
            state->current_turn = 1;

        } else if (state->current_turn == 1) {
            usleep(15000);
        } else {
            break;
        }
    }

    // Print outcome
    if (state->winner == 2) {
        printf("Result: I am the winning process! (Player 2 / O)\n");
    } else if (state->winner == 1) {
        printf("Result: I am the process who lost the game. (Player 2 / O)\n");
    } else if (state->winner == 3) {
        printf("Result: Game ended in a draw!\n");
    }
    fflush(stdout);

    shmdt(state);
    return 0;
}
