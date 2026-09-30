#include "test_helper.hpp"

using namespace solix::test;

TEST_CASE("MethodDeclaration - Declarations", "[declarations][method]") {
    SECTION("Case 3.1: Virtual Method Overriding") {
        std::string code = R"(
class Parent {
    public virtual int32 get_val() { return 1; }
}
class Child extends Parent {
    public override int32 get_val() { return 2; }
}

static int32 main() {
    Parent p = new Child();
    return p.get_val();
}
)";
        CHECK(run_source(code) == 2);
    }

    SECTION("Case 3.2: Static Utility Method Invocation") {
        std::string code = R"(
class MathUtil {
    public static int32 add(int32 a, int32 b) {
        return a + b;
    }
}

static int32 main() {
    return MathUtil.add(20, 22) == 42 ? 0 : 1;
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 3.3: Method Overloading with Multiple Types") {
        std::string code = R"(
class Calculator {
    public int32 compute(int32 x) { return x * 2; }
    public float64 compute(float64 x) { return x * 2.0; }
}

static int32 main() {
    Calculator c = new Calculator();
    return (c.compute(10) == 20 && c.compute(1.5) == 3.0) ? 0 : 1;
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 3.4: Protected Method Accessible in Subclass") {
        std::string code = R"(
class BaseWorker {
    protected int32 getCode() { return 100; }
}
class DerivedWorker extends BaseWorker {
    public int32 test() { return this.getCode(); }
}

static int32 main() {
    DerivedWorker w = new DerivedWorker();
    return w.test();
}
)";
        CHECK(run_source(code) == 100);
    }

    SECTION("Case 4.1: Abstract Method with Body") {
        std::string code = R"(
abstract class Base {
    abstract void run() {}
}
)";
        assert_compile_error(code, "Abstract method 'run' cannot have a body");
    }

    SECTION("Case 4.2: Calling Private Method from Outside Class") {
        std::string code = R"(
class Encapsulated {
    private void secret() {}
}

void test() {
    Encapsulated e = new Encapsulated();
    e.secret();
}
)";
        assert_compile_error(code, "Cannot access private method 'secret' of class 'Encapsulated'");
    }

    SECTION("Case 4.3: Missing Return Statement in Non-Void Method") {
        std::string code = R"(
int32 badMethod(bool flag) {
    if (flag) {
        return 1;
    }
}
)";
        assert_compile_error(code, "Not all control paths return a value in function 'badMethod'");
    }

    SECTION("Case 4.4: Incompatible Override Signature Return Type") {
        std::string code = R"(
class SuperClass {
    public virtual int32 getValue() { return 0; }
}
class SubClass extends SuperClass {
    public override String getValue() { return "bad"; }
}
)";
        assert_compile_error(code, "Overriding method 'getValue' has incompatible return type 'String' (expected 'int32')");
    }

    SECTION("Case 4.7: Nested Method Declaration Inside Another Method") {
        std::string code = R"(
void outer() {
    void inner() {}
}
)";
        assert_compile_error(code, "Methods cannot be declared inside another method");
    }
}
