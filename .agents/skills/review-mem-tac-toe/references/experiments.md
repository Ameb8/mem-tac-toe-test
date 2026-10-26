# Independent experiments

## Faithful execution and coverage

Establish actual launch commands, cwd, required key files/relative paths, and wrapper behavior first. Start both roles in the documented order. Map each mark and input stream to its process. A single executable may intentionally run twice; fork-based play uses its shared terminal rather than imaginary independent input streams.

Prefer bounded direct PTY interaction using an available tool such as `pexpect`, or repository PTY helpers after inspecting their API. Send one move at a time when the intended player is ready, in the actual input syntax. Keep stdin open through normal play. A preloaded pipe can introduce EOF, buffering, and interleaving differences; use pipes for probes specifically testing those conditions and label them accordingly.

Set finite startup, interaction, and total deadlines; bound captured output. Use CPU/memory limits for runaway or extreme-input probes. A short timeout is an observation, not proof of deadlock. Repeat timing-sensitive cases under controlled pacing, including a patient ordinary game, without indefinitely retrying an unchanged probe. Waiting for player input is normal. Distinguish an extra-move requirement after a result from evidence harvested before natural exit.

Use fresh sessions for independent games. Asymmetric coordinates distinguish orientation mistakes from interface conventions. After rejected input, verify unchanged board/turn, then send valid input and continue enough to establish recovery. Test both players because parsing and result paths can differ. Compare diagnostic meaning rather than exact wording.

Cover all eight 3 × 3 win lines, either player winning, full-board draw, last-cell win, and mixed nonwinning lines. For N = 4 and 5 check board dimensions, coordinates outside the 3 × 3 area, and N marks required before a win. Use `docs/test-cases.md` for scripts/probe selection, independently checking expected outcomes. Small/huge sizes and undocumented caps may need interpretation; do not invent a minimum size or require unlimited allocation.

Correlate source with actual PIDs, relationships, and shared-memory operations. Matching boards alone do not prove shared-memory communication; the creator may differ from the starting player. Trace output writers if shared-terminal attribution materially affects assessment; otherwise mark attribution uncertain. Observe IPC before launch, after natural termination, and after reviewer cleanup, accounting for POSIX mappings/semaphores and System V resources.

## Isolate causes and harness mismatches

Preserve the original anomaly and recreate its trigger. Compare correctly paced direct interaction with actual dispatched harness inputs. Identify the adaptation that changes the outcome. An adaptation can explain a harness mismatch while revealing a separate violation, such as stdin-only grid size.

Follow the affected implementation path: argument conversion/bounds, input stream recovery, board storage/indexing, win counting/draw precedence, creator updates, outcome notifications, shared-state ownership/initialization, turn synchronization, and resource/error paths. Check that shared allocation is large enough and actually holds required state. An unchecked return value is an inspection risk until its consequence is established; safe failure injection or tracing can strengthen evidence.

Use a debugger, syscall trace, sanitizers, or diagnostic build when it resolves material uncertainty. Keep builds in temporary directories, preserving original source content. Record compiler, target, flags, hashes, and instrumentation. Do not substitute diagnostic-build behavior for supplied-binary observations. A change in optimization or instrumentation can mask races.

An optional selective harness retest must use a separate review profile with **no build command**, preserving the original launch configuration unless a justified adaptation is being tested. Inspect current CLI help, use existing binaries, and save newly named reports under `review-evidence/`. Never overwrite supplied binaries, original profiles/reports, or run build commands in place.

## Bound side effects and retain evidence

Give probes `E01`-style IDs. Retain purpose, exact command or replay script, cwd/environment, input bytes, timing/readiness, expected state/outcome, observed output by process, natural exit/signal versus reviewer intervention, limits, and conclusion. Link full logs/scripts from concise report excerpts. Record repetitions and capture truncation.

Clean up in a `finally`-style path, terminating/reaping only experiment-owned processes and identifiable descendants. Avoid broad `pkill`/`killall` and deleting all user IPC resources. Fixed names/keys can collide; run probes sequentially and establish ownership before resource removal. Preserve leftover-resource evidence before cleanup. Reviewer cleanup signals and IPC deletion are not submission actions.

If dependencies, launch problems, permissions, or missing tools block a probe, record the exact obstacle and continue other experiments and report/source inspection. No particular debugger or execution framework is mandatory. Document resulting uncertainty.
