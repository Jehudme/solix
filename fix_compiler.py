import sys

with open("language/src/compiler.cpp", "r") as f:
    code = f.read()

# Fix Control flow fields
code = code.replace("if_stmt->if_block", "if_stmt->then_branch")
code = code.replace("if_stmt->else_block", "if_stmt->else_branch")
code = code.replace("while_stmt->body", "while_stmt->children[0]")
code = code.replace("do_while_stmt->body", "do_while_stmt->children[0]")
code = code.replace("for_stmt->body", "for_stmt->children[0]")
code = code.replace("for_stmt->update", "for_stmt->iteration")

# Fix OpCodes in disassemble
code = code.replace("OpCode::SUB:", "OpCode::SUBTRACT:")
code = code.replace("OpCode::MUL:", "OpCode::MULTIPLY:")
code = code.replace("OpCode::DIV:", "OpCode::DIVIDE:")
code = code.replace("OpCode::MOD:", "OpCode::MODULO:")
code = code.replace("\"SUB\\n\"", "\"SUBTRACT\\n\"")
code = code.replace("\"MUL\\n\"", "\"MULTIPLY\\n\"")
code = code.replace("\"DIV\\n\"", "\"DIVIDE\\n\"")
code = code.replace("\"MOD\\n\"", "\"MODULO\\n\"")

# Fix offset -> i
code = code.replace("bytecode[offset]", "bcode[i]")
code = code.replace("offset += 4", "i += 4")

with open("language/src/compiler.cpp", "w") as f:
    f.write(code)

print("Done")
