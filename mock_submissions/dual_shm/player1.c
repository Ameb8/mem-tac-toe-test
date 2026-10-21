#include "common.h"
#include <ctype.h>

int parse_int(const char* str) {
    if (!str || *str == '\0') return -1;
    for (int i = 0; str[i] != '\0'; ++i) {
        if (!isdigit(str[i])) return -1;
    }
    return atoi(str);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Error: Grid size must be provided as command line argument.\nUsage: %s <N>\n", argv[0]);
        return 1;
    }

    int N = parse_int(argv[1]);
    if (N < 1 || N > 8) {
        fprintf(stderr, "Error: Invalid grid size '%s'. Must be an integer between 1 and 8.\n", argv[1]);
        return 1;
    }

    int shmid = shmget(SHM_KEY, sizeof(struct GameState), IPC_CREAT | 0666);
    if (shmid < 0) {
        perror("shmget failed");
        return 1;
    }

    struct GameState* state = (struct GameState*)shmat(shmid, NULL, 0);
    if (state == (void*)-1) {
        perror("shmat failed");
        return 1;
    }

    // Initialize game
    state->size = N;
    state->current_turn = 1;
    state->winner = 0;
    state->step_count = 0;
    for (int i = 0; i < N * N; ++i) {
        state->board[i] = '.';
    }

    printf("=== Tic-Tac-Toe Player 1 (Creator) ===\n");
    printf("Grid size: %dx%d. You are playing as 'X'.\n", N, N);
    print_grid(state);

    int last_step_seen = 0;

    while (state->winner == 0) {
        if (state->current_turn == 1) {
            printf("Player 1 (X) Enter move (row col): \n");
            fflush(stdout);

            int r, c;
            int ret = scanf("%d %d", &r, &c);
            if (ret == EOF) {
                printf("Player 1 received EOF. Exiting.\n");
                break;
            }
            if (ret != 2) {
                printf("Invalid non-numeric input. Try again.\n");
                fflush(stdout);
                // Clear input line
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
            state->board[r * N + c] = 'X';
            state->step_count++;
            last_step_seen = state->step_count;
            print_grid(state);

            state->winner = check_winner(state);
            if (state->winner != 0) {
                state->current_turn = 0;
                break;
            }
            state->current_turn = 2;

        } else if (state->current_turn == 2) {
            // Waiting for Player 2
            usleep(15000);
            if (state->step_count > last_step_seen) {
                last_step_seen = state->step_count;
                // Creator is responsible to print grid after each new step!
                print_grid(state);
            }
        } else {
            break;
        }
    }

    // Print outcome
    if (state->winner == 1) {
        printf("Result: I am the winning process! (Player 1 / X)\n");
    } else if (state->winner == 2) {
        printf("Result: I am the process who lost the game. (Player 1 / X)\n");
    } else if (state->winner == 3) {
        printf("Result: Game ended in a draw!\n");
    }
    fflush(stdout);

    // Give peer time to see winner before detaching and removing shm
    usleep(50000);

    shmdt(state);
    shmctl(shmid, IPC_RMID, NULL);
    return 0;
}
