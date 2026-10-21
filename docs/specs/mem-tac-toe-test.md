# Mem-tac-toe testing and review program

## Purpose

Provide repeatable execution and review evidence for multiple implementations of the assignment in [lab-2.md](lab-2.md). The program runs scripted interactions and produces a report that places expected board states alongside the implementation's observed board output.

The grader decides whether the implementation is correct and how it satisfies the rubric. The program does not assign grades, declare behavioral tests passed or failed, calculate differences between boards, or infer compliance from output.

The main benefit is reducing repeated building, launching, typing, and evidence collection while keeping the grader in control of interpretation.

## Scope

The initial version supports Linux C/C++ submissions with terminal interfaces, including:

- Two independently launched player programs.
- A single executable launched twice with different arguments or roles.
- A launcher that creates parent and child players, potentially sharing one terminal.
- Different executable names, coordinate bases, input formats, prompts, and board renderings.

It covers scripted games, input-error scenarios, bounded execution, and portable review reports. It supports individual submissions and batches using the same scenarios.

Automatic grading, exhaustive game exploration, a universal board parser, source-code analysis, and automatic proof of shared-memory use are outside the initial scope. Process topology, board output, or messages alone must not be presented as proof of assignment compliance.

## Main concepts

| Concept | Meaning |
|---|---|
| Submission | One implementation and its source/build directory |
| Profile | Instructions for building, launching, and interacting with a submission |
| Scenario | An ordered sequence of game actions and review checkpoints |
| Reference board | The expected game state calculated independently from the submission |
| Session | The processes, terminal connections, and captured output for one scenario run |
| Checkpoint | A point in the scenario at which expected state and observed evidence are recorded together |
| Report | A self-contained review artifact containing checkpoints, transcripts, and execution details |

## High-level design

The program has five responsibilities:

1. **Scenario library:** describes games and unusual input without depending on a particular submission's interface.
2. **Submission profiles:** translate those scenarios into build commands, launch commands, input text, and interaction boundaries.
3. **Session runner:** manages processes via pseudo-terminals (PTYs), sends input, collects output, and limits execution time.
4. **Reference model:** calculates expected boards, turns, and outcomes using the game rules.
5. **Report generator:** presents expected states and observed evidence for human review.

Scenarios and the reference model are shared across submissions. Interface differences belong in profiles. The runner does not depend on a submission's internal shared-memory layout or source-level functions.

## Submission profiles

A profile is a declarative configuration file (YAML or JSON) that records all information needed to build, launch, pace, and interact with one submission:

- **Identity & Build:** Submission location, build command, working directory, and environment settings.
- **Process Topology:** Declares whether the submission runs as two independent processes (`dual`) or a single launcher/fork sharing a terminal (`single`).
- **Launch Commands:** Launch command arrays for Player 1 and Player 2, including `{N}` placeholder substitution for grid size.
- **Roles & Ownership:** Identifies which player starts, which mark each player uses, and which output connection belongs to the shared-memory creator (`p1`, `p2`, or `shared`).
- **Coordinate Conventions:** Coordinate base (`0` or `1`), coordinate order (`row_col` or `col_row`), and move input format template (e.g. `"{r} {c}\n"`, `"{r},{c}\n"`, or separate prompts `["{r}\n", "{c}\n"]`).
- **Interaction Pacing:** Move prompt regex pattern and quiet-window settling interval (`settling_ms`).
- **Execution Limits:** Per-action response timeouts and maximum session duration.

The profile explicitly distinguishes declared roles from observed process information. For example, a profile may identify a terminal as the creator's terminal without the program claiming to have verified shared-memory creation.

### Profile Schema Example

```yaml
name: "student_submission_a"
build_cmd: "make clean && make"
work_dir: "."

# Topology: "dual" (two separate processes) or "single" (fork / shared terminal)
topology: "dual"
launch_p1: ["./player1", "{N}"]
launch_p2: ["./player2"]
creator: "p1"

# Coordinate conventions
coord_base: 0                 # 0-based or 1-based indexing
coord_order: "row_col"        # "row_col" or "col_row"
input_format: "{r} {c}\n"     # Input string template or list of prompts

# Turn boundary & pacing
turn_prompt: "Enter move:"    # Optional regex to pace interaction
settling_ms: 100              # Quiet-window settling duration in milliseconds
timeout_sec: 10               # Maximum seconds to wait per turn
```

### Adapters and the Shim Pattern

Common differences must be expressible purely through profile configuration. For unusual interfaces that deviate from standard conventions (e.g., an implementation requiring board size via an interactive prompt rather than CLI arguments, or requiring custom environment setups), custom interaction adapters should be implemented as lightweight external shell wrapper scripts ("shims") invoked directly by the profile's launch commands:

```bash
#!/usr/bin/env bash
# shim_player1.sh: Feeds interactive size prompt, then hands stdin back to runner
(echo "$1"; cat) | ./player1
```

This "shim escape hatch" prevents the central runner from accumulating bespoke conditional code for individual edge cases.

Profiles and shims must never require changing a student's source code to add a test protocol. If a submission needs repair before it can run, any repaired variant must be identified separately from the original.

## Scenarios and expected states

Scenarios use a consistent internal coordinate system and logical player identities. Profiles translate them into the submission's conventions.

A scenario specifies grid size, starting player, ordered actions, and checkpoints. Actions include ordinary moves, deliberately invalid coordinates, malformed input, and intentional end-of-input where relevant.

The reference model maintains the board, next player, and game outcome. A win requires an entire row, column, or principal diagonal of N marks on an N × N board. A full board without a win is a draw. Ordinary game scenarios stop at their expected terminal outcome.

Expected state follows the scripted actions, not the submission's output. A deliberately invalid action leaves the expected board and turn unchanged. Reports must state this distinction: a later expected state does not imply that the submission accepted or rejected any earlier input correctly.

Before execution, the program validates scenario consistency. An accidental occupied-cell move or a move after an expected win is a scenario error unless explicitly designated as an intentional invalid action. These checks concern the test data only.

The initial scenario library includes:

| Scenario group | Review evidence |
|---|---|
| First player wins | Expected boards and final outcome for rows, columns, and both diagonals |
| Second player wins | Expected boards and each process's final output |
| Draw | A full board without a winner and final output from both players |
| Larger grids | Games on 4 × 4 and 5 × 5 boards, including a checkpoint with three aligned marks before an N-mark win |
| Invalid coordinates | Negative and out-of-range input followed by valid moves |
| Occupied cells | Attempts to reuse cells occupied by either player, followed by valid moves |
| Malformed input | Non-numeric and incomplete input, with a defined recovery sequence |
| Command-line errors | Missing, non-numeric, and invalid grid-size arguments, with output and termination evidence |

Not every scenario has a board checkpoint. Command-line scenarios present the supplied arguments and captured diagnostics instead. The report describes the expected behavior for review without asserting that it occurred.

Optional robustness scenarios, such as peer termination or startup in an unusual order, are labeled separately from explicit assignment requirements. Submission-specific options such as coordinate-base switches are not treated as requirements for other implementations.

## Interaction and checkpoint collection

Each scenario runs in a fresh session. The runner launches the configured programs connected to **pseudo-terminals (PTYs)** (such as via Python's `pty.openpty()`) rather than standard OS pipes (`pipe()` / `subprocess.PIPE`). In Linux C/C++, standard I/O library buffers stdout in 4KB blocks when connected to a pipe, which suppresses move prompts and board updates until process exit or an explicit `fflush(stdout)`. Attaching processes to pseudo-terminals ensures standard line-buffered interactive terminal behavior and reliable real-time output capture without modifying the student's source code.

Interaction uses observable boundaries where available. A boundary signals when to send the next input or capture a checkpoint; it is not a correctness assertion. For example, seeing a move prompt may permit the next action without proving whose turn it is.

For interfaces with missing, variable, or non-deterministic prompt strings, the profile configures a **quiet-window settling interval** (typically 80–150 ms of silence on the PTY after receiving output). Once the terminal stream goes quiet for this duration, the runner captures the checkpoint and delivers the next scripted action. The report identifies this pacing method where applied. Inputs are sent individually rather than preloaded as a single input block, and the input stream remains open unless the scenario intentionally tests EOF.

Every checkpoint records:

- The action just sent, its intended player, and the exact submitted text.
- The expected board, next player, and expected outcome at that point.
- The observed output since the preceding checkpoint, identified by process or terminal connection.
- Any board snapshot extracted from that output, with its source evidence.
- How the checkpoint boundary was chosen and any uncertainty about its association with the action.

Output may contain initial boards, duplicate boards, or delayed updates. The report preserves multiple observed snapshots rather than silently choosing whichever resembles the expected board. If an output interval cannot be associated confidently with one action, that limitation is visible.

If a session exits early or stops responding, the runner records the available evidence and stops dependent actions. Remaining actions are listed as not executed, with their planned expected states clearly separated from observed checkpoints. Divergent output alone does not stop a session unless interaction can no longer proceed.

## Presenting the actual board

The primary observed evidence is the submission's own terminal rendering. At every checkpoint, the report places the expected board beside the captured board/output excerpt in a monospace presentation.

Profiles may optionally extract a board into a normalized grid for easier reading. Extraction is purely a formatting operation: it must not fill missing cells, correct unexpected marks, or consult the expected board to resolve ambiguity. The original excerpt remains available beside any normalized representation.

If no reliable board can be extracted, the report shows the raw excerpt and says that no normalized board is available. Missing or ambiguous output is never replaced with an inferred board. This allows unfamiliar output formats to remain reviewable without requiring a parser for every submission.

Terminal display changes, such as clearing the screen or moving the cursor, must be preserved in the raw capture and represented by readable checkpoint snapshots where supported. Any display limitation is disclosed.

## Report

The primary artifact is a self-contained HTML report that can be opened locally without a running server or internet access. A batch report links to individual submission reports using relative paths. Captured output is escaped and treated as text.

Each submission report contains:

- Submission identity and source revision or content identifier where available.
- The profile and scenario versions used, launch/build commands, and run time.
- Build output and whether execution could begin.
- An index of scenarios with execution status and links to their evidence.
- Expected-versus-observed checkpoint views in action order.
- Final output from each process or terminal, alongside the expected game outcome.
- Complete transcripts and process exit information.
- Startup issues, timeouts, collection limitations, actions not executed, and cleanup results.

Execution statuses include **completed**, **build error**, **launch error**, **timed out**, **interrupted**, and **not run**. “Completed” means that the planned interaction was delivered and evidence collection finished; it does not mean the implementation is correct. Normal and unexpected termination details remain visible independently of that status.

There are no correctness scores, pass/fail badges, board-difference highlights, or automatic rubric conclusions. Expected boards and expected outcomes are labeled as reference information. The report retains process attribution where available and explicitly labels shared-terminal output when individual writers cannot be distinguished.

An illustrative checkpoint:

```text
Scenario: first player wins on the main diagonal
Action 3: send X's move (1, 1)
Submitted input: "1 1\n"

Expected board                 Observed creator-terminal output
X O .                          [original captured rendering]
. X .
. . .

Expected next player: O
Collection boundary: next configured move prompt
```

## Usage

The intended workflow is:

1. Select or create a submission profile using its documented usage and a short initial inspection.
2. Run one simple game to check that launching, input formatting, and capture work.
3. Adjust the profile if the interface requires it.
4. Run the shared scenario set for that submission or a batch of configured submissions.
5. Open the report, compare the boards and messages, and record grading decisions separately.

The following commands illustrate the proposed interface; they do not describe an existing executable:

```bash
mem-tac-review run --profile profiles/student-a.yaml \
  --scenario diagonal-win --output reports/student-a-trial

mem-tac-review run --profile profiles/student-a.yaml \
  --suite lab-2 --output reports/student-a

mem-tac-review batch --profiles profiles/ \
  --suite lab-2 --output reports/cohort
```

Users can select scenarios, override time limits, and rerun a scenario without overwriting earlier evidence. A batch continues after a submission cannot build or a session times out.

The command's exit status indicates whether the requested execution/reporting operation completed or encountered an operational problem. It never indicates that a submission's game behavior is correct.

## Execution boundaries and cleanup

Builds and sessions have bounded execution time and output storage. The runner tracks all processes it starts, including descendants, and terminates and reaps them when a session ends. Reports survive interrupted or incomplete sessions whenever possible.

Use isolated environments with private IPC namespaces for batch execution where available. If isolation is unavailable, run submissions sequentially and clearly identify the limitation; sequential execution alone does not isolate hardcoded IPC resources from unrelated programs.

Cleanup targets only resources attributable to the session. The program must not broadly delete shared-memory segments or temporary files belonging to other work. Record observed resource state before runner cleanup separately from cleanup actions, so intervention does not conceal what the submission left behind.

Running a submission executes its build scripts and programs. The chosen execution environment must be documented; the runner must not claim to provide a security sandbox merely because it manages processes and timeouts.

## Initial delivery and acceptance criteria

The first version is ready when:

- Two independently structured submissions can run the same logical scenarios through different profiles.
- Both separate-terminal and shared-terminal player arrangements can be represented.
- Each executed checkpoint pairs an independently generated expected board with attributable observed evidence.
- A submission can produce a useful report without a custom board parser.
- Optional normalized boards retain their original evidence and expose extraction limitations.
- Build errors, early exits, missing prompts, and timeouts produce reviewable reports and do not block the rest of a batch.
- Reports contain no automated correctness or grading conclusions.
- Session cleanup is bounded and does not remove unrelated resources.
- A saved report remains readable after execution has ended.

Pilot the first version on a few submissions with different interfaces before expanding configuration options. The main measure of usefulness is reduced grading effort after profile setup, while preserving enough original evidence for the grader to make an independent decision.
