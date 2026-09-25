#include "solix/compilation.hpp"
#include "solix/runtime.hpp"
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>

using namespace solix;

static void load_stdlib_into_options(CompilationOptions &opts) {
    std::vector<std::filesystem::path> candidates = {
        "launcher/rsc/lib/solix",
        "../launcher/rsc/lib/solix",
        "../../launcher/rsc/lib/solix",
        "build/launcher/lib/solix",
        "launcher/lib/solix"
    };

    std::filesystem::path found_path;
    for (const auto &cand : candidates) {
        if (std::filesystem::exists(cand)) {
            found_path = cand;
            break;
        }
    }

    if (!found_path.empty()) {
        for (const auto &entry : std::filesystem::recursive_directory_iterator(found_path)) {
            if (entry.is_regular_file() && entry.path().extension() == ".slx") {
                if (std::filesystem::file_size(entry.path()) > 0) {
                    opts.sources[std::filesystem::canonical(entry.path())] = std::nullopt;
                }
            }
        }
    }
}

TEST_CASE("Phase 5 - Standard Library Architecture and Packaging", "[phase5][stdlib_architecture]") {
    SECTION("solix.core package types and root exports resolve correctly") {
        CompilationOptions comp_opts;
        comp_opts.log_level = CompilationOptions::LogLevel::OFF;
        load_stdlib_into_options(comp_opts);

        Source src_key = std::string("test_core_pkg.slx");
        comp_opts.sources[src_key] = R"(
            package test.pkg;

            alias CoreString = solix.core.String;
            alias RootString = solix.String;

            public class CorePackageTest {
                public static int32 main(char[][] args) {
                    CoreString s1 = new CoreString("Hello Core");
                    RootString s2 = new RootString("Hello Root");
                    if (s1.size() == 10 && s2.size() == 10) {
                        return 0;
                    }
                    return 1;
                }
            }
        )";

        Bytecode bytecode;
        REQUIRE_NOTHROW(bytecode = solix::run(comp_opts));

        RuntimeOptions run_opts;
        run_opts.bytecode_source = bytecode;
        for (const auto &[name, func] : solix::get_builtin_natives()) {
            run_opts.native_functions[name] = func;
        }
        REQUIRE(solix::run(run_opts) == 0);
    }

    SECTION("solix.core exceptions resolve and catch properly") {
        CompilationOptions comp_opts;
        comp_opts.log_level = CompilationOptions::LogLevel::OFF;
        load_stdlib_into_options(comp_opts);

        Source src_key = std::string("test_core_exceptions.slx");
        comp_opts.sources[src_key] = R"(
            package test.pkg;

            alias NullPointerException = solix.core.NullPointerException;
            alias Exception = solix.Exception;

            public class CoreExceptionTest {
                public static int32 main(char[][] args) {
                    try {
                        throw new NullPointerException("Null reference encountered");
                    } catch (Exception e) {
                        return 0;
                    }
                    return 1;
                }
            }
        )";

        Bytecode bytecode;
        REQUIRE_NOTHROW(bytecode = solix::run(comp_opts));

        RuntimeOptions run_opts;
        run_opts.bytecode_source = bytecode;
        for (const auto &[name, func] : solix::get_builtin_natives()) {
            run_opts.native_functions[name] = func;
        }
        REQUIRE(solix::run(run_opts) == 0);
    }
}
