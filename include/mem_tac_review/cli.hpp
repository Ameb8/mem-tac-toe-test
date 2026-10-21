#pragma once

#include <string>
#include <vector>
#include <optional>

namespace mem_tac_review {

struct RunOptions {
    std::string profile_path;
    std::string scenario_id;
    std::string suite_name = "lab-2";
    std::string output_dir;
    std::optional<int> timeout_sec_override;
    std::optional<int> settling_ms_override;
    bool skip_build = false;
    bool verbose = false;
};

struct BatchOptions {
    std::string profiles_dir;
    std::string suite_name = "lab-2";
    std::string output_dir = "reports/cohort";
    std::optional<int> timeout_sec_override;
    std::optional<int> settling_ms_override;
    bool skip_build = false;
    bool verbose = false;
};

class CommandLineInterface {
public:
    static int run(int argc, char* argv[]);

    static void print_help();
    static void print_version();
    static void list_scenarios();
    static void list_suites();

private:
    static int execute_run(const RunOptions& opts);
    static int execute_batch(const BatchOptions& opts);
    static int execute_verify_scenario(const std::string& path);
};

} // namespace mem_tac_review
