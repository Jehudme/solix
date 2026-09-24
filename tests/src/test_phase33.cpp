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

TEST_CASE("Phase 33 - Assembler CodeGen & ARC Safety", "[phase33]") {
    SECTION("Constructor Field Initializers Dispatched and Evaluated") {
        CompilationOptions options;
        options.log_level = CompilationOptions::LogLevel::OFF;
        Source src_key = std::string("test_field_init");
        options.sources[src_key] = R"(
            public class FieldInitTest {
                public int32 x = 42;
                public int32 y = 100;
                public FieldInitTest() {}

                public static int32 main() {
                    FieldInitTest t = new FieldInitTest();
                    if (t.x != 42) return 1;
                    if (t.y != 100) return 2;
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

    SECTION("Postfix vs Prefix Increment and Decrement") {
        CompilationOptions options;
        options.log_level = CompilationOptions::LogLevel::OFF;
        Source src_key = std::string("test_inc_dec");
        options.sources[src_key] = R"(
            public class PostfixTest {
                public static int32 main() {
                    int32 i = 5;
                    int32 post = i++;
                    if (post != 5) return 1;
                    if (i != 6) return 2;

                    int32 pre = ++i;
                    if (pre != 7) return 3;
                    if (i != 7) return 4;

                    int32 post_dec = i--;
                    if (post_dec != 7) return 5;
                    if (i != 6) return 6;

                    int32 pre_dec = --i;
                    if (pre_dec != 5) return 7;
                    if (i != 5) return 8;

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

    SECTION("Return Local Reference Prevents Use-After-Free") {
        CompilationOptions options;
        options.log_level = CompilationOptions::LogLevel::OFF;
        Source src_key = std::string("test_ret_ref");
        options.sources[src_key] = R"(
            public class NodeObj {
                public int32 val;
                public NodeObj(int32 v) { this.val = v; }
            }

            public class ReturnRefTest {
                public static NodeObj create_node(int32 v) {
                    NodeObj local_node = new NodeObj(v);
                    return local_node;
                }

                public static int32 main() {
                    NodeObj result = create_node(999);
                    if (result.val != 999) return 1;
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

    SECTION("Array Literal Reference Elements Retain References") {
        CompilationOptions options;
        options.log_level = CompilationOptions::LogLevel::OFF;
        Source src_key = std::string("test_arr_lit_ref");
        options.sources[src_key] = R"(
            public class Element {
                public int32 id;
                public Element(int32 i) { this.id = i; }
            }

            public class ArrayLitRefTest {
                public static int32 main() {
                    Element[] arr = { new Element(10), new Element(20), new Element(30) };
                    if (arr[0].id != 10) return 1;
                    if (arr[1].id != 20) return 2;
                    if (arr[2].id != 30) return 3;
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
