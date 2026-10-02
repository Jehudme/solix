#include "solixlib_test_helper.hpp"

using namespace solix::test;

TEST_CASE("Standard Library - solix.time.Chrono", "[solixlib][time]") {
    SECTION("Case 6.1: Duration conversions and arithmetic") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.time.Duration;

            class Main {
                public static int32 main() {
                    Duration d_s = Duration.from_seconds(5L);
                    if (d_s.to_seconds() != 5L) return 1;
                    if (d_s.to_milliseconds() != 5000L) return 2;
                    if (d_s.to_nanoseconds() != 5000000000L) return 3;

                    Duration d_m = Duration.from_minutes(2L);
                    if (d_m.to_seconds() != 120L) return 4;

                    Duration d_h = Duration.from_hours(1L);
                    if (d_h.to_minutes() != 60L) return 5;

                    Duration d_d = Duration.from_days(1L);
                    if (d_d.to_hours() != 24L) return 6;

                    // Addition
                    Duration sum = d_s.add(Duration.from_seconds(3L));
                    if (sum.to_seconds() != 8L) return 7;

                    // Subtraction
                    Duration diff = d_s.subtract(Duration.from_seconds(2L));
                    if (diff.to_seconds() != 3L) return 8;

                    return 0;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 6.2: Instant monotonic measurements and elapsed time") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.time.Instant;
            import solix.time.Duration;
            import solix.time.Stopwatch;

            class Main {
                public static int32 main() {
                    Instant t1 = Instant.now();
                    Stopwatch.sleep(10L); // 10 ms sleep
                    Instant t2 = Instant.now();

                    Duration elapsed = t2.duration_since(t1);
                    if (elapsed.to_nanoseconds() <= 0L) return 1;
                    if (elapsed.to_milliseconds() < 5L) return 2;

                    return 0;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 6.3: DateTime timestamp extraction and leap year validation") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.time.DateTime;

            class Main {
                public static int32 main() {
                    // Known epoch: 1700000000000 ms (2023-11-14 22:13:20 UTC)
                    DateTime dt = DateTime.from_epoch_millis(1700000000000L, true);
                    if (dt.year() != 2023) return 1;
                    if (dt.month() != 11) return 2;
                    if (dt.day() != 14) return 3;
                    if (dt.hour() != 22) return 4;
                    if (dt.minute() != 13) return 5;
                    if (dt.second() != 20) return 6;
                    if (dt.millisecond() != 0) return 7;

                    // Leap year logic
                    if (!DateTime.is_leap_year(2024)) return 8;
                    if (DateTime.is_leap_year(2023)) return 9;
                    if (DateTime.is_leap_year(1900)) return 10;
                    if (!DateTime.is_leap_year(2000)) return 11;

                    return 0;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 6.4: DateTime ISO 8601 formatting") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.time.DateTime;
            import solix.core.String;

            class Main {
                public static int32 main() {
                    DateTime dt = DateTime.from_epoch_millis(1700000000000L, true);
                    String iso = dt.to_iso8601();
                    if (!iso.equals(new String("2023-11-14T22:13:20.000Z"))) return 1;

                    return 0;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 6.5: Stopwatch lifecycle and elapsed measurement") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.time.Stopwatch;
            import solix.time.Duration;

            class Main {
                public static int32 main() {
                    Stopwatch sw = new Stopwatch();
                    if (sw.is_running()) return 1;
                    if (sw.elapsed().to_nanoseconds() != 0L) return 2;

                    sw.start();
                    if (!sw.is_running()) return 3;

                    Stopwatch.sleep(15L); // 15 ms
                    sw.stop();
                    if (sw.is_running()) return 4;

                    int64 elapsed_ms = sw.elapsed_milliseconds();
                    if (elapsed_ms < 10L) return 5;

                    sw.reset();
                    if (sw.elapsed().to_nanoseconds() != 0L) return 6;

                    return 0;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 6.6: Negative duration subtraction and comparison") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.time.Duration;

            class Main {
                public static int32 main() {
                    Duration small = Duration.from_seconds(2L);
                    Duration large = Duration.from_seconds(10L);

                    Duration negative_diff = small.subtract(large);
                    if (negative_diff.to_seconds() != -8L) return 1;

                    if (negative_diff.compare_to(Duration.from_seconds(0L)) >= 0) return 2;
                    if (large.compare_to(small) <= 0) return 3;

                    return 0;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }
}
