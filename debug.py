import sys

with open("language/src/compiler.cpp", "r") as f:
    code = f.read()

target = 'throw_compile_error(expr, "Unhandled AST node type in compiler (compileExpression): " + std::to_string(static_cast<int>(expr->node_type)) + " " + std::string(expr->tokens.empty() ? "empty" : expr->tokens.front().value.value_or("null")));'
replacement = 'throw_compile_error(expr, "Unhandled AST node type in compiler (compileExpression): " + std::to_string(static_cast<int>(expr->node_type)) + " parent: " + (expr->parent_node ? std::to_string(static_cast<int>(expr->parent_node->node_type)) : "null"));'

code = code.replace(target, replacement)
with open("language/src/compiler.cpp", "w") as f:
    f.write(code)
