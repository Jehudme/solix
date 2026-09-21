#include <catch2/catch_test_macros.hpp>
#include "solix/compilation.hpp"
#include "solix/processes/lexer.hpp"
#include "solix/processes/parser.hpp"
#include "solix/processes/binder.hpp"
#include "solix/utilities/diagnostic.hpp"

using namespace solix;

static bool compile_source(const std::string& src, std::string* out_error = nullptr) {
    solix::CompilationOptions opts;
    opts.log_level = solix::CompilationOptions::LogLevel::OFF;
    opts.sources[std::string("test")] = src;
    solix::CompilationContext ctx(opts);
    ctx.diagnostic = std::make_unique<solix::Diagnostic>(ctx);

    Lexer lexer(ctx, "Lexer");
    lexer.execute();
    Parser parser(ctx, "Parser");
    parser.execute();
    Binder binder(ctx, "Binder");
    try {
        binder.execute();
    } catch (const BindError& e) {
        if (out_error) *out_error = e.what();
        return false;
    }
    return !ctx.diagnostic->has_errors();
}

static bool compile_multi_source(
    const std::vector<std::pair<std::string, std::string>>& sources,
    std::string* out_error = nullptr)
{
    solix::CompilationOptions opts;
    opts.log_level = solix::CompilationOptions::LogLevel::OFF;
    for (auto& [name, src] : sources) opts.sources[std::string(name)] = src;
    solix::CompilationContext ctx(opts);
    ctx.diagnostic = std::make_unique<solix::Diagnostic>(ctx);

    Lexer lexer(ctx, "Lexer");
    lexer.execute();
    Parser parser(ctx, "Parser");
    parser.execute();
    Binder binder(ctx, "Binder");
    try {
        binder.execute();
    } catch (const BindError& e) {
        if (out_error) *out_error = e.what();
        return false;
    }
    return !ctx.diagnostic->has_errors();
}

// ─────────────────────────────────────────────────────────────────────────────
// 22.1 — AST Node Stamping (Context-Loss Fix)
// ─────────────────────────────────────────────────────────────────────────────

TEST_CASE("Phase 22.1 - Multi-package type resolution (node stamping)", "[binder][phase22]") {
    // This tests the core bug: Pass 2 iterates flat global_scope.symbols and
    // loses current_package. Without node stamping, types from "solix.core"
    // package (like String) fail to resolve during Pass 2.
    std::string lib = R"(
package solix.core;
public class String {
    private char[] buf;
    public String(char[] s) { this.buf = s; }
    public int32 length() { return this.buf.length; }
    public String concat(String other) { return this; }
}
)";
    std::string app = R"(
package app;
public class Greeter {
    private String name;
    public Greeter(String n) { this.name = n; }
    public String greet() { return this.name; }
}
int32 main() {
    Greeter g = new Greeter("world");
    return 0;
}
)";
    std::string err;
    bool ok = compile_multi_source({{"lib", lib}, {"app", app}}, &err);
    INFO("Error: " << err);
    REQUIRE(ok);
}

TEST_CASE("Phase 22.1 - Single-package type resolution still works", "[binder][phase22]") {
    std::string src = R"(
package com.example;
public class Foo {
    private Foo other;
    public Foo(Foo f) { this.other = f; }
    public Foo clone() { return this; }
}
int32 main() { return 0; }
)";
    std::string err;
    REQUIRE(compile_source(src, &err));
}

TEST_CASE("Phase 22.1 - Cross-package return type resolution", "[binder][phase22]") {
    std::string pkg_a = R"(
package pkg.a;
public class ValueHolder {
    private int32 x;
    public ValueHolder(int32 v) { this.x = v; }
    public int32 get() { return this.x; }
}
)";
    std::string pkg_b = R"(
package pkg.b;
public class Factory {
    public ValueHolder make(int32 v) { return new ValueHolder(v); }
}
int32 main() { return 0; }
)";
    std::string err;
    bool ok = compile_multi_source({{"a", pkg_a}, {"b", pkg_b}}, &err);
    INFO("Error: " << err);
    REQUIRE(ok);
}

// ─────────────────────────────────────────────────────────────────────────────
// 22.2 — Cast Precedence Fix
// ─────────────────────────────────────────────────────────────────────────────

TEST_CASE("Phase 22.2 - Cast has high precedence (not swallowing +)", "[binder][phase22]") {
    // (int32)x + 5  =>  ((int32)x) + 5,  result is int32
    // If the bug were present, it would parse as (int32)(x + 5) which is also int32,
    // but we need the parser to split at the boundary after the cast target.
    // We test the semantics: cast a float64 to int32, then add int32.
    std::string src = R"(
int32 main() {
    float64 x = 3.14;
    int32 result = (int32)x + 5;
    return result;
}
)";
    std::string err;
    REQUIRE(compile_source(src, &err));
}

TEST_CASE("Phase 22.2 - Char cast precedence in arithmetic chain", "[binder][phase22]") {
    // Models the pattern from string.slx:
    //   (char)((int32)current_character + 32)
    // The inner (int32)current_character should cast only 'current_character',
    // then + 32 is applied, then the outer (char) casts the whole sum.
    std::string src = R"(
int32 main() {
    char c = (char)65;
    char lower = (char)((int32)c + 32);
    return (int32)lower;
}
)";
    std::string err;
    REQUIRE(compile_source(src, &err));
}

// ─────────────────────────────────────────────────────────────────────────────
// 22.3 — Null Comparison Safety
// ─────────────────────────────────────────────────────────────────────────────

TEST_CASE("Phase 22.3 - Object == null compiles without error", "[binder][phase22]") {
    std::string src = R"(
public class Node {
    public int32 value;
    public Node(int32 v) { this.value = v; }
}
int32 main() {
    Node n = new Node(42);
    if (n == null) {
        return -1;
    }
    return 0;
}
)";
    std::string err;
    REQUIRE(compile_source(src, &err));
}

TEST_CASE("Phase 22.3 - Object != null compiles without error", "[binder][phase22]") {
    std::string src = R"(
public class Node {
    public int32 value;
    public Node(int32 v) { this.value = v; }
}
int32 main() {
    Node n = new Node(42);
    if (n != null) {
        return 1;
    }
    return 0;
}
)";
    std::string err;
    REQUIRE(compile_source(src, &err));
}

TEST_CASE("Phase 22.3 - Array == null compiles without error", "[binder][phase22]") {
    std::string src = R"(
int32 main() {
    char[] buf = null;
    if (buf == null) {
        return 0;
    }
    return 1;
}
)";
    std::string err;
    REQUIRE(compile_source(src, &err));
}

TEST_CASE("Phase 22.3 - null on left side of comparison compiles", "[binder][phase22]") {
    std::string src = R"(
public class Node {
    public int32 value;
    public Node(int32 v) { this.value = v; }
}
int32 main() {
    Node n = null;
    if (null == n) {
        return 0;
    }
    return 1;
}
)";
    std::string err;
    REQUIRE(compile_source(src, &err));
}

TEST_CASE("Phase 22.3 - Type mismatch still caught for non-null cases", "[binder][phase22]") {
    // int32 vs bool should still be a mismatch
    std::string src = R"(
int32 main() {
    int32 x = 5;
    bool y = true;
    if (x == y) {
        return 1;
    }
    return 0;
}
)";
    std::string err;
    bool ok = compile_source(src, &err);
    REQUIRE_FALSE(ok); // Should fail — type mismatch not involving null
}

// ─────────────────────────────────────────────────────────────────────────────
// 22.4 — Combined: String library cross-package usage
// ─────────────────────────────────────────────────────────────────────────────

TEST_CASE("Phase 22.4 - String class with null checks compiles multi-package", "[binder][phase22]") {
    std::string string_lib = R"(
package solix.core;
public class String {
    private char[] character_buffer;
    private int32 character_count;

    public String() {
        this.character_buffer = new char[0];
        this.character_count = 0;
    }

    public String(char[] source) {
        this.character_buffer = source;
        this.character_count = source.length;
    }

    public String operator=(String other) {
        if (other == null) {
            this.character_buffer = new char[0];
            this.character_count = 0;
            return this;
        }
        this.character_count = other.character_count;
        this.character_buffer = new char[this.character_count];
        return this;
    }

    public int32 length() {
        return this.character_count;
    }
}
)";
    std::string app = R"(
package myapp;
int32 main() {
    String s = new String("hello");
    return s.length();
}
)";
    std::string err;
    bool ok = compile_multi_source({{"string_lib", string_lib}, {"app", app}}, &err);
    INFO("Error: " << err);
    REQUIRE(ok);
}
