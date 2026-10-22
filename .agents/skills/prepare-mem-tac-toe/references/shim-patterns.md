# Shim Patterns and Adapters

The **Shim Pattern** is an escape hatch used when an implementation's launch or input protocol deviates from standard command-line conventions, without modifying the student's source code.

---

## What is a Shim?

A shim is a small wrapper script (usually a Bash script) placed in the submission directory or profile directory. It adapts the test harness launch invocation to the student program's specific expectations.

> [!IMPORTANT]
> **Zero Code Modification Rule:** Shims must wrap or adapt process execution externally. Never edit or patch the student's C/C++ source code directly.

---

## Pattern 1: Interactive Grid Size via Stdin

### Problem
The student's program does not read the board size from `argv[1]`, but instead prompts the user on standard input:
```text
Enter board size: 
```

### Solution: Stdin Feeder Shim (`shim_p1.sh`)
```bash
#!/usr/bin/env bash
# shim_p1.sh: Feeds interactive size prompt, then hands stdin back to runner
GRID_SIZE="$1"
shift

# Pipe the grid size first, then keep stdin open for interactive runner moves
(echo "$GRID_SIZE"; cat) | ./player1 "$@"
```

### Profile Configuration
```yaml
launch_p1: ["./shim_p1.sh", "{N}"]
```
Make sure `shim_p1.sh` is executable (`chmod +x shim_p1.sh`).

---

## Pattern 2: Dynamic Game Key / Shm-Name Generation

### Problem
The program requires a unique shared-memory name or key on the command line (e.g. `./game 3 my_key`), but running concurrent or rapid sequential tests causes collisions with leftover segments.

### Solution: Key Injection Shim (`shim_game.sh`)
```bash
#!/usr/bin/env bash
# shim_game.sh: Injects a unique key or shared name based on PID or timestamp
GRID_SIZE="$1"
ROLE="$2"
SHM_NAME="/ttt_shm_${USER}_$$"

./game "$GRID_SIZE" "$SHM_NAME" "$ROLE"
```

### Profile Configuration
```yaml
launch_p1: ["./shim_game.sh", "{N}", "1"]
launch_p2: ["./shim_game.sh", "{N}", "2"]
```

---

## Pattern 3: Stale IPC Token Cleanup

### Problem
The program creates an `ftok` token file (e.g. `touch ttt_token.key`) or leaves named semaphores behind (`/dev/shm/sem.*`), causing subsequent scenario runs to fail with `EEXIST`.

### Solution: Pre-Launch Cleanup Shim
```bash
#!/usr/bin/env bash
# Clean up any stale token files before launching
rm -f ttt_*.key /dev/shm/*${USER}* 2>/dev/null || true
exec ./player1 "$@"
```

---

## Checklist for Shims

1. Make sure the script starts with `#!/usr/bin/env bash`.
2. Give execution permissions: `chmod +x <path_to_shim>`.
3. If using `work_dir`, place the shim in `work_dir` or refer to it via absolute/relative path.
4. Test running the shim manually once to ensure it doesn't have bash syntax errors.
