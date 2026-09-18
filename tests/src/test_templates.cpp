#include <catch2/catch_test_macros.hpp>
#include "solix/compilation.hpp"
#include "solix/processes/lexer.hpp"
#include "solix/processes/parser.hpp"
#include "solix/processes/binder.hpp"
#include "solix/utilities/diagnostic.hpp"

using namespace solix;

TEST_CASE("Phase 19 - Templates Monomorphization", "[binder]") {
  solix::CompilationOptions opts;
  opts.log_level = solix::CompilationOptions::LogLevel::DEBUG;
  solix::CompilationContext ctx(opts);
  ctx.diagnostic = std::make_unique<solix::Diagnostic>(ctx);
  opts.sources[std::string("test")] = R"(
    class Box<T> {
        T value;
        public T get_value() { return value; }
        public void set_value(T v) { value = v; }
    }
    
    int32 main() {
        Box<int32> int_box = new Box<int32>();
        int_box.set_value(5);
        int32 x = int_box.get_value();
        
        Box<float64> float_box = new Box<float64>();
        float_box.set_value(3.14);
        float64 y = float_box.get_value();
        
        return 0;
    }
  )";
  solix::Lexer lexer(ctx, "Lexer");
  lexer.execute();
  solix::Parser parser(ctx, "Parser");
  parser.execute();
  solix::Binder binder(ctx, "Binder");
  REQUIRE_NOTHROW(binder.execute());
  
  REQUIRE(ctx.nodes[std::string("__instantiated_templates")].size() == 2);
}
