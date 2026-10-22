# Source Investigation Checklist

When preparing an unfamiliar MemTacToe submission for evaluation, systematically examine the source code to determine the profile parameters.

---

## 1. File Discovery & Build Mechanism

Inspect all files in the submission folder:
- **Build files:** Check for `Makefile`, `makefile`, `CMakeLists.txt`, `build.sh`, etc.
  - If a Makefile exists, inspect `all`, `clean`, and default targets.
  - If no Makefile exists, identify which `.c` or `.cpp` files need to be compiled.
- **Compiler flags & libraries:**
  - System V IPC (`shmget`, `semget`) usually compiles with standard `gcc`.
  - POSIX shared memory (`shm_open`, `shm_unlink`) and timers (`clock_gettime`) often require `-lrt`.
  - POSIX semaphores (`sem_open`, `sem_wait`) or threading require `-pthread`.
  - Math functions (`sqrt`, `pow`) require `-lm`.
  - Check for C standard compatibility: e.g. if code uses `bool` without `<stdbool.h>`, `-std=c2x` or `-std=gnu2x` or C++ may compile it cleanly without source edits.

> [!CAUTION]
> **Never modify the student's source code!** If the code fails to compile with reasonable standard flags, note the compilation error. Do not fix typos, missing includes, or syntax bugs in student code.

---

## 2. Process Entry Points & Topology

Locate all `int main(...)` functions across the source files:
- **Two separate `main` functions (or two distinct executables):**
  - Example: `player1.c` and `player2.c`, or `player_x.c` and `player_o.c`.
  - Topology is almost certainly `"dual"`.
- **One `main` function using `fork()`:**
  - If parent and child run concurrently in the same terminal session:
  - Topology is `"single"`.
  - `launch_p1` launches the single program; `launch_p2` is not used.
- **One `main` function launched twice:**
  - A single program that checks if shared memory already exists or takes an argument specifying role (e.g. `./game 3 1` and `./game 3 2`, or `./tic_tac_toe 3 shm_name`).
  - Topology is `"dual"`. Both `launch_p1` and `launch_p2` invoke the same binary with appropriate arguments.

---

## 3. Launch Arguments & CLI Handling

Examine argument parsing around `argc` and `argv[...]`:
- What is `argv[1]`? (Usually `{N}`, the grid size).
- Are additional arguments expected?
  - Shared memory key or name: e.g. `argv[2]` is game key/name.
  - Player role: e.g. `argv[2]` is `'X'` or `'O'`.
- What are the grid size limits?
  - Does the implementation check `if (n < 3)` or `if (n > MAX_N)`?
  - Does it strictly reject $N > 3$? Note this in your investigation so expected outcomes for $N \times N$ scenarios are clear.
- Does the program read grid size from standard input instead of CLI arguments?
  - If so, use a **shim script** (see [Shim Patterns](./shim-patterns.md)).

---

## 4. Shared Memory & Synchronization Roles

Determine which process creates the shared memory segment:
- Search for creation flags:
  - System V: `shmget(..., IPC_CREAT | IPC_EXCL | ...)`
  - POSIX: `shm_open(..., O_CREAT | O_EXCL | ...)`
  - POSIX Anonymous: `mmap(..., MAP_SHARED | MAP_ANONYMOUS, ...)`
- Set `creator: "p1"` if Player 1 creates the segment and Player 2 joins.
- Set `creator: "p2"` if Player 2 creates the segment.
- Set `creator: "shared"` if either can create or if handled by an external/shared setup.

---

## 5. Coordinate Conventions

Determine how user coordinates are parsed from stdin:
- **Index base (`coord_base`):**
  - Check for subtraction: `row--; col--;` or `r - 1` -> **`coord_base: 1`**.
  - Check user prompt message: `"enter row and column (1-3)"` -> **`coord_base: 1`**.
  - Check validation bounds: `if (r >= n || r < 0)` without subtraction -> **`coord_base: 0`**.
- **Coordinate order (`coord_order`):**
  - Standard matrix notation (row, then column): `coord_order: "row_col"`.
  - Cartesian / x-y notation where user enters column/x first: `coord_order: "col_row"`.
- **Input format (`input_format`):**
  - Space-separated single line: `"{r} {c}\n"` (most common).
  - Comma-separated: `"{r},{c}\n"`.
  - Separate interactive prompts: e.g. first asks for row, then column:
    ```yaml
    input_format:
      - "{r}\n"
      - "{c}\n"
    ```

---

## 6. Interaction Pacing & Turn Prompts

- Look for the exact output string printed immediately before waiting for input (`fgets`, `scanf`, `cin`):
  - Example: `printf("Player 1, enter move: "); fflush(stdout);`
  - Turn prompt regex: `turn_prompt: "enter move:"` or `turn_prompt: "(?i)enter.*:"`.
- If prompts are dynamic, inconsistent, or lack trailing spaces/colons:
  - Use a flexible regex or rely on quiet-window settling (`settling_ms: 120` to `150`).
- Check if output is explicitly flushed (`fflush(stdout)`):
  - Note: The test harness runs under pseudo-terminals (PTYs), which provides standard interactive line buffering even without explicit `fflush()`.

---

## 7. Viability Assessment

Before running the harness, evaluate whether the submission can be run:
- **Supported:** Terminal-based C/C++ implementations (dual process or single fork) that take interactive text input.
- **Unviable / Unsupported:**
  - Submissions requiring graphical displays (X11, SDL, GUI).
  - Submissions missing core source files or incomplete code that fails compilation.
  - Submissions that hang indefinitely on startup due to race conditions or deadlocks that cannot be resolved via pacing or shims.

If unviable, explain clearly why the harness cannot be run on the implementation and do not proceed with running invalid tests.
