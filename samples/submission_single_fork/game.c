#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>

#define MAX_N 10

struct GameState {
    int n;
    int turn;       // 1 for parent (X), 2 for child (O), 0 for game ended
    int winner;     // 0 in-progress, 1 parent wins, 2 child wins, 3 draw
    char board[MAX_N][MAX_N];
};

void print_board(const struct GameState *game) {
    printf("Current Board:\n");
    for (int r = 0; r < game->n; r++) {
        for (int c = 0; c < game->n; c++) {
            printf("%c%s", game->board[r][c], (c == game->n - 1) ? "" : " ");
        }
        printf("\n");
    }
    fflush(stdout);
}

int check_winner(struct GameState *game, char mark) {
    int n = game->n;
    for (int r = 0; r < n; r++) {
        int win = 1;
        for (int c = 0; c < n; c++) {
            if (game->board[r][c] != mark) { win = 0; break; }
        }
        if (win) return 1;
    }
    for (int c = 0; c < n; c++) {
        int win = 1;
        for (int r = 0; r < n; r++) {
            if (game->board[r][c] != mark) { win = 0; break; }
        }
        if (win) return 1;
    }
    int win_diag1 = 1;
    for (int i = 0; i < n; i++) {
        if (game->board[i][i] != mark) { win_diag1 = 0; break; }
    }
    if (win_diag1) return 1;

    int win_diag2 = 1;
    for (int i = 0; i < n; i++) {
        if (game->board[i][n - 1 - i] != mark) { win_diag2 = 0; break; }
    }
    if (win_diag2) return 1;

    return 0;
}

int is_full(const struct GameState *game) {
    for (int r = 0; r < game->n; r++) {
        for (int c = 0; c < game->n; c++) {
            if (game->board[r][c] == '.') return 0;
        }
    }
    return 1;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <grid_size>\n", argv[0]);
        return 1;
    }

    char *endptr;
    long n_val = strtol(argv[1], &endptr, 10);
    if (*endptr != '\0' || n_val < 1 || n_val > MAX_N) {
        fprintf(stderr, "Invalid grid size: %s\n", argv[1]);
        return 1;
    }
    int n = (int)n_val;

    struct GameState *game = mmap(NULL, sizeof(struct GameState),
                                  PROT_READ | PROT_WRITE,
                                  MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (game == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    game->n = n;
    game->turn = 1;
    game->winner = 0;
    for (int r = 0; r < n; r++) {
        for (int c = 0; c < n; c++) {
            game->board[r][c] = '.';
        }
    }

    printf("Shared-terminal game started with grid size %d\n", n);
    print_board(game);

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }

    char line[128];

    if (pid == 0) {
        // Child process plays O (turn == 2)
        while (game->turn != 0) {
            if (game->turn == 2) {
                printf("Child (O) enter move: ");
                fflush(stdout);

                if (!fgets(line, sizeof(line), stdin)) {
                    break;
                }

                int r = -1, c = -1;
                if (sscanf(line, "%d %d", &r, &c) != 2) {
                    printf("Invalid input format.\n");
                    fflush(stdout);
                    continue;
                }

                if (r < 0 || r >= n || c < 0 || c >= n || game->board[r][c] != '.') {
                    printf("Invalid move (%d, %d).\n", r, c);
                    fflush(stdout);
                    continue;
                }

                game->board[r][c] = 'O';
                if (check_winner(game, 'O')) {
                    game->winner = 2;
                    game->turn = 0;
                } else if (is_full(game)) {
                    game->winner = 3;
                    game->turn = 0;
                } else {
                    game->turn = 1;
                }
            } else {
                usleep(15000);
            }
        }

        if (game->winner == 2) {
            printf("Child: I am the winning process!\n");
        } else if (game->winner == 1) {
            printf("Child: I am the process who lost the game.\n");
        } else if (game->winner == 3) {
            printf("Child: Game ended in a draw.\n");
        }
        fflush(stdout);
        munmap(game, sizeof(struct GameState));
        return 0;
    } else {
        // Parent process plays X (turn == 1) and prints updated board
        while (game->turn != 0) {
            if (game->turn == 1) {
                printf("Parent (X) enter move: ");
                fflush(stdout);

                if (!fgets(line, sizeof(line), stdin)) {
                    break;
                }

                int r = -1, c = -1;
                if (sscanf(line, "%d %d", &r, &c) != 2) {
                    printf("Invalid input format.\n");
                    fflush(stdout);
                    continue;
                }

                if (r < 0 || r >= n || c < 0 || c >= n || game->board[r][c] != '.') {
                    printf("Invalid move (%d, %d).\n", r, c);
                    fflush(stdout);
                    continue;
                }

                game->board[r][c] = 'X';
                if (check_winner(game, 'X')) {
                    game->winner = 1;
                    game->turn = 0;
                } else if (is_full(game)) {
                    game->winner = 3;
                    game->turn = 0;
                } else {
                    game->turn = 2;
                }
                print_board(game);
            } else if (game->turn == 2) {
                usleep(15000);
                if (game->turn != 2) {
                    // Child moved, parent prints board
                    print_board(game);
                }
            }
        }

        if (game->winner == 1) {
            printf("Parent: I am the winning process!\n");
        } else if (game->winner == 2) {
            printf("Parent: I am the process who lost the game.\n");
        } else if (game->winner == 3) {
            printf("Parent: Game ended in a draw.\n");
        }
        fflush(stdout);

        waitpid(pid, NULL, 0);
        munmap(game, sizeof(struct GameState));
        return 0;
    }
}
