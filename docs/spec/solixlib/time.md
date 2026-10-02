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

### Arithmetic & Comparison
- `plus(Duration other) -> Duration`
- `minus(Duration other) -> Duration`
- `equals(Duration other) -> bool`
- `compare_to(Duration other) -> int32`
- `is_zero() -> bool`
- `is_negative() -> bool`
- `abs() -> Duration`
- `to_string() -> String` (Formats in human-readable notation, e.g. `123ms`, `45s`, `600ns`)

---

## 2. Class: `solix.time.Instant`

Represents an instantaneous point on the monotonic timeline, backed by native OS steady clock (`CLOCK_MONOTONIC` on POSIX / `QueryPerformanceCounter` on Windows).

### Static Methods
- `Instant.now() -> Instant`: Captures current monotonic timestamp.
- `Instant.of_nanos(int64 nanos) -> Instant`: Reconstructs instant from nanosecond tick.

### Methods
- `nanos() -> int64`: Returns underlying nanosecond tick.
- `duration_until(Instant other) -> Duration`: Computes duration between `this` and `other`.
- `elapsed() -> Duration`: Computes duration elapsed from `this` until now.
- `plus(Duration dur) -> Instant`: Returns instant offset by duration into the future.
- `minus(Duration dur) -> Instant`: Returns instant offset by duration into the past.
- `is_before(Instant other) -> bool`
- `is_after(Instant other) -> bool`
- `equals(Instant other) -> bool`
- `compare_to(Instant other) -> int32`
- `to_string() -> String`

---

## 3. Class: `solix.time.DateTime`

Represents calendar date and wall-clock time decomposed into UTC or local time components.

### Static Methods
- `DateTime.utc_now() -> DateTime`: Current wall-clock time in UTC.
- `DateTime.local_now() -> DateTime`: Current wall-clock time in local time zone.
- `DateTime.of(int32 year, int32 month, int32 day, int32 hour, int32 minute, int32 second, int32 millisecond) -> DateTime`: Validated calendar constructor (throws `IllegalArgumentException` on out-of-bounds values).
- `DateTime.is_leap_year(int32 year) -> bool`: Gregorian leap year validation.
- `DateTime.sleep(Duration dur) -> void`: Suspends execution for specified duration.

### Component Getters
- `year() -> int32`
- `month() -> int32` (1–12)
- `day() -> int32` (1–31)
- `hour() -> int32` (0–23)
- `minute() -> int32` (0–59)
- `second() -> int32` (0–59)
- `millisecond() -> int32` (0–999)
- `day_of_week() -> int32` (0 = Sunday, 1 = Monday, ..., 6 = Saturday)
- `day_of_year() -> int32` (1–366)
- `is_utc() -> bool`

### Conversions & Formatting
- `to_iso8601() -> String`: Formats in ISO 8601 extended notation (`YYYY-MM-DDTHH:MM:SS.mmmZ` for UTC).
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
