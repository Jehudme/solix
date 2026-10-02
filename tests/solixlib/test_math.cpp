#include "solixlib_test_helper.hpp"

using namespace solix::test;

TEST_CASE("Standard Library - solix.math.Math & Random", "[solixlib][math]") {
    SECTION("Case 5.1: Mathematical constants & basic functions") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.math.Math;

            class Main {
                public static int32 main() {
                    // Constants
                    if (Math.PI < 3.14 || Math.PI > 3.15) return 1;
                    if (Math.E < 2.71 || Math.E > 2.72) return 2;
                    if (Math.TAU < 6.28 || Math.TAU > 6.29) return 3;

                    // Abs
                    if (Math.abs(-42) != 42) return 4;
                    if (Math.abs(100) != 100) return 5;
                    if (Math.abs(-3.5) != 3.5) return 6;

                    // Min / Max
                    if (Math.min(10, 20) != 10) return 7;
                    if (Math.max(10, 20) != 20) return 8;
                    if (Math.min(1.5, 2.5) != 1.5) return 9;
                    if (Math.max(1.5, 2.5) != 2.5) return 10;

                    // Clamp
                    if (Math.clamp(5, 10, 20) != 10) return 11;
                    if (Math.clamp(25, 10, 20) != 20) return 12;
                    if (Math.clamp(15, 10, 20) != 15) return 13;

                    // Sign
                    if (Math.sign(-50) != -1) return 14;
                    if (Math.sign(50) != 1) return 15;
                    if (Math.sign(0) != 0) return 16;

                    // Copy sign
                    if (Math.copy_sign(5.0, -1.0) != -5.0) return 17;
                    if (Math.copy_sign(-5.0, 1.0) != 5.0) return 18;

                    return 0;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 5.2: Exponential, power, and logarithmic functions") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.math.Math;

            class Main {
                public static int32 main() {
                    // Sqrt & Cbrt
                    float64 sq = Math.sqrt(16.0);
                    if (sq < 3.99 || sq > 4.01) return 1;

                    float64 cb = Math.cbrt(27.0);
                    if (cb < 2.99 || cb > 3.01) return 2;

                    // Hypot
                    float64 hyp = Math.hypot(3.0, 4.0);
                    if (hyp < 4.99 || hyp > 5.01) return 3;

                    // Pow
                    float64 p = Math.pow(2.0, 8.0);
                    if (p < 255.9 || p > 256.1) return 4;

                    // Exp & Log
                    float64 ex = Math.exp(1.0);
                    if (ex < 2.71 || ex > 2.72) return 5;

                    float64 lg = Math.log(Math.E);
                    if (lg < 0.99 || lg > 1.01) return 6;

                    float64 lg10 = Math.log10(100.0);
                    if (lg10 < 1.99 || lg10 > 2.01) return 7;

                    float64 lg2 = Math.log2(8.0);
                    if (lg2 < 2.99 || lg2 > 3.01) return 8;

                    return 0;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 5.3: Trigonometric and rounding functions") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.math.Math;

            class Main {
                public static int32 main() {
                    // Trig
                    float64 s0 = Math.sin(0.0);
                    if (s0 < -0.01 || s0 > 0.01) return 1;

                    float64 c0 = Math.cos(0.0);
                    if (c0 < 0.99 || c0 > 1.01) return 2;

                    // Radians & degrees
                    float64 rad = Math.to_radians(180.0);
                    if (rad < 3.14 || rad > 3.15) return 3;

                    float64 deg = Math.to_degrees(Math.PI);
                    if (deg < 179.9 || deg > 180.1) return 4;

                    // Rounding
                    if (Math.floor(3.7) != 3.0) return 5;
                    if (Math.ceil(3.2) != 4.0) return 6;
                    if (Math.round(3.5) != 4.0) return 7;
                    if (Math.trunc(-3.7) != -3.0) return 8;

                    return 0;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 5.4: PRNG determinism with seed and distribution") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.math.Random;

            class Main {
                public static int32 main() {
                    // Deterministic seed
                    Random rng1 = new Random(123456789L);
                    Random rng2 = new Random(123456789L);

                    int64 r1_1 = rng1.next_int64();
                    int64 r2_1 = rng2.next_int64();
                    if (r1_1 != r2_1) return 1;

                    int64 r1_2 = rng1.next_int64();
                    int64 r2_2 = rng2.next_int64();
                    if (r1_2 != r2_2) return 2;

                    // Range checks
                    int32 bounded = rng1.next_int(50);
                    if (bounded < 0 || bounded >= 50) return 3;

                    int32 ranged = rng1.next_int(100, 200);
                    if (ranged < 100 || ranged >= 200) return 4;

                    float64 d = rng1.next_double();
                    if (d < 0.0 || d > 1.0) return 5;

                    return 0;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 5.5: Negative: Random bounds validation throws IllegalArgumentException") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.math.Random;
            import solix.exceptions.IllegalArgumentException;

            class Main {
                public static int32 main() {
                    Random rng = new Random();

                    bool caught_zero = false;
                    try {
                        rng.next_int(0);
                    } catch (IllegalArgumentException e) {
                        caught_zero = true;
                    }
                    if (!caught_zero) return 1;

                    bool caught_neg = false;
                    try {
                        rng.next_int(-10);
                    } catch (IllegalArgumentException e) {
                        caught_neg = true;
                    }
                    if (!caught_neg) return 2;

                    bool caught_range = false;
                    try {
                        rng.next_int(100, 50); // min > max
                    } catch (IllegalArgumentException e) {
                        caught_range = true;
                    }
                    if (!caught_range) return 3;

                    return 0;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 5.6: Generic Math operations across numeric types (int32, int64, float64)") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.math.Math;

            class Main {
                public static int32 main() {
                    // abs<T>
                    int32 a_i = Math.abs<int32>(-15);
                    if (a_i != 15) return 1;
                    int64 a_l = Math.abs<int64>(-10000000000);
                    if (a_l != 10000000000) return 2;
                    float64 a_f = Math.abs<float64>(-3.5);
                    if (a_f != 3.5) return 3;

                    // min<T> & max<T>
                    int32 min_i = Math.min<int32>(25, 12);
                    if (min_i != 12) return 4;
                    int64 max_l = Math.max<int64>(500, 1000);
                    if (max_l != 1000) return 5;
                    float64 min_f = Math.min<float64>(2.71, 3.14);
                    if (min_f != 2.71) return 6;

                    // clamp<T>
                    int32 c_i = Math.clamp<int32>(15, 0, 10);
                    if (c_i != 10) return 7;
                    int64 c_l = Math.clamp<int64>(50, 10, 100);
                    if (c_l != 50) return 8;
                    float64 c_f = Math.clamp<float64>(-0.5, 0.0, 1.0);
                    if (c_f != 0.0) return 9;

                    // sign<T>
                    if (Math.sign<int32>(-99) != -1) return 10;
                    if (Math.sign<int64>(9999999999) != 1) return 11;
                    if (Math.sign<float64>(0.0) != 0) return 12;

                    // copy_sign<T>
                    int32 cs_i = Math.copy_sign<int32>(42, -1);
                    if (cs_i != -42) return 13;
                    int64 cs_l = Math.copy_sign<int64>(100, -5);
                    if (cs_l != -100) return 14;
                    float64 cs_f = Math.copy_sign<float64>(9.5, -1.0);
                    if (cs_f != -9.5) return 15;

                    return 0;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }
}
