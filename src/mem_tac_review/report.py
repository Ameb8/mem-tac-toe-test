"""Self-contained HTML and JSON report generator."""

from __future__ import annotations

from dataclasses import asdict
import html
import json
from pathlib import Path
from typing import Any, Optional

from mem_tac_review.models import (
    BatchResult,
    Checkpoint,
    SessionResult,
    SessionStatus,
    SubmissionResult,
)

EMBEDDED_CSS = """
:root {
  --bg-color: #f8fafc;
  --surface-color: #ffffff;
  --border-color: #e2e8f0;
  --text-main: #0f172a;
  --text-muted: #64748b;
  --accent-color: #2563eb;
  --notice-bg: #f1f5f9;
  --notice-border: #cbd5e1;
  --code-bg: #0f172a;
  --code-text: #f8fafc;
  --chip-completed: #0284c7;
  --chip-error: #dc2626;
  --chip-timeout: #d97706;
}

* { box-sizing: border-box; margin: 0; padding: 0; }
body {
  font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
  background-color: var(--bg-color);
  color: var(--text-main);
  line-height: 1.5;
  padding: 2rem;
}
.container { max-width: 1200px; margin: 0 auto; }
header { margin-bottom: 2rem; border-bottom: 2px solid var(--border-color); padding-bottom: 1.5rem; }
h1 { font-size: 1.875rem; font-weight: 700; margin-bottom: 0.5rem; }
h2 { font-size: 1.5rem; font-weight: 600; margin: 1.5rem 0 1rem; }
h3 { font-size: 1.25rem; font-weight: 600; margin: 1rem 0 0.5rem; }
p { margin-bottom: 0.75rem; }
.meta-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(280px, 1fr)); gap: 1rem; margin: 1.5rem 0; }
.card { background: var(--surface-color); border: 1px solid var(--border-color); border-radius: 6px; padding: 1.25rem; margin-bottom: 1.5rem; box-shadow: 0 1px 3px rgba(0,0,0,0.05); }

.notice-banner {
  background: var(--notice-bg);
  border-left: 4px solid var(--notice-border);
  padding: 1rem 1.25rem;
  border-radius: 4px;
  font-size: 0.95rem;
  margin-bottom: 2rem;
}

.status-badge {
  display: inline-block;
  padding: 0.25rem 0.6rem;
  border-radius: 9999px;
  font-size: 0.8rem;
  font-weight: 600;
  text-transform: uppercase;
  background: var(--notice-bg);
  color: var(--text-main);
  border: 1px solid var(--border-color);
}
.status-completed { background: #e0f2fe; color: #0369a1; border-color: #bae6fd; }
.status-error { background: #fee2e2; color: #b91c1c; border-color: #fecaca; }
.status-timeout { background: #fef3c7; color: #b45309; border-color: #fde68a; }

table { width: 100%; border-collapse: collapse; margin: 1rem 0; font-size: 0.95rem; }
th, td { text-align: left; padding: 0.75rem; border: 1px solid var(--border-color); }
th { background: #f1f5f9; font-weight: 600; }
tr:nth-child(even) { background: #f8fafc; }

pre, code {
  font-family: ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, "Liberation Mono", "Courier New", monospace;
  font-size: 0.9rem;
}
pre {
  background: #f1f5f9;
  border: 1px solid var(--border-color);
  border-radius: 4px;
  padding: 0.75rem;
  overflow-x: auto;
  white-space: pre-wrap;
  word-break: break-all;
}
.terminal-pre {
  background: #1e293b;
  color: #f8fafc;
  border-color: #334155;
}

.checkpoint-card {
  border: 1px solid var(--border-color);
  border-radius: 6px;
  background: var(--surface-color);
  margin-bottom: 1.5rem;
  padding: 1rem;
}
.checkpoint-header {
  display: flex;
  justify-content: space-between;
  align-items: baseline;
  border-bottom: 1px solid var(--border-color);
  padding-bottom: 0.5rem;
  margin-bottom: 0.75rem;
}
.side-by-side {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 1.25rem;
  margin: 1rem 0;
}
@media (max-width: 768px) {
  .side-by-side { grid-template-columns: 1fr; }
}
.col-header { font-weight: 600; font-size: 0.9rem; color: var(--text-muted); margin-bottom: 0.35rem; }
.board-box {
  background: #f8fafc;
  border: 1px solid var(--border-color);
  border-radius: 4px;
  padding: 0.75rem;
  min-height: 100px;
}
details { margin-top: 1rem; }
summary { cursor: pointer; font-weight: 600; color: var(--accent-color); padding: 0.35rem 0; }
"""


class ReportGenerator:
    """Generates self-contained HTML and structured JSON review reports."""

    @classmethod
    def generate_submission_html(cls, sub: SubmissionResult) -> str:
        """Render self-contained HTML report for a single submission."""
        title = f"MemTacToe Review Report: {html.escape(sub.submission_name)}"

        # Scenario Table Rows
        scen_rows: list[str] = []
        for idx, s_res in enumerate(sub.scenario_results, 1):
            badge_cls = "status-completed" if s_res.status == SessionStatus.COMPLETED else (
                "status-timeout" if s_res.status == SessionStatus.TIMED_OUT else "status-error"
            )
            scen_rows.append(
                f"<tr>"
                f"<td>{idx}</td>"
                f"<td><a href='#scenario-{html.escape(s_res.scenario_id)}'><strong>{html.escape(s_res.scenario_id)}</strong></a>: {html.escape(s_res.scenario_name)}</td>"
                f"<td><span class='status-badge {badge_cls}'>{html.escape(s_res.status.value)}</span></td>"
                f"<td>{s_res.duration_sec:.2f}s</td>"
                f"<td>{len(s_res.checkpoints)} checkpoints</td>"
                f"</tr>"
            )

        # Scenarios Detailed Sections
        scen_sections: list[str] = []
        for s_res in sub.scenario_results:
            scen_sections.append(cls._render_scenario_section(s_res, sub.profile_data.get("creator", "p1")))

        # Build Section
        build_html = ""
        if sub.build_result:
            b_res = sub.build_result
            build_status_badge = (
                "<span class='status-badge status-completed'>Success</span>"
                if b_res.success
                else "<span class='status-badge status-error'>Build Failed</span>"
            )
            build_html = f"""
            <div class="card">
              <h2>Build Evidence</h2>
              <p><strong>Command:</strong> <code>{html.escape(b_res.command or "N/A")}</code> | <strong>Status:</strong> {build_status_badge} (exit code {b_res.exit_code}) | <strong>Duration:</strong> {b_res.duration_sec:.2f}s</p>
              <pre>{html.escape(b_res.output or "No build output.")}</pre>
            </div>
            """

        profile_details = "<br>".join(
            f"<strong>{html.escape(k)}:</strong> <code>{html.escape(str(v))}</code>"
            for k, v in sub.profile_data.items()
        )

        return f"""<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>{title}</title>
  <style>{EMBEDDED_CSS}</style>
</head>
<body>
  <div class="container">
    <header>
      <h1>MemTacToe Review Report</h1>
      <p style="font-size: 1.15rem; color: var(--text-muted);">Submission: <strong>{html.escape(sub.submission_name)}</strong></p>
    </header>

    <div class="notice-banner">
      <strong>Notice for Evaluators:</strong> This report provides structured execution evidence and side-by-side reference boards for human review. In strict accordance with the project specification, it does not assign grades, declare behavioral tests passed or failed, or calculate board differences. A status of "completed" denotes that the scripted session executed through its planned interactions; it is not a declaration of game correctness.
    </div>

    <div class="card">
      <h2>Submission Metadata</h2>
      <div class="meta-grid">
        <div>
          <p><strong>Submission Name:</strong> {html.escape(sub.submission_name)}</p>
          <p><strong>Profile Name:</strong> {html.escape(sub.profile_name)}</p>
          <p><strong>Platform:</strong> {html.escape(sub.platform_info)}</p>
          <p><strong>Run Timestamp:</strong> {html.escape(sub.timestamp_iso)}</p>
        </div>
        <div>
          <p><strong>Profile Configuration:</strong></p>
          <p>{profile_details}</p>
        </div>
      </div>
    </div>

    {build_html}

    <div class="card">
      <h2>Scenario Execution Index</h2>
      <table>
        <thead>
          <tr>
            <th>#</th>
            <th>Scenario</th>
            <th>Execution Status</th>
            <th>Duration</th>
            <th>Evidence</th>
          </tr>
        </thead>
        <tbody>
          {"".join(scen_rows)}
        </tbody>
      </table>
    </div>

    {"".join(scen_sections)}

  </div>
</body>
</html>
"""

    @classmethod
    def _render_scenario_section(cls, s_res: SessionResult, creator_role: str) -> str:
        badge_cls = "status-completed" if s_res.status == SessionStatus.COMPLETED else (
            "status-timeout" if s_res.status == SessionStatus.TIMED_OUT else "status-error"
        )

        # Checkpoints HTML
        cp_html_list: list[str] = []
        for cp in s_res.checkpoints:
            cp_html_list.append(cls._render_checkpoint(cp, creator_role))

        # Unexecuted actions notice
        unexecuted_html = ""
        if s_res.unexecuted_actions:
            items = "".join(f"<li>{html.escape(a.description or str(a.type))}</li>" for a in s_res.unexecuted_actions)
            unexecuted_html = f"""
            <div style="background: #fffbeb; border: 1px solid #fef3c7; padding: 1rem; border-radius: 4px; margin-bottom: 1rem;">
              <strong>Actions not executed due to early exit / timeout:</strong>
              <ul style="margin-left: 1.5rem; margin-top: 0.5rem;">{items}</ul>
            </div>
            """

        # IPC summary
        ipc_html = ""
        if s_res.ipc_leaks_detected:
            leaks = "".join(f"<li><code>{html.escape(leak)}</code></li>" for leak in s_res.ipc_leaks_detected)
            ipc_html = f"""
            <div style="background: #fef2f2; border: 1px solid #fee2e2; padding: 1rem; border-radius: 4px; margin: 1rem 0;">
              <strong>Lingering shared-memory segment(s) detected after session:</strong>
              <ul style="margin-left: 1.5rem; margin-top: 0.5rem;">{leaks}</ul>
            </div>
            """

        # Transcripts HTML
        transcripts_html_list: list[str] = []
        for role, text in s_res.transcripts.items():
            transcripts_html_list.append(
                f"<div><strong>Process / Terminal: <code>{html.escape(role)}</code></strong><pre class='terminal-pre'>{html.escape(text or '[No output captured]')}</pre></div>"
            )

        # Process exit details
        proc_exit_list: list[str] = []
        for proc in s_res.processes:
            proc_exit_list.append(
                f"<li>Process <code>{html.escape(proc.role)}</code> (PID {proc.pid or 'N/A'}): Exit Code {proc.exit_code}</li>"
            )

        return f"""
        <div class="card" id="scenario-{html.escape(s_res.scenario_id)}">
          <div style="display: flex; justify-content: space-between; align-items: center; border-bottom: 1px solid var(--border-color); padding-bottom: 0.75rem; margin-bottom: 1rem;">
            <div>
              <h3>Scenario {html.escape(s_res.scenario_id)}: {html.escape(s_res.scenario_name)}</h3>
              <p style="color: var(--text-muted); font-size: 0.9rem; margin: 0;">Duration: {s_res.duration_sec:.2f}s | {s_res.status_reason}</p>
            </div>
            <span class="status-badge {badge_cls}">{html.escape(s_res.status.value)}</span>
          </div>

          {unexecuted_html}
          {ipc_html}

          <h4>Checkpoints & Evidence</h4>
          {"".join(cp_html_list) if cp_html_list else "<p style='color: var(--text-muted);'>No checkpoints captured.</p>"}

          <details>
            <summary>View Process Exits & Full Transcripts</summary>
            <div style="margin-top: 0.75rem;">
              <ul style="margin-left: 1.5rem; margin-bottom: 1rem;">{"".join(proc_exit_list)}</ul>
              {"".join(transcripts_html_list)}
            </div>
          </details>
        </div>
        """

    @classmethod
    def _render_checkpoint(cls, cp: Checkpoint, creator_role: str) -> str:
        # Checkpoint side-by-side view
        observed_blocks: list[str] = []
        for role, txt in cp.observed_output.items():
            label = f"Observed {role}-terminal output"
            if role == creator_role:
                label += " (Creator)"
            observed_blocks.append(
                f"<div style='margin-bottom: 0.5rem;'><strong>{html.escape(label)}:</strong><pre>{html.escape(txt or '[Silence / No new output]')}</pre></div>"
            )

        extracted_html = ""
        if cp.extracted_board_str:
            extracted_html = f"""
            <div style="margin-top: 0.5rem;">
              <strong>Extracted Normalized Board (formatting only):</strong>
              <pre style="background: #eff6ff; border-color: #bfdbfe;">{html.escape(cp.extracted_board_str)}</pre>
            </div>
            """
        else:
            extracted_html = "<p style='color: var(--text-muted); font-size: 0.85rem; margin-top: 0.5rem;'><em>No normalized board extracted (raw excerpt preserved above).</em></p>"

        return f"""
        <div class="checkpoint-card">
          <div class="checkpoint-header">
            <div>
              <strong>Action {cp.action_index}:</strong> {html.escape(cp.action_summary)}
              <div style="font-size: 0.85rem; color: var(--text-muted);">
                Submitted input: <code>{html.escape(repr(cp.submitted_text))}</code> to player <code>{html.escape(cp.target_player)}</code>
              </div>
            </div>
            <div style="font-size: 0.85rem; color: var(--text-muted);">
              Boundary: <em>{html.escape(cp.boundary_reason)}</em>
            </div>
          </div>

          <div class="side-by-side">
            <div>
              <div class="col-header">Expected Reference Board & State</div>
              <div class="board-box">
                <pre>{html.escape(cp.expected_board_str)}</pre>
                <div style="font-size: 0.85rem; color: var(--text-muted); margin-top: 0.5rem;">
                  Expected Next Player: <strong>{html.escape(cp.expected_next_player or 'None (Game Terminal)')}</strong><br>
                  Expected Outcome: <strong>{html.escape(cp.expected_outcome)}</strong>
                </div>
              </div>
            </div>
            <div>
              <div class="col-header">Observed Terminal Output</div>
              <div class="board-box">
                {"".join(observed_blocks)}
                {extracted_html}
              </div>
            </div>
          </div>
        </div>
        """

    @classmethod
    def generate_batch_html(cls, batch: BatchResult, submission_rel_paths: dict[str, str]) -> str:
        """Render self-contained HTML batch overview report."""
        sub_rows: list[str] = []
        for sub in batch.submissions:
            rel_link = submission_rel_paths.get(sub.submission_name, "#")
            completed_count = sum(1 for s in sub.scenario_results if s.status == SessionStatus.COMPLETED)
            total_count = len(sub.scenario_results)
            b_status = "N/A"
            if sub.build_result:
                b_status = "Success" if sub.build_result.success else f"Failed ({sub.build_result.exit_code})"

            sub_rows.append(
                f"<tr>"
                f"<td><a href='{html.escape(rel_link)}'><strong>{html.escape(sub.submission_name)}</strong></a></td>"
                f"<td>{html.escape(sub.profile_name)}</td>"
                f"<td>{html.escape(b_status)}</td>"
                f"<td>{completed_count} / {total_count} scenarios completed</td>"
                f"<td><a href='{html.escape(rel_link)}'>Open Submission Report</a></td>"
                f"</tr>"
            )

        return f"""<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>MemTacToe Batch Review Report</title>
  <style>{EMBEDDED_CSS}</style>
</head>
<body>
  <div class="container">
    <header>
      <h1>MemTacToe Batch Review Report</h1>
      <p style="font-size: 1.15rem; color: var(--text-muted);">Suite: <strong>{html.escape(batch.suite_name)}</strong> | Timestamp: {html.escape(batch.timestamp_iso)}</p>
    </header>

    <div class="notice-banner">
      <strong>Notice for Evaluators:</strong> This batch report aggregates review evidence across submissions. The harness does not assign scores, declare pass/fail, or infer compliance. Open individual submission reports to compare expected game states with observed terminal output.
    </div>

    <div class="card">
      <h2>Batch Submissions Summary</h2>
      <table>
        <thead>
          <tr>
            <th>Submission</th>
            <th>Profile</th>
            <th>Build</th>
            <th>Execution Progress</th>
            <th>Report Link</th>
          </tr>
        </thead>
        <tbody>
          {"".join(sub_rows)}
        </tbody>
      </table>
    </div>
  </div>
</body>
</html>
"""

    @classmethod
    def save_submission_report(cls, sub: SubmissionResult, output_dir: Path | str) -> Path:
        """Write report.html and report.json into output_dir."""
        out = Path(output_dir)
        out.mkdir(parents=True, exist_ok=True)

        html_content = cls.generate_submission_html(sub)
        (out / "report.html").write_text(html_content, encoding="utf-8")

        # Save JSON
        json_content = json.dumps(asdict(sub), indent=2)
        (out / "report.json").write_text(json_content, encoding="utf-8")

        return out / "report.html"

    @classmethod
    def save_batch_report(cls, batch: BatchResult, output_dir: Path | str) -> Path:
        """Save batch report and all individual submission reports."""
        out = Path(output_dir)
        out.mkdir(parents=True, exist_ok=True)

        submission_rel_paths: dict[str, str] = {}
        for sub in batch.submissions:
            sub_dir = out / "submissions" / sub.submission_name
            cls.save_submission_report(sub, sub_dir)
            submission_rel_paths[sub.submission_name] = f"submissions/{sub.submission_name}/report.html"

        batch_html = cls.generate_batch_html(batch, submission_rel_paths)
        (out / "index.html").write_text(batch_html, encoding="utf-8")

        batch_json = json.dumps(asdict(batch), indent=2)
        (out / "batch_report.json").write_text(batch_json, encoding="utf-8")

        return out / "index.html"
