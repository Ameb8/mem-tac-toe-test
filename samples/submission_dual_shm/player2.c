#include "common.h"

int main(void) {
    // Try to attach to shared memory created by player 1
    int shmid = -1;
    for (int i = 0; i < 50; i++) {
        shmid = shmget(SHM_KEY, sizeof(struct GameState), 0666);
        if (shmid >= 0) break;
        usleep(50000);
    }

    if (shmid < 0) {
        fprintf(stderr, "Player 2: Failed to connect to shared memory\n");
        return 1;
    }

    struct GameState *game = (struct GameState *)shmat(shmid, NULL, 0);
    if (game == (void *)-1) {
        perror("Player 2 shmat");
        return 1;
    }

    game->p2_ready = 1;
    printf("Player 2 (O) connected to game of size %d\n", game->n);
    fflush(stdout);

    int n = game->n;
    char line[128];

    while (game->turn != 0) {
        if (game->turn == 2) {
            printf("Player 2 (O) enter move: ");
            fflush(stdout);

            if (!fgets(line, sizeof(line), stdin)) {
                fprintf(stderr, "Player 2 received EOF\n");
                break;
            }

            int r = -1, c = -1;
            if (sscanf(line, "%d %d", &r, &c) != 2) {
                printf("Invalid input format. Enter two numbers.\n");
                fflush(stdout);
                continue;
            }

            if (r < 0 || r >= n || c < 0 || c >= n) {
                printf("Move (%d, %d) out of bounds.\n", r, c);
                fflush(stdout);
                continue;
            }

            if (game->board[r][c] != '.') {
                printf("Cell (%d, %d) is already occupied.\n", r, c);
                fflush(stdout);
                continue;
            }

            // Apply move
            game->board[r][c] = 'O';
            if (check_winner(game, 'O')) {
                game->winner = 2;
                game->turn = 0;
            } else if (is_board_full(game)) {
                game->winner = 3;
                game->turn = 0;
            } else {
                game->turn = 1; // P1 turn
            }
        } else if (game->turn == 1) {
            usleep(20000);
        }
    }

    // Outcome reporting
    if (game->winner == 2) {
        printf("Player 2: I am the winning process!\n");
    } else if (game->winner == 1) {
        printf("Player 2: I am the process who lost the game.\n");
    } else if (game->winner == 3) {
        printf("Player 2: Game ended in a draw.\n");
    }
    fflush(stdout);

    shmdt(game);
    return 0;
}
