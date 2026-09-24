#include "solix/compilation.hpp"
#include "processes/assembler.hpp"
#include "processes/binder.hpp"
#include "processes/lexer.hpp"
#include "processes/parser.hpp"
#include "solix/runtime.hpp"
#include "utilities/diagnostic.hpp"
#include "utilities/optcodes.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace solix;

TEST_CASE("Phase 32 - Runtime VM & Memory Safety", "[phase32]") {
    SECTION("Integer and Floating Point Negation") {
        CompilationOptions options;
        options.log_level = CompilationOptions::LogLevel::OFF;
        Source src_key = std::string("test_negation");
        options.sources[src_key] = R"(
            public class NegationTest {
                public static int32 main() {
                    int32 a = 15;
                    int32 b = -a;
                    int32 c = -b;
                    if (b != -15) return 1;
                    if (c != 15) return 2;

                    float64 x = 4.5;
                    float64 y = -x;
                    if (y >= 0.0) return 3;
                    return 0;
                }
            }
        )";

        CompilationContext context(options);
        context.diagnostic = std::make_unique<Diagnostic>(context);

        Lexer lexer(context, "Lexer");
        lexer.execute();

        Parser parser(context, "Parser");
        parser.execute();

        Binder binder(context, "Binder");
        binder.execute();

        Assembler assembler(context, "Assembler");
        assembler.execute();

        RuntimeOptions opts;
        RuntimeContext runtime(opts);
        runtime.bytecode = context.bytecode;
        REQUIRE_NOTHROW(runtime.execute());
    }

    SECTION("Primitive Type Conversions (int <-> float, truncation)") {
        CompilationOptions options;
        options.log_level = CompilationOptions::LogLevel::OFF;
        Source src_key = std::string("test_conv");
        options.sources[src_key] = R"(
            public class ConvTest {
                public static int32 main() {
                    int32 big = 257;
                    int8 truncated = (int8)big;
                    if ((int32)truncated != 1) return 1;

                    float64 f = 7.9;
                    int32 truncated_int = (int32)f;
                    if (truncated_int != 7) return 2;

                    int32 int_val = 42;
                    float64 conv_float = (float64)int_val;
                    if (conv_float < 41.9 || conv_float > 42.1) return 3;

                    return 0;
                }
            }
        )";

        CompilationContext context(options);
        context.diagnostic = std::make_unique<Diagnostic>(context);

        Lexer lexer(context, "Lexer");
        lexer.execute();

        Parser parser(context, "Parser");
        parser.execute();

        Binder binder(context, "Binder");
        binder.execute();

        Assembler assembler(context, "Assembler");
        assembler.execute();

        RuntimeOptions opts;
        RuntimeContext runtime(opts);
        runtime.bytecode = context.bytecode;
        REQUIRE_NOTHROW(runtime.execute());
    }

    SECTION("Sized Free-List Dynamic Allocation and Splitting") {
        Memory mem(1024, 1024 * 16);
        // Allocate a block of 10 words
        Address addr1 = mem.dynamic_allocation(10);
        REQUIRE(addr1 != 0);
        // Free it
        mem.deallocate(addr1);
        REQUIRE(mem.free_blocks.size() == 1);

        // Allocate a block of 4 words: should reuse and split addr1 (10 words >= 4 + 2)
        Address addr2 = mem.dynamic_allocation(4);
        REQUIRE(addr2 == addr1); // Reused the same base address
        REQUIRE(mem.free_blocks.size() == 1); // Split remainder is in free_blocks

        // Allocate a larger block of 50 words: free block of ~5 words is too small, so allocates fresh
        Address addr3 = mem.dynamic_allocation(50);
        REQUIRE(addr3 > addr2 + 10);
    }

    SECTION("Unhandled Exception Reference Count Decrement") {
        RuntimeOptions opts;
        RuntimeContext runtime(opts);
        Address exc_addr = runtime.memory.dynamic_allocation(2); // initial ref count 1
        
        // Simulating bytecode throwing exception that bubbles to top level
        Bytecode code = {
            static_cast<uint8_t>(OpCode::PUSH_CONST_I64),
            0, 0, 0, 0, 0, 0, static_cast<uint8_t>((exc_addr >> 8) & 0xFF), static_cast<uint8_t>(exc_addr & 0xFF),
            static_cast<uint8_t>(OpCode::THROW_EXCEPTION),
            0xFF, 0xFF, 0xFF, 0xFF // No local cleanup
        };
        runtime.bytecode = code;
        REQUIRE_THROWS_AS(runtime.execute(), std::runtime_error);
        
        // The exception ref count was decreased back to initial (1)
        uint32_t ref = static_cast<uint32_t>(runtime.memory.heap[exc_addr - 1] & 0xFFFFFFFF);
        REQUIRE(ref == 1);
    }
}
