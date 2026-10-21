"""Extraction of normalized board snapshots from raw terminal output."""

from __future__ import annotations

import re
from typing import Any, Optional

# Regex to strip ANSI terminal escape codes
ANSI_ESCAPE_PATTERN = re.compile(r"\x1B(?:[@-Z\\-_]|\[[0-?]*[ -/]*[@-~])")


class BoardExtractor:
    """Attempts to extract an N x N board representation from raw terminal output.

    Rules from specification:
    - Extraction is purely a formatting operation.
    - Must NOT fill missing cells, correct unexpected marks, or consult the expected board.
    - If no reliable board can be extracted, return None.
    """

    @classmethod
    def strip_ansi(cls, text: str) -> str:
        """Strip ANSI escape sequences from text."""
        return ANSI_ESCAPE_PATTERN.sub("", text)

    @classmethod
    def extract_board(
        cls,
        raw_text: str,
        grid_size: int,
        extraction_config: Optional[dict[str, Any]] = None,
    ) -> Optional[str]:
        """Attempt to extract an N x N normalized board string from terminal output."""
        clean_text = cls.strip_ansi(raw_text)
        lines = [line.strip() for line in clean_text.splitlines() if line.strip()]
        if not lines:
            return None

        # If custom regex pattern provided in profile
        if extraction_config and "row_pattern" in extraction_config:
            try:
                row_pat = re.compile(extraction_config["row_pattern"])
                matched_rows: list[list[str]] = []
                for line in lines:
                    m = row_pat.match(line)
                    if m:
                        matched_rows.append(list(m.groups()))
                if len(matched_rows) >= grid_size:
                    # Take the last grid_size rows
                    candidate = matched_rows[-grid_size:]
                    if all(len(r) == grid_size for r in candidate):
                        return "\n".join(" ".join(c for c in r) for r in candidate)
            except Exception:
                pass

        # Heuristic 1: Pipe / delimiter separated grid rows, e.g. "X | O | ." or "| X | O | . |"
        # Search backwards from the end of output to find the most recent board snapshot
        pipe_rows: list[list[str]] = []
        for line in reversed(lines):
            # Ignore divider lines like "---+---+---" or "===="
            if re.match(r"^[-+=| _\s]+$", line) and not any(ch in line for ch in ("X", "O", "x", "o", ".")):
                continue

            if "|" in line:
                cells = [c.strip() for c in line.split("|")]
                cells = [c for c in cells if c]  # remove empty edge elements
                if len(cells) == grid_size:
                    pipe_rows.append(cells)
                    if len(pipe_rows) == grid_size:
                        break
                else:
                    # If we already started collecting a board and hit a non-matching row, reset
                    if pipe_rows:
                        pipe_rows.clear()
            else:
                if pipe_rows:
                    pipe_rows.clear()

        if len(pipe_rows) == grid_size:
            pipe_rows.reverse()
            # Normalize cell marks if standard, preserving unexpected characters
            return "\n".join(" ".join(cell if cell else "." for cell in row) for row in pipe_rows)

        # Heuristic 2: Whitespace separated single characters, e.g. "X O ." or ". . ."
        # Look for contiguous sequences of N lines with N single tokens
        space_rows: list[list[str]] = []
        for line in reversed(lines):
            tokens = line.split()
            # Each token is typically 1 char or bracketed like "[X]"
            if len(tokens) == grid_size:
                clean_tokens = [t.strip("[](){}") for t in tokens]
                if all(len(t) == 1 for t in clean_tokens):
                    space_rows.append(clean_tokens)
                    if len(space_rows) == grid_size:
                        break
                    continue

            if space_rows:
                space_rows.clear()

        if len(space_rows) == grid_size:
            space_rows.reverse()
            return "\n".join(" ".join(row) for row in space_rows)

        return None
