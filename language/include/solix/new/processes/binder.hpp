#pragma once
#include "solix/new/processes/process.hpp"
namespace solix {
struct Binder : public CompilationProcess {
    using CompilationProcess::CompilationProcess;
    void execute() override {}
};
}
