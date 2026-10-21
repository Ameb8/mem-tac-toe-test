# MemTacToe Test Cases

This document contains potential test cases for MemTacToe testing harness. These are suggesions, not all necassary to include in test harness. See docs/specs/ for canonical test case suite.

## Priorities and grading boundaries

| Priority | Coverage | Rubric connection |
|---|---|---|
| P1 | Build and launch, two players in different processes, shared memory, command-line size, standard-input moves, legal games, board updates, outcomes, ordinary input errors | Direct assignment requirements and core evidence for the rubric |
| P2 | Recovery, lifecycle, synchronization edge cases, repeat runs, operating-system failures | General workability and additional evidence for error handling |
| P3 | Unusual startup, concurrent games, extreme input, small sizes | Additional robustness or clarification of unspecified behavior |

The rubric awards **2 points for error handling**, **2 for printing results**, and **either 5 for a parent/child implementation using `fork()` or 6 for separate processes without `fork()`**. The process alternatives are mutually exclusive; the parent/child route has a maximum of 9/10. Do not invent per-case point deductions: record evidence and assess the rubric categories together.

The notes also say the processes should be different programs. Record whether there are different player programs, one executable launched independently twice, or a parent/child pair. Flag the single-executable interpretation for a consistent instructor decision rather than silently treating it as equivalent to different programs. A fork-based implementation remains assessable through the rubric's explicit alternative.

System V calls such as `shmget()` are encouraged, not mandatory. Other actual shared-memory mechanisms are acceptable evidence; sockets, pipes, files, or two independent boards alone do not satisfy shared-memory communication. Do not require this repository's flags, coordinate-base switching, exact wording, semaphores, timing thresholds, or board decoration.

## Setup and evidence for every submission

- Record the submission identity, platform, build command, launch commands, starting player, coordinate convention, input format, and any documented size limit.
- Build from source in a clean build directory or isolated copy. Keep the original submission intact and identify any repairs separately.
- Use fresh sessions for independent cases. Label each process's output; for shared-terminal arrangements, record when the writer cannot be established.
- Record which process creates shared memory. It may differ from the starting player. Use source inspection or tracing when output cannot establish ownership or process behavior.
- Send moves individually, waiting for the appropriate turn or observable update. Keep standard input open until the game ends except in an EOF test; preloading all moves and closing input can change behavior.
- Set and record reasonable build, startup, and response limits (for example, 10 seconds for a response after both players are ready). A deliberate wait for user input or an absent peer is not itself a failure. Record timeouts and preserve output before terminating only the test's processes.
- Capture both output streams, board snapshots after each accepted move, final messages, exit status, and any manual intervention. Mark cases **pass**, **fail**, **not run**, or **uncertain**, with supporting evidence. A build failure prevents execution; do not claim unrun behavioral cases failed independently.
- Isolate IPC resources where possible; otherwise run submissions sequentially and check for stale resources. Remove only resources attributable to the test, after recording what the submission left behind.

## P1: assignment and rubric cases

### Implementation and basic operation

| ID | Test procedure | Expected result / evidence |
|---|---|---|
| A01 | Build using the submitted instructions on Linux/Unix. Launch the documented player arrangement. | C/C++ source builds and launches. Record missing dependencies, undocumented steps, warnings that affect execution, and any required repairs. |
| A02 | Run a complete game while observing process IDs and inspecting process creation code or traces. | Two distinct processes perform the two player roles. Identify independent launch versus parent/child `fork()` behavior for the 6-point or 5-point route. Two threads in one process are insufficient. |
| A03 | Inspect the shared-memory setup, attachment/inheritance, reads, writes, and synchronization; correlate with a running game. | Both processes actually use the same shared game state. A move made by either becomes visible to the other. Matching terminal displays or shared-memory allocation alone do not prove the board is communicated through it. |
| A04 | Start with size `3` supplied through the documented command-line interface; enter moves from standard input for each player. | A 3 × 3 board is used, and each player's input controls that player's moves. A hardcoded board or size obtained only through an interactive prompt does not fulfill the command-line requirement. |
| A05 | Play several alternating moves, including corners, center, and an asymmetric location such as `(0,2)`. | Exactly one correct cell changes per valid move; marks belong to the correct player, previous moves persist, and row/column mapping is consistent. No swapped coordinates, disappearing marks, or duplicate moves. |
| A06 | Pause before the first move, then alternate moves with deliberate pauses. | A clean empty board/state exists initially; play alternates, and neither process advances the game without a legal move. Check both processes' turns. |
| A07 | Observe the shared-memory creator after **every** accepted move, including the opponent's moves and the terminal move. | The creator prints the updated grid after each step, with the correct full state. Output solely from the mover or only at game end is insufficient. An initial empty-board print is useful, but is not explicitly required. |
| A08 | Run the first-player win, second-player win, and draw scripts below; capture both processes' final output. | The winner identifies itself as winning; the loser identifies itself as losing. For a draw, both report a draw. Semantically equivalent wording is sufficient. Verify both processes finish or return to a documented next-game state without requiring an extra move. |

### Repeatable 3 × 3 games

All coordinates below are **zero-based `(row,column)`**. Translate them to the submission's convention and syntax. `X` means the first player and `O` the second; map these to the actual marks and processes. Entries are in chronological order, alternating X then O, starting a fresh game for each row. `.` means empty.

For **each** script, compare the creator's printed board after every move against the accumulated moves, verify there is no premature result, then verify the final board and both players' outcome messages. After the final move, no further move should be necessary or accepted into that completed game.

| ID | Scenario | Moves in order | Expected final rows / result |
|---|---|---|---|
| G01 | Top row | `(0,0) (1,0) (0,1) (1,1) (0,2)` | `XXX / OO. / ...`; X wins on move 5 |
| G02 | Middle row | `(1,0) (0,0) (1,1) (0,1) (1,2)` | `OO. / XXX / ...`; X wins on move 5 |
| G03 | Bottom row | `(2,0) (0,0) (2,1) (0,1) (2,2)` | `OO. / ... / XXX`; X wins on move 5 |
| G04 | Left column | `(0,0) (0,1) (1,0) (1,1) (2,0)` | `XO. / XO. / X..`; X wins on move 5 |
| G05 | Middle column | `(0,1) (0,0) (1,1) (1,0) (2,1)` | `OX. / OX. / .X.`; X wins on move 5 |
| G06 | Right column | `(0,2) (0,0) (1,2) (1,0) (2,2)` | `O.X / O.X / ..X`; X wins on move 5 |
| G07 | Main diagonal | `(0,0) (0,1) (1,1) (0,2) (2,2)` | `XOO / .X. / ..X`; X wins on move 5 |
| G08 | Other diagonal | `(0,2) (0,0) (1,1) (0,1) (2,0)` | `OOX / .X. / X..`; X wins on move 5 |
| G09 | Second player wins | `(0,0) (1,1) (1,0) (0,1) (0,2) (2,1)` | `XOX / XO. / .O.`; O wins on move 6 |
| G10 | Full-board draw | `(0,0) (0,1) (0,2) (1,2) (1,0) (2,0) (1,1) (2,2) (2,1)` | `XOX / XXO / OXO`; draw on move 9 |
| G11 | Win on the final empty cell | `(0,0) (0,1) (0,2) (1,0) (1,1) (1,2) (2,1) (2,0) (2,2)` | `XOX / OXO / OXX`; X wins on move 9, **not** a draw |
| G12 | Mixed lines are not wins | `(0,0) (0,1) (0,2) (1,1) (1,0) (2,0)` | `XOX / XO. / O..`; game continues, X's turn |

For G12, continue with `(2,1) (1,2) (2,2)`: final board `XOX / XOO / OXX`, a draw. The earlier mixed row and diagonal must not trigger a win.

### General N × N behavior

Run each following pattern for **N = 4 and N = 5**, supplying N on the command line. Each line describes a complete fresh game. For round `k = 0` through `N-2`, enter X's move and then O's move; finally enter the listed last X move.

| ID | Pattern | X in round k | O in round k | Last X move | Expected result |
|---|---|---|---|---|---|
| N01 | Row | `(0,k)` | `(1,k)` | `(0,N-1)` | X fills the top row and wins |
| N02 | Column | `(k,0)` | `(k,1)` | `(N-1,0)` | X fills the left column and wins |
| N03 | Main diagonal | `(k,k)` | `(k,k+1)` | `(N-1,N-1)` | X fills the main diagonal and wins |
| N04 | Other diagonal | `(k,N-1-k)` | `(k,N-2-k)` | `(N-1,0)` | X fills the other diagonal and wins |

Check that the rendered board has N rows and N columns, including valid coordinates beyond the original 3 × 3 area. Three aligned X marks must **not** end these games. On N = 5, four aligned marks must not end them either: the assignment defines an N-mark win. Every accepted move must still be printed by the creator, and both processes must report the final result correctly. Record unsupported sizes and their diagnostics; a documented cap does not automatically exempt fixed-size behavior from the generalized-grid requirement.

### Ordinary error handling

Test move errors on **both players' turns**. After every rejected move, verify the board and turn are unchanged, then make a valid move and finish a game to check recovery. Clear feedback and continued play are expected for ordinary move mistakes; exact diagnostic wording is not prescribed.

| ID | Test procedure | Expected behavior |
|---|---|---|
| E01 | Omit the required grid-size argument on the invocation responsible for specifying it. | Usage/error and controlled exit, or a clearly documented valid default. Record that a default's acceptability is unspecified; separately verify command-line size support with A04/N01. Do not apply this test to a joiner that legitimately reads N from shared memory. |
| E02 | Try size arguments `abc`, `3abc`, `3.5`, `0`, `-1`, and a decimal integer too large for the implementation's integer type. Run separately. | Invalid sizes are diagnosed and rejected safely, without silently truncating malformed input, crashing, or leaving a partially running game. |
| E03 | Enter a negative row, negative column, row N, and column N, varying only one coordinate at a time. | Out-of-bounds moves are rejected. For one-based interfaces use 0 and N+1 instead. Exercise both lower and upper boundaries. |
| E04 | Try an occupied cell containing the current player's mark, then one containing the opponent's mark. | Neither mark can be overwritten; the same player still owes a valid move. |
| E05 | Supply nonnumeric move input, e.g. `abc`, then a mixed numeric/nonnumeric pair in the expected input format. Follow each with a valid move. | Clear rejection, no stale coordinate reuse, no infinite error loop, and successful recovery. |
| E06 | Supply only the first coordinate; pause, then supply the second using the documented format. Also try an empty line. | No partial move or uninitialized value is applied. Waiting for a second token is valid for token-based input; line-based input may reject and reprompt. A blank line may be ignored or rejected. The assignment does not prescribe line boundaries. |

## P2: general functionality and workability

These cases supplement the rubric evidence. The assignment does not explicitly prescribe peer-crash recovery, signal handling, or a particular cleanup strategy; report limitations without presenting every missing enhancement as a separate assignment violation.

| ID | Test procedure | Expected behavior / observation |
|---|---|---|
| W01 | Finish a win, start a fresh game, finish a draw, then start again with a different N. | Fresh state each time; no old marks, results, turn counters, or size. No manual repair required for ordinary reuse. |
| W02 | Close standard input before the first move and during a later turn, testing each player separately. | Controlled EOF handling without an endless prompt/error loop or false win/draw. Record what happens to the waiting peer and whether intervention is required. |
| W03 | Interrupt each player in separate runs with Ctrl+C; repeat with abrupt termination while its opponent is waiting. | Observe whether the survivor notices, reports interruption, and can exit or recover. Record indefinite waits, orphaned processes, and resources left behind. A specific detection deadline or automatic forfeit is not required. |
| W04 | Wait between moves, then enter the next legitimate move immediately after the preceding update; repeat a short game several times. | No lost wakeups, missed board updates, duplicate moves, or intermittent deadlock. Legitimate idle waiting must remain usable. |
| W05 | Attempt input on the nonactive player's terminal before its turn. Then resume normal play. | No out-of-turn state mutation or extra turn. Input may be queued until that player's turn. For shared standard input, record that a physical typist cannot reliably be attributed to a process. |
| W06 | Observe CPU use while both processes are waiting for input or one is waiting for a turn. | Record sustained busy waiting as a workability/efficiency concern. Do not require a specific semaphore or sleep design. |
| W07 | Inspect processes and submission-owned shared-memory/semaphore resources after normal win and draw, before grader cleanup. Repeat after an interrupted game. | Record leaks separately from resources intentionally retained for reuse. Repeated games should remain workable without resource accumulation or interference. Account for POSIX as well as System V resources as applicable. |
| W08 | Where practical in an isolated environment, induce a shared-memory creation/attachment failure, for example a controlled permission or resource-limit failure. | Clear diagnostic and controlled failure rather than continuing with invalid handles/pointers. Source-review return-value checks if safe injection is impractical; label that evidence as inspection, not execution. |

## P3: additional robustness and unspecified boundaries

| ID | Test procedure | Expected behavior / observation |
|---|---|---|
| R01 | Start the joiner before the creator, then start the creator. Also launch only one player. | A clear error, documented wait/retry, or successful eventual connection is reasonable. Document startup dependencies; unsupported reverse order alone is not an explicit violation. |
| R02 | Try joining with a conflicting size, starting a duplicate creator, or adding a third player. Only apply options the interface exposes. | Existing game state is not silently resized, reset, or corrupted; extra participants are rejected or handled coherently. |
| R03 | Attempt two simultaneous games if the interface claims to support them. | Games remain independent. If fixed IPC keys permit only one game, record that limitation; concurrent sessions are not required. |
| R04 | Test N = 1 and N = 2, a documented maximum, and just beyond it. For huge N, use resource limits rather than permitting uncontrolled allocation. | Supported sizes behave consistently; unsupported sizes fail clearly and safely. The assignment states no minimum or maximum, so record small-size policy for instructor interpretation rather than assuming N must be at least 3. |
| R05 | Try long numeric/text input, integer-overflow coordinates, extra tokens, and whitespace variations. | No crash, memory corruption, accidental stale move, or unrecoverable parsing loop. Extra-token policy may vary; record it rather than imposing a particular line parser. |
| R06 | Restart after abrupt termination with any submission-owned stale resources still present. | Record whether recovery is automatic, clearly diagnosed, or requires documented manual cleanup. Do not remove resources before observing this behavior. |

## Per-submission record

```text
Submission / revision:
Environment and build result:
Launch commands and input convention:
Creator process / starting player:
Process structure and shared-memory evidence:
Cases run and outcome (pass / fail / not run / uncertain):
Evidence references (transcripts, boards, source locations):
Error handling findings (2-point category):
Board and result printing findings (2-point category):
Process implementation findings (5- or 6-point alternative):
General workability and optional robustness findings:
Timeouts, interventions, repairs, and remaining uncertainty:
Resources left by submission / grader cleanup performed:
```
