#include <catch2/catch_test_macros.hpp>
#include "solix/new/compilation.hpp"
#include "solix/new/processes/diagnostic.hpp"
#include "solix/new/processes/lexer.hpp"
#include "solix/new/processes/parser.hpp"
#include "solix/new/processes/binder.hpp"

using namespace solix;

inline CompilationContext* run_full_pipeline(const std::filesystem::path& path) {
    CompilationOptions options;
    options.log_level = CompilationOptions::LogLevel::DEBUG;
    Source src_key = path;
    options.sources[src_key] = std::nullopt;

    auto* context = new CompilationContext(options);
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
    auto* context = run_full_pipeline("/home/jehud/Projects/solix/tests/resources/test.slx");
    REQUIRE(context != nullptr);
    
    Source src_key = std::filesystem::path("/home/jehud/Projects/solix/tests/resources/test.slx");
    REQUIRE(!context->nodes[src_key].empty());
    
    delete context;
}
