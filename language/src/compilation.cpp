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
    
    Lexer lexer(context, "Lexer");
    lexer.execute();
    
    Parser parser(context, "Parser");
    parser.execute();
    
    Binder binder(context, "Binder");
    binder.execute();
    
    Assembler assembler(context, "Assembler");
    assembler.execute();
    
    if (options.assembly_output_path.has_value()) {
        std::ofstream asm_file(options.assembly_output_path.value());
        if (asm_file) {
            asm_file << context.assembly;
        }
    }
    
    return std::move(context.bytecode);
}
} // namespace solix
