#include "test_helper.hpp"

using namespace solix::test;

TEST_CASE("PackageStatement - Modules", "[modules][package]") {
    SECTION("Case 3.1: Hierarchical Multi-Level Package") {
        const std::string code = R"(
            package std.collections.generic;

            public class CustomList {
                public CustomList() {}
            }

            class Main {
                public static int32 main() {
                    CustomList list = new CustomList();
                    return 0;
                }
            }
        )";
        assert_compile_success(code);
        REQUIRE(run_and_evaluate_int(code) == 0);
    }

    SECTION("Case 3.2: Intra-Package Unqualified Access") {
        std::unordered_map<std::string, std::string> sources = {
            {"user.slx", R"(
                package app.models;

                public class User {
                    public int32 id;
                    public User() {
                        id = 42;
                    }
                }
            )"},
            {"account.slx", R"(
                package app.models;

                public class Account {
                    public User owner;
                    public Account() {
                        owner = new User();
                    }
                }

                class Main {
                    public static int32 main() {
                        Account acc = new Account();
                        return acc.owner.id == 42 ? 0 : 1;
                    }
                }
            )"}
        };
        assert_compile_sources_success(sources);
        REQUIRE(run_sources(sources) == 0);
    }

    SECTION("Case 4.1: Package Statement Not First") {
        const std::string code = R"(
            class Dummy {}
            package app;
        )";
        assert_compile_error(code, "'package' statement must be the first statement in the file");
    }

    SECTION("Case 4.2: Duplicate Package Statement") {
        const std::string code = R"(
            package alpha;
            package beta;
        )";
        assert_compile_error(code, "Only one 'package' statement is allowed per file");
    }
}
