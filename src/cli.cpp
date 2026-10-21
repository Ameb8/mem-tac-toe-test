#include "mem_tac_review/cli.hpp"
#include "mem_tac_review/scenario_library.hpp"
#include "mem_tac_review/profile.hpp"
#include "mem_tac_review/session_runner.hpp"
#include "mem_tac_review/report_generator.hpp"

#include <iostream>
#include <iomanip>
#include <filesystem>

namespace mem_tac_review {

void CommandLineInterface::print_version() {
    std::cout << "mem-tac-review version 1.0.0 (C++20 Linux)" << std::endl;
}

void CommandLineInterface::print_help() {
    std::cout << R"(mem-tac-review - Mem-tac-toe testing and review evidence collection harness

Usage:
  mem-tac-review run --profile <path> [options]
  mem-tac-review batch --profiles <dir> [options]
  mem-tac-review list [--scenarios | --suites]
  mem-tac-review verify-scenario --file <path>
  mem-tac-review --help | -h
  mem-tac-review --version

Commands:
  run                   Execute scenarios for a single submission profile
  batch                 Execute scenarios across all profiles in a directory
  list                  Display available canonical scenarios or suites
  verify-scenario       Validate syntax and rule consistency of a custom scenario file

Run Options:
  --profile <file>      Submission profile YAML or JSON file (required for 'run')
  --scenario <name>     Run a specific scenario by ID or alias (e.g. diagonal-win)
  --suite <name>        Scenario suite to run (default: 'lab-2', or 'p1', 'p2', 'all')
  --output <dir>        Output directory for HTML evidence report (default: reports/<name>)
  --timeout <sec>       Override per-action response timeout in seconds
  --settling-ms <ms>    Override quiet-window settling duration in milliseconds
  --no-build            Skip build command execution
  --verbose, -v         Enable detailed progress output to stdout

Batch Options:
  --profiles <dir>      Directory containing profile YAML or JSON files (required for 'batch')
  --suite <name>        Scenario suite to run across all profiles (default: 'lab-2')
  --output <dir>        Output directory for batch report (default: reports/cohort)
  --timeout <sec>       Override per-action response timeout in seconds
  --settling-ms <ms>    Override quiet-window settling duration in milliseconds
  --no-build            Skip build command execution
  --verbose, -v         Enable detailed progress output to stdout

Examples:
  mem-tac-review run --profile profiles/student-a.yaml --scenario diagonal-win --output reports/trial
  mem-tac-review run --profile profiles/student-a.yaml --suite lab-2 --output reports/student-a
  mem-tac-review batch --profiles profiles/ --suite lab-2 --output reports/cohort
)";
}

void CommandLineInterface::list_scenarios() {
    ScenarioLibrary lib;
    auto scenarios = lib.list_all();
    std::cout << "Available Scenarios (" << scenarios.size() << " total):\n";
    std::cout << "--------------------------------------------------------------------------------\n";
    std::cout << std::left << std::setw(22) << "ID"
              << std::setw(32) << "Name"
              << std::setw(6) << "Size"
              << "Suites\n";
    std::cout << "--------------------------------------------------------------------------------\n";
    for (const auto* sc : scenarios) {
        std::string suites_str;
        for (size_t i = 0; i < sc->suites().size(); ++i) {
            if (i > 0) suites_str += ", ";
            suites_str += sc->suites()[i];
        }
        std::cout << std::left << std::setw(22) << sc->id()
                  << std::setw(32) << sc->name().substr(0, 30)
                  << std::setw(6) << (std::to_string(sc->grid_size()) + "x" + std::to_string(sc->grid_size()))
                  << suites_str << "\n";
    }
    std::cout << "--------------------------------------------------------------------------------\n";
}

void CommandLineInterface::list_suites() {
    ScenarioLibrary lib;
    auto suites = lib.list_suites();
    std::cout << "Available Suites:\n";
    for (const auto& s : suites) {
        auto in_suite = lib.get_by_suite(s);
        std::cout << "  - " << std::left << std::setw(12) << s
                  << "(" << in_suite.size() << " scenarios)\n";
    }
}

int CommandLineInterface::execute_verify_scenario(const std::string& path) {
    if (path.empty()) {
        std::cerr << "Error: --file argument is required for verify-scenario\n";
        return 1;
    }
    ScenarioLibrary lib;
    std::string err;
    if (!lib.load_file(path, &err)) {
        std::cerr << "Scenario verification failed:\n" << err << "\n";
        return 2;
    }
    std::cout << "Scenario file '" << path << "' is valid.\n";
    return 0;
}

int CommandLineInterface::execute_run(const RunOptions& opts) {
    if (opts.profile_path.empty()) {
        std::cerr << "Error: --profile <path> is required for 'run'\n";
        return 1;
    }

    Profile profile;
    try {
        profile = Profile::load_from_file(opts.profile_path);
    } catch (const std::exception& e) {
        std::cerr << "Error loading profile: " << e.what() << "\n";
        return 2;
    }

    auto val = profile.validate();
    if (!val.ok) {
        std::cerr << "Profile validation failed:\n";
        for (const auto& err : val.errors) {
            std::cerr << "  - " << err << "\n";
        }
        return 2;
    }

    if (opts.timeout_sec_override.has_value()) {
        profile.set_timeout_sec(*opts.timeout_sec_override);
    }
    if (opts.settling_ms_override.has_value()) {
        profile.set_settling_ms(*opts.settling_ms_override);
    }

    ScenarioLibrary lib;
    std::vector<const Scenario*> scenarios_to_run;

    if (!opts.scenario_id.empty()) {
        const Scenario* sc = lib.get(opts.scenario_id);
        if (!sc) {
            std::cerr << "Error: Unknown scenario '" << opts.scenario_id << "'\n";
            return 1;
        }
        scenarios_to_run.push_back(sc);
    } else {
        scenarios_to_run = lib.get_by_suite(opts.suite_name);
        if (scenarios_to_run.empty()) {
            std::cerr << "Error: No scenarios found in suite '" << opts.suite_name << "'\n";
            return 1;
        }
    }

    std::filesystem::path out_dir = opts.output_dir.empty()
                                        ? std::filesystem::path("reports") / profile.name()
                                        : std::filesystem::path(opts.output_dir);

    std::cout << "Starting mem-tac-review execution for: " << profile.name() << "\n";
    std::cout << "Scenarios: " << scenarios_to_run.size() << " | Output: " << out_dir.string() << "\n\n";

    SessionRunner runner;
    auto sub_result = runner.run_suite(profile, scenarios_to_run, opts.skip_build, opts.verbose);

    std::string err_msg;
    if (!ReportGenerator::write_submission_report(sub_result, profile, out_dir, &err_msg)) {
        std::cerr << "Error writing report: " << err_msg << "\n";
        return 2;
    }

    std::cout << ReportGenerator::generate_console_summary(sub_result);
    std::cout << "\nReport successfully generated: " << (out_dir / "index.html").string() << "\n";
    return 0;
}

int CommandLineInterface::execute_batch(const BatchOptions& opts) {
    if (opts.profiles_dir.empty()) {
        std::cerr << "Error: --profiles <dir> is required for 'batch'\n";
        return 1;
    }

    auto profiles = Profile::load_directory(opts.profiles_dir);
    if (profiles.empty()) {
        std::cerr << "Error: No valid profile files found in directory: " << opts.profiles_dir << "\n";
        return 1;
    }

    ScenarioLibrary lib;
    auto scenarios_to_run = lib.get_by_suite(opts.suite_name);
    if (scenarios_to_run.empty()) {
        std::cerr << "Error: No scenarios found in suite '" << opts.suite_name << "'\n";
        return 1;
    }

    std::filesystem::path out_dir = opts.output_dir.empty() ? "reports/cohort" : opts.output_dir;

    std::cout << "Starting batch review for " << profiles.size() << " profiles in suite '"
              << opts.suite_name << "'\n";
    std::cout << "Cohort output: " << out_dir.string() << "\n\n";

    std::vector<SubmissionResult> batch_results;
    SessionRunner runner;

    for (auto& profile : profiles) {
        if (opts.timeout_sec_override.has_value()) {
            profile.set_timeout_sec(*opts.timeout_sec_override);
        }
        if (opts.settling_ms_override.has_value()) {
            profile.set_settling_ms(*opts.settling_ms_override);
        }

        std::cout << "--> Running submission: " << profile.name() << " ... " << std::flush;
        auto sub_res = runner.run_suite(profile, scenarios_to_run, opts.skip_build, opts.verbose);
        batch_results.push_back(sub_res);
        std::cout << "Done (" << sub_res.total_duration_ms << "ms)\n";
    }

    std::string err_msg;
    if (!ReportGenerator::write_batch_report(batch_results, profiles, opts.suite_name, out_dir, &err_msg)) {
        std::cerr << "Error writing batch report: " << err_msg << "\n";
        return 2;
    }

    std::cout << "\n" << ReportGenerator::generate_batch_console_summary(batch_results);
    std::cout << "\nBatch report successfully generated: " << (out_dir / "index.html").string() << "\n";
    return 0;
}

int CommandLineInterface::run(int argc, char* argv[]) {
    if (argc < 2) {
        print_help();
        return 1;
    }

    std::string command = argv[1];

    if (command == "--help" || command == "-h" || command == "help") {
        print_help();
        return 0;
    }
    if (command == "--version" || command == "-v" || command == "version") {
        print_version();
        return 0;
    }
    if (command == "list") {
        bool show_suites = false;
        for (int i = 2; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--suites") show_suites = true;
        }
        if (show_suites) {
            list_suites();
        } else {
            list_scenarios();
        }
        return 0;
    }
    if (command == "verify-scenario") {
        std::string file_path;
        for (int i = 2; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--file" && i + 1 < argc) {
                file_path = argv[++i];
            }
        }
        return execute_verify_scenario(file_path);
    }
    if (command == "run") {
        RunOptions opts;
        for (int i = 2; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--profile" && i + 1 < argc) {
                opts.profile_path = argv[++i];
            } else if (arg == "--scenario" && i + 1 < argc) {
                opts.scenario_id = argv[++i];
            } else if (arg == "--suite" && i + 1 < argc) {
                opts.suite_name = argv[++i];
            } else if (arg == "--output" && i + 1 < argc) {
                opts.output_dir = argv[++i];
            } else if (arg == "--timeout" && i + 1 < argc) {
                opts.timeout_sec_override = std::stoi(argv[++i]);
            } else if (arg == "--settling-ms" && i + 1 < argc) {
                opts.settling_ms_override = std::stoi(argv[++i]);
            } else if (arg == "--no-build") {
                opts.skip_build = true;
            } else if (arg == "--verbose" || arg == "-v") {
                opts.verbose = true;
            } else {
                std::cerr << "Unknown run option: " << arg << "\n";
                print_help();
                return 1;
            }
        }
        return execute_run(opts);
    }
    if (command == "batch") {
        BatchOptions opts;
        for (int i = 2; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--profiles" && i + 1 < argc) {
                opts.profiles_dir = argv[++i];
            } else if (arg == "--suite" && i + 1 < argc) {
                opts.suite_name = argv[++i];
            } else if (arg == "--output" && i + 1 < argc) {
                opts.output_dir = argv[++i];
            } else if (arg == "--timeout" && i + 1 < argc) {
                opts.timeout_sec_override = std::stoi(argv[++i]);
            } else if (arg == "--settling-ms" && i + 1 < argc) {
                opts.settling_ms_override = std::stoi(argv[++i]);
            } else if (arg == "--no-build") {
                opts.skip_build = true;
            } else if (arg == "--verbose" || arg == "-v") {
                opts.verbose = true;
            } else {
                std::cerr << "Unknown batch option: " << arg << "\n";
                print_help();
                return 1;
            }
        }
        return execute_batch(opts);
    }

    std::cerr << "Unknown command: " << command << "\n";
    print_help();
    return 1;
}

} // namespace mem_tac_review
