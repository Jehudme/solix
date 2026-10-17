#include <catch2/catch_all.hpp>
#include "cli_test_helper.hpp"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>

using namespace solix::test;

TEST_CASE("CLI Command - Dependency Profile Selection & Variable Propagation", "[command][dependency][profiles]") {
    TempDir sandbox;

    SECTION("Positive - Case 22.1: Explicit Profile Selection on Dependency Ingests Profile Sources") {
        auto orig_cwd = std::filesystem::current_path();

        // 1. Setup lib_a
        auto lib_a_dir = sandbox.path() / "lib_a";
        std::filesystem::create_directories(lib_a_dir / "src");

        nlohmann::json lib_a_manifest;
        lib_a_manifest["project"] = "lib_a";
        lib_a_manifest["version"] = "1.0.0";
        lib_a_manifest["dependencies"] = nlohmann::json::array();
        lib_a_manifest["profiles"] = {
            {"debug", {
                {"output_directory", "build/debug"},
                {"compilation", {
                    {"entry_point", "dummy"},
                    {"additional_dependencies", nlohmann::json::array({
                        {{"type", "source"}, {"path", "src/debug_helper.slx"}}
                    })}
                }}
            }},
            {"fast", {
                {"output_directory", "build/fast"},
                {"compilation", {
                    {"entry_point", "dummy"},
                    {"additional_dependencies", nlohmann::json::array({
                        {{"type", "source"}, {"path", "src/fast_helper.slx"}}
                    })}
                }}
            }}
        };

        sandbox.create_file("lib_a/solix.json", lib_a_manifest.dump(2));
        sandbox.create_file("lib_a/src/debug_helper.slx", "class Helper { public static int32 get_val() { return 11; } }\n");
        sandbox.create_file("lib_a/src/fast_helper.slx", "class Helper { public static int32 get_val() { return 42; } }\n");

        // 2. Setup app selecting profile "fast"
        auto app_dir = sandbox.path() / "app";
        std::filesystem::create_directories(app_dir / "src");

        nlohmann::json app_manifest;
        app_manifest["project"] = "app";
        app_manifest["version"] = "1.0.0";
        app_manifest["dependencies"] = nlohmann::json::array({
            {
                {"type", "project"},
                {"path", "../lib_a"},
                {"profile", "fast"}
            },
            {
                {"type", "source"},
                {"path", "src/main.slx"}
            }
        });
        app_manifest["profiles"] = {
            {"debug", {
                {"output_directory", "build/debug"},
                {"exe_filename", "out.slxbin"},
                {"compilation", {
                    {"entry_point", "main"}
                }}
            }}
        };

        sandbox.create_file("app/solix.json", app_manifest.dump(2));
        sandbox.create_file("app/src/main.slx", "class Main { public static int32 main() { return Helper.get_val(); } }\n");

        std::filesystem::current_path(app_dir);
        auto build_res = run_cli({"build"});
        CHECK(build_res.exit_code == 0);
        CHECK(std::filesystem::exists(app_dir / "build/debug/out.slxbin"));

        auto run_res = run_cli({"run", (app_dir / "build/debug/out.slxbin").string()});
        std::filesystem::current_path(orig_cwd);

        CHECK(run_res.exit_code == 42);
    }

    SECTION("Positive - Case 22.2: Profile Inheritance When Dependency Does Not Specify Profile") {
        auto orig_cwd = std::filesystem::current_path();

        // 1. Setup lib_a
        auto lib_a_dir = sandbox.path() / "lib_a";
        std::filesystem::create_directories(lib_a_dir / "src");

        nlohmann::json lib_a_manifest;
        lib_a_manifest["project"] = "lib_a";
        lib_a_manifest["version"] = "1.0.0";
        lib_a_manifest["dependencies"] = nlohmann::json::array();
        lib_a_manifest["profiles"] = {
            {"debug", {
                {"output_directory", "build/debug"},
                {"compilation", {
                    {"entry_point", "dummy"},
                    {"additional_dependencies", nlohmann::json::array({
                        {{"type", "source"}, {"path", "src/debug_helper.slx"}}
                    })}
                }}
            }},
            {"release", {
                {"output_directory", "build/release"},
                {"compilation", {
                    {"entry_point", "dummy"},
                    {"additional_dependencies", nlohmann::json::array({
                        {{"type", "source"}, {"path", "src/release_helper.slx"}}
                    })}
                }}
            }}
        };

        sandbox.create_file("lib_a/solix.json", lib_a_manifest.dump(2));
        sandbox.create_file("lib_a/src/debug_helper.slx", "class HelperRel { public static int32 get_rel_val() { return 11; } }\n");
        sandbox.create_file("lib_a/src/release_helper.slx", "class HelperRel { public static int32 get_rel_val() { return 100; } }\n");

        // 2. Setup app building with profile "release" without specifying profile on lib_a
        auto app_dir = sandbox.path() / "app";
        std::filesystem::create_directories(app_dir / "src");

        nlohmann::json app_manifest;
        app_manifest["project"] = "app";
        app_manifest["version"] = "1.0.0";
        app_manifest["dependencies"] = nlohmann::json::array({
            {
                {"type", "project"},
                {"path", "../lib_a"}
            },
            {
                {"type", "source"},
                {"path", "src/main.slx"}
            }
        });
        app_manifest["profiles"] = {
            {"release", {
                {"output_directory", "build/release"},
                {"exe_filename", "out.slxbin"},
                {"compilation", {
                    {"entry_point", "main"}
                }}
            }}
        };

        sandbox.create_file("app/solix.json", app_manifest.dump(2));
        sandbox.create_file("app/src/main.slx", "class Main { public static int32 main() { return HelperRel.get_rel_val(); } }\n");

        std::filesystem::current_path(app_dir);
        auto build_res = run_cli({"build", "-p", "release"});
        CHECK(build_res.exit_code == 0);
        CHECK(std::filesystem::exists(app_dir / "build/release/out.slxbin"));

        auto run_res = run_cli({"run", (app_dir / "build/release/out.slxbin").string()});
        std::filesystem::current_path(orig_cwd);

        CHECK(run_res.exit_code == 100);
    }

    SECTION("Positive - Case 22.3: Profile Fallback When Inherited Profile Does Not Exist in Dependency") {
        auto orig_cwd = std::filesystem::current_path();

        // 1. Setup lib_a with only "debug" profile
        auto lib_a_dir = sandbox.path() / "lib_a";
        std::filesystem::create_directories(lib_a_dir / "src");

        nlohmann::json lib_a_manifest;
        lib_a_manifest["project"] = "lib_a";
        lib_a_manifest["version"] = "1.0.0";
        lib_a_manifest["dependencies"] = nlohmann::json::array();
        lib_a_manifest["profiles"] = {
            {"debug", {
                {"output_directory", "build/debug"},
                {"compilation", {
                    {"entry_point", "dummy"},
                    {"additional_dependencies", nlohmann::json::array({
                        {{"type", "source"}, {"path", "src/debug_helper.slx"}}
                    })}
                }}
            }}
        };

        sandbox.create_file("lib_a/solix.json", lib_a_manifest.dump(2));
        sandbox.create_file("lib_a/src/debug_helper.slx", "class HelperFallback { public static int32 get_fallback_val() { return 77; } }\n");

        // 2. Setup app building with profile "custom_prod"
        auto app_dir = sandbox.path() / "app";
        std::filesystem::create_directories(app_dir / "src");

        nlohmann::json app_manifest;
        app_manifest["project"] = "app";
        app_manifest["version"] = "1.0.0";
        app_manifest["dependencies"] = nlohmann::json::array({
            {
                {"type", "project"},
                {"path", "../lib_a"}
            },
            {
                {"type", "source"},
                {"path", "src/main.slx"}
            }
        });
        app_manifest["profiles"] = {
            {"custom_prod", {
                {"output_directory", "build/custom_prod"},
                {"exe_filename", "out.slxbin"},
                {"compilation", {
                    {"entry_point", "main"}
                }}
            }}
        };

        sandbox.create_file("app/solix.json", app_manifest.dump(2));
        sandbox.create_file("app/src/main.slx", "class Main { public static int32 main() { return HelperFallback.get_fallback_val(); } }\n");

        std::filesystem::current_path(app_dir);
        auto build_res = run_cli({"build", "-p", "custom_prod"});
        CHECK(build_res.exit_code == 0);
        CHECK(std::filesystem::exists(app_dir / "build/custom_prod/out.slxbin"));

        auto run_res = run_cli({"run", (app_dir / "build/custom_prod/out.slxbin").string()});
        std::filesystem::current_path(orig_cwd);

        CHECK(run_res.exit_code == 77);
    }

    SECTION("Positive - Case 22.4: Profile-Specific Transitive Project Dependencies Discovered and Compiled") {
        auto orig_cwd = std::filesystem::current_path();

        // 1. Setup lib_b (transitive leaf)
        auto lib_b_dir = sandbox.path() / "lib_b";
        std::filesystem::create_directories(lib_b_dir / "src");

        nlohmann::json lib_b_manifest;
        lib_b_manifest["project"] = "lib_b";
        lib_b_manifest["version"] = "1.0.0";
        lib_b_manifest["dependencies"] = nlohmann::json::array({
            {{"type", "source"}, {"path", "src/b_helper.slx"}}
        });
        lib_b_manifest["profiles"] = {
            {"debug", {
                {"output_directory", "build/debug"},
                {"compilation", {{"entry_point", "dummy"}}}
            }}
        };

        sandbox.create_file("lib_b/solix.json", lib_b_manifest.dump(2));
        sandbox.create_file("lib_b/src/b_helper.slx", "class HelperB { public static int32 compute_b() { return 88; } }\n");

        // 2. Setup lib_a with "minimal" and "full" profiles; "full" depends on lib_b
        auto lib_a_dir = sandbox.path() / "lib_a";
        std::filesystem::create_directories(lib_a_dir / "src");

        nlohmann::json lib_a_manifest;
        lib_a_manifest["project"] = "lib_a";
        lib_a_manifest["version"] = "1.0.0";
        lib_a_manifest["dependencies"] = nlohmann::json::array();
        lib_a_manifest["profiles"] = {
            {"minimal", {
                {"output_directory", "build/minimal"},
                {"compilation", {
                    {"entry_point", "dummy"},
                    {"additional_dependencies", nlohmann::json::array()}
                }}
            }},
            {"full", {
                {"output_directory", "build/full"},
                {"compilation", {
                    {"entry_point", "dummy"},
                    {"additional_dependencies", nlohmann::json::array({
                        {
                            {"type", "project"},
                            {"path", "../lib_b"},
                            {"profile", "debug"}
                        }
                    })}
                }}
            }}
        };

        sandbox.create_file("lib_a/solix.json", lib_a_manifest.dump(2));

        // 3. Setup app declaring lib_a with profile "full"
        auto app_dir = sandbox.path() / "app";
        std::filesystem::create_directories(app_dir / "src");

        nlohmann::json app_manifest;
        app_manifest["project"] = "app";
        app_manifest["version"] = "1.0.0";
        app_manifest["dependencies"] = nlohmann::json::array({
            {
                {"type", "project"},
                {"path", "../lib_a"},
                {"profile", "full"}
            },
            {
                {"type", "source"},
                {"path", "src/main.slx"}
            }
        });
        app_manifest["profiles"] = {
            {"debug", {
                {"output_directory", "build/debug"},
                {"exe_filename", "out.slxbin"},
                {"compilation", {
                    {"entry_point", "main"}
                }}
            }}
        };

        sandbox.create_file("app/solix.json", app_manifest.dump(2));
        sandbox.create_file("app/src/main.slx", "class Main { public static int32 main() { return HelperB.compute_b(); } }\n");

        std::filesystem::current_path(app_dir);
        auto build_res = run_cli({"build"});
        CHECK(build_res.exit_code == 0);
        CHECK(std::filesystem::exists(app_dir / "build/debug/out.slxbin"));

        auto run_res = run_cli({"run", (app_dir / "build/debug/out.slxbin").string()});
        std::filesystem::current_path(orig_cwd);

        CHECK(run_res.exit_code == 88);
    }

    SECTION("Negative - Case 22.5: Error When Requested Profile Does Not Exist in Dependency Manifest") {
        auto orig_cwd = std::filesystem::current_path();

        // 1. Setup lib_a with only "debug" and "release"
        auto lib_a_dir = sandbox.path() / "lib_a";
        std::filesystem::create_directories(lib_a_dir / "src");

        nlohmann::json lib_a_manifest;
        lib_a_manifest["project"] = "lib_a";
        lib_a_manifest["version"] = "1.0.0";
        lib_a_manifest["dependencies"] = nlohmann::json::array();
        lib_a_manifest["profiles"] = {
            {"debug", {
                {"output_directory", "build/debug"},
                {"compilation", {{"entry_point", "dummy"}}}
            }},
            {"release", {
                {"output_directory", "build/release"},
                {"compilation", {{"entry_point", "dummy"}}}
            }}
        };

        sandbox.create_file("lib_a/solix.json", lib_a_manifest.dump(2));

        // 2. Setup app declaring lib_a with invalid profile "non_existent"
        auto app_dir = sandbox.path() / "app";
        std::filesystem::create_directories(app_dir / "src");

        nlohmann::json app_manifest;
        app_manifest["project"] = "app";
        app_manifest["version"] = "1.0.0";
        app_manifest["dependencies"] = nlohmann::json::array({
            {
                {"type", "project"},
                {"path", "../lib_a"},
                {"profile", "non_existent"}
            },
            {
                {"type", "source"},
                {"path", "src/main.slx"}
            }
        });
        app_manifest["profiles"] = {
            {"debug", {
                {"output_directory", "build/debug"},
                {"exe_filename", "out.slxbin"},
                {"compilation", {
                    {"entry_point", "main"}
                }}
            }}
        };

        sandbox.create_file("app/solix.json", app_manifest.dump(2));
        sandbox.create_file("app/src/main.slx", "class Main { public static int32 main() { return 0; } }\n");

        std::filesystem::current_path(app_dir);
        auto build_res = run_cli({"build"});
        std::filesystem::current_path(orig_cwd);

        CHECK(build_res.exit_code != 0);
        CHECK_THAT(build_res.err, Catch::Matchers::ContainsSubstring("does not define requested profile 'non_existent'"));
        CHECK_THAT(build_res.err, Catch::Matchers::ContainsSubstring("debug"));
        CHECK_THAT(build_res.err, Catch::Matchers::ContainsSubstring("release"));
    }

    SECTION("Positive - Case 22.6: Dependency Pre-Compile Command Runs in Dependency Working Directory Prior to Consumer Build") {
        auto orig_cwd = std::filesystem::current_path();

        // 1. Setup lib_a with pre_compile generating a source file inside lib_a/src/
        auto lib_a_dir = sandbox.path() / "lib_a";
        std::filesystem::create_directories(lib_a_dir / "src");

        nlohmann::json lib_a_manifest;
        lib_a_manifest["project"] = "lib_a";
        lib_a_manifest["version"] = "1.0.0";
        lib_a_manifest["dependencies"] = nlohmann::json::array();
        lib_a_manifest["profiles"] = {
            {"release", {
                {"output_directory", "build/release"},
                {"pre_compile", nlohmann::json::array({
#if defined(_WIN32)
                    "powershell -Command \"Set-Content -Path 'src/generated.slx' -Value 'class GenHelper { public static int32 gen_code() { return 55; } }'\""
#else
                    "sh -c \"echo 'class GenHelper { public static int32 gen_code() { return 55; } }' > src/generated.slx\""
#endif
                })},
                {"compilation", {
                    {"entry_point", "dummy"},
                    {"additional_dependencies", nlohmann::json::array({
                        {{"type", "source"}, {"path", "src/generated.slx"}}
                    })}
                }}
            }}
        };

        sandbox.create_file("lib_a/solix.json", lib_a_manifest.dump(2));

        // 2. Setup app depending on lib_a with profile "release"
        auto app_dir = sandbox.path() / "app";
        std::filesystem::create_directories(app_dir / "src");

        nlohmann::json app_manifest;
        app_manifest["project"] = "app";
        app_manifest["version"] = "1.0.0";
        app_manifest["dependencies"] = nlohmann::json::array({
            {
                {"type", "project"},
                {"path", "../lib_a"},
                {"profile", "release"}
            },
            {
                {"type", "source"},
                {"path", "src/main.slx"}
            }
        });
        app_manifest["profiles"] = {
            {"debug", {
                {"output_directory", "build/debug"},
                {"exe_filename", "out.slxbin"},
                {"compilation", {
                    {"entry_point", "main"}
                }}
            }}
        };

        sandbox.create_file("app/solix.json", app_manifest.dump(2));
        sandbox.create_file("app/src/main.slx", "class Main { public static int32 main() { return GenHelper.gen_code(); } }\n");

        std::filesystem::current_path(app_dir);
        auto build_res = run_cli({"build"});
        CHECK(build_res.exit_code == 0);
        CHECK(std::filesystem::exists(lib_a_dir / "src/generated.slx"));
        CHECK(std::filesystem::exists(app_dir / "build/debug/out.slxbin"));

        auto run_res = run_cli({"run", (app_dir / "build/debug/out.slxbin").string()});
        std::filesystem::current_path(orig_cwd);

        CHECK(run_res.exit_code == 55);
    }
}
