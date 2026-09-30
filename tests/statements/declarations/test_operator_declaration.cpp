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

    SECTION("Case 3.2: Overloading Subtraction and Equality Operators") {
        std::string code = R"(
class Complex {
    public int32 re;
    public int32 im;
    public Complex(int32 r, int32 i) { this.re = r; this.im = i; }

    public Complex operator-(Complex rhs) {
        return new Complex(this.re - rhs.re, this.im - rhs.im);
    }
    public bool operator==(Complex rhs) {
        return this.re == rhs.re && this.im == rhs.im;
    }
}

static int32 main() {
    Complex c1 = new Complex(10, 5);
    Complex c2 = new Complex(4, 2);
    Complex res = c1 - c2;
    return (res == new Complex(6, 3)) ? 0 : 1;
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 4.1: Unsupported Operator Overload") {
        std::string code = R"(
class Test {
    bool operator&&(Test other) { return true; }
}
)";
        assert_compile_error(code, "Operator '&&' cannot be overloaded");
    }

    SECTION("Case 4.2: Binary Operator Declared with Wrong Arity") {
        std::string code = R"(
class Vector {
    public Vector operator+(Vector a, Vector b) {}
}
)";
        assert_compile_error(code, "Member binary operator '+' must take exactly 1 argument");
    }

    SECTION("Case 4.4: Operator Overload Declared at Top Level") {
        std::string code = R"(
public int32 operator+(int32 a, int32 b) {
    return a + b;
}
)";
        assert_compile_error(code, "Operator overloads can only be declared inside a class body");
    }

    SECTION("Case 4.5: Operator Overload Declared Inside Method Body") {
        std::string code = R"(
void test() {
    operator+(int32 a) {}
}
)";
        assert_compile_error(code, "Operator overloads can only be declared inside a class body");
    }
}
