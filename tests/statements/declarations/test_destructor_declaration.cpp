#include "test_helper.hpp"

using namespace solix::test;

TEST_CASE("DestructorDeclaration - Declarations", "[declarations][destructor]") {
    SECTION("Case 3.1: Explicit Dereference RAII Cleanup") {
        const std::string code = R"(
            class Tracker {
                public static int32 destroyed_count = 0;
                public ~Tracker() {
                    Tracker.destroyed_count = Tracker.destroyed_count + 1;
                }
            }

            class Main {
                public static int32 main() {
                    Tracker.destroyed_count = 0;
                    Tracker t = new Tracker();
                    if (Tracker.destroyed_count != 0) return 1;
                    t = null;
                    if (Tracker.destroyed_count != 1) return 2;
                    return 0;
                }
            }
        )";
        assert_compile_success(code);
        REQUIRE(run_and_evaluate_int(code) == 0);
    }

    SECTION("Case 3.2: Block Scope Exit RAII Cleanup") {
        const std::string code = R"(
            class Resource {
                public static int32 counter = 0;
                public ~Resource() {
                    Resource.counter = Resource.counter + 1;
                }
            }

            class Main {
                public static int32 main() {
                    Resource.counter = 0;
                    {
                        Resource r1 = new Resource();
                        Resource r2 = new Resource();
                        if (Resource.counter != 0) return 1;
                    }
                    if (Resource.counter != 2) return 2;
                    return 0;
                }
            }
        )";
        assert_compile_success(code);
        REQUIRE(run_and_evaluate_int(code) == 0);
    }

    SECTION("Case 3.3: Function Return RAII Cleanup") {
        const std::string code = R"(
            class Guard {
                public static int32 count = 0;
                public ~Guard() {
                    Guard.count = Guard.count + 1;
                }
            }

            class Main {
                public static int32 doWork() {
                    Guard g = new Guard();
                    return 42;
                }

                public static int32 main() {
                    Guard.count = 0;
                    int32 val = Main.doWork();
                    if (val != 42) return 1;
                    if (Guard.count != 1) return 2;
                    return 0;
                }
            }
        )";
        assert_compile_success(code);
        REQUIRE(run_and_evaluate_int(code) == 0);
    }

    SECTION("Case 3.4: Subclass-to-Base Inheritance Chaining") {
        const std::string code = R"(
            class BaseResource {
                public static int32 sequence = 0;
                public ~BaseResource() {
                    BaseResource.sequence = BaseResource.sequence * 10 + 1;
                }
            }

            class DerivedResource extends BaseResource {
                public ~DerivedResource() {
                    BaseResource.sequence = BaseResource.sequence * 10 + 2;
                }
            }

            class Main {
                public static int32 main() {
                    BaseResource.sequence = 0;
                    {
                        DerivedResource d = new DerivedResource();
                    }
                    // Expected sequence: Derived runs (2), then Base runs (21)
                    if (BaseResource.sequence != 21) return 1;
                    return 0;
                }
            }
        )";
        assert_compile_success(code);
        REQUIRE(run_and_evaluate_int(code) == 0);
    }

    SECTION("Case 3.5: Multi-Level Inheritance Chaining") {
        const std::string code = R"(
            class Grandparent {
                public static int32 order = 0;
                public ~Grandparent() {
                    Grandparent.order = Grandparent.order * 10 + 1;
                }
            }

            class Parent extends Grandparent {
                public ~Parent() {
                    Grandparent.order = Grandparent.order * 10 + 2;
                }
            }

            class Child extends Parent {
                public ~Child() {
                    Grandparent.order = Grandparent.order * 10 + 3;
                }
            }

            class Main {
                public static int32 main() {
                    Grandparent.order = 0;
                    {
                        Child c = new Child();
                    }
                    // Expected order: Child (3) -> Parent (32) -> Grandparent (321)
                    if (Grandparent.order != 321) return 1;
                    return 0;
                }
            }
        )";
        assert_compile_success(code);
        REQUIRE(run_and_evaluate_int(code) == 0);
    }

    SECTION("Case 3.6: Field and State Mutation within Destructor") {
        const std::string code = R"(
            class Logger {
                public static int32 last_logged = 0;
                public int32 id;
                public Logger(int32 id) { this.id = id; }
                public ~Logger() {
                    Logger.last_logged = this.id;
                }
            }

            class Main {
                public static int32 main() {
                    {
                        Logger l = new Logger(777);
                    }
                    if (Logger.last_logged != 777) return 1;
                    return 0;
                }
            }
        )";
        assert_compile_success(code);
        REQUIRE(run_and_evaluate_int(code) == 0);
    }

    SECTION("Case 3.7: Cascading Destruction of Nested Resources") {
        const std::string code = R"(
            class InnerNode {
                public static int32 count = 0;
                public ~InnerNode() {
                    InnerNode.count = InnerNode.count + 1;
                }
            }

            class OuterContainer {
                public InnerNode node;
                public OuterContainer() {
                    this.node = new InnerNode();
                }
                public ~OuterContainer() {
                    this.node = null;
                }
            }

            class Main {
                public static int32 main() {
                    InnerNode.count = 0;
                    {
                        OuterContainer c = new OuterContainer();
                    }
                    if (InnerNode.count != 1) return 1;
                    return 0;
                }
            }
        )";
        assert_compile_success(code);
        REQUIRE(run_and_evaluate_int(code) == 0);
    }

    SECTION("Case 3.8: Exception Rewinding RAII across Multiple Call Frames") {
        const std::string code = R"(
            class Exception {}

            class AutoCleanup {
                public static int32 cleaned_frames = 0;
                public ~AutoCleanup() {
                    AutoCleanup.cleaned_frames = AutoCleanup.cleaned_frames + 1;
                }
            }

            class TestExceptions {
                public static void levelThree() {
                    AutoCleanup c3 = new AutoCleanup();
                    throw new Exception();
                }

                public static void levelTwo() {
                    AutoCleanup c2 = new AutoCleanup();
                    TestExceptions.levelThree();
                }

                public static void levelOne() {
                    AutoCleanup c1 = new AutoCleanup();
                    TestExceptions.levelTwo();
                }
            }

            class Main {
                public static int32 main() {
                    AutoCleanup.cleaned_frames = 0;
                    try {
                        TestExceptions.levelOne();
                    } catch (Exception e) {
                        if (AutoCleanup.cleaned_frames != 3) return 1;
                        return 0;
                    }
                    return 2;
                }
            }
        )";
        assert_compile_success(code);
        REQUIRE(run_and_evaluate_int(code) == 0);
    }

    SECTION("Case 3.9: Exception in Try Block RAII") {
        const std::string code = R"(
            class Exception {}

            class ScopedLock {
                public static bool unlocked = false;
                public ~ScopedLock() {
                    ScopedLock.unlocked = true;
                }
            }

            class Main {
                public static int32 main() {
                    ScopedLock.unlocked = false;
                    try {
                        ScopedLock lock = new ScopedLock();
                        throw new Exception();
                    } catch (Exception e) {
                        if (!ScopedLock.unlocked) return 1;
                        return 0;
                    }
                    return 2;
                }
            }
        )";
        assert_compile_success(code);
        REQUIRE(run_and_evaluate_int(code) == 0);
    }

    SECTION("Case 3.10: Variable & Field Reassignment Cleanup") {
        const std::string code = R"(
            class Cell {
                public static int32 counter = 0;
                public ~Cell() {
                    Cell.counter = Cell.counter + 1;
                }
            }

            class Main {
                public static int32 main() {
                    Cell.counter = 0;
                    Cell c = new Cell();
                    if (Cell.counter != 0) return 1;
                    c = new Cell();
                    if (Cell.counter != 1) return 2;
                    c = null;
                    if (Cell.counter != 2) return 3;
                    return 0;
                }
            }
        )";
        assert_compile_success(code);
        REQUIRE(run_and_evaluate_int(code) == 0);
    }

    SECTION("Case 3.11: Array of Destructible Objects Cleanup") {
        const std::string code = R"(
            class Item {
                public static int32 count = 0;
                public ~Item() {
                    Item.count = Item.count + 1;
                }
            }

            class Main {
                public static int32 main() {
                    Item.count = 0;
                    {
                        Item[] arr = new Item[3];
                        arr[0] = new Item();
                        arr[1] = new Item();
                        arr[2] = new Item();
                        if (Item.count != 0) return 1;
                    }
                    if (Item.count != 3) return 2;
                    return 0;
                }
            }
        )";
        assert_compile_success(code);
        REQUIRE(run_and_evaluate_int(code) == 0);
    }

    SECTION("Case 4.1: Destructor Declared with Parameters") {
        const std::string code = R"(
            class Widget {
                public ~Widget(int32 x) {}
            }
        )";
        assert_compile_error(code, "Destructors cannot accept parameters");
    }

    SECTION("Case 4.2: Destructor Declared with Explicit Return Type or Returning Value") {
        const std::string code = R"(
            class Widget {
                public ~Widget() {
                    return 10;
                }
            }
        )";
        assert_compile_error(code, "Cannot return a value from a destructor");
    }

    SECTION("Case 4.3: Duplicate Destructors in Same Class") {
        const std::string code = R"(
            class Widget {
                public ~Widget() {}
                public ~Widget() {}
            }
        )";
        assert_compile_error(code, "cannot declare multiple destructors");
    }

    SECTION("Case 4.4: Destructor Name Mismatch") {
        const std::string code = R"(
            class Widget {
                public ~Other() {}
            }
        )";
        assert_compile_error(code, "Destructor name '~Other' does not match enclosing class 'Widget'");
    }

    SECTION("Case 4.5: Destructor in Interface") {
        const std::string code = R"(
            interface IService {
                ~IService();
            }
        )";
        assert_compile_error(code, "Interfaces cannot declare destructors");
    }

    SECTION("Case 4.6: Destructor Declared with Forbidden Modifiers") {
        const std::string code = R"(
            class Widget {
                public static ~Widget() {}
            }
        )";
        assert_compile_error(code, "Destructors cannot have static, virtual, override, or abstract modifiers");
    }
}
