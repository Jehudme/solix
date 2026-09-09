#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include "solix/parser.hpp"
#include <regex>

using namespace solix;

TEST_CASE("Parser: Comprehensive Symbol Population and UUID Generation", "[parser][symbols]") {
    std::string source = R"(
        package com.solix.advanced.test;

        alias Matrix = array array float64;
        alias ID = uint64;

        public enum ApplicationState {
            INITIALIZING,
            RUNNING,
            CRASHED,
            SHUTDOWN
        }

        public class Engine {
            private const string engineName = "Solix V8";
            protected static int32 instanceCount = 0;
            internal bool isRunning = false;
            public float32 temperature = 25.0;

            private class CoreProcessor {
                private ID coreId;
                public array int32 cache = 1;

                public CoreProcessor(ID id) {
                    this.coreId = id;
                    Engine.instanceCount++;
                }

                public enum CoreStatus {
                    IDLE,
                    PROCESSING,
                    OVERHEATING
                }

                public CoreStatus currentStatus = CoreStatus.IDLE;

                public const inline void process(array int32 data) {
                    if (this.currentStatus == CoreStatus.OVERHEATING) {
                        return;
                    }
                    
                    int32 temp = 0; 
                    
                    for (int32 i = 0; i < 1024; i = i + 1) {
                        int32 temp = data[i] * 2; 
                        
                        if (temp > 100) {
                            while (temp > 0) {
                                temp = temp - 1;
                                if (temp == 50) break;
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

            public static Matrix createIdentityMatrix(int32 size) {
                Matrix mat = 1;
                for (int32 i = 0; i < size; i = i + 1) {
                    mat[i] = 1;
                    for (int32 j = 0; j < size; j = j + 1) {
                        mat[i][j] = 1.0;
                    }
                }
                return mat;
            }

            public static void main(array string args) {
                ApplicationState state = ApplicationState.INITIALIZING;
                
                int32 x = 10;
                int32 y = 20;
                int32 z = 30;

                x = y + z * 2 - 10 / 5;

                bool flag = false;
                bool result = flag = !true;

                array int32 numbers = 1;
                numbers[0] = numbers[0] + 1;
                
                Engine myEngine = 1;
                Engine.CoreProcessor proc = 1;
                proc.process(numbers);

                float64 casted = 1.0;

                switch (state) {
                    case ApplicationState.INITIALIZING:
                        state = ApplicationState.RUNNING;
                        break;
                    case ApplicationState.CRASHED:
                    case ApplicationState.SHUTDOWN:
                        return;
                    default:
                        break;
                }

                do {
                    x = x + 1;
                } while (x < 100);
            }
        }
    )";

    parser::AstTree tree;
    REQUIRE_NOTHROW(tree.include(std::string_view(source)));

    // Verify symbols were populated correctly
    REQUIRE(tree.symbols.count("com.solix.advanced.test.Matrix") == 1);
    REQUIRE(tree.symbols.count("com.solix.advanced.test.ApplicationState") == 1);
    REQUIRE(tree.symbols.count("com.solix.advanced.test.Engine") == 1);
    REQUIRE(tree.symbols.count("com.solix.advanced.test.Engine.CoreProcessor") == 1);
    REQUIRE(tree.symbols.count("com.solix.advanced.test.Engine.engineName") == 1);
    REQUIRE(tree.symbols.count("com.solix.advanced.test.Engine.instanceCount") == 1);
    REQUIRE(tree.symbols.count("com.solix.advanced.test.Engine.CoreProcessor.CoreStatus") == 1);

    // Verify method and constructor UUIDs
    int main_method_count = 0;
    int process_method_count = 0;
    int outer_temp_count = 0;
    int inner_temp_count = 0;
    
    // We can't know the exact UUIDs, so we iterate through all keys
    for (const auto& [key, node] : tree.symbols) {
        if (key.find("com.solix.advanced.test.Engine.main<") != std::string::npos) main_method_count++;
        if (key.find("com.solix.advanced.test.Engine.CoreProcessor.process<") != std::string::npos) process_method_count++;
        
        // Track the variable 'temp' inside process method
        if (key.find(".process.") != std::string::npos && key.find(".temp<") != std::string::npos) {
            outer_temp_count++; // Just counts all 'temp' variables
        }
    }
    
    REQUIRE(main_method_count == 1);
    REQUIRE(process_method_count == 1);
    REQUIRE(outer_temp_count == 2); // There should be exactly two 'temp' variables because of block shadowing!
}
