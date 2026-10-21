#pragma once

#include "mem_tac_review/reference_model.hpp"

#include <string>
#include <vector>
#include <map>
#include <optional>
#include <filesystem>

namespace mem_tac_review {

enum class Topology {
    DUAL,   // Two independent processes
    SINGLE  // Single launcher / fork sharing a terminal
};

inline std::string topology_to_string(Topology t) {
    return (t == Topology::DUAL) ? "dual" : "single";
}

enum class CreatorRole {
    P1,
    P2,
    SHARED
};

inline std::string creator_role_to_string(CreatorRole r) {
    switch (r) {
        case CreatorRole::P1: return "p1";
        case CreatorRole::P2: return "p2";
        case CreatorRole::SHARED: return "shared";
    }
    return "unknown";
}

enum class CoordOrder {
    ROW_COL,
    COL_ROW
};

inline std::string coord_order_to_string(CoordOrder o) {
    return (o == CoordOrder::ROW_COL) ? "row_col" : "col_row";
}

struct ProfileValidationResult {
    bool ok = true;
    std::vector<std::string> errors;

    void add_error(const std::string& err) {
        ok = false;
        errors.push_back(err);
    }
};

class Profile {
public:
    Profile() = default;

    // Load from YAML or JSON string or file
    static Profile from_yaml_string(const std::string& content);
    static Profile load_from_file(const std::filesystem::path& path);
    static std::vector<Profile> load_directory(const std::filesystem::path& dir);

    ProfileValidationResult validate() const;

    // Getters
    const std::string& name() const noexcept { return name_; }
    const std::string& build_cmd() const noexcept { return build_cmd_; }
    const std::string& work_dir() const noexcept { return work_dir_; }
    Topology topology() const noexcept { return topology_; }
    const std::vector<std::string>& launch_p1() const noexcept { return launch_p1_; }
    const std::vector<std::string>& launch_p2() const noexcept { return launch_p2_; }
    CreatorRole creator() const noexcept { return creator_; }
    Player starting_player() const noexcept { return starting_player_; }
    int coord_base() const noexcept { return coord_base_; }
    CoordOrder coord_order() const noexcept { return coord_order_; }
    const std::vector<std::string>& input_format_prompts() const noexcept { return input_format_prompts_; }
    const std::string& turn_prompt() const noexcept { return turn_prompt_; }
    int settling_ms() const noexcept { return settling_ms_; }
    int timeout_sec() const noexcept { return timeout_sec_; }
    int session_timeout_sec() const noexcept { return session_timeout_sec_; }
    int build_timeout_sec() const noexcept { return build_timeout_sec_; }
    const std::map<std::string, std::string>& env() const noexcept { return env_; }
    const std::filesystem::path& profile_path() const noexcept { return profile_path_; }

    // Setters
    void set_name(std::string n) { name_ = std::move(n); }
    void set_build_cmd(std::string c) { build_cmd_ = std::move(c); }
    void set_work_dir(std::string d) { work_dir_ = std::move(d); }
    void set_topology(Topology t) { topology_ = t; }
    void set_launch_p1(std::vector<std::string> cmd) { launch_p1_ = std::move(cmd); }
    void set_launch_p2(std::vector<std::string> cmd) { launch_p2_ = std::move(cmd); }
    void set_creator(CreatorRole c) { creator_ = c; }
    void set_starting_player(Player p) { starting_player_ = p; }
    void set_coord_base(int b) { coord_base_ = b; }
    void set_coord_order(CoordOrder o) { coord_order_ = o; }
    void set_input_format(const std::string& fmt);
    void set_input_format_prompts(std::vector<std::string> prompts) { input_format_prompts_ = std::move(prompts); }
    void set_turn_prompt(std::string p) { turn_prompt_ = std::move(p); }
    void set_settling_ms(int ms) { settling_ms_ = ms; }
    void set_timeout_sec(int sec) { timeout_sec_ = sec; }
    void set_session_timeout_sec(int sec) { session_timeout_sec_ = sec; }
    void set_build_timeout_sec(int sec) { build_timeout_sec_ = sec; }
    void set_profile_path(std::filesystem::path p) { profile_path_ = std::move(p); }

    // Helpers to translate logical coordinates into submission input strings
    std::vector<std::string> format_move_inputs(int logical_row, int logical_col) const;

    // Substitute {N} in launch arguments
    std::vector<std::string> format_launch_args(const std::vector<std::string>& cmd, int grid_size) const;

private:
    std::string name_ = "unnamed_submission";
    std::string build_cmd_ = "";
    std::string work_dir_ = ".";
    Topology topology_ = Topology::DUAL;
    std::vector<std::string> launch_p1_;
    std::vector<std::string> launch_p2_;
    CreatorRole creator_ = CreatorRole::P1;
    Player starting_player_ = Player::P1;
    int coord_base_ = 0;
    CoordOrder coord_order_ = CoordOrder::ROW_COL;
    std::vector<std::string> input_format_prompts_ = {"{r} {c}\n"};
    std::string turn_prompt_ = "";
    int settling_ms_ = 100;
    int timeout_sec_ = 10;
    int session_timeout_sec_ = 60;
    int build_timeout_sec_ = 60;
    std::map<std::string, std::string> env_;
    std::filesystem::path profile_path_;
};

} // namespace mem_tac_review
