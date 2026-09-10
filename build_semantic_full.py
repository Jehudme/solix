import re

with open("language/src/semantic.cpp", "r") as f:
    code = f.read()

# 1. Update resolveType
bad_resolve = """TypeInfo SemanticAnalyzer::resolveType(parser::AstTree& tree, const std::string& raw_type_name, const std::vector<lexer::Token>& tokens) {
    TypeInfo info;
    std::string type_str = raw_type_name;
    
    // Simple parsing of "array array int32"
    while (type_str.find("array ") == 0) {
        info.array_depth++;
        type_str = type_str.substr(6);
    }
    
    // Trim spaces
    while (!type_str.empty() && type_str.back() == ' ') type_str.pop_back();
    
    info.base_name = type_str;
    
    if (type_str == "int32" || type_str == "float64" || type_str == "bool" || type_str == "string") {
        info.is_primitive = true;
    } else {
        // Need to resolve class reference
        // Simplification for now: check global symbols map
        // Actually, solix fully qualified names need package resolution. Let's just do a rough check.
    }
    
    return info;
}"""

good_resolve = """TypeInfo SemanticAnalyzer::resolveType(parser::AstTree& tree, const std::string& raw_type_name, const std::vector<lexer::Token>& tokens) {
    TypeInfo info;
    std::string type_str = raw_type_name;
    
    while (type_str.find("array ") == 0) {
        info.array_depth++;
        type_str = type_str.substr(6);
    }
    
    while (!type_str.empty() && type_str.back() == ' ') type_str.pop_back();
    
    if (type_str == "int32" || type_str == "float64" || type_str == "bool" || type_str == "string") {
        info.base_name = type_str;
        info.is_primitive = true;
    } else {
        std::string full_name = current_package + type_str;
        if (tree.symbols.count(full_name)) {
            info.class_ref = tree.symbols[full_name];
            info.base_name = full_name;
        } else if (tree.symbols.count(type_str)) {
            info.class_ref = tree.symbols[type_str];
            info.base_name = type_str;
        } else {
            throw std::runtime_error("Undefined type: " + type_str);
        }
    }
    return info;
}"""
code = code.replace(bad_resolve, good_resolve)

# 2. Add resolveType to VariableDeclaration in resolveAndCheck
bad_var = """        // Generate UUID for local var
        var->symbol_name = current_package + (current_class ? current_class->class_name + "." : "") + var->var_name + generate_uuid();
        tree.symbols[var->symbol_name] = var;
        
        // If it has an initializer, check it
        if (var->initializer) {"""

good_var = """        // Generate UUID for local var
        var->symbol_name = current_package + (current_class ? current_class->class_name + "." : "") + var->var_name + generate_uuid();
        tree.symbols[var->symbol_name] = var;
        
        TypeInfo t_info = resolveType(tree, var->type_name, var->tokens);
        var->resolved_type = t_info.base_name;
        var->resolved_array_depth = t_info.array_depth;
        
        // If it has an initializer, check it
        if (var->initializer) {"""
code = code.replace(bad_var, good_var)


# 3. Update evaluateExpression
bad_eval = """TypeInfo SemanticAnalyzer::evaluateExpression(parser::AstTree& tree, parser::Node* expr) {
    TypeInfo result;
    if (!expr) return result;
    
    if (expr->node_type == parser::NodeType::LITERAL_EXPRESSION) {
        auto lit = static_cast<parser::LiteralExpression*>(expr);
        if (lit->token.type == lexer::TokenType::NUMBER) {
            if (lit->token.value.value_or("").find('.') != std::string::npos) result.base_name = "float64";
            else result.base_name = "int32";
        }
        else if (lit->token.type == lexer::TokenType::STRING) result.base_name = "string";
        else if (lit->token.type == lexer::TokenType::IDENTIFIER && (lit->token.value == "true" || lit->token.value == "false")) result.base_name = "bool";
        result.is_primitive = true;
    } else if (expr->node_type == parser::NodeType::IDENTIFIER_EXPRESSION) {
        auto ident = static_cast<parser::IdentifierExpression*>(expr);
        if (ident->name == "true" || ident->name == "false") {
            result.base_name = "bool";
            result.is_primitive = true;
        } else {
            parser::Node* decl = lookupSymbol(tree, ident->name);
            if (!decl) throw std::runtime_error("Undefined variable: " + ident->name);
            ident->resolved_declaration = decl;
        }
        // In a real implementation we would copy the resolved_type of the declaration
    } else if (expr->node_type == parser::NodeType::BINARY_EXPRESSION) {
        auto bin = static_cast<parser::BinaryExpression*>(expr);
        evaluateExpression(tree, bin->left.get());
        evaluateExpression(tree, bin->right.get());
        // For simplicity, just return int32 for now unless we do deep promotion rules
        result.base_name = "int32";
        result.is_primitive = true;
    } else if (expr->node_type == parser::NodeType::ASSIGNMENT_EXPRESSION) {
        auto assign = static_cast<parser::AssignmentExpression*>(expr);
        evaluateExpression(tree, assign->target.get());
        evaluateExpression(tree, assign->value.get());
    }
    
    expr->resolved_type = result.base_name;
    expr->resolved_array_depth = result.array_depth;
    return result;
}"""

good_eval = """TypeInfo SemanticAnalyzer::evaluateExpression(parser::AstTree& tree, parser::Node* expr) {
    TypeInfo result;
    if (!expr) return result;
    
    if (expr->node_type == parser::NodeType::LITERAL_EXPRESSION) {
        auto lit = static_cast<parser::LiteralExpression*>(expr);
        if (lit->token.type == lexer::TokenType::NUMBER) {
            if (lit->token.value.value_or("").find('.') != std::string::npos) result.base_name = "float64";
            else result.base_name = "int32";
        }
        else if (lit->token.type == lexer::TokenType::STRING) result.base_name = "string";
        else if (lit->token.type == lexer::TokenType::IDENTIFIER && (lit->token.value == "true" || lit->token.value == "false")) result.base_name = "bool";
        result.is_primitive = true;
    } else if (expr->node_type == parser::NodeType::IDENTIFIER_EXPRESSION) {
        auto ident = static_cast<parser::IdentifierExpression*>(expr);
        if (ident->name == "true" || ident->name == "false") {
            result.base_name = "bool";
            result.is_primitive = true;
        } else {
            parser::Node* decl = lookupSymbol(tree, ident->name);
            if (!decl) throw std::runtime_error("Undefined variable: " + ident->name);
            ident->resolved_declaration = decl;
            result.base_name = decl->resolved_type;
            result.array_depth = decl->resolved_array_depth;
            // Also handle fields
            if (decl->node_type == parser::NodeType::FIELD_DECLARATION) {
                enforceAccessModifier(decl, {});
            }
        }
    } else if (expr->node_type == parser::NodeType::BINARY_EXPRESSION) {
        auto bin = static_cast<parser::BinaryExpression*>(expr);
        TypeInfo left = evaluateExpression(tree, bin->left.get());
        TypeInfo right = evaluateExpression(tree, bin->right.get());
        if (left.base_name == "float64" || right.base_name == "float64") result.base_name = "float64";
        else result.base_name = "int32";
        result.is_primitive = true;
    } else if (expr->node_type == parser::NodeType::ASSIGNMENT_EXPRESSION) {
        auto assign = static_cast<parser::AssignmentExpression*>(expr);
        enforceLValue(assign->target.get(), {});
        TypeInfo target = evaluateExpression(tree, assign->target.get());
        TypeInfo val = evaluateExpression(tree, assign->value.get());
        if (target.base_name != val.base_name || target.array_depth != val.array_depth) {
            throw std::runtime_error("Type mismatch in assignment");
        }
        result = target;
    } else if (expr->node_type == parser::NodeType::MEMBER_ACCESS_EXPRESSION) {
        auto mem = static_cast<parser::MemberAccessExpression*>(expr);
        TypeInfo obj = evaluateExpression(tree, mem->object.get());
        if (obj.is_primitive || !tree.symbols.count(obj.base_name)) throw std::runtime_error("Cannot access member on primitive or undefined type");
        auto cls = static_cast<parser::ClassDeclaration*>(tree.symbols[obj.base_name]);
        parser::Node* found = nullptr;
        for (const auto& child : cls->children) {
            if (child->node_type == parser::NodeType::FIELD_DECLARATION) {
                auto field = static_cast<parser::FieldDeclaration*>(child.get());
                if (field->field_name == mem->member_name) { found = field; break; }
            } else if (child->node_type == parser::NodeType::METHOD_DECLARATION) {
                auto method = static_cast<parser::MethodDeclaration*>(child.get());
                if (method->method_name == mem->member_name) { found = method; break; }
            }
        }
        if (!found) throw std::runtime_error("Undefined member: " + mem->member_name);
        enforceAccessModifier(found, {});
        
        if (found->node_type == parser::NodeType::FIELD_DECLARATION) {
            auto f = static_cast<parser::FieldDeclaration*>(found);
            TypeInfo field_t = resolveType(tree, f->type_name, {});
            result = field_t;
        } else {
            auto m = static_cast<parser::MethodDeclaration*>(found);
            TypeInfo ret_t = resolveType(tree, m->return_type, {});
            result = ret_t;
        }
    } else if (expr->node_type == parser::NodeType::ARRAY_ACCESS_EXPRESSION) {
        auto arr = static_cast<parser::ArrayAccessExpression*>(expr);
        TypeInfo target = evaluateExpression(tree, arr->target.get());
        if (target.array_depth == 0) throw std::runtime_error("Cannot index into non-array type");
        TypeInfo index = evaluateExpression(tree, arr->index.get());
        if (index.base_name != "int32") throw std::runtime_error("Array index must be int32");
        result = target;
        result.array_depth--;
    } else if (expr->node_type == parser::NodeType::CALL_EXPRESSION) {
        auto call = static_cast<parser::CallExpression*>(expr);
        TypeInfo target = evaluateExpression(tree, call->target.get());
        for (auto& arg : call->arguments) evaluateExpression(tree, arg.get());
        result = target; // If target was method, it resolved to its return type
    }
    
    expr->resolved_type = result.base_name;
    expr->resolved_array_depth = result.array_depth;
    return result;
}"""
code = code.replace(bad_eval, good_eval)

# 4. Update Access & LValue
bad_acc = """void SemanticAnalyzer::enforceAccessModifier(parser::Node* target_node, const std::vector<lexer::Token>& tokens) {
    // Basic access modifier placeholder
}

void SemanticAnalyzer::enforceLValue(parser::Node* expr, const std::vector<lexer::Token>& tokens) {
    if (expr->node_type != parser::NodeType::IDENTIFIER_EXPRESSION &&
        expr->node_type != parser::NodeType::ARRAY_ACCESS_EXPRESSION &&
        expr->node_type != parser::NodeType::MEMBER_ACCESS_EXPRESSION) {
        throw std::runtime_error("Invalid assignment target (must be an L-Value)");
    }
}"""

good_acc = """void SemanticAnalyzer::enforceAccessModifier(parser::Node* target_node, const std::vector<lexer::Token>& tokens) {
    if (!target_node || !target_node->parent_node) return;
    
    lexer::TokenType access = lexer::TokenType::KEYWORD_PUBLIC;
    parser::Node* owner_class = target_node->parent_node;
    
    if (target_node->node_type == parser::NodeType::FIELD_DECLARATION) {
        access = static_cast<parser::FieldDeclaration*>(target_node)->access_modifier;
    } else if (target_node->node_type == parser::NodeType::METHOD_DECLARATION) {
        access = static_cast<parser::MethodDeclaration*>(target_node)->access_modifier;
    }
    
    if (access == lexer::TokenType::KEYWORD_PRIVATE) {
        if (current_class != owner_class) throw std::runtime_error("Cannot access private member outside its class");
    }
}

void SemanticAnalyzer::enforceLValue(parser::Node* expr, const std::vector<lexer::Token>& tokens) {
    if (expr->node_type != parser::NodeType::IDENTIFIER_EXPRESSION &&
        expr->node_type != parser::NodeType::ARRAY_ACCESS_EXPRESSION &&
        expr->node_type != parser::NodeType::MEMBER_ACCESS_EXPRESSION) {
        throw std::runtime_error("Invalid assignment target (must be an L-Value)");
    }
}"""
code = code.replace(bad_acc, good_acc)


with open("language/src/semantic.cpp", "w") as f:
    f.write(code)
