#include "common.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <grid_size>\n", argv[0]);
        return 1;
    }

    char *endptr;
    long n_val = strtol(argv[1], &endptr, 10);
    if (*endptr != '\0' || n_val < 1 || n_val > MAX_N) {
        fprintf(stderr, "Invalid grid size: %s (must be integer 1..%d)\n", argv[1], MAX_N);
        return 1;
    }
    int n = (int)n_val;

    // Create shared memory
    int shmid = shmget(SHM_KEY, sizeof(struct GameState), IPC_CREAT | 0666);
    if (shmid < 0) {
        perror("shmget");
        return 1;
    }

    struct GameState *game = (struct GameState *)shmat(shmid, NULL, 0);
    if (game == (void *)-1) {
        perror("shmat");
        return 1;
    }

    // Initialize game
    game->n = n;
    game->turn = 1; // P1 starts
    game->winner = 0;
    game->p1_ready = 1;
    game->p2_ready = 0;
    for (int r = 0; r < n; r++) {
        for (int c = 0; c < n; c++) {
            game->board[r][c] = '.';
        }
    }

    printf("Player 1 (X) started with grid size %d\n", n);
    print_board(game);

    // Wait for player 2
    printf("Waiting for Player 2 to join...\n");
    fflush(stdout);
    int wait_count = 0;
    while (!game->p2_ready && wait_count < 100) {
        usleep(50000);
        wait_count++;
    }
    printf("Player 2 connected. Game begins!\n");
    fflush(stdout);

    char line[128];
    while (game->turn != 0) {
        if (game->turn == 1) {
            printf("Player 1 (X) enter move: ");
            fflush(stdout);

            if (!fgets(line, sizeof(line), stdin)) {
                fprintf(stderr, "Player 1 received EOF\n");
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
            game->board[r][c] = 'X';
            if (check_winner(game, 'X')) {
                game->winner = 1;
                game->turn = 0;
            } else if (is_board_full(game)) {
                game->winner = 3;
                game->turn = 0;
            } else {
                game->turn = 2; // P2 turn
            }
            print_board(game);
        } else if (game->turn == 2) {
            // Waiting for P2 move
            usleep(20000);
            if (game->turn != 2) {
                // P2 made a move (or game ended)! Creator prints updated board!
                print_board(game);
            }
        }
    }

    // Outcome reporting
    if (game->winner == 1) {
        printf("Player 1: I am the winning process!\n");
    } else if (game->winner == 2) {
        printf("Player 1: I am the process who lost the game.\n");
    } else if (game->winner == 3) {
        printf("Player 1: Game ended in a draw.\n");
    }
    fflush(stdout);

    // Give P2 time to see outcome before cleanup
    usleep(100000);

    // Cleanup shared memory
    shmdt(game);
    shmctl(shmid, IPC_RMID, NULL);
    return 0;
}
