# `solix.core` Package

The `solix.core` package forms the fundamental runtime library of Solix. It provides primitive boxing, string manipulation, monadic error containers, exception hierarchies, and essential algorithms.

## Classes & Types

- [`String`](String.md) — Immutable UTF-8 character string
- [`StringBuilder`](StringBuilder.md) — Mutable buffer for efficient dynamic string creation
- [`Exceptions`](Exceptions.md) — Solix exception hierarchy (`Exception`, `RuntimeException`, `NullPointerException`, etc.)
- [`Optional<T>`](Optional.md) — Monadic container for optional values without null
- [`Result<T, E>`](Result.md) — Monadic value-or-error container
- [`Arrays`](Arrays.md) — Algorithms for sorting, searching, copying, and filling arrays
- [`Objects`](Objects.md) — Null safety, universal hashing, and equality utilities
