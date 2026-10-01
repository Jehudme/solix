#include "test_helper.hpp"
#include "solix/path_utils.hpp"

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

    SECTION("Case 3.5: Cross-Platform Binary Path Resolution") {
        auto exe_path = solix::get_executable_path();
        CHECK(!exe_path.empty());
        CHECK(std::filesystem::exists(exe_path));
        auto exe_dir = solix::get_executable_dir();
        CHECK(!exe_dir.empty());
        CHECK(std::filesystem::exists(exe_dir));
        CHECK(exe_path.parent_path() == exe_dir);
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
        assert_compile_error(code, "Cannot resolve import");
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

    SECTION("Case 3.3: Hierarchical Multi-Level Wildcard Import") {
        std::unordered_map<std::string, std::string> sources;
        sources["collections/list.slx"] = R"(
            package my.collections;

            public class MyList {
                public int32 size() { return 0; }
            }
        )";
        sources["main.slx"] = R"(
            import my.collections.*;

            class Main {
                public static int32 main() {
                    MyList list = new MyList();
                    return list.size() == 0 ? 0 : 1;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_sources(sources) == 0);
    }

    SECTION("Case 3.4: Qualified Access with Active Import") {
        std::unordered_map<std::string, std::string> sources;
        sources["core/item.slx"] = R"(
            package my.core;

            public class Item {
                public int32 val;
                public Item(int32 v) { this.val = v; }
                public int32 getVal() { return this.val; }
            }
        )";
        sources["main.slx"] = R"(
            import my.core.Item;

            class Main {
                public static int32 main() {
                    my.core.Item s1 = new my.core.Item(10);
                    Item s2 = new Item(20);
                    return (s1.getVal() == 10 && s2.getVal() == 20) ? 0 : 1;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_sources(sources) == 0);
    }

    SECTION("Case 4.3: Misplaced Import Statement") {
        const std::string code = R"(
            class Foo {}
            import my.core.Item;
        )";
        assert_compile_error(code, "Import statements must appear before class declarations");
    }

    SECTION("Case 4.4: Importing Non-Existent Member from Existing Package") {
        std::unordered_map<std::string, std::string> sources;
        sources["core.slx"] = R"(
            package my.core;
            public class RealSymbol {}
        )";
        sources["main.slx"] = R"(
            import my.core.FakeSymbol;
        )";
        assert_compile_sources_error(sources, "Symbol 'FakeSymbol' not found in package 'my.core'");
    }

    SECTION("Case 4.5: Import Statement Inside Class Body") {
        const std::string code = R"(
            class Foo {
                import my.core.Item;
            }
        )";
        assert_compile_error(code, "Import statements must appear before class declarations");
    }

    SECTION("Case 4.6: Import Statement Inside Function Body") {
        const std::string code = R"(
            class Main {
                public static int32 main() {
                    import my.core.Item;
                    return 0;
                }
            }
        )";
        assert_compile_error(code, "Import statements must appear before class declarations");
    }
}

