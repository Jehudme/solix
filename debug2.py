import sys

with open("language/src/compiler.cpp", "r") as f:
    code = f.read()

target = """void Compiler::compileExpression(parser::Node* expr) {
    if (!expr) return;"""
replacement = """#include <iostream>
void Compiler::compileExpression(parser::Node* expr) {
    if (!expr) return;
    std::cout << "compileExpr: " << (int)expr->node_type << std::endl;"""

code = code.replace(target, replacement)
with open("language/src/compiler.cpp", "w") as f:
    f.write(code)
