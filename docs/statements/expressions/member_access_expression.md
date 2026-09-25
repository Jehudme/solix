# MemberAccessExpression

## 1. Overview & Purpose

A `MemberAccessExpression` (`receiver.member`) accesses fields or nested properties. Instance field offsets are resolved statically at compile time.

---

## 2. Compilation & Runtime Mechanics (With Real Bytecode)

### Bytecode Disassembly Example
```solix
// Solix Code
int32 age = user.age;
```

```bytecode
// Compiled VM Bytecode
GET_LOCAL 1                 // user
GET_PROPERTY 0              // Reads field at offset 0
SET_LOCAL 2                 // age
```

---

## 3. Valid Test Cases (Positive Scenarios)

### Case 3.1: Chained Member Access
```solix
class Address { String city; }
class Person { Address addr; }

void test(Person p) {
    String city = p.addr.city;
}
```
*Expected Result*: Resolves offsets sequentially; compiles cleanly.

---

## 4. Invalid Test Cases & Expected Errors (Negative Scenarios)

### Case 4.1: Member Access on Null Reference (Runtime Fault)
```solix
Person p = null;
String c = p.addr; // Throws NullReferenceException
```
*Expected Runtime Exception*:
```text
[FATAL VM PANIC] NullReferenceException: Attempted to read property from null object reference
```
