#include "solix/semantic.hpp"
#include <random>
#include <sstream>
#include <iomanip>

namespace solix {
namespace semantic {

std::string TypeInfo::to_string() const {
    std::string res = "";
    for (int i = 0; i < array_depth; i++) res += "array ";
    res += base_name;
    return res;
}

static std::string generate_uuid() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    std::stringstream ss;
    ss << "<";
    for (int i = 0; i < 8; i++) {
        ss << std::hex << dis(gen);
    }
    ss << ">";
    return ss.str();
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
    for (auto it = scope_stack.rbegin(); it != scope_stack.rend(); ++it) {
        if (it->symbols.count(name)) {
            return it->symbols.at(name);
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
    // For now, if we don't find it, we return nullptr. We can improve global lookups later.
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
        registerGlobalSymbols(tree, node.get(), "");
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
        auto pkg = static_cast<parser::PackageStatement*>(root);
        my_prefix = pkg->package_name + ".";
        pkg->symbol_name = pkg->package_name;
        tree.symbols[pkg->symbol_name] = pkg;
        current_package = my_prefix;
    } else if (root->node_type == parser::NodeType::CLASS_DECLARATION) {
        auto cls = static_cast<parser::ClassDeclaration*>(root);
        std::string full_name = prefix + cls->class_name;
        if (tree.symbols.count(full_name)) throw std::runtime_error("Duplicate global symbol: " + full_name);
        cls->symbol_name = full_name;
        tree.symbols[full_name] = cls;
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
    
    // Only go into children if it's a structural class/enum node! (Do not enter method bodies in pass 1)
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
        auto var = static_cast<parser::VariableDeclaration*>(root);
        declareLocal(var->var_name, root, {});
        
        // Generate UUID for local var
        var->symbol_name = current_package + (current_class ? current_class->class_name + "." : "") + var->var_name + generate_uuid();
        tree.symbols[var->symbol_name] = var;
        
        // If it has an initializer, check it
        if (var->initializer) {
            evaluateExpression(tree, var->initializer.get());
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
    }
    
    if (is_scope_creator) popScope();
}

TypeInfo SemanticAnalyzer::resolveType(parser::AstTree& tree, const std::string& raw_type_name, const std::vector<lexer::Token>& tokens) {
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
}

TypeInfo SemanticAnalyzer::evaluateExpression(parser::AstTree& tree, parser::Node* expr) {
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
        parser::Node* decl = lookupSymbol(tree, ident->name);
        if (!decl) throw std::runtime_error("Undefined variable: " + ident->name);
        ident->resolved_declaration = decl;
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
}

void SemanticAnalyzer::enforceAccessModifier(parser::Node* target_node, const std::vector<lexer::Token>& tokens) {
    // Basic access modifier placeholder
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
