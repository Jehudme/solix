#include "solixlib_test_helper.hpp"

using namespace solix::test;

TEST_CASE("Standard Library - solix.core.Primitives & Types", "[solixlib][primitives]") {
    SECTION("Case 4.1: Boxed Int operations and bitwise utilities") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.core.Int;
            import solix.core.String;

            class Main {
                public static int32 main() {
                    Int num = new Int(42);
                    if (num.value() != 42) return 1;
                    if (num.hash_code() != 42) return 2;
                    if (!num.to_string().equals(new String("42"))) return 3;

                    Int other = new Int(42);
                    if (!num.equals(other)) return 4;
                    if (num.compare_to(other) != 0) return 5;

                    Int smaller = new Int(10);
                    if (num.compare_to(smaller) <= 0) return 6;
                    if (smaller.compare_to(num) >= 0) return 7;

                    // Bitwise utilities
                    if (Int.count_leading_zeros(1) != 31) return 8;
                    if (Int.count_trailing_zeros(8) != 3) return 9;
                    if (Int.bit_count(7) != 3) return 10;
                    if (Int.reverse_bytes(0x12345678) != 0x78563412) return 11;

                    // Parsing
                    int32 parsed = Int.parse(new String("12345"));
                    if (parsed != 12345) return 12;

                    Int out_box = new Int(0);
                    if (!Int.try_parse(new String("999"), out_box)) return 13;
                    if (out_box.value() != 999) return 14;

                    if (Int.try_parse(new String("not_a_number"), out_box)) return 15;

                    return 0;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 4.2: Boxed Double operations and IEEE 754 constants") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.core.Double;
            import solix.core.String;

            class Main {
                public static int32 main() {
                    Double d = new Double(3.14);
                    if (d.value() < 3.13 || d.value() > 3.15) return 1;

                    Double d2 = new Double(3.14);
                    if (!d.equals(d2)) return 2;
                    if (d.compare_to(d2) != 0) return 3;

                    Double bigger = new Double(5.0);
                    if (d.compare_to(bigger) >= 0) return 4;

                    // IEEE 754 queries
                    if (!Double.is_nan(Double.NaN)) return 5;
                    if (Double.is_nan(1.23)) return 6;

                    if (!Double.is_infinite(Double.POSITIVE_INFINITY)) return 7;
                    if (!Double.is_infinite(Double.NEGATIVE_INFINITY)) return 8;
                    if (Double.is_infinite(0.0)) return 9;

                    // Parsing
                    float64 parsed = Double.parse(new String("2.718"));
                    if (parsed < 2.717 || parsed > 2.719) return 10;

                    return 0;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 4.3: Boxed Bool and Char operations") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.core.Bool;
            import solix.core.Char;
            import solix.core.String;

            class Main {
                public static int32 main() {
                    // Bool
                    Bool b_true = new Bool(true);
                    Bool b_false = new Bool(false);
                    if (!b_true.value()) return 1;
                    if (b_false.value()) return 2;
                    if (b_true.equals(b_false)) return 3;
                    if (b_true.compare_to(b_false) <= 0) return 4;
                    if (!Bool.parse(new String("true"))) return 5;
                    if (Bool.parse(new String("false"))) return 6;

                    // Char
                    Char ch = new Char('A');
                    if (ch.value() != 'A') return 7;
                    if (ch.hash_code() != 65) return 8;
                    if (!Char.is_letter('Z')) return 9;
                    if (Char.is_letter('1')) return 10;
                    if (!Char.is_digit('9')) return 11;
                    if (Char.is_digit('x')) return 12;
                    if (!Char.is_whitespace(' ')) return 13;
                    if (!Char.is_upper_case('M')) return 14;
                    if (!Char.is_lower_case('m')) return 15;
                    if (Char.to_lower_case('G') != 'g') return 16;
                    if (Char.to_upper_case('k') != 'K') return 17;

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

    SECTION("Case 4.6: Any dynamic container boxing and unwrapping") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.core.Any;
            import solix.core.String;

            class Main {
                public static int32 main() {
                    Any any_int = Any.from_int(42);
                    if (!any_int.is_int()) return 1;
                    if (any_int.as_int() != 42) return 2;
                    if (!any_int.type_name().equals(new String("int32"))) return 3;

                    Any any_str = Any.from_string(new String("Hello"));
                    if (!any_str.is_string()) return 4;
                    if (!any_str.as_string().equals(new String("Hello"))) return 5;
                    if (!any_str.type_name().equals(new String("String"))) return 6;

                    Any any_bool = Any.from_bool(true);
                    if (!any_bool.is_bool()) return 7;
                    if (!any_bool.as_bool()) return 8;

                    Any any_dbl = Any.from_double(1.5);
                    if (!any_dbl.is_double()) return 9;
                    if (any_dbl.as_double() < 1.4 || any_dbl.as_double() > 1.6) return 10;

                    Any any_ch = Any.from_char('Z');
                    if (!any_ch.is_char()) return 11;
                    if (any_ch.as_char() != 'Z') return 12;

                    // Equality
                    Any same_int = Any.from_int(42);
                    if (!any_int.equals(same_int)) return 13;
                    if (any_int.equals(any_str)) return 14;

                    return 0;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 4.7: Negative: Any invalid unwrapping throws InvalidOperationException") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.core.Any;
            import solix.exceptions.InvalidOperationException;

            class Main {
                public static int32 main() {
                    Any any_int = Any.from_int(99);
                    try {
                        String s = any_int.as_string();
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

    SECTION("Case 4.8: Negative: Int.parse and Double.parse throw FormatException") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.core.Int;
            import solix.core.Double;
            import solix.core.String;
            import solix.exceptions.FormatException;

            class Main {
                public static int32 main() {
                    bool int_caught = false;
                    try {
                        Int.parse(new String("invalid_number"));
                    } catch (FormatException e) {
                        int_caught = true;
                    }

                    if (!int_caught) return 1;

                    bool dbl_caught = false;
                    try {
                        Double.parse(new String("bad_double"));
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
}
