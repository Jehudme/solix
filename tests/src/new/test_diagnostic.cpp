#include <catch2/catch_test_macros.hpp>
#include "solix/new/compilation.hpp"
#include "solix/new/processes/diagnostic.hpp"
#include "solix/new/processes/process.hpp"
#include <spdlog/sinks/ostream_sink.h>
#include <sstream>
#include <memory>
#include <iostream>

using namespace solix;

class DummyProcess : public CompilationProcess {
public:
    DummyProcess(CompilationContext& ctx) : CompilationProcess(ctx, "DummyLexer") {}
    
    void execute() override {
        log_info("Starting lexical analysis...");
        log_warn("Found unexpected token");
    }
};

TEST_CASE("Diagnostic Process creates sub loggers successfully", "[new_architecture]") {
    CompilationOptions options;
    options.log_level = CompilationOptions::LogLevel::TRACE;
    options.sink_type = CompilationOptions::LogSinkType::STDOUT;
    
    CompilationContext context(options);
    
    context.diagnostic = std::make_unique<Diagnostic>(context);
    
    REQUIRE(context.diagnostic->get_root_logger() != nullptr);
    
    auto sub_logger = context.diagnostic->create_process_logger("Parser");
    REQUIRE(sub_logger != nullptr);
    REQUIRE(sub_logger->name() == "Parser");
}

TEST_CASE("CompilationProcess uses diagnostic logger", "[new_architecture]") {
    CompilationOptions options;
    // We can't easily capture stdout without redirecting standard streams or using custom sinks, 
    // but we can ensure it compiles and runs without crashing.
    options.log_level = CompilationOptions::LogLevel::INFO;
    
    CompilationContext context(options);
    context.diagnostic = std::make_unique<Diagnostic>(context);
    
    DummyProcess process(context);
    REQUIRE_NOTHROW(process.execute());
}
