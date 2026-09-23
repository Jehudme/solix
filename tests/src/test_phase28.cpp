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

TEST_CASE("Phase 28 - Object Initialization and Array Mutation", "[phase28]") {
    SECTION("Implicit default constructor and field initializers") {
        CompilationOptions options;
        options.log_level = CompilationOptions::LogLevel::OFF;
        Source src_key = std::string("test");
        options.sources[src_key] = R"(
            public class FieldInitTest {
                public int32 val = 100;
                public int32 extra = 50;
            }

            public class MainClass {
                public static int32 main() {
                    FieldInitTest obj = new FieldInitTest();
                    if (obj.val == 100 && obj.extra == 50) return 300;
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

    SECTION("Array element compound mutation with DUP2") {
        CompilationOptions options;
        options.log_level = CompilationOptions::LogLevel::OFF;
        Source src_key = std::string("test");
        options.sources[src_key] = R"(
            public class ArrayMutTest {
                public static int32 main() {
                    int32[] arr = new int32[3];
                    arr[1] = 10;
                    arr[1]++;
                    if (arr[1] == 11) return 400;
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
}
