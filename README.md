# MemTacToe Test (`mem-tac-review`)

`mem-tac-review` provides repeatable execution and review evidence for multiple implementations of the Operating Systems Lab 2 assignment ("MemTacToe", CS470.001 at CWU).

The harness runs scripted interactions through pseudo-terminals (PTYs) and produces self-contained HTML and JSON reports that place reference game states alongside each implementation's observed terminal output.

> [!IMPORTANT]
> **No Automated Grading:** This program does **not** assign grades, calculate differences between boards, or declare behavioral tests passed or failed. Completing a scripted interaction indicates operational delivery of the test case, not correctness of game logic. The human grader evaluates the evidence and applies rubric criteria independently.

---

## Key Features

- **Pseudo-Terminal (PTY) Process Execution:** Uses `pty.openpty()` to ensure line-buffered interactive terminal behavior, preventing 4KB stdout pipe buffering in Linux C/C++ without modifying student source code.
- **Support for Diverse Process Topologies:**
  - `dual`: Two independent executables (`player1`, `player2`) communicating via shared memory.
  - `single`: Fork-based implementations where parent and child share a terminal.
- **Zero-Code Interface Adaptation:** Declarative profiles handle:
  - 0-based vs 1-based indexing (`coord_base`).
  - Row-column vs column-row order (`coord_order`).
  - Single input strings (`"{r} {c}\n"`, `"{r},{c}\n"`) or multiple prompts (`["{r}\n", "{c}\n"]`).
  - Custom turn prompts (regex) or quiet-window settling intervals (`settling_ms`).
- **Interactive Pacing & Boundaries:** Configurable quiet-window settling (80–150 ms silence on PTY) and turn-prompt matching ensure robust capture even with unbuffered or irregular prompts.
- **Reference Model:** Independent $N \times N$ game engine tracks legal moves, rows, columns, diagonals, and draws for any $N \ge 1$.
- **System V IPC Inspection:** Inspects active shared memory segments before and after each session (`ipcs -m` / `/proc/sysvipc/shm`) to report unreleased segments without deleting unrelated resources.
- **Self-Contained Review Reports:** Standalone HTML reports (no external CSS, fonts, or CDNs) with monospace side-by-side expected vs observed boards, process exit codes, and full terminal transcripts.

---

## Tech Stack & Requirements

- **Python:** 3.14+
- **Environment & Dependency Manager:** [uv](https://docs.astral.sh/uv/)
- **Target Platform:** Linux (tested on Linux ARM64 / Raspberry Pi and Linux x86_64)

---

## Installation & Setup

1. **Clone the repository:**
   ```bash
   git clone https://github.com/ameb8/mem-tac-toe-test.git
   cd mem-tac-toe-test
   ```

2. **Sync dependencies with UV:**
   ```bash
   uv sync --python 3.14
   ```

3. **Verify CLI availability:**
   ```bash
   uv run mem-tac-review --help
   ```

---

## Usage Guide

### 1. Run a Single Submission

Run a single scenario (e.g. `G01`):
```bash
uv run mem-tac-review run --profile profiles/dual_shm.yaml \
  --scenario G01 --output reports/student-a-trial -v
```

Run the canonical `lab-2` test suite:
```bash
uv run mem-tac-review run --profile profiles/dual_shm.yaml \
  --suite lab-2 --output reports/student-a -v
```

### 2. Batch Execution

Run a scenario or suite across all profiles in a directory:
```bash
uv run mem-tac-review batch --profiles profiles/ \
  --suite lab-2 --output reports/cohort -v
```
A batch continues sequentially even if individual submissions fail to build or encounter timeouts.

### 3. Scenario & Suite Inspection

List all registered scenarios:
```bash
uv run mem-tac-review list-scenarios
```

List available suites:
```bash
uv run mem-tac-review list-suites
```

Inspect the steps and expected outcomes of a specific scenario:
```bash
uv run mem-tac-review show-scenario G07
```

Validate a submission profile configuration:
```bash
uv run mem-tac-review validate-profile profiles/dual_shm.yaml
```

---

## Profile Configuration

Profiles are declarative YAML or JSON files that describe how to build, launch, and interact with a submission:

### Example: Dual-Process System V Shared Memory (`profiles/dual_shm.yaml`)
```yaml
name: "dual_shm_submission"
build_cmd: "make clean && make"
work_dir: "../samples/submission_dual_shm"

# Process Topology: "dual" (two separate processes) or "single" (fork / shared terminal)
topology: "dual"
launch_p1: ["./player1", "{N}"]
launch_p2: ["./player2"]
creator: "p1"
starting_player: "p1"

# Coordinate conventions
coord_base: 0                 # 0-based or 1-based indexing
coord_order: "row_col"        # "row_col" or "col_row"
input_format: "{r} {c}\n"     # Single string or list of prompt replies

# Turn boundary & interaction pacing
turn_prompt: "enter move:"    # Optional regex to match prompt
settling_ms: 100              # Quiet-window settling duration in ms
timeout_sec: 5.0              # Max seconds to wait per turn
session_timeout_sec: 60.0     # Max total session duration
```

### Example: Fork-Based Shared Terminal (`profiles/single_fork.yaml`)
```yaml
name: "single_fork_submission"
build_cmd: "make clean && make"
work_dir: "../samples/submission_single_fork"
topology: "single"
launch_p1: ["./game", "{N}"]
creator: "p1"
coord_base: 0
coord_order: "row_col"
input_format: "{r} {c}\n"
turn_prompt: "enter move:"
settling_ms: 100
timeout_sec: 5.0
```

### The Shim Pattern
For interfaces requiring non-standard inputs (such as reading the grid size from standard input instead of command-line arguments), wrapper shims can be invoked directly by `launch_p1`:
```bash
#!/usr/bin/env bash
# shim_player1.sh: Feeds interactive size prompt, then hands stdin back to runner
(echo "$1"; cat) | ./player1
```

---

## Scenario Library & Canonical Suites

The harness includes built-in scenarios covering the core rubric requirements:

| Group | Scenario IDs | Description |
|---|---|---|
| **3 × 3 Win Scenarios** | `G01` – `G08` | Top/middle/bottom row wins, left/middle/right column wins, main/other diagonal wins. |
| **Player 2 Win** | `G09` | Second player (O) wins on move 6. |
| **Draws** | `G10`, `G12` | Full-board draw and non-winning mixed lines. |
| **Edge Win** | `G11` | Win on move 9 (final empty cell); verifies win takes precedence over draw. |
| **$N \times N$ Grids** | `N01_4`, `N03_4`, `N01_5` | 4 × 4 and 5 × 5 games verifying $N$-mark wins. |
| **CLI Argument Errors** | `E01`, `E02_*` | Missing grid size argument, non-numeric size (`abc`), zero, negative size. |
| **In-Game Error Handling** | `E03` – `E06` | Out-of-bounds coordinates, occupied cell attempts, non-numeric strings, and recovery. |

Predefined suites include:
- `lab-2`: Canonical suite covering core P1 rubric requirements.
- `games-3x3`: All 3 × 3 game scripts (`G01`–`G12`).
- `games-nxn`: $N \times N$ game patterns for $N=4$ and $N=5$.
- `errors`: All CLI and in-game input error scenarios.
- `cli`: Command-line argument tests.

---

## Report Structure

Each generated `report.html` is a standalone, self-contained HTML file:
1. **Submission Metadata:** Source directory, topology, platform info, launch commands, run timestamp.
2. **Build Evidence:** Build command, duration, compiler output, and success/failure status.
3. **Scenario Index:** Table of all executed scenarios with statuses (`completed`, `build error`, `launch error`, `timed out`, `interrupted`).
4. **Side-by-Side Checkpoint Cards:**
   - Left: Expected reference board, expected next turn, expected game outcome.
   - Right: Observed terminal output attributed to process, plus optional extracted normalized board.
   - Pacing boundary notation (e.g. `turn prompt matched`, `quiet window settled`).
5. **IPC Leak Summary:** Differences in active shared-memory segments before and after the run.
6. **Full Transcripts & Exit Codes:** Complete unedited PTY transcripts and process termination details.

---

## Testing & Quality Assurance

Run the complete test suite (unit tests, integration tests, and end-to-end sample submission tests):
```bash
uv run pytest -v
```
