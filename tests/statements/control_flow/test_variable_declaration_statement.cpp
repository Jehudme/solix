#include "test_helper.hpp"

using namespace solix::test;

TEST_CASE("VariableDeclarationStatement - Control Flow", "[control_flow][variable_declaration]") {
    SECTION("Case 3.1: Primitive and Reference Declarations") {
        std::string code = R"(
static int32 main() {
    int32 a = 1;
    float64 b = 2.5;
    bool c = true;
    int32[] nums = new int32[5];
    return a;
}
)";
        CHECK(run_source(code) == 1);
    }

    SECTION("Case 3.2: Polymorphic Upcasting") {
        std::string code = R"(
class Animal {}
class Cat extends Animal {}

static int32 main() {
    Animal a = new Cat();
    return 0;
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 4.1: Type Mismatch in Initializer") {
        std::string code = R"(
void test() {
    int32 x = "hello";
}
)";
        assert_compile_error(code, "Type mismatch in variable declaration");
    }

    SECTION("Case 4.2: Duplicate Declaration in Same Scope") {
        std::string code = R"(
void test() {
    int32 score = 10;
    int32 score = 20;
}
)";
        assert_compile_error(code, "already defined");
    }

    SECTION("Case 4.3: Unknown Type Name") {
        std::string code = R"(
void test() {
    NonExistentType obj = null;
}
)";
        assert_compile_error(code, "Unknown type");
    }

    SECTION("Case 4.4: Declaring Variable as Void") {
        std::string code = R"(
void test() {
    void placeholder;
}
)";
        assert_compile_error(code, "Variable cannot be of type 'void'");
    }
}

TEST_CASE("VariableDeclarationStatement - Interface Binding", "[control_flow][variable_declaration][!mayfail]") {
    SECTION("Case 3.3: Interface Binding") {
        std::string code = R"(
interface Printable { void print(); }
class Doc implements Printable { void print() {} }

void test() {
    Printable p = new Doc();
}
)";
        assert_compile_success(code);
    }
}
