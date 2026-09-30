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

    SECTION("Case 3.4: Multiple Declarations on Single Line") {
        std::string code = R"(
static int32 main() {
    int32 a = 1, b = 2, c = 3;
    return (a + b + c) == 6 ? 0 : 1;
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 3.5: Constant Local Variable Declaration") {
        std::string code = R"(
static int32 main() {
    const int32 LIMIT = 100;
    return LIMIT == 100 ? 0 : 1;
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 3.6: File-Scope Global Variable Declaration") {
        std::string code = R"(
int32 global_counter = 45;

static int32 main() {
    return global_counter == 45 ? 0 : 1;
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
NonExistentType obj = null;
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

    SECTION("Case 4.5: Reassigning Const Local Variable") {
        std::string code = R"(
void test() {
    const int32 x = 10;
    x = 20;
}
)";
        assert_compile_error(code, "Cannot assign to const variable 'x'");
    }

    SECTION("Case 4.6: Assigning Null to Primitive Type") {
        std::string code = R"(
void test() {
    int32 x = null;
}
)";
        assert_compile_error(code, "Cannot assign 'null' to primitive type 'int32'");
    }

    SECTION("Case 4.7: Solitary Variable Declaration in If Without Braces") {
        std::string code = R"(
void test() {
    if (true)
        int32 x = 5;
}
)";
        assert_compile_error(code, "Variable declarations are not allowed as immediate solitary branch statements without a block");
    }

    SECTION("Case 4.8: Solitary Variable Declaration in While Without Braces") {
        std::string code = R"(
void test() {
    while (true)
        int32 x = 5;
}
)";
        assert_compile_error(code, "Variable declarations are not allowed as immediate solitary loop statements without a block");
    }
}
