# NewInstanceExpression (`NodeType::NEW_INSTANCE`)

## 1. Description & Purpose

The `new` instance expression instantiates a class on the heap (`new ClassName(arguments)`). It triggers memory allocation for the object instance, initializes internal ARC header metadata and VTable pointers, and invokes the matching constructor with the evaluated arguments. The resulting reference count is initialized to 1.

## 2. Syntax & Grammar

```solix
'new' <class-type> ['<' <type-args...> '>'] '(' <arguments...> ')'
```

## 3. Underlying Systems & Mechanics

- Emits `ALLOC` with class `instance_size`.
- Sets word 0 to `vtable_id` and initial ARC count = 1.
- Injects field default initializers.
- Emits `INVOKE_DIRECT` to chosen constructor matching argument types.
- Leaves allocated reference on stack.

## 4. Positive Test Scenarios (Valid Variations)

1. **Default Constructor Call**: `Player p = new Player();`
2. **Parameterized Constructor Call**: `Player p = new Player(100);`
3. **Generic Class Instantiation**: `List<String> list = new List<String>();`

## 5. Negative Test Scenarios (Invalid Variations)

1. **Instantiating Abstract Class**:
   - `Shape s = new Shape();`  
     *Error*: `Cannot instantiate abstract class 'Shape'`
2. **No Matching Constructor Signature**:
   - `class A { public A(int32 x) {} } A a = new A();`  
     *Error*: `No matching constructor: A.ctor()`
