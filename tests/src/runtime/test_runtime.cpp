#include <catch2/catch_test_macros.hpp>
#include "solix/runtime.hpp"
#include "solix/compiler.hpp"
#include <iostream>

using namespace solix;

TEST_CASE("Runtime Native Print Test", "[runtime]") {
    bool print_called = false;
    
    solix::runtime::register_native_function("com.solix.advanced.test.Engine.print", [&print_called](solix::runtime::Program& p) {
        solix::runtime::Value val = p.pop_value();
        solix::runtime::Address addr = static_cast<solix::runtime::Address>(val);
        
        // Read length (4 bytes)
        std::vector<uint8_t> len_data = p.get_memory().read_global(addr, 0, 4);
        uint32_t len = (len_data[0] << 24) | (len_data[1] << 16) | (len_data[2] << 8) | len_data[3];
        
        // Read string data
        std::vector<uint8_t> str_data = p.get_memory().read_global(addr, 4, len);
        std::string str(str_data.begin(), str_data.end());
        
        std::cout << str << std::endl;
        
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
