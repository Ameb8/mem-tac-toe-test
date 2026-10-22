#!/usr/bin/env python3
"""Helper script to inspect a MemTacToe C/C++ submission and gather profile clues."""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import re
import sys
from typing import Any


def analyze_file(file_path: Path) -> dict[str, Any]:
    text = file_path.read_text(encoding="utf-8", errors="ignore")
    lines = text.splitlines()

    has_main = bool(re.search(r"\bint\s+main\s*\(", text))
    has_fork = bool(re.search(r"\bfork\s*\(", text))
    has_sysv_shm = bool(re.search(r"\bshmget\s*\(", text))
    has_posix_shm = bool(re.search(r"\bshm_open\s*\(", text))
    has_mmap = bool(re.search(r"\bmmap\s*\(", text))
    has_sysv_sem = bool(re.search(r"\bsemget\s*\(", text))
    has_posix_sem = bool(re.search(r"\bsem_open\s*\(", text))
    needs_pthread = bool(re.search(r"\b(pthread_|sem_open|sem_wait)", text))
    needs_rt = bool(re.search(r"\b(shm_open|shm_unlink|clock_gettime)", text))

    prompts: list[str] = []
    coord_clues: list[str] = []
    arg_clues: list[str] = []

    for line in lines:
        stripped = line.strip()
        # Prompt search
        if any(kw in stripped.lower() for kw in ["enter", "prompt", "move:", "coord", "turn:"]):
            if any(call in stripped for call in ["printf", "puts", "cout", "fprintf", "write"]):
                prompts.append(stripped)

        # Coordinate clues
        if any(clue in stripped for clue in ["- 1", "--", "1 to", "1-", "0 to", "0-"]):
            if any(term in stripped.lower() for term in ["row", "col", "coord", "x", "y"]):
                coord_clues.append(stripped)

        # Arg clues
        if "argv[" in stripped or "argc" in stripped:
            arg_clues.append(stripped)

    return {
        "file": str(file_path),
        "lines": len(lines),
        "has_main": has_main,
        "has_fork": has_fork,
        "has_sysv_shm": has_sysv_shm,
        "has_posix_shm": has_posix_shm,
        "has_mmap": has_mmap,
        "has_sysv_sem": has_sysv_sem,
        "has_posix_sem": has_posix_sem,
        "needs_pthread": needs_pthread,
        "needs_rt": needs_rt,
        "prompts": prompts[:5],
        "coord_clues": coord_clues[:5],
        "arg_clues": arg_clues[:5],
    }


def inspect_directory(dir_path: Path) -> dict[str, Any]:
    resolved = dir_path.resolve()
    if not resolved.is_dir():
        raise FileNotFoundError(f"Directory not found: {resolved}")

    c_files = list(resolved.rglob("*.c"))
    cpp_files = list(resolved.rglob("*.cpp")) + list(resolved.rglob("*.cc"))
    all_sources = c_files + cpp_files
    headers = list(resolved.rglob("*.h")) + list(resolved.rglob("*.hpp"))
    makefiles = list(resolved.rglob("Makefile")) + list(resolved.rglob("makefile"))

    all_code_files = all_sources + headers
    analyses = [analyze_file(f) for f in all_code_files]
    main_files = [a for a in analyses if a["has_main"]]

    uses_fork = any(a["has_fork"] for a in analyses)
    uses_sysv_shm = any(a["has_sysv_shm"] for a in analyses)
    uses_posix_shm = any(a["has_posix_shm"] for a in analyses)
    uses_mmap = any(a["has_mmap"] for a in analyses)
    needs_pthread = any(a["needs_pthread"] for a in analyses)
    needs_rt = any(a["needs_rt"] for a in analyses)

    # Topology heuristic
    if uses_fork and len(main_files) == 1:
        suggested_topology = "single"
    elif len(main_files) >= 2 or uses_sysv_shm or uses_posix_shm:
        suggested_topology = "dual"
    else:
        suggested_topology = "single" if uses_fork else "dual"

    # Suggested compiler command
    libs = []
    if needs_pthread:
        libs.append("-pthread")
    if needs_rt:
        libs.append("-lrt")
    lib_str = f" {' '.join(libs)}" if libs else ""

    return {
        "directory": str(resolved),
        "source_count": len(all_sources),
        "sources": [str(f.relative_to(resolved)) for f in all_sources],
        "headers": [str(f.relative_to(resolved)) for f in headers],
        "makefiles": [str(f.relative_to(resolved)) for f in makefiles],
        "main_files": [f["file"] for f in main_files],
        "suggested_topology": suggested_topology,
        "ipc_summary": {
            "sysv_shm": uses_sysv_shm,
            "posix_shm": uses_posix_shm,
            "mmap": uses_mmap,
            "fork": uses_fork,
            "pthread_or_posix_sem": needs_pthread,
            "librt": needs_rt,
        },
        "file_details": analyses,
        "suggested_compile_flags": f"-Wall -Wextra -O2{lib_str}",
    }


def main() -> int:
    parser = argparse.ArgumentParser(description="Inspect MemTacToe submission directory.")
    parser.add_argument("dir", help="Path to submission source directory")
    args = parser.parse_args()

    try:
        report = inspect_directory(Path(args.dir))
    except Exception as e:
        sys.stderr.write(f"Error inspecting directory: {e}\n")
        return 1

    print(f"=== Submission Inspection: {report['directory']} ===")
    print(f"Source files ({report['source_count']}): {', '.join(report['sources']) or 'None'}")
    print(f"Headers: {', '.join(report['headers']) or 'None'}")
    print(f"Makefiles: {', '.join(report['makefiles']) or 'None'}")
    print(f"Main entry points: {len(report['main_files'])}")
    for mf in report['main_files']:
        print(f"  - {mf}")
    print(f"Suggested topology: {report['suggested_topology']}")
    print("Detected IPC primitives:")
    for k, v in report['ipc_summary'].items():
        if v:
            print(f"  * {k}: Yes")
    print(f"Suggested compile flags: {report['suggested_compile_flags']}")

    print("\n--- Interaction Clues ---")
    for f in report['file_details']:
        if f['prompts'] or f['coord_clues'] or f['arg_clues']:
            print(f"\nFile: {f['file']}")
            if f['arg_clues']:
                print("  Argument handling clues:")
                for a in f['arg_clues']:
                    print(f"    {a}")
            if f['prompts']:
                print("  Prompt clues:")
                for p in f['prompts']:
                    print(f"    {p}")
            if f['coord_clues']:
                print("  Coordinate / Indexing clues:")
                for c in f['coord_clues']:
                    print(f"    {c}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
