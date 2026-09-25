# ClassDeclaration (`NodeType::CLASS_DECL`)

## 1. Description & Purpose

The `class` declaration is the fundamental building block of object-oriented programming in Solix. Classes define reference types with encapsulated state (fields), initialization logic (constructors), and behaviors (methods and operators). Solix supports single class inheritance (`extends`), multiple interface implementation (`implements`), nested class declarations, generic parameterization (`class Box<T>`), and abstract classes (`abstract class Base`). Instances of classes are reference-counted heap objects managed by the Solix Automatic Reference Counting (ARC) runtime.

## 2. Syntax & Grammar

```solix
[access-modifier] ['abstract'] class <identifier> ['<' <T...> '>'] ['extends' | ':' <base-class>] ['implements' <ifaces...>] '{' <members...> '}'
```

## 3. Underlying Systems & Mechanics

- **Memory Layout & Instance Size**:
  - In Pass 2, instance layout calculates cumulative byte/word offsets. Base class fields are placed first (offsets `1 .. N-1`), followed by derived fields.
  - Word 0 contains the internal ARC header and `vtable_id`.
  - `instance_size` is calculated and hardcoded into `ALLOC` opcodes.
- **Dynamic VTable Generation**:
  - Virtual methods are assigned sequential slot indices.
  - Subclasses clone the base vtable and overwrite overridden slots.
  - Classes assigned a unique integer `vtable_id`.
- **Default Constructor Synthesis**:
  - If no constructor is written, the compiler automatically synthesizes `public ClassName() {}`.
- **Template Monomorphization**:
  - Template classes (`class Box<T>`) are stored as blueprints in `template_registry`. Upon instantiation, the AST is duplicated, substituted, and bound with a mangled name (`Box$int32`).

## 4. Positive Test Scenarios (Valid Variations)

1. **Minimal Concrete Class**: `class Simple {}`
2. **Single Inheritance**: `class Dog extends Animal {}` or `class Dog : Animal {}`
3. **Generic Class with Multiple Parameters**: `class Map<K, V> { K key; V val; }`
4. **Abstract Base Class**: `public abstract class Shape { public abstract float64 area(); }`
5. **Nested Field Declarations with Default Initializers**:
   ```solix
   class Player {
       int32 hp = 100;
       String name = new String("Player1");
   ```
6. **Nested Classes & Enums**: Declaring inner classes or enums inside the class body:
   ```solix
   public class Outer {
       public class Inner {
           int32 inner_id;
       }
       public enum InnerState { A, B }
   }
   ```

## 5. Negative Test Scenarios (Invalid Variations)

1. **Duplicate Class in Package**:
   - `class Player {} class Player {}`  
     *Error*: `Duplicate global symbol: Player`
2. **Extending Non-Existent Base Class**:
   - `class Dog extends GhostAnimal {}`  
     *Error*: `Base class not found: GhostAnimal`
3. **Extending Primitive or Array Type**:
   - `class IntWrapper extends int32 {}`  
     *Error*: `Base class cannot be a primitive or array type`
4. **Circular Inheritance**:
   - `class A extends B {} class B extends A {}`  
     *Error*: `Cyclic inheritance detected for class 'A'`
5. **Multiple Inheritance**:
   - `class Dog extends Mammal, Canine {}`  
     *Error*: Syntax error: unexpected token `,` after base class
6. **Direct Instantiation of Abstract Class**:
   - `Shape s = new Shape();`  
     *Error*: `Cannot instantiate abstract class 'Shape'`
