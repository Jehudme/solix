#include <catch2/catch_test_macros.hpp>
#include "solix/compilation.hpp"
#include "solix/processes/lexer.hpp"
#include "solix/processes/parser.hpp"
#include "solix/processes/binder.hpp"
#include "solix/utilities/diagnostic.hpp"

using namespace solix;

TEST_CASE("Phase 20 - Alias and Function Templates", "[binder]") {
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
    
    // Alias template
    alias BoxAlias<U> = Box<U>;
    // Concrete alias
    alias IntBox = Box<int32>;
    
    // Function template (Wait, top-level functions are supported!)
    public void swap_boxes<T>(Box<T> a, Box<T> b) {
        T temp = a.get_value();
        a.set_value(b.get_value());
        b.set_value(temp);
    }
    
    int32 main() {
        BoxAlias<float64> f_box = new BoxAlias<float64>();
        f_box.set_value(1.23);
        
        IntBox i_box1 = new IntBox();
        i_box1.set_value(10);
        IntBox i_box2 = new IntBox();
        i_box2.set_value(20);
        
        // Call generic function
        swap_boxes<int32>(i_box1, i_box2);
        
        return 0;
    }
  )";
  solix::Lexer lexer(ctx, "Lexer");
  lexer.execute();
  solix::Parser parser(ctx, "Parser");
  parser.execute();
  solix::Binder binder(ctx, "Binder");
  REQUIRE_NOTHROW(binder.execute());
  
  // Verify that Box<float64> and Box<int32> were generated!
  REQUIRE(binder.global_scope.resolve("Box<float64>") != nullptr);
  REQUIRE(binder.global_scope.resolve("Box<int32>") != nullptr);
  // Verify that swap_boxes<int32>(Box<int32>, Box<int32>) was generated!
  // Mangled name for swap_boxes<int32>(Box<int32>, Box<int32>)
  REQUIRE(binder.global_scope.resolve("swap_boxes<int32>(Box<int32>,Box<int32>)") != nullptr);
}
