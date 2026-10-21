"""Unit tests for PTY process handling, extractor, and IPC inspection."""

import sys
import time
import pytest

from mem_tac_review.extractor import BoardExtractor
from mem_tac_review.ipc import IpcInspector
from mem_tac_review.pty_process import PtyProcess, PtySession


def test_board_extractor_pipe_separated() -> None:
    sample_output = """
Game started!
Current Board:
---+---+---
 X | O | . 
---+---+---
 . | X | . 
---+---+---
 . | . | O 
---+---+---
Enter your move:
"""
    extracted = BoardExtractor.extract_board(sample_output, 3)
    assert extracted is not None
    assert extracted == "X O .\n. X .\n. . O"


def test_board_extractor_space_separated() -> None:
    sample_output = """
Round 2:
X O .
. X .
. . .
Move:
"""
    extracted = BoardExtractor.extract_board(sample_output, 3)
    assert extracted is not None
    assert extracted == "X O .\n. X .\n. . ."


def test_board_extractor_unreliable() -> None:
    # Incomplete or non-matching grid should return None
    sample_output = """
Random message without a board
Welcome to Tic-Tac-Toe
Enter coordinates:
"""
    extracted = BoardExtractor.extract_board(sample_output, 3)
    assert extracted is None


def test_ipc_inspector() -> None:
    # Snapshot SysV segments
    snap = IpcInspector.get_sysv_shm_snapshot()
    assert isinstance(snap, dict)
    new_ids, summary = IpcInspector.format_snapshot_diff(snap, snap)
    assert len(new_ids) == 0
    assert "No unreleased shared-memory segments detected." in summary


def test_pty_process_echo_and_interaction() -> None:
    # Test launching a real process via PTY
    # We use a python one-liner that reads a line and echoes it with a prompt
    cmd = [
        sys.executable,
        "-u",
        "-c",
        "import sys; sys.stdout.write('Prompt: '); sys.stdout.flush(); line = sys.stdin.readline(); sys.stdout.write('Echo: ' + line); sys.stdout.flush()",
    ]
    proc = PtyProcess(command=cmd)
    try:
        session = PtySession({"p1": proc})

        # Wait for "Prompt: "
        ok, reason = session.wait_for_boundary("p1", turn_prompt="Prompt:", settling_ms=100, timeout_sec=3.0)
        assert ok is True
        assert "matched" in reason

        unread = session.drain_all()
        assert "Prompt:" in unread["p1"]

        # Send input
        proc.write("hello\n")

        # Wait for "Echo: hello"
        ok2, reason2 = session.wait_for_boundary("p1", settling_ms=100, timeout_sec=3.0)
        assert ok2 is True

        unread2 = session.drain_all()
        assert "Echo: hello" in unread2["p1"]
    finally:
        session.terminate_all()
