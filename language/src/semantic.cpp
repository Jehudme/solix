#include "solix/semantic.hpp"
#include <random>
#include <sstream>
#include <iomanip>

namespace solix {
namespace semantic {

std::string TypeInfo::to_string() const {
    std::string result_string = "";
    result_string += base_name;
    for (int index = 0; index < array_depth; index++) result_string += "[]";
    return result_string;
}

static std::string generate_uuid() {
    static std::random_device random_device_obj;
    static std::mt19937 random_generator(random_device_obj());
    static std::uniform_int_distribution<> uniform_distribution(0, 15);
    std::stringstream string_stream;
    string_stream << "<";
    for (int index = 0; index < 8; index++) {
        string_stream << std::hex << uniform_distribution(random_generator);
    }
    string_stream << ">";
    return string_stream.str();
}

void SemanticAnalyzer::pushScope() {
    scope_stack.push_back(Scope{});
}

void SemanticAnalyzer::popScope() {
    if (!scope_stack.empty()) {
        scope_stack.pop_back();
    }
}

void SemanticAnalyzer::declareLocal(const std::string& name, parser::Node* node, const std::vector<lexer::Token>& tokens) {
    if (scope_stack.empty()) return;
    if (scope_stack.back().symbols.count(name)) {
        throw std::runtime_error("Duplicate local variable in same scope: " + name);
    }
    scope_stack.back().symbols[name] = node;
}

parser::Node* SemanticAnalyzer::lookupSymbol(parser::AstTree& tree, const std::string& name) {
    // 1. Check local scopes bottom-up
    for (auto iterator = scope_stack.rbegin(); iterator != scope_stack.rend(); ++iterator) {
        if (iterator->symbols.count(name)) {
            return iterator->symbols.at(name);
        }
    }
    // 2. Check current class fields
    if (current_class) {
        for (const auto& child : current_class->children) {
            if (child->node_type == parser::NodeType::FIELD_DECLARATION) {
                auto field = static_cast<parser::FieldDeclaration*>(child.get());
                if (field->field_name == name) return field;
            }
        }
    }
    // 3. Check global symbols (Assuming the name might be fully qualified, but locals/fields are just names)
    // For now, if we don't find iterator, we return nullptr. We can improve global lookups later.
    return nullptr;
}

void SemanticAnalyzer::analyze(parser::AstTree& tree) {
    scope_stack.clear();
    loop_depth = 0;
    current_class = nullptr;
    current_method = nullptr;
    current_package = "";
    
    // Pass 1: Global Outline
    for (const auto& node : tree.nodes) {
        if (node->node_type == parser::NodeType::PACKAGE_STATEMENT) {
            auto package_statement = static_cast<parser::PackageStatement*>(node.get());
            current_package = package_statement->package_name + ".";
            package_statement->symbol_name = package_statement->package_name;
            tree.symbols[package_statement->symbol_name] = package_statement;
        } else {
            registerGlobalSymbols(tree, node.get(), current_package);
        }
    }
    
    // Pass 2: Deep Dive
    for (const auto& node : tree.nodes) {
        resolveAndCheck(tree, node.get());
    }
}

void SemanticAnalyzer::registerGlobalSymbols(parser::AstTree& tree, parser::Node* root, const std::string& prefix) {
    if (!root) return;
    std::string my_prefix = prefix;
    
    if (root->node_type == parser::NodeType::PACKAGE_STATEMENT) {
        auto package_statement = static_cast<parser::PackageStatement*>(root);
        current_package = package_statement->package_name + ".";
        package_statement->symbol_name = package_statement->package_name;
        tree.symbols[package_statement->symbol_name] = package_statement;
    } else if (root->node_type == parser::NodeType::CLASS_DECLARATION) {
        auto class_declaration = static_cast<parser::ClassDeclaration*>(root);
        std::string full_name = prefix + class_declaration->class_name;
        if (tree.symbols.count(full_name)) throw std::runtime_error("Duplicate global symbol: " + full_name);
        class_declaration->symbol_name = full_name;
        tree.symbols[full_name] = class_declaration;
        my_prefix = full_name + ".";
    } else if (root->node_type == parser::NodeType::ENUM_DECLARATION) {
        auto enm = static_cast<parser::EnumDeclaration*>(root);
        std::string full_name = prefix + enm->enum_name;
        if (tree.symbols.count(full_name)) throw std::runtime_error("Duplicate global symbol: " + full_name);
        enm->symbol_name = full_name;
        tree.symbols[full_name] = enm;
        my_prefix = full_name + ".";
    } else if (root->node_type == parser::NodeType::ALIAS_STATEMENT) {
        auto alias = static_cast<parser::AliasStatement*>(root);
        std::string full_name = prefix + alias->alias_name;
        if (tree.symbols.count(full_name)) throw std::runtime_error("Duplicate global symbol: " + full_name);
        alias->symbol_name = full_name;
        tree.symbols[full_name] = alias;
    } else if (root->node_type == parser::NodeType::FIELD_DECLARATION) {
        auto field = static_cast<parser::FieldDeclaration*>(root);
        std::string full_name = prefix + field->field_name;
        if (tree.symbols.count(full_name)) throw std::runtime_error("Duplicate field symbol: " + full_name);
        field->symbol_name = full_name;
        tree.symbols[full_name] = field;
    } else if (root->node_type == parser::NodeType::METHOD_DECLARATION) {
        auto method = static_cast<parser::MethodDeclaration*>(root);
        std::string full_name = prefix + method->method_name + generate_uuid();
        method->symbol_name = full_name;
        tree.symbols[full_name] = method;
        my_prefix = prefix + method->method_name + ".";
    } else if (root->node_type == parser::NodeType::CONSTRUCTOR_DECLARATION) {
        auto ctor = static_cast<parser::ConstructorDeclaration*>(root);
        std::string full_name = prefix + "constructor" + generate_uuid();
        ctor->symbol_name = full_name;
        tree.symbols[full_name] = ctor;
        my_prefix = prefix + "constructor.";
    }
    
    // Only go into children if iterator's a structural class/enum node! (Do not enter method bodies in pass 1)
    if (root->node_type == parser::NodeType::CLASS_DECLARATION || root->node_type == parser::NodeType::ENUM_DECLARATION) {
        for (const auto& child : root->children) {
            registerGlobalSymbols(tree, child.get(), my_prefix);
        }
    }
}

void SemanticAnalyzer::resolveAndCheck(parser::AstTree& tree, parser::Node* root) {
    if (!root) return;
    
    bool is_scope_creator = (root->node_type == parser::NodeType::BLOCK_STATEMENT ||
                             root->node_type == parser::NodeType::METHOD_DECLARATION ||
                             root->node_type == parser::NodeType::CONSTRUCTOR_DECLARATION);
    
    if (is_scope_creator) pushScope();
    
    if (root->node_type == parser::NodeType::CLASS_DECLARATION) {
        current_class = static_cast<parser::ClassDeclaration*>(root);
    } else if (root->node_type == parser::NodeType::METHOD_DECLARATION) {
        current_method = static_cast<parser::MethodDeclaration*>(root);
        for (const auto& param : current_method->parameters) {
            declareLocal(param.name, root, {});
        }
    } else if (root->node_type == parser::NodeType::CONSTRUCTOR_DECLARATION) {
        auto ctor = static_cast<parser::ConstructorDeclaration*>(root);
        for (const auto& param : ctor->parameters) {
            declareLocal(param.name, root, {});
        }
    } else if (root->node_type == parser::NodeType::VARIABLE_DECLARATION) {
        auto variable_declaration = static_cast<parser::VariableDeclaration*>(root);
        declareLocal(variable_declaration->var_name, root, {});
        
        // Generate UUID for local variable_declaration
        variable_declaration->symbol_name = current_package + (current_class ? current_class->class_name + "." : "") + variable_declaration->var_name + generate_uuid();
        tree.symbols[variable_declaration->symbol_name] = variable_declaration;
        
        TypeInfo type_info_result = resolveType(tree, variable_declaration->type_name, {});
        variable_declaration->resolved_type = type_info_result.base_name;
        variable_declaration->resolved_array_depth = type_info_result.array_depth;
        
        // If iterator has an initializer, check iterator
        if (variable_declaration->initializer) {
            TypeInfo initializer_type = evaluateExpression(tree, variable_declaration->initializer.get());
            if (initializer_type.base_name != variable_declaration->resolved_type || initializer_type.array_depth != variable_declaration->resolved_array_depth) {
                throw std::runtime_error("Type mismatch in assignment");
            }
        }
    } else if (root->node_type == parser::NodeType::FOR_STATEMENT) {
        loop_depth++;
    } else if (root->node_type == parser::NodeType::WHILE_STATEMENT) {
        loop_depth++;
    } else if (root->node_type == parser::NodeType::DO_WHILE_STATEMENT) {
        loop_depth++;
    } else if (root->node_type == parser::NodeType::BREAK_STATEMENT || root->node_type == parser::NodeType::CONTINUE_STATEMENT) {
        if (loop_depth == 0) {
            throw std::runtime_error("break/continue statement outside of loop");
        }
    }
    
    // Visit structural children
    for (const auto& child : root->children) {
        resolveAndCheck(tree, child.get());
    }
    
    // Visit specific branches
    if (root->node_type == parser::NodeType::IF_STATEMENT) {
        auto if_stmt = static_cast<parser::IfStatement*>(root);
        if (if_stmt->condition) evaluateExpression(tree, if_stmt->condition.get());
        if (if_stmt->then_branch) resolveAndCheck(tree, if_stmt->then_branch.get());
        if (if_stmt->else_branch) resolveAndCheck(tree, if_stmt->else_branch.get());
    } else if (root->node_type == parser::NodeType::FOR_STATEMENT) {
        auto for_stmt = static_cast<parser::ForStatement*>(root);
        if (for_stmt->initialization) resolveAndCheck(tree, for_stmt->initialization.get());
        if (for_stmt->condition) evaluateExpression(tree, for_stmt->condition.get());
        if (for_stmt->iteration) evaluateExpression(tree, for_stmt->iteration.get());
        if (for_stmt->body) resolveAndCheck(tree, for_stmt->body.get());
        loop_depth--;
    } else if (root->node_type == parser::NodeType::WHILE_STATEMENT) {
        auto while_stmt = static_cast<parser::WhileStatement*>(root);
        if (while_stmt->condition) evaluateExpression(tree, while_stmt->condition.get());
        if (while_stmt->body) resolveAndCheck(tree, while_stmt->body.get());
        loop_depth--;
    } else if (root->node_type == parser::NodeType::DO_WHILE_STATEMENT) {
        auto dowhile_stmt = static_cast<parser::DoWhileStatement*>(root);
        if (dowhile_stmt->body) resolveAndCheck(tree, dowhile_stmt->body.get());
        if (dowhile_stmt->condition) evaluateExpression(tree, dowhile_stmt->condition.get());
        loop_depth--;
    } else if (root->node_type == parser::NodeType::EXPRESSION_STATEMENT) {
        auto expr_stmt = static_cast<parser::ExpressionStatement*>(root);
        if (expr_stmt->expression) evaluateExpression(tree, expr_stmt->expression.get());
    } else if (root->node_type == parser::NodeType::RETURN_STATEMENT) {
        auto ret_stmt = static_cast<parser::ReturnStatement*>(root);
        if (ret_stmt->value) {
            TypeInfo val_type = evaluateExpression(tree, ret_stmt->value.get());
            if (current_method) {
                TypeInfo expected_type = resolveType(tree, current_method->return_type, {});
                if (val_type != expected_type) throw std::runtime_error("Return type mismatch");
            }
        }
    }
    
    if (is_scope_creator) popScope();
}

TypeInfo SemanticAnalyzer::resolveType(parser::AstTree& tree, const std::string& raw_type_name, const std::vector<lexer::Token>& tokens) {
    TypeInfo info;
    std::string type_str = raw_type_name;
    
    while (type_str.length() >= 2 && type_str.substr(type_str.length() - 2) == "[]") {
        info.array_depth++;
        type_str = type_str.substr(0, type_str.length() - 2);
    }
    
    while (!type_str.empty() && type_str.back() == ' ') type_str.pop_back();
    
    if (type_str == "void" || type_str == "bool" || 
        type_str == "int8" || type_str == "int16" || type_str == "int32" || type_str == "int64" ||
        type_str == "uint8" || type_str == "uint16" || type_str == "uint32" || type_str == "uint64" ||
        type_str == "float32" || type_str == "float64" || 
        type_str == "char") {
        info.base_name = type_str;
        info.is_primitive = true;
    } else if (type_str == "string") {
        info.base_name = type_str;
        info.is_primitive = false;
    } else {
        parser::Node* symbol = nullptr;
        std::string full_name = current_package + type_str;
        if (tree.symbols.count(full_name)) {
            symbol = tree.symbols[full_name];
            info.base_name = full_name;
        } else if (tree.symbols.count(type_str)) {
            symbol = tree.symbols[type_str];
            info.base_name = type_str;
        } else {
            throw std::runtime_error("Undefined type: " + type_str);
        }

        if (symbol->node_type == parser::NodeType::ALIAS_STATEMENT) {
            auto alias = static_cast<parser::AliasStatement*>(symbol);
            TypeInfo resolved = resolveType(tree, alias->target_type, tokens);
            resolved.array_depth += info.array_depth;
            return resolved;
        }
        info.class_ref = symbol;
    }
    return info;
}

TypeInfo SemanticAnalyzer::evaluateExpression(parser::AstTree& tree, parser::Node* expr) {
    TypeInfo result;
    if (!expr) return result;
    
    if (expr->node_type == parser::NodeType::LITERAL_EXPRESSION) {
        auto lit = static_cast<parser::LiteralExpression*>(expr);
        if (lit->token.type == lexer::TokenType::NUMBER) {
            if (lit->token.value.value_or("").find('.') != std::string::npos) result.base_name = "float64";
            else result.base_name = "int32";
            result.is_primitive = true;
        }
        else if (lit->token.type == lexer::TokenType::STRING) {
            result.base_name = "string";
            result.is_primitive = false;
        }
        else if (lit->token.type == lexer::TokenType::IDENTIFIER && (lit->token.value == "true" || lit->token.value == "false")) {
            result.base_name = "bool";
            result.is_primitive = true;
        }
    } else if (expr->node_type == parser::NodeType::IDENTIFIER_EXPRESSION) {
        auto identifier_expr = static_cast<parser::IdentifierExpression*>(expr);
        if (identifier_expr->name == "true" || identifier_expr->name == "false") {
            result.base_name = "bool";
            result.is_primitive = true;
        } else if (identifier_expr->name == "null") {
            result.base_name = "null";
            result.is_primitive = false;
        } else if (identifier_expr->name == "this") {
            if (!current_class) throw std::runtime_error("Cannot use 'this' outside of a class");
            result.base_name = current_package + current_class->class_name;
            result.class_ref = current_class;
        } else {
            parser::Node* declaration_node = lookupSymbol(tree, identifier_expr->name);
            if (!declaration_node) throw std::runtime_error("Undefined variable: " + identifier_expr->name);
            identifier_expr->resolved_declaration = declaration_node;
            result.base_name = declaration_node->resolved_type;
            result.array_depth = declaration_node->resolved_array_depth;
            // Also handle fields
            if (declaration_node->node_type == parser::NodeType::FIELD_DECLARATION) {
                enforceAccessModifier(declaration_node, {});
            } else if (declaration_node->node_type == parser::NodeType::METHOD_DECLARATION) {
                result.is_method = true;
                result.method_ref = declaration_node;
            }
        }
    } else if (expr->node_type == parser::NodeType::BINARY_EXPRESSION) {
        auto binary_expr = static_cast<parser::BinaryExpression*>(expr);
        TypeInfo left = evaluateExpression(tree, binary_expr->left.get());
        TypeInfo right = evaluateExpression(tree, binary_expr->right.get());
        if (left.base_name == "float64" || right.base_name == "float64") result.base_name = "float64";
        else result.base_name = "int32";
        result.is_primitive = true;
    } else if (expr->node_type == parser::NodeType::ASSIGNMENT_EXPRESSION) {
        auto assignment_expr = static_cast<parser::AssignmentExpression*>(expr);
        enforceLValue(assignment_expr->target.get(), {});
        TypeInfo target = evaluateExpression(tree, assignment_expr->target.get());
        TypeInfo val = evaluateExpression(tree, assignment_expr->value.get());
        if (target.base_name != val.base_name || target.array_depth != val.array_depth) {
            throw std::runtime_error("Type mismatch in assignment");
        }
        result = target;
    } else if (expr->node_type == parser::NodeType::MEMBER_ACCESS_EXPRESSION) {
        auto member_access_expr = static_cast<parser::MemberAccessExpression*>(expr);
        TypeInfo object_type_info = evaluateExpression(tree, member_access_expr->object.get());
        if (object_type_info.is_primitive || !tree.symbols.count(object_type_info.base_name)) throw std::runtime_error("Cannot access member on primitive or undefined type");
        auto class_declaration = static_cast<parser::ClassDeclaration*>(tree.symbols[object_type_info.base_name]);
        parser::Node* found_member = nullptr;
        for (const auto& child : class_declaration->children) {
            if (child->node_type == parser::NodeType::FIELD_DECLARATION) {
                auto field = static_cast<parser::FieldDeclaration*>(child.get());
                if (field->field_name == member_access_expr->member_name) { found_member = field; break; }
            } else if (child->node_type == parser::NodeType::METHOD_DECLARATION) {
                auto method = static_cast<parser::MethodDeclaration*>(child.get());
                if (method->method_name == member_access_expr->member_name) { found_member = method; break; }
            }
        }
        if (!found_member) throw std::runtime_error("Undefined member: " + member_access_expr->member_name);
        enforceAccessModifier(found_member, {});
        member_access_expr->resolved_declaration = found_member;
        
        if (found_member->node_type == parser::NodeType::FIELD_DECLARATION) {
            auto field_declaration = static_cast<parser::FieldDeclaration*>(found_member);
            TypeInfo field_type_info = resolveType(tree, field_declaration->type_name, {});
            result = field_type_info;
        } else {
            auto method_declaration = static_cast<parser::MethodDeclaration*>(found_member);
            result.is_method = true;
            result.method_ref = method_declaration;
        }
    } else if (expr->node_type == parser::NodeType::ARRAY_ACCESS_EXPRESSION) {
        auto array_access_expr = static_cast<parser::ArrayAccessExpression*>(expr);
        TypeInfo target = evaluateExpression(tree, array_access_expr->array.get());
        if (target.array_depth == 0) throw std::runtime_error("Cannot index into non-array type");
        TypeInfo index = evaluateExpression(tree, array_access_expr->index.get());
        if (index.base_name != "int32") throw std::runtime_error("Array index must be int32");
        result = target;
        result.array_depth--;
    } else if (expr->node_type == parser::NodeType::CALL_EXPRESSION) {
        auto call = static_cast<parser::CallExpression*>(expr);
        TypeInfo target = evaluateExpression(tree, call->callee.get());
        if (!target.is_method) throw std::runtime_error("Attempted to call a non-method");
        auto method_declaration = static_cast<parser::MethodDeclaration*>(target.method_ref);
        if (call->arguments.size() != method_declaration->parameters.size()) throw std::runtime_error("Argument count mismatch");
        for (size_t index = 0; index < call->arguments.size(); index++) {
            TypeInfo argument_type = evaluateExpression(tree, call->arguments[index].get());
            TypeInfo parameter_type_info = resolveType(tree, method_declaration->parameters[index].type, {});
            if (argument_type != parameter_type_info) throw std::runtime_error("Argument type mismatch");
        }
        result = resolveType(tree, method_declaration->return_type, {});
    } else if (expr->node_type == parser::NodeType::UNARY_EXPRESSION) {
        auto uny = static_cast<parser::UnaryExpression*>(expr);
        result = evaluateExpression(tree, uny->operand.get());
    } else if (expr->node_type == parser::NodeType::TERNARY_EXPRESSION) {
        auto ter = static_cast<parser::TernaryExpression*>(expr);
        evaluateExpression(tree, ter->condition.get());
        result = evaluateExpression(tree, ter->true_branch.get());
        evaluateExpression(tree, ter->false_branch.get());
    } else if (expr->node_type == parser::NodeType::CAST_EXPRESSION) {
        auto cst = static_cast<parser::CastExpression*>(expr);
        evaluateExpression(tree, cst->expression.get());
        result = resolveType(tree, cst->target_type, {});
    } else if (expr->node_type == parser::NodeType::NEW_INSTANCE_EXPRESSION) {
        auto inst = static_cast<parser::NewInstanceExpression*>(expr);
        for (auto& arg : inst->arguments) evaluateExpression(tree, arg.get());
        result = resolveType(tree, inst->class_name, {});
    } else if (expr->node_type == parser::NodeType::ARRAY_CREATION_EXPRESSION) {
        auto ac = static_cast<parser::ArrayCreationExpression*>(expr);
        evaluateExpression(tree, ac->size.get());
        result = resolveType(tree, ac->type_name, {});
        result.array_depth++;
    } else if (expr->node_type == parser::NodeType::ARRAY_LITERAL_EXPRESSION) {
        auto array_literal = static_cast<parser::ArrayLiteralExpression*>(expr);
        if (!array_literal->elements.empty()) {
            result = evaluateExpression(tree, array_literal->elements[0].get());
            result.array_depth++;
            
            // Type-check remaining elements
            for (size_t i = 1; i < array_literal->elements.size(); i++) {
                TypeInfo element_type = evaluateExpression(tree, array_literal->elements[i].get());
                if (element_type.base_name != result.base_name || element_type.array_depth != result.array_depth - 1) {
                    throw std::runtime_error("Array literal elements must have consistent types");
                }
            }
        }
    }
    
    expr->resolved_type = result.base_name;
    expr->resolved_array_depth = result.array_depth;
    return result;
}

void SemanticAnalyzer::enforceAccessModifier(parser::Node* target_node, const std::vector<lexer::Token>& tokens) {
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
}

} // namespace semantic
} // namespace solix
