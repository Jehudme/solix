# ArrayLiteralExpression (`NodeType::ARRAY_LITERAL`)

## 1. Description & Purpose

The `array literal` expression constructs and populates an array inline using either bracket notation (`[elem1, elem2]`) or brace notation (`{elem1, elem2}`). The compiler infers the array's element type from common ancestor unification of the elements, allocates the heap array buffer, and populates the elements in sequence.

## 2. Syntax & Grammar

```solix
'[' <expression> (',' <expression>)* [','] ']'
'{' <expression> (',' <expression>)* [','] '}'
```

## 3. Underlying Systems & Mechanics

- Compiler infers element type from expressions.
- Polymorphic deduction: finds most specific common base class.
- Emits `ARRAY_ALLOC` with literal length followed by sequential stores.

## 4. Positive Test Scenarios (Valid Variations)

1. **Square Bracket Primitive Array Literal**: `int32[] arr = [1, 2, 3, 4, 5];`
2. **Curly Brace Primitive Array Literal**: `int32[] arr2 = {10, 20, 30};`
3. **Polymorphic Object Array Literal**: `Animal[] pets = [new Dog(), new Cat()];`

## 5. Negative Test Scenarios (Invalid Variations)

1. **Heterogeneous Incompatible Elements**:
   - `auto arr = [1, "two", new Dog()];`  
     *Error*: `Mixed types in array literal`
