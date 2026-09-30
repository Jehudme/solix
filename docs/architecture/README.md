# Solix Compiler & VM Architecture

This directory contains technical deep dives into the internal design and mechanics of the Solix compiler pipeline and runtime virtual machine.

## Architecture Documents

- [`pipeline.md`](pipeline.md) — The complete compilation flow: Lexer $\to$ Parser $\to$ Binder $\to$ Assembler $\to$ Runtime VM
- [`binder_passes.md`](binder_passes.md) — Detailed mechanics of Binder Passes 1a, 1b, 2, and 3
- [`arc_internals.md`](arc_internals.md) — Object heap layout, Automatic Reference Counting (ARC), and cycle-breaking weak references
- [`exception_unwinding.md`](exception_unwinding.md) — Bytecode exception handling, VTable catch matching, and cleanup trampolines
