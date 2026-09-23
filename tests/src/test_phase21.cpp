#include <catch2/catch_test_macros.hpp>
#include "solix/compilation.hpp"
#include "processes/lexer.hpp"
#include "processes/parser.hpp"
#include "processes/binder.hpp"
#include "utilities/diagnostic.hpp"

using namespace solix;

TEST_CASE("Phase 21 - Implicit Deduction & Explicit Specialization", "[binder]") {
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
    
    // Generic function
    public void print_val<T>(T val) {}
    
    // Explicit Specialization!
    public void print_val<char[]>(char[] val) {}
    
    // Complex generic function
    public void process_pair<T1, T2>(int32 id, T2 val2, T1 val1) {}
    
    int32 main() {
        // Implicit deduction
        print_val(42); // Should instantiate print_val<int32>
        
        // This should route to the Explicit Specialization, avoiding duplicate generation
        print_val("hello"); // char[]
        
        // Complex implicit deduction
        process_pair(100, "hello", false); // T2=char[], T1=bool => process_pair<bool, char[]>
        
        return 0;
    }
  )";
  solix::Lexer lexer(ctx, "Lexer");
  lexer.execute();
  solix::Parser parser(ctx, "Parser");
  parser.execute();
  solix::Binder binder(ctx, "Binder");
  REQUIRE_NOTHROW(binder.execute());
  
  // Verify print_val<int32>(int32)
  REQUIRE(binder.global_scope.resolve("print_val<int32>(int32)") != nullptr);
  
  // Verify explicit specialization print_val<char[]>(char[])
  REQUIRE(binder.global_scope.resolve("print_val<char[]>(char[])") != nullptr);
  
  // Verify process_pair<bool,char[]>(int32,char[],bool)
  REQUIRE(binder.global_scope.resolve("process_pair<bool,char[]>(int32,char[],bool)") != nullptr);
}
