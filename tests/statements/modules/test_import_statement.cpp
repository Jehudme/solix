#include "test_helper.hpp"

using namespace solix::test;

TEST_CASE("ImportStatement - Modules", "[modules][import]") {
    SECTION("Case 3.1: Selective Import") {
        std::unordered_map<std::string, std::string> sources = {
            {"pkg/list.slx", R"(
                package std.collections;
                public class List {
                    public List() {}
                }
            )"},
            {"main.slx", R"(
                import std.collections.List;

                class Main {
                    public static int32 main() {
                        List items = new List();
                        return 0;
                    }
                }
            )"}
        };
        assert_compile_sources_success(sources);
        REQUIRE(run_sources(sources) == 0);
    }

    SECTION("Case 3.2: Wildcard Import") {
        std::unordered_map<std::string, std::string> sources = {
            {"pkg/console.slx", R"(
                package std.io;
                public class Console {
                    public static int32 get_magic() {
                        return 42;
                    }
                }
            )"},
            {"main.slx", R"(
                import std.io.*;

                class Main {
                    public static int32 main() {
                        int32 v = Console.get_magic();
                        return v == 42 ? 0 : 1;
                    }
                }
            )"}
        };
        assert_compile_sources_success(sources);
        REQUIRE(run_sources(sources) == 0);
    }

    SECTION("Case 4.1: Importing Non-Existent Package") {
        const std::string code = R"(
            import invalid.pkg.Foo;

            class Main {
                public static int32 main() {
                    return 0;
                }
            }
        )";
        assert_compile_error(code, "Cannot resolve imported symbol");
    }

    SECTION("Case 4.2: Ambiguous Symbol Collision") {
        std::unordered_map<std::string, std::string> sources = {
            {"pkg_a.slx", R"(
                package pkg_a;
                public class Token {}
            )"},
            {"pkg_b.slx", R"(
                package pkg_b;
                public class Token {}
            )"},
            {"main.slx", R"(
                import pkg_a.*;
                import pkg_b.*;

                class Main {
                    public static int32 main() {
                        Token t;
                        return 0;
                    }
                }
            )"}
        };
        assert_compile_sources_error(sources, "ambiguous");
    }
}
