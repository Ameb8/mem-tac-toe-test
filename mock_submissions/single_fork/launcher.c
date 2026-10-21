#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <ctype.h>

struct SharedGame {
    int size;
    int current_turn; // 1 = P1 (X), 2 = P2 (O), 0 = Finished
    int winner;       // 1 = P1, 2 = P2, 3 = Draw, 0 = In Progress
    int step_count;
    char board[64];
};

static void print_board(const struct SharedGame* game) {
    int N = game->size;
    printf("\n--- Board State (Step %d) ---\n", game->step_count);
    for (int r = 0; r < N; ++r) {
        for (int c = 0; c < N; ++c) {
            printf("%c ", game->board[r * N + c]);
        }
        printf("\n");
    }
    fflush(stdout);
}

static int check_win(const struct SharedGame* game) {
    int N = game->size;
    const char* b = game->board;

    for (int r = 0; r < N; ++r) {
        char ch = b[r * N + 0];
        if (ch == '.') continue;
        int win = 1;
        for (int c = 1; c < N; ++c) {
            if (b[r * N + c] != ch) { win = 0; break; }
        }
        if (win) return (ch == 'X') ? 1 : 2;
    }

    for (int c = 0; c < N; ++c) {
        char ch = b[0 * N + c];
        if (ch == '.') continue;
        int win = 1;
        for (int r = 1; r < N; ++r) {
            if (b[r * N + c] != ch) { win = 0; break; }
        }
        if (win) return (ch == 'X') ? 1 : 2;
    }

    char d1 = b[0];
    if (d1 != '.') {
        int win = 1;
        for (int i = 1; i < N; ++i) {
            if (b[i * N + i] != d1) { win = 0; break; }
        }
        if (win) return (d1 == 'X') ? 1 : 2;
    }

    char d2 = b[0 * N + (N - 1)];
    if (d2 != '.') {
        int win = 1;
        for (int i = 1; i < N; ++i) {
            if (b[i * N + (N - 1 - i)] != d2) { win = 0; break; }
        }
        if (win) return (d2 == 'X') ? 1 : 2;
    }

    int full = 1;
    for (int i = 0; i < N * N; ++i) {
        if (b[i] == '.') { full = 0; break; }
    }
    if (full) return 3;

    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <size>\n", argv[0]);
        return 1;
    }

    int N = atoi(argv[1]);
    if (N < 1 || N > 8) {
        fprintf(stderr, "Error: Invalid grid size %d\n", N);
        return 1;
    }

    struct SharedGame* game = mmap(NULL, sizeof(struct SharedGame),
                                   PROT_READ | PROT_WRITE,
                                   MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (game == MAP_FAILED) {
        perror("mmap failed");
        return 1;
    }

    game->size = N;
    game->current_turn = 1;
    game->winner = 0;
    game->step_count = 0;
    for (int i = 0; i < N * N; ++i) game->board[i] = '.';

    printf("Starting fork-based game with grid size %d.\n", N);
    print_board(game);

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        return 1;
    }

    if (pid == 0) {
        // Child: Player 2 (O)
        while (game->winner == 0) {
            if (game->current_turn == 2) {
                printf("[Child] Player 2 (O) move (row col): \n");
                fflush(stdout);

                int r, c;
                int ret = scanf("%d %d", &r, &c);
                if (ret == EOF) break;
                if (ret != 2) {
                    printf("Invalid input. Try again.\n");
                    fflush(stdout);
                    int ch; while ((ch = getchar()) != '\n' && ch != EOF);
                    continue;
                }
                if (r < 0 || r >= N || c < 0 || c >= N || game->board[r * N + c] != '.') {
                    printf("Invalid cell!\n");
                    fflush(stdout);
                    continue;
                }

                game->board[r * N + c] = 'O';
                game->step_count++;
                game->winner = check_win(game);
                if (game->winner != 0) {
                    game->current_turn = 0;
                    break;
                }
                game->current_turn = 1;
            } else {
                usleep(15000);
            }
        }

        if (game->winner == 2) {
            printf("[Child] I am the winning process (Player 2)!\n");
        } else if (game->winner == 1) {
            printf("[Child] I am the process who lost the game (Player 2).\n");
        } else if (game->winner == 3) {
            printf("[Child] Game ended in a draw.\n");
        }
        fflush(stdout);
        _exit(0);
    }

    // Parent: Player 1 (X)
    int last_step = 0;
    while (game->winner == 0) {
        if (game->current_turn == 1) {
            printf("[Parent] Player 1 (X) move (row col): \n");
            fflush(stdout);

            int r, c;
            int ret = scanf("%d %d", &r, &c);
            if (ret == EOF) break;
            if (ret != 2) {
                printf("Invalid input. Try again.\n");
                fflush(stdout);
                int ch; while ((ch = getchar()) != '\n' && ch != EOF);
                continue;
            }
            if (r < 0 || r >= N || c < 0 || c >= N || game->board[r * N + c] != '.') {
                printf("Invalid cell!\n");
                fflush(stdout);
                continue;
            }

            game->board[r * N + c] = 'X';
            game->step_count++;
            last_step = game->step_count;
            print_board(game);

            game->winner = check_win(game);
            if (game->winner != 0) {
                game->current_turn = 0;
                break;
            }
            game->current_turn = 2;
        } else if (game->current_turn == 2) {
            usleep(15000);
            if (game->step_count > last_step) {
                last_step = game->step_count;
                print_board(game);
            }
        }
    }

    if (game->winner == 1) {
        printf("[Parent] I am the winning process (Player 1)!\n");
    } else if (game->winner == 2) {
        printf("[Parent] I am the process who lost the game (Player 1).\n");
    } else if (game->winner == 3) {
        printf("[Parent] Game ended in a draw.\n");
    }
    fflush(stdout);

    waitpid(pid, NULL, 0);
    munmap(game, sizeof(struct SharedGame));
    return 0;
}
