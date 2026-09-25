# MemberAccessExpression (`NodeType::MEMBER_ACCESS`)

## 1. Description & Purpose

The `member access` expression (`object.property`) accesses a field, nested member, or enum value on an object instance or type namespace. For instance access, the compiler resolves the field's byte offset within the object layout. At runtime, the VM checks for `null` receivers and reads or writes the target memory offset using `GET_PROPERTY` or `SET_PROPERTY` opcodes.

## 2. Syntax & Grammar

```solix
<expression> '.' <identifier>
<namespace-path> '::' <identifier>
```

## 3. Underlying Systems & Mechanics

- Checks if LHS is a symbol path (`extract_symbol_path`): if it resolves to a `ClassDeclaration` or `EnumDeclaration`, accesses static member without instance evaluation.
- If instance, calculates field memory offset or dispatches method.
- Special property `.length` on arrays reads length header.

## 4. Positive Test Scenarios (Valid Variations)

1. **Instance Field Access**: `int32 hp = player.hp;`
2. **Array Length Access**: `int32 len = arr.length;`
3. **Static Member via Full Path**: `solix.core.Objects.is_null(obj);`
4. **Static Member via Partial Path**: `core.Objects.is_null(obj);`
5. **Static Member via C++ Scope Operator**: `core::Objects::is_null(obj);`

## 5. Negative Test Scenarios (Invalid Variations)

1. **Accessing Private Member Outside Class**:
   - `class A { private int32 x; } A a = new A(); a.x = 10;`  
     *Error*: `Cannot access private member of class 'A'`
2. **Accessing Non-Existent Property on Array**:
   - `int32 s = arr.size;`  
     *Error*: `Arrays only have the 'length' property`
3. **Dereferencing Null Object (Runtime)**:
   - `Player p = (Player)null; int32 hp = p.health;`  
     *Runtime Exception*: `NullPointer`
