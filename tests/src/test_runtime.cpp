#include <catch2/catch_test_macros.hpp>
#include "solix/runtime.hpp"
#include "utilities/optcodes.hpp"
#include <iostream>
#include <sstream>

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

TEST_CASE("Extended Console Streams, Control, Colors, and Inputs", "[runtime]") {
    const auto &natives = get_builtin_natives();

    // Verify Error Stream
    REQUIRE(natives.count("solix.systems.Console.error(char[])") == 1);
    REQUIRE(natives.count("solix.systems.Console.error(solix.String)") == 1);
    REQUIRE(natives.count("solix.systems.Console.errorln(char[])") == 1);
    REQUIRE(natives.count("solix.systems.Console.errorln(solix.String)") == 1);
    REQUIRE(natives.count("solix.systems.Console.errorln()") == 1);

    // Verify Warning Stream
    REQUIRE(natives.count("solix.systems.Console.warn(char[])") == 1);
    REQUIRE(natives.count("solix.systems.Console.warn(solix.String)") == 1);
    REQUIRE(natives.count("solix.systems.Console.warnln(char[])") == 1);
    REQUIRE(natives.count("solix.systems.Console.warnln(solix.String)") == 1);

    // Verify Info & Debug Streams
    REQUIRE(natives.count("solix.systems.Console.info(char[])") == 1);
    REQUIRE(natives.count("solix.systems.Console.info(solix.String)") == 1);
    REQUIRE(natives.count("solix.systems.Console.infoln(char[])") == 1);
    REQUIRE(natives.count("solix.systems.Console.infoln(solix.String)") == 1);
    REQUIRE(natives.count("solix.systems.Console.debug(char[])") == 1);
    REQUIRE(natives.count("solix.systems.Console.debug(solix.String)") == 1);
    REQUIRE(natives.count("solix.systems.Console.debugln(char[])") == 1);
    REQUIRE(natives.count("solix.systems.Console.debugln(solix.String)") == 1);

    // Verify Control & Colors
    REQUIRE(natives.count("solix.systems.Console.clear()") == 1);
    REQUIRE(natives.count("solix.systems.Console.flush()") == 1);
    REQUIRE(natives.count("solix.systems.Console.beep()") == 1);
    REQUIRE(natives.count("solix.systems.Console.set_color(solix.systems.ConsoleColor)") == 1);
    REQUIRE(natives.count("solix.systems.Console.set_background(solix.systems.ConsoleColor)") == 1);
    REQUIRE(natives.count("solix.systems.Console.reset_color()") == 1);

    // Verify Input Functions
    REQUIRE(natives.count("solix.systems.Console.input_char()") == 1);
    REQUIRE(natives.count("solix.systems.Console.input_bool()") == 1);
    REQUIRE(natives.count("solix.systems.Console.input_int8()") == 1);
    REQUIRE(natives.count("solix.systems.Console.input_int16()") == 1);
    REQUIRE(natives.count("solix.systems.Console.input_int32()") == 1);
    REQUIRE(natives.count("solix.systems.Console.input_int64()") == 1);
    REQUIRE(natives.count("solix.systems.Console.input_uint8()") == 1);
    REQUIRE(natives.count("solix.systems.Console.input_uint16()") == 1);
    REQUIRE(natives.count("solix.systems.Console.input_uint32()") == 1);
    REQUIRE(natives.count("solix.systems.Console.input_uint64()") == 1);
    REQUIRE(natives.count("solix.systems.Console.input_float32()") == 1);
    REQUIRE(natives.count("solix.systems.Console.input_float64()") == 1);
    REQUIRE(natives.count("solix.systems.Console.input_line()") == 1);

    // Test execution with simulated stdin
    RuntimeOptions opts;
    RuntimeContext ctx(opts);

    std::stringstream input_stream;
    input_stream << "Hello Solix\n42\ntrue\n";
    auto *old_cin_buf = std::cin.rdbuf(input_stream.rdbuf());

    // Test input_line()
    auto fn_input_line = natives.at("solix.systems.Console.input_line()");
    uint64_t line_addr_val = fn_input_line(ctx, 0, nullptr, 0);
    REQUIRE(line_addr_val > 0);
    Address line_addr = static_cast<Address>(line_addr_val);
    uint32_t len = static_cast<uint32_t>(ctx.memory.heap[line_addr - 1] >> 32);
    REQUIRE(len == 11); // "Hello Solix"
    std::string read_str;
    for (uint32_t i = 0; i < len; ++i) {
        read_str += static_cast<char>(ctx.memory.heap[line_addr + i]);
    }
    REQUIRE(read_str == "Hello Solix");

    // Test input_int32()
    auto fn_input_int32 = natives.at("solix.systems.Console.input_int32()");
    uint64_t int_val = fn_input_int32(ctx, 0, nullptr, 0);
    REQUIRE(static_cast<int32_t>(int_val) == 42);

    // Test input_bool()
    auto fn_input_bool = natives.at("solix.systems.Console.input_bool()");
    uint64_t bool_val = fn_input_bool(ctx, 0, nullptr, 0);
    REQUIRE(bool_val == 1);

    // Restore cin buffer
    std::cin.rdbuf(old_cin_buf);

    // Test colors & control execution
    uint64_t color_arg[1] = { 2 }; // RED
    auto fn_set_color = natives.at("solix.systems.Console.set_color(solix.systems.ConsoleColor)");
    REQUIRE_NOTHROW(fn_set_color(ctx, 0, color_arg, 1));

    auto fn_reset_color = natives.at("solix.systems.Console.reset_color()");
    REQUIRE_NOTHROW(fn_reset_color(ctx, 0, nullptr, 0));

    auto fn_flush = natives.at("solix.systems.Console.flush()");
    REQUIRE_NOTHROW(fn_flush(ctx, 0, nullptr, 0));
}
