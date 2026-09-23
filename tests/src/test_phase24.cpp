#include "solix/compilation.hpp"
#include "solix/processes/assembler.hpp"
#include "solix/processes/binder.hpp"
#include "solix/processes/lexer.hpp"
#include "solix/processes/parser.hpp"
#include "solix/runtime.hpp"
#include "solix/utilities/diagnostic.hpp"
#include "solix/utilities/optcodes.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace solix;

TEST_CASE("Phase 24 - Typed ALU Opcodes (_I64 & _F64)", "[phase24]") {
    SECTION("Integer arithmetic emits _I64 opcodes") {
        CompilationOptions options;
        options.log_level = CompilationOptions::LogLevel::OFF;
        Source src_key = std::string("test");
        options.sources[src_key] = R"(
            public class Phase24Test {
                public static int32 main() {
                    int32 a = 15;
                    int32 b = 3;
                    int32 c = a + b * 2;
                    return c;
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

        const auto& code = context.bytecode;
        bool found_add_i64 = false;
        bool found_mul_i64 = false;

        for (uint8_t op : code) {
            if (static_cast<OpCode>(op) == OpCode::ADD_I64) found_add_i64 = true;
            if (static_cast<OpCode>(op) == OpCode::MUL_I64) found_mul_i64 = true;
        }

        REQUIRE(found_add_i64);
        REQUIRE(found_mul_i64);
    }

    SECTION("Float arithmetic emits _F64 opcodes") {
        CompilationOptions options;
        options.log_level = CompilationOptions::LogLevel::OFF;
        Source src_key = std::string("test");
        options.sources[src_key] = R"(
            public class Phase24FloatTest {
                public static float64 main() {
                    float64 a = 15.5;
                    float64 b = 2.5;
                    float64 c = a + b * 2.0;
                    return c;
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

        const auto& code = context.bytecode;
        bool found_add_f64 = false;
        bool found_mul_f64 = false;

        for (uint8_t op : code) {
            if (static_cast<OpCode>(op) == OpCode::ADD_F64) found_add_f64 = true;
            if (static_cast<OpCode>(op) == OpCode::MUL_F64) found_mul_f64 = true;
        }

        REQUIRE(found_add_f64);
        REQUIRE(found_mul_f64);
    }
}
