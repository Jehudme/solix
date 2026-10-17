#pragma once

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

#ifndef _WIN32
#include <sys/wait.h>
#endif

namespace solix::cli {

class ScopedCwd {
public:
    explicit ScopedCwd(const std::filesystem::path& target_dir) {
        std::error_code ec;
        prev_cwd_ = std::filesystem::current_path(ec);
        if (!target_dir.empty() && std::filesystem::exists(target_dir, ec)) {
            std::filesystem::current_path(target_dir, ec);
        }
    }

    ~ScopedCwd() {
        std::error_code ec;
        if (!prev_cwd_.empty()) {
            std::filesystem::current_path(prev_cwd_, ec);
        }
    }

private:
    std::filesystem::path prev_cwd_;
};

class CommandRunner {
public:
    static bool run_single_command(const std::string& cmd_str,
                                  const std::filesystem::path& working_dir,
                                  const std::string& phase_prefix = "") {
        std::error_code ec;
        if (!working_dir.empty() && !std::filesystem::exists(working_dir, ec)) {
            std::cerr << "Error: Target directory does not exist for command: "
                      << working_dir.string() << std::endl;
            return false;
        }

        std::cout << (phase_prefix.empty() ? "" : "[" + phase_prefix + "] ")
                  << "$ " << cmd_str << std::endl;

        ScopedCwd cwd_guard(working_dir);

        int raw_status = std::system(cmd_str.c_str());

        int exit_code = 0;
#ifdef _WIN32
        exit_code = raw_status;
#else
        if (WIFEXITED(raw_status)) {
            exit_code = WEXITSTATUS(raw_status);
        } else if (WIFSIGNALED(raw_status)) {
            exit_code = 128 + WTERMSIG(raw_status);
        } else {
            exit_code = raw_status;
        }
#endif

        if (exit_code != 0) {
            std::cerr << "Error: " << (phase_prefix.empty() ? "Command" : phase_prefix + " command")
                      << " failed with exit code " << exit_code << ": " << cmd_str << std::endl;
            return false;
        }
        return true;
    }

    static std::vector<std::string> extract_commands(const nlohmann::json& node,
                                                    const std::string& key,
                                                    const std::string& alias_key = "") {
        std::vector<std::string> commands;
        const nlohmann::json* target = nullptr;
        if (node.contains(key)) {
            target = &node[key];
        } else if (!alias_key.empty() && node.contains(alias_key)) {
            target = &node[alias_key];
        }

        if (!target) return commands;

        if (target->is_array()) {
            for (const auto& item : *target) {
                if (item.is_string()) {
                    commands.push_back(item.get<std::string>());
                }
            }
        } else if (target->is_string()) {
            commands.push_back(target->get<std::string>());
        }
        return commands;
    }

    static bool run_commands(const std::vector<std::string>& commands,
                            const std::filesystem::path& working_dir,
                            const std::string& phase_prefix) {
        for (const auto& cmd : commands) {
            if (cmd.empty()) continue;
            if (!run_single_command(cmd, working_dir, phase_prefix)) {
                return false;
            }
        }
        return true;
    }
};

} // namespace solix::cli
