#pragma once
#include "processes/process.hpp"
namespace solix {
struct Lexer : public CompilationProcess {
    using CompilationProcess::CompilationProcess;
    void execute() override;
};
}
