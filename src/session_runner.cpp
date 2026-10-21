#include "mem_tac_review/session_runner.hpp"

#include <iostream>
#include <sstream>
#include <regex>
#include <poll.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <ctime>
#include <iomanip>
#include <thread>

namespace mem_tac_review {

static int64_t current_time_ms() {
    auto now = std::chrono::steady_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
}

static std::string get_current_iso_timestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf;
    gmtime_r(&now_c, &tm_buf);
    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

static std::string get_platform_info() {
    struct utsname u;
    if (uname(&u) == 0) {
        return std::string(u.sysname) + " " + u.release + " (" + u.machine + ")";
    }
    return "Linux";
}

BuildResult SessionRunner::build(const Profile& profile, bool verbose) {
    BuildResult res;
    if (profile.build_cmd().empty()) {
        res.success = true;
        res.output = "No build command configured.";
        return res;
    }

    if (verbose) {
        std::cout << "[BUILD] Running: " << profile.build_cmd() << " in " << profile.work_dir() << std::endl;
    }

    auto start_time = current_time_ms();

    // Use PtyProcess to run build command with clean environment and terminal
    PtyProcess build_proc;
    std::vector<std::string> cmd = {profile.build_cmd()};
    if (!build_proc.spawn(cmd, profile.work_dir(), profile.env(), "build")) {
        res.success = false;
        res.exit_code = -1;
        res.output = "Failed to spawn build process.";
        res.duration_ms = current_time_ms() - start_time;
        return res;
    }

    int timeout_sec = profile.build_timeout_sec();
    auto deadline = start_time + timeout_sec * 1000;

    while (build_proc.is_running()) {
        if (current_time_ms() > deadline) {
            build_proc.terminate();
            res.success = false;
            res.exit_code = 124; // timeout
            res.output = build_proc.accumulated_output() + "\n[BUILD TIMED OUT]";
            res.duration_ms = current_time_ms() - start_time;
            return res;
        }

        struct pollfd pfd;
        pfd.fd = build_proc.master_fd();
        pfd.events = POLLIN;
        pfd.revents = 0;
        int ret = poll(&pfd, 1, 50);
        if (ret > 0 && (pfd.revents & POLLIN)) {
            build_proc.read_available();
        }
    }

    build_proc.read_available();
    res.exit_code = build_proc.exit_code();
    res.success = (res.exit_code == 0);
    res.output = build_proc.accumulated_output();
    res.duration_ms = current_time_ms() - start_time;

    if (verbose) {
        std::cout << "[BUILD] Completed with exit code " << res.exit_code << " in " << res.duration_ms << "ms" << std::endl;
    }

    return res;
}

SessionResult SessionRunner::run_scenario(const Profile& profile, const Scenario& scenario, bool verbose) {
    SessionResult result;
    result.scenario_id = scenario.id();
    result.scenario_name = scenario.name();
    auto session_start_time = current_time_ms();

    if (verbose) {
        std::cout << "[SCENARIO START] " << scenario.id() << " (" << scenario.name() << ")" << std::endl;
    }

    // Set up regex for turn prompt if configured
    std::optional<std::regex> prompt_regex;
    if (!profile.turn_prompt().empty()) {
        try {
            prompt_regex = std::regex(profile.turn_prompt(), std::regex::ECMAScript | std::regex::icase);
        } catch (const std::regex_error& e) {
            if (verbose) {
                std::cerr << "[WARNING] Invalid turn_prompt regex: " << e.what() << std::endl;
            }
        }
    }

    PtyProcess proc1;
    PtyProcess proc2;

    if (scenario.type() == ScenarioType::CLI_ERROR) {
        // CLI error handling scenario
        std::vector<std::string> p1_cmd;
        if (!scenario.cli_args_p1().empty()) {
            p1_cmd = profile.launch_p1();
            if (!p1_cmd.empty()) {
                p1_cmd = {p1_cmd[0]};
                for (const auto& a : scenario.cli_args_p1()) {
                    p1_cmd.push_back(a);
                }
            }
        } else {
            // Drop {N} argument if empty override
            p1_cmd = profile.launch_p1();
            if (!p1_cmd.empty()) {
                p1_cmd = {p1_cmd[0]};
            }
        }

        if (!proc1.spawn(p1_cmd, profile.work_dir(), profile.env(), "p1")) {
            result.status = ExecutionStatus::LAUNCH_ERROR;
            result.status_details = "Failed to spawn process with CLI error arguments";
            result.duration_ms = current_time_ms() - session_start_time;
            return result;
        }

        // Wait up to timeout_sec for process to exit
        auto deadline = current_time_ms() + profile.timeout_sec() * 1000;
        while (proc1.is_running() && current_time_ms() < deadline) {
            struct pollfd pfd = {proc1.master_fd(), POLLIN, 0};
            poll(&pfd, 1, 50);
            proc1.read_available();
        }

        proc1.terminate();
        result.p1_exit_code = proc1.exit_code();
        result.p1_exited = proc1.has_exited();
        result.final_p1_output = proc1.accumulated_output();
        result.full_transcript = proc1.accumulated_output();
        result.status = ExecutionStatus::COMPLETED;
        result.duration_ms = current_time_ms() - session_start_time;

        Checkpoint cp;
        cp.step_index = 1;
        cp.target_connection = "p1";
        std::ostringstream args_oss;
        for (const auto& a : p1_cmd) args_oss << a << " ";
        cp.submitted_input = args_oss.str();
        cp.observed_output = proc1.accumulated_output();
        cp.creator_output = proc1.accumulated_output();
        cp.expected_board = "N/A (CLI parameter validation scenario)";
        cp.expected_outcome = "Diagnostic message and controlled exit";
        cp.boundary_method = "Process termination / timeout";
        cp.timestamp_ms = current_time_ms() - session_start_time;
        result.checkpoints.push_back(cp);

        return result;
    }

    // Normal game scenario
    ReferenceModel ref_model(scenario.grid_size(), scenario.starting_player());

    auto p1_args = profile.format_launch_args(profile.launch_p1(), scenario.grid_size());
    auto p2_args = profile.format_launch_args(profile.launch_p2(), scenario.grid_size());

    bool spawn_ok = false;
    if (profile.topology() == Topology::SINGLE) {
        spawn_ok = proc1.spawn(p1_args, profile.work_dir(), profile.env(), "shared");
    } else {
        spawn_ok = proc1.spawn(p1_args, profile.work_dir(), profile.env(), "p1") &&
                   proc2.spawn(p2_args, profile.work_dir(), profile.env(), "p2");
    }

    if (!spawn_ok) {
        result.status = ExecutionStatus::LAUNCH_ERROR;
        result.status_details = "Failed to launch submission processes";
        result.duration_ms = current_time_ms() - session_start_time;
        return result;
    }

    // Helper lambda to poll and read from both PTYs
    auto poll_and_read = [&](int timeout_ms, std::string& p1_delta, std::string& p2_delta) -> bool {
        struct pollfd pfds[2];
        int nfds = 0;
        int idx_p1 = -1, idx_p2 = -1;

        if (proc1.master_fd() >= 0) {
            idx_p1 = nfds++;
            pfds[idx_p1].fd = proc1.master_fd();
            pfds[idx_p1].events = POLLIN;
            pfds[idx_p1].revents = 0;
        }
        if (proc2.master_fd() >= 0) {
            idx_p2 = nfds++;
            pfds[idx_p2].fd = proc2.master_fd();
            pfds[idx_p2].events = POLLIN;
            pfds[idx_p2].revents = 0;
        }

        if (nfds == 0) return false;

        (void)poll(pfds, nfds, timeout_ms);
        bool got_data = false;

        if (idx_p1 >= 0 && (pfds[idx_p1].revents & (POLLIN | POLLHUP | POLLERR))) {
            std::string s = proc1.read_available();
            if (!s.empty()) {
                p1_delta += s;
                got_data = true;
            }
        }
        if (idx_p2 >= 0 && (pfds[idx_p2].revents & (POLLIN | POLLHUP | POLLERR))) {
            std::string s = proc2.read_available();
            if (!s.empty()) {
                p2_delta += s;
                got_data = true;
            }
        }
        return got_data;
    };

    // Initial settling window for startup banners and initial board
    {
        std::string init_p1, init_p2;
        auto settling_limit = current_time_ms() + profile.settling_ms();
        while (current_time_ms() < settling_limit) {
            poll_and_read(20, init_p1, init_p2);
        }
    }

    result.status = ExecutionStatus::COMPLETED;
    auto max_session_deadline = session_start_time + profile.session_timeout_sec() * 1000;

    for (size_t act_idx = 0; act_idx < scenario.actions().size(); ++act_idx) {
        if (current_time_ms() > max_session_deadline) {
            result.status = ExecutionStatus::TIMED_OUT;
            result.status_details = "Session exceeded maximum allowed duration (" +
                                    std::to_string(profile.session_timeout_sec()) + "s)";
            break;
        }

        const auto& act = scenario.actions()[act_idx];
        std::string conn_target = (profile.topology() == Topology::SINGLE)
                                      ? "shared"
                                      : (act.target_player == Player::P1 ? "p1" : "p2");
        PtyProcess& target_proc = (conn_target == "p2") ? proc2 : proc1;

        // Check if target process has exited prematurely
        if (!target_proc.is_running()) {
            result.status = ExecutionStatus::INTERRUPTED;
            result.status_details = "Process " + conn_target + " exited unexpectedly before action " +
                                    std::to_string(act_idx + 1);
            break;
        }

        // Prepare input text to send
        std::vector<std::string> inputs_to_send;
        if (act.type == ActionType::MOVE || act.type == ActionType::INVALID_COORD || act.type == ActionType::OCCUPIED_CELL) {
            inputs_to_send = profile.format_move_inputs(act.row, act.col);
        } else if (act.type == ActionType::MALFORMED_INPUT) {
            inputs_to_send = {act.raw_input};
        } else if (act.type == ActionType::PAUSE) {
            std::this_thread::sleep_for(std::chrono::milliseconds(act.pause_ms));
        }

        // Apply to reference model
        if (act.type == ActionType::MOVE) {
            ref_model.apply_move(act.target_player, act.row, act.col);
        }

        std::string submitted_full;
        for (const auto& in_str : inputs_to_send) {
            submitted_full += in_str;
        }

        // Send input
        if (act.type == ActionType::EOF_INPUT) {
            target_proc.close_input();
            submitted_full = "<EOF>";
        } else {
            for (const auto& in_str : inputs_to_send) {
                target_proc.send_input(in_str);
            }
        }

        // Wait for boundary
        std::string delta_p1, delta_p2;
        std::string boundary_method = "Quiet-window settling";
        bool timed_out = false;

        auto action_start = current_time_ms();
        auto action_deadline = action_start + profile.timeout_sec() * 1000;
        auto last_data_received = action_start;
        bool has_received_data = false;

        while (true) {
            auto now = current_time_ms();
            if (now >= action_deadline) {
                timed_out = true;
                boundary_method = "Action response timeout (" + std::to_string(profile.timeout_sec()) + "s)";
                break;
            }

            std::string chunk_p1, chunk_p2;
            bool got_chunk = poll_and_read(20, chunk_p1, chunk_p2);
            if (got_chunk) {
                delta_p1 += chunk_p1;
                delta_p2 += chunk_p2;
                last_data_received = current_time_ms();
                has_received_data = true;

                // Check prompt regex on target or creator output
                if (prompt_regex.has_value()) {
                    std::string& check_buf = (conn_target == "p2") ? delta_p2 : delta_p1;
                    if (std::regex_search(check_buf, *prompt_regex)) {
                        boundary_method = "Configured move prompt regex: " + profile.turn_prompt();
                        // Brief extra wait for settling
                        std::this_thread::sleep_for(std::chrono::milliseconds(20));
                        poll_and_read(10, delta_p1, delta_p2);
                        break;
                    }
                }
            }

            // Quiet window check
            if (has_received_data && (now - last_data_received >= profile.settling_ms())) {
                boundary_method = "Quiet-window settling (" + std::to_string(profile.settling_ms()) + " ms)";
                break;
            }

            // If processes exited
            if (!proc1.is_running() && (profile.topology() == Topology::SINGLE || !proc2.is_running())) {
                boundary_method = "Process termination";
                break;
            }
        }

        // Collect checkpoint
        Checkpoint cp;
        cp.step_index = act_idx + 1;
        cp.action = act;
        cp.submitted_input = submitted_full;
        cp.intended_player = act.target_player;
        cp.target_connection = conn_target;
        cp.expected_board = ref_model.format_board();
        cp.expected_next_player = ref_model.is_game_over()
                                      ? "None (Game over)"
                                      : player_to_string(ref_model.current_player());
        cp.expected_outcome = outcome_to_string(ref_model.outcome());

        // Observed output
        if (profile.topology() == Topology::SINGLE) {
            cp.observed_output = delta_p1;
            cp.creator_output = delta_p1;
        } else {
            cp.observed_output = "--- [P1 Output] ---\n" + delta_p1 + "\n--- [P2 Output] ---\n" + delta_p2;
            cp.creator_output = (profile.creator() == CreatorRole::P2) ? delta_p2 : delta_p1;
        }

        cp.boundary_method = boundary_method;
        cp.timestamp_ms = current_time_ms() - session_start_time;
        cp.timed_out = timed_out;

        result.checkpoints.push_back(cp);

        if (timed_out) {
            result.status = ExecutionStatus::TIMED_OUT;
            result.status_details = "Action " + std::to_string(act_idx + 1) + " timed out after " +
                                    std::to_string(profile.timeout_sec()) + "s";
            break;
        }
    }

    // Brief post-game settling
    {
        std::string end_p1, end_p2;
        auto end_deadline = current_time_ms() + profile.settling_ms() + 50;
        while (current_time_ms() < end_deadline) {
            poll_and_read(20, end_p1, end_p2);
        }
    }

    // Clean up processes
    proc1.terminate();
    if (profile.topology() == Topology::DUAL) {
        proc2.terminate();
    }

    result.duration_ms = current_time_ms() - session_start_time;
    result.p1_exit_code = proc1.exit_code();
    result.p1_exited = proc1.has_exited();
    result.final_p1_output = proc1.accumulated_output();

    if (profile.topology() == Topology::DUAL) {
        result.p2_exit_code = proc2.exit_code();
        result.p2_exited = proc2.has_exited();
        result.final_p2_output = proc2.accumulated_output();
        result.full_transcript = "=== P1 TRANSCRIPT ===\n" + proc1.accumulated_output() +
                                 "\n\n=== P2 TRANSCRIPT ===\n" + proc2.accumulated_output();
    } else {
        result.final_shared_output = proc1.accumulated_output();
        result.full_transcript = "=== SHARED TERMINAL TRANSCRIPT ===\n" + proc1.accumulated_output();
    }

    result.expected_final_outcome = outcome_to_string(ref_model.outcome());

    if (verbose) {
        std::cout << "[SCENARIO END] " << scenario.id() << " -> " << execution_status_to_string(result.status)
                  << " (" << result.checkpoints.size() << " checkpoints, " << result.duration_ms << "ms)" << std::endl;
    }

    return result;
}

SubmissionResult SessionRunner::run_suite(const Profile& profile,
                                          const std::vector<const Scenario*>& scenarios,
                                          bool skip_build,
                                          bool verbose) {
    SubmissionResult sub_res;
    sub_res.submission_name = profile.name();
    sub_res.profile_path = profile.profile_path();
    sub_res.platform_info = get_platform_info();
    sub_res.run_timestamp = get_current_iso_timestamp();

    auto suite_start = current_time_ms();

    if (!skip_build) {
        sub_res.build_result = build(profile, verbose);
        if (!sub_res.build_result.success) {
            // Build failed! Mark all scenarios as BUILD_ERROR
            for (const auto* sc : scenarios) {
                SessionResult sr;
                sr.scenario_id = sc->id();
                sr.scenario_name = sc->name();
                sr.status = ExecutionStatus::BUILD_ERROR;
                sr.status_details = "Build failed with exit code " + std::to_string(sub_res.build_result.exit_code);
                sub_res.scenario_results.push_back(sr);
            }
            sub_res.total_duration_ms = current_time_ms() - suite_start;
            return sub_res;
        }
    } else {
        sub_res.build_result.success = true;
        sub_res.build_result.output = "Build step skipped as requested.";
    }

    for (const auto* sc : scenarios) {
        auto sr = run_scenario(profile, *sc, verbose);
        sub_res.scenario_results.push_back(sr);
    }

    sub_res.total_duration_ms = current_time_ms() - suite_start;
    return sub_res;
}

} // namespace mem_tac_review
