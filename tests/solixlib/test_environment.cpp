#include "solixlib_test_helper.hpp"

using namespace solix::test;

TEST_CASE("Standard Library - solix.system.Environment", "[system][environment][process][solixlib]") {
    SECTION("Case 14.1: Setting and retrieving environment variables") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.system.Environment;
            import solix.core.String;
            import solix.collections.HashMap;

            class Main {
                public static int32 main() {
                    String key = new String("SOLIX_TEST_VAR_123");
                    String val = new String("magic_value_456");

                    Environment.set_env(key, val);
                    String read_val = Environment.get_env(key);
                    if (read_val == null) return 1;
                    if (!read_val.equals(val)) return 2;

                    HashMap<String, String> all_env = Environment.get_all_env();
                    if (all_env == null) return 3;
                    if (!all_env.contains_key(key)) return 4;
                    if (!all_env.get(key).equals(val)) return 5;

                    String missing = Environment.get_env(new String("NON_EXISTENT_VAR_XYZ_987"));
                    if (missing != null) return 6;

                    return 0;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 14.2: Operating system identification and host hardware inspection") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.system.Environment;
            import solix.core.String;

            class Main {
                public static int32 main() {
                    String os = Environment.os_name();
                    if (os == null || os.length() == 0) return 1;

                    String ver = Environment.os_version();
                    if (ver == null || ver.length() == 0) return 2;

                    int32 cpus = Environment.processor_count();
                    if (cpus <= 0) return 3;

                    bool is_win = Environment.is_windows();
                    bool is_lin = Environment.is_linux();
                    bool is_mac = Environment.is_macos();

                    // Exactly one of the main OS flags should normally be true on supported platforms
                    int32 flag_sum = 0;
                    if (is_win) flag_sum = flag_sum + 1;
                    if (is_lin) flag_sum = flag_sum + 1;
                    if (is_mac) flag_sum = flag_sum + 1;
                    if (flag_sum != 1) return 4;

                    if (!is_lin) return 5;

                    return 0;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 14.3: Synchronous child process execution via Process.run() with standard output capture") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.system.Process;
            import solix.system.ProcessResult;
            import solix.collections.List;
            import solix.core.String;

            class Main {
                public static int32 main() {
                    List<String> args = new List<String>();
                    args.add(new String("hello_solix_process"));

                    ProcessResult res = Process.run(new String("echo"), args);
                    if (res == null) return 1;
                    if (res.exit_code != 0) return 2;
                    if (res.standard_output == null || res.standard_output.length() == 0) return 3;
                    if (res.standard_output.index_of(new String("hello_solix_process")) < 0) return 4;

                    return 0;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 14.4: Lifecycle child process management with Process") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.system.Process;
            import solix.collections.List;
            import solix.core.String;

            class Main {
                public static int32 main() {
                    List<String> args = new List<String>();
                    args.add(new String("lifecycle_test"));

                    Process p = new Process(new String("echo"), args);
                    if (p.has_exited()) return 1;

                    p.start();
                    int32 code = p.wait_for_exit();
                    if (code != 0) return 2;
                    if (!p.has_exited()) return 3;
                    if (p.exit_code() != 0) return 4;

                    String out = p.get_standard_output();
                    if (out == null || out.index_of(new String("lifecycle_test")) < 0) return 5;

                    return 0;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 14.5: Spawning non-existent command returns non-zero exit code") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.system.Process;
            import solix.system.ProcessResult;
            import solix.collections.List;
            import solix.core.String;

            class Main {
                public static int32 main() {
                    ProcessResult res = Process.run(new String("solix_non_existent_binary_xyz_12345"), new List<String>());
                    if (res == null) return 1;
                    if (res.exit_code == 0) return 2; // Must fail with non-zero exit code

                    return 0;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }
}
