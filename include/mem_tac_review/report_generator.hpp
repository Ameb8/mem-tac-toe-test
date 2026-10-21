#pragma once

#include "mem_tac_review/session_runner.hpp"

#include <string>
#include <filesystem>

namespace mem_tac_review {

class ReportGenerator {
public:
    ReportGenerator() = default;

    // Generates a self-contained HTML report for a single submission
    static std::string generate_submission_html(const SubmissionResult& result, const Profile& profile);

    // Writes the submission HTML report to disk (e.g. <output_dir>/index.html)
    static bool write_submission_report(const SubmissionResult& result,
                                        const Profile& profile,
                                        const std::filesystem::path& output_dir,
                                        std::string* error_msg = nullptr);

    // Generates a batch overview HTML report linking to individual submissions
    static std::string generate_batch_html(const std::vector<SubmissionResult>& batch_results,
                                           const std::string& suite_name,
                                           const std::string& timestamp);

    // Writes the batch report to disk (e.g. <output_dir>/index.html) and subdirectories for each submission
    static bool write_batch_report(const std::vector<SubmissionResult>& batch_results,
                                   const std::vector<Profile>& profiles,
                                   const std::string& suite_name,
                                   const std::filesystem::path& output_dir,
                                   std::string* error_msg = nullptr);

    // Generates a concise console summary of results
    static std::string generate_console_summary(const SubmissionResult& result);
    static std::string generate_batch_console_summary(const std::vector<SubmissionResult>& batch_results);

    // Utility: HTML escape string
    static std::string html_escape(const std::string& data);

    // Utility: clean/strip ANSI terminal color codes
    static std::string strip_ansi(const std::string& str);
};

} // namespace mem_tac_review
