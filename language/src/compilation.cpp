#include "solix/compilation.hpp"
#include "processes/assembler.hpp"
#include "processes/binder.hpp"
#include "processes/lexer.hpp"
#include "processes/parser.hpp"
#include "utilities/statements.hpp"
#include "utilities/diagnostic.hpp"
#include "utilities/token.hpp"
#include <fstream>

namespace solix {
CompilationContext::CompilationContext(const CompilationOptions &opts)
    : options(opts) {}
CompilationContext::~CompilationContext() = default;

std::vector<uint8_t> run(CompilationOptions& options) {
    CompilationContext context(options);
    context.diagnostic = std::make_unique<Diagnostic>(context);
    
    auto start_time = std::chrono::steady_clock::now();
    size_t file_count = options.sources.size();
    if (context.diagnostic->get_root_logger()) {
        context.diagnostic->get_root_logger()->info("Compiling {} source file{}...",
            file_count, file_count == 1 ? "" : "s");
    }
    
    Lexer lexer(context, "Lexer");
    lexer.execute();
    if (context.diagnostic->has_errors()) {
        throw CompilationFailedException("Lexical analysis failed with errors");
    }
    
    Parser parser(context, "Parser");
    parser.execute();
    if (context.diagnostic->has_errors()) {
        throw CompilationFailedException("Syntax analysis failed with errors");
    }
    
    Binder binder(context, "Binder");
    try {
        binder.execute();
    } catch (const std::exception &e) {
        throw CompilationFailedException(std::string("Semantic analysis failed: ") + e.what());
    }
    if (context.diagnostic->has_errors()) {
        throw CompilationFailedException("Semantic analysis failed with errors");
    }
    
    Assembler assembler(context, "Assembler");
    try {
        assembler.execute();
    } catch (const std::exception &e) {
        throw CompilationFailedException(std::string("Assembly failed: ") + e.what());
    }
    if (context.diagnostic->has_errors()) {
        throw CompilationFailedException("Assembly failed with errors");
    }
    
    if (options.assembly_output_path.has_value()) {
        std::ofstream asm_file(options.assembly_output_path.value());
        if (asm_file) {
            asm_file << context.assembly;
        }
    }
    
    auto end_time = std::chrono::steady_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    if (context.diagnostic->get_root_logger()) {
        context.diagnostic->get_root_logger()->info("Compilation completed in {} ms.", elapsed_ms);
    }
    
    return std::move(context.bytecode);
}
} // namespace solix
