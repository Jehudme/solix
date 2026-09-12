import sys

with open("language/src/compiler.cpp", "r") as f:
    code = f.read()

code = code.replace("while_stmt->children[0]", "while_stmt->body")
code = code.replace("do_while_stmt->children[0]", "do_while_stmt->body")
code = code.replace("for_stmt->children[0]", "for_stmt->body")

with open("language/src/compiler.cpp", "w") as f:
    f.write(code)

print("Done")
