#include <catch2/catch_test_macros.hpp>
#include "solix/runtime.hpp"
#include "solix/compiler.hpp"
#include <iostream>

using namespace solix;

TEST_CASE("Runtime Native Print Test", "[runtime]") {
    bool print_called = false;
    
    solix::runtime::register_native_function("com.solix.advanced.test.Engine.print", [&print_called](solix::runtime::Program& p) {
        // Pop argument size, wait...
        // Actually compiler pushes args, then calls native
        // Native function doesn't get arg count natively in this simple VM? Wait, CALL_NATIVE just passes ID.
        // Let's assume print takes 1 string argument.
        solix::runtime::Value val = p.pop_value();
        // Since it's a string, val is an address. We would read it from memory pool.
        // For testing, just verify it's called.
        print_called = true;
        p.push_value(0);
    });

    compiler::Compiler comp;
    comp.include(std::filesystem::path("/home/jehud/Projects/solix/tests/resources/test.slx"));
    std::vector<uint8_t> bytecode = comp.compile("main");

    runtime::Program prog(bytecode);
    prog.run();

    REQUIRE(print_called == true);
}
