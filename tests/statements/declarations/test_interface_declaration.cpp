#include "test_helper.hpp"

using namespace solix::test;

TEST_CASE("InterfaceDeclaration - Declarations", "[declarations][interface]") {
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

    SECTION("Case 3.5: Multi-Interface Dynamic Dispatch Execution") {
        std::string code = R"(
interface Worker { int32 work(); }
interface Greeter { int32 greet(); }

class Android implements Worker, Greeter {
    public int32 work() { return 10; }
    public int32 greet() { return 20; }
}

static int32 main() {
    Android a = new Android();
    Worker w = a;
    Greeter g = a;
    if (w.work() != 10) return 1;
    if (g.greet() != 20) return 2;
    return 0;
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 3.6: Order-Independent Multi-Interface Resolution") {
        std::string code = R"(
interface FirstIface { int32 getA(); }
interface SecondIface { int32 getB(); }

class ClassAB implements FirstIface, SecondIface {
    public int32 getA() { return 1; }
    public int32 getB() { return 2; }
}

class ClassBA implements SecondIface, FirstIface {
    public int32 getA() { return 10; }
    public int32 getB() { return 20; }
}

static int32 main() {
    FirstIface a1 = new ClassAB();
    SecondIface b1 = new ClassAB();
    FirstIface a2 = new ClassBA();
    SecondIface b2 = new ClassBA();

    if (a1.getA() != 1) return 1;
    if (b1.getB() != 2) return 2;
    if (a2.getA() != 10) return 3;
    if (b2.getB() != 20) return 4;
    return 0;
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 3.7: Sub-Interface Dynamic Dispatch and Cast") {
        std::string code = R"(
interface Reader { int32 read(); }
interface AdvancedReader extends Reader { int32 reset(); }

class StreamBuffer implements AdvancedReader {
    public int32 read() { return 42; }
    public int32 reset() { return 7; }
}

static int32 main() {
    AdvancedReader ar = new StreamBuffer();
    Reader r = ar;
    if (r.read() != 42) return 1;
    if (ar.read() != 42) return 2;
    if (ar.reset() != 7) return 3;
    return 0;
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 3.8: Class Inheritance with Interface Implementation Override") {
        std::string code = R"(
interface Action { int32 execute(); }

class BaseTask implements Action {
    public int32 execute() { return 50; }
}

class DerivedTask extends BaseTask {
    public override int32 execute() { return 100; }
}

static int32 main() {
    Action a1 = new BaseTask();
    Action a2 = new DerivedTask();
    if (a1.execute() != 50) return 1;
    if (a2.execute() != 100) return 2;
    return 0;
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 3.9: InstanceOf Check for Interfaces") {
        std::string code = R"(
interface Worker { int32 work(); }
interface Sleeper { void sleep(); }

class Human implements Worker {
    public int32 work() { return 1; }
}

static int32 main() {
    Human h = new Human();
    Worker w = h;
    if (!(h instanceof Worker)) return 1;
    if (!(w instanceof Worker)) return 2;
    if (h instanceof Sleeper) return 3;
    if (w instanceof Sleeper) return 4;
    return 0;
}
)";
        CHECK(run_source(code) == 0);
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

    SECTION("Case 4.6: Generic Interface Polymorphic Dispatch") {
        std::string code = R"(
interface IBox<T> {
    T get();
    void set(T v);
}
class Box<T> implements IBox<T> {
    private T val;
    public Box(T v) { this.val = v; }
    public T get() { return this.val; }
    public void set(T v) { this.val = v; }
}

public static int32 main() {
    IBox<int32> b = new Box<int32>(42);
    b.set(100);
    return b.get() == 100 ? 0 : 1;
}
)";
        assert_compile_success(code);
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 4.7: Generic Sub-Interface Inheritance and Upcasting") {
        std::string code = R"(
interface IBase<T> {
    T get();
}
interface IDerived<T> extends IBase<T> {
    void set(T v);
}
class Box<T> implements IDerived<T> {
    private T val;
    public Box(T v) { this.val = v; }
    public T get() { return this.val; }
    public void set(T v) { this.val = v; }
}

public static int32 main() {
    IDerived<int32> d = new Box<int32>(10);
    if (d.get() != 10) return 2;
    d.set(20);
    IBase<int32> b = d;
    return b.get() == 20 ? 0 : 1;
}
)";
        assert_compile_success(code);
        CHECK(run_source(code) == 0);
    }
}

