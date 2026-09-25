#include "solix/compilation.hpp"
#include "solix/runtime.hpp"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <filesystem>

using namespace solix;

static void load_stdlib(CompilationOptions &opts) {
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

static int32_t run_program(CompilationOptions &comp_opts) {
    Bytecode bytecode = solix::run(comp_opts);
    RuntimeOptions run_opts;
    run_opts.bytecode_source = bytecode;
    for (const auto &[name, func] : solix::get_builtin_natives()) {
        run_opts.native_functions[name] = func;
    }
    return solix::run(run_opts);
}

TEST_CASE("Phase 7 - Symbol Path Resolution and Disambiguation", "[phase7][symbol_resolution]") {

    SECTION("1. Full symbol path resolution for types and static methods") {
        CompilationOptions comp_opts;
        comp_opts.log_level = CompilationOptions::LogLevel::OFF;
        load_stdlib(comp_opts);

        Source src_key = std::string("test_full_path.slx");
        comp_opts.sources[src_key] = R"(
            package test.sym;

            public class FullPathTest {
                public static int32 main(char[][] args) {
                    solix.core.String s = new solix.core.String("hello");
                    bool b = solix.core.Objects.is_null(s);
                    if (s.size() == 5 && !b) {
                        return 0;
                    }
                    return 1;
                }
            }
        )";

        REQUIRE(run_program(comp_opts) == 0);
    }

    SECTION("2. Partial symbol path (sub-namespace suffix) resolution") {
        CompilationOptions comp_opts;
        comp_opts.log_level = CompilationOptions::LogLevel::OFF;
        load_stdlib(comp_opts);

        Source src_key = std::string("test_partial_path.slx");
        comp_opts.sources[src_key] = R"(
            package test.sym;

            public class PartialPathTest {
                public static int32 main(char[][] args) {
                    core.String s = new core.String("world");
                    bool b = core.Objects.is_null(s);
                    if (s.size() == 5 && !b) {
                        return 0;
                    }
                    return 1;
                }
            }
        )";

        REQUIRE(run_program(comp_opts) == 0);
    }

    SECTION("3. C++ scope resolution operator :: resolution") {
        CompilationOptions comp_opts;
        comp_opts.log_level = CompilationOptions::LogLevel::OFF;
        load_stdlib(comp_opts);

        Source src_key = std::string("test_scope_res.slx");
        comp_opts.sources[src_key] = R"(
            package test.sym;

            public class ScopeResTest {
                public static int32 main(char[][] args) {
                    solix::core::String s1 = new solix::core::String("scope1");
                    core::String s2 = new core::String("scope2");
                    bool b1 = solix::core::Objects::is_null(s1);
                    bool b2 = core::Objects::is_null(s2);
                    if (s1.size() == 6 && s2.size() == 6 && !b1 && !b2) {
                        return 0;
                    }
                    return 1;
                }
            }
        )";

        REQUIRE(run_program(comp_opts) == 0);
    }

    SECTION("4. Alias with full and partial paths") {
        CompilationOptions comp_opts;
        comp_opts.log_level = CompilationOptions::LogLevel::OFF;
        load_stdlib(comp_opts);

        Source src_key = std::string("test_alias_path.slx");
        comp_opts.sources[src_key] = R"(
            package test.sym;

            alias PartialStr = core.String;
            alias FullObjs = solix.core.Objects;

            public class AliasPathTest {
                public static int32 main(char[][] args) {
                    PartialStr s = new PartialStr("alias_val");
                    bool b = FullObjs.is_null(s);
                    if (s.size() == 9 && !b) {
                        return 0;
                    }
                    return 1;
                }
            }
        )";

        REQUIRE(run_program(comp_opts) == 0);
    }

    SECTION("5. Import statement with full and partial paths") {
        CompilationOptions comp_opts;
        comp_opts.log_level = CompilationOptions::LogLevel::OFF;
        load_stdlib(comp_opts);

        Source src_key = std::string("test_import_path.slx");
        comp_opts.sources[src_key] = R"(
            package test.sym;

            import core.String;
            import core.Objects;

            public class ImportPathTest {
                public static int32 main(char[][] args) {
                    String s = new String("import_val");
                    bool b = Objects.is_null(s);
                    if (s.size() == 10 && !b) {
                        return 0;
                    }
                    return 1;
                }
            }
        )";

        REQUIRE(run_program(comp_opts) == 0);
    }

    SECTION("6. Multi-package collision disambiguation via full, partial, and import") {
        CompilationOptions comp_opts;
        comp_opts.log_level = CompilationOptions::LogLevel::OFF;

        comp_opts.sources[std::string("pkg_a.slx")] = R"(
            package com.vendor.audio;

            public class Track {
                public static int32 id() { return 100; }
            }
        )";

        comp_opts.sources[std::string("pkg_b.slx")] = R"(
            package com.vendor.video;

            public class Track {
                public static int32 id() { return 200; }
            }
        )";

        comp_opts.sources[std::string("main.slx")] = R"(
            package com.vendor.app;

            import video.Track;

            public class AppMain {
                public static int32 main(char[][] args) {
                    // 1. Explicit import selects video.Track
                    int32 v1 = Track.id(); // 200

                    // 2. Partial qualification differentiates audio
                    int32 v2 = audio.Track.id(); // 100

                    // 3. Full qualification
                    int32 v3 = com.vendor.audio.Track.id(); // 100

                    // 4. C++ scope syntax
                    int32 v4 = audio::Track::id(); // 100

                    if (v1 == 200 && v2 == 100 && v3 == 100 && v4 == 100) {
                        return 0;
                    }
                    return 1;
                }
            }
        )";

        REQUIRE(run_program(comp_opts) == 0);
    }

    SECTION("7. Unresolved collision causes clean ambiguous symbol error") {
        CompilationOptions comp_opts;
        comp_opts.log_level = CompilationOptions::LogLevel::OFF;

        comp_opts.sources[std::string("pkg_a.slx")] = R"(
            package com.collision.alpha;
            public class NodeItem {
                public static int32 code() { return 1; }
            }
        )";

        comp_opts.sources[std::string("pkg_b.slx")] = R"(
            package com.collision.beta;
            public class NodeItem {
                public static int32 code() { return 2; }
            }
        )";

        comp_opts.sources[std::string("main_err.slx")] = R"(
            package com.collision.user;

            public class ErrMain {
                public static int32 main(char[][] args) {
                    // Unqualified NodeItem is ambiguous
                    return NodeItem.code();
                }
            }
        )";

        REQUIRE_THROWS_WITH(solix::run(comp_opts), Catch::Matchers::ContainsSubstring("Ambiguous symbol 'NodeItem'"));
    }
}
