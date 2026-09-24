#include <catch2/matchers/catch_matchers_string.hpp>
#include "solix/compilation.hpp"
#include "processes/binder.hpp"
#include "processes/lexer.hpp"
#include "processes/parser.hpp"
#include "utilities/diagnostic.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace solix;

inline CompilationContext *
run_full_pipeline(const std::filesystem::path &path) {
  CompilationOptions options;
  options.log_level = CompilationOptions::LogLevel::DEBUG;
  Source src_key = path;
  options.sources[src_key] = std::nullopt;

  std::filesystem::path stdlib_dir = path.parent_path() / "../../launcher/rsc/lib/solix";
  if (std::filesystem::exists(stdlib_dir)) {
    for (const auto& entry : std::filesystem::recursive_directory_iterator(stdlib_dir)) {
      if (entry.is_regular_file() && entry.path().extension() == ".slx" && std::filesystem::file_size(entry.path()) > 0) {
        options.sources[std::filesystem::canonical(entry.path())] = std::nullopt;
      }
    }
  }

  auto *context = new CompilationContext(options);
  context->diagnostic = std::make_unique<Diagnostic>(*context);

  Lexer lexer(*context, "Lexer");
  lexer.execute();

  Parser parser(*context, "Parser");
  parser.execute();

  Binder binder(*context, "Binder");
  binder.execute();

  return context;
}

TEST_CASE("New Binder - Full test.slx script", "[new_binder]") {
  std::string test_path = (std::filesystem::path(__FILE__).parent_path().parent_path() / "resources" / "test.slx").string();
  auto *context = run_full_pipeline(test_path);
  REQUIRE(context != nullptr);

  Source src_key = std::filesystem::path(test_path);
  REQUIRE(!context->nodes[src_key].empty());

  delete context;
}


TEST_CASE("Phase 4: Cast Safety", "[binder]") {
    auto run_binder = [](const std::string& code) {
        CompilationOptions options;
        options.log_level = CompilationOptions::LogLevel::DEBUG;
        options.sources[std::string("inline_test")] = code;
        
        auto* context = new CompilationContext(options);
        context->diagnostic = std::make_unique<Diagnostic>(*context);
        
        Lexer lexer(*context, "Lexer");
        lexer.execute();
        
        Parser parser(*context, "Parser");
        parser.execute();
        
        Binder binder(*context, "Binder");
        binder.execute();
        
        delete context;
    };

    SECTION("Primitive casts allowed") {
        REQUIRE_NOTHROW(run_binder("int32 main() { float64 a = 5.0; int32 b = (int32) a; return 0; }"));
    }

    SECTION("Primitive <-> Class cast disallowed") {
        REQUIRE_THROWS_WITH(run_binder("class Dog {} int32 main() { Dog d = new Dog(); int32 a = (int32) d; return 0; }"), Catch::Matchers::ContainsSubstring("Cannot cast between primitive and class types"));
    }

    SECTION("Upcast allowed") {
        REQUIRE_NOTHROW(run_binder("class Animal {} class Dog : Animal {} int32 main() { Dog d = new Dog(); Animal a = (Animal) d; return 0; }"));
    }

    SECTION("Downcast allowed (with note)") {
        REQUIRE_NOTHROW(run_binder("class Animal {} class Dog : Animal {} int32 main() { Animal a = new Animal(); Dog d = (Dog) a; return 0; }"));
    }

    SECTION("Unrelated cast disallowed") {
        REQUIRE_THROWS_WITH(run_binder("class Dog {} class Engine {} int32 main() { Dog d = new Dog(); Engine e = (Engine) d; return 0; }"), Catch::Matchers::ContainsSubstring("no inheritance relationship"));
    }
}

TEST_CASE("Phase 5: Access Control", "[binder]") {
    auto run_binder = [](const std::string& code) {
        CompilationOptions options;
        options.log_level = CompilationOptions::LogLevel::DEBUG;
        options.sources[std::string("inline_test")] = code;
        
        auto* context = new CompilationContext(options);
        context->diagnostic = std::make_unique<Diagnostic>(*context);
        
        Lexer lexer(*context, "Lexer");
        lexer.execute();
        
        Parser parser(*context, "Parser");
        parser.execute();
        
        Binder binder(*context, "Binder");
        binder.execute();
        
        delete context;
    };

    SECTION("Private access from outside fails") {
        REQUIRE_THROWS_WITH(run_binder("class A { private int32 x; } int32 main() { A a = new A(); a.x = 5; return 0; }"), Catch::Matchers::ContainsSubstring("Cannot access private member"));
    }

    SECTION("Private access from inside succeeds") {
        REQUIRE_NOTHROW(run_binder("class A { private int32 x; public void set() { x = 5; } }"));
    }

    SECTION("Protected access from subclass succeeds") {
        REQUIRE_NOTHROW(run_binder("class A { protected int32 x; } class B : A { public void set() { x = 5; } }"));
    }

    SECTION("Protected access from outside fails") {
        REQUIRE_THROWS_WITH(run_binder("class A { protected int32 x; } int32 main() { A a = new A(); a.x = 5; return 0; }"), Catch::Matchers::ContainsSubstring("Cannot access protected member"));
    }
}

TEST_CASE("Binder - Forward Declaration of Classes", "[binder]") {
  solix::CompilationOptions opts;
  opts.log_level = solix::CompilationOptions::LogLevel::ERR;
  solix::CompilationContext ctx(opts);
  ctx.diagnostic = std::make_unique<solix::Diagnostic>(ctx);
  opts.sources[std::string("test")] = R"(
    class A {
        B b;
        void foo() {
            b = new B();
        }
    }
    class B {
        int32 x;
    }
  )";
  solix::Lexer lexer(ctx, "Lexer");
  lexer.execute();
  solix::Parser parser(ctx, "Parser");
  parser.execute();
  solix::Binder binder(ctx, "Binder");
  REQUIRE_NOTHROW(binder.execute());
}
