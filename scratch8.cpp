#include <iostream>
#include <fstream>
#include <sstream>

int main() {
    std::ifstream file("language/src/compiler.cpp");
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();
    
    std::string search = "            case OpCode::CALL: ss << \"CALL\\n\"; break;";
    size_t pos = source.find(search);
    if (pos == std::string::npos) {
        std::cout << "Not found\n";
        return 1;
    }
    
    std::string insertion = R"(            case OpCode::JUMP: {
                uint32_t jump_ip = *reinterpret_cast<const uint32_t*>(&bytecode[offset]);
                ss << "JUMP " << jump_ip << "\n";
                offset += 4;
                break;
            }
            case OpCode::JUMP_IF_FALSE: {
                uint32_t jump_ip = *reinterpret_cast<const uint32_t*>(&bytecode[offset]);
                ss << "JUMP_IF_FALSE " << jump_ip << "\n";
                offset += 4;
                break;
            }
)";
    source.insert(pos, insertion);
    std::ofstream out("language/src/compiler.cpp");
    out << source;
    std::cout << "Done insertion\n";
    return 0;
}
