#include "solix/compiler.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <file1.slx> [file2.slx ...]" << std::endl;
        return 1;
    }

    try {
        solix::compiler::Compiler compiler;
        
        for (int i = 1; i < argc; i++) {
            compiler.include(std::filesystem::path(argv[i]));
        }

        std::vector<uint8_t> bytecode = compiler.compile("main");
        
        std::string disasm = compiler.disassemble(bytecode);
        std::cout << disasm << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
