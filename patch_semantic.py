import sys

with open("language/src/semantic.cpp", "r") as f:
    code = f.read()

target = """    // Pass 1: Global Outline
    for (const auto& node : tree.nodes) {
        if (node->node_type == parser::NodeType::PACKAGE_STATEMENT) {
            auto package_statement = static_cast<parser::PackageStatement*>(node.get());
            current_package = package_statement->package_name + ".";
            package_statement->symbol_name = package_statement->package_name;
            tree.symbols[package_statement->symbol_name] = package_statement;
        } else {
            registerGlobalSymbols(tree, node.get(), current_package);
        }
    }"""

replacement = """    // Pass 1: Global Outline
    std::string current_file = "";
    
    for (const auto& node : tree.nodes) {
        // Prevent package leakage between different files
        if (node->file_path.string() != current_file) {
            current_file = node->file_path.string();
            current_package = "";
        }
        
        if (node->node_type == parser::NodeType::PACKAGE_STATEMENT) {
            auto package_statement = static_cast<parser::PackageStatement*>(node.get());
            current_package = package_statement->package_name + ".";
            package_statement->symbol_name = package_statement->package_name;
            tree.symbols[package_statement->symbol_name] = package_statement;
        } else {
            registerGlobalSymbols(tree, node.get(), current_package);
        }
    }"""

code = code.replace(target, replacement)

with open("language/src/semantic.cpp", "w") as f:
    f.write(code)
