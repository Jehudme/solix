# InterfaceDeclaration (`NodeType::INTERFACE`)

## 1. Description & Purpose

The `interface` declaration defines an abstract behavioral contract that implementing classes must satisfy. Interfaces declare method signatures without function bodies. In Solix, a class can implement multiple interfaces, allowing flexible polymorphism without the multiple inheritance diamond problem. Interface methods are dispatched dynamically at runtime via interface dispatch tables (ITables) or Virtual Method Tables (VTables).

## 2. Syntax & Grammar

```solix
[access-modifier] interface <identifier> '{' <method-signature...>';' '}'
```

## 3. Underlying Systems & Mechanics

- Declares a pure protocol of method signatures without bodies or state.
- Checked during Pass 2: any concrete class claiming `implements Iface` must provide concrete implementations matching every method signature.

## 4. Positive Test Scenarios (Valid Variations)

1. **Single Method Interface**: `interface Runnable { void run(); }`
2. **Multi-Method Interface**: `interface Collection { int32 size(); void clear(); }`
3. **Multiple Conformance**: `class Texture implements Renderable, Disposable { ... }`

## 5. Negative Test Scenarios (Invalid Variations)

1. **Declaring Fields in Interface**:
   - `interface Bad { int32 x; }`  
     *Error*: `Interfaces cannot declare fields`
2. **Method with Body in Interface**:
   - `interface Bad { void run() {} }`  
     *Error*: `Interface methods cannot have bodies`
3. **Direct Instantiation**:
   - `Runnable r = new Runnable();`  
     *Error*: `Cannot instantiate interface 'Runnable'`
4. **Class Failing to Implement Interface Method**:
   - `class App implements Runnable {}` (without `run()`)  
     *Error*: `Class 'App' does not implement required interface method 'void Runnable.run()'`
