# Solix Standard Library: Math & Random (`solix.math`)

## Overview

The `solix.math` module provides cross-platform mathematical operations, IEEE 754 constants, transcendental functions, geometric computations, and a high-performance pseudo-random number generator (`Random`) backed by standard C++ `<cmath>` and Mersenne Twister `std::mt19937_64`.

---

## 1. Class: `solix.math.Math`

A static utility class providing mathematical constants and functions.

### Constants
- `PI`: Ratio of circumference to diameter ($\approx 3.141592653589793$).
- `E`: Base of natural logarithms ($\approx 2.718281828459045$).
- `TAU`: Ratio of circumference to radius ($2\pi \approx 6.283185307179586$).
- `EPSILON`: Minimal non-zero floating-point comparison threshold ($10^{-15}$).

### Basic Arithmetic & Bounds
- `abs(int32 v) -> int32`, `abs(int64 v) -> int64`, `abs(float64 v) -> float64`: Returns absolute value.
- `min(a, b)`, `max(a, b)`: Overloaded for `int32`, `int64`, and `float64`.
- `clamp(val, min_val, max_val)`: Constrains `val` within `[min_val, max_val]`.
- `sign(v)`: Returns `-1`, `0`, or `1` depending on value sign.
- `copy_sign(magnitude, sign_val)`: Composes magnitude of first with sign of second.

### Exponential & Logarithmic
- `sqrt(float64 v) -> float64`: Square root.
- `cbrt(float64 v) -> float64`: Cube root.
- `hypot(float64 a, float64 b) -> float64`: Euclidean distance $\sqrt{a^2 + b^2}$.
- `pow(float64 b, float64 e) -> float64`: Computes $b^e$.
- `exp(float64 v) -> float64`: Exponential function $e^v$.
- `log(float64 v) -> float64`: Natural logarithm ($\ln v$).
- `log10(float64 v) -> float64`: Common logarithm ($\log_{10} v$).
- `log2(float64 v) -> float64`: Binary logarithm ($\log_2 v$).

### Trigonometric & Angular
- `sin(v)`, `cos(v)`, `tan(v)`: Standard trigonometric functions (radians).
- `asin(v)`, `acos(v)`, `atan(v)`: Inverse trigonometric functions.
- `atan2(y, x)`: Two-argument arc tangent with full quadrant resolution.
- `sinh(v)`, `cosh(v)`, `tanh(v)`: Hyperbolic functions.
- `to_radians(float64 deg) -> float64`: Converts degrees to radians.
- `to_degrees(float64 rad) -> float64`: Converts radians to degrees.

### Rounding & Truncation
- `floor(float64 v) -> float64`: Largest integer $\le v$.
- `ceil(float64 v) -> float64`: Smallest integer $\ge v$.
- `round(float64 v) -> float64`: Rounds to nearest integer.
- `trunc(float64 v) -> float64`: Truncates towards zero.

---

## 2. Class: `solix.math.Random`

Deterministic pseudo-random number generator backed by 64-bit Mersenne Twister engine.

### Constructors
- `new Random()`: Initializes generator seeded from high-resolution monotonic clock.
- `new Random(int64 seed)`: Initializes generator with fixed seed for reproducible sequences.

### Methods
- `next_int() -> int32`: Generates non-negative 32-bit random integer.
- `next_int(int32 max) -> int32`: Generates integer in `[0, max)`. Throws `IllegalArgumentException` if `max <= 0`.
- `next_int(int32 min, int32 max) -> int32`: Generates integer in `[min, max)`. Throws `IllegalArgumentException` if `min >= max`.
- `next_int64() -> int64`: Generates full 64-bit random integer.
- `next_double() -> float64`: Generates uniform floating-point value in `[0.0, 1.0)`.
- `next_bool() -> bool`: Generates pseudo-random boolean.
