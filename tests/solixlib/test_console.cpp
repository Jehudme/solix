#include "solixlib_test_helper.hpp"

using namespace solix::test;

TEST_CASE("Standard Library - solix.system.Console", "[solixlib][console]") {
    SECTION("Case 2.1: Console.print and println with primitive types") {
        StreamRedirectGuard guard;
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.system.Console;

            class Main {
                public static int32 main() {
                    Console.print(42);
                    Console.print(' ');
                    Console.print(true);
                    Console.print(' ');
                    Console.println(3.14);
                    return 0;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
        std::string output = guard.cout_stream.str();
        CHECK(output.find("42") != std::string::npos);
        CHECK(output.find("true") != std::string::npos);
        CHECK(output.find("3.14") != std::string::npos);
        CHECK(output.find("\n") != std::string::npos);
    }

    SECTION("Case 2.2: Console.print and println with char[] strings") {
        StreamRedirectGuard guard;
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.system.Console;

            class Main {
                public static int32 main() {
                    Console.print("Hello, ");
                    Console.println("Solix!");
                    return 0;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
        std::string output = guard.cout_stream.str();
        CHECK(output == "Hello, Solix!\n");
    }

    SECTION("Case 2.3: Console.error outputting in ANSI red to stderr") {
        StreamRedirectGuard guard;
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.system.Console;

            class Main {
                public static int32 main() {
                    Console.error("Fatal exception encountered");
                    return 0;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
        std::string err_output = guard.cerr_stream.str();
        CHECK(err_output.find("\033[31m") != std::string::npos);
        CHECK(err_output.find("Fatal exception encountered") != std::string::npos);
        CHECK(err_output.find("\033[0m") != std::string::npos);
    }

    SECTION("Case 2.4: Console.warning, info, and success colorized diagnostics") {
        StreamRedirectGuard guard;
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.system.Console;

            class Main {
                public static int32 main() {
                    Console.warning("Deprecated feature used");
                    Console.info("System initialized");
                    Console.success("All tests passed");
                    return 0;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
        std::string out = guard.cout_stream.str();
        CHECK(out.find("\033[33m") != std::string::npos); // Yellow warning
        CHECK(out.find("Deprecated feature used") != std::string::npos);
        CHECK(out.find("\033[36m") != std::string::npos); // Cyan info
        CHECK(out.find("System initialized") != std::string::npos);
        CHECK(out.find("\033[32m") != std::string::npos); // Green success
        CHECK(out.find("All tests passed") != std::string::npos);
    }

    SECTION("Case 2.5: Console.input_int parsing valid integer") {
        StreamRedirectGuard guard("12345\n");
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.system.Console;

            class Main {
                public static int32 main() {
                    int32 val = Console.input_int();
                    return val == 12345 ? 0 : 1;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 2.6: Console.input_double parsing valid double") {
        StreamRedirectGuard guard("42.5\n");
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.system.Console;

            class Main {
                public static int32 main() {
                    float64 d = Console.input_double();
                    return (d > 42.4 && d < 42.6) ? 0 : 1;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 2.7: Console.input_bool parsing boolean values") {
        StreamRedirectGuard guard("true\n");
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.system.Console;

            class Main {
                public static int32 main() {
                    bool b = Console.input_bool();
                    return b ? 0 : 1;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 2.8: Console.input_char reading single character") {
        StreamRedirectGuard guard("Z\n");
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.system.Console;

            class Main {
                public static int32 main() {
                    char c = Console.input_char();
                    return c == 'Z' ? 0 : 1;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 2.9: Console.input_chars reading whole line") {
        StreamRedirectGuard guard("Custom input string\n");
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.system.Console;

            class Main {
                public static int32 main() {
                    char[] line = Console.input_chars();
                    char[] expected = "Custom input string";
                    if (line == null) return 1;
                    if (line.length != expected.length) return 2;
                    for (int32 i = 0; i < line.length; i = i + 1) {
                        if (line[i] != expected[i]) return 3;
                    }
                    return 0;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 2.10 [Negative]: Console.input_int throws FormatException on invalid input") {
        StreamRedirectGuard guard("not_a_number\n");
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.system.Console;
            import solix.exceptions.*;

            class Main {
                public static int32 main() {
                    try {
                        int32 v = Console.input_int();
                        return 1;
                    } catch (FormatException fe) {
                        return 0;
                    }
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 2.11 [Negative]: Console.input_double throws FormatException on invalid input") {
        StreamRedirectGuard guard("3.14.bad\n");
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.system.Console;
            import solix.exceptions.*;

            class Main {
                public static int32 main() {
                    try {
                        float64 d = Console.input_double();
                        return 1;
                    } catch (FormatException fe) {
                        return 0;
                    }
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 2.12: Terminal Control Operations") {
        StreamRedirectGuard guard;
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.system.Console;

            class Main {
                public static int32 main() {
                    Console.set_color(35);
                    Console.print("Magenta");
                    Console.reset_color();
                    Console.set_cursor_position(10, 5);
                    Console.set_title("Solix App");
                    Console.flush();
                    return 0;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
        std::string out = guard.cout_stream.str();
        CHECK(out.find("\033[35m") != std::string::npos);
        CHECK(out.find("\033[0m") != std::string::npos);
        CHECK(out.find("\033[10;5H") != std::string::npos);
        CHECK(out.find("\033]0;Solix App\007") != std::string::npos);
    }
}
