import sys

with open("language/include/solix/compiler.hpp", "r") as f:
    code = f.read()

code = code.replace(r"parser::AstTree ast_tree;\n    std::vector<std::vector<uint32_t>> loop_break_patches;\n    std::vector<std::vector<uint32_t>> loop_continue_patches;", "parser::AstTree ast_tree;\n    std::vector<std::vector<uint32_t>> loop_break_patches;\n    std::vector<std::vector<uint32_t>> loop_continue_patches;")

with open("language/include/solix/compiler.hpp", "w") as f:
    f.write(code)

with open("language/src/compiler.cpp", "r") as f:
    code = f.read()
code = code.replace("throw_compile_error(entry_node,", "throw_compile_error(nullptr,")
with open("language/src/compiler.cpp", "w") as f:
    f.write(code)
