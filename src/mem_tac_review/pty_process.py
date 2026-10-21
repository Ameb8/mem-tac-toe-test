"""PTY Process and interaction management."""

from __future__ import annotations

import errno
import os
import pty
import re
import select
import signal
import subprocess
import time
from typing import Optional


class PtyProcess:
    """A process running attached to a pseudo-terminal (PTY)."""

    def __init__(
        self,
        command: list[str],
        work_dir: str = ".",
        env: Optional[dict[str, str]] = None,
        role: str = "p1",
    ) -> None:
        self.command = command
        self.work_dir = work_dir
        self.role = role
        self.env = os.environ.copy()
        if env:
            self.env.update(env)

        self.master_fd: Optional[int] = None
        self.process: Optional[subprocess.Popen] = None
        self.accumulated_output: str = ""
        self.unread_output: str = ""
        self.is_closed = False
        self._spawn()

    def _spawn(self) -> None:
        master_fd, slave_fd = pty.openpty()
        self.master_fd = master_fd

        # Set master non-blocking
        os.set_blocking(master_fd, False)

        try:
            self.process = subprocess.Popen(
                self.command,
                cwd=self.work_dir,
                env=self.env,
                stdin=slave_fd,
                stdout=slave_fd,
                stderr=slave_fd,
                preexec_fn=os.setsid,  # New process group for reliable cleanup
                close_fds=True,
            )
        finally:
            # Always close slave fd in parent process
            os.close(slave_fd)

    @property
    def pid(self) -> Optional[int]:
        return self.process.pid if self.process else None

    def poll(self) -> Optional[int]:
        if not self.process:
            return None
        return self.process.poll()

    def is_alive(self) -> bool:
        return self.poll() is None

    def read_available(self) -> str:
        """Read all available data from PTY master fd without blocking."""
        if self.master_fd is None or self.is_closed:
            return ""

        chunks: list[str] = []
        while True:
            try:
                data = os.read(self.master_fd, 4096)
                if not data:
                    break
                text = data.decode("utf-8", errors="replace")
                chunks.append(text)
            except (BlockingIOError, InterruptedError):
                break
            except OSError as e:
                # Under Linux, reading from a master PTY after child exit raises EIO
                if e.errno == errno.EIO:
                    break
                raise

        new_text = "".join(chunks)
        if new_text:
            self.accumulated_output += new_text
            self.unread_output += new_text
        return new_text

    def write(self, text: str) -> None:
        """Send input text to child process stdin via PTY master."""
        if self.master_fd is None or self.is_closed:
            raise RuntimeError("Cannot write to closed PTY")
        data = text.encode("utf-8")
        os.write(self.master_fd, data)

    def close_stdin(self) -> None:
        """Send EOF (EOT character \x04) to child terminal."""
        if self.master_fd is not None and not self.is_closed:
            try:
                os.write(self.master_fd, b"\x04")
            except OSError:
                pass

    def drain_unread(self) -> str:
        """Return and clear output buffered since last checkpoint."""
        self.read_available()
        text = self.unread_output
        self.unread_output = ""
        return text

    def terminate(self, timeout_sec: float = 2.0) -> int:
        """Terminate process and all descendants in process group, then close PTY."""
        exit_code = None
        if self.process and self.process.pid:
            pid = self.process.pid
            try:
                # Send SIGTERM to process group
                os.killpg(pid, signal.SIGTERM)
            except (ProcessLookupError, PermissionError):
                pass

            # Wait for exit
            start = time.monotonic()
            while time.monotonic() - start < timeout_sec:
                exit_code = self.process.poll()
                if exit_code is not None:
                    break
                time.sleep(0.05)

            # If still alive, send SIGKILL
            if self.process.poll() is None:
                try:
                    os.killpg(pid, signal.SIGKILL)
                except (ProcessLookupError, PermissionError):
                    pass
                exit_code = self.process.wait()
            else:
                exit_code = self.process.poll()

        # Read any remaining output before closing
        self.read_available()

        if self.master_fd is not None:
            try:
                os.close(self.master_fd)
            except OSError:
                pass
            self.master_fd = None

        self.is_closed = True
        return exit_code if exit_code is not None else 0


class PtySession:
    """Manages one or two PTY processes and interactions for a scenario run."""

    def __init__(self, processes: dict[str, PtyProcess]) -> None:
        self.processes = processes  # e.g. {"p1": PtyProcess, "p2": PtyProcess}

    def read_all(self) -> dict[str, str]:
        """Read newly available output from all processes."""
        result: dict[str, str] = {}
        for role, proc in self.processes.items():
            proc.read_available()
            result[role] = proc.unread_output
        return result

    def drain_all(self) -> dict[str, str]:
        """Drain and clear unread output from all processes."""
        result: dict[str, str] = {}
        for role, proc in self.processes.items():
            result[role] = proc.drain_unread()
        return result

    def get_transcripts(self) -> dict[str, str]:
        """Get full transcripts accumulated so far."""
        for proc in self.processes.values():
            proc.read_available()
        return {role: proc.accumulated_output for role, proc in self.processes.items()}

    def are_all_dead(self) -> bool:
        """Check if all processes have exited."""
        return all(not proc.is_alive() for proc in self.processes.values())

    def wait_for_boundary(
        self,
        target_role: str,
        turn_prompt: Optional[str] = None,
        settling_ms: int = 100,
        timeout_sec: float = 10.0,
    ) -> tuple[bool, str]:
        """Pace interaction until an observable boundary is reached.

        Pacing rules from spec:
        - If turn_prompt is set and matched in target role's new output -> boundary reached.
        - If quiet-window settling interval (settling_ms) elapses with silence after some output -> boundary reached.
        - If all processes exit -> boundary reached (process exited).
        - If timeout_sec elapses -> timed out.

        Returns (success: bool, boundary_reason: str).
        """
        start_time = time.monotonic()
        settling_sec = settling_ms / 1000.0
        prompt_re = re.compile(turn_prompt) if turn_prompt else None

        target_proc = self.processes.get(target_role)

        # Initialize with unread output currently waiting in buffer
        output_since_start: dict[str, str] = {
            role: proc.unread_output for role, proc in self.processes.items()
        }
        has_received_data = any(len(txt) > 0 for txt in output_since_start.values())
        last_received_time: Optional[float] = time.monotonic() if has_received_data else None

        while True:
            # Poll all processes
            fds = [proc.master_fd for proc in self.processes.values() if proc.master_fd is not None]
            if not fds or self.are_all_dead():
                # Read whatever remains
                for proc in self.processes.values():
                    new_text = proc.read_available()
                    if new_text:
                        output_since_start[proc.role] += new_text
                return True, "all processes exited"

            # Check prompt regex if configured on output received
            if prompt_re:
                for role, recent in output_since_start.items():
                    if prompt_re.search(recent):
                        return True, f"turn prompt matched '{turn_prompt}' on {role}"

            # Select on available file descriptors with small slice (20ms)
            rlist, _, _ = select.select(fds, [], [], 0.02)
            now = time.monotonic()

            for proc in self.processes.values():
                if proc.master_fd in rlist:
                    new_text = proc.read_available()
                    if new_text:
                        last_received_time = now
                        has_received_data = True
                        output_since_start[proc.role] += new_text

            # Check prompt regex again after reading
            if prompt_re:
                for role, recent in output_since_start.items():
                    if prompt_re.search(recent):
                        return True, f"turn prompt matched '{turn_prompt}' on {role}"

            # Check quiet window settling
            if has_received_data and last_received_time is not None:
                if (now - last_received_time) >= settling_sec:
                    return True, f"quiet window settled ({settling_ms}ms silence)"

            if not has_received_data and (now - start_time) >= max(settling_sec, 0.2):
                if not prompt_re or (now - start_time) >= settling_sec * 3:
                    return True, f"quiet window settled ({settling_ms}ms initial)"

            # Check timeout
            if (now - start_time) >= timeout_sec:
                return False, f"timed out after {timeout_sec:.1f}s"

    def terminate_all(self) -> None:
        """Terminate all processes in the session."""
        for proc in self.processes.values():
            proc.terminate()
