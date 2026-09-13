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
    compiler.include(std::string_view(code));
    std::vector<uint8_t> bytecode;
    REQUIRE_NOTHROW([&]() {
        bytecode = compiler.compile("main");
    }());
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
    compiler.include(std::string_view(code));
    std::vector<uint8_t> bytecode;
    REQUIRE_NOTHROW([&]() {
        bytecode = compiler.compile("main");
    }());
    std::string asm_code = compiler.disassemble(bytecode);
    
    // We expect CALL, INC_REF, SET_LOCAL
    REQUIRE(asm_code.find("CALL") != std::string::npos);
}

TEST_CASE("Compiler: test.slx Full Compilation", "[compiler]") {
    std::string code = R"(
package com.solix.advanced.test;

public enum CoreStatus {
    IDLE,
    PROCESSING,
    OVERHEATING
}

public class Engine {
    private const string engineName = "Solix V8";
    protected static int32 instanceCount = 0;
    internal bool isRunning = false;
    public float32 temperature = 25.0;

    private class CoreProcessor {
        public ID coreId;
        public int32[] cache = new int32[1024];
        public CoreStatus currentStatus = CoreStatus.IDLE;

        public CoreProcessor(ID id) {
            this.coreId = id;
            Engine.instanceCount++;
        }

        public const inline void process(int32[] data) {
            if (this.currentStatus == CoreStatus.OVERHEATING) {
                return;
            }

            int32 temp = 0;
            for (int32 i = 0; i < 1024; i++) {
                temp = data[i] * 2;

                if (temp > 100) {
                    while (temp > 0) {
                        temp--;
                        if (temp == 50) {
                            break;
                        }
                    }
                } else if (temp < 0) {
                    continue;
                } else {
                    this.cache[i] = temp;
                }
            }
        }
    }

    public Engine() {
        this.isRunning = true;
    }

    public static float64[][] createIdentityMatrix(int32 size) {
        Matrix mat = new float64[][size];
        for (int32 i = 0; i < size; i++) {
            for (int32 j = 0; j < size; j++) {
                mat[i][j] = (i == j) ? 1.0 : 0.0;
            }
        }
        return mat;
    }

    public static void main(string[] args) {
        ApplicationState state = ApplicationState.INITIALIZING;
        
        int32 x = 10;
        int32 y = 20;
        int32 z = 30;
        
        x = y + z * 2 - 10 / 5;

        bool flag = false;
        bool result = flag = !true;

        int32[] numbers = { 1, 2, 3, 4, 5 };
        numbers[0]++;
        
        Engine myEngine = new Engine();
        CoreProcessor proc = new CoreProcessor(12345);

        proc.process(numbers);

        float64 casted = (float64) x;

        float64 complexCall = Engine.createIdentityMatrix(x)[0][0] + (float64) proc.cache[0];

        switch (state) {
            case ApplicationState.CRASHED:
            case ApplicationState.SHUTDOWN:
                return;
            case ApplicationState.INITIALIZING:
                state = ApplicationState.RUNNING;
                break;
            default:
                break;
        }

        do {
            x++;
        } while (x < 100);
    }
}

public enum ApplicationState {
    INITIALIZING,
    RUNNING,
    SHUTDOWN,
    CRASHED
}

alias ID = uint64;
alias Matrix = float64[][];
)";

    Compiler compiler;
    compiler.include(std::string_view(code));
    std::vector<uint8_t> bytecode;
    REQUIRE_NOTHROW([&]() {
        bytecode = compiler.compile("main");
    }());
    std::string asm_code = compiler.disassemble(bytecode);
    
    // Check if it successfully compiled jumps and allocations
    REQUIRE(asm_code.find("JUMP") != std::string::npos);
    REQUIRE(asm_code.find("ALLOC_DYNAMIC") != std::string::npos);
    REQUIRE(asm_code.find("CALL") != std::string::npos);
}
