# Solix Standard Library: Time & Chrono (`solix.time`)

## Overview

The `solix.time` module provides comprehensive, high-resolution, cross-platform time handling for Solix applications. It adheres to modern chrono separation between relative spans of time (`Duration`), monotonic timestamps (`Instant`), wall-clock calendar timestamps (`DateTime`), and execution profiling (`Stopwatch`).

---

## 1. Class: `solix.time.Duration`

Represents a signed duration of time stored at nanosecond resolution (`int64`). Implements `IComparable<Duration>`, `IEquatable<Duration>`, and `IStringable`.

### Static Factory Methods
- `Duration.of_nanos(int64 nanos) -> Duration`
- `Duration.of_micros(int64 micros) -> Duration`
- `Duration.of_millis(int64 millis) -> Duration`
- `Duration.of_seconds(int64 seconds) -> Duration`
- `Duration.of_minutes(int64 minutes) -> Duration`
- `Duration.of_hours(int64 hours) -> Duration`
- `Duration.of_days(int64 days) -> Duration`
- `Duration.zero() -> Duration`

### Accessors & Unit Conversions
- `to_nanos() -> int64`
- `to_micros() -> int64`
- `to_millis() -> int64`
- `to_seconds() -> int64`
- `to_minutes() -> int64`
- `to_hours() -> int64`
- `to_days() -> int64`

### Arithmetic, Operators & Comparison
- `add(Duration other) -> Duration`
- `subtract(Duration other) -> Duration`
- `operator+(Duration other) -> Duration`
- `operator-(Duration other) -> Duration`
- `operator*(int32 factor) -> Duration`
- `operator*(int64 factor) -> Duration`
- `operator/(int32 divisor) -> Duration`
- `operator/(int64 divisor) -> Duration`
- `equals(Duration other) -> bool`
- `operator==(Duration other) -> bool`
- `operator!=(Duration other) -> bool`
- `compare_to(Duration other) -> int32`
- `operator<(Duration other) -> bool`
- `operator<=(Duration other) -> bool`
- `operator>(Duration other) -> bool`
- `operator>=(Duration other) -> bool`
- `to_string() -> String` (Formats in human-readable notation, e.g. `123ms`)

---

## 2. Class: `solix.time.Instant`

Represents an instantaneous point on the monotonic timeline, backed by native OS steady clock (`CLOCK_MONOTONIC` on POSIX / `QueryPerformanceCounter` on Windows).

### Static Methods
- `Instant.now() -> Instant`: Captures current monotonic timestamp.
- `Instant.of_nanos(int64 nanos) -> Instant`: Reconstructs instant from nanosecond tick.

### Methods & Operators
- `to_nanoseconds() -> int64`: Returns underlying nanosecond tick.
- `duration_since(Instant earlier) -> Duration`: Computes duration between `earlier` and `this`.
- `operator-(Instant earlier) -> Duration`: Computes elapsed duration between `this` and `earlier`.
- `elapsed() -> Duration`: Computes duration elapsed from `this` until now.
- `operator+(Duration dur) -> Instant`: Returns instant offset by duration into the future.
- `operator-(Duration dur) -> Instant`: Returns instant offset by duration into the past.
- `equals(Instant other) -> bool`
- `operator==(Instant other) -> bool`
- `operator!=(Instant other) -> bool`
- `compare_to(Instant other) -> int32`
- `operator<(Instant other) -> bool`
- `operator<=(Instant other) -> bool`
- `operator>(Instant other) -> bool`
- `operator>=(Instant other) -> bool`

---

## 3. Class: `solix.time.DateTime`

Represents calendar date and wall-clock time decomposed into UTC or local time components.

### Static Methods
- `DateTime.now() -> DateTime`: Current wall-clock time in local time zone.
- `DateTime.utc_now() -> DateTime`: Current wall-clock time in UTC.
- `DateTime.from_epoch_millis(int64 ms, bool is_utc) -> DateTime`: Constructs from epoch milliseconds.
- `DateTime.is_leap_year(int32 year) -> bool`: Gregorian leap year validation.

### Component Getters
- `year() -> int32`
- `month() -> int32` (1–12)
- `day() -> int32` (1–31)
- `hour() -> int32` (0–23)
- `minute() -> int32` (0–59)
- `second() -> int32` (0–59)
- `millisecond() -> int32` (0–999)
- `to_epoch_millis() -> int64`
- `is_utc() -> bool`

### Conversions & Formatting
- `to_iso8601() -> String`: Formats in ISO 8601 extended notation (`YYYY-MM-DDTHH:MM:SS.mmmZ` for UTC).
- `format(String pattern) -> String`: Formats date using custom pattern tokens (`yyyy`, `MM`, `dd`, `HH`, `mm`, `ss`, `SSS`).
- `to_string() -> String`: Default string representation matching ISO 8601.

---

## 4. Class: `solix.time.Stopwatch`

High-resolution diagnostic stopwatch for measuring elapsed execution time.

### Methods
- `new Stopwatch()`: Instantiates stopped stopwatch.
- `start() -> void`: Resumes or starts counting.
- `stop() -> void`: Pauses counting.
- `reset() -> void`: Resets elapsed time to zero and stops.
- `restart() -> void`: Resets and immediately starts counting.
- `is_running() -> bool`: Query whether stopwatch is active.
- `elapsed() -> Duration`: Returns cumulative elapsed duration.
- `elapsed_millis() -> int64`: Shortcut for `elapsed().to_millis()`.
- `elapsed_nanos() -> int64`: Shortcut for `elapsed().to_nanos()`.
- `to_string() -> String`: Returns string representation of elapsed duration.
