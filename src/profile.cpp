#include "mem_tac_review/profile.hpp"

#include <yaml-cpp/yaml.h>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>

namespace mem_tac_review {

static std::string replace_all(std::string str, const std::string& from, const std::string& to) {
    if (from.empty()) return str;
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
    return str;
}

void Profile::set_input_format(const std::string& fmt) {
    input_format_prompts_ = {fmt};
}

Profile Profile::from_yaml_string(const std::string& content) {
    YAML::Node node = YAML::Load(content);
    Profile p;

    if (node["name"]) {
        p.name_ = node["name"].as<std::string>();
    }
    if (node["build_cmd"]) {
        p.build_cmd_ = node["build_cmd"].as<std::string>();
    }
    if (node["work_dir"]) {
        p.work_dir_ = node["work_dir"].as<std::string>();
    }
    if (node["topology"]) {
        std::string top = node["topology"].as<std::string>();
        if (top == "single") {
            p.topology_ = Topology::SINGLE;
        } else {
            p.topology_ = Topology::DUAL;
        }
    }
    if (node["launch_p1"]) {
        p.launch_p1_.clear();
        for (const auto& arg : node["launch_p1"]) {
            p.launch_p1_.push_back(arg.as<std::string>());
        }
    }
    if (node["launch_p2"]) {
        p.launch_p2_.clear();
        for (const auto& arg : node["launch_p2"]) {
            p.launch_p2_.push_back(arg.as<std::string>());
        }
    }
    if (node["creator"]) {
        std::string c = node["creator"].as<std::string>();
        if (c == "p2") {
            p.creator_ = CreatorRole::P2;
        } else if (c == "shared") {
            p.creator_ = CreatorRole::SHARED;
        } else {
            p.creator_ = CreatorRole::P1;
        }
    }
    if (node["starting_player"]) {
        std::string sp = node["starting_player"].as<std::string>();
        if (sp == "p2") {
            p.starting_player_ = Player::P2;
        } else {
            p.starting_player_ = Player::P1;
        }
    }
    if (node["coord_base"]) {
        p.coord_base_ = node["coord_base"].as<int>();
    }
    if (node["coord_order"]) {
        std::string order = node["coord_order"].as<std::string>();
        if (order == "col_row") {
            p.coord_order_ = CoordOrder::COL_ROW;
        } else {
            p.coord_order_ = CoordOrder::ROW_COL;
        }
    }
    if (node["input_format"]) {
        p.input_format_prompts_.clear();
        if (node["input_format"].IsSequence()) {
            for (const auto& item : node["input_format"]) {
                p.input_format_prompts_.push_back(item.as<std::string>());
            }
        } else {
            p.input_format_prompts_.push_back(node["input_format"].as<std::string>());
        }
    }
    if (node["turn_prompt"]) {
        p.turn_prompt_ = node["turn_prompt"].as<std::string>();
    }
    if (node["settling_ms"]) {
        p.settling_ms_ = node["settling_ms"].as<int>();
    }
    if (node["timeout_sec"]) {
        p.timeout_sec_ = node["timeout_sec"].as<int>();
    }
    if (node["session_timeout_sec"]) {
        p.session_timeout_sec_ = node["session_timeout_sec"].as<int>();
    }
    if (node["build_timeout_sec"]) {
        p.build_timeout_sec_ = node["build_timeout_sec"].as<int>();
    }
    if (node["env"] && node["env"].IsMap()) {
        for (YAML::const_iterator it = node["env"].begin(); it != node["env"].end(); ++it) {
            p.env_[it->first.as<std::string>()] = it->second.as<std::string>();
        }
    }

    return p;
}

Profile Profile::load_from_file(const std::filesystem::path& path) {
    std::ifstream fin(path);
    if (!fin.is_open()) {
        throw std::runtime_error("Could not open profile file: " + path.string());
    }
    std::stringstream buffer;
    buffer << fin.rdbuf();
    Profile p = from_yaml_string(buffer.str());
    p.set_profile_path(path);
    return p;
}

std::vector<Profile> Profile::load_directory(const std::filesystem::path& dir) {
    std::vector<Profile> profiles;
    if (!std::filesystem::exists(dir) || !std::filesystem::is_directory(dir)) {
        return profiles;
    }
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        if (entry.is_regular_file()) {
            auto ext = entry.path().extension();
            if (ext == ".yaml" || ext == ".yml" || ext == ".json") {
                try {
                    profiles.push_back(load_from_file(entry.path()));
                } catch (...) {
                    // Skip or ignore unreadable files
                }
            }
        }
    }
    // Sort profiles deterministically by name
    std::sort(profiles.begin(), profiles.end(), [](const Profile& a, const Profile& b) {
        return a.name() < b.name();
    });
    return profiles;
}

ProfileValidationResult Profile::validate() const {
    ProfileValidationResult res;
    if (name_.empty()) {
        res.add_error("Profile 'name' cannot be empty");
    }
    if (launch_p1_.empty()) {
        res.add_error("Profile 'launch_p1' command array cannot be empty");
    }
    if (topology_ == Topology::DUAL && launch_p2_.empty()) {
        res.add_error("Profile with 'dual' topology requires 'launch_p2' command array");
    }
    if (coord_base_ != 0 && coord_base_ != 1) {
        res.add_error("Profile 'coord_base' must be 0 or 1");
    }
    if (settling_ms_ < 0) {
        res.add_error("Profile 'settling_ms' cannot be negative");
    }
    if (timeout_sec_ <= 0) {
        res.add_error("Profile 'timeout_sec' must be positive");
    }
    if (session_timeout_sec_ <= 0) {
        res.add_error("Profile 'session_timeout_sec' must be positive");
    }
    return res;
}

std::vector<std::string> Profile::format_move_inputs(int logical_row, int logical_col) const {
    int user_r = logical_row + coord_base_;
    int user_c = logical_col + coord_base_;

    int first_val = (coord_order_ == CoordOrder::ROW_COL) ? user_r : user_c;
    int second_val = (coord_order_ == CoordOrder::ROW_COL) ? user_c : user_r;

    std::string s_r = std::to_string(user_r);
    std::string s_c = std::to_string(user_c);
    std::string s_first = std::to_string(first_val);
    std::string s_second = std::to_string(second_val);

    std::vector<std::string> inputs;
    for (const auto& prompt : input_format_prompts_) {
        std::string s = prompt;
        s = replace_all(s, "{r}", s_r);
        s = replace_all(s, "{c}", s_c);
        s = replace_all(s, "{first}", s_first);
        s = replace_all(s, "{second}", s_second);
        s = replace_all(s, "{1}", s_first);
        s = replace_all(s, "{2}", s_second);
        inputs.push_back(s);
    }
    return inputs;
}

std::vector<std::string> Profile::format_launch_args(const std::vector<std::string>& cmd, int grid_size) const {
    std::vector<std::string> result;
    std::string n_str = std::to_string(grid_size);
    for (const auto& arg : cmd) {
        result.push_back(replace_all(arg, "{N}", n_str));
    }
    return result;
}

} // namespace mem_tac_review
