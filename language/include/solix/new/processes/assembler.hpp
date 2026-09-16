#pragma once
#include "solix/new/processes/process.hpp"
namespace solix {
struct Assembler : public CompilationProcess {
    using CompilationProcess::CompilationProcess;
    void execute() override {}
};
}
