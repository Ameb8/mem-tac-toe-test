#pragma once

#include "mem_tac_review/scenario.hpp"

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <filesystem>

namespace mem_tac_review {

class ScenarioLibrary {
public:
    ScenarioLibrary();

    void register_scenario(Scenario scenario);

    const Scenario* get(const std::string& id) const;
    std::vector<const Scenario*> get_by_suite(const std::string& suite_name) const;
    std::vector<const Scenario*> list_all() const;
    std::vector<std::string> list_suites() const;

    // Load custom scenarios from YAML or JSON file or directory
    bool load_file(const std::filesystem::path& path, std::string* error_msg = nullptr);
    size_t load_directory(const std::filesystem::path& dir, std::string* error_msg = nullptr);

    // Save scenario to YAML
    bool save_file(const Scenario& scenario, const std::filesystem::path& path, std::string* error_msg = nullptr) const;

    static Scenario from_yaml_string(const std::string& yaml_str) { return Scenario::from_yaml_string(yaml_str); }
    static std::string to_yaml_string(const Scenario& scenario) { return Scenario::to_yaml_string(scenario); }

private:
    void init_canonical_scenarios();

    std::map<std::string, Scenario> scenarios_;
    std::map<std::string, std::string> aliases_;
};

} // namespace mem_tac_review
