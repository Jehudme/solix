# MethodDeclaration (`NodeType::METHOD_DECL`)

## 1. Description & Purpose

The `method` declaration defines a callable function associated with a class or interface, or declared at the top-level global scope. Methods can be instance methods (with an implicit `this` pointer), static methods (called on the type itself), or abstract methods (requiring override in concrete subclasses). Solix methods support generic type parameters, parameter overloading, and virtual dynamic dispatch using VTables indexed by method slot IDs.

## 2. Syntax & Grammar

```solix
[access-modifier] ['static' | 'virtual' | 'override' | 'abstract' | 'inline' | 'native'] <return-type> <name> ['<' <T...> '>'] '(' <params...> ')' (<block> | ';')
```

## 3. Underlying Systems & Mechanics

- **Instance Calling Convention**: Parameter 0 is implicit `this`.
- **Static Methods**: No `this` pointer; resolved at compile time via `INVOKE_STATIC`.
- **Virtual Dispatch**: Emits `INVOKE_VIRTUAL` referencing slot index in `vtable_id`.
- **Native Host Interop (`native`)**: Bound to C++ runtime function pointer in `RuntimeOptions::native_functions`.
- **Generic Monomorphization**: Cloned and instantiated per concrete type combination, cached in monomorphization table.

## 4. Positive Test Scenarios (Valid Variations)

1. **Standard Instance Method**: `public void set_hp(int32 hp) { this.hp = hp; }`
2. **Static Utility Method**: `public static int32 max(int32 a, int32 b) { return a > b ? a : b; }`
3. **Top-Level Free Functions (Outside Class)**:
   ```solix
   public static int32 main(char[][] args) {
       return 0;
   }
   ```
4. **Virtual and Override Polymorphism**:
   - Base: `public virtual void render();`
   - Derived: `public override void render() { ... }`
5. **Abstract Method Declaration**: `public abstract void serialize();` (in abstract class; vtable slot populated with `THROW_ABSTRACT` guard)
6. **Generic Template Method**: `public static void swap<T>(T[] arr, int32 i, int32 j) { ... }`
7. **Native Method Declaration**: `public native static void print(char[] str);`

## 5. Negative Test Scenarios (Invalid Variations)

1. **Abstract Method with Body**:
   - `public abstract void f() { return; }`  
     *Error*: `Abstract methods cannot have a body`
2. **Non-Abstract Non-Native Method without Body**:
   - `public void f();`  
     *Error*: `Non-abstract method must have a body`
3. **Static Virtual Method**:
   - `public static virtual void f() {}`  
     *Error*: `Static methods cannot be virtual or abstract`
4. **Override Without Base Virtual Match**:
   - `public override void fake_method() {}`  
     *Error*: `Method 'fake_method' marked override but does not override any base method`
5. **Override Signature Mismatch**:
   - Base: `virtual void run(int32 speed);` Subclass: `override void run(float64 speed);`  
     *Error*: `Overriding method signature does not match base virtual method`
6. **Native Method with Body**:
   - `public native void print(char[] s) {}`  
     *Error*: `Native methods cannot have a body`
7. **Calling Abstract Method Directly at Runtime**:
   - Invoking abstract method without subclass override  
     *Runtime VM Check*: `THROW_ABSTRACT`
