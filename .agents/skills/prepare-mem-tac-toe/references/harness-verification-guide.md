# Harness Verification and Reporting Guide

The `mem-tac-review` harness provides repeatable execution and review evidence for human graders.

> [!IMPORTANT]
> **Core Rule on Evaluation & Fixing:**
> 1. **Do not grade or evaluate game logic correctness:** The agent reviews harness results *only* to confirm whether the harness ran correctly (operational delivery). Whether the implementation correctly detects wins, draws, or renders the board is evaluated by the human instructor using the generated report.
> 2. **Do not modify or fix student code:** If an implementation fails to compile, crashes, deadlocks, or cannot be run by the harness, document the exact failure and stop.

---

## 1. Verifying Harness Execution

When the harness executes:
```bash
uv run mem-tac-review run --profile profiles/<name>.yaml --suite lab-2 --output reports/<name> -v
```

Inspect the generated `reports/<name>/report.json` and check:

### A. Report Generation
- Did the command exit with code 0?
- Was `reports/<name>/report.html` created?
- Was `reports/<name>/report.json` created?

### B. Build Result (`report.json -> build_result`)
- `build_result.success`: `true` or `false`
- `build_result.exit_code`: `0` for success
- `build_result.output`: Compiler output (check for compilation warnings or errors)

If `build_result.success` is `false`:
- The harness correctly recorded a build failure.
- All scenarios are marked `build error`.
- Record the compiler error output in your summary and state that the harness could not run scenarios due to implementation build failure.

### C. Scenario Statuses (`report.json -> scenario_results`)

Each scenario has a `status`:

| Status | Meaning | What to Check |
|---|---|---|
| `completed` | The harness completed all scripted interaction steps and recorded all checkpoints. | Harness executed normally. |
| `timed_out` | A turn or action did not produce output/settle within `timeout_sec`. | Check transcripts: Did the student process deadlock, crash silently, or wait on an unexpected input prompt? Or was `timeout_sec` too short? |
| `launch_error` | The binary or command in `launch_p1`/`launch_p2` could not be started by the OS. | Check file permissions (`chmod +x`), paths, or missing dynamic libraries. |
| `build_error` | Scenarios were not run because compilation failed. | Implementation compiler error. |
| `interrupted` | The process terminated prematurely (e.g. segmentation fault). | Check `processes[...].exit_code` and transcripts for error signals (e.g. exit code 139 for SIGSEGV). |

### D. IPC Leak Detection (`report.json -> ipc_leaks_detected`)
- Lists any System V shared memory segment IDs (`shmid`) created during the run that were not removed (`IPC_RMID`) by the student program before exit.
- Note any leaked segments in your final summary.

---

## 2. Distinguishing Issues

Always distinguish between **Harness Issues** and **Implementation Issues**:

### Harness Operational Issues (Harness problem)
- Python crash / traceback when running `mem-tac-review`.
- Failure in pseudo-terminal allocation (`pty.openpty()`).
- Report directory write permission errors.
- Profile parsing failure due to invalid harness schema.

### Implementation Issues (Student submission problem)
- **Compilation failure:** Code fails to compile under standard compilers.
- **Premature crash / Segfault:** Process exits unexpectedly before completing game moves.
- **Interactive hang / Deadlock:** Implementation stops responding due to semaphore locks or unhandled stdin formatting.
- **Resource leak:** Shared memory segments left unreleased in the kernel.

---

## 3. Reporting Findings to the User

Present findings concisely using the following structure:

```markdown
### MemTacToe Review Summary: <submission_name>

- **Harness Operational Status:** [Ran Successfully / Operational Failure]
- **Reports Generated:**
  - HTML Report: `reports/<name>/report.html`
  - JSON Evidence: `reports/<name>/report.json`
- **Build Status:** [Success (exit code 0) / Failed (exit code N)]
- **Scenario Execution:**
  - Completed: `X / Y` scenarios
  - Timed Out: `N` scenarios (List IDs)
  - Launch/Build Errors: `N` scenarios
- **Shared Memory Leaks:** [None detected / N segments leaked]

#### Implementation Observations (Operational Only):
- <Detail any compilation errors, crashes, hangs, or argument mismatches observed during the run>
- *(Note: Game logic correctness, win/draw validity, and rubric grading are reserved for human instructor review via the generated HTML report.)*
```
