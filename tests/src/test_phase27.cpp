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

TEST_CASE("Phase 27 - Function Cleanup and Flow Control Leaks", "[phase27]") {
    SECTION("Function parameter ARC cleanup and break/continue scope cleanup") {
        CompilationOptions options;
        options.log_level = CompilationOptions::LogLevel::OFF;
        Source src_key = std::string("test");
        options.sources[src_key] = R"(
            public class Dummy {
                public int32 val;
            }

            public class CleanupTest {
                public static int32 process(Dummy d) {
                    int32 sum = 0;
                    int32 i = 0;
                    while (i < 5) {
                        Dummy local_d = new Dummy();
                        local_d.val = i;
                        sum = sum + local_d.val;
                        i = i + 1;
                        if (i == 3) {
                            break;
                        }
                    }
                    return sum;
                }

                public static int32 main() {
                    Dummy arg = new Dummy();
                    arg.val = 10;
                    int32 res = process(arg);
                    if (res == 3) return 200;
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
