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

TEST_CASE("Phase 30 - Nested Templates and Polymorphic Array Literals", "[phase30]") {
    SECTION("Polymorphic array literal assignability") {
        CompilationOptions options;
        options.log_level = CompilationOptions::LogLevel::OFF;
        Source src_key = std::string("test");
        options.sources[src_key] = R"(
            public class BaseObj {
                public int32 id;
            }

            public class DerivedObj : BaseObj {
                public int32 sub_id;
            }

            public class PolyArrayTest {
                public static int32 main() {
                    BaseObj b = new BaseObj();
                    DerivedObj d = new DerivedObj();
                    BaseObj[] arr = new BaseObj[]{ b, d };
                    if (arr.length == 2) return 600;
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
