#include "solix/processes/parser.hpp"
#include "solix/compilation.hpp"
#include "solix/statements.hpp"
#include <stdexcept>

namespace solix {

class ParseError : public std::runtime_error {
public:
    ParseError(const std::string& msg) : std::runtime_error(msg) {}
};

class ParserState {
    std::vector<const Token*> tokens;
    size_t current = 0;
    Parser* process;
    const Source* current_source;

public:
    ParserState(const TokenList& tkns, Parser* proc, const Source* src) 
        
: process(proc), current_source(src) {
        for (const auto& t : tkns) {
            if (t.type != TokenType::COMMENT_SINGLE_LINE && t.type != TokenType::COMMENT_MULTI_LINE) {
                tokens.push_back(&t);
            }
        }
    }


    const Token& peek() const { return *tokens[current]; }
    const Token& previous() const { return *tokens[current - 1]; }
    bool is_at_end() const { return peek().type == TokenType::EOF_TOKEN; }

    const Token& advance() {
        if (!is_at_end()) current++;
        return previous();
    }

    bool check(TokenType type) const {
        if (is_at_end()) return false;
        return peek().type == type;
    }

    bool match(TokenType type) {
        if (check(type)) {
            advance();
            return true;
        }
        return false;
    }

    bool match(std::initializer_list<TokenType> types) {
        for (TokenType type : types) {
            if (check(type)) {
                advance();
                return true;
            }
        }
        return false;
    }

    const Token& consume(TokenType type, const std::string& message) {
        if (check(type)) return advance();
        throw ParseError(message + " at line " + std::to_string(peek().line) + " col " + std::to_string(peek().column));
    }
    
    // Expression parsing
    std::unique_ptr<Node> parse_expression();
    std::unique_ptr<Node> parse_assignment();
    std::unique_ptr<Node> parse_ternary();
    std::unique_ptr<Node> parse_logical_or();
    std::unique_ptr<Node> parse_logical_and();
    std::unique_ptr<Node> parse_equality();
    std::unique_ptr<Node> parse_comparison();
    std::unique_ptr<Node> parse_term();
    std::unique_ptr<Node> parse_factor();
    std::unique_ptr<Node> parse_unary();
    std::unique_ptr<Node> parse_call_or_access();
    std::unique_ptr<Node> parse_primary();

    // Statement parsing
    std::unique_ptr<Node> parse_statement();
    std::unique_ptr<Node> parse_block();
    std::unique_ptr<Node> parse_if_statement();
    std::unique_ptr<Node> parse_while_statement();
    std::unique_ptr<Node> parse_do_while_statement();
    std::unique_ptr<Node> parse_for_statement();
    std::unique_ptr<Node> parse_return_statement();
    std::unique_ptr<Node> parse_break_statement();
    std::unique_ptr<Node> parse_continue_statement();
    std::unique_ptr<Node> parse_switch_statement();
    std::unique_ptr<Node> parse_expression_statement();
    std::unique_ptr<Node> parse_variable_declaration(bool is_const, bool is_ref);

    // Top Level parsing
    std::unique_ptr<Node> parse_top_level_declaration();
    std::unique_ptr<Node> parse_class_declaration(TokenType modifier);
    std::unique_ptr<Node> parse_enum_declaration(TokenType modifier);
    std::unique_ptr<Node> parse_package_statement();
    std::unique_ptr<Node> parse_alias_statement();
    std::unique_ptr<Node> parse_field_or_method(TokenType modifier, bool is_static, bool is_inline, bool is_native, bool is_const, bool is_virtual, bool is_override, bool is_weak, bool is_abstract);
    
    TypeInfo parse_type_info();
};


TypeInfo ParserState::parse_type_info() {
    TypeInfo type;
    const Token& t = advance();
    if (t.type >= TokenType::PRIMITIVE_VOID && t.type <= TokenType::PRIMITIVE_CHAR) {
        // primitive
        if (t.type == TokenType::PRIMITIVE_VOID) type.name = "void";
        else if (t.type == TokenType::PRIMITIVE_BOOL) type.name = "bool";
        else if (t.type == TokenType::PRIMITIVE_INT8) type.name = "int8";
        else if (t.type == TokenType::PRIMITIVE_INT16) type.name = "int16";
        else if (t.type == TokenType::PRIMITIVE_INT32) type.name = "int32";
        else if (t.type == TokenType::PRIMITIVE_INT64) type.name = "int64";
        else if (t.type == TokenType::PRIMITIVE_UINT8) type.name = "uint8";
        else if (t.type == TokenType::PRIMITIVE_UINT16) type.name = "uint16";
        else if (t.type == TokenType::PRIMITIVE_UINT32) type.name = "uint32";
        else if (t.type == TokenType::PRIMITIVE_UINT64) type.name = "uint64";
        else if (t.type == TokenType::PRIMITIVE_FLOAT32) type.name = "float32";
        else if (t.type == TokenType::PRIMITIVE_FLOAT64) type.name = "float64";
        else if (t.type == TokenType::PRIMITIVE_CHAR) type.name = "char";
    } else if (t.type == TokenType::IDENTIFIER) {
        type.name = std::get<std::string>(t.value);
        while (match(TokenType::PUNCTUATION_DOT)) {
            type.name += ".";
            type.name += std::get<std::string>(consume(TokenType::IDENTIFIER, "Expected identifier after '.' in type").value);
        }
    } else {
        throw ParseError("Expected a type name at line " + std::to_string(peek().line));
    }

    while (match(TokenType::PUNCTUATION_ARRAY_BRACKETS)) {
        type.array_depth++;
    }
    
    // In case there are loose brackets like []
    while (check(TokenType::PUNCTUATION_OPEN_BRACKET) && tokens[current+1]->type == TokenType::PUNCTUATION_CLOSE_BRACKET) {
        advance(); advance();
        type.array_depth++;
    }
    
    return type;
}

std::unique_ptr<Node> ParserState::parse_expression() {
    return parse_assignment();
}

std::unique_ptr<Node> ParserState::parse_assignment() {
    std::unique_ptr<Node> expr = parse_ternary();

    if (match({TokenType::OPERATOR_ASSIGN, TokenType::OPERATOR_PLUS_ASSIGN, TokenType::OPERATOR_MINUS_ASSIGN, 
               TokenType::OPERATOR_MULTIPLY_ASSIGN, TokenType::OPERATOR_DIVIDE_ASSIGN, TokenType::OPERATOR_MODULO_ASSIGN})) {
        Token equals = previous();
        std::unique_ptr<Node> value = parse_assignment();

        if (expr->node_type == NodeType::IDENTIFIER || expr->node_type == NodeType::MEMBER_ACCESS || expr->node_type == NodeType::ARRAY_ACCESS) {
            return std::make_unique<AssignmentExpression>(equals, std::move(expr), equals.type, std::move(value));
        }
        throw ParseError("Invalid assignment target");
    }

    return expr;
}

std::unique_ptr<Node> ParserState::parse_ternary() {
    std::unique_ptr<Node> expr = parse_logical_or();
    if (match(TokenType::OPERATOR_QUESTION)) {
        Token quest = previous();
        std::unique_ptr<Node> true_br = parse_expression();
        consume(TokenType::PUNCTUATION_COLON, "Expected ':' in ternary expression");
        std::unique_ptr<Node> false_br = parse_expression();
        return std::make_unique<TernaryExpression>(quest, std::move(expr), std::move(true_br), std::move(false_br));
    }
    return expr;
}

std::unique_ptr<Node> ParserState::parse_logical_or() {
    std::unique_ptr<Node> expr = parse_logical_and();
    while (match(TokenType::OPERATOR_LOGICAL_OR)) {
        Token op = previous();
        std::unique_ptr<Node> right = parse_logical_and();
        expr = std::make_unique<BinaryExpression>(op, std::move(expr), op.type, std::move(right));
    }
    return expr;
}

std::unique_ptr<Node> ParserState::parse_logical_and() {
    std::unique_ptr<Node> expr = parse_equality();
    while (match(TokenType::OPERATOR_LOGICAL_AND)) {
        Token op = previous();
        std::unique_ptr<Node> right = parse_equality();
        expr = std::make_unique<BinaryExpression>(op, std::move(expr), op.type, std::move(right));
    }
    return expr;
}

std::unique_ptr<Node> ParserState::parse_equality() {
    std::unique_ptr<Node> expr = parse_comparison();
    while (match({TokenType::OPERATOR_EQUAL, TokenType::OPERATOR_NOT_EQUAL})) {
        Token op = previous();
        std::unique_ptr<Node> right = parse_comparison();
        expr = std::make_unique<BinaryExpression>(op, std::move(expr), op.type, std::move(right));
    }
    return expr;
}

std::unique_ptr<Node> ParserState::parse_comparison() {
    std::unique_ptr<Node> expr = parse_term();
    while (true) {
        if (match({TokenType::OPERATOR_GREATER_THAN, TokenType::OPERATOR_GREATER_EQUAL, 
                      TokenType::OPERATOR_LESS_THAN, TokenType::OPERATOR_LESS_EQUAL})) {
            Token op = previous();
            std::unique_ptr<Node> right = parse_term();
            expr = std::make_unique<BinaryExpression>(op, std::move(expr), op.type, std::move(right));
        } else if (match(TokenType::KEYWORD_INSTANCEOF)) {
            Token op = previous();
            TypeInfo tgt = parse_type_info();
            expr = std::make_unique<InstanceofExpression>(op, std::move(expr), tgt);
        } else {
            break;
        }
    }
    return expr;
}

std::unique_ptr<Node> ParserState::parse_term() {
    std::unique_ptr<Node> expr = parse_factor();
    while (match({TokenType::OPERATOR_PLUS, TokenType::OPERATOR_MINUS})) {
        Token op = previous();
        std::unique_ptr<Node> right = parse_factor();
        expr = std::make_unique<BinaryExpression>(op, std::move(expr), op.type, std::move(right));
    }
    return expr;
}

std::unique_ptr<Node> ParserState::parse_factor() {
    std::unique_ptr<Node> expr = parse_unary();
    while (match({TokenType::OPERATOR_MULTIPLY, TokenType::OPERATOR_DIVIDE, TokenType::OPERATOR_MODULO})) {
        Token op = previous();
        std::unique_ptr<Node> right = parse_unary();
        expr = std::make_unique<BinaryExpression>(op, std::move(expr), op.type, std::move(right));
    }
    return expr;
}

std::unique_ptr<Node> ParserState::parse_unary() {
    if (match({TokenType::OPERATOR_LOGICAL_NOT, TokenType::OPERATOR_MINUS, TokenType::OPERATOR_INCREMENT, TokenType::OPERATOR_DECREMENT})) {
        Token op = previous();
        std::unique_ptr<Node> right = parse_unary();
        return std::make_unique<UnaryExpression>(op, op.type, std::move(right), true);
    }
    return parse_call_or_access();
}

std::unique_ptr<Node> ParserState::parse_call_or_access() {
    std::unique_ptr<Node> expr = parse_primary();
    
    while (true) {
        if (match(TokenType::PUNCTUATION_OPEN_PAREN)) {
            Token paren = previous();
            auto call_expr = std::make_unique<MethodCallExpression>(paren, std::move(expr));
            if (!check(TokenType::PUNCTUATION_CLOSE_PAREN)) {
                do {
                    call_expr->arguments.push_back(parse_expression());
                } while (match(TokenType::PUNCTUATION_COMMA));
            }
            consume(TokenType::PUNCTUATION_CLOSE_PAREN, "Expected ')' after arguments");
            expr = std::move(call_expr);
        } else if (match(TokenType::PUNCTUATION_DOT)) {
            Token dot = previous();
            Token name = consume(TokenType::IDENTIFIER, "Expected property name after '.'");
            expr = std::make_unique<MemberAccessExpression>(dot, std::move(expr), std::get<std::string>(name.value));
        } else if (match(TokenType::PUNCTUATION_DOUBLE_COLON)) {
            Token d_colon = previous();
            Token name = consume(TokenType::IDENTIFIER, "Expected property name after '::'");
            auto acc = std::make_unique<MemberAccessExpression>(d_colon, std::move(expr), std::get<std::string>(name.value)); acc->is_scope_resolution = true; expr = std::move(acc);
            // Wait, we need to distinguish :: from . in the AST?
            // Actually, in Solix, `Base::method` is functionally a member access with a flag, or we just let binder figure it out!
            // Let's set a flag on MemberAccessExpression if it's static scope resolution.
        } else if (match(TokenType::PUNCTUATION_OPEN_BRACKET)) {
            Token bracket = previous();
            std::unique_ptr<Node> index = parse_expression();
            consume(TokenType::PUNCTUATION_CLOSE_BRACKET, "Expected ']' after array index");
            expr = std::make_unique<ArrayAccessExpression>(bracket, std::move(expr), std::move(index));
        } else {
            break;
        }
    }
    
    // Postfix ++ / --
    if (match({TokenType::OPERATOR_INCREMENT, TokenType::OPERATOR_DECREMENT})) {
        Token op = previous();
        expr = std::make_unique<UnaryExpression>(op, op.type, std::move(expr), false);
    }
    
    return expr;
}

std::unique_ptr<Node> ParserState::parse_primary() {
    if (match(TokenType::NUMBER) || match(TokenType::STRING)) {
        return std::make_unique<LiteralNode>(previous(), previous().value);
    }
    
    if (match(TokenType::IDENTIFIER)) {
        return std::make_unique<IdentifierNode>(previous(), std::get<std::string>(previous().value));
    }
    
    if (match(TokenType::KEYWORD_NEW)) {
        Token new_tok = previous();
        TypeInfo type = parse_type_info();
        
        if (match(TokenType::PUNCTUATION_OPEN_BRACKET)) {
            std::unique_ptr<Node> size = parse_expression();
            consume(TokenType::PUNCTUATION_CLOSE_BRACKET, "Expected ']' after array size");
            return std::make_unique<ArrayCreationExpression>(new_tok, std::move(type), std::move(size));
        } else {
            auto inst = std::make_unique<NewInstanceExpression>(new_tok, std::move(type));
            consume(TokenType::PUNCTUATION_OPEN_PAREN, "Expected '(' after class name in new");
            if (!check(TokenType::PUNCTUATION_CLOSE_PAREN)) {
                do {
                    inst->arguments.push_back(parse_expression());
                } while (match(TokenType::PUNCTUATION_COMMA));
            }
            consume(TokenType::PUNCTUATION_CLOSE_PAREN, "Expected ')' after arguments");
            return inst;
        }
    }
    
    if (match(TokenType::PUNCTUATION_OPEN_BRACE)) {
        // Array literal {1, 2, 3}
        Token brace = previous();
        auto arr_lit = std::make_unique<ArrayLiteralExpression>(brace);
        if (!check(TokenType::PUNCTUATION_CLOSE_BRACE)) {
            do {
                arr_lit->elements.push_back(parse_expression());
            } while (match(TokenType::PUNCTUATION_COMMA));
        }
        consume(TokenType::PUNCTUATION_CLOSE_BRACE, "Expected '}' at end of array literal");
        return arr_lit;
    }
    
    if (match(TokenType::PUNCTUATION_OPEN_PAREN)) {
        // Could be a cast like (int32)x or grouping (1 + 2)
        // A proper parser distinguishes this by checking if the next token is a primitive or known type.
        // For simplicity, if we see a type we try to parse it as cast.
        Token paren = previous();
        
        if ((peek().type >= TokenType::PRIMITIVE_VOID && peek().type <= TokenType::PRIMITIVE_CHAR) || 
            (peek().type == TokenType::IDENTIFIER && (tokens[current+1]->type == TokenType::PUNCTUATION_CLOSE_PAREN || tokens[current+1]->type == TokenType::PUNCTUATION_ARRAY_BRACKETS))) {
            
            // It's likely a cast
            size_t restore = current;
            try {
                TypeInfo cast_type = parse_type_info();
                if (match(TokenType::PUNCTUATION_CLOSE_PAREN)) {
                    std::unique_ptr<Node> expr = parse_expression();
                    return std::make_unique<CastExpression>(paren, std::move(cast_type), std::move(expr));
                }
            } catch(...) {
                current = restore;
            }
            current = restore;
        }
        
        std::unique_ptr<Node> expr = parse_expression();
        consume(TokenType::PUNCTUATION_CLOSE_PAREN, "Expected ')' after expression");
        return expr;
    }
    
    throw ParseError("Expected expression");
}

std::unique_ptr<Node> ParserState::parse_block() {
    Token brace = consume(TokenType::PUNCTUATION_OPEN_BRACE, "Expected '{' before block");
    auto block = std::make_unique<BlockStatement>(brace);
    while (!check(TokenType::PUNCTUATION_CLOSE_BRACE) && !is_at_end()) {
        block->children.push_back(parse_statement());
    }
    consume(TokenType::PUNCTUATION_CLOSE_BRACE, "Expected '}' after block");
    return block;
}

std::unique_ptr<Node> ParserState::parse_statement() {
    if (check(TokenType::PUNCTUATION_OPEN_BRACE)) return parse_block();
    if (match(TokenType::KEYWORD_IF)) return parse_if_statement();
    if (match(TokenType::KEYWORD_WHILE)) return parse_while_statement();
    if (match(TokenType::KEYWORD_DO)) return parse_do_while_statement();
    if (match(TokenType::KEYWORD_FOR)) return parse_for_statement();
    if (match(TokenType::KEYWORD_SWITCH)) return parse_switch_statement();
    if (match(TokenType::KEYWORD_RETURN)) return parse_return_statement();
    if (match(TokenType::KEYWORD_BREAK)) return parse_break_statement();
    if (match(TokenType::KEYWORD_CONTINUE)) return parse_continue_statement();
    
    // Check if it's a variable declaration: const Type, Type id, etc.
    if (match(TokenType::KEYWORD_CONST)) {
        return parse_variable_declaration(true, false);
    }
    
    // Check for variable declaration starting with type: primitive or Identifier followed by identifier
    bool is_var_decl = false;
    if (peek().type >= TokenType::PRIMITIVE_VOID && peek().type <= TokenType::PRIMITIVE_CHAR) is_var_decl = true;
    else if (peek().type == TokenType::IDENTIFIER) {
        size_t temp = current;
        while (temp < tokens.size() && tokens[temp]->type == TokenType::IDENTIFIER) {
            temp++;
            if (temp < tokens.size() && tokens[temp]->type == TokenType::PUNCTUATION_DOT) {
                temp++;
            } else {
                break;
            }
        }
        while (temp < tokens.size() && (tokens[temp]->type == TokenType::PUNCTUATION_ARRAY_BRACKETS || 
              (tokens[temp]->type == TokenType::PUNCTUATION_OPEN_BRACKET && temp+1 < tokens.size() && tokens[temp+1]->type == TokenType::PUNCTUATION_CLOSE_BRACKET))) {
            if (tokens[temp]->type == TokenType::PUNCTUATION_OPEN_BRACKET) temp += 2;
            else temp++;
        }
        if (temp < tokens.size() && tokens[temp]->type == TokenType::OPERATOR_LOGICAL_AND) temp++;
        if (temp < tokens.size() && tokens[temp]->type == TokenType::IDENTIFIER) is_var_decl = true;
    }
    
    if (is_var_decl) {
             
        // It could be an identifier array or reference `Type& id`
        size_t restore = current;
        try {
            TypeInfo type = parse_type_info();
            bool is_ref = match(TokenType::OPERATOR_LOGICAL_AND); // Using & for ref if needed
            if (check(TokenType::IDENTIFIER)) {
                current = restore;
                return parse_variable_declaration(false, false); // parse_var_decl does its own parsing
            }
        } catch(...) {
            current = restore;
        }
        current = restore;
    }
    
    return parse_expression_statement();
}

std::unique_ptr<Node> ParserState::parse_if_statement() {
    Token if_tok = previous();
    consume(TokenType::PUNCTUATION_OPEN_PAREN, "Expected '(' after 'if'");
    std::unique_ptr<Node> condition = parse_expression();
    consume(TokenType::PUNCTUATION_CLOSE_PAREN, "Expected ')' after if condition");
    
    std::unique_ptr<Node> then_branch = parse_statement();
    std::unique_ptr<Node> else_branch = nullptr;
    if (match(TokenType::KEYWORD_ELSE)) {
        else_branch = parse_statement();
    }
    
    return std::make_unique<IfStatement>(if_tok, std::move(condition), std::move(then_branch), std::move(else_branch));
}

std::unique_ptr<Node> ParserState::parse_while_statement() {
    Token while_tok = previous();
    consume(TokenType::PUNCTUATION_OPEN_PAREN, "Expected '(' after 'while'");
    std::unique_ptr<Node> condition = parse_expression();
    consume(TokenType::PUNCTUATION_CLOSE_PAREN, "Expected ')' after condition");
    std::unique_ptr<Node> body = parse_statement();
    
    auto stmt = std::make_unique<WhileStatement>(while_tok);
    stmt->condition = std::move(condition);
    stmt->body = std::move(body);
    return stmt;
}

std::unique_ptr<Node> ParserState::parse_do_while_statement() {
    Token do_tok = previous();
    std::unique_ptr<Node> body = parse_statement();
    consume(TokenType::KEYWORD_WHILE, "Expected 'while' after do body");
    consume(TokenType::PUNCTUATION_OPEN_PAREN, "Expected '(' after 'while'");
    std::unique_ptr<Node> condition = parse_expression();
    consume(TokenType::PUNCTUATION_CLOSE_PAREN, "Expected ')' after condition");
    consume(TokenType::PUNCTUATION_SEMICOLON, "Expected ';' after do-while");
    
    auto stmt = std::make_unique<DoWhileStatement>(do_tok);
    stmt->body = std::move(body);
    stmt->condition = std::move(condition);
    return stmt;
}

std::unique_ptr<Node> ParserState::parse_for_statement() {
    Token for_tok = previous();
    consume(TokenType::PUNCTUATION_OPEN_PAREN, "Expected '(' after 'for'");
    
    auto stmt = std::make_unique<ForStatement>(for_tok);
    
    if (match(TokenType::PUNCTUATION_SEMICOLON)) {
        stmt->initialization = nullptr;
    } else if (check(TokenType::KEYWORD_CONST) || (peek().type >= TokenType::PRIMITIVE_VOID && peek().type <= TokenType::PRIMITIVE_CHAR) ||
               (peek().type == TokenType::IDENTIFIER && tokens[current+1]->type == TokenType::IDENTIFIER)) {
        bool is_const = match(TokenType::KEYWORD_CONST);
        stmt->initialization = parse_variable_declaration(is_const, false);
    } else {
        stmt->initialization = parse_expression_statement();
    }
    
    if (!check(TokenType::PUNCTUATION_SEMICOLON)) {
        stmt->condition = parse_expression();
    }
    consume(TokenType::PUNCTUATION_SEMICOLON, "Expected ';' after loop condition");
    
    if (!check(TokenType::PUNCTUATION_CLOSE_PAREN)) {
        stmt->iteration = parse_expression();
    }
    consume(TokenType::PUNCTUATION_CLOSE_PAREN, "Expected ')' after for clauses");
    
    stmt->body = parse_statement();
    return stmt;
}

std::unique_ptr<Node> ParserState::parse_switch_statement() {
    Token switch_tok = previous();
    consume(TokenType::PUNCTUATION_OPEN_PAREN, "Expected '(' after 'switch'");
    std::unique_ptr<Node> condition = parse_expression();
    consume(TokenType::PUNCTUATION_CLOSE_PAREN, "Expected ')' after switch condition");
    
    auto stmt = std::make_unique<SwitchStatement>(switch_tok);
    stmt->condition = std::move(condition);
    
    consume(TokenType::PUNCTUATION_OPEN_BRACE, "Expected '{' before switch cases");
    while (!check(TokenType::PUNCTUATION_CLOSE_BRACE) && !is_at_end()) {
        if (match(TokenType::KEYWORD_CASE)) {
            auto case_stmt = std::make_unique<CaseStatement>(previous());
            case_stmt->case_value = parse_expression();
            consume(TokenType::PUNCTUATION_COLON, "Expected ':' after case value");
            stmt->children.push_back(std::move(case_stmt));
        } else if (match(TokenType::KEYWORD_DEFAULT)) {
            auto case_stmt = std::make_unique<CaseStatement>(previous());
            case_stmt->is_default = true;
            consume(TokenType::PUNCTUATION_COLON, "Expected ':' after default");
            stmt->children.push_back(std::move(case_stmt));
        } else {
            // It's a statement inside the case
            if (stmt->children.empty()) throw ParseError("Statement without a preceding case in switch");
            stmt->children.back()->children.push_back(parse_statement());
        }
    }
    consume(TokenType::PUNCTUATION_CLOSE_BRACE, "Expected '}' after switch cases");
    
    return stmt;
}

std::unique_ptr<Node> ParserState::parse_return_statement() {
    Token ret_tok = previous();
    std::unique_ptr<Node> value = nullptr;
    if (!check(TokenType::PUNCTUATION_SEMICOLON)) {
        value = parse_expression();
    }
    consume(TokenType::PUNCTUATION_SEMICOLON, "Expected ';' after return");
    return std::make_unique<ReturnStatement>(ret_tok, std::move(value));
}

std::unique_ptr<Node> ParserState::parse_break_statement() {
    Token b_tok = previous();
    consume(TokenType::PUNCTUATION_SEMICOLON, "Expected ';' after break");
    return std::make_unique<BreakStatement>(b_tok);
}

std::unique_ptr<Node> ParserState::parse_continue_statement() {
    Token c_tok = previous();
    consume(TokenType::PUNCTUATION_SEMICOLON, "Expected ';' after continue");
    return std::make_unique<ContinueStatement>(c_tok);
}

std::unique_ptr<Node> ParserState::parse_expression_statement() {
    std::unique_ptr<Node> expr = parse_expression();
    consume(TokenType::PUNCTUATION_SEMICOLON, "Expected ';' after expression");
    return std::make_unique<ExpressionStatement>(expr->node_type == NodeType::UNKNOWN ? previous() : Token{TokenType::UNKNOWN_TOKEN, expr->line, expr->column, expr->source, std::nullptr_t{}}, std::move(expr));
}

std::unique_ptr<Node> ParserState::parse_variable_declaration(bool is_const, bool is_ref) {
    Token start = peek();
    TypeInfo type = parse_type_info();
    if (match(TokenType::OPERATOR_LOGICAL_AND)) {
        is_ref = true;
    }
    
    Token name_tok = consume(TokenType::IDENTIFIER, "Expected variable name");
    auto decl = std::make_unique<VariableDeclaration>(name_tok, std::get<std::string>(name_tok.value), std::move(type));
    decl->is_const = is_const;
    decl->is_reference_type = is_ref;
    
    if (match(TokenType::OPERATOR_ASSIGN)) {
        decl->initializer = parse_expression();
    }
    consume(TokenType::PUNCTUATION_SEMICOLON, "Expected ';' after variable declaration");
    return decl;
}


std::unique_ptr<Node> ParserState::parse_top_level_declaration() {
    if (match(TokenType::KEYWORD_PACKAGE)) return parse_package_statement();
    if (match(TokenType::KEYWORD_ALIAS)) return parse_alias_statement();
    
    TokenType modifier = TokenType::KEYWORD_INTERNAL;
        bool is_static = false, is_inline = false, is_native = false, is_const = false, is_virtual = false, is_override = false, is_weak = false, is_abstract = false;
    while (true) {
        if (match({TokenType::KEYWORD_PUBLIC, TokenType::KEYWORD_PRIVATE, TokenType::KEYWORD_PROTECTED, TokenType::KEYWORD_INTERNAL})) {
            modifier = previous().type;
        } else if (match(TokenType::KEYWORD_STATIC)) {
            is_static = true;
        } else if (match(TokenType::KEYWORD_INLINE)) {
            is_inline = true;
        } else if (match(TokenType::KEYWORD_NATIVE)) {
            is_native = true;
        } else if (match(TokenType::KEYWORD_CONST)) {
            is_const = true;
        } else {
            break;
        }
    }
    
    if (match(TokenType::KEYWORD_CLASS)) return parse_class_declaration(modifier);
    if (match(TokenType::KEYWORD_ENUM)) return parse_enum_declaration(modifier);
    
    // Fallback: it could be a free function or a global variable
    return parse_field_or_method(modifier, is_static, is_inline, is_native, is_const, false, false, false, false);
}

std::unique_ptr<Node> ParserState::parse_package_statement() {
    Token pkg = previous();
    std::string name_str = std::get<std::string>(consume(TokenType::IDENTIFIER, "Expected package name").value);
    while (match(TokenType::PUNCTUATION_DOT)) {
        name_str += ".";
        name_str += std::get<std::string>(consume(TokenType::IDENTIFIER, "Expected sub-package name after '.'").value);
    }
    consume(TokenType::PUNCTUATION_SEMICOLON, "Expected ';' after package declaration");
    return std::make_unique<PackageStatement>(pkg, name_str);
}

std::unique_ptr<Node> ParserState::parse_alias_statement() {
    Token alias = previous();
    Token name = consume(TokenType::IDENTIFIER, "Expected alias name");
    consume(TokenType::OPERATOR_ASSIGN, "Expected '=' in alias declaration");
    TypeInfo type = parse_type_info();
    consume(TokenType::PUNCTUATION_SEMICOLON, "Expected ';' after alias declaration");
    return std::make_unique<AliasStatement>(alias, std::get<std::string>(name.value), std::move(type));
}

std::unique_ptr<Node> ParserState::parse_enum_declaration(TokenType modifier) {
    Token enum_tok = previous();
    Token name = consume(TokenType::IDENTIFIER, "Expected enum name");
    auto decl = std::make_unique<EnumDeclaration>(enum_tok, std::get<std::string>(name.value));
    decl->access_modifier = modifier;
    
    consume(TokenType::PUNCTUATION_OPEN_BRACE, "Expected '{' before enum body");
    while (!check(TokenType::PUNCTUATION_CLOSE_BRACE) && !is_at_end()) {
        Token member = consume(TokenType::IDENTIFIER, "Expected enum member");
        decl->members.push_back(std::get<std::string>(member.value));
        if (!match(TokenType::PUNCTUATION_COMMA)) break;
    }
    consume(TokenType::PUNCTUATION_CLOSE_BRACE, "Expected '}' after enum body");
    return decl;
}

std::unique_ptr<Node> ParserState::parse_class_declaration(TokenType modifier) {
    Token class_tok = previous();
    Token name = consume(TokenType::IDENTIFIER, "Expected class name");
    auto decl = std::make_unique<ClassDeclaration>(class_tok, std::get<std::string>(name.value));

    if (match(TokenType::KEYWORD_EXTENDS)) {
        decl->base_class_name = std::get<std::string>(consume(TokenType::IDENTIFIER, "Expected base class name after 'extends'").value);
    }
    decl->access_modifier = modifier;
    
    consume(TokenType::PUNCTUATION_OPEN_BRACE, "Expected '{' before class body");
    while (!check(TokenType::PUNCTUATION_CLOSE_BRACE) && !is_at_end()) {
        TokenType field_mod = TokenType::KEYWORD_PRIVATE;
        bool is_static = false, is_inline = false, is_native = false, is_const = false, is_virtual = false, is_override = false, is_weak = false, is_abstract = false;
        while (true) {
            if (match({TokenType::KEYWORD_PUBLIC, TokenType::KEYWORD_PRIVATE, TokenType::KEYWORD_PROTECTED, TokenType::KEYWORD_INTERNAL})) {
                field_mod = previous().type;
            } else if (match(TokenType::KEYWORD_STATIC)) {
                is_static = true;
            } else if (match(TokenType::KEYWORD_INLINE)) {
                is_inline = true;
            } else if (match(TokenType::KEYWORD_NATIVE)) {
                is_native = true;
            } else if (match(TokenType::KEYWORD_CONST)) {
                is_const = true;
            } else if (match(TokenType::KEYWORD_VIRTUAL)) {
                is_virtual = true;
            } else if (match(TokenType::KEYWORD_OVERRIDE)) {
                is_override = true;
            } else if (match(TokenType::KEYWORD_WEAK)) {
                is_weak = true;
            } else if (match(TokenType::KEYWORD_ABSTRACT)) {
                is_abstract = true;
            } else {
                break;
            }
        }
        
        if (match(TokenType::KEYWORD_CLASS)) { decl->children.push_back(parse_class_declaration(field_mod)); continue; }
        if (match(TokenType::KEYWORD_ENUM)) { decl->children.push_back(parse_enum_declaration(field_mod)); continue; }
        
        // Is it a constructor?
        if (check(TokenType::IDENTIFIER) && std::get<std::string>(peek().value) == decl->class_name && tokens[current+1]->type == TokenType::PUNCTUATION_OPEN_PAREN) {
            Token ctor_name = advance();
            auto ctor = std::make_unique<ConstructorDeclaration>(ctor_name, decl->class_name);
            ctor->access_modifier = field_mod;
            
            consume(TokenType::PUNCTUATION_OPEN_PAREN, "Expected '(' after constructor name");
            while (!check(TokenType::PUNCTUATION_CLOSE_PAREN) && !is_at_end()) {
                TypeInfo p_type = parse_type_info();
                bool p_ref = match(TokenType::OPERATOR_LOGICAL_AND);
                Token p_name = consume(TokenType::IDENTIFIER, "Expected parameter name");
                auto param = std::make_unique<VariableDeclaration>(p_name, std::get<std::string>(p_name.value), std::move(p_type));
                param->is_reference_type = p_ref;
                ctor->parameters.push_back(std::move(param));
                if (!match(TokenType::PUNCTUATION_COMMA)) break;
            }
            consume(TokenType::PUNCTUATION_CLOSE_PAREN, "Expected ')' after constructor parameters");

            std::vector<std::unique_ptr<Node>> injected_initializers;
            if (match(TokenType::PUNCTUATION_COLON)) {
                do {
                    if (match(TokenType::KEYWORD_SUPER)) {
                        Token super_tok = previous();
                        consume(TokenType::PUNCTUATION_OPEN_PAREN, "Expected '(' after super");
                        std::vector<std::unique_ptr<Node>> args;
                        while (!check(TokenType::PUNCTUATION_CLOSE_PAREN) && !is_at_end()) {
                            args.push_back(parse_expression());
                            if (!match(TokenType::PUNCTUATION_COMMA)) break;
                        }
                        consume(TokenType::PUNCTUATION_CLOSE_PAREN, "Expected ')' after super arguments");
                        auto super_id = std::make_unique<IdentifierNode>(super_tok, "super");
                        auto super_call = std::make_unique<MethodCallExpression>(super_tok, std::move(super_id));
                        super_call->arguments = std::move(args);
                        auto expr_stmt = std::make_unique<ExpressionStatement>(super_tok, std::move(super_call));
                        injected_initializers.push_back(std::move(expr_stmt));
                    } else {
                        Token field_name = consume(TokenType::IDENTIFIER, "Expected 'super' or field name in initializer list");
                        consume(TokenType::PUNCTUATION_OPEN_PAREN, "Expected '(' after field name");
                        auto val = parse_expression();
                        consume(TokenType::PUNCTUATION_CLOSE_PAREN, "Expected ')' after field value");
                        
                        auto this_id = std::make_unique<IdentifierNode>(field_name, "this");
                        auto field_acc = std::make_unique<MemberAccessExpression>(field_name, std::move(this_id), std::get<std::string>(field_name.value));
                        auto assign = std::make_unique<AssignmentExpression>(field_name, std::move(field_acc), TokenType::OPERATOR_ASSIGN, std::move(val));
                        auto expr_stmt2 = std::make_unique<ExpressionStatement>(field_name, std::move(assign));
                        injected_initializers.push_back(std::move(expr_stmt2));
                    }
                } while (match(TokenType::PUNCTUATION_COMMA));
            }
            
            auto body = parse_block();
            for (auto it = injected_initializers.rbegin(); it != injected_initializers.rend(); ++it) {
                (*it)->parent = body.get();
                body->children.insert(body->children.begin(), std::move(*it));
            }
            ctor->children.push_back(std::move(body));
            decl->children.push_back(std::move(ctor));
        } else {
            decl->children.push_back(parse_field_or_method(field_mod, is_static, is_inline, is_native, is_const, is_virtual, is_override, is_weak, is_abstract));

        }
    }
    consume(TokenType::PUNCTUATION_CLOSE_BRACE, "Expected '}' after class body");
    
    for (auto& child : decl->children) {
        child->parent = decl.get();
    }
    
    return decl;
}

std::unique_ptr<Node> ParserState::parse_field_or_method(TokenType modifier, bool is_static, bool is_inline, bool is_native, bool is_const, bool is_virtual, bool is_override, bool is_weak, bool is_abstract) {
    
    TypeInfo type = parse_type_info();
    bool is_ref = match(TokenType::OPERATOR_LOGICAL_AND);

    Token name;
    std::string name_str;
    if (match(TokenType::KEYWORD_OPERATOR)) {
        name = previous();
        Token op_token = peek();
        advance();
        name_str = "operator";
        if (op_token.type == TokenType::OPERATOR_PLUS) name_str += "+";
        else if (op_token.type == TokenType::OPERATOR_MINUS) name_str += "-";
        else if (op_token.type == TokenType::OPERATOR_MULTIPLY) name_str += "*";
        else if (op_token.type == TokenType::OPERATOR_DIVIDE) name_str += "/";
        else if (op_token.type == TokenType::OPERATOR_ASSIGN) name_str += "=";
        else throw ParseError("Invalid operator for overloading");
    } else {
        name = consume(TokenType::IDENTIFIER, "Expected field or method name");
        name_str = std::get<std::string>(name.value);
    }
    
    if (match(TokenType::PUNCTUATION_OPEN_PAREN)) {
        // It's a method
        auto method = std::make_unique<MethodDeclaration>(name, name_str, std::move(type));
        method->access_modifier = modifier;
        method->is_static = is_static;
        method->is_inline = is_inline;
        method->is_native = is_native;
        method->is_virtual = is_virtual;
        method->is_abstract = is_abstract;
        method->is_override = is_override;
        
        while (!check(TokenType::PUNCTUATION_CLOSE_PAREN) && !is_at_end()) {
            TypeInfo p_type = parse_type_info();
            bool p_ref = match(TokenType::OPERATOR_LOGICAL_AND);
            Token p_name = consume(TokenType::IDENTIFIER, "Expected parameter name");
            auto param = std::make_unique<VariableDeclaration>(p_name, std::get<std::string>(p_name.value), std::move(p_type));
            param->is_reference_type = p_ref;
            method->parameters.push_back(std::move(param));
            if (!match(TokenType::PUNCTUATION_COMMA)) break;
        }
        consume(TokenType::PUNCTUATION_CLOSE_PAREN, "Expected ')' after method parameters");
        
        if (is_native || is_abstract) {
            consume(TokenType::PUNCTUATION_SEMICOLON, "Expected ';' after native or abstract method declaration");
        } else {
            method->children.push_back(parse_block());
        }
        return method;
    } else {
        // It's a field
        auto field = std::make_unique<FieldDeclaration>(name, name_str, std::move(type));
        field->access_modifier = modifier;
        field->is_static = is_static;
        field->is_const = is_const;
        field->is_reference_type = is_ref;
        field->is_weak = is_weak;
        if (match(TokenType::OPERATOR_ASSIGN)) {
            field->initializer = parse_expression();
        }
        consume(TokenType::PUNCTUATION_SEMICOLON, "Expected ';' after field declaration");
        return field;
    }
}

void Parser::execute() {
    log_info("Starting Syntax Analysis (Parsing)...");
    
    for (const auto& [source, token_lists] : context.tokens) {
        if (token_lists.empty()) continue;
        
        const TokenList& tokens = token_lists.front();
        if (tokens.empty()) continue;
        
        ParserState state(tokens, this, &source);
        
        std::string source_name;
        if (std::holds_alternative<std::filesystem::path>(source)) {
            source_name = std::get<std::filesystem::path>(source).string();
        } else {
            source_name = std::get<std::string>(source);
        }
        
        log_trace("Parsing file: {}", source_name);
        
        try {
            while (!state.is_at_end()) {
                context.nodes[source].push_back(state.parse_top_level_declaration());
            }
        } catch (const ParseError& e) {
            log_error("Syntax Error in {}: {}", source_name, e.what());
            // In a real parser we would synchronize and continue
        }
        
        log_debug("Successfully parsed {} into {} top-level AST nodes", source_name, context.nodes[source].size());
    }
    
    log_info("Syntax Analysis completed.");
}

} // namespace solix
