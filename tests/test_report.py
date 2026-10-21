"""Unit tests for HTML and JSON report generator."""

from pathlib import Path
import pytest

from mem_tac_review.models import (
    BatchResult,
    BuildResult,
    Checkpoint,
    ProcessInfo,
    SessionResult,
    SessionStatus,
    SubmissionResult,
)
from mem_tac_review.report import ReportGenerator


def test_submission_report_generation(tmp_path: Path) -> None:
    checkpoint = Checkpoint(
        action_index=1,
        target_player="X",
        submitted_text="0 0\n",
        action_summary="Send X move (0, 0)",
        expected_board_str="X . .\n. . .\n. . .",
        expected_next_player="O",
        expected_outcome="IN_PROGRESS",
        observed_output={"p1": "Enter move: 0 0\nUpdated Board:\nX . .\n. . .\n. . ."},
        extracted_board_str="X . .\n. . .\n. . .",
        boundary_reason="quiet window settled",
        timestamp_iso="2026-10-21T12:00:00Z",
    )

    session_res = SessionResult(
        scenario_id="G01",
        scenario_name="Top row win",
        status=SessionStatus.COMPLETED,
        status_reason="All actions executed",
        duration_sec=1.25,
        checkpoints=[checkpoint],
        transcripts={"p1": "Full terminal transcript here"},
        processes=[ProcessInfo(role="p1", pid=1234, exit_code=0)],
    )

    sub_res = SubmissionResult(
        submission_name="student_test",
        profile_name="student_test_profile",
        profile_data={"topology": "dual", "creator": "p1"},
        timestamp_iso="2026-10-21T12:00:00Z",
        platform_info="Linux aarch64",
        build_result=BuildResult("make", True, 0, "Build succeeded", 0.5),
        scenario_results=[session_res],
    )

    out_file = ReportGenerator.save_submission_report(sub_res, tmp_path)
    assert out_file.is_file()
    assert (tmp_path / "report.json").is_file()

    html_content = out_file.read_text(encoding="utf-8")
    assert "student_test" in html_content
    assert "Top row win" in html_content
    assert "X . ." in html_content
    assert "Expected Reference Board &amp; State" in html_content or "Expected Reference Board" in html_content
    assert "Notice for Evaluators" in html_content
    # Check that there are NO pass/fail claims or grades
    assert "Grade:" not in html_content
    assert "Score:" not in html_content


def test_batch_report_generation(tmp_path: Path) -> None:
    sub1 = SubmissionResult(
        submission_name="sub1",
        profile_name="prof1",
        profile_data={},
        timestamp_iso="2026-10-21T12:00:00Z",
        platform_info="Linux",
        build_result=None,
        scenario_results=[],
    )
    sub2 = SubmissionResult(
        submission_name="sub2",
        profile_name="prof2",
        profile_data={},
        timestamp_iso="2026-10-21T12:00:00Z",
        platform_info="Linux",
        build_result=None,
        scenario_results=[],
    )
    batch = BatchResult(
        timestamp_iso="2026-10-21T12:00:00Z",
        suite_name="lab-2",
        submissions=[sub1, sub2],
    )

    index_html = ReportGenerator.save_batch_report(batch, tmp_path)
    assert index_html.is_file()
    assert (tmp_path / "batch_report.json").is_file()
    assert (tmp_path / "submissions" / "sub1" / "report.html").is_file()
    assert (tmp_path / "submissions" / "sub2" / "report.html").is_file()

    html_content = index_html.read_text(encoding="utf-8")
    assert "sub1" in html_content
    assert "sub2" in html_content
    assert "submissions/sub1/report.html" in html_content
