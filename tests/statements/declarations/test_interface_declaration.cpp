#include "test_helper.hpp"

using namespace solix::test;

TEST_CASE("InterfaceDeclaration - Declarations", "[declarations][interface][!mayfail]") {
    SECTION("Case 3.1: Multiple Interface Conformance") {
        std::string code = R"(
interface Printable { void print(); }
interface Serializable { void save(); }

class Doc implements Printable, Serializable {
    public void print() {}
    public void save() {}
}
)";
        assert_compile_success(code);
    }

    SECTION("Case 3.2: Interface Hierarchy with Sub-Interface Extension") {
        std::string code = R"(
interface Reader {
    int32 read();
}
interface AdvancedReader extends Reader {
    void reset();
}
)";
        assert_compile_success(code);
    }

    SECTION("Case 3.3: Polymorphic Method Invocation via Interface Variable") {
        std::string code = R"(
interface Worker {
    int32 work();
}
class Robot implements Worker {
    public int32 work() { return 99; }
}

static int32 main() {
    Worker w = new Robot();
    return w.work() == 99 ? 0 : 1;
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 3.4: Nested Interface Declaration Inside Class") {
        std::string code = R"(
class Button {
    public interface OnClickListener {
        void onClick();
    }
}
)";
        assert_compile_success(code);
    }

    SECTION("Case 4.1: Interface Method with Body") {
        std::string code = R"(
interface Reader {
    int32 read() { return 0; }
}
)";
        assert_compile_error(code, "Interface methods cannot have a body");
    }

    SECTION("Case 4.2: Class Incompletely Implementing Interface") {
        std::string code = R"(
interface Service {
    void start();
    void stop();
}
class IncompleteService implements Service {
    public void start() {}
}
)";
        assert_compile_error(code, "Class 'IncompleteService' does not implement interface method 'stop()'");
    }

    SECTION("Case 4.3: Interface Containing State Fields") {
        std::string code = R"(
interface BadInterface {
    int32 stateField;
}
)";
        assert_compile_error(code, "Interfaces must not declare instance fields");
    }

    SECTION("Case 4.5: Interface Declared Inside Function Body") {
        std::string code = R"(
void test() {
    interface LocalInterface {}
}
)";
        assert_compile_error(code, "Interfaces cannot be declared inside a function or method body");
    }
}
