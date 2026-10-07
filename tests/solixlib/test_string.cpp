#include "solixlib_test_helper.hpp"

using namespace solix::test;

TEST_CASE("Standard Library - solix.core.String & StringBuilder", "[solixlib][string]") {
    SECTION("Case 3.1: Bidirectional conversions (String to/from primitives)") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.core.String;

            class Main {
                public static int32 main() {
                    String s_int = String.from_int(42);
                    if (!s_int.equals(new String("42"))) return 1;

                    int32 parsed_int = String.to_int(s_int);
                    if (parsed_int != 42) return 2;

                    String s_bool = String.from_bool(true);
                    if (!s_bool.equals(new String("true"))) return 3;
                    if (!String.to_bool(s_bool)) return 4;

                    String s_dbl = String.from_double(3.14, 2);
                    if (!s_dbl.equals(new String("3.14"))) return 5;
                    float64 parsed_dbl = String.to_double(s_dbl);
                    if (parsed_dbl < 3.13 || parsed_dbl > 3.15) return 6;

                    return 0;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 3.2: Formatting, joining, and padding") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.core.String;

            class Main {
                public static int32 main() {
                    // Join
                    String[] words = new String[3];
                    words[0] = new String("Apple");
                    words[1] = new String("Banana");
                    words[2] = new String("Cherry");
                    String joined = String.join(new String(", "), words);
                    if (!joined.equals(new String("Apple, Banana, Cherry"))) return 1;

                    // Pad Left & Right
                    String base = new String("7");
                    String padded_l = base.pad_left(3, '0');
                    if (!padded_l.equals(new String("007"))) return 2;

                    String padded_r = base.pad_right(3, 'x');
                    if (!padded_r.equals(new String("7xx"))) return 3;

                    // Format
                    String[] args = new String[2];
                    args[0] = new String("World");
                    args[1] = new String("Solix");
                    String templ = new String("Hello {0} from {1}!");
                    String formatted = String.format(templ, args);
                    if (!formatted.equals(new String("Hello World from Solix!"))) return 4;

                    return 0;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 3.3: Slicing, trimming, transformations, repeat, reverse, and replace") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.core.String;

            class Main {
                public static int32 main() {
                    String str = new String("  Hello, Beautiful Solix!  ");
                    String trimmed = str.trim();
                    if (!trimmed.equals(new String("Hello, Beautiful Solix!"))) return 1;

                    String sub = trimmed.substring(7, 9);
                    if (!sub.equals(new String("Beautiful"))) return 2;

                    if (!trimmed.starts_with(new String("Hello"))) return 3;
                    if (!trimmed.ends_with(new String("Solix!"))) return 4;
                    if (!trimmed.contains(new String("Beautiful"))) return 5;

                    String lower = trimmed.to_lower();
                    if (!lower.equals(new String("hello, beautiful solix!"))) return 6;

                    String upper = trimmed.to_upper();
                    if (!upper.equals(new String("HELLO, BEAUTIFUL SOLIX!"))) return 7;

                    String replaced = trimmed.replace(new String("Beautiful"), new String("Awesome"));
                    if (!replaced.equals(new String("Hello, Awesome Solix!"))) return 8;

                    String rev = new String("abc").reverse();
                    if (!rev.equals(new String("cba"))) return 9;

                    String rep = new String("ha").repeat(3);
                    if (!rep.equals(new String("hahaha"))) return 10;

                    return 0;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 3.4: StringBuilder mutations and capacity growth") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.core.String;
            import solix.core.StringBuilder;

            class Main {
                public static int32 main() {
                    StringBuilder sb = new StringBuilder();
                    sb.append(new String("Hello"));
                    sb.append_char(' ');
                    sb.append(new String("World"));
                    sb.append_char(' ');
                    sb.append_int(2026);

                    String res = sb.to_string();
                    if (!res.equals(new String("Hello World 2026"))) return 1;

                    sb.insert(5, new String(","));
                    if (!sb.to_string().equals(new String("Hello, World 2026"))) return 2;

                    sb.remove(0, 7); // removes "Hello, "
                    if (!sb.to_string().equals(new String("World 2026"))) return 3;

                    return 0;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 3.5: Console printing String and IStringable") {
        StreamRedirectGuard guard;
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.core.String;
            import solix.core.StringBuilder;
            import solix.system.Console;

            class Main {
                public static int32 main() {
                    String str = new String("Printed via String overload");
                    Console.println(str);

                    StringBuilder sb = new StringBuilder();
                    sb.append(new String("Printed via IStringable overload"));
                    Console.println(sb);
                    return 0;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
        std::string out = guard.cout_stream.str();
        CHECK(out.find("Printed via String overload\n") != std::string::npos);
        CHECK(out.find("Printed via IStringable overload\n") != std::string::npos);
    }

    SECTION("Case 3.6: Console.input() reading String from stdin") {
        StreamRedirectGuard guard("Read full string line\n");
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.core.String;
            import solix.system.Console;

            class Main {
                public static int32 main() {
                    String line = Console.input();
                    if (line.equals(new String("Read full string line"))) return 0;
                    return 1;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 3.7 [Negative]: String.to_int throws FormatException on invalid input") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.core.String;
            import solix.exceptions.*;

            class Main {
                public static int32 main() {
                    try {
                        int32 v = String.to_int(new String("not_a_number"));
                        return 1;
                    } catch (FormatException fe) {
                        return 0;
                    }
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 3.8 [Negative]: String.char_at out of bounds throws IndexOutOfBoundsException") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.core.String;
            import solix.exceptions.*;

            class Main {
                public static int32 main() {
                    try {
                        String s = new String("abc");
                        char c = s.char_at(5);
                        return 1;
                    } catch (IndexOutOfBoundsException ioobe) {
                        return 0;
                    }
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 3.9 [Negative]: String.substring negative or overflow throws IndexOutOfBoundsException") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.core.String;
            import solix.exceptions.*;

            class Main {
                public static int32 main() {
                    try {
                        String s = new String("hello");
                        String sub = s.substring(2, 10); // out of range
                        return 1;
                    } catch (IndexOutOfBoundsException ioobe) {
                        return 0;
                    }
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 3.10: Primitive conversion constructors for String") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.core.String;

            class Main {
                public static int32 main() {
                    String sInt = new String(42);
                    if (!sInt.equals(new String("42"))) return 1;

                    String sLong = new String(1234567890123L);
                    if (!sLong.equals(new String("1234567890123"))) return 2;

                    String sBoolT = new String(true);
                    if (!sBoolT.equals(new String("true"))) return 3;

                    String sBoolF = new String(false);
                    if (!sBoolF.equals(new String("false"))) return 4;

                    String sChar = new String('X');
                    if (!sChar.equals(new String("X"))) return 5;

                    String sFloat = new String(3.14);
                    if (!sFloat.starts_with(new String("3.14"))) return 6;

                    return 0;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }
}
