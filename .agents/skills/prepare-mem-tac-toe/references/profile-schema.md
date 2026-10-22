# MemTacToe Review Profile Schema Reference

Submission profiles are declarative YAML or JSON configuration files that instruct `mem-tac-review` how to build, launch, pace, and interact with a student implementation.

---

## Configuration Fields

### Identity & Build

| Field | Type | Required | Default | Description |
|---|---|---|---|---|
| `name` | string | **Yes** | — | Unique profile identifier (e.g. `"puacaden"`, `"student_a"`). |
| `work_dir` | string | No | `"."` | Working directory where compilation and binaries execute. Can be relative to the profile file or absolute. |
| `build_cmd` | string | No | `null` | Shell command executed to compile the submission before running tests (e.g., `"make clean && make"` or `"gcc -Wall -O2 -o player1 player1.c && gcc -Wall -O2 -o player2 player2.c"`). If omitted or null, no compilation step is performed. |
| `env` | dict | No | `{}` | Key-value dictionary of environment variables passed to build and execution processes. |

### Process Topology & Launch

| Field | Type | Required | Default | Description |
|---|---|---|---|---|
| `topology` | string | No | `"dual"` | Process architecture: `"dual"` (two independently launched processes communicating via shared memory / IPC) or `"single"` (fork-based or single launcher process sharing one terminal). |
| `launch_p1` | list[str] | No | `["./player1", "{N}"]` | Command array used to launch Player 1. `{N}` is dynamically replaced with the scenario's grid size (e.g. `3`, `4`, `5`). |
| `launch_p2` | list[str] | No | `["./player2"]` | Command array used to launch Player 2 (only used when `topology: "dual"`). Can also include `{N}` or extra arguments if required. |
| `creator` | string | No | `"p1"` | Identifies which process creates the shared memory segment: `"p1"`, `"p2"`, or `"shared"`. The creator process is launched first. |
| `starting_player` | string | No | `"p1"` | Specifies which player moves first: `"p1"` or `"p2"`. |
| `p1_mark` | string | No | `"X"` | Mark character used by Player 1 (typically `"X"`). |
| `p2_mark` | string | No | `"O"` | Mark character used by Player 2 (typically `"O"`). |

### Coordinate Conventions

| Field | Type | Required | Default | Description |
|---|---|---|---|---|
| `coord_base` | integer | No | `0` | Coordinate indexing base: `0` (0-indexed: `0` to `N-1`) or `1` (1-indexed: `1` to `N`). |
| `coord_order` | string | No | `"row_col"` | Coordinate order: `"row_col"` (row first, then column) or `"col_row"` (column/x first, then row/y). |
| `input_format` | str \| list[str] | No | `"{r} {c}\n"` | Stdin move template. Examples: `"{r} {c}\n"`, `"{r},{c}\n"`, `"{r}\n{c}\n"`, or a sequence of prompts `["{r}\n", "{c}\n"]`. |

### Interaction Pacing & Timeouts

| Field | Type | Required | Default | Description |
|---|---|---|---|---|
| `turn_prompt` | string | No | `null` | Regular expression matching the prompt printed by the program when awaiting input (e.g., `"(?i)enter.*:"`, `"enter row and column"`). Case-insensitive matching can be enabled with `(?i)`. |
| `settling_ms` | integer | No | `100` | Quiet-window settling duration in milliseconds (typically 80–200 ms). If output stream goes quiet for this duration, interaction proceeds. Vital for programs that don't output consistent prompt strings. |
| `timeout_sec` | float | No | `10.0` | Maximum seconds to wait per turn/checkpoint before timing out. |
| `session_timeout_sec` | float | No | `60.0` | Maximum total duration in seconds allowed for a single scenario session. |

---

## Example Profiles

### Example 1: Dual-Process System V Shared Memory (`topology: "dual"`)
```yaml
name: "puacaden"
build_cmd: "gcc -Wall -Wextra -O2 -o player1 player1.c && gcc -Wall -Wextra -O2 -o player2 player2.c"
work_dir: "../submissions/puacaden/src"
topology: "dual"
launch_p1: ["./player1", "{N}"]
launch_p2: ["./player2", "{N}"]
creator: "p1"
starting_player: "p1"
p1_mark: "X"
p2_mark: "O"
coord_base: 1
coord_order: "row_col"
input_format: "{r} {c}\n"
turn_prompt: "enter row and column"
settling_ms: 100
timeout_sec: 5.0
```

### Example 2: Single Fork-Based Process (`topology: "single"`)
```yaml
name: "whitmorejane"
build_cmd: "gcc -Wall -Wextra -O2 -pthread -o ticTacToe ticTacToe.c"
work_dir: "../submissions/whitmorejane/src"
topology: "single"
launch_p1: ["./ticTacToe", "{N}"]
creator: "p1"
starting_player: "p1"
coord_base: 0
coord_order: "row_col"
input_format: "{r} {c}\n"
turn_prompt: "enter your coordinates:"
settling_ms: 100
timeout_sec: 5.0
```

### Example 3: Dual-Process with Extra CLI Arguments
When the program expects extra arguments (such as a shared-memory name or game key):
```yaml
name: "andersonalexander"
build_cmd: "gcc -Wall -Wextra -O2 -o tic_tac_toe tic_tac_toe.c -lrt"
work_dir: "../submissions/andersonalexander/src"
topology: "dual"
launch_p1: ["./tic_tac_toe", "{N}", "shm_game"]
launch_p2: ["./tic_tac_toe", "{N}", "shm_game"]
creator: "p1"
coord_base: 1
coord_order: "col_row"
input_format: "{r} {c}\n"
turn_prompt: "Enter x and y coordinate"
settling_ms: 120
timeout_sec: 5.0
```
