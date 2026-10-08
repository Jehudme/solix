#include "ide.hpp"
#include "../cli_utils.hpp"
#include "solix/path_utils.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <optional>
#include <cstdlib>

namespace solix::cli {

static std::optional<std::filesystem::path> find_bundled_vsix() {
    std::filesystem::path exe_dir = solix::get_executable_dir();

    std::vector<std::filesystem::path> candidates = {
        exe_dir.parent_path() / "share" / "solix" / "vscode" / "solix-0.1.0.vsix",
        exe_dir / ".." / "share" / "solix" / "vscode" / "solix-0.1.0.vsix",
        exe_dir / "share" / "solix" / "vscode" / "solix-0.1.0.vsix",
        exe_dir.parent_path().parent_path() / "editors" / "vscode" / "solix-0.1.0.vsix",
        exe_dir.parent_path() / "editors" / "vscode" / "solix-0.1.0.vsix"
    };

    for (const auto& cand : candidates) {
        std::filesystem::path normal_cand = cand.lexically_normal();
        if (std::filesystem::exists(normal_cand)) {
            return normal_cand;
        }
    }
    return std::nullopt;
}

static std::vector<std::string> get_supported_ides() {
    return {"code", "code-insiders", "cursor", "codium"};
}

static bool is_command_available(const std::string& cmd) {
#if defined(_WIN32)
    std::string check_cmd = "where " + cmd + " >nul 2>&1";
#else
    std::string check_cmd = "which " + cmd + " >/dev/null 2>&1";
#endif
    return (std::system(check_cmd.c_str()) == 0);
}

void setup_ide_command(CLI::App& app) {
    auto* ide_cmd = app.add_subcommand("ide", "IDE integration and editor tooling");
    auto* install_subcmd = ide_cmd->add_subcommand("install", "Install Solix extension into IDEs");

    auto editor_opt = std::make_shared<std::string>("");
    install_subcmd->add_option("--editor,-e", *editor_opt, "Specific editor binary to target (code, code-insiders, cursor, codium)");

    install_subcmd->callback([editor_opt]() {
        auto vsix_path = find_bundled_vsix();
        if (!vsix_path) {
            std::cerr << "Error: Bundled Solix VS Code extension (.vsix) not found." << std::endl;
            cli_exit(1);
        }

        std::vector<std::string> target_ides;
        if (!editor_opt->empty()) {
            target_ides.push_back(*editor_opt);
        } else {
            target_ides = get_supported_ides();
        }

        int installed_count = 0;
        for (const auto& ide : target_ides) {
            if (is_command_available(ide)) {
                std::cout << "Installing Solix extension into " << ide << "..." << std::endl;
                std::string cmd = ide + " --install-extension \"" + vsix_path->string() + "\"";
                int ret = std::system(cmd.c_str());
                if (ret == 0) {
                    std::cout << "  ✓ Extension successfully installed for " << ide << "." << std::endl;
                    installed_count++;
                } else {
                    std::cerr << "  ✗ Warning: Failed to install extension into " << ide << " (exit code " << ret << ")." << std::endl;
                }
            } else if (!editor_opt->empty()) {
                std::cerr << "Error: Specified editor binary '" << ide << "' was not found in PATH." << std::endl;
                cli_exit(1);
            }
        }

        if (installed_count == 0 && editor_opt->empty()) {
            std::cout << "No compatible IDE binary (code, code-insiders, cursor, codium) detected in PATH." << std::endl;
            std::cout << "You can manually install the extension from: " << vsix_path->string() << std::endl;
        }
    });
}

} // namespace solix::cli
