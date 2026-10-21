"""End-to-end integration tests verifying real C and custom submissions."""

from pathlib import Path
import pytest

from mem_tac_review.cli import main


def test_e2e_dual_shm_submission(tmp_path: Path) -> None:
    profile_path = Path("profiles/dual_shm.yaml")
    out_dir = tmp_path / "dual_shm_report"

    code = main([
        "run",
        "--profile", str(profile_path),
        "--scenario", "G01",
        "--output", str(out_dir),
    ])
    assert code == 0
    assert (out_dir / "report.html").is_file()
    assert (out_dir / "report.json").is_file()


def test_e2e_single_fork_submission(tmp_path: Path) -> None:
    profile_path = Path("profiles/single_fork.yaml")
    out_dir = tmp_path / "single_fork_report"

    code = main([
        "run",
        "--profile", str(profile_path),
        "--scenario", "G07",
        "--output", str(out_dir),
    ])
    assert code == 0
    assert (out_dir / "report.html").is_file()
    assert (out_dir / "report.json").is_file()


def test_e2e_custom_coords_submission(tmp_path: Path) -> None:
    profile_path = Path("profiles/custom_coords.yaml")
    out_dir = tmp_path / "custom_coords_report"

    code = main([
        "run",
        "--profile", str(profile_path),
        "--scenario", "G01",
        "--output", str(out_dir),
    ])
    assert code == 0
    assert (out_dir / "report.html").is_file()
    assert (out_dir / "report.json").is_file()


def test_e2e_build_fail_submission(tmp_path: Path) -> None:
    profile_path = Path("profiles/build_fail.yaml")
    out_dir = tmp_path / "build_fail_report"

    code = main([
        "run",
        "--profile", str(profile_path),
        "--scenario", "G01",
        "--output", str(out_dir),
    ])
    assert code == 0
    assert (out_dir / "report.html").is_file()
    assert (out_dir / "report.json").is_file()


def test_e2e_batch_all_profiles(tmp_path: Path) -> None:
    out_dir = tmp_path / "batch_report"

    code = main([
        "batch",
        "--profiles", "profiles/",
        "--scenario", "E01",
        "--output", str(out_dir),
    ])
    assert code == 0
    assert (out_dir / "index.html").is_file()
    assert (out_dir / "batch_report.json").is_file()
