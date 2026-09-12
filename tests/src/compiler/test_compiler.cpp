#include <catch2/catch_test_macros.hpp>
#include "solix/compiler.hpp"
#include <iostream>

using namespace solix;
using namespace solix::compiler;

TEST_CASE("Compiler: Boot Sequence & Main Call", "[compiler]") {
    std::string code = R"(
        package test;
        class Program {
            public static int32 global_var = 42;
            
            public static void main() {
                int32 a = 10;
                int32 b = a + global_var;
            }
        }
    )";
    
    Compiler compiler;
    REQUIRE_NOTHROW(compiler.compile(code, "main"));
    
    auto bytecode = compiler.compile(code, "main");
    std::string asm_code = compiler.disassemble(bytecode);
    
    // We expect ALLOC_STATIC and SET_GLOBAL for global_var
    REQUIRE(asm_code.find("ALLOC_STATIC") != std::string::npos);
    REQUIRE(asm_code.find("SET_GLOBAL") != std::string::npos);
    REQUIRE(asm_code.find("CALL") != std::string::npos);
    REQUIRE(asm_code.find("HALT") != std::string::npos);
}

TEST_CASE("Compiler: Complex test.asm.slx Compilation", "[compiler]") {
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
            public static void main() {
                Node head = new Node(1);
                Node second = new Node(2);
                head.next = second;
            }
        }
    )";
    
    Compiler compiler;
    REQUIRE_NOTHROW(compiler.compile(code, "main"));
    
    auto bytecode = compiler.compile(code, "main");
    std::string asm_code = compiler.disassemble(bytecode);
    
    // We expect CALL, INC_REF, SET_LOCAL
    REQUIRE(asm_code.find("CALL") != std::string::npos);
}
