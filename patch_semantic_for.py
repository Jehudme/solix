import sys

with open("language/src/semantic.cpp", "r") as f:
    code = f.read()

target = """        if (for_statement->iteration) evaluateExpression(tree, for_statement->iteration.get());"""
replacement = """        if (for_statement->iteration) resolveAndCheck(tree, for_statement->iteration.get());"""

code = code.replace(target, replacement)
with open("language/src/semantic.cpp", "w") as f:
    f.write(code)
