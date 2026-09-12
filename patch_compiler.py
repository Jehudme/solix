import sys

with open("language/src/compiler.cpp", "r") as f:
    code = f.read()

target = """std::vector<uint8_t> Compiler::compile(std::string_view source_code, std::string_view entry_point) {
    // 1. Lex and Parse
    ast_tree = parser::AstTree();
    ast_tree.include(source_code);"""

replacement = """void Compiler::include(std::string_view source_code, std::optional<std::filesystem::path> path) {
    ast_tree.include(source_code, path);
}

void Compiler::include(std::filesystem::path path) {
    ast_tree.include(path);
}

std::vector<uint8_t> Compiler::compile(std::string_view entry_point) {"""

code = code.replace(target, replacement)

with open("language/src/compiler.cpp", "w") as f:
    f.write(code)
