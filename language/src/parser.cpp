#include "solix/parser.hpp"
#include "solix/lexer.hpp"
#include <stdexcept>
#include <iostream>
#include <algorithm>

namespace solix::parser {
Node::Node(const std::vector<lexer::Token>& tokens, NodeType type, Node* parent) : node_type(type), parent_node(parent) {
    if (!tokens.empty()) {
        if (tokens.front().path.has_value()) file_path = tokens.front().path.value();
        line = tokens.front().line;
        column = tokens.front().column;
    }
}


// ==========================================
// Error Handling & Helpers
// ==========================================

[[noreturn]] void throw_parse_error(const lexer::Token& tok, const std::string& msg) {
    std::string err = "[Solix Parser Error] ";
    if (tok.path.has_value()) {
        err += tok.path.value().string() + ":";
    }
    err += std::to_string(tok.line) + ":" + std::to_string(tok.column);
    err += " - " + msg;
    if (tok.value.has_value()) {
        err += " (At token: '" + std::string(tok.value.value()) + "')";
    }
    throw std::runtime_error(err);
}

[[noreturn]] void throw_parse_error(const std::vector<lexer::Token>& tokens, const std::string& msg) {
    if (tokens.empty()) throw std::runtime_error("Parse Error: " + msg + " (Unexpected EOF)");
    throw_parse_error(tokens[0], msg);
}

std::vector<lexer::Token> strip_parentheses(const std::vector<lexer::Token>& tokens) {
    if (tokens.size() < 2) return tokens;
    if (tokens.front().type == lexer::TokenType::PUNCTUATION_OPEN_PAREN &&
        tokens.back().type == lexer::TokenType::PUNCTUATION_CLOSE_PAREN) {
        
        int depth = 0;
        bool wraps_entire = true;
        for (size_t i = 0; i < tokens.size(); i++) {
            
        if (tokens[i].type == lexer::TokenType::PUNCTUATION_OPEN_PAREN) depth++;
            else if (tokens[i].type == lexer::TokenType::PUNCTUATION_CLOSE_PAREN) depth--;
            
            if (depth < 0) throw_parse_error(tokens[i], "Mismatched parentheses: unexpected ')'");
            if (depth == 0 && i < tokens.size() - 1) {
                wraps_entire = false;
                break;
            }
        }
        if (depth > 0) throw_parse_error(tokens.back(), "Mismatched parentheses: expected ')'");
        
        if (wraps_entire) {
            std::vector<lexer::Token> inner(tokens.begin() + 1, tokens.end() - 1);
            return strip_parentheses(inner);
        }
    }
    return tokens;
}

std::vector<std::vector<lexer::Token>> divideTokensIntoStatements(const std::vector<lexer::Token>& tokens) {
    std::vector<std::vector<lexer::Token>> statements;
    std::vector<lexer::Token> current;
    int p = 0, b = 0, br = 0;
    
    for (size_t i = 0; i < tokens.size(); i++) {
        const auto& tok = tokens[i];
        current.push_back(tok);
        
        if (tok.type == lexer::TokenType::PUNCTUATION_OPEN_PAREN) p++;
        else if (tok.type == lexer::TokenType::PUNCTUATION_CLOSE_PAREN) p--;
        else if (tok.type == lexer::TokenType::PUNCTUATION_OPEN_BRACE) b++;
        else if (tok.type == lexer::TokenType::PUNCTUATION_CLOSE_BRACE) b--;
        else if (tok.type == lexer::TokenType::PUNCTUATION_OPEN_BRACKET) br++;
        else if (tok.type == lexer::TokenType::PUNCTUATION_CLOSE_BRACKET) br--;
        
        if (p < 0 || b < 0 || br < 0) {
            throw_parse_error(tok, "Mismatched grouping symbol");
        }
        
        if (p == 0 && b == 0 && br == 0) {
            if (tok.type == lexer::TokenType::PUNCTUATION_SEMICOLON) {
                statements.push_back(current);
                current.clear();
            } else if (tok.type == lexer::TokenType::PUNCTUATION_CLOSE_BRACE) {
                bool split = true;
                if (i + 1 < tokens.size()) {
                    auto next_type = tokens[i+1].type;
                    if (next_type == lexer::TokenType::KEYWORD_ELSE || 
                        (next_type == lexer::TokenType::KEYWORD_WHILE && current.front().type == lexer::TokenType::IDENTIFIER)) {
                        split = false;
                    }
                }
                if (split) {
                    statements.push_back(current);
                    current.clear();
                }
            }
        }
    }
    if (p != 0 || b != 0 || br != 0) {
        throw_parse_error(tokens.back(), "Mismatched braces or parentheses in block");
    }
    if (!current.empty()) statements.push_back(current);
    
    return statements;
}

size_t consumeType(const std::vector<lexer::Token>& tokens, size_t start_index) {
    size_t i = start_index;
    while (i < tokens.size() && tokens[i].type == lexer::TokenType::PRIMITIVE_ARRAY) {
        i++;
    }
    if (i >= tokens.size()) throw_parse_error(tokens, "Expected type after 'array'");
    
    // Accept primitive types or identifiers
    if (tokens[i].type != lexer::TokenType::IDENTIFIER &&
        tokens[i].type != lexer::TokenType::PRIMITIVE_INT32 &&
        tokens[i].type != lexer::TokenType::PRIMITIVE_FLOAT64 &&
        tokens[i].type != lexer::TokenType::PRIMITIVE_BOOL &&
        tokens[i].type != lexer::TokenType::PRIMITIVE_STRING &&
        tokens[i].type != lexer::TokenType::PRIMITIVE_INT8 &&
        tokens[i].type != lexer::TokenType::PRIMITIVE_INT16 &&
        tokens[i].type != lexer::TokenType::PRIMITIVE_INT64 &&
        tokens[i].type != lexer::TokenType::PRIMITIVE_UINT8 &&
        tokens[i].type != lexer::TokenType::PRIMITIVE_UINT16 &&
        tokens[i].type != lexer::TokenType::PRIMITIVE_UINT32 &&
        tokens[i].type != lexer::TokenType::PRIMITIVE_UINT64 &&
        tokens[i].type != lexer::TokenType::PRIMITIVE_FLOAT32 &&
        tokens[i].type != lexer::TokenType::PRIMITIVE_CHAR &&
        tokens[i].type != lexer::TokenType::PRIMITIVE_VOID) {
        return start_index; // Not a type
    }
    i++;
    
    while (i < tokens.size() && tokens[i].type == lexer::TokenType::PUNCTUATION_DOT) {
        i++;
        if (i >= tokens.size() || tokens[i].type != lexer::TokenType::IDENTIFIER) {
            throw_parse_error(tokens[i - 1], "Expected identifier after '.' in type");
        }
        i++;
    }
    return i;
}

// ==========================================
// Core Parser Logic
// ==========================================

const NodeType determineNodeType(const std::vector<lexer::Token>& raw_tokens) {
    if (raw_tokens.empty()) return NodeType::GENERIC;
    std::vector<lexer::Token> tokens = strip_parentheses(raw_tokens);
    if (tokens.empty()) return NodeType::GENERIC;
    
    auto t0 = tokens[0].type;
    
    // Top-Level
    if (t0 == lexer::TokenType::KEYWORD_PACKAGE) return NodeType::PACKAGE_STATEMENT;
    if (t0 == lexer::TokenType::KEYWORD_ALIAS) return NodeType::ALIAS_STATEMENT;
    
    for (int i = 0; i < tokens.size(); i++) {
        if (tokens[i].type == lexer::TokenType::KEYWORD_ENUM) return NodeType::ENUM_DECLARATION;
        if (tokens[i].type == lexer::TokenType::KEYWORD_CLASS) return NodeType::CLASS_DECLARATION;
        // Stop checking deep for enum/class
        if (tokens[i].type == lexer::TokenType::PUNCTUATION_OPEN_BRACE || 
            tokens[i].type == lexer::TokenType::PUNCTUATION_OPEN_PAREN ||
            tokens[i].type == lexer::TokenType::PUNCTUATION_SEMICOLON) break;
    }
    
    // Control Flow
    if (t0 == lexer::TokenType::KEYWORD_IF) return NodeType::IF_STATEMENT;
    if (t0 == lexer::TokenType::KEYWORD_FOR) return NodeType::FOR_STATEMENT;
    if (t0 == lexer::TokenType::KEYWORD_WHILE) return NodeType::WHILE_STATEMENT;
    if (t0 == lexer::TokenType::KEYWORD_SWITCH) return NodeType::SWITCH_STATEMENT;
    if (t0 == lexer::TokenType::KEYWORD_CASE || t0 == lexer::TokenType::KEYWORD_DEFAULT) return NodeType::CASE_STATEMENT;
    if (t0 == lexer::TokenType::KEYWORD_BREAK) return NodeType::BREAK_STATEMENT;
    if (t0 == lexer::TokenType::KEYWORD_CONTINUE) return NodeType::CONTINUE_STATEMENT;
    if (t0 == lexer::TokenType::KEYWORD_RETURN) return NodeType::RETURN_STATEMENT;
    if (t0 == lexer::TokenType::IDENTIFIER && tokens.size() > 1 && tokens[0].value.value_or("") == "do" && tokens[1].type == lexer::TokenType::PUNCTUATION_OPEN_BRACE) return NodeType::DO_WHILE_STATEMENT;
    
    // Methods / Constructors
    if (tokens.back().type == lexer::TokenType::PUNCTUATION_CLOSE_BRACE) {
        int brace_depth = 0;
        int p_depth = 0;
        int body_start = -1;
        int param_end = -1;
        int param_start = -1;
        
        for (int i = tokens.size() - 1; i >= 0; i--) {
            if (tokens[i].type == lexer::TokenType::PUNCTUATION_CLOSE_BRACE) brace_depth++;
            else if (tokens[i].type == lexer::TokenType::PUNCTUATION_OPEN_BRACE) brace_depth--;
            
            if (brace_depth == 0 && body_start == -1) {
                body_start = i;
                if (i > 0 && tokens[i-1].type == lexer::TokenType::PUNCTUATION_CLOSE_PAREN) {
                    param_end = i - 1;
                }
            }
            if (body_start != -1 && param_end != -1 && param_start == -1 && i <= param_end) {
                if (tokens[i].type == lexer::TokenType::PUNCTUATION_CLOSE_PAREN) p_depth++;
                else if (tokens[i].type == lexer::TokenType::PUNCTUATION_OPEN_PAREN) p_depth--;
                if (p_depth == 0 && tokens[i].type == lexer::TokenType::PUNCTUATION_OPEN_PAREN) param_start = i;
            }
        }
        
        if (param_start > 0) {
            if (tokens[param_start-1].type == lexer::TokenType::IDENTIFIER) {
                // Modifiers and Return Type check
                bool has_return_type = false;
                for (int i = param_start - 2; i >= 0; i--) {
                    auto t = tokens[i].type;
                    if (t != lexer::TokenType::KEYWORD_PUBLIC && t != lexer::TokenType::KEYWORD_PRIVATE &&
                        t != lexer::TokenType::KEYWORD_PROTECTED && t != lexer::TokenType::KEYWORD_INTERNAL &&
                        t != lexer::TokenType::KEYWORD_STATIC && t != lexer::TokenType::KEYWORD_CONST &&
                        t != lexer::TokenType::KEYWORD_INLINE) {
                        has_return_type = true;
                        break;
                    }
                }
                std::cout << "METHOD!\n";
                    if (has_return_type) return NodeType::METHOD_DECLARATION;
                std::cout << "CTOR!\n";
                    return NodeType::CONSTRUCTOR_DECLARATION;
            }
        }
    }
    
    // Block
    if (tokens[0].type == lexer::TokenType::PUNCTUATION_OPEN_BRACE && tokens.back().type == lexer::TokenType::PUNCTUATION_CLOSE_BRACE) {
        return NodeType::BLOCK_STATEMENT;
    }
    
    // Variable / Field Declaration
    size_t i = 0;
    while (i < tokens.size() && (
        tokens[i].type == lexer::TokenType::KEYWORD_PUBLIC ||
        tokens[i].type == lexer::TokenType::KEYWORD_PRIVATE ||
        tokens[i].type == lexer::TokenType::KEYWORD_PROTECTED ||
        tokens[i].type == lexer::TokenType::KEYWORD_INTERNAL ||
        tokens[i].type == lexer::TokenType::KEYWORD_STATIC ||
        tokens[i].type == lexer::TokenType::KEYWORD_CONST ||
        tokens[i].type == lexer::TokenType::KEYWORD_INLINE)) {
        i++;
    }
    
    size_t after_type = consumeType(tokens, i);
    if (after_type > i && after_type < tokens.size() && tokens[after_type].type == lexer::TokenType::IDENTIFIER) {
        bool has_field_modifier = false;
        for (size_t m = 0; m < i; m++) {
            if (tokens[m].type != lexer::TokenType::KEYWORD_CONST) has_field_modifier = true;
        }
        if (has_field_modifier) return NodeType::FIELD_DECLARATION;
        return NodeType::VARIABLE_DECLARATION; 
    }
    
    if (tokens.back().type == lexer::TokenType::PUNCTUATION_SEMICOLON) {
        return NodeType::EXPRESSION_STATEMENT;
    }
    
    // Expressions
    // Check Assignment (LEFT TO RIGHT at depth 0)
    int p = 0, b = 0, br = 0;
    for (int i = 0; i < tokens.size(); i++) {
        auto t = tokens[i].type;
        if (t == lexer::TokenType::PUNCTUATION_OPEN_PAREN) p++;
        else if (t == lexer::TokenType::PUNCTUATION_CLOSE_PAREN) p--;
        else if (t == lexer::TokenType::PUNCTUATION_OPEN_BRACE) b++;
        else if (t == lexer::TokenType::PUNCTUATION_CLOSE_BRACE) b--;
        else if (t == lexer::TokenType::PUNCTUATION_OPEN_BRACKET) br++;
        else if (t == lexer::TokenType::PUNCTUATION_CLOSE_BRACKET) br--;
        
        if (p == 0 && b == 0 && br == 0 && t == lexer::TokenType::OPERATOR_ASSIGN) {
            return NodeType::ASSIGNMENT_EXPRESSION;
        }
    }
    
    
    
    // Check Binary (RIGHT TO LEFT)
    // Precedence: (Lower number = evaluated later = higher up in AST)
    auto getPrecedence = [](lexer::TokenType t) -> int {
        switch (t) {
            case lexer::TokenType::OPERATOR_LOGICAL_OR: return 1;
            case lexer::TokenType::OPERATOR_LOGICAL_AND: return 2;
            
            
            
            case lexer::TokenType::OPERATOR_EQUAL:
            case lexer::TokenType::OPERATOR_NOT_EQUAL: return 6;
            case lexer::TokenType::OPERATOR_LESS_THAN:
                        case lexer::TokenType::OPERATOR_GREATER_THAN:
            
                        
            case lexer::TokenType::OPERATOR_PLUS:
            case lexer::TokenType::OPERATOR_MINUS: return 9;
            case lexer::TokenType::OPERATOR_MULTIPLY:
            case lexer::TokenType::OPERATOR_DIVIDE:
            case lexer::TokenType::OPERATOR_MODULO: return 10;
            default: return 0;
        }
    };
    
    p = 0; b = 0; br = 0;
    int min_prec = 100;
    int min_idx = -1;
    for (int i = tokens.size() - 1; i >= 0; i--) {
        auto t = tokens[i].type;
        if (t == lexer::TokenType::PUNCTUATION_CLOSE_PAREN) p++;
        else if (t == lexer::TokenType::PUNCTUATION_OPEN_PAREN) p--;
        else if (t == lexer::TokenType::PUNCTUATION_CLOSE_BRACE) b++;
        else if (t == lexer::TokenType::PUNCTUATION_OPEN_BRACE) b--;
        else if (t == lexer::TokenType::PUNCTUATION_CLOSE_BRACKET) br++;
        else if (t == lexer::TokenType::PUNCTUATION_OPEN_BRACKET) br--;
        
        if (p == 0 && b == 0 && br == 0) {
            int prec = getPrecedence(t);
            if (prec > 0 && prec < min_prec) {
                min_prec = prec;
                min_idx = i;
            }
        }
    }
    if (min_idx != -1) return NodeType::BINARY_EXPRESSION;
    
    // Unary
    if (t0 == lexer::TokenType::OPERATOR_LOGICAL_NOT || t0 == lexer::TokenType::OPERATOR_MINUS ||
        t0 == lexer::TokenType::OPERATOR_INCREMENT || t0 == lexer::TokenType::OPERATOR_DECREMENT) {
        return NodeType::UNARY_EXPRESSION;
    }
    if (tokens.back().type == lexer::TokenType::OPERATOR_INCREMENT || tokens.back().type == lexer::TokenType::OPERATOR_DECREMENT) {
        return NodeType::UNARY_EXPRESSION;
    }
    
    // Postfix Call and Array Access
    if (tokens.back().type == lexer::TokenType::PUNCTUATION_CLOSE_PAREN) {
        if (t0 == lexer::TokenType::PUNCTUATION_OPEN_PAREN && tokens[1].type != lexer::TokenType::PUNCTUATION_CLOSE_PAREN) {
            // Might be a cast (Type) expr
            return NodeType::CAST_EXPRESSION;
        }
        return NodeType::CALL_EXPRESSION;
    }
    if (tokens.back().type == lexer::TokenType::PUNCTUATION_CLOSE_BRACKET) {
        return NodeType::ARRAY_ACCESS_EXPRESSION;
    }
    
    // Member Access (RIGHT TO LEFT)
    p = 0; b = 0; br = 0;
    for (int i = tokens.size() - 1; i >= 0; i--) {
        auto t = tokens[i].type;
        if (t == lexer::TokenType::PUNCTUATION_CLOSE_PAREN) p++;
        else if (t == lexer::TokenType::PUNCTUATION_OPEN_PAREN) p--;
        else if (t == lexer::TokenType::PUNCTUATION_CLOSE_BRACE) b++;
        else if (t == lexer::TokenType::PUNCTUATION_OPEN_BRACE) b--;
        else if (t == lexer::TokenType::PUNCTUATION_CLOSE_BRACKET) br++;
        else if (t == lexer::TokenType::PUNCTUATION_OPEN_BRACKET) br--;
        
        if (p == 0 && b == 0 && br == 0 && t == lexer::TokenType::PUNCTUATION_DOT) {
            return NodeType::MEMBER_ACCESS_EXPRESSION;
        }
    }
    
    if (t0 == lexer::TokenType::KEYWORD_NEW) {
        if (tokens.back().type == lexer::TokenType::PUNCTUATION_CLOSE_BRACKET) return NodeType::ARRAY_CREATION_EXPRESSION;
        return NodeType::NEW_INSTANCE_EXPRESSION;
    }
    
    if (t0 == lexer::TokenType::PUNCTUATION_OPEN_BRACE && tokens.back().type == lexer::TokenType::PUNCTUATION_CLOSE_BRACE) {
        return NodeType::ARRAY_LITERAL_EXPRESSION;
    }
    
    if (tokens.size() == 1) {
        if (t0 == lexer::TokenType::NUMBER || t0 == lexer::TokenType::STRING) return NodeType::LITERAL_EXPRESSION;
        if (t0 == lexer::TokenType::IDENTIFIER) return NodeType::IDENTIFIER_EXPRESSION;
    }
    

    
    throw_parse_error(tokens, "Unable to determine AST Node Type");
}

std::unique_ptr<Node> parseTokensToNode(const std::vector<lexer::Token>& raw_tokens, Node* parent) {
    if (raw_tokens.empty()) return nullptr;
    auto tokens = strip_parentheses(raw_tokens);
    if (tokens.empty() || (tokens.size() == 1 && tokens[0].type == lexer::TokenType::EOF_TOKEN)) return nullptr;
    
    NodeType type = determineNodeType(tokens);
    switch (type) {
        case NodeType::PACKAGE_STATEMENT: return std::make_unique<PackageStatement>(tokens, parent);
        case NodeType::ALIAS_STATEMENT: return std::make_unique<AliasStatement>(tokens, parent);
        case NodeType::ENUM_DECLARATION: return std::make_unique<EnumDeclaration>(tokens, parent);
        case NodeType::CLASS_DECLARATION: return std::make_unique<ClassDeclaration>(tokens, parent);
        case NodeType::IF_STATEMENT: return std::make_unique<IfStatement>(tokens, parent);
        case NodeType::FOR_STATEMENT: return std::make_unique<ForStatement>(tokens, parent);
        case NodeType::WHILE_STATEMENT: return std::make_unique<WhileStatement>(tokens, parent);
        case NodeType::SWITCH_STATEMENT: return std::make_unique<SwitchStatement>(tokens, parent);
        case NodeType::CASE_STATEMENT: return std::make_unique<CaseStatement>(tokens, parent);
        case NodeType::BREAK_STATEMENT: return std::make_unique<BreakStatement>(tokens, parent);
        case NodeType::CONTINUE_STATEMENT: return std::make_unique<ContinueStatement>(tokens, parent);
        case NodeType::RETURN_STATEMENT: return std::make_unique<ReturnStatement>(tokens, parent);
        case NodeType::DO_WHILE_STATEMENT: return std::make_unique<DoWhileStatement>(tokens, parent);
        case NodeType::METHOD_DECLARATION: return std::make_unique<MethodDeclaration>(tokens, parent);
        case NodeType::CONSTRUCTOR_DECLARATION: return std::make_unique<ConstructorDeclaration>(tokens, parent);
        case NodeType::BLOCK_STATEMENT: return std::make_unique<BlockStatement>(tokens, parent);
        case NodeType::FIELD_DECLARATION: return std::make_unique<FieldDeclaration>(tokens, parent);
        case NodeType::VARIABLE_DECLARATION: return std::make_unique<VariableDeclaration>(tokens, parent);
        case NodeType::ASSIGNMENT_EXPRESSION: return std::make_unique<AssignmentExpression>(tokens, parent);
        case NodeType::TERNARY_EXPRESSION: return std::make_unique<TernaryExpression>(tokens, parent);
        case NodeType::BINARY_EXPRESSION: return std::make_unique<BinaryExpression>(tokens, parent);
        case NodeType::UNARY_EXPRESSION: return std::make_unique<UnaryExpression>(tokens, parent);
        case NodeType::CALL_EXPRESSION: return std::make_unique<CallExpression>(tokens, parent);
        case NodeType::ARRAY_ACCESS_EXPRESSION: return std::make_unique<ArrayAccessExpression>(tokens, parent);
        case NodeType::MEMBER_ACCESS_EXPRESSION: return std::make_unique<MemberAccessExpression>(tokens, parent);
        case NodeType::NEW_INSTANCE_EXPRESSION: return std::make_unique<NewInstanceExpression>(tokens, parent);
        case NodeType::ARRAY_CREATION_EXPRESSION: return std::make_unique<ArrayCreationExpression>(tokens, parent);
        case NodeType::ARRAY_LITERAL_EXPRESSION: return std::make_unique<ArrayLiteralExpression>(tokens, parent);
        case NodeType::CAST_EXPRESSION: return std::make_unique<CastExpression>(tokens, parent);
        case NodeType::LITERAL_EXPRESSION: return std::make_unique<LiteralExpression>(tokens, parent);
        case NodeType::IDENTIFIER_EXPRESSION: return std::make_unique<IdentifierExpression>(tokens, parent);
        case NodeType::EXPRESSION_STATEMENT: return std::make_unique<ExpressionStatement>(tokens, parent);
        default: throw_parse_error(tokens, "Unimplemented node type");
    }
}


// ==========================================
// Expression Nodes
// ==========================================

LiteralExpression::LiteralExpression(const std::vector<lexer::Token>& tokens, Node* parent) : Node(tokens, NodeType::LITERAL_EXPRESSION, parent) {
    if (tokens.size() != 1) throw_parse_error(tokens, "Literal expression must be exactly one token");
    token = tokens[0];
}

IdentifierExpression::IdentifierExpression(const std::vector<lexer::Token>& tokens, Node* parent) : Node(tokens, NodeType::IDENTIFIER_EXPRESSION, parent) {
    if (tokens.size() != 1 || tokens[0].type != lexer::TokenType::IDENTIFIER) throw_parse_error(tokens, "Identifier expression must be a single identifier token");
    name = std::string(tokens[0].value.value_or(""));
}

BinaryExpression::BinaryExpression(const std::vector<lexer::Token>& tokens, Node* parent) : Node(tokens, NodeType::BINARY_EXPRESSION, parent) {
    auto getPrecedence = [](lexer::TokenType t) -> int {
        switch (t) {
            case lexer::TokenType::OPERATOR_LOGICAL_OR: return 1;
            case lexer::TokenType::OPERATOR_LOGICAL_AND: return 2;
            case lexer::TokenType::OPERATOR_EQUAL:
            case lexer::TokenType::OPERATOR_NOT_EQUAL: return 6;
            case lexer::TokenType::OPERATOR_LESS_THAN:
            case lexer::TokenType::OPERATOR_LESS_EQUAL:
            case lexer::TokenType::OPERATOR_GREATER_THAN:
            case lexer::TokenType::OPERATOR_GREATER_EQUAL: return 7;
            case lexer::TokenType::OPERATOR_PLUS:
            case lexer::TokenType::OPERATOR_MINUS: return 9;
            case lexer::TokenType::OPERATOR_MULTIPLY:
            case lexer::TokenType::OPERATOR_DIVIDE:
            case lexer::TokenType::OPERATOR_MODULO: return 10;
            default: return 0;
        }
    };
    int p = 0, b = 0, br = 0;
    int min_prec = 100;
    int min_idx = -1;
    for (int i = tokens.size() - 1; i >= 0; i--) {
        auto t = tokens[i].type;
        if (t == lexer::TokenType::PUNCTUATION_CLOSE_PAREN) p++;
        else if (t == lexer::TokenType::PUNCTUATION_OPEN_PAREN) p--;
        else if (t == lexer::TokenType::PUNCTUATION_CLOSE_BRACE) b++;
        else if (t == lexer::TokenType::PUNCTUATION_OPEN_BRACE) b--;
        else if (t == lexer::TokenType::PUNCTUATION_CLOSE_BRACKET) br++;
        else if (t == lexer::TokenType::PUNCTUATION_OPEN_BRACKET) br--;
        
        if (p == 0 && b == 0 && br == 0) {
            int prec = getPrecedence(t);
            if (prec > 0 && prec < min_prec) {
                min_prec = prec;
                min_idx = i;
            }
        }
    }
    if (min_idx == -1) throw_parse_error(tokens, "Invalid binary expression");
    op = tokens[min_idx].type;
    std::vector<lexer::Token> left_tokens(tokens.begin(), tokens.begin() + min_idx);
    std::vector<lexer::Token> right_tokens(tokens.begin() + min_idx + 1, tokens.end());
    if (left_tokens.empty()) throw_parse_error(tokens, "Missing left operand");
    if (right_tokens.empty()) throw_parse_error(tokens, "Missing right operand");
    left = parseTokensToNode(left_tokens, this);
    right = parseTokensToNode(right_tokens, this);
}
UnaryExpression::UnaryExpression(const std::vector<lexer::Token>& tokens, Node* parent) : Node(tokens, NodeType::UNARY_EXPRESSION, parent) {
    auto t0 = tokens.front().type;
    auto tb = tokens.back().type;
    if (t0 == lexer::TokenType::OPERATOR_LOGICAL_NOT || t0 == lexer::TokenType::OPERATOR_MINUS ||
        t0 == lexer::TokenType::OPERATOR_INCREMENT || t0 == lexer::TokenType::OPERATOR_DECREMENT) {
        is_postfix = false;
        op = t0;
        std::vector<lexer::Token> operand_tokens(tokens.begin() + 1, tokens.end());
        if (operand_tokens.empty()) throw_parse_error(tokens, "Missing operand for unary operator");
        operand = parseTokensToNode(operand_tokens, this);
    } else if (tb == lexer::TokenType::OPERATOR_INCREMENT || tb == lexer::TokenType::OPERATOR_DECREMENT) {
        is_postfix = true;
        op = tb;
        std::vector<lexer::Token> operand_tokens(tokens.begin(), tokens.end() - 1);
        if (operand_tokens.empty()) throw_parse_error(tokens, "Missing operand for unary operator");
        operand = parseTokensToNode(operand_tokens, this);
    } else {
        throw_parse_error(tokens, "Invalid unary expression");
    }
}
AssignmentExpression::AssignmentExpression(const std::vector<lexer::Token>& tokens, Node* parent) : Node(tokens, NodeType::ASSIGNMENT_EXPRESSION, parent) {
    int p = 0, b = 0, br = 0;
    int assign_idx = -1;
    for (size_t i = 0; i < tokens.size(); i++) {
        auto t = tokens[i].type;
        if (t == lexer::TokenType::PUNCTUATION_OPEN_PAREN) p++;
        else if (t == lexer::TokenType::PUNCTUATION_CLOSE_PAREN) p--;
        else if (t == lexer::TokenType::PUNCTUATION_OPEN_BRACE) b++;
        else if (t == lexer::TokenType::PUNCTUATION_CLOSE_BRACE) b--;
        else if (t == lexer::TokenType::PUNCTUATION_OPEN_BRACKET) br++;
        else if (t == lexer::TokenType::PUNCTUATION_CLOSE_BRACKET) br--;
        
        if (p == 0 && b == 0 && br == 0 && t == lexer::TokenType::OPERATOR_ASSIGN) {
            assign_idx = i;
            break;
        }
    }
    if (assign_idx == -1) throw_parse_error(tokens, "Invalid assignment expression");
    std::vector<lexer::Token> target_tokens(tokens.begin(), tokens.begin() + assign_idx);
    std::vector<lexer::Token> val_tokens(tokens.begin() + assign_idx + 1, tokens.end());
    if (target_tokens.empty()) throw_parse_error(tokens, "Missing assignment target");
    if (val_tokens.empty()) throw_parse_error(tokens, "Missing assignment value");
    target = parseTokensToNode(target_tokens, this);
    value = parseTokensToNode(val_tokens, this);
}
CallExpression::CallExpression(const std::vector<lexer::Token>& tokens, Node* parent) : Node(tokens, NodeType::CALL_EXPRESSION, parent) {
    if (tokens.back().type != lexer::TokenType::PUNCTUATION_CLOSE_PAREN) throw_parse_error(tokens, "Expected ')' at end of call");
    int p = 0;
    int open_idx = -1;
    for (int i = tokens.size() - 1; i >= 0; i--) {
        if (tokens[i].type == lexer::TokenType::PUNCTUATION_CLOSE_PAREN) p++;
        else 
        if (tokens[i].type == lexer::TokenType::PUNCTUATION_OPEN_PAREN) p--;
        if (p == 0) {
            open_idx = i;
            break;
        }
    }
    if (open_idx <= 0) throw_parse_error(tokens, "Invalid call expression");
    std::vector<lexer::Token> callee_tokens(tokens.begin(), tokens.begin() + open_idx);
    callee = parseTokensToNode(callee_tokens, this);
    
    // Parse arguments
    int depth = 0;
    std::vector<lexer::Token> current_arg;
    for (size_t i = open_idx + 1; i < tokens.size() - 1; i++) {
        auto t = tokens[i].type;
        if (t == lexer::TokenType::PUNCTUATION_OPEN_PAREN || t == lexer::TokenType::PUNCTUATION_OPEN_BRACE || t == lexer::TokenType::PUNCTUATION_OPEN_BRACKET) depth++;
        else if (t == lexer::TokenType::PUNCTUATION_CLOSE_PAREN || t == lexer::TokenType::PUNCTUATION_CLOSE_BRACE || t == lexer::TokenType::PUNCTUATION_CLOSE_BRACKET) depth--;
        
        if (depth == 0 && t == lexer::TokenType::PUNCTUATION_COMMA) {
            if (current_arg.empty()) throw_parse_error(tokens[i], "Empty argument in function call");
            arguments.push_back(parseTokensToNode(current_arg, this));
            current_arg.clear();
        } else {
            current_arg.push_back(tokens[i]);
        }
    }
    if (!current_arg.empty()) {
        arguments.push_back(parseTokensToNode(current_arg, this));
    }
}
ArrayAccessExpression::ArrayAccessExpression(const std::vector<lexer::Token>& tokens, Node* parent) : Node(tokens, NodeType::ARRAY_ACCESS_EXPRESSION, parent) {
    if (tokens.back().type != lexer::TokenType::PUNCTUATION_CLOSE_BRACKET) throw_parse_error(tokens, "Expected ']' at end of array access");
    int br = 0;
    int open_idx = -1;
    for (int i = tokens.size() - 1; i >= 0; i--) {
        if (tokens[i].type == lexer::TokenType::PUNCTUATION_CLOSE_BRACKET) br++;
        else if (tokens[i].type == lexer::TokenType::PUNCTUATION_OPEN_BRACKET) br--;
        if (br == 0) {
            open_idx = i;
            break;
        }
    }
    if (open_idx <= 0) throw_parse_error(tokens, "Invalid array access");
    std::vector<lexer::Token> arr_tokens(tokens.begin(), tokens.begin() + open_idx);
    std::vector<lexer::Token> idx_tokens(tokens.begin() + open_idx + 1, tokens.end() - 1);
    if (arr_tokens.empty()) throw_parse_error(tokens, "Missing array target");
    if (idx_tokens.empty()) throw_parse_error(tokens, "Missing index expression");
    array = parseTokensToNode(arr_tokens, this);
    index = parseTokensToNode(idx_tokens, this);
}
MemberAccessExpression::MemberAccessExpression(const std::vector<lexer::Token>& tokens, Node* parent) : Node(tokens, NodeType::MEMBER_ACCESS_EXPRESSION, parent) {
    int p = 0, b = 0, br = 0;
    int dot_idx = -1;
    for (int i = tokens.size() - 1; i >= 0; i--) {
        auto t = tokens[i].type;
        if (t == lexer::TokenType::PUNCTUATION_CLOSE_PAREN) p++;
        else if (t == lexer::TokenType::PUNCTUATION_OPEN_PAREN) p--;
        else if (t == lexer::TokenType::PUNCTUATION_CLOSE_BRACE) b++;
        else if (t == lexer::TokenType::PUNCTUATION_OPEN_BRACE) b--;
        else if (t == lexer::TokenType::PUNCTUATION_CLOSE_BRACKET) br++;
        else if (t == lexer::TokenType::PUNCTUATION_OPEN_BRACKET) br--;
        
        if (p == 0 && b == 0 && br == 0 && t == lexer::TokenType::PUNCTUATION_DOT) {
            dot_idx = i;
            break;
        }
    }
    if (dot_idx == -1 || dot_idx == tokens.size() - 1) throw_parse_error(tokens, "Invalid member access");
    if (dot_idx != tokens.size() - 2 || tokens.back().type != lexer::TokenType::IDENTIFIER) throw_parse_error(tokens.back(), "Expected single identifier after '.'");
    
    std::vector<lexer::Token> obj_tokens(tokens.begin(), tokens.begin() + dot_idx);
    if (obj_tokens.empty()) throw_parse_error(tokens, "Missing object reference before '.'");
    object = parseTokensToNode(obj_tokens, this);
    member_name = std::string(tokens.back().value.value_or(""));
}
NewInstanceExpression::NewInstanceExpression(const std::vector<lexer::Token>& tokens, Node* parent) : Node(tokens, NodeType::NEW_INSTANCE_EXPRESSION, parent) {
    int p = 0;
    int open_idx = -1;
    for (int i = tokens.size() - 1; i >= 0; i--) {
        if (tokens[i].type == lexer::TokenType::PUNCTUATION_CLOSE_PAREN) p++;
        else 
        if (tokens[i].type == lexer::TokenType::PUNCTUATION_OPEN_PAREN) p--;
        if (p == 0) {
            open_idx = i;
            break;
        }
    }
    if (open_idx <= 1) throw_parse_error(tokens, "Invalid new instance syntax");
    for (size_t i = 1; i < open_idx; i++) {
        class_name += std::string(tokens[i].value.value_or("")) + (tokens[i].type == lexer::TokenType::PUNCTUATION_DOT ? "" : " ");
    }
    
    // args inside open_idx
    std::vector<lexer::Token> current_arg;
    int depth = 0;
    for (size_t i = open_idx + 1; i < tokens.size() - 1; i++) {
        auto t = tokens[i].type;
        if (t == lexer::TokenType::PUNCTUATION_OPEN_PAREN || t == lexer::TokenType::PUNCTUATION_OPEN_BRACE || t == lexer::TokenType::PUNCTUATION_OPEN_BRACKET) depth++;
        else if (t == lexer::TokenType::PUNCTUATION_CLOSE_PAREN || t == lexer::TokenType::PUNCTUATION_CLOSE_BRACE || t == lexer::TokenType::PUNCTUATION_CLOSE_BRACKET) depth--;
        
        if (depth == 0 && t == lexer::TokenType::PUNCTUATION_COMMA) {
            if (current_arg.empty()) throw_parse_error(tokens[i], "Empty argument");
            arguments.push_back(parseTokensToNode(current_arg, this));
            current_arg.clear();
        } else {
            current_arg.push_back(tokens[i]);
        }
    }
    if (!current_arg.empty()) arguments.push_back(parseTokensToNode(current_arg, this));
}
ArrayCreationExpression::ArrayCreationExpression(const std::vector<lexer::Token>& tokens, Node* parent) : Node(tokens, NodeType::ARRAY_CREATION_EXPRESSION, parent) {
    if (tokens[0].type != lexer::TokenType::KEYWORD_NEW) throw_parse_error(tokens, "Expected 'new' for array creation");
    int br = 0;
    int open_idx = -1;
    for (int i = tokens.size() - 1; i >= 0; i--) {
        if (tokens[i].type == lexer::TokenType::PUNCTUATION_CLOSE_BRACKET) br++;
        else if (tokens[i].type == lexer::TokenType::PUNCTUATION_OPEN_BRACKET) br--;
        if (br == 0) {
            open_idx = i;
            break;
        }
    }
    if (open_idx <= 1) throw_parse_error(tokens, "Invalid array creation syntax");
    for (size_t i = 1; i < open_idx; i++) {
        type_name += std::string(tokens[i].value.value_or("")) + (tokens[i].type == lexer::TokenType::PUNCTUATION_DOT ? "" : " ");
    }
    std::vector<lexer::Token> size_tokens(tokens.begin() + open_idx + 1, tokens.end() - 1);
    if (size_tokens.empty()) throw_parse_error(tokens, "Array size expression cannot be empty");
    size = parseTokensToNode(size_tokens, this);
}
ArrayLiteralExpression::ArrayLiteralExpression(const std::vector<lexer::Token>& tokens, Node* parent) : Node(tokens, NodeType::ARRAY_LITERAL_EXPRESSION, parent) {
    if (tokens.front().type != lexer::TokenType::PUNCTUATION_OPEN_BRACE || tokens.back().type != lexer::TokenType::PUNCTUATION_CLOSE_BRACE) throw_parse_error(tokens, "Array literal must be enclosed in braces");
    int depth = 0;
    std::vector<lexer::Token> current_elem;
    for (size_t i = 1; i < tokens.size() - 1; i++) {
        auto t = tokens[i].type;
        if (t == lexer::TokenType::PUNCTUATION_OPEN_PAREN || t == lexer::TokenType::PUNCTUATION_OPEN_BRACE || t == lexer::TokenType::PUNCTUATION_OPEN_BRACKET) depth++;
        else if (t == lexer::TokenType::PUNCTUATION_CLOSE_PAREN || t == lexer::TokenType::PUNCTUATION_CLOSE_BRACE || t == lexer::TokenType::PUNCTUATION_CLOSE_BRACKET) depth--;
        
        if (depth == 0 && t == lexer::TokenType::PUNCTUATION_COMMA) {
            if (current_elem.empty()) throw_parse_error(tokens[i], "Empty element in array literal");
            elements.push_back(parseTokensToNode(current_elem, this));
            current_elem.clear();
        } else {
            current_elem.push_back(tokens[i]);
        }
    }
    if (!current_elem.empty()) elements.push_back(parseTokensToNode(current_elem, this));
}
CastExpression::CastExpression(const std::vector<lexer::Token>& tokens, Node* parent) : Node(tokens, NodeType::CAST_EXPRESSION, parent) {
    if (tokens[0].type != lexer::TokenType::PUNCTUATION_OPEN_PAREN) throw_parse_error(tokens, "Invalid cast syntax");
    int p = 0;
    size_t close_idx = 0;
    for (size_t i = 0; i < tokens.size(); i++) {
        
        if (tokens[i].type == lexer::TokenType::PUNCTUATION_OPEN_PAREN) p++;
        else if (tokens[i].type == lexer::TokenType::PUNCTUATION_CLOSE_PAREN) p--;
        if (p == 0) {
            close_idx = i;
            break;
        }
    }
    for (size_t i = 1; i < close_idx; i++) {
        target_type += std::string(tokens[i].value.value_or(""));
    }
    std::vector<lexer::Token> expr_tokens(tokens.begin() + close_idx + 1, tokens.end());
    expression = parseTokensToNode(expr_tokens, this);
}
TernaryExpression::TernaryExpression(const std::vector<lexer::Token>& tokens, Node* parent) : Node(tokens, NodeType::TERNARY_EXPRESSION, parent) {}

// ==========================================
// Top-Level Statements
// ==========================================

PackageStatement::PackageStatement(const std::vector<lexer::Token>& tokens, Node* parent) : Node(tokens, NodeType::PACKAGE_STATEMENT, parent) {
    if (tokens.size() < 3) throw_parse_error(tokens, "Expected package name");
    if (tokens.back().type != lexer::TokenType::PUNCTUATION_SEMICOLON) throw_parse_error(tokens, "Expected ';' after package name");
    for (size_t i = 1; i < tokens.size(); i++) {
        if (tokens[i].type == lexer::TokenType::PUNCTUATION_SEMICOLON) break;
        package_name += std::string(tokens[i].value.value_or(""));
    }
}
AliasStatement::AliasStatement(const std::vector<lexer::Token>& tokens, Node* parent) : Node(tokens, NodeType::ALIAS_STATEMENT, parent) {
    if (tokens.size() < 4) throw_parse_error(tokens, "Invalid alias declaration syntax");
    alias_name = std::string(tokens[1].value.value_or(""));
    if (tokens[2].type != lexer::TokenType::OPERATOR_ASSIGN) throw_parse_error(tokens[2], "Expected '=' in alias declaration");
    for (size_t i = 3; i < tokens.size(); i++) {
        if (tokens[i].type == lexer::TokenType::PUNCTUATION_SEMICOLON) break;
        target_type += std::string(tokens[i].value.value_or(""));
    }
}
EnumDeclaration::EnumDeclaration(const std::vector<lexer::Token>& tokens, Node* parent) : Node(tokens, NodeType::ENUM_DECLARATION, parent) {
    size_t i = 0;
    while (i < tokens.size() && tokens[i].type != lexer::TokenType::KEYWORD_ENUM) {
        access_modifier = tokens[i].type;
        i++;
    }
    i++;
    if (i >= tokens.size() || tokens[i].type != lexer::TokenType::IDENTIFIER) throw_parse_error(tokens, "Expected identifier for enum name");
    enum_name = std::string(tokens[i].value.value_or(""));
    i++;
    if (i >= tokens.size() || tokens[i].type != lexer::TokenType::PUNCTUATION_OPEN_BRACE) throw_parse_error(tokens, "Expected '{' for enum body");
    
    for (size_t j = i + 1; j < tokens.size(); j++) {
        if (tokens[j].type == lexer::TokenType::PUNCTUATION_CLOSE_BRACE) break;
        if (tokens[j].type == lexer::TokenType::IDENTIFIER) {
            members.push_back(std::string(tokens[j].value.value_or("")));
        } else if (tokens[j].type != lexer::TokenType::PUNCTUATION_COMMA) {
            throw_parse_error(tokens[j], "Enum members must be valid identifiers separated by commas");
        }
    }
}
ClassDeclaration::ClassDeclaration(const std::vector<lexer::Token>& tokens, Node* parent) : Node(tokens, NodeType::CLASS_DECLARATION, parent) {
    size_t i = 0;
    while (i < tokens.size() && tokens[i].type != lexer::TokenType::KEYWORD_CLASS) {
        access_modifier = tokens[i].type;
        i++;
    }
    i++;
    if (i >= tokens.size() || tokens[i].type != lexer::TokenType::IDENTIFIER) throw_parse_error(tokens, "Expected identifier for class name");
    class_name = std::string(tokens[i].value.value_or(""));
    i++;
    if (i >= tokens.size() || tokens[i].type != lexer::TokenType::PUNCTUATION_OPEN_BRACE) throw_parse_error(tokens, "Expected '{' for class body");
    
    std::vector<lexer::Token> body_tokens(tokens.begin() + i + 1, tokens.end() - 1);
    auto statements = divideTokensIntoStatements(body_tokens);
    for (const auto& stmt : statements) {
        if (!stmt.empty()) children.push_back(parseTokensToNode(stmt, this));
    }
}

// ==========================================
// Class-Level Declarations
// ==========================================

FieldDeclaration::FieldDeclaration(const std::vector<lexer::Token>& tokens, Node* parent) : Node(tokens, NodeType::FIELD_DECLARATION, parent) {
    size_t i = 0;
    while (i < tokens.size() && (
        tokens[i].type == lexer::TokenType::KEYWORD_PUBLIC ||
        tokens[i].type == lexer::TokenType::KEYWORD_PRIVATE ||
        tokens[i].type == lexer::TokenType::KEYWORD_PROTECTED ||
        tokens[i].type == lexer::TokenType::KEYWORD_INTERNAL ||
        tokens[i].type == lexer::TokenType::KEYWORD_STATIC ||
        tokens[i].type == lexer::TokenType::KEYWORD_CONST)) {
        if (tokens[i].type == lexer::TokenType::KEYWORD_STATIC) is_static = true;
        else if (tokens[i].type == lexer::TokenType::KEYWORD_CONST) is_const = true;
        else access_modifier = tokens[i].type;
        i++;
    }
    
    size_t type_start = i;
    i = consumeType(tokens, i);
    if (i == type_start || i >= tokens.size() || tokens[i].type != lexer::TokenType::IDENTIFIER) throw_parse_error(tokens, "Invalid field declaration syntax");
    
    for (size_t j = type_start; j < i; j++) type_name += std::string(tokens[j].value.value_or("")) + (tokens[j].type == lexer::TokenType::PUNCTUATION_DOT ? "" : " ");
    field_name = std::string(tokens[i].value.value_or(""));
    i++;
    
    if (i < tokens.size() && tokens[i].type == lexer::TokenType::OPERATOR_ASSIGN) {
        if (i + 1 >= tokens.size() || tokens[i+1].type == lexer::TokenType::PUNCTUATION_SEMICOLON) throw_parse_error(tokens[i], "Expected expression after '='");
        std::vector<lexer::Token> init_tokens(tokens.begin() + i + 1, tokens.end());
        if (!init_tokens.empty() && init_tokens.back().type == lexer::TokenType::PUNCTUATION_SEMICOLON) init_tokens.pop_back();
        initializer = parseTokensToNode(init_tokens, this);
    }
}
ConstructorDeclaration::ConstructorDeclaration(const std::vector<lexer::Token>& tokens, Node* parent) : Node(tokens, NodeType::CONSTRUCTOR_DECLARATION, parent) {
    size_t i = 0;
    while (i < tokens.size() && (
        tokens[i].type == lexer::TokenType::KEYWORD_PUBLIC ||
        tokens[i].type == lexer::TokenType::KEYWORD_PRIVATE ||
        tokens[i].type == lexer::TokenType::KEYWORD_PROTECTED ||
        tokens[i].type == lexer::TokenType::KEYWORD_INTERNAL)) {
        access_modifier = tokens[i].type;
        i++;
    }
    
    i++; // skip name
    if (i >= tokens.size() || tokens[i].type != lexer::TokenType::PUNCTUATION_OPEN_PAREN) throw_parse_error(tokens, "Expected '(' after constructor name");
    
    int p_depth = 0;
    size_t param_end = i;
    for (size_t j = i; j < tokens.size(); j++) {
        if (tokens[j].type == lexer::TokenType::PUNCTUATION_OPEN_PAREN) p_depth++;
        else if (tokens[j].type == lexer::TokenType::PUNCTUATION_CLOSE_PAREN) p_depth--;
        if (p_depth == 0) {
            param_end = j;
            break;
        }
    }
    
    if (param_end < tokens.size() - 1 && tokens[param_end + 1].type == lexer::TokenType::PUNCTUATION_OPEN_BRACE) {
        std::vector<lexer::Token> body_tokens(tokens.begin() + param_end + 1, tokens.end());
        children.push_back(parseTokensToNode(body_tokens, this));
    }
}
MethodDeclaration::MethodDeclaration(const std::vector<lexer::Token>& tokens, Node* parent) : Node(tokens, NodeType::METHOD_DECLARATION, parent) {
    size_t i = 0;
    while (i < tokens.size() && (
        tokens[i].type == lexer::TokenType::KEYWORD_PUBLIC ||
        tokens[i].type == lexer::TokenType::KEYWORD_PRIVATE ||
        tokens[i].type == lexer::TokenType::KEYWORD_PROTECTED ||
        tokens[i].type == lexer::TokenType::KEYWORD_INTERNAL ||
        tokens[i].type == lexer::TokenType::KEYWORD_STATIC ||
        tokens[i].type == lexer::TokenType::KEYWORD_CONST ||
        tokens[i].type == lexer::TokenType::KEYWORD_INLINE)) {
        if (tokens[i].type == lexer::TokenType::KEYWORD_STATIC) is_static = true;
        else if (tokens[i].type == lexer::TokenType::KEYWORD_INLINE) is_inline = true;
        else access_modifier = tokens[i].type;
        i++;
    }
    
    size_t type_start = i;
    i = consumeType(tokens, i);
    if (i == type_start || i >= tokens.size() || tokens[i].type != lexer::TokenType::IDENTIFIER) throw_parse_error(tokens, "Invalid method declaration syntax");
    
    for (size_t j = type_start; j < i; j++) return_type += std::string(tokens[j].value.value_or("")) + " ";
    method_name = std::string(tokens[i].value.value_or(""));
    i++;
    
    if (i >= tokens.size() || tokens[i].type != lexer::TokenType::PUNCTUATION_OPEN_PAREN) throw_parse_error(tokens, "Expected '(' after method name");
    
    int p_depth = 0;
    size_t param_start = i + 1;
    size_t param_end = i;
    for (size_t j = i; j < tokens.size(); j++) {
        if (tokens[j].type == lexer::TokenType::PUNCTUATION_OPEN_PAREN) p_depth++;
        else if (tokens[j].type == lexer::TokenType::PUNCTUATION_CLOSE_PAREN) p_depth--;
        if (p_depth == 0) {
            param_end = j;
            break;
        }
    }
    
    if (param_end < tokens.size() - 1 && tokens[param_end + 1].type == lexer::TokenType::PUNCTUATION_OPEN_BRACE) {
        std::vector<lexer::Token> body_tokens(tokens.begin() + param_end + 1, tokens.end());
        children.push_back(parseTokensToNode(body_tokens, this));
    }
}

// ==========================================
// Control Flow & Statements
// ==========================================

BlockStatement::BlockStatement(const std::vector<lexer::Token>& tokens, Node* parent) : Node(tokens, NodeType::BLOCK_STATEMENT, parent) {
    if (tokens.front().type != lexer::TokenType::PUNCTUATION_OPEN_BRACE || tokens.back().type != lexer::TokenType::PUNCTUATION_CLOSE_BRACE) throw_parse_error(tokens, "Block must be enclosed in braces");
    std::vector<lexer::Token> inner(tokens.begin() + 1, tokens.end() - 1);
    auto statements = divideTokensIntoStatements(inner);
    for (const auto& stmt : statements) {
        if (!stmt.empty()) children.push_back(parseTokensToNode(stmt, this));
    }
}
IfStatement::IfStatement(const std::vector<lexer::Token>& tokens, Node* parent) : Node(tokens, NodeType::IF_STATEMENT, parent) {
    if (tokens.size() < 4) throw_parse_error(tokens, "Expected condition in if statement");
    if (tokens[1].type != lexer::TokenType::PUNCTUATION_OPEN_PAREN) throw_parse_error(tokens[1], "Expected '(' after if");
    
    int p = 0;
    size_t cond_end = 0;
    for (size_t i = 1; i < tokens.size(); i++) {
        
        if (tokens[i].type == lexer::TokenType::PUNCTUATION_OPEN_PAREN) p++;
        else if (tokens[i].type == lexer::TokenType::PUNCTUATION_CLOSE_PAREN) p--;
        if (p == 0) {
            cond_end = i;
            break;
        }
    }
    std::vector<lexer::Token> cond_tokens(tokens.begin() + 2, tokens.begin() + cond_end);
    if (cond_tokens.empty()) throw_parse_error(tokens, "Expected condition in if statement");
    condition = parseTokensToNode(cond_tokens, this);
    
    size_t then_start = cond_end + 1;
    if (then_start >= tokens.size()) throw_parse_error(tokens, "Expected body for if statement");
    
    size_t else_idx = tokens.size();
    if (tokens[then_start].type == lexer::TokenType::PUNCTUATION_OPEN_BRACE) {
        int b = 0;
        for (size_t i = then_start; i < tokens.size(); i++) {
            if (tokens[i].type == lexer::TokenType::PUNCTUATION_OPEN_BRACE) b++;
            else if (tokens[i].type == lexer::TokenType::PUNCTUATION_CLOSE_BRACE) b--;
            if (b == 0) {
                else_idx = i + 1;
                break;
            }
        }
    } else {
        for (size_t i = then_start; i < tokens.size(); i++) {
            if (tokens[i].type == lexer::TokenType::KEYWORD_ELSE) {
                else_idx = i;
                break;
            }
        }
    }
    
    std::vector<lexer::Token> then_tokens(tokens.begin() + then_start, tokens.begin() + else_idx);
    if (then_tokens.empty()) throw_parse_error(tokens, "Expected body for if statement");
    then_branch = parseTokensToNode(then_tokens, this);
    
    if (else_idx < tokens.size() && tokens[else_idx].type == lexer::TokenType::KEYWORD_ELSE) {
        if (else_idx + 1 >= tokens.size()) throw_parse_error(tokens, "Expected body after else");
        std::vector<lexer::Token> else_tokens(tokens.begin() + else_idx + 1, tokens.end());
        else_branch = parseTokensToNode(else_tokens, this);
    }
}
ForStatement::ForStatement(const std::vector<lexer::Token>& tokens, Node* parent) : Node(tokens, NodeType::FOR_STATEMENT, parent) {
    if (tokens.size() < 4 || tokens[1].type != lexer::TokenType::PUNCTUATION_OPEN_PAREN) throw_parse_error(tokens, "Invalid for-loop syntax");
    int p = 0;
    size_t cond_end = 0;
    for (size_t i = 1; i < tokens.size(); i++) {
        
        if (tokens[i].type == lexer::TokenType::PUNCTUATION_OPEN_PAREN) p++;
        else if (tokens[i].type == lexer::TokenType::PUNCTUATION_CLOSE_PAREN) p--;
        if (p == 0) {
            cond_end = i;
            break;
        }
    }
    
    std::vector<lexer::Token> cond_tokens(tokens.begin() + 2, tokens.begin() + cond_end);
    std::vector<std::vector<lexer::Token>> parts;
    std::vector<lexer::Token> curr;
    int inner_p = 0;
    for (const auto& t : cond_tokens) {
        if (t.type == lexer::TokenType::PUNCTUATION_OPEN_PAREN) inner_p++;
        else if (t.type == lexer::TokenType::PUNCTUATION_CLOSE_PAREN) inner_p--;
        if (inner_p == 0 && t.type == lexer::TokenType::PUNCTUATION_SEMICOLON) {
            parts.push_back(curr);
            curr.clear();
        } else {
            curr.push_back(t);
        }
    }
    parts.push_back(curr);
    
    if (parts.size() != 3) throw_parse_error(tokens, "Invalid for-loop syntax: expected two semicolons");
    if (!parts[0].empty()) {
        parts[0].push_back(lexer::Token{lexer::TokenType::PUNCTUATION_SEMICOLON, std::nullopt, std::nullopt, 0, 0});
        initialization = parseTokensToNode(parts[0], this);
    }
    if (!parts[1].empty()) condition = parseTokensToNode(parts[1], this);
    if (!parts[2].empty()) {
        parts[2].push_back(lexer::Token{lexer::TokenType::PUNCTUATION_SEMICOLON, std::nullopt, std::nullopt, 0, 0});
        iteration = parseTokensToNode(parts[2], this);
    }
    
    if (cond_end + 1 >= tokens.size()) throw_parse_error(tokens, "Expected body for for-loop");
    std::vector<lexer::Token> body_tokens(tokens.begin() + cond_end + 1, tokens.end());
    body = parseTokensToNode(body_tokens, this);
}
WhileStatement::WhileStatement(const std::vector<lexer::Token>& tokens, Node* parent) : Node(tokens, NodeType::WHILE_STATEMENT, parent) {
    if (tokens.size() < 4 || tokens[1].type != lexer::TokenType::PUNCTUATION_OPEN_PAREN) throw_parse_error(tokens, "Invalid while-loop syntax");
    int p = 0;
    size_t cond_end = 0;
    for (size_t i = 1; i < tokens.size(); i++) {
        
        if (tokens[i].type == lexer::TokenType::PUNCTUATION_OPEN_PAREN) p++;
        else if (tokens[i].type == lexer::TokenType::PUNCTUATION_CLOSE_PAREN) p--;
        if (p == 0) {
            cond_end = i;
            break;
        }
    }
    std::vector<lexer::Token> cond_tokens(tokens.begin() + 2, tokens.begin() + cond_end);
    if (cond_tokens.empty()) throw_parse_error(tokens, "Expected condition in while statement");
    condition = parseTokensToNode(cond_tokens, this);
    
    if (cond_end + 1 >= tokens.size()) throw_parse_error(tokens, "Expected body for while statement");
    std::vector<lexer::Token> body_tokens(tokens.begin() + cond_end + 1, tokens.end());
    body = parseTokensToNode(body_tokens, this);
}
DoWhileStatement::DoWhileStatement(const std::vector<lexer::Token>& tokens, Node* parent) : Node(tokens, NodeType::DO_WHILE_STATEMENT, parent) {
    if (tokens.size() < 5) throw_parse_error(tokens, "Invalid do-while syntax");
    size_t while_idx = 0;
    for (size_t i = tokens.size() - 1; i >= 0; i--) {
        if (tokens[i].type == lexer::TokenType::KEYWORD_WHILE) {
            while_idx = i;
            break;
        }
    }
    if (while_idx == 0 || while_idx >= tokens.size() - 1) throw_parse_error(tokens, "Expected 'while' after do block");
    
    std::vector<lexer::Token> body_tokens(tokens.begin() + 1, tokens.begin() + while_idx);
    body = parseTokensToNode(body_tokens, this);
    
    std::vector<lexer::Token> cond_tokens(tokens.begin() + while_idx + 1, tokens.end());
    if (cond_tokens.back().type == lexer::TokenType::PUNCTUATION_SEMICOLON) cond_tokens.pop_back();
    condition = parseTokensToNode(cond_tokens, this);
}
SwitchStatement::SwitchStatement(const std::vector<lexer::Token>& tokens, Node* parent) : Node(tokens, NodeType::SWITCH_STATEMENT, parent) {
    if (tokens.size() < 4 || tokens[1].type != lexer::TokenType::PUNCTUATION_OPEN_PAREN) throw_parse_error(tokens, "Invalid switch syntax");
    int p = 0;
    size_t cond_end = 0;
    for (size_t i = 1; i < tokens.size(); i++) {
        
        if (tokens[i].type == lexer::TokenType::PUNCTUATION_OPEN_PAREN) p++;
        else if (tokens[i].type == lexer::TokenType::PUNCTUATION_CLOSE_PAREN) p--;
        if (p == 0) {
            cond_end = i;
            break;
        }
    }
    std::vector<lexer::Token> cond_tokens(tokens.begin() + 2, tokens.begin() + cond_end);
    if (cond_tokens.empty()) throw_parse_error(tokens, "Expected condition in switch statement");
    condition = parseTokensToNode(cond_tokens, this);
    
    if (cond_end + 1 >= tokens.size() || tokens[cond_end+1].type != lexer::TokenType::PUNCTUATION_OPEN_BRACE) throw_parse_error(tokens, "Expected block for switch statement");
    
    std::vector<lexer::Token> block_tokens(tokens.begin() + cond_end + 2, tokens.end() - 1);
    std::vector<std::vector<lexer::Token>> case_stmts;
    std::vector<lexer::Token> curr_case;
    int b = 0;
    for (const auto& t : block_tokens) {
        if (t.type == lexer::TokenType::PUNCTUATION_OPEN_BRACE) b++;
        else if (t.type == lexer::TokenType::PUNCTUATION_CLOSE_BRACE) b--;
        if (b == 0 && (t.type == lexer::TokenType::KEYWORD_CASE || t.type == lexer::TokenType::KEYWORD_DEFAULT)) {
            if (!curr_case.empty()) case_stmts.push_back(curr_case);
            curr_case.clear();
        }
        curr_case.push_back(t);
    }
    if (!curr_case.empty()) case_stmts.push_back(curr_case);
    
    for (const auto& c : case_stmts) {
        children.push_back(parseTokensToNode(c, this));
    }
}
CaseStatement::CaseStatement(const std::vector<lexer::Token>& tokens, Node* parent) : Node(tokens, NodeType::CASE_STATEMENT, parent) {
    if (tokens.front().type == lexer::TokenType::KEYWORD_DEFAULT) is_default = true;
    size_t colon_idx = 0;
    for (size_t i = 1; i < tokens.size(); i++) {
        if (tokens[i].type == lexer::TokenType::PUNCTUATION_COLON) {
            colon_idx = i;
            break;
        }
    }
    if (colon_idx == 0) throw_parse_error(tokens, "Expected ':' after case or default");
    
    if (!is_default) {
        std::vector<lexer::Token> val_tokens(tokens.begin() + 1, tokens.begin() + colon_idx);
        if (val_tokens.empty()) throw_parse_error(tokens, "Expected value for case");
        case_value = parseTokensToNode(val_tokens, this);
    }
    
    if (colon_idx + 1 < tokens.size()) {
        std::vector<lexer::Token> body_tokens(tokens.begin() + colon_idx + 1, tokens.end());
        auto stmts = divideTokensIntoStatements(body_tokens);
        for (const auto& stmt : stmts) {
            if (!stmt.empty()) children.push_back(parseTokensToNode(stmt, this));
        }
    }
}
VariableDeclaration::VariableDeclaration(const std::vector<lexer::Token>& tokens, Node* parent) : Node(tokens, NodeType::VARIABLE_DECLARATION, parent) {
    size_t start = 0;
    while (start < tokens.size() && tokens[start].type == lexer::TokenType::KEYWORD_CONST) {
        is_const = true;
        start++;
    }
    size_t i = consumeType(tokens, start);
    if (i == 0 || i >= tokens.size() || tokens[i].type != lexer::TokenType::IDENTIFIER) throw_parse_error(tokens, "Invalid variable declaration syntax");
    for (size_t j = start; j < i; j++) type_name += std::string(tokens[j].value.value_or("")) + (tokens[j].type == lexer::TokenType::PUNCTUATION_DOT ? "" : " ");
    var_name = std::string(tokens[i].value.value_or(""));
    i++;
    if (i < tokens.size() && tokens[i].type == lexer::TokenType::OPERATOR_ASSIGN) {
        if (i + 1 >= tokens.size() || tokens[i+1].type == lexer::TokenType::PUNCTUATION_SEMICOLON) throw_parse_error(tokens[i], "Expected expression after '='");
        std::vector<lexer::Token> init_tokens(tokens.begin() + i + 1, tokens.end());
        if (!init_tokens.empty() && init_tokens.back().type == lexer::TokenType::PUNCTUATION_SEMICOLON) init_tokens.pop_back();
        initializer = parseTokensToNode(init_tokens, this);
    }
}
ExpressionStatement::ExpressionStatement(const std::vector<lexer::Token>& tokens, Node* parent) : Node(tokens, NodeType::EXPRESSION_STATEMENT, parent) {
    if (tokens.empty() || tokens.back().type != lexer::TokenType::PUNCTUATION_SEMICOLON) throw_parse_error(tokens, "Expected ';' at end of expression statement");
    std::vector<lexer::Token> expr_tokens(tokens.begin(), tokens.end() - 1);
    if (expr_tokens.empty()) throw_parse_error(tokens, "Empty expression statement");
    expression = parseTokensToNode(expr_tokens, this);
}
ReturnStatement::ReturnStatement(const std::vector<lexer::Token>& tokens, Node* parent) : Node(tokens, NodeType::RETURN_STATEMENT, parent) {
    if (tokens.empty() || tokens.back().type != lexer::TokenType::PUNCTUATION_SEMICOLON) throw_parse_error(tokens, "Expected ';' at end of return statement");
    if (tokens.size() > 2) {
        std::vector<lexer::Token> val_tokens(tokens.begin() + 1, tokens.end() - 1);
        value = parseTokensToNode(val_tokens, this);
    }
}
BreakStatement::BreakStatement(const std::vector<lexer::Token>& tokens, Node* parent) : Node(tokens, NodeType::BREAK_STATEMENT, parent) {
    if (tokens.back().type != lexer::TokenType::PUNCTUATION_SEMICOLON) throw_parse_error(tokens, "Expected ';' at end of statement");
}
ContinueStatement::ContinueStatement(const std::vector<lexer::Token>& tokens, Node* parent) : Node(tokens, NodeType::CONTINUE_STATEMENT, parent) {
    if (tokens.back().type != lexer::TokenType::PUNCTUATION_SEMICOLON) throw_parse_error(tokens, "Expected ';' at end of statement");
}

// ==========================================
// AstTree & Helper Implementation
// ==========================================
AstTree::AstTree() {}

const Node* AstTree::resolveDeclaration(const Node* current_scope, const std::string& name) const {
    const Node* prev = nullptr;
    const Node* curr = current_scope;
    while (curr != nullptr) {
        if (curr->node_type == NodeType::BLOCK_STATEMENT) {
            auto block = static_cast<const BlockStatement*>(curr);
            for (const auto& child : block->children) {
                if (child.get() == prev) break; // Don't look ahead
                if (child->node_type == NodeType::VARIABLE_DECLARATION) {
                    auto var_decl = static_cast<const VariableDeclaration*>(child.get());
                    if (var_decl->var_name == name) return var_decl;
                }
            }
        } else if (curr->node_type == NodeType::CLASS_DECLARATION) {
            auto class_decl = static_cast<const ClassDeclaration*>(curr);
            for (const auto& child : class_decl->children) {
                if (child->node_type == NodeType::FIELD_DECLARATION) {
                    auto field_decl = static_cast<const FieldDeclaration*>(child.get());
                    if (field_decl->field_name == name) return field_decl;
                } else if (child->node_type == NodeType::METHOD_DECLARATION) {
                    auto method_decl = static_cast<const MethodDeclaration*>(child.get());
                    if (method_decl->method_name == name) return method_decl;
                }
            }
        } else if (curr->node_type == NodeType::METHOD_DECLARATION) {
            auto method_decl = static_cast<const MethodDeclaration*>(curr);
            for (const auto& param : method_decl->parameters) {
                if (param.name == name) return method_decl; // Ideally return a Parameter node, but we return the method for now
            }
        } else if (curr->node_type == NodeType::CONSTRUCTOR_DECLARATION) {
            auto ctor_decl = static_cast<const ConstructorDeclaration*>(curr);
            for (const auto& param : ctor_decl->parameters) {
                if (param.name == name) return ctor_decl;
            }
        }
        
        prev = curr;
        curr = curr->parent_node;
    }
    
    auto it = symbols.find(name);
    if (it != symbols.end()) return it->second;
    
    return nullptr;
}


static void populateSymbols(AstTree* tree, Node* root, std::string prefix) {
    if (!root) return;
    std::string my_prefix = prefix;
    
    if (root->node_type == NodeType::PACKAGE_STATEMENT) {
        my_prefix = static_cast<PackageStatement*>(root)->package_name + ".";
    } else if (root->node_type == NodeType::CLASS_DECLARATION) {
        auto cls = static_cast<ClassDeclaration*>(root);
        std::string full_name = prefix + cls->class_name;
        if (tree->symbols.count(full_name)) throw std::runtime_error("Duplicate global symbol: " + full_name);
        tree->symbols[full_name] = cls;
        my_prefix = full_name + ".";
    } else if (root->node_type == NodeType::ENUM_DECLARATION) {
        auto enm = static_cast<EnumDeclaration*>(root);
        std::string full_name = prefix + enm->enum_name;
        if (tree->symbols.count(full_name)) throw std::runtime_error("Duplicate global symbol: " + full_name);
        tree->symbols[full_name] = enm;
    } else if (root->node_type == NodeType::ALIAS_STATEMENT) {
        auto alias = static_cast<AliasStatement*>(root);
        std::string full_name = prefix + alias->alias_name;
        if (tree->symbols.count(full_name)) throw std::runtime_error("Duplicate global symbol: " + full_name);
        tree->symbols[full_name] = alias;
    }
    
    for (const auto& child : root->children) {
        populateSymbols(tree, child.get(), my_prefix);
    }
}

void AstTree::include(std::filesystem::path file_path) {
    auto tokens = lexer::tokenize_file(file_path);
    auto stmts = divideTokensIntoStatements(tokens);
    
    std::string pkg_prefix = "";
    for (const auto& stmt : stmts) {
        if (!stmt.empty()) {
            auto node = parseTokensToNode(stmt);
            populateSymbols(this, node.get(), pkg_prefix);
            if (node && node->node_type == NodeType::PACKAGE_STATEMENT) {
                pkg_prefix = static_cast<PackageStatement*>(node.get())->package_name + ".";
            }
            if (node) nodes.push_back(std::move(node));
        }
    }
}

void AstTree::include(std::string_view source_code, std::optional<std::filesystem::path> file_path) {
    auto tokens = lexer::tokenize(source_code);
    auto stmts = divideTokensIntoStatements(tokens);
    
    std::string pkg_prefix = "";
    for (const auto& stmt : stmts) {
        if (!stmt.empty()) {
            auto node = parseTokensToNode(stmt);
            populateSymbols(this, node.get(), pkg_prefix);
            if (node && node->node_type == NodeType::PACKAGE_STATEMENT) {
                pkg_prefix = static_cast<PackageStatement*>(node.get())->package_name + ".";
            }
            if (node) nodes.push_back(std::move(node));
        }
    }
}

} // namespace solix::parser
