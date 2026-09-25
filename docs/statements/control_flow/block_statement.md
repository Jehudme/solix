# BlockStatement (`NodeType::BLOCK`)

## 1. Description & Purpose

The `block` statement groups a sequence of zero or more statements into a single syntactic unit enclosed by curly braces (`{ ... }`). Crucially, a block creates a new lexical scope in the symbol table. It dictates variable visibility, variable shadowing rules, and local variable lifetime. Under Solix's ARC memory model, the exit of a block triggers automatic cleanup code emission: reference-counted variables declared within the block have their references decremented (`DEC_REF` / `RELEASE`) in reverse declaration order.

## 2. Syntax & Grammar

```solix
'{' <statement...>* '}'
```

## 3. Underlying Systems & Mechanics

- Pushes a new lexical `SymbolTable` scope into the scope stack on entry.
- Allocates local register indices (`local_variable_index`).
- **ARC Automatic Scope Exit Cleanup**:
  - The compiler tracks all reference-typed local variables introduced in this block.
  - Upon natural exit of the block, the compiler emits `DEC_REF` / `RELEASE` instructions for every reference variable declared in that block in reverse declaration order.
- Restores parent lexical scope on exit.

## 4. Positive Test Scenarios (Valid Variations)

1. **Empty Block**: `{}`
2. **Function Body Block**: `{ return 0; }`
3. **Arbitrary Nested Scoping**:
   ```solix
   int32 x = 10;
   {
       int32 y = 20;
       String scoped_str = new String("temp");
       // scoped_str freed by ARC exactly at closing brace
   }
   // y is out of scope here
   ```

## 5. Negative Test Scenarios (Invalid Variations)

1. **Accessing Local Variable Outside Its Enclosing Block**:
   - `{ int32 inside = 42; } int32 outside = inside;`  
     *Error*: `Undefined identifier: inside`
2. **Unmatched Closing Brace**:
   - `void f() { int32 x = 1; } }`  
     *Error*: Syntax error: unexpected token `}`
3. **Unterminated Block**:
   - `void f() { int32 x = 1;`  
     *Error*: Syntax error: expected `}` before end of file
