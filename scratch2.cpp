#include "solix/compiler.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

using namespace solix;

int main() {
    std::ifstream file("tests/resources/test.slx");
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();
    
    std::cout << "Source size: " << source.size() << std::endl;

    compiler::Compiler compiler;
    try {
        std::cout << "Compiling..." << std::endl;
        std::vector<uint8_t> program = compiler.compile(source, "main");
        std::cout << "Disassembling..." << std::endl;
        std::cout << "Disassembled program:\n" << compiler.disassemble(program) << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Compilation failed: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown error!" << std::endl;
    }
    std::cout << "Done." << std::endl;
    return 0;
}
