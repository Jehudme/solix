# Solix Lexical Grammar Specification

This document defines the complete lexical grammar of Solix: how raw source text is tokenized into the stream of tokens consumed by the parser. It is normative — implementations must conform to these rules.

---

## Table of Contents

1. [Character Encoding](#1-character-encoding)
2. [Comments](#2-comments)
3. [Whitespace](#3-whitespace)
4. [Identifiers](#4-identifiers)
5. [Reserved Keywords](#5-reserved-keywords)
6. [Primitive Type Keywords](#6-primitive-type-keywords)
7. [Numeric Literals](#7-numeric-literals)
8. [Character Literals](#8-character-literals)
9. [String Literals](#9-string-literals)
10. [Operators](#10-operators)
11. [Punctuation](#11-punctuation)

---

## 1. Character Encoding

Solix source files are encoded in **UTF-8**. Only the ASCII subset (`U+0000`–`U+007F`) is significant to the lexer; non-ASCII bytes are permitted only inside string literals and comments.

### Byte Order Mark (BOM)

If the first three bytes of a source file are `0xEF 0xBB 0xBF` (the UTF-8 BOM), they are silently consumed and ignored. A BOM appearing anywhere else in the file is a lexical error.

### Line Endings

Both Unix line endings (`LF`, `U+000A`) and Windows line endings (`CRLF`, `U+000D U+000A`) are accepted. A lone `CR` (`U+000D`) not followed by `LF` is treated as a single line terminator. All line-ending forms are normalized to `LF` for the purposes of line-number tracking in diagnostics.

---

## 2. Comments

Comments are lexically erased before parsing and carry no semantic meaning.

### Single-Line Comments

A single-line comment begins with `//` and extends to the end of the current line. The line terminator itself is not part of the comment and is consumed separately as whitespace.

```solix
// This entire line is a comment.
int32 x = 42; // Inline comment — everything after // is ignored.
```

### Block Comments

A block comment begins with `/*` and ends with the first subsequent `*/`. Block comments **do not nest** — an inner `/*` has no effect.

```solix
/*
 * This is a block comment.
 * It spans multiple lines.
 */
int32 y = 0; /* inline block comment */
```

> [!WARNING]
> Block comments do not nest. `/* outer /* inner */ still inside? */` — the comment ends at the first `*/`, leaving ` still inside? */` as unparsed source text, which will produce a syntax error.

---

## 3. Whitespace

The following characters are classified as **whitespace**:

| Character | Unicode | Name            |
|-----------|---------|-----------------|
| ` `       | U+0020  | Space           |
| `\t`      | U+0009  | Horizontal Tab  |
| `\n`      | U+000A  | Line Feed       |
| `\r`      | U+000D  | Carriage Return |

Whitespace is insignificant except as a separator between tokens. Adjacent tokens that would otherwise be ambiguous (e.g., two identifiers, or a keyword followed by an identifier) must be separated by at least one whitespace character or a comment.

---

## 4. Identifiers

An **identifier** is a name that refers to a declared entity such as a variable, method, class, or package.

### Syntax

```
identifier  ::= id_start id_continue*
id_start    ::= [A-Za-z_]
id_continue ::= [A-Za-z0-9_]
```

- Identifiers must begin with an ASCII letter (`A`–`Z`, `a`–`z`) or an underscore (`_`).
- Subsequent characters may be ASCII letters, decimal digits (`0`–`9`), or underscores.
- Identifiers are **case-sensitive**: `count`, `Count`, and `COUNT` are three distinct names.
- Unicode characters are **not** permitted in identifiers; they may only appear inside string or character literals, and comments.
- An identifier that exactly matches a reserved keyword (see §5) or a primitive type keyword (see §6) is illegal as a user-defined name.

**Valid identifiers:**

```
x
_temp
MyClass
parse_int32
_unused_
result2
```

**Invalid identifiers:**

```
2result      // starts with a digit
my-var       // hyphen is not id_continue
café         // non-ASCII character
```

---

## 5. Reserved Keywords

The following 40 tokens are **reserved keywords**. They may not be used as identifiers.

### Control Flow

| Keyword    | Description                                        |
|------------|----------------------------------------------------|
| `if`       | Conditional branch                                 |
| `else`     | Alternative branch of `if`                         |
| `while`    | Pre-condition loop                                 |
| `do`       | Post-condition loop (used with `while`)            |
| `for`      | Counted or iterator loop                           |
| `switch`   | Multi-way integer dispatch                         |
| `case`     | Labeled arm of a `switch`                          |
| `default`  | Fallback arm of a `switch`                         |
| `break`    | Exit the enclosing loop or `switch`                |
| `continue` | Advance to the next iteration of the enclosing loop|
| `return`   | Exit the current function, optionally with a value |
| `try`      | Introduce a guarded block for exception handling   |
| `catch`    | Handle a thrown exception by type                  |
| `finally`  | Execute cleanup code regardless of exception path  |
| `throw`    | Raise an exception object                          |

### Declarations

| Keyword     | Description                                           |
|-------------|-------------------------------------------------------|
| `class`     | Declare a reference type                              |
| `interface` | Declare an abstract contract                          |
| `enum`      | Declare a strongly-typed enumeration                  |
| `package`   | Declare the compilation unit's namespace              |
| `import`    | Bring symbols from another package into scope         |
| `alias`     | Declare a type synonym or generic alias               |

### Modifiers

| Keyword      | Description                                                         |
|--------------|---------------------------------------------------------------------|
| `public`     | Accessible from any package                                         |
| `private`    | Accessible only within the declaring class                          |
| `protected`  | Accessible within the declaring class and its subclasses            |
| `internal`   | Accessible only within the same package                             |
| `static`     | Belongs to the class rather than an instance                        |
| `inline`     | Hint to inline the method body at call sites                        |
| `native`     | Body is implemented by a registered C++ native function             |
| `const`      | Declares an immutable field or variable                             |
| `virtual`    | Method may be overridden in a subclass                              |
| `override`   | Method explicitly overrides a `virtual` method from a superclass    |
| `weak`       | Field holds a weak (non-owning) reference for ARC cycle-breaking    |
| `abstract`   | Class or method has no concrete implementation                      |

### Expression Keywords

| Keyword      | Description                                                |
|--------------|------------------------------------------------------------|
| `new`        | Allocate and construct a heap object                       |
| `super`      | Reference the superclass or its constructor                |
| `this`       | Reference the current object instance                      |
| `instanceof` | Test whether a reference is an instance of a type          |
| `sizeof`     | Query compile-time or runtime byte size of a type or instance |
| `operator`   | Declare an operator overload                               |
| `null`       | The null reference literal                                 |
| `true`       | Boolean literal true                                       |
| `false`      | Boolean literal false                                      |

### Type Relation Keywords

| Keyword      | Description                                         |
|--------------|-----------------------------------------------------|
| `extends`    | Specify the superclass in a class declaration       |
| `implements` | Specify implemented interfaces in a class declaration|

---

## 6. Primitive Type Keywords

The following identifiers are reserved as **primitive type names**. They denote built-in value types and may not be used as user-defined identifiers.

| Keyword   | Description                         | Width   |
|-----------|-------------------------------------|---------|
| `bool`    | Boolean (`true` / `false`)          | 1 bit logical (stored as 64-bit word) |
| `char`    | ASCII character                     | 8 bits  |
| `int8`    | Signed 8-bit integer                | 8 bits  |
| `int16`   | Signed 16-bit integer               | 16 bits |
| `int32`   | Signed 32-bit integer               | 32 bits |
| `int64`   | Signed 64-bit integer               | 64 bits |
| `uint8`   | Unsigned 8-bit integer              | 8 bits  |
| `uint16`  | Unsigned 16-bit integer             | 16 bits |
| `uint32`  | Unsigned 32-bit integer             | 32 bits |
| `uint64`  | Unsigned 64-bit integer             | 64 bits |
| `float32` | 32-bit IEEE 754 floating-point      | 32 bits |
| `float64` | 64-bit IEEE 754 floating-point      | 64 bits |
| `void`    | Absence of a value (return type only)| —      |

---

## 7. Numeric Literals

Numeric literals represent compile-time constant values. The lexer classifies them by their prefix and content; suffixes refine the type.

### 7.1 Integer Literals

#### Decimal

A sequence of one or more decimal digits (`0`–`9`). Leading zeros are **not** permitted (a leading zero is reserved for the `0x` and `0b` prefixes below).

```
42
1000
0
```

#### Hexadecimal

Prefix `0x` or `0X`, followed by one or more hexadecimal digits (`0`–`9`, `a`–`f`, `A`–`F`).

```
0xFF
0xDEADBEEF
0x0001
```

#### Binary

Prefix `0b` or `0B`, followed by one or more binary digits (`0` or `1`).

```
0b1010
0b11110000
0b0
```

#### Integer Type Suffixes

By default an integer literal has the smallest type that fits: `int32` for values in the 32-bit signed range, `int64` otherwise. The suffix `L` or `l` forces the type to `int64`.

| Suffix | Type   | Example     |
|--------|--------|-------------|
| *(none)*| `int32` (default, if fits) | `42`     |
| `L` or `l` | `int64` | `42L`   |

### 7.2 Floating-Point Literals

A floating-point literal contains a decimal point and/or an exponent. The default type is `float64`.

```
3.14
2.718e10
1.0e-3
0.5
```

The suffix `f` or `F` narrows the literal to `float32`:

```
3.14f
1.0F
```

**Syntax summary:**

```
float_literal  ::= decimal_digits '.' decimal_digits? exponent? float_suffix?
                 | decimal_digits exponent float_suffix?
exponent       ::= ('e' | 'E') ('+' | '-')? decimal_digits
float_suffix   ::= 'f' | 'F'
```

> [!NOTE]
> A literal such as `1e3` (no decimal point) is a valid `float64` literal. A literal such as `1.` (trailing decimal point, no fractional digits) is also valid and equals `1.0`.

---

## 8. Character Literals

A character literal represents a single `char` value and is delimited by single quotes (`'`).

```solix
char a = 'A';
char nl = '\n';
char nul = '\0';
```

**Syntax:**

```
char_literal   ::= "'" char_body "'"
char_body      ::= printable_ascii_char_except_backslash_and_single_quote
                 | escape_sequence
```

### Escape Sequences

| Escape | Unicode | Description          |
|--------|---------|----------------------|
| `\n`   | U+000A  | Line feed (newline)  |
| `\t`   | U+0009  | Horizontal tab       |
| `\r`   | U+000D  | Carriage return      |
| `\\`   | U+005C  | Backslash            |
| `\'`   | U+0027  | Single quote         |
| `\"`   | U+0022  | Double quote         |
| `\0`   | U+0000  | Null character       |
| `\xHH` | U+00HH  | Hexadecimal byte value (exactly 2 hex digits) |

A character literal must contain exactly one character body element (after escape expansion). An empty literal `''` or a multi-character literal `'ab'` is a lexical error.

---

## 9. String Literals

A string literal represents a sequence of zero or more characters and is delimited by double quotes (`"`).

```solix
string greeting = "Hello, World!";
string empty = "";
string escaped = "Line 1\nLine 2\tTabbed";
string hex = "Null byte: \x00 End";
```

**Syntax:**

```
string_literal ::= '"' string_body* '"'
string_body    ::= printable_ascii_char_except_backslash_and_double_quote
                 | escape_sequence
```

The same escape sequences defined in §8 apply inside string literals (`\n`, `\t`, `\r`, `\\`, `\'`, `\"`, `\0`, `\xHH`). The lexer decodes these sequences directly into their corresponding byte values when constructing the token literal representation:
- `\n` decodes to ASCII byte `0x0A` (line feed)
- `\t` decodes to ASCII byte `0x09` (horizontal tab)
- `\r` decodes to ASCII byte `0x0D` (carriage return)
- `\\` decodes to ASCII byte `0x5C` (`\`)
- `\"` decodes to ASCII byte `0x22` (`"`)
- `\'` decodes to ASCII byte `0x27` (`'`)
- `\0` decodes to ASCII byte `0x00` (null byte)
- `\xHH` decodes to the exact hexadecimal byte value denoted by two hex digits `HH`. Any invalid hexadecimal character following `\x` causes a lexical error (`E_LEX: Invalid hex character in string literal`).

String literals may span multiple source lines only if each embedded newline is represented by an explicit `\n` escape sequence; a raw unescaped line terminator inside a string literal is a lexical error.


---

## 10. Operators

The following operator tokens are recognized by the lexer. Longer tokens are matched greedily (maximal munch): `<=` is a single token, not `<` followed by `=`.

| Category        | Operators                                      |
|-----------------|------------------------------------------------|
| Arithmetic      | `+`  `-`  `*`  `/`  `%`                        |
| Comparison      | `==`  `!=`  `<`  `<=`  `>`  `>=`              |
| Logical         | `&&`  `\|\|`  `!`                               |
| Bitwise         | `&`  `\|`  `^`  `~`  `<<`  `>>`               |
| Assignment      | `=`  `+=`  `-=`  `*=`  `/=`  `%=`             |
| Increment/Decrement | `++`  `--`                                 |
| Ternary         | `?`  `:`                                       |
| Member access   | `.`                                            |
| Lambda/arrow    | `=>`                                           |
| Scope resolution| `::`                                           |

> [!NOTE]
> The `=>` token is used in lambda or handler syntax. The `::` token is used for explicit scope-qualified name resolution (e.g., `ClassName::staticMethod`).

---

## 11. Punctuation

The following single-character tokens serve as structural delimiters and separators. They carry no operator semantics.

| Token | Name              | Usage                                      |
|-------|-------------------|--------------------------------------------|
| `;`   | Semicolon         | Statement terminator                       |
| `,`   | Comma             | Parameter/argument separator, list separator |
| `(`   | Left parenthesis  | Expression grouping, parameter list open   |
| `)`   | Right parenthesis | Expression grouping, parameter list close  |
| `{`   | Left brace        | Block / initializer open                   |
| `}`   | Right brace       | Block / initializer close                  |
| `[`   | Left bracket      | Array type annotation, subscript open      |
| `]`   | Right bracket     | Array type annotation, subscript close     |
