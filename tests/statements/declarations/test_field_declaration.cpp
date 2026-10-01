#include "test_helper.hpp"

using namespace solix::test;

TEST_CASE("FieldDeclaration - Declarations", "[declarations][field]") {
    SECTION("Case 3.1: Cycle Breaking with Weak References") {
        std::string code = R"(
class Parent {
    public Child c;
}
class Child {
    public weak Parent p;
}

static int32 main() {
    Parent p = new Parent();
    Child c = new Child();
    p.c = c;
    c.p = p;
    return 0;
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 3.2: Field Access Modifiers (Public, Private, Protected)") {
        std::string code = R"(
class Account {
    public int32 id;
    private int32 secret;
    protected int32 balance;

    public Account(int32 id, int32 sec, int32 bal) {
        this.id = id;
        this.secret = sec;
        this.balance = bal;
    }
    public int32 getSecret() { return this.secret; }
}

static int32 main() {
    Account a = new Account(1, 1234, 500);
    return a.getSecret() == 1234 ? 0 : 1;
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 3.3: Static Class Fields Shared Across Instances") {
        std::string code = R"(
class Counter {
    public static int32 count;
}

static int32 main() {
    Counter.count = 10;
    Counter c = new Counter();
    Counter.count++;
    return Counter.count == 11 ? 0 : 1;
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 3.4: Constant Field Declaration") {
        std::string code = R"(
class Config {
    public const int32 MAX_USERS = 500;
}

static int32 main() {
    return Config.MAX_USERS == 500 ? 0 : 1;
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 3.5: In-Class Field Initialization with Object and Primitive Literals") {
        std::string code = R"(
class Item {
    public int32 id;
    public Item(int32 i) { this.id = i; }
}

class Entity {
    private Item item = new Item(42);
    private int32 count = 100;

    public int32 getItemId() {
        return this.item.id;
    }
    public int32 getCount() {
        return this.count;
    }
}

static int32 main() {
    Entity e = new Entity();
    return (e.getItemId() == 42 && e.getCount() == 100) ? 0 : 1;
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 3.6: Static Field Initialization with Object Instantiation") {
        std::string code = R"(
class Tag {
    public int32 code;
    public Tag(int32 c) { this.code = c; }
}

class Config {
    public static Tag tag = new Tag(99);
    public static int32 version = 3;

    public static Tag getTag() {
        return tag;
    }
}

static int32 main() {
    return (Config.getTag().code == 99 && Config.version == 3) ? 0 : 1;
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 4.1: Duplicate Field Identifier") {
        std::string code = R"(
class Item {
    int32 count;
    float64 count;
}
)";
        assert_compile_error(code, "Field 'count' is already declared in class 'Item'");
    }

    SECTION("Case 4.2: Field Access on Null Reference (Runtime Fault)") {
        std::string code = R"(
class Item {
    public int32 count;
}
static int32 main() {
    Item item = null;
    item.count = 5;
    return 0;
}
)";
        CHECK_THROWS_WITH(run_source(code), Catch::Matchers::ContainsSubstring("NullPointer"));
    }

    SECTION("Case 4.3: Accessing Private Field Outside Class") {
        std::string code = R"(
class Vault {
    private int32 passcode;
}

void test() {
    Vault v = new Vault();
    int32 x = v.passcode;
}
)";
        assert_compile_error(code, "Cannot access private member 'passcode' of class 'Vault'");
    }

    SECTION("Case 4.4: Modifying Const Field") {
        std::string code = R"(
class Constants {
    public const int32 RATE = 5;
}

void test() {
    Constants.RATE = 10;
}
)";
        assert_compile_error(code, "Cannot assign to read-only constant field 'RATE'");
    }

    SECTION("Case 4.5: Incompatible Field Initializer Type") {
        std::string code = R"(
class Model {
    public int32 count = "invalid";
}
)";
        assert_compile_error(code, "Incompatible initializer for field 'count': expected 'int32', got 'String'");
    }

    SECTION("Case 4.7: Field Access Modifier Applied to Local Variable") {
        std::string code = R"(
void test() {
    public int32 x = 10;
}
)";
        assert_compile_error(code, "Access modifiers ('public', 'private', 'protected') are not allowed on local variables");
    }

    SECTION("Case 4.8: Standalone Field Declared at File Top Level") {
        std::string code = R"(
public int32 globalField = 42;
)";
        assert_compile_error(code, "Variable declarations with access modifiers must be inside a class body");
    }
}
