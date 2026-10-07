#include "test_helper.hpp"

using namespace solix::test;

TEST_CASE("Compiler Diagnostics - Consistency of Path, Row, and Column", "[diagnostics][core]") {
    SECTION("Case 51.1: Syntax Error Reports Source Name, Row, Column, and Caret") {
        std::unordered_map<std::string, std::string> sources;
        sources["syntax_err.slx"] = R"(
class Main {
    public static int32 main() {
        int32 x = ;
        return 0;
    }
}
)";
        auto result = compile_sources(sources);
        REQUIRE_FALSE(result.success);
        REQUIRE_FALSE(result.reports.empty());

        bool found = false;
        for (const auto &rep : result.reports) {
            if (rep.code == "E_PARSE") {
                found = true;
                CHECK(rep.source_path == "syntax_err.slx");
                CHECK(rep.line == 4);
                CHECK(rep.column > 0);
            }
        }
        REQUIRE(found);
    }

    SECTION("Case 51.2: Lexical Error Reports Source Name, Row, and Column") {
        std::unordered_map<std::string, std::string> sources;
        sources["lex_err.slx"] = R"(
class Main {
    public static int32 main() {
        @
        return 0;
    }
}
)";
        auto result = compile_sources(sources);
        REQUIRE_FALSE(result.success);
        REQUIRE_FALSE(result.reports.empty());

        bool found = false;
        for (const auto &rep : result.reports) {
            if (rep.code == "E_LEX") {
                found = true;
                CHECK(rep.source_path == "lex_err.slx");
                CHECK(rep.line == 4);
                CHECK(rep.column > 0);
            }
        }
        REQUIRE(found);
    }

    SECTION("Case 51.3: Semantic Binding Error Reports Source Name, Row, Column, and Caret") {
        std::unordered_map<std::string, std::string> sources;
        sources["bind_err.slx"] = R"(
class Main {
    public static int32 main() {
        int32 x = "mismatched";
        return 0;
    }
}
)";
        auto result = compile_sources(sources);
        REQUIRE_FALSE(result.success);
        REQUIRE_FALSE(result.reports.empty());

        bool found = false;
        for (const auto &rep : result.reports) {
            if (rep.code == "E_BIND") {
                found = true;
                CHECK(rep.source_path == "bind_err.slx");
                CHECK(rep.line == 4);
                CHECK(rep.column > 0);
            }
        }
        REQUIRE(found);
    }
}
