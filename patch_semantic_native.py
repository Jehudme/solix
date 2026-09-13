import sys

with open("language/src/semantic.cpp", "r") as f:
    code = f.read()

target = """    if (node->node_type == parser::NodeType::METHOD_DECLARATION) {
        auto method = static_cast<parser::MethodDeclaration*>(node);
        method->symbol_name = package_prefix + method->method_name;
        tree.symbols[method->symbol_name] = method;
    }"""

replacement = """    if (node->node_type == parser::NodeType::METHOD_DECLARATION) {
        auto method = static_cast<parser::MethodDeclaration*>(node);
        method->symbol_name = package_prefix + method->method_name;
        tree.symbols[method->symbol_name] = method;
        if (method->is_native) {
            tree.native_methods.push_back(method);
        }
    }"""

code = code.replace(target, replacement)

with open("language/src/semantic.cpp", "w") as f:
    f.write(code)
