#pragma once
#include "solix/processes/process.hpp"
namespace solix {
struct Lexer : public CompilationProcess {
    using CompilationProcess::CompilationProcess;
    void execute() override;
};
}
