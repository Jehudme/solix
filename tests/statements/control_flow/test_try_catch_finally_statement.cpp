#include "test_helper.hpp"

using namespace solix::test;

TEST_CASE("TryCatchFinallyStatement - Control Flow", "[control_flow][try_catch_finally]") {
    SECTION("Case 3.1: Specific Catch Hierarchy") {
        std::string code = R"(
class Exception {}
class CustomError extends Exception {}

int32 main() {
    int32 result = 0;
    try {
        throw new CustomError();
    } catch (CustomError c) {
        result = 1;
    } catch (Exception e) {
        result = 2;
    }
    return result;
}
)";
        CHECK(run_source(code) == 1);
    }

    SECTION("Case 3.2: Try Block with Only Finally Clause") {
        std::string code = R"(
static int32 cleanup_marker = 0;

void work() {
    try {
        cleanup_marker += 10;
    } finally {
        cleanup_marker += 20;
    }
}

int32 main() {
    work();
    return cleanup_marker == 30 ? 0 : 1;
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 3.3: Exception in Catch with Guaranteed Finally Execution") {
        std::string code = R"(
class Exception {}

static bool finally_ran = false;

void faulty() {
    try {
        throw new Exception();
    } catch (Exception e) {
        throw new Exception();
    } finally {
        finally_ran = true;
    }
}

int32 main() {
    try {
        faulty();
    } catch (Exception e) {}
    return finally_ran ? 0 : 1;
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 4.1: Unreachable Catch Clause") {
        std::string code = R"(
class Exception {}
class SubErr extends Exception {}

void test() {
    try {
        int32 x = 0;
    } catch (Exception e) {
    } catch (SubErr s) {
    }
}
)";
        assert_compile_error(code, "Unreachable catch clause: 'SubErr' is already handled by preceding catch for 'Exception'");
    }

    SECTION("Case 4.2: Catching Non-Exception Type") {
        std::string code = R"(
void test() {
    try {} catch (int32 x) {}
}
)";
        assert_compile_error(code, "Catch type must derive from 'Exception', got 'int32'");
    }

    SECTION("Case 4.3: Duplicate Catch Clause for Same Type") {
        std::string code = R"(
class Exception {}

void test() {
    try {}
    catch (Exception e) {}
    catch (Exception e2) {}
}
)";
        assert_compile_error(code, "Duplicate catch clause for type 'Exception'");
    }

    SECTION("Case 4.5: Try-Catch Statement Placed Directly in Class Body") {
        std::string code = R"(
class Exception {}
class BadClass {
    try {} catch (Exception e) {}
}
)";
        assert_compile_error(code, "Statements are not allowed directly in class body");
    }
}
