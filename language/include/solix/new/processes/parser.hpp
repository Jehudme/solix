#pragma once
#include "solix/new/processes/process.hpp"
namespace solix {
struct Parser : public CompilationProcess {
    using CompilationProcess::CompilationProcess;
    void execute() override {}
};
}
