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

static int32 main() {
    Person p = new Person();
    p.addr = new Address();
    p.addr.zip = 90210;
    return p.addr.zip == 90210 ? 0 : 1;
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

static int32 main() {
    Person p = null;
    Address a = p.addr;
    return 0;
}
)";
        CHECK_THROWS_WITH(run_source(code), Catch::Matchers::ContainsSubstring("NullPointer"));
    }
}
