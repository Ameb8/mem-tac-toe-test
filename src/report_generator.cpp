#include "mem_tac_review/report_generator.hpp"

#include <sstream>
#include <fstream>
#include <regex>
#include <iomanip>

namespace mem_tac_review {

std::string ReportGenerator::html_escape(const std::string& data) {
    std::string buffer;
    buffer.reserve(data.size() * 11 / 10);
    for (char c : data) {
        switch (c) {
            case '&':  buffer.append("&amp;");       break;
            case '\"': buffer.append("&quot;");      break;
            case '\'': buffer.append("&#39;");       break;
            case '<':  buffer.append("&lt;");        break;
            case '>':  buffer.append("&gt;");        break;
            default:   buffer.push_back(c);          break;
        }
    }
    return buffer;
}

std::string ReportGenerator::strip_ansi(const std::string& str) {
    static const std::regex ansi_regex(R"(\x1B\[[0-9;]*[a-zA-Z]|\x1B\([a-zA-Z])");
    return std::regex_replace(str, ansi_regex, "");
}

static std::string status_badge_class(ExecutionStatus s) {
    switch (s) {
        case ExecutionStatus::COMPLETED: return "badge-completed";
        case ExecutionStatus::BUILD_ERROR: return "badge-build-error";
        case ExecutionStatus::LAUNCH_ERROR: return "badge-launch-error";
        case ExecutionStatus::TIMED_OUT: return "badge-timeout";
        case ExecutionStatus::INTERRUPTED: return "badge-interrupted";
        case ExecutionStatus::NOT_RUN: return "badge-not-run";
    }
    return "badge-not-run";
}

static const char* REPORT_CSS = R"(
:root {
  --bg-primary: #0f172a;
  --bg-secondary: #1e293b;
  --bg-tertiary: #334155;
  --text-primary: #f8fafc;
  --text-secondary: #cbd5e1;
  --text-muted: #94a3b8;
  --border-color: #334155;
  --terminal-bg: #030712;
  --accent-blue: #38bdf8;
}

* { box-sizing: border-box; margin: 0; padding: 0; }
body {
  font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, "Helvetica Neue", Arial, sans-serif;
  background-color: var(--bg-primary);
  color: var(--text-primary);
  line-height: 1.5;
  padding: 2rem;
}

.container { max-width: 1400px; margin: 0 auto; }
header {
  border-bottom: 1px solid var(--border-color);
  padding-bottom: 1.5rem;
  margin-bottom: 2rem;
}
h1 { font-size: 2rem; font-weight: 700; margin-bottom: 0.5rem; }
h2 { font-size: 1.4rem; font-weight: 600; margin: 1.5rem 0 1rem; color: var(--accent-blue); }
h3 { font-size: 1.1rem; font-weight: 600; margin-bottom: 0.5rem; }

.meta-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(280px, 1fr));
  gap: 1rem;
  margin-top: 1rem;
}
.meta-card {
  background: var(--bg-secondary);
  border: 1px solid var(--border-color);
  border-radius: 8px;
  padding: 1rem;
}
.meta-label { font-size: 0.85rem; color: var(--text-muted); text-transform: uppercase; letter-spacing: 0.05em; }
.meta-val { font-size: 1rem; font-weight: 500; margin-top: 0.25rem; font-family: monospace; word-break: break-all; }

.badge {
  display: inline-block;
  padding: 0.25rem 0.6rem;
  border-radius: 9999px;
  font-size: 0.8rem;
  font-weight: 600;
  text-transform: uppercase;
  letter-spacing: 0.05em;
}
.badge-completed { background: #0369a1; color: #e0f2fe; }
.badge-build-error { background: #b91c1c; color: #fee2e2; }
.badge-launch-error { background: #7e22ce; color: #f3e8ff; }
.badge-timeout { background: #b45309; color: #fef3c7; }
.badge-interrupted { background: #c2410c; color: #ffedd5; }
.badge-not-run { background: #475569; color: #f1f5f9; }

.nav-index {
  background: var(--bg-secondary);
  border: 1px solid var(--border-color);
  border-radius: 8px;
  padding: 1rem;
  margin-bottom: 2rem;
}
.nav-list { display: flex; flex-wrap: wrap; gap: 0.5rem; list-style: none; margin-top: 0.5rem; }
.nav-item a {
  display: inline-flex;
  align-items: center;
  gap: 0.5rem;
  background: var(--bg-tertiary);
  color: var(--text-primary);
  text-decoration: none;
  padding: 0.4rem 0.8rem;
  border-radius: 6px;
  font-size: 0.9rem;
  transition: background 0.15s;
}
.nav-item a:hover { background: #475569; }

.scenario-card {
  background: var(--bg-secondary);
  border: 1px solid var(--border-color);
  border-radius: 8px;
  padding: 1.5rem;
  margin-bottom: 2rem;
}
.scenario-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  flex-wrap: wrap;
  gap: 0.5rem;
  border-bottom: 1px solid var(--border-color);
  padding-bottom: 0.75rem;
  margin-bottom: 1rem;
}

.checkpoint-card {
  background: #111827;
  border: 1px solid var(--border-color);
  border-radius: 6px;
  padding: 1rem;
  margin-bottom: 1.25rem;
}
.cp-header {
  display: flex;
  justify-content: space-between;
  flex-wrap: wrap;
  gap: 0.5rem;
  margin-bottom: 0.75rem;
  font-size: 0.95rem;
}
.cp-submitted {
  font-family: monospace;
  background: var(--bg-secondary);
  padding: 0.2rem 0.5rem;
  border-radius: 4px;
}

.side-by-side {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 1rem;
}
@media (max-width: 900px) {
  .side-by-side { grid-template-columns: 1fr; }
}

.pane {
  background: var(--terminal-bg);
  border: 1px solid var(--border-color);
  border-radius: 6px;
  padding: 0.75rem;
  overflow-x: auto;
}
.pane-title {
  font-size: 0.8rem;
  text-transform: uppercase;
  color: var(--text-muted);
  letter-spacing: 0.05em;
  margin-bottom: 0.5rem;
  border-bottom: 1px dashed var(--border-color);
  padding-bottom: 0.25rem;
}
pre {
  font-family: "SFMono-Regular", Consolas, "Liberation Mono", Menlo, Courier, monospace;
  font-size: 0.9rem;
  line-height: 1.4;
  white-space: pre-wrap;
  word-break: break-all;
}

.cp-footer {
  margin-top: 0.5rem;
  font-size: 0.85rem;
  color: var(--text-muted);
  display: flex;
  justify-content: space-between;
  flex-wrap: wrap;
}

details {
  background: var(--terminal-bg);
  border: 1px solid var(--border-color);
  border-radius: 6px;
  padding: 0.5rem 1rem;
  margin-top: 1rem;
}
summary {
  cursor: pointer;
  font-weight: 600;
  color: var(--accent-blue);
  padding: 0.25rem 0;
}
details[open] summary { margin-bottom: 0.5rem; }

table {
  width: 100%;
  border-collapse: collapse;
  margin: 1rem 0;
}
th, td {
  padding: 0.75rem 1rem;
  text-align: left;
  border-bottom: 1px solid var(--border-color);
}
th { background: var(--bg-tertiary); font-weight: 600; }
tr:hover { background: rgba(255, 255, 255, 0.02); }
)";

std::string ReportGenerator::generate_submission_html(const SubmissionResult& sub, const Profile& profile) {
    std::ostringstream html;

    html << "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n";
    html << "<meta charset=\"UTF-8\">\n";
    html << "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
    html << "<title>Review Evidence: " << html_escape(sub.submission_name) << "</title>\n";
    html << "<style>\n" << REPORT_CSS << "\n</style>\n";
    html << "</head>\n<body>\n";
    html << "<div class=\"container\">\n";

    // Header
    html << "<header>\n";
    html << "<h1>MemTacToe Review Evidence Report</h1>\n";
    html << "<div style=\"color: var(--text-muted); font-size: 0.95rem;\">"
         << "This report presents reference board states alongside observed terminal evidence for human grading. "
         << "It does not assign scores or pass/fail grades."
         << "</div>\n";

    html << "<div class=\"meta-grid\">\n";
    html << "  <div class=\"meta-card\"><div class=\"meta-label\">Submission</div><div class=\"meta-val\">"
         << html_escape(sub.submission_name) << "</div></div>\n";
    html << "  <div class=\"meta-card\"><div class=\"meta-label\">Platform</div><div class=\"meta-val\">"
         << html_escape(sub.platform_info) << "</div></div>\n";
    html << "  <div class=\"meta-card\"><div class=\"meta-label\">Generated At</div><div class=\"meta-val\">"
         << html_escape(sub.run_timestamp) << "</div></div>\n";
    html << "  <div class=\"meta-card\"><div class=\"meta-label\">Total Duration</div><div class=\"meta-val\">"
         << sub.total_duration_ms << " ms</div></div>\n";
    html << "</div>\n";
    html << "</header>\n";

    // Profile & Build info
    html << "<h2>Submission Configuration & Build</h2>\n";
    html << "<div class=\"scenario-card\">\n";
    html << "<div class=\"meta-grid\">\n";
    html << "  <div class=\"meta-card\"><div class=\"meta-label\">Topology</div><div class=\"meta-val\">"
         << topology_to_string(profile.topology()) << "</div></div>\n";
    html << "  <div class=\"meta-card\"><div class=\"meta-label\">Creator Terminal</div><div class=\"meta-val\">"
         << creator_role_to_string(profile.creator()) << "</div></div>\n";
    html << "  <div class=\"meta-card\"><div class=\"meta-label\">Coord Base / Order</div><div class=\"meta-val\">"
         << profile.coord_base() << "-based (" << coord_order_to_string(profile.coord_order()) << ")</div></div>\n";
    html << "  <div class=\"meta-card\"><div class=\"meta-label\">Pacing / Settling</div><div class=\"meta-val\">"
         << profile.settling_ms() << " ms (timeout: " << profile.timeout_sec() << "s)</div></div>\n";
    html << "</div>\n";

    html << "<div style=\"margin-top: 1rem;\">\n";
    html << "<h3>Build Status: <span class=\"badge "
         << (sub.build_result.success ? "badge-completed" : "badge-build-error") << "\">"
         << (sub.build_result.success ? "Build Succeeded" : "Build Failed") << "</span></h3>\n";
    if (!profile.build_cmd().empty()) {
        html << "<div style=\"font-size: 0.9rem; color: var(--text-muted); margin-bottom: 0.5rem;\">Command: <code>"
             << html_escape(profile.build_cmd()) << "</code></div>\n";
    }
    html << "<details " << (sub.build_result.success ? "" : "open") << ">\n";
    html << "<summary>View Build Output (" << sub.build_result.duration_ms << " ms)</summary>\n";
    html << "<pre>" << html_escape(strip_ansi(sub.build_result.output)) << "</pre>\n";
    html << "</details>\n";
    html << "</div>\n";
    html << "</div>\n";

    // Scenarios Navigation Index
    html << "<h2>Scenarios Overview</h2>\n";
    html << "<div class=\"nav-index\">\n";
    html << "<ul class=\"nav-list\">\n";
    for (const auto& sr : sub.scenario_results) {
        html << "<li class=\"nav-item\"><a href=\"#" << html_escape(sr.scenario_id) << "\">"
             << "<span class=\"badge " << status_badge_class(sr.status) << "\">"
             << execution_status_to_string(sr.status) << "</span> "
             << html_escape(sr.scenario_name)
             << "</a></li>\n";
    }
    html << "</ul>\n";
    html << "</div>\n";

    // Scenario Details
    html << "<h2>Scenario Evidence Details</h2>\n";
    for (const auto& sr : sub.scenario_results) {
        html << "<div class=\"scenario-card\" id=\"" << html_escape(sr.scenario_id) << "\">\n";
        html << "<div class=\"scenario-header\">\n";
        html << "<div><h3>" << html_escape(sr.scenario_name) << " <code>[" << html_escape(sr.scenario_id) << "]</code></h3></div>\n";
        html << "<div><span class=\"badge " << status_badge_class(sr.status) << "\">"
             << execution_status_to_string(sr.status) << "</span> "
             << "<span style=\"color: var(--text-muted); font-size: 0.9rem;\">(" << sr.duration_ms << " ms)</span></div>\n";
        html << "</div>\n";

        if (!sr.status_details.empty()) {
            html << "<div style=\"margin-bottom: 1rem; color: #fbbf24; font-size: 0.9rem;\">"
                 << "<strong>Status Note:</strong> " << html_escape(sr.status_details) << "</div>\n";
        }

        // Checkpoints
        if (sr.checkpoints.empty()) {
            html << "<div style=\"color: var(--text-muted); font-style: italic;\">No checkpoints recorded for this scenario.</div>\n";
        } else {
            for (const auto& cp : sr.checkpoints) {
                html << "<div class=\"checkpoint-card\">\n";
                html << "<div class=\"cp-header\">\n";
                html << "<div><strong>Action " << cp.step_index << ":</strong> "
                     << html_escape(cp.action.description.empty() ? action_type_to_string(cp.action.type) : cp.action.description)
                     << "</div>\n";
                html << "<div>Target: <strong>" << html_escape(cp.target_connection) << "</strong> | Input: <span class=\"cp-submitted\">"
                     << html_escape(cp.submitted_input) << "</span></div>\n";
                html << "</div>\n";

                html << "<div class=\"side-by-side\">\n";

                // Left pane: Expected
                html << "<div class=\"pane\">\n";
                html << "<div class=\"pane-title\">Expected Reference State</div>\n";
                html << "<pre>" << html_escape(cp.expected_board) << "\n\n";
                html << "Next player: " << html_escape(cp.expected_next_player) << "\n";
                html << "Expected outcome: " << html_escape(cp.expected_outcome) << "</pre>\n";
                html << "</div>\n";

                // Right pane: Observed Output
                html << "<div class=\"pane\">\n";
                std::string creator_label = (profile.topology() == Topology::SINGLE)
                                                ? "Observed Shared Terminal Output"
                                                : "Observed Creator Terminal Output (" + creator_role_to_string(profile.creator()) + ")";
                html << "<div class=\"pane-title\">" << creator_label << "</div>\n";
                std::string obs = strip_ansi(cp.creator_output);
                if (obs.empty()) {
                    html << "<pre style=\"color: var(--text-muted);\">[No output observed during interval]</pre>\n";
                } else {
                    html << "<pre>" << html_escape(obs) << "</pre>\n";
                }
                html << "</div>\n";

                html << "</div>\n"; // side-by-side

                // Checkpoint Footer
                html << "<div class=\"cp-footer\">\n";
                html << "<div>Collection boundary: <em>" << html_escape(cp.boundary_method) << "</em></div>\n";
                html << "<div>Captured at +" << cp.timestamp_ms << " ms</div>\n";
                html << "</div>\n";

                html << "</div>\n"; // checkpoint-card
            }
        }

        // Final process outcomes
        html << "<div style=\"margin-top: 1rem; border-top: 1px dashed var(--border-color); padding-top: 0.75rem;\">\n";
        html << "<h4>Process Termination & Final Outcome</h4>\n";
        html << "<div style=\"font-size: 0.9rem; margin-top: 0.5rem;\">\n";
        html << "Expected Final Outcome: <strong>" << html_escape(sr.expected_final_outcome) << "</strong><br>\n";
        if (profile.topology() == Topology::DUAL) {
            html << "Player 1 exit code: <code>" << sr.p1_exit_code << "</code> | ";
            html << "Player 2 exit code: <code>" << sr.p2_exit_code << "</code>\n";
        } else {
            html << "Shared launcher exit code: <code>" << sr.p1_exit_code << "</code>\n";
        }
        html << "</div>\n";

        // Raw Transcript
        html << "<details>\n";
        html << "<summary>View Full Session Transcript</summary>\n";
        html << "<pre>" << html_escape(strip_ansi(sr.full_transcript)) << "</pre>\n";
        html << "</details>\n";

        html << "</div>\n"; // process termination

        html << "</div>\n"; // scenario-card
    }

    html << "</div>\n</body>\n</html>\n";
    return html.str();
}

bool ReportGenerator::write_submission_report(const SubmissionResult& result,
                                             const Profile& profile,
                                             const std::filesystem::path& output_dir,
                                             std::string* error_msg) {
    try {
        std::filesystem::create_directories(output_dir);
        auto file_path = output_dir / "index.html";
        std::ofstream fout(file_path);
        if (!fout.is_open()) {
            if (error_msg) *error_msg = "Could not open file for writing: " + file_path.string();
            return false;
        }
        fout << generate_submission_html(result, profile);
        return true;
    } catch (const std::exception& e) {
        if (error_msg) *error_msg = e.what();
        return false;
    }
}

std::string ReportGenerator::generate_batch_html(const std::vector<SubmissionResult>& batch_results,
                                                const std::string& suite_name,
                                                const std::string& timestamp) {
    std::ostringstream html;
    html << "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n";
    html << "<meta charset=\"UTF-8\">\n";
    html << "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
    html << "<title>MemTacToe Batch Review Cohort Report</title>\n";
    html << "<style>\n" << REPORT_CSS << "\n</style>\n";
    html << "</head>\n<body>\n";
    html << "<div class=\"container\">\n";

    html << "<header>\n";
    html << "<h1>MemTacToe Batch Review Cohort Report</h1>\n";
    html << "<div style=\"color: var(--text-muted); font-size: 0.95rem;\">"
         << "Cohort overview linking to individual submission evidence reports for suite: <code>"
         << html_escape(suite_name) << "</code>. Generated at " << html_escape(timestamp)
         << "</div>\n";
    html << "</header>\n";

    html << "<h2>Submissions in Cohort</h2>\n";
    html << "<div class=\"scenario-card\">\n";
    html << "<table>\n<thead>\n";
    html << "<tr>\n";
    html << "<th>Submission</th>\n";
    html << "<th>Build Status</th>\n";
    html << "<th>Scenarios</th>\n";
    html << "<th>Completed</th>\n";
    html << "<th>Timed Out</th>\n";
    html << "<th>Errors / Interrupted</th>\n";
    html << "<th>Duration</th>\n";
    html << "<th>Report</th>\n";
    html << "</tr>\n</thead>\n<tbody>\n";

    for (const auto& sub : batch_results) {
        int completed = 0;
        int timed_out = 0;
        int errors = 0;

        for (const auto& sr : sub.scenario_results) {
            if (sr.status == ExecutionStatus::COMPLETED) completed++;
            else if (sr.status == ExecutionStatus::TIMED_OUT) timed_out++;
            else errors++;
        }

        html << "<tr>\n";
        html << "<td><strong>" << html_escape(sub.submission_name) << "</strong></td>\n";
        html << "<td><span class=\"badge "
             << (sub.build_result.success ? "badge-completed" : "badge-build-error") << "\">"
             << (sub.build_result.success ? "Succeeded" : "Failed") << "</span></td>\n";
        html << "<td>" << sub.scenario_results.size() << "</td>\n";
        html << "<td><span class=\"badge badge-completed\">" << completed << "</span></td>\n";
        html << "<td>" << (timed_out > 0 ? "<span class=\"badge badge-timeout\">" + std::to_string(timed_out) + "</span>" : "0") << "</td>\n";
        html << "<td>" << (errors > 0 ? "<span class=\"badge badge-build-error\">" + std::to_string(errors) + "</span>" : "0") << "</td>\n";
        html << "<td>" << sub.total_duration_ms << " ms</td>\n";
        html << "<td><a href=\"./" << html_escape(sub.submission_name) << "/index.html\" style=\"color: var(--accent-blue);\">View Evidence &rarr;</a></td>\n";
        html << "</tr>\n";
    }

    html << "</tbody>\n</table>\n";
    html << "</div>\n";

    html << "</div>\n</body>\n</html>\n";
    return html.str();
}

bool ReportGenerator::write_batch_report(const std::vector<SubmissionResult>& batch_results,
                                        const std::vector<Profile>& profiles,
                                        const std::string& suite_name,
                                        const std::filesystem::path& output_dir,
                                        std::string* error_msg) {
    try {
        std::filesystem::create_directories(output_dir);

        // Write batch index.html
        std::string ts = batch_results.empty() ? "" : batch_results[0].run_timestamp;
        auto batch_html = generate_batch_html(batch_results, suite_name, ts);
        std::ofstream fout(output_dir / "index.html");
        if (!fout.is_open()) {
            if (error_msg) *error_msg = "Could not write batch index.html";
            return false;
        }
        fout << batch_html;

        // Write individual submission reports in subdirectories
        for (size_t i = 0; i < batch_results.size() && i < profiles.size(); ++i) {
            auto sub_dir = output_dir / batch_results[i].submission_name;
            write_submission_report(batch_results[i], profiles[i], sub_dir, error_msg);
        }

        return true;
    } catch (const std::exception& e) {
        if (error_msg) *error_msg = e.what();
        return false;
    }
}

std::string ReportGenerator::generate_console_summary(const SubmissionResult& sub) {
    std::ostringstream oss;
    oss << "=== Evidence Summary for: " << sub.submission_name << " ===\n";
    oss << "Build: " << (sub.build_result.success ? "SUCCESS" : "FAILED") << "\n";
    oss << "Scenarios executed: " << sub.scenario_results.size() << "\n";

    int completed = 0, timed_out = 0, other = 0;
    for (const auto& sr : sub.scenario_results) {
        if (sr.status == ExecutionStatus::COMPLETED) completed++;
        else if (sr.status == ExecutionStatus::TIMED_OUT) timed_out++;
        else other++;

        oss << "  - [" << execution_status_to_string(sr.status) << "] "
            << sr.scenario_id << " (" << sr.scenario_name << ") - "
            << sr.checkpoints.size() << " checkpoints, " << sr.duration_ms << "ms\n";
    }

    oss << "Status totals: " << completed << " completed, " << timed_out << " timed out, "
        << other << " build/launch/error\n";
    oss << "Total duration: " << sub.total_duration_ms << "ms\n";
    return oss.str();
}

std::string ReportGenerator::generate_batch_console_summary(const std::vector<SubmissionResult>& batch_results) {
    std::ostringstream oss;
    oss << "========================================================\n";
    oss << "                BATCH REVIEW COHORT SUMMARY             \n";
    oss << "========================================================\n";
    for (const auto& sub : batch_results) {
        int completed = 0, timed_out = 0, err = 0;
        for (const auto& sr : sub.scenario_results) {
            if (sr.status == ExecutionStatus::COMPLETED) completed++;
            else if (sr.status == ExecutionStatus::TIMED_OUT) timed_out++;
            else err++;
        }
        oss << std::left << std::setw(24) << sub.submission_name
            << " | Build: " << (sub.build_result.success ? "OK  " : "FAIL")
            << " | Completed: " << std::setw(2) << completed
            << " | Timed out: " << std::setw(2) << timed_out
            << " | Errors: " << std::setw(2) << err
            << " | Duration: " << sub.total_duration_ms << "ms\n";
    }
    oss << "========================================================\n";
    return oss.str();
}

} // namespace mem_tac_review
