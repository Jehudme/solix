#include "test_helper.hpp"

using namespace solix::test;

TEST_CASE("OperatorDeclaration - Declarations", "[declarations][operator]") {
    SECTION("Case 3.1: Vector Addition Overload") {
        std::string code = R"(
class Vector {
    public int32 x;
    Vector(int32 x) { this.x = x; }
    public Vector operator+(Vector other) {
        return new Vector(this.x + other.x);
    }
}

static int32 main() {
    Vector v1 = new Vector(10);
    Vector v2 = new Vector(20);
    Vector v3 = v1 + v2;
    return v3.x;
}
)";
        CHECK(run_source(code) == 30);
    }

    SECTION("Case 4.1: Unsupported Operator Overload") {
        std::string code = R"(
class Test {
    bool operator&&(Test other) { return true; }
}
)";
        assert_compile_error(code, "Operator '&&' cannot be overloaded");
    }
}
