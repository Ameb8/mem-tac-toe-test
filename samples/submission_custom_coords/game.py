import sys

def print_board(n, grid):
    print("Board:")
    for r in range(n):
        print(" ".join(grid[r]))
    sys.stdout.flush()

def main():
    if len(sys.argv) < 2:
        print("Usage: game <size>")
        sys.exit(1)

    n = int(sys.argv[1])
    grid = [["." for _ in range(n)] for _ in range(n)]
    turn = "X"

    print(f"Game started N={n}")
    print_board(n, grid)

    while True:
        sys.stdout.write(f"Player {turn} Move: ")
        sys.stdout.flush()
        line = sys.stdin.readline()
        if not line:
            break
        parts = line.strip().split(",")
        if len(parts) != 2:
            print("Invalid format! Use r,c (1-based)")
            sys.stdout.flush()
            continue
        try:
            r = int(parts[0]) - 1
            c = int(parts[1]) - 1
        except ValueError:
            print("Non-numeric coordinates")
            sys.stdout.flush()
            continue

        if not (0 <= r < n and 0 <= c < n):
            print("Out of bounds")
            sys.stdout.flush()
            continue

        if grid[r][c] != ".":
            print("Occupied")
            sys.stdout.flush()
            continue

        grid[r][c] = turn
        print_board(n, grid)

        # Check win
        # row
        if all(grid[r][j] == turn for j in range(n)):
            print(f"Winner: {turn}")
            sys.stdout.flush()
            break
        # col
        if all(grid[i][c] == turn for i in range(n)):
            print(f"Winner: {turn}")
            sys.stdout.flush()
            break
        # diag1
        if r == c and all(grid[i][i] == turn for i in range(n)):
            print(f"Winner: {turn}")
            sys.stdout.flush()
            break
        # diag2
        if r + c == n - 1 and all(grid[i][n - 1 - i] == turn for i in range(n)):
            print(f"Winner: {turn}")
            sys.stdout.flush()
            break

        turn = "O" if turn == "X" else "X"

if __name__ == "__main__":
    main()
