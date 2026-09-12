#include "solix/compiler.hpp"
#include <iostream>

using namespace solix;
using namespace solix::compiler;

int main() {
    std::string code = R"(
        package test;
        class Node {
            public int32 value;
            public Node next;
            
            public constructor(int32 v) {
                value = v;
            }
        }
        
        class Program {
            public static int32 global_var = 42;

            public static void main() {
                int32 a = 10;
                int32 b = a + global_var;
                Node head = new Node(b);
                Node second = new Node(2);
                head.next = second;
            }
        }
    )";
    
    Compiler compiler;
    auto bytecode = compiler.compile(code, "main");
    std::cout << compiler.disassemble(bytecode) << std::endl;
    return 0;
}
