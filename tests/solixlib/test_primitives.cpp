#include "solixlib_test_helper.hpp"

using namespace solix::test;

TEST_CASE("Standard Library - solix.core.Primitives & Types", "[solixlib][primitives]") {
    SECTION("Case 4.1: Primitives bitwise utilities and integer constants") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.core.Primitives;
            import solix.core.String;

            class Main {
                public static int32 main() {
                    // Integer constants
                    if (Primitives.MIN_INT != -2147483647 - 1) return 1;
                    if (Primitives.MAX_INT != 2147483647) return 2;

                    // Bitwise utilities
                    if (Primitives.count_leading_zeros(1) != 31) return 8;
                    if (Primitives.count_trailing_zeros(8) != 3) return 9;
                    if (Primitives.bit_count(7) != 3) return 10;
                    if (Primitives.reverse_bytes(0x12345678) != 0x78563412) return 11;

                    // Parsing
                    int32 parsed = Primitives.parse_int(new String("12345"));
                    if (parsed != 12345) return 12;

                    return 0;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 4.2: Primitives IEEE 754 constants and floating-point queries") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.core.Primitives;
            import solix.core.String;

            class Main {
                public static int32 main() {
                    // IEEE 754 queries
                    if (!Primitives.is_nan(Primitives.NaN)) return 5;
                    if (Primitives.is_nan(1.23)) return 6;

                    if (!Primitives.is_infinite(Primitives.POSITIVE_INFINITY)) return 7;
                    if (!Primitives.is_infinite(Primitives.NEGATIVE_INFINITY)) return 8;
                    if (Primitives.is_infinite(0.0)) return 9;

                    // Parsing
                    float64 parsed = Primitives.parse_double(new String("2.718"));
                    if (parsed < 2.717 || parsed > 2.719) return 10;

                    return 0;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 4.3: Primitives boolean and char operations") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.core.Primitives;
            import solix.core.String;

            class Main {
                public static int32 main() {
                    // Bool parse
                    if (!Primitives.parse_bool(new String("true"))) return 5;
                    if (Primitives.parse_bool(new String("false"))) return 6;

                    // Char classification and transformation
                    if (!Primitives.is_letter('Z')) return 9;
                    if (Primitives.is_letter('1')) return 10;
                    if (!Primitives.is_digit('9')) return 11;
                    if (Primitives.is_digit('x')) return 12;
                    if (!Primitives.is_whitespace(' ')) return 13;
                    if (!Primitives.is_upper_case('M')) return 14;
                    if (!Primitives.is_lower_case('m')) return 15;
                    if (Primitives.to_lower_case('G') != 'g') return 16;
                    if (Primitives.to_upper_case('k') != 'K') return 17;

                    return 0;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 4.4: Optional<T> value presence, unwrapping, and fallback") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.core.Optional;
            import solix.core.String;

            class Main {
                public static int32 main() {
                    Optional<int32> some_val = new Optional<int32>(100);
                    if (!some_val.has_value()) return 1;
                    if (some_val.is_empty()) return 2;
                    if (some_val.value() != 100) return 3;
                    if (some_val.value_or(200) != 100) return 4;

                    Optional<int32> none_val = new Optional<int32>();
                    if (none_val.has_value()) return 5;
                    if (!none_val.is_empty()) return 6;
                    if (none_val.value_or(555) != 555) return 7;

                    return 0;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 4.5: Negative: accessing Optional.empty().value() throws InvalidOperationException") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.core.Optional;
            import solix.exceptions.InvalidOperationException;

            class Main {
                public static int32 main() {
                    Optional<int32> opt = new Optional<int32>();
                    try {
                        int32 v = opt.value();
                        return 1; // Should not reach here
                    } catch (InvalidOperationException e) {
                        return 0; // Success
                    }
                    return 2;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 4.6: Optional<T> functional operations (if_present, filter)") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.core.Optional;
            import solix.core.String;

            class Cell {
                public int32 value;
                public Cell(int32 v) { this.value = v; }
            }

            class Main {
                public static int32 main() {
                    Cell c = new Cell(0);
                    Optional<int32> some = new Optional<int32>(42);
                    some.if_present([c](int32 v) => {
                        c.value = v;
                    });
                    if (c.value != 42) return 1;

                    // if_present on empty should not execute callback
                    Optional<int32> empty = new Optional<int32>();
                    empty.if_present([c](int32 v) => {
                        c.value = 999;
                    });
                    if (c.value != 42) return 2;

                    // filter matching
                    Optional<int32> filtered_pass = some.filter([](int32 v) => v > 10);
                    if (!filtered_pass.has_value()) return 3;
                    if (filtered_pass.value() != 42) return 4;

                    // filter discarding
                    Optional<int32> filtered_fail = some.filter([](int32 v) => v > 100);
                    if (filtered_fail.has_value()) return 5;
                    if (!filtered_fail.is_empty()) return 6;

                    // filter on already empty
                    Optional<int32> empty_filtered = empty.filter([](int32 v) => true);
                    if (empty_filtered.has_value()) return 7;

                    return 0;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 4.7: Negative: Primitives.parse_int and parse_double throw FormatException") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.core.Primitives;
            import solix.core.String;
            import solix.exceptions.FormatException;

            class Main {
                public static int32 main() {
                    bool int_caught = false;
                    try {
                        Primitives.parse_int(new String("invalid_number"));
                    } catch (FormatException e) {
                        int_caught = true;
                    }

                    if (!int_caught) return 1;

                    bool dbl_caught = false;
                    try {
                        Primitives.parse_double(new String("bad_double"));
                    } catch (FormatException e) {
                        dbl_caught = true;
                    }

                    if (!dbl_caught) return 2;

                    return 0;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 4.8: Primitive scalar types and parsing") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.core.Primitives;
            import solix.core.String;

            class Main {
                public static int32 main() {
                    // int8 / byte
                    int8 b = (int8)42;
                    if (b != 42) return 1;

                    // int16 / short
                    int16 s = (int16)1234;
                    if (s != 1234) return 4;

                    // int64 / long
                    int64 l = 9000000000L;
                    if (l != 9000000000L) return 7;

                    // uint8
                    uint8 ub = (uint8)200;
                    if (ub != (uint8)200) return 9;

                    // uint16
                    uint16 us = (uint16)60000;
                    if (us != (uint16)60000) return 11;

                    // uint32
                    uint32 ui = (uint32)3000000000;
                    if (ui != (uint32)3000000000) return 13;

                    // uint64
                    uint64 ul = (uint64)10000000000;
                    if (ul != (uint64)10000000000) return 14;

                    // float32
                    float32 f = (float32)3.14;
                    if (f < (float32)3.13 || f > (float32)3.15) return 15;

                    return 0;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 4.9: Operator overloading on String") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.core.String;

            class Main {
                public static int32 main() {
                    // String concatenation and equality
                    String s1 = new String("hello ");
                    String s2 = new String("world");
                    String s3 = s1 + s2;
                    if (!s3.equals_chars("hello world")) return 16;
                    if (s1 == s2) return 17;
                    if (!(s1 != s2)) return 18;

                    return 0;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 4.10: Functional parity for Optional<T> (map, flat_map)") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.core.Optional;

            class Main {
                public static int32 main() {
                    Optional<int32> opt = new Optional<int32>(21);

                    // map on present value
                    Optional<int32> doubled = opt.map<int32>([](int32 x) => x * 2);
                    if (!doubled.has_value() || doubled.value() != 42) return 1;

                    // map on empty optional
                    Optional<int32> empty = new Optional<int32>();
                    Optional<int32> empty_map = empty.map<int32>([](int32 x) => x * 2);
                    if (empty_map.has_value()) return 2;

                    // flat_map chaining
                    Optional<int32> flat = opt.flat_map<int32>([](int32 x) => new Optional<int32>(x + 10));
                    if (!flat.has_value() || flat.value() != 31) return 3;

                    // flat_map returning empty
                    Optional<int32> flat_empty = opt.flat_map<int32>([](int32 x) => new Optional<int32>());
                    if (flat_empty.has_value()) return 4;

                    // flat_map on empty
                    Optional<int32> from_empty_flat = empty.flat_map<int32>([](int32 x) => new Optional<int32>(x + 10));
                    if (from_empty_flat.has_value()) return 5;

                    return 0;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 4.11: Friendly Primitive Type Aliases via wildcard and direct import") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.core.*;

            class Main {
                public static int32 main() {
                    int i = 42;
                    long l = 100000;
                    short s = 123;
                    byte b = 10;
                    ubyte ub = 200;
                    ushort us = 50000;
                    uint ui = 3000000;
                    ulong ul = 9000000;
                    float f = 3.14;
                    double d = 6.28;

                    if (i != 42) return 1;
                    if (l != 100000) return 2;
                    if (s != 123) return 3;
                    if (b != 10) return 4;
                    if (ub != 200) return 5;
                    if (us != 50000) return 6;
                    if (ui != 3000000) return 7;
                    if (ul != 9000000) return 8;
                    if (f < (float)3.13 || f > (float)3.15) return 9;
                    if (d < 6.27 || d > 6.29) return 10;

                    return 0;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 4.12: Arithmetic and function signature typing with friendly primitive aliases") {
        auto sources = load_solixlib_sources();
        sources["math_ops.slx"] = R"(
            package test.math;
            import solix.core.*;

            public class MathOps {
                public static int add(int a, int b) {
                    return a + b;
                }

                public static double multiply(double a, double b) {
                    return a * b;
                }
            }
        )";
        sources["main.slx"] = R"(
            import test.math.MathOps;
            import solix.core.*;

            class Main {
                public static int32 main() {
                    int sum = MathOps.add(15, 27);
                    if (sum != 42) return 1;

                    double product = MathOps.multiply(2.5, 4.0);
                    if (product < 9.99 || product > 10.01) return 2;

                    return 0;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }
}
