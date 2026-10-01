#include "test_helper.hpp"

using namespace solix::test;

TEST_CASE("DefaultExpression - Expressions", "[expressions][default]") {
    SECTION("Case 1.1: Primitive Default Value Evaluation") {
        std::string code = R"(
public class Main {
    public static int32 main() {
        int32 i = default(int32);
        bool b = default(bool);
        float64 f = default(float64);
        char c = default(char);
        if (i == 0 && b == false && f == 0.0 && c == '\0') {
            return 0;
        }
        return 1;
    }
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 1.2: Reference Type Default Values") {
        std::string code = R"(
public class Item {
    public int32 id;
}

public class Main {
    public static int32 main() {
        Item item = default(Item);
        int32[] arr = default(int32[]);
        if (item == null && arr == null) {
            return 0;
        }
        return 1;
    }
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 1.3: Function Pointer Default Value") {
        std::string code = R"(
public class Main {
    public static int32 main() {
        int32(*)(int32) fn = default(int32(*)(int32));
        if (fn == null) {
            return 0;
        }
        return 1;
    }
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 1.4: Generic Container Slot Clearance with default(T)") {
        std::string code = R"(
public class Box<T> {
    public T val;
    public Box(T v) { this.val = v; }
    public void clear() {
        this.val = default(T);
    }
}

public class User {
    public int32 id;
}

public class Main {
    public static int32 main() {
        Box<int32> intBox = new Box<int32>(42);
        intBox.clear();
        Box<User> userBox = new Box<User>(new User());
        userBox.clear();
        if (intBox.val == 0 && userBox.val == null) {
            return 0;
        }
        return 1;
    }
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 1.5: Generic Function Returning default(T)") {
        std::string code = R"(
public class Helpers {
    public static T getDefault<T>() {
        return default(T);
    }
}

public class Main {
    public static int32 main() {
        int32 defInt = Helpers.getDefault<int32>();
        bool defBool = Helpers.getDefault<bool>();
        if (defInt == 0 && defBool == false) {
            return 0;
        }
        return 1;
    }
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 2.1: Default of Void Type (Compile-Time Error)") {
        std::string code = R"(
public class Main {
    public static void test() {
        int32 x = default(void);
    }
}
)";
        assert_compile_error(code, "Cannot use void in default expression");
    }

    SECTION("Case 2.2: Default of Undeclared Type (Compile-Time Error)") {
        std::string code = R"(
public class Main {
    public static void test() {
        int32 x = default(UnknownType);
    }
}
)";
        assert_compile_error(code, "Unknown type: UnknownType");
    }
}
