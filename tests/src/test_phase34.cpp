#include "solix/compilation.hpp"
#include "processes/assembler.hpp"
#include "processes/binder.hpp"
#include "processes/lexer.hpp"
#include "processes/parser.hpp"
#include "solix/runtime.hpp"
#include "utilities/diagnostic.hpp"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

using namespace solix;

TEST_CASE("Phase 34 - Semantic Analysis & Type System Binding", "[phase34]") {
    SECTION("Method Template Instantiation Caching & Idempotency") {
        CompilationOptions options;
        options.log_level = CompilationOptions::LogLevel::OFF;
        Source src_key = std::string("test_method_template");
        options.sources[src_key] = R"(
            public class TempTest {
                public static T identity<T>(T val) {
                    return val;
                }

                public static int32 main() {
                    int32 a = TempTest.identity<int32>(42);
                    int32 b = TempTest.identity<int32>(99);
                    if (a != 42 || b != 99) return 1;
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
        REQUIRE_NOTHROW(binder.execute());

        Assembler assembler(context, "Assembler");
        assembler.execute();

        RuntimeOptions opts;
        RuntimeContext runtime(opts);
        runtime.bytecode = context.bytecode;
        REQUIRE_NOTHROW(runtime.execute());
    }

    SECTION("Cross-Package Base Class Resolution Before VTable Calculation") {
        CompilationOptions options;
        options.log_level = CompilationOptions::LogLevel::OFF;
        options.sources[std::string("base_pkg")] = R"(
            package core.entities;
            public class BaseEntity {
                public virtual int32 get_id() { return 100; }
            }
        )";
        options.sources[std::string("derived_pkg")] = R"(
            package app.services;
            public class UserEntity : BaseEntity {
                public override int32 get_id() { return 200; }
            }

            public class ServiceRunner {
                public static int32 main() {
                    UserEntity u = new UserEntity();
                    if (u.get_id() != 200) return 1;
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
        REQUIRE_NOTHROW(binder.execute());

        Assembler assembler(context, "Assembler");
        assembler.execute();

        RuntimeOptions opts;
        RuntimeContext runtime(opts);
        runtime.bytecode = context.bytecode;
        REQUIRE_NOTHROW(runtime.execute());
    }

    SECTION("Robust VTable Override Signature Matching on Qualified Types") {
        CompilationOptions options;
        options.log_level = CompilationOptions::LogLevel::OFF;
        options.sources[std::string("models")] = R"(
            package data.models;
            public class Payload {
                public int32 value = 55;
            }
        )";
        options.sources[std::string("processor")] = R"(
            package data.processors;
            public class BaseProcessor {
                public virtual int32 process(data.models.Payload p) {
                    return p.value;
                }
            }

            public class DerivedProcessor : BaseProcessor {
                public override int32 process(data.models.Payload p) {
                    return p.value + 10;
                }
            }

            public class ProcRunner {
                public static int32 main() {
                    data.models.Payload p = new data.models.Payload();
                    BaseProcessor proc = new DerivedProcessor();
                    if (proc.process(p) != 65) return 1;
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
        REQUIRE_NOTHROW(binder.execute());

        Assembler assembler(context, "Assembler");
        assembler.execute();

        RuntimeOptions opts;
        RuntimeContext runtime(opts);
        runtime.bytecode = context.bytecode;
        REQUIRE_NOTHROW(runtime.execute());
    }

    SECTION("Field Initializer Type Mismatch Caught in Pass 3") {
        CompilationOptions options;
        options.log_level = CompilationOptions::LogLevel::OFF;
        Source src_key = std::string("test_bad_field");
        options.sources[src_key] = R"(
            public class BadField {
                public int32 x = "incompatible_string";
            }
        )";

        CompilationContext context(options);
        context.diagnostic = std::make_unique<Diagnostic>(context);

        Lexer lexer(context, "Lexer");
        lexer.execute();

        Parser parser(context, "Parser");
        parser.execute();

        Binder binder(context, "Binder");
        REQUIRE_THROWS_WITH(binder.execute(), Catch::Matchers::ContainsSubstring("Type mismatch in field initialization"));
    }

    SECTION("Subtype Polymorphism in Method and Constructor Overload Resolution") {
        CompilationOptions options;
        options.log_level = CompilationOptions::LogLevel::OFF;
        Source src_key = std::string("test_subtyping");
        options.sources[src_key] = R"(
            public class Animal {
                public int32 legs = 4;
            }
            public class Dog : Animal {
                public int32 bark_count = 1;
            }

            public class Shelter {
                public Animal resident;
                public Shelter(Animal a) {
                    this.resident = a;
                }

                public static int32 inspect(Animal a) {
                    return a.legs;
                }

                public static int32 main() {
                    Dog d = new Dog();
                    // Pass Dog to method expecting Animal
                    int32 legs = Shelter.inspect(d);
                    if (legs != 4) return 1;

                    // Pass Dog to constructor expecting Animal
                    Shelter s = new Shelter(d);
                    if (s.resident.legs != 4) return 2;

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
        REQUIRE_NOTHROW(binder.execute());

        Assembler assembler(context, "Assembler");
        assembler.execute();

        RuntimeOptions opts;
        RuntimeContext runtime(opts);
        runtime.bytecode = context.bytecode;
        REQUIRE_NOTHROW(runtime.execute());
    }
}
