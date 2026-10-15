#include "test_helper.hpp"
#include "solix/runtime.hpp"
#include <catch2/catch_all.hpp>

using namespace solix::test;

TEST_CASE("InstructionBudgeting - Deterministic Quota and Polymorphic Runtime", "[runtime][metering]") {

    SECTION("Case 1: Infinite loop halts deterministically with exit code 75 on quota exhaustion") {
        const std::string code = R"(
            public class Main {
                public static int32 main() {
                    int32 counter = 0;
                    while (true) {
                        counter++;
                    }
                    return 0;
                }
            }
        )";
        auto res = compile_source(code);
        REQUIRE(res.success);

        solix::RuntimeOptions opts;
        opts.bytecode_source = res.bytecode;
        opts.instruction_budget = 200;

        int32_t exit_code = solix::run(opts);
        REQUIRE(exit_code == 75);
    }

    SECTION("Case 2: Normal execution completes cleanly with exit code 0 under sufficient budget") {
        const std::string code = R"(
            public class Main {
                public static int32 main() {
                    int32 sum = 0;
                    for (int32 i = 0; i < 10; i++) {
                        sum += i;
                    }
                    return sum == 45 ? 0 : 1;
                }
            }
        )";
        auto res = compile_source(code);
        REQUIRE(res.success);

        solix::RuntimeOptions opts;
        opts.bytecode_source = res.bytecode;
        opts.instruction_budget = 10000;

        int32_t exit_code = solix::run(opts);
        REQUIRE(exit_code == 0);
    }

    SECTION("Case 3: Polymorphic RuntimeContext override via custom_context_factory") {
        class CustomRuntimeContext : public solix::RuntimeContext {
        public:
            using solix::RuntimeContext::RuntimeContext;
            void on_budget_exceeded() override {
                exit_code = 99;
            }
        };

        const std::string code = R"(
            public class Main {
                public static int32 main() {
                    while (true) {}
                    return 0;
                }
            }
        )";
        auto res = compile_source(code);
        REQUIRE(res.success);

        solix::RuntimeOptions opts;
        opts.bytecode_source = res.bytecode;
        opts.instruction_budget = 100;
        opts.custom_context_factory = [](const solix::RuntimeOptions &o) {
            return std::make_unique<CustomRuntimeContext>(o);
        };

        int32_t exit_code = solix::run(opts);
        REQUIRE(exit_code == 99);
    }
}
