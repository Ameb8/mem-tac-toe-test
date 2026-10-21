"""System IPC inspection for shared-memory tracking."""

from __future__ import annotations

import os
from pathlib import Path
import re
import subprocess
from typing import Optional


class IpcInspector:
    """Inspects Linux System V and POSIX shared-memory segments."""

    @classmethod
    def get_sysv_shm_snapshot(cls) -> dict[str, str]:
        """Capture currently active System V shared-memory segments.

        Returns a mapping of shmid -> details line.
        """
        segments: dict[str, str] = {}
        # Try reading /proc/sysvipc/shm directly if available
        proc_shm = Path("/proc/sysvipc/shm")
        if proc_shm.is_file():
            try:
                lines = proc_shm.read_text(encoding="utf-8").strip().splitlines()
                # First line is header:       key      shmid perms                  size  cpid  lpid nattch ...
                for line in lines[1:]:
                    parts = line.split()
                    if len(parts) >= 2:
                        shmid = parts[1]
                        segments[shmid] = line.strip()
                return segments
            except Exception:
                pass

        # Fallback to ipcs -m
        try:
            res = subprocess.run(
                ["ipcs", "-m"],
                capture_output=True,
                text=True,
                timeout=5,
                check=False,
            )
            if res.returncode == 0:
                lines = res.stdout.strip().splitlines()
                # Parse lines after header
                for line in lines:
                    parts = line.split()
                    # Example line: 0x00000000 12345 ameb8 666 4096 2 ...
                    if len(parts) >= 2 and parts[1].isdigit():
                        shmid = parts[1]
                        segments[shmid] = line.strip()
        except Exception:
            pass

        return segments

    @classmethod
    def format_snapshot_diff(
        cls, before: dict[str, str], after: dict[str, str]
    ) -> tuple[list[str], str]:
        """Compare pre-run and post-run snapshots.

        Returns:
            newly_created_ids: list of shmids created during run and still present
            summary: human-readable diff summary
        """
        new_ids = [shmid for shmid in after if shmid not in before]
        summary_lines: list[str] = []
        if new_ids:
            summary_lines.append(f"Detected {len(new_ids)} unreleased shared-memory segment(s) after session:")
            for shmid in new_ids:
                summary_lines.append(f"  shmid {shmid}: {after[shmid]}")
        else:
            summary_lines.append("No unreleased shared-memory segments detected.")

        return new_ids, "\n".join(summary_lines)
