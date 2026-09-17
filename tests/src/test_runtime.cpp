#include <catch2/catch_test_macros.hpp>
#include "solix/runtime.hpp"
#include "solix/utilities/optcodes.hpp"

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
