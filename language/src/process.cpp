#include "solix/new/processes/process.hpp"
#include "solix/new/compilation.hpp"
#include "solix/new/processes/diagnostic.hpp"

namespace solix {

CompilationProcess::CompilationProcess(CompilationContext &ctx, std::string name) : context(ctx) {
    if (context.diagnostic) {
        logger = context.diagnostic->create_process_logger(name);
    }
}

CompilationProcess::~CompilationProcess() {}

} // namespace solix
