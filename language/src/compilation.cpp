#include "solix/new/compilation.hpp"
#include "solix/new/processes/assembler.hpp"
#include "solix/new/processes/binder.hpp"
#include "solix/new/processes/lexer.hpp"
#include "solix/new/processes/parser.hpp"
#include "solix/new/statements.hpp"
#include "solix/new/utilities/diagnostic.hpp"
#include "solix/new/utilities/token.hpp"

namespace solix {
CompilationContext::CompilationContext(const CompilationOptions &opts)
    : options(opts) {}
CompilationContext::~CompilationContext() = default;
} // namespace solix
