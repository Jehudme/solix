#include "solix/compilation.hpp"
#include "solix/processes/assembler.hpp"
#include "solix/processes/binder.hpp"
#include "solix/processes/lexer.hpp"
#include "solix/processes/parser.hpp"
#include "solix/statements.hpp"
#include "solix/utilities/diagnostic.hpp"
#include "solix/utilities/token.hpp"

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
    
    return std::move(context.bytecode);
}
} // namespace solix
