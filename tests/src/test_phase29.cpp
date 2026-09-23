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

TEST_CASE("Phase 29 - Static String Pool", "[phase29]") {
    SECTION("String literal deduplication and string pool initialization") {
        CompilationOptions options;
        options.log_level = CompilationOptions::LogLevel::OFF;
        Source src_key = std::string("test");
        options.sources[src_key] = R"(
            public class StringPoolTest {
                public static int32 main() {
                    char[] s1 = "hello world";
                    char[] s2 = "hello world";
                    if (s1.length == 11 && s2.length == 11) return 500;
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

        REQUIRE(context.string_pool.count("hello world") == 1);

        Assembler assembler(context, "Assembler");
        assembler.execute();

        RuntimeOptions opts;
        RuntimeContext runtime(opts);
        runtime.bytecode = context.bytecode;
        REQUIRE_NOTHROW(runtime.execute());
    }
}
