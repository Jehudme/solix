#include <catch2/catch_test_macros.hpp>
#include "solix/runtime.hpp"
#include "utilities/optcodes.hpp"

using namespace solix;

TEST_CASE("Runtime VM bounds checking", "[runtime]") {
    RuntimeOptions opts;
    opts.stack_capacity = 10;
    opts.heap_capacity = 10;
    RuntimeContext ctx(opts);
    
    SECTION("Stack overflow") {
        for (int i = 0; i < 10; ++i) {
            ctx.push(i);
        }
        REQUIRE_THROWS_AS(ctx.push(10), std::runtime_error);
    }
    
    SECTION("Stack underflow") {
        REQUIRE_THROWS_AS(ctx.pop(), std::runtime_error);
    }
    
    SECTION("Heap bounds on GET_PROPERTY") {
        ctx.bytecode = {
            static_cast<uint8_t>(OpCode::GET_PROPERTY),
            0, 0, 0, 15, // offset 15
            static_cast<uint8_t>(OpCode::HALT)
        };
        ctx.push(0); // Address 0
        REQUIRE_THROWS_AS(ctx.execute(), std::runtime_error);
    }
}

TEST_CASE("Builtin Natives Registry and Console Prints", "[runtime]") {
    const auto &natives = get_builtin_natives();
    REQUIRE_FALSE(natives.empty());

    // Verify all 12 primitives exist for print and println
    const std::vector<std::string> prim_types = {
        "bool", "char", "int8", "int16", "int32", "int64",
        "uint8", "uint16", "uint32", "uint64", "float32", "float64"
    };

    for (const auto &t : prim_types) {
        REQUIRE(natives.count("solix.systems.Console.print(" + t + ")") == 1);
        REQUIRE(natives.count("solix.systems.Console.println(" + t + ")") == 1);
        REQUIRE(natives.count("solix.systems.Console.print(" + t + "[])") == 1);
        REQUIRE(natives.count("solix.systems.Console.println(" + t + "[])") == 1);
    }

    REQUIRE(natives.count("solix.systems.Console.println()") == 1);
    REQUIRE(natives.count("solix.systems.Console.print(solix.String)") == 1);
    REQUIRE(natives.count("solix.systems.Console.println(solix.String)") == 1);

    // Verify Engine.print was removed as requested
    REQUIRE(natives.count("com.solix.advanced.test.Engine.print(char[])") == 0);
    REQUIRE(natives.count("com.solix.advanced.test.Engine.print(int32)") == 0);

    // Test calling an actual native function via the registry
    RuntimeOptions opts;
    RuntimeContext ctx(opts);
    uint64_t args[1] = { 42 };
    auto fn = natives.at("solix.systems.Console.println(int32)");
    REQUIRE_NOTHROW(fn(ctx, 0, args, 1));
}
