import re

with open("language/src/semantic.cpp", "r") as f:
    code = f.read()

helper = """
[[noreturn]] static void throw_semantic_error(parser::Node* node, const std::string& msg) {
    std::string err = "[Semantic Error] ";
    if (node) {
        if (!node->file_path.empty()) {
            err += node->file_path.string() + ":";
        }
        err += std::to_string(node->line) + ":" + std::to_string(node->column) + " - ";
    }
    err += msg;
    throw std::runtime_error(err);
}
"""

code = code.replace("namespace semantic {", "namespace semantic {\n" + helper)

def replace_throw(find_str, node_var):
    global code
    pattern = r'throw std::runtime_error\((.*?' + re.escape(find_str) + r'.*?)\);'
    code = re.sub(pattern, r'throw_semantic_error(' + node_var + r', \1);', code)

replace_throw("Duplicate local variable", "node")
replace_throw("Duplicate global symbol", "root")
replace_throw("Duplicate field symbol", "root")
replace_throw("Type mismatch in assignment for variable", "variable_declaration")
replace_throw("Return type mismatch", "return_stmt")
replace_throw("Undefined type", "nullptr")
replace_throw("'this' used outside of class", "expr")
replace_throw("Wait! this is evaluating to", "expr")
replace_throw("Undefined variable", "identifier_expr")
replace_throw("Type mismatch in assignment: expected", "expr")
replace_throw("Cannot access member on primitive", "expr")
replace_throw("Undefined enum member", "member_access_expr")
replace_throw("Undefined member", "member_access_expr")
replace_throw("Cannot access member on non-class/non-enum", "expr")
replace_throw("Cannot index into non-array", "expr")
replace_throw("Array index must be int32", "expr")
replace_throw("Attempted to call a non-method", "expr")
replace_throw("Argument count mismatch", "expr")
replace_throw("Argument type mismatch", "expr")
replace_throw("No matching constructor found", "inst")
replace_throw("Array literal elements must have consistent types", "expr")
replace_throw("Cannot access private member outside", "nullptr")
replace_throw("Invalid assignment target", "nullptr")

# Fix break/continue
break_cont_old = """    } else if (root->node_type == parser::NodeType::CONSTRUCTOR_DECLARATION) {
        auto ctor = static_cast<parser::ConstructorDeclaration*>(root);
        for (const auto& child : ctor->children) {
            resolveAndCheck(tree, child.get());
        }
    }"""
break_cont_new = """    } else if (root->node_type == parser::NodeType::CONSTRUCTOR_DECLARATION) {
        auto ctor = static_cast<parser::ConstructorDeclaration*>(root);
        for (const auto& child : ctor->children) {
            resolveAndCheck(tree, child.get());
        }
    } else if (root->node_type == parser::NodeType::BREAK_STATEMENT || root->node_type == parser::NodeType::CONTINUE_STATEMENT) {
        if (loop_depth <= 0) {
            throw_semantic_error(root, "break or continue statement outside of loop");
        }
    }"""
if break_cont_old in code: code = code.replace(break_cont_old, break_cont_new)

# Fix unhandled nodes
unhandled_old = """    } else if (root->node_type == parser::NodeType::RETURN_STATEMENT) {
        auto return_stmt = static_cast<parser::ReturnStatement*>(root);
        if (return_stmt->value) {
            TypeInfo val_type = evaluateExpression(tree, return_stmt->value.get());
            if (current_method) {
                TypeInfo expected_type = resolveType(tree, current_method->return_type, {});
                if (val_type != expected_type) throw_semantic_error(return_stmt, "Return type mismatch");
            }
        }
    }"""
unhandled_new = """    } else if (root->node_type == parser::NodeType::RETURN_STATEMENT) {
        auto return_stmt = static_cast<parser::ReturnStatement*>(root);
        if (return_stmt->value) {
            TypeInfo val_type = evaluateExpression(tree, return_stmt->value.get());
            if (current_method) {
                TypeInfo expected_type = resolveType(tree, current_method->return_type, {});
                if (val_type != expected_type) throw_semantic_error(return_stmt, "Return type mismatch");
            }
        }
    } else if (root->node_type == parser::NodeType::LITERAL_EXPRESSION ||
               root->node_type == parser::NodeType::IDENTIFIER_EXPRESSION ||
               root->node_type == parser::NodeType::BINARY_EXPRESSION ||
               root->node_type == parser::NodeType::ASSIGNMENT_EXPRESSION ||
               root->node_type == parser::NodeType::MEMBER_ACCESS_EXPRESSION ||
               root->node_type == parser::NodeType::ARRAY_ACCESS_EXPRESSION ||
               root->node_type == parser::NodeType::CALL_EXPRESSION ||
               root->node_type == parser::NodeType::UNARY_EXPRESSION ||
               root->node_type == parser::NodeType::TERNARY_EXPRESSION ||
               root->node_type == parser::NodeType::CAST_EXPRESSION ||
               root->node_type == parser::NodeType::NEW_INSTANCE_EXPRESSION ||
               root->node_type == parser::NodeType::ARRAY_CREATION_EXPRESSION ||
               root->node_type == parser::NodeType::ARRAY_LITERAL_EXPRESSION ||
               root->node_type == parser::NodeType::PACKAGE_STATEMENT) {
        // Handled elsewhere or safe to ignore here
    } else {
        throw_semantic_error(root, "Unhandled AST node type in semantic analyzer: " + std::to_string(static_cast<int>(root->node_type)));
    }"""
if unhandled_old in code: code = code.replace(unhandled_old, unhandled_new)

with open("language/src/semantic.cpp", "w") as f:
    f.write(code)

print("Patched semantic.cpp cleanly")
