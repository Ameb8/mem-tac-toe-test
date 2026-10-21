#pragma once

#include "mem_tac_review/profile.hpp"
#include "mem_tac_review/scenario.hpp"
#include "mem_tac_review/pty_session.hpp"

#include <string>
#include <vector>
#include <chrono>
#include <filesystem>

namespace mem_tac_review {

struct BuildResult {
    bool success = false;
    int exit_code = 0;
    std::string output;
    int64_t duration_ms = 0;
};

enum class ExecutionStatus {
    COMPLETED,
    BUILD_ERROR,
    LAUNCH_ERROR,
    TIMED_OUT,
    INTERRUPTED,
    NOT_RUN
};

inline std::string execution_status_to_string(ExecutionStatus s) {
    switch (s) {
        case ExecutionStatus::COMPLETED: return "completed";
        case ExecutionStatus::BUILD_ERROR: return "build error";
        case ExecutionStatus::LAUNCH_ERROR: return "launch error";
        case ExecutionStatus::TIMED_OUT: return "timed out";
        case ExecutionStatus::INTERRUPTED: return "interrupted";
        case ExecutionStatus::NOT_RUN: return "not run";
    }
    return "unknown";
}

struct Checkpoint {
    size_t step_index = 0;
    ScenarioAction action;
    std::string submitted_input;
    Player intended_player = Player::P1;
    std::string target_connection;
    std::string expected_board;
    std::string expected_next_player;
    std::string expected_outcome;
    std::string observed_output;
    std::string creator_output;
    std::string boundary_method;
    int64_t timestamp_ms = 0;
    bool timed_out = false;
};

struct SessionResult {
    std::string scenario_id;
    std::string scenario_name;
    ExecutionStatus status = ExecutionStatus::NOT_RUN;
    std::string status_details;
    int64_t duration_ms = 0;
    std::vector<Checkpoint> checkpoints;
    std::string final_p1_output;
    std::string final_p2_output;
    std::string final_shared_output;
    std::string expected_final_outcome;
    int p1_exit_code = 0;
    int p2_exit_code = 0;
    bool p1_exited = false;
    bool p2_exited = false;
    std::string full_transcript;
};

struct SubmissionResult {
    std::string submission_name;
    std::filesystem::path profile_path;
    BuildResult build_result;
    std::vector<SessionResult> scenario_results;
    int64_t total_duration_ms = 0;
    std::string platform_info;
    std::string run_timestamp;
};

class SessionRunner {
public:
    SessionRunner() = default;

    // Runs the build command specified in the profile
    BuildResult build(const Profile& profile, bool verbose = false);

    // Executes a single scenario against the submission profile
    SessionResult run_scenario(const Profile& profile, const Scenario& scenario, bool verbose = false);

    // Runs a suite of scenarios against a profile
    SubmissionResult run_suite(const Profile& profile,
                               const std::vector<const Scenario*>& scenarios,
                               bool skip_build = false,
                               bool verbose = false);
};

} // namespace mem_tac_review
