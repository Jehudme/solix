import sys

with open("language/src/compiler.cpp", "r") as f:
    code = f.read()

target = """        if (for_stmt->iteration) {
            compileExpression(for_stmt->iteration.get());
            emitByte(static_cast<uint8_t>(OpCode::POP));
        }"""
        
replacement = """        if (for_stmt->iteration) {
            compileNode(for_stmt->iteration.get());
        }"""

code = code.replace(target, replacement)
with open("language/src/compiler.cpp", "w") as f:
    f.write(code)
