#pragma once
#include <CLI/CLI.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <chrono>
#include <random>
#include <cstdlib>
#include <optional>

#include "commands/compile.hpp"
#include "commands/execute.hpp"
#include "commands/build.hpp"
#include "commands/new.hpp"
#include "commands/package.hpp"
#include "commands/version.hpp"
#include "commands/ide.hpp"

namespace solix::test {

struct CliResult {
    int exit_code{0};
    std::string out;
    std::string err;
};

class TempDir {
public:
    TempDir() {
        auto now = std::chrono::steady_clock::now().time_since_epoch().count();
        static std::random_device rd;
        static std::mt19937_64 gen(rd());
        std::uniform_int_distribution<uint64_t> dis;
        dir_ = std::filesystem::temp_directory_path() / ("solix_test_" + std::to_string(now) + "_" + std::to_string(dis(gen)));
        std::filesystem::create_directories(dir_);
    }

    ~TempDir() {
        std::error_code ec;
        std::filesystem::remove_all(dir_, ec);
    }

    TempDir(const TempDir&) = delete;
    TempDir& operator=(const TempDir&) = delete;
    TempDir(TempDir&&) = default;
    TempDir& operator=(TempDir&&) = default;

    const std::filesystem::path& path() const { return dir_; }

    std::filesystem::path create_file(const std::string& rel_path, const std::string& content) {
        auto p = dir_ / rel_path;
        if (p.has_parent_path()) {
            std::filesystem::create_directories(p.parent_path());
        }
        std::ofstream ofs(p);
        ofs << content;
        ofs.close();
        return p;
    }

    std::string read_file(const std::string& rel_path) const {
        auto p = dir_ / rel_path;
        std::ifstream ifs(p);
        std::stringstream ss;
        ss << ifs.rdbuf();
        return ss.str();
    }

    bool exists(const std::string& rel_path) const {
        return std::filesystem::exists(dir_ / rel_path);
    }

private:
    std::filesystem::path dir_;
};

class EnvGuard {
public:
    EnvGuard(const std::string& key, const std::string& value) : key_(key) {
        const char* existing = std::getenv(key.c_str());
        if (existing) {
            had_existing_ = true;
            old_val_ = existing;
        }
#if defined(_WIN32)
        _putenv_s(key.c_str(), value.c_str());
#else
        setenv(key.c_str(), value.c_str(), 1);
#endif
    }

    ~EnvGuard() {
        if (had_existing_) {
#if defined(_WIN32)
            _putenv_s(key_.c_str(), old_val_.c_str());
#else
            setenv(key_.c_str(), old_val_.c_str(), 1);
#endif
        } else {
#if defined(_WIN32)
            _putenv_s(key_.c_str(), "");
#else
            unsetenv(key_.c_str());
#endif
        }
    }

private:
    std::string key_;
    std::string old_val_;
    bool had_existing_{false};
};

inline CliResult run_cli(const std::vector<std::string>& args, const std::filesystem::path& solix_home = "") {
    std::optional<EnvGuard> home_guard;
    if (!solix_home.empty()) {
        home_guard.emplace("SOLIX_HOME", solix_home.string());
    }

    CLI::App app{"Solix Programming Language Compiler and VM"};
    app.require_subcommand(1);

    solix::cli::setup_compile_command(app);
    solix::cli::setup_execute_command(app);
    solix::cli::setup_build_command(app);
    solix::cli::setup_new_command(app);
    solix::cli::setup_package_commands(app);
    solix::cli::setup_version_command(app);
    solix::cli::setup_ide_command(app);

    std::stringstream out_ss;
    std::stringstream err_ss;
    std::streambuf* old_cout = std::cout.rdbuf(out_ss.rdbuf());
    std::streambuf* old_cerr = std::cerr.rdbuf(err_ss.rdbuf());

    CliResult result;
    try {
        std::vector<std::string> parse_args = args;
        std::reverse(parse_args.begin(), parse_args.end());
        app.parse(parse_args);
        result.exit_code = 0;
    } catch (const CLI::RuntimeError& e) {
        result.exit_code = e.get_exit_code();
    } catch (const CLI::ParseError& e) {
        result.exit_code = app.exit(e, out_ss, err_ss);
    } catch (const std::exception& e) {
        result.exit_code = 1;
        err_ss << "Exception: " << e.what() << "\n";
    } catch (...) {
        result.exit_code = 1;
        err_ss << "Unknown exception occurred\n";
    }

    std::cout.rdbuf(old_cout);
    std::cerr.rdbuf(old_cerr);

    result.out = out_ss.str();
    result.err = err_ss.str();
    return result;
}

} // namespace solix::test
