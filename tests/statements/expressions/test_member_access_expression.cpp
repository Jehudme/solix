#include "test_helper.hpp"

using namespace solix::test;

TEST_CASE("MemberAccessExpression - Expressions", "[expressions][member_access]") {
    SECTION("Case 3.1: Chained Member Access") {
        std::string code = R"(
class Address {
    public int32 zip;
}
class Person {
    public Address addr;
}

int32 main() {
    Person p = new Person();
    p.addr = new Address();
    p.addr.zip = 90210;
    return p.addr.zip == 90210 ? 0 : 1;
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 3.2: Static Field Access on Class Name") {
        std::string code = R"(
class MathConstants {
    public static int32 SCALE = 100;
}

int32 main() {
    return MathConstants.SCALE == 100 ? 0 : 1;
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 4.1: Member Access on Null Reference (Runtime Fault)") {
        std::string code = R"(
class Address {
    public int32 zip;
}
class Person {
    public Address addr;
}

int32 main() {
    Person p = null;
    Address a = p.addr;
    return 0;
}
)";
        CHECK_THROWS_WITH(run_source(code), Catch::Matchers::ContainsSubstring("NullPointer"));
    }

    SECTION("Case 4.2: Accessing Non-Existent Member Field") {
        std::string code = R"(
class Empty {}

void test() {
    Empty e = new Empty();
    int32 x = e.unknownField;
}
)";
        assert_compile_error(code, "Class 'Empty' has no member named 'unknownField'");
    }

    SECTION("Case 4.4: Using Super Keyword in Non-Derived Class") {
        std::string code = R"(
class BaseOnly {
    public void test() {
        super.doSomething();
    }
}
)";
        assert_compile_error(code, "Keyword 'super' cannot be used in a class with no superclass");
    }
}

