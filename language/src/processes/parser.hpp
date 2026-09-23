#pragma once
#include "solix/processes/process.hpp"
namespace solix {
struct Parser : public CompilationProcess {
    using CompilationProcess::CompilationProcess;
    void execute() override;
};
}
