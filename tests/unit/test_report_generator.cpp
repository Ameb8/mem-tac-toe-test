#include "mem_tac_review/report_generator.hpp"

#include <cassert>
#include <iostream>

using namespace mem_tac_review;

void test_html_escape() {
    std::string raw = "<hello & 'world' \"test\">";
    std::string escaped = ReportGenerator::html_escape(raw);
    assert(escaped == "&lt;hello &amp; &#39;world&#39; &quot;test&quot;&gt;");
}

void test_strip_ansi() {
    std::string raw = "\x1B[31mRed Text\x1B[0m and \x1B[1mBold\x1B[0m";
    std::string clean = ReportGenerator::strip_ansi(raw);
    assert(clean == "Red Text and Bold");
}

void test_submission_report_generation() {
    SubmissionResult sub;
    sub.submission_name = "test_student";
    sub.platform_info = "Linux 6.1 (aarch64)";
    sub.run_timestamp = "2026-10-21T07:00:00Z";
    sub.build_result.success = true;
    sub.build_result.output = "Build ok";

    SessionResult sr;
    sr.scenario_id = "g01_top_row";
    sr.scenario_name = "Top row win";
    sr.status = ExecutionStatus::COMPLETED;
    sr.duration_ms = 450;
    sr.expected_final_outcome = "P1 (X) wins";

    Checkpoint cp;
    cp.step_index = 1;
    cp.submitted_input = "0 0\n";
    cp.target_connection = "p1";
    cp.expected_board = "X . .\n. . .\n. . .";
    cp.expected_next_player = "O";
    cp.expected_outcome = "In progress";
    cp.observed_output = "X . .\n. . .\n. . .";
    cp.creator_output = "X . .\n. . .\n. . .";
    cp.boundary_method = "Quiet-window settling (100 ms)";
    sr.checkpoints.push_back(cp);

    sub.scenario_results.push_back(sr);

    Profile p;
    p.set_name("test_student");

    std::string html = ReportGenerator::generate_submission_html(sub, p);
    assert(!html.empty());
    assert(html.find("test_student") != std::string::npos);
    assert(html.find("Expected Reference State") != std::string::npos);
    assert(html.find("Observed Creator Terminal Output") != std::string::npos);
    assert(html.find("Quiet-window settling (100 ms)") != std::string::npos);
    assert(html.find("<!DOCTYPE html>") != std::string::npos);

    // Verify writing to disk
    std::filesystem::path out_dir = "tests/test_output_report";
    assert(ReportGenerator::write_submission_report(sub, p, out_dir));
    assert(std::filesystem::exists(out_dir / "index.html"));
    std::filesystem::remove_all(out_dir);
}

void test_batch_report_generation() {
    SubmissionResult sub1;
    sub1.submission_name = "student_a";
    sub1.build_result.success = true;

    SubmissionResult sub2;
    sub2.submission_name = "student_b";
    sub2.build_result.success = false;

    std::vector<SubmissionResult> batch = {sub1, sub2};
    std::string html = ReportGenerator::generate_batch_html(batch, "lab-2", "2026-10-21T07:00:00Z");
    assert(html.find("student_a") != std::string::npos);
    assert(html.find("student_b") != std::string::npos);
    assert(html.find("./student_a/index.html") != std::string::npos);
}

int main() {
    test_html_escape();
    test_strip_ansi();
    test_submission_report_generation();
    test_batch_report_generation();

    std::cout << "All ReportGenerator tests passed!" << std::endl;
    return 0;
}
