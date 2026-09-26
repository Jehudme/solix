#include "processes/parser.hpp"
#include "solix/compilation.hpp"
#include "utilities/diagnostic.hpp"
#include "utilities/statements.hpp"
#include <stdexcept>

namespace solix {

class ParseError : public std::runtime_error {
public:
  uint32_t line = 0;
  uint32_t column = 0;
  ParseError(const std::string &msg, uint32_t line = 0, uint32_t column = 0)
      : std::runtime_error(msg), line(line), column(column) {}
};

class ParserState {
  std::vector<const Token *> tokens;
  size_t current = 0;
  Parser *process;
  const Source *current_source;
  bool has_parsed_declaration = false;
  bool has_package_statement = false;

public:
  ParserState(const TokenList &tkns, Parser *proc, const Source *src)
      : process(proc), current_source(src) {
    for (const auto &t : tkns) {
      if (t.type != TokenType::COMMENT_SINGLE_LINE &&
          t.type != TokenType::COMMENT_MULTI_LINE) {
        tokens.push_back(&t);
      }
    }
  }

  template <typename... Args>
  void log_trace(const std::string &format, Args &&...args) {
    if (process)
      process->log_trace(format, std::forward<Args>(args)...);
  }

  template <typename... Args>
  void log_debug(const std::string &format, Args &&...args) {
    if (process)
      process->log_debug(format, std::forward<Args>(args)...);
  }

  template <typename... Args>
  void log_info(const std::string &format, Args &&...args) {
    if (process)
      process->log_info(format, std::forward<Args>(args)...);
  }

  template <typename... Args>
  void log_warn(const std::string &format, Args &&...args) {
    if (process)
      process->log_warn(format, std::forward<Args>(args)...);
  }

  template <typename... Args>
  void log_error(const std::string &format, Args &&...args) {
    if (process)
      process->log_error(format, std::forward<Args>(args)...);
  }

  const Token &peek() const { return *tokens[current]; }
  const Token &previous() const { return *tokens[current - 1]; }
  bool is_at_end() const { return peek().type == TokenType::EOF_TOKEN; }

  const Token &advance() {
    if (!is_at_end())
      current++;
    return previous();
  }

  bool check(TokenType type) const {
    if (is_at_end())
      return false;
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

  const Token &consume(TokenType type, const std::string &message) {
    if (check(type))
      return advance();
    throw ParseError(message + " at line " + std::to_string(peek().line) +
                     " col " + std::to_string(peek().column),
                     peek().line, peek().column);
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
  std::unique_ptr<Node> parse_try_statement();
  std::unique_ptr<Node> parse_throw_statement();
  std::unique_ptr<Node> parse_expression_statement();
  std::unique_ptr<Node> parse_variable_declaration(bool is_const, bool is_ref);

  // Top Level parsing
  std::unique_ptr<Node> parse_top_level_declaration();
  std::unique_ptr<Node> parse_class_declaration(TokenType modifier);
  std::unique_ptr<Node> parse_enum_declaration(TokenType modifier);
  std::unique_ptr<Node> parse_package_statement();
  std::unique_ptr<Node> parse_alias_statement();
  std::unique_ptr<Node> parse_import_statement();
  std::unique_ptr<Node> parse_field_or_method(TokenType modifier,
                                              bool is_static, bool is_inline,
                                              bool is_native, bool is_const,
                                              bool is_virtual, bool is_override,
                                              bool is_weak, bool is_abstract);

  TypeInfo parse_type_info();
};

TypeInfo ParserState::parse_type_info() {
  TypeInfo type;
  const Token &t = advance();
  if (t.type >= TokenType::PRIMITIVE_VOID &&
      t.type <= TokenType::PRIMITIVE_CHAR) {
    if (t.type == TokenType::PRIMITIVE_VOID)
      type.name = "void";
    else if (t.type == TokenType::PRIMITIVE_BOOL)
      type.name = "bool";
    else if (t.type == TokenType::PRIMITIVE_INT8)
      type.name = "int8";
    else if (t.type == TokenType::PRIMITIVE_INT16)
      type.name = "int16";
    else if (t.type == TokenType::PRIMITIVE_INT32)
      type.name = "int32";
    else if (t.type == TokenType::PRIMITIVE_INT64)
      type.name = "int64";
    else if (t.type == TokenType::PRIMITIVE_UINT8)
      type.name = "uint8";
    else if (t.type == TokenType::PRIMITIVE_UINT16)
      type.name = "uint16";
    else if (t.type == TokenType::PRIMITIVE_UINT32)
      type.name = "uint32";
    else if (t.type == TokenType::PRIMITIVE_UINT64)
      type.name = "uint64";
    else if (t.type == TokenType::PRIMITIVE_FLOAT32)
      type.name = "float32";
    else if (t.type == TokenType::PRIMITIVE_FLOAT64)
      type.name = "float64";
    else if (t.type == TokenType::PRIMITIVE_CHAR)
      type.name = "char";
  } else if (t.type == TokenType::IDENTIFIER) {
    type.name = std::get<std::string>(t.value);
    while (match(TokenType::PUNCTUATION_DOT) || match(TokenType::PUNCTUATION_DOUBLE_COLON)) {
      type.name += ".";
      type.name +=
          std::get<std::string>(consume(TokenType::IDENTIFIER,
                                        "Expected identifier after '.' or '::' in type")
                                    .value);
    }

    if (match(TokenType::OPERATOR_LESS_THAN)) {
      log_trace("Parsing generic type arguments for {}", type.name);
      do {
        type.type_args.push_back(parse_type_info());
      } while (match(TokenType::PUNCTUATION_COMMA));
      consume(TokenType::OPERATOR_GREATER_THAN,
              "Expected '>' after template arguments");
    }
  } else {
    throw ParseError("Expected a type name at line " +
                     std::to_string(peek().line));
  }

  while (match(TokenType::PUNCTUATION_ARRAY_BRACKETS)) {
    type.array_depth++;
  }

  while (check(TokenType::PUNCTUATION_OPEN_BRACKET) &&
         tokens[current + 1]->type == TokenType::PUNCTUATION_CLOSE_BRACKET) {
    advance();
    advance();
    type.array_depth++;
  }

  log_trace("Parsed type: {}", type.to_string());
  return type;
}

std::unique_ptr<Node> ParserState::parse_expression() {
  return parse_assignment();
}

std::unique_ptr<Node> ParserState::parse_assignment() {
  std::unique_ptr<Node> expr = parse_ternary();

  if (match({TokenType::OPERATOR_ASSIGN, TokenType::OPERATOR_PLUS_ASSIGN,
             TokenType::OPERATOR_MINUS_ASSIGN,
             TokenType::OPERATOR_MULTIPLY_ASSIGN,
             TokenType::OPERATOR_DIVIDE_ASSIGN,
             TokenType::OPERATOR_MODULO_ASSIGN})) {
    Token equals = previous();
    log_trace("Parsing assignment target at line {}", equals.line);
    std::unique_ptr<Node> value = parse_assignment();

    if (expr->node_type == NodeType::IDENTIFIER ||
        expr->node_type == NodeType::MEMBER_ACCESS ||
        expr->node_type == NodeType::ARRAY_ACCESS) {
      return std::make_unique<AssignmentExpression>(
          equals, std::move(expr), equals.type, std::move(value));
    }
    throw ParseError("Invalid assignment target");
  }

  return expr;
}

std::unique_ptr<Node> ParserState::parse_ternary() {
  std::unique_ptr<Node> expr = parse_logical_or();
  if (match(TokenType::OPERATOR_QUESTION)) {
    Token quest = previous();
    log_trace("Parsing ternary expression at line {}", quest.line);
    std::unique_ptr<Node> true_br = parse_expression();
    consume(TokenType::PUNCTUATION_COLON, "Expected ':' in ternary expression");
    std::unique_ptr<Node> false_br = parse_expression();
    return std::make_unique<TernaryExpression>(
        quest, std::move(expr), std::move(true_br), std::move(false_br));
  }
  return expr;
}

std::unique_ptr<Node> ParserState::parse_logical_or() {
  std::unique_ptr<Node> expr = parse_logical_and();
  while (match(TokenType::OPERATOR_LOGICAL_OR)) {
    Token op = previous();
    log_trace("Parsing logical OR expression at line {}", op.line);
    std::unique_ptr<Node> right = parse_logical_and();
    expr = std::make_unique<BinaryExpression>(op, std::move(expr), op.type,
                                              std::move(right));
  }
  return expr;
}

std::unique_ptr<Node> ParserState::parse_logical_and() {
  std::unique_ptr<Node> expr = parse_equality();
  while (match(TokenType::OPERATOR_LOGICAL_AND)) {
    Token op = previous();
    log_trace("Parsing logical AND expression at line {}", op.line);
    std::unique_ptr<Node> right = parse_equality();
    expr = std::make_unique<BinaryExpression>(op, std::move(expr), op.type,
                                              std::move(right));
  }
  return expr;
}

std::unique_ptr<Node> ParserState::parse_equality() {
  std::unique_ptr<Node> expr = parse_comparison();
  while (match({TokenType::OPERATOR_EQUAL, TokenType::OPERATOR_NOT_EQUAL})) {
    Token op = previous();
    log_trace("Parsing equality expression at line {}", op.line);
    std::unique_ptr<Node> right = parse_comparison();
    expr = std::make_unique<BinaryExpression>(op, std::move(expr), op.type,
                                              std::move(right));
  }
  return expr;
}

std::unique_ptr<Node> ParserState::parse_comparison() {
  std::unique_ptr<Node> expr = parse_term();
  while (true) {
    if (match({TokenType::OPERATOR_GREATER_THAN,
               TokenType::OPERATOR_GREATER_EQUAL, TokenType::OPERATOR_LESS_THAN,
               TokenType::OPERATOR_LESS_EQUAL})) {
      Token op = previous();
      log_trace("Parsing comparison expression at line {}", op.line);
      std::unique_ptr<Node> right = parse_term();
      expr = std::make_unique<BinaryExpression>(op, std::move(expr), op.type,
                                                std::move(right));
    } else if (match(TokenType::KEYWORD_INSTANCEOF)) {
      Token op = previous();
      log_trace("Parsing instanceof expression at line {}", op.line);
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
    log_trace("Parsing term expression (+/-) at line {}", op.line);
    std::unique_ptr<Node> right = parse_factor();
    expr = std::make_unique<BinaryExpression>(op, std::move(expr), op.type,
                                              std::move(right));
  }
  return expr;
}

std::unique_ptr<Node> ParserState::parse_factor() {
  std::unique_ptr<Node> expr = parse_unary();
  while (match({TokenType::OPERATOR_MULTIPLY, TokenType::OPERATOR_DIVIDE,
                TokenType::OPERATOR_MODULO})) {
    Token op = previous();
    log_trace("Parsing factor expression (*///%) at line {}", op.line);
    std::unique_ptr<Node> right = parse_unary();
    expr = std::make_unique<BinaryExpression>(op, std::move(expr), op.type,
                                              std::move(right));
  }
  return expr;
}

std::unique_ptr<Node> ParserState::parse_unary() {
  if (match({TokenType::OPERATOR_LOGICAL_NOT, TokenType::OPERATOR_MINUS,
             TokenType::OPERATOR_INCREMENT, TokenType::OPERATOR_DECREMENT})) {
    Token op = previous();
    log_trace("Parsing unary expression at line {}", op.line);
    std::unique_ptr<Node> right = parse_unary();
    return std::make_unique<UnaryExpression>(op, op.type, std::move(right),
                                             true);
  }
  return parse_call_or_access();
}

std::unique_ptr<Node> ParserState::parse_call_or_access() {
  std::unique_ptr<Node> expr = parse_primary();
  std::vector<TypeInfo> pending_type_args;

  while (true) {
    if (peek().type == TokenType::OPERATOR_LESS_THAN) {
      size_t temp = current;
      int bracket_count = 1;
      temp++;
      bool valid_template = true;
      while (temp < tokens.size() && bracket_count > 0) {
        if (tokens[temp]->type == TokenType::OPERATOR_LESS_THAN)
          bracket_count++;
        else if (tokens[temp]->type == TokenType::OPERATOR_GREATER_THAN)
          bracket_count--;
        else if (tokens[temp]->type == TokenType::PUNCTUATION_SEMICOLON) {
          valid_template = false;
          break;
        } else if (tokens[temp]->type == TokenType::PUNCTUATION_OPEN_BRACE) {
          valid_template = false;
          break;
        }
        temp++;
      }
      if (valid_template && temp < tokens.size() &&
          tokens[temp]->type == TokenType::PUNCTUATION_OPEN_PAREN) {
        log_debug(
            "Parsing explicit template arguments for method call at line {}",
            peek().line);
        advance(); // consume '<'
        do {
          pending_type_args.push_back(parse_type_info());
        } while (match(TokenType::PUNCTUATION_COMMA));
        consume(TokenType::OPERATOR_GREATER_THAN,
                "Expected '>' after generic method arguments");
        continue;
      }
    }

    if (match(TokenType::PUNCTUATION_OPEN_PAREN)) {
      Token paren = previous();
      log_trace("Parsing method call arguments at line {}", paren.line);
      auto call_expr =
          std::make_unique<MethodCallExpression>(paren, std::move(expr));
      call_expr->type_args = std::move(pending_type_args);
      pending_type_args.clear();
      if (!check(TokenType::PUNCTUATION_CLOSE_PAREN)) {
        do {
          call_expr->arguments.push_back(parse_expression());
        } while (match(TokenType::PUNCTUATION_COMMA));
      }
      consume(TokenType::PUNCTUATION_CLOSE_PAREN,
              "Expected ')' after arguments");
      expr = std::move(call_expr);
    } else if (match(TokenType::PUNCTUATION_DOT)) {
      Token dot = previous();
      Token name =
          consume(TokenType::IDENTIFIER, "Expected property name after '.'");
      std::string prop = std::get<std::string>(name.value);
      log_trace("Parsed member access: .{}", prop);
      expr =
          std::make_unique<MemberAccessExpression>(dot, std::move(expr), prop);
    } else if (match(TokenType::PUNCTUATION_DOUBLE_COLON)) {
      Token d_colon = previous();
      Token name =
          consume(TokenType::IDENTIFIER, "Expected property name after '::'");
      std::string prop = std::get<std::string>(name.value);
      log_trace("Parsed scope resolution: ::{}", prop);
      auto acc = std::make_unique<MemberAccessExpression>(
          d_colon, std::move(expr), prop);
      acc->is_scope_resolution = true;
      expr = std::move(acc);
    } else if (match(TokenType::PUNCTUATION_OPEN_BRACKET)) {
      Token bracket = previous();
      log_trace("Parsing array index access at line {}", bracket.line);
      std::unique_ptr<Node> index = parse_expression();
      consume(TokenType::PUNCTUATION_CLOSE_BRACKET,
              "Expected ']' after array index");
      expr = std::make_unique<ArrayAccessExpression>(bracket, std::move(expr),
                                                     std::move(index));
    } else {
      break;
    }
  }

  // Postfix ++ / --
  if (match({TokenType::OPERATOR_INCREMENT, TokenType::OPERATOR_DECREMENT})) {
    Token op = previous();
    log_trace("Parsing postfix operator at line {}", op.line);
    expr =
        std::make_unique<UnaryExpression>(op, op.type, std::move(expr), false);
  }

  return expr;
}

std::unique_ptr<Node> ParserState::parse_primary() {
  if (match(TokenType::NUMBER) || match(TokenType::STRING) || match(TokenType::CHAR)) {
    return std::make_unique<LiteralNode>(previous(), previous().value);
  }

  if (match(TokenType::IDENTIFIER)) {
    return std::make_unique<IdentifierNode>(
        previous(), std::get<std::string>(previous().value));
  }

  if (match(TokenType::KEYWORD_NEW)) {
    Token new_tok = previous();
    TypeInfo type = parse_type_info();

    if (match(TokenType::PUNCTUATION_OPEN_BRACKET)) {
      log_trace("Parsing new array creation for {} at line {}",
                type.to_string(), new_tok.line);
      std::unique_ptr<Node> size = parse_expression();
      consume(TokenType::PUNCTUATION_CLOSE_BRACKET,
              "Expected ']' after array size");
      return std::make_unique<ArrayCreationExpression>(new_tok, std::move(type),
                                                       std::move(size));
    } else {
      log_trace("Parsing new instance creation for {} at line {}",
                type.to_string(), new_tok.line);
      auto inst =
          std::make_unique<NewInstanceExpression>(new_tok, std::move(type));
      consume(TokenType::PUNCTUATION_OPEN_PAREN,
              "Expected '(' after class name in new");
      if (!check(TokenType::PUNCTUATION_CLOSE_PAREN)) {
        do {
          inst->arguments.push_back(parse_expression());
        } while (match(TokenType::PUNCTUATION_COMMA));
      }
      consume(TokenType::PUNCTUATION_CLOSE_PAREN,
              "Expected ')' after arguments");
      return inst;
    }
  }

  if (match(TokenType::PUNCTUATION_OPEN_BRACE)) {
    Token brace = previous();
    log_trace("Parsing array literal at line {}", brace.line);
    auto arr_lit = std::make_unique<ArrayLiteralExpression>(brace);
    if (!check(TokenType::PUNCTUATION_CLOSE_BRACE)) {
      do {
        arr_lit->elements.push_back(parse_expression());
      } while (match(TokenType::PUNCTUATION_COMMA));
    }
    consume(TokenType::PUNCTUATION_CLOSE_BRACE,
            "Expected '}' at end of array literal");
    return arr_lit;
  }

  if (match(TokenType::PUNCTUATION_OPEN_PAREN)) {
    Token paren = previous();
    if ((peek().type >= TokenType::PRIMITIVE_VOID &&
         peek().type <= TokenType::PRIMITIVE_CHAR) ||
        (peek().type == TokenType::IDENTIFIER &&
         (tokens[current + 1]->type == TokenType::PUNCTUATION_CLOSE_PAREN ||
          tokens[current + 1]->type ==
              TokenType::PUNCTUATION_ARRAY_BRACKETS))) {

      size_t restore = current;
      try {
        TypeInfo cast_type = parse_type_info();
        if (match(TokenType::PUNCTUATION_CLOSE_PAREN)) {
          log_trace("Parsing explicit cast to {} at line {}",
                    cast_type.to_string(), paren.line);
          std::unique_ptr<Node> expr = parse_unary();
          return std::make_unique<CastExpression>(paren, std::move(cast_type),
                                                  std::move(expr));
        }
      } catch (...) {
        current = restore;
      }
      current = restore;
    }

    std::unique_ptr<Node> expr = parse_expression();
    consume(TokenType::PUNCTUATION_CLOSE_PAREN,
            "Expected ')' after expression");
    return expr;
  }

  throw ParseError("Expected expression, got " +
                   std::to_string((int)peek().type));
}

std::unique_ptr<Node> ParserState::parse_block() {
  Token brace =
      consume(TokenType::PUNCTUATION_OPEN_BRACE, "Expected '{' before block");
  log_trace("Entering block statement at line {}", brace.line);
  auto block = std::make_unique<BlockStatement>(brace);
  while (!check(TokenType::PUNCTUATION_CLOSE_BRACE) && !is_at_end()) {
    block->children.push_back(parse_statement());
  }
  consume(TokenType::PUNCTUATION_CLOSE_BRACE, "Expected '}' after block");
  log_trace("Exiting block statement at line {} with {} statements", brace.line,
            block->children.size());
  return block;
}

std::unique_ptr<Node> ParserState::parse_statement() {
  if (check(TokenType::PUNCTUATION_OPEN_BRACE))
    return parse_block();
  if (match(TokenType::KEYWORD_IF))
    return parse_if_statement();
  if (match(TokenType::KEYWORD_WHILE))
    return parse_while_statement();
  if (match(TokenType::KEYWORD_DO))
    return parse_do_while_statement();
  if (match(TokenType::KEYWORD_FOR))
    return parse_for_statement();
  if (match(TokenType::KEYWORD_SWITCH))
    return parse_switch_statement();
  if (match(TokenType::KEYWORD_TRY))
    return parse_try_statement();
  if (match(TokenType::KEYWORD_THROW))
    return parse_throw_statement();
  if (match(TokenType::KEYWORD_RETURN))
    return parse_return_statement();
  if (match(TokenType::KEYWORD_BREAK))
    return parse_break_statement();
  if (match(TokenType::KEYWORD_CONTINUE))
    return parse_continue_statement();

  if (match(TokenType::KEYWORD_CONST)) {
    return parse_variable_declaration(true, false);
  }

  bool is_var_decl = false;
  if (peek().type >= TokenType::PRIMITIVE_VOID &&
      peek().type <= TokenType::PRIMITIVE_CHAR)
    is_var_decl = true;
  else if (peek().type == TokenType::IDENTIFIER) {
    size_t temp = current;
    while (temp < tokens.size() &&
           tokens[temp]->type == TokenType::IDENTIFIER) {
      temp++;
      if (temp < tokens.size() &&
          (tokens[temp]->type == TokenType::PUNCTUATION_DOT ||
           tokens[temp]->type == TokenType::PUNCTUATION_DOUBLE_COLON)) {
        temp++;
      } else {
        break;
      }
    }
    if (temp < tokens.size() &&
        tokens[temp]->type == TokenType::OPERATOR_LESS_THAN) {
      int bracket_count = 1;
      temp++;
      while (temp < tokens.size() && bracket_count > 0) {
        if (tokens[temp]->type == TokenType::OPERATOR_LESS_THAN)
          bracket_count++;
        else if (tokens[temp]->type == TokenType::OPERATOR_GREATER_THAN)
          bracket_count--;
        temp++;
      }
    }
    while (temp < tokens.size() &&
           (tokens[temp]->type == TokenType::PUNCTUATION_ARRAY_BRACKETS ||
            (tokens[temp]->type == TokenType::PUNCTUATION_OPEN_BRACKET &&
             temp + 1 < tokens.size() &&
             tokens[temp + 1]->type == TokenType::PUNCTUATION_CLOSE_BRACKET))) {
      if (tokens[temp]->type == TokenType::PUNCTUATION_OPEN_BRACKET)
        temp += 2;
      else
        temp++;
    }
    if (temp < tokens.size() &&
        tokens[temp]->type == TokenType::PUNCTUATION_AMPERSAND)
      temp++;
    if (temp < tokens.size() && tokens[temp]->type == TokenType::IDENTIFIER)
      is_var_decl = true;
  }

  if (is_var_decl) {
    size_t restore = current;
    try {
      TypeInfo type = parse_type_info();
      bool is_ref = match(TokenType::PUNCTUATION_AMPERSAND);
      if (check(TokenType::IDENTIFIER)) {
        current = restore;
        return parse_variable_declaration(false, false);
      }
    } catch (...) {
      current = restore;
    }
    current = restore;
  }

  return parse_expression_statement();
}

std::unique_ptr<Node> ParserState::parse_if_statement() {
  Token if_tok = previous();
  log_trace("Parsing 'if' statement at line {}", if_tok.line);
  consume(TokenType::PUNCTUATION_OPEN_PAREN, "Expected '(' after 'if'");
  std::unique_ptr<Node> condition = parse_expression();
  consume(TokenType::PUNCTUATION_CLOSE_PAREN,
          "Expected ')' after if condition");

  std::unique_ptr<Node> then_branch = parse_statement();
  std::unique_ptr<Node> else_branch = nullptr;
  if (match(TokenType::KEYWORD_ELSE)) {
    log_trace("Parsing 'else' branch for 'if' at line {}", if_tok.line);
    else_branch = parse_statement();
  }

  return std::make_unique<IfStatement>(if_tok, std::move(condition),
                                       std::move(then_branch),
                                       std::move(else_branch));
}

std::unique_ptr<Node> ParserState::parse_while_statement() {
  Token while_tok = previous();
  log_trace("Parsing 'while' loop at line {}", while_tok.line);
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
  log_trace("Parsing 'do-while' loop at line {}", do_tok.line);
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
  log_trace("Parsing 'for' loop at line {}", for_tok.line);
  consume(TokenType::PUNCTUATION_OPEN_PAREN, "Expected '(' after 'for'");

  auto stmt = std::make_unique<ForStatement>(for_tok);

  if (match(TokenType::PUNCTUATION_SEMICOLON)) {
    stmt->initialization = nullptr;
  } else if (check(TokenType::KEYWORD_CONST) ||
             (peek().type >= TokenType::PRIMITIVE_VOID &&
              peek().type <= TokenType::PRIMITIVE_CHAR) ||
             (peek().type == TokenType::IDENTIFIER &&
              tokens[current + 1]->type == TokenType::IDENTIFIER)) {
    bool is_const = match(TokenType::KEYWORD_CONST);
    stmt->initialization = parse_variable_declaration(is_const, false);
  } else {
    stmt->initialization = parse_expression_statement();
  }

  if (!check(TokenType::PUNCTUATION_SEMICOLON)) {
    stmt->condition = parse_expression();
  }
  consume(TokenType::PUNCTUATION_SEMICOLON,
          "Expected ';' after loop condition");

  if (!check(TokenType::PUNCTUATION_CLOSE_PAREN)) {
    stmt->iteration = parse_expression();
  }
  consume(TokenType::PUNCTUATION_CLOSE_PAREN, "Expected ')' after for clauses");

  stmt->body = parse_statement();
  return stmt;
}

std::unique_ptr<Node> ParserState::parse_switch_statement() {
  Token switch_tok = previous();
  log_trace("Parsing 'switch' statement at line {}", switch_tok.line);
  consume(TokenType::PUNCTUATION_OPEN_PAREN, "Expected '(' after 'switch'");
  std::unique_ptr<Node> condition = parse_expression();
  consume(TokenType::PUNCTUATION_CLOSE_PAREN,
          "Expected ')' after switch condition");

  auto stmt = std::make_unique<SwitchStatement>(switch_tok);
  stmt->condition = std::move(condition);

  consume(TokenType::PUNCTUATION_OPEN_BRACE,
          "Expected '{' before switch cases");
  while (!check(TokenType::PUNCTUATION_CLOSE_BRACE) && !is_at_end()) {
    if (match(TokenType::KEYWORD_CASE)) {
      log_trace("Parsing 'case' block at line {}", previous().line);
      auto case_stmt = std::make_unique<CaseStatement>(previous());
      case_stmt->case_value = parse_expression();
      consume(TokenType::PUNCTUATION_COLON, "Expected ':' after case value");
      stmt->children.push_back(std::move(case_stmt));
    } else if (match(TokenType::KEYWORD_DEFAULT)) {
      log_trace("Parsing 'default' case at line {}", previous().line);
      auto case_stmt = std::make_unique<CaseStatement>(previous());
      case_stmt->is_default = true;
      consume(TokenType::PUNCTUATION_COLON, "Expected ':' after default");
      stmt->children.push_back(std::move(case_stmt));
    } else {
      if (stmt->children.empty())
        throw ParseError("Statement without a preceding case in switch");
      stmt->children.back()->children.push_back(parse_statement());
    }
  }
  consume(TokenType::PUNCTUATION_CLOSE_BRACE,
          "Expected '}' after switch cases");

  return stmt;
}

std::unique_ptr<Node> ParserState::parse_return_statement() {
  Token ret_tok = previous();
  log_trace("Parsing 'return' statement at line {}", ret_tok.line);
  std::unique_ptr<Node> value = nullptr;
  if (!check(TokenType::PUNCTUATION_SEMICOLON)) {
    value = parse_expression();
  }
  consume(TokenType::PUNCTUATION_SEMICOLON, "Expected ';' after return");
  return std::make_unique<ReturnStatement>(ret_tok, std::move(value));
}

std::unique_ptr<Node> ParserState::parse_break_statement() {
  Token b_tok = previous();
  log_trace("Parsing 'break' at line {}", b_tok.line);
  consume(TokenType::PUNCTUATION_SEMICOLON, "Expected ';' after break");
  return std::make_unique<BreakStatement>(b_tok);
}

std::unique_ptr<Node> ParserState::parse_continue_statement() {
  Token c_tok = previous();
  log_trace("Parsing 'continue' at line {}", c_tok.line);
  consume(TokenType::PUNCTUATION_SEMICOLON, "Expected ';' after continue");
  return std::make_unique<ContinueStatement>(c_tok);
}

std::unique_ptr<Node> ParserState::parse_expression_statement() {
  std::unique_ptr<Node> expr = parse_expression();
  consume(TokenType::PUNCTUATION_SEMICOLON, "Expected ';' after expression");
  return std::make_unique<ExpressionStatement>(
      expr->node_type == NodeType::UNKNOWN
          ? previous()
          : Token{TokenType::UNKNOWN_TOKEN, expr->line, expr->column,
                  expr->source, std::nullptr_t{}},
      std::move(expr));
}

std::unique_ptr<Node> ParserState::parse_variable_declaration(bool is_const,
                                                              bool is_ref) {
  Token start = peek();
  TypeInfo type = parse_type_info();
  if (match(TokenType::PUNCTUATION_AMPERSAND)) {
    is_ref = true;
  }

  Token name_tok = consume(TokenType::IDENTIFIER, "Expected variable name");
  std::string v_name = std::get<std::string>(name_tok.value);
  log_debug("Parsing local variable declaration '{}' of type '{}' at line {}",
            v_name, type.to_string(), start.line);

  auto decl =
      std::make_unique<VariableDeclaration>(name_tok, v_name, std::move(type));
  decl->is_const = is_const;
  decl->is_reference_type = is_ref;

  if (match(TokenType::OPERATOR_ASSIGN)) {
    log_trace("Parsing initializer for variable '{}'", v_name);
    decl->initializer = parse_expression();
  }
  consume(TokenType::PUNCTUATION_SEMICOLON,
          "Expected ';' after variable declaration");
  return decl;
}

std::unique_ptr<Node> ParserState::parse_top_level_declaration() {
  if (match(TokenType::KEYWORD_PACKAGE)) {
    if (has_package_statement) {
      throw ParseError("Only one 'package' statement is allowed per file", previous().line, previous().column);
    }
    if (has_parsed_declaration) {
      throw ParseError("'package' statement must be the first statement in the file", previous().line, previous().column);
    }
    has_package_statement = true;
    return parse_package_statement();
  }
  has_parsed_declaration = true;
  if (match(TokenType::KEYWORD_IMPORT))
    return parse_import_statement();
  if (match(TokenType::KEYWORD_ALIAS))
    return parse_alias_statement();

  TokenType modifier = TokenType::KEYWORD_INTERNAL;
  bool is_static = false, is_inline = false, is_native = false,
       is_const = false, is_virtual = false, is_override = false,
       is_weak = false, is_abstract = false;
  while (true) {
    if (match({TokenType::KEYWORD_PUBLIC, TokenType::KEYWORD_PRIVATE,
               TokenType::KEYWORD_PROTECTED, TokenType::KEYWORD_INTERNAL})) {
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

  if (match(TokenType::KEYWORD_CLASS))
    return parse_class_declaration(modifier);
  if (match(TokenType::KEYWORD_ENUM))
    return parse_enum_declaration(modifier);

  return parse_field_or_method(modifier, is_static, is_inline, is_native,
                               is_const, false, false, false, false);
}

std::unique_ptr<Node> ParserState::parse_package_statement() {
  Token pkg = previous();
  std::string name_str = std::get<std::string>(
      consume(TokenType::IDENTIFIER, "Expected package name").value);
  while (match(TokenType::PUNCTUATION_DOT)) {
    name_str += ".";
    name_str += std::get<std::string>(
        consume(TokenType::IDENTIFIER, "Expected sub-package name after '.'")
            .value);
  }
  consume(TokenType::PUNCTUATION_SEMICOLON,
          "Expected ';' after package declaration");
  log_debug("Declared package: {}", name_str);
  return std::make_unique<PackageStatement>(pkg, name_str);
}

std::unique_ptr<Node> ParserState::parse_alias_statement() {
  Token alias = previous();
  Token name = consume(TokenType::IDENTIFIER, "Expected alias name");
  std::string a_name = std::get<std::string>(name.value);

  std::vector<std::string> tparams;
  if (match(TokenType::OPERATOR_LESS_THAN)) {
    do {
      tparams.push_back(std::get<std::string>(
          consume(TokenType::IDENTIFIER, "Expected template parameter name")
              .value));
    } while (match(TokenType::PUNCTUATION_COMMA));
    consume(TokenType::OPERATOR_GREATER_THAN,
            "Expected '>' after template parameters");
  }

  consume(TokenType::OPERATOR_ASSIGN, "Expected '=' in alias declaration");
  TypeInfo type = parse_type_info();
  consume(TokenType::PUNCTUATION_SEMICOLON,
          "Expected ';' after alias declaration");

  log_debug("Declared alias '{}' = '{}' (generics count: {})", a_name,
            type.to_string(), tparams.size());
  auto decl = std::make_unique<AliasStatement>(alias, a_name, std::move(type));
  decl->template_parameters = tparams;
  return decl;
}

std::unique_ptr<Node> ParserState::parse_import_statement() {
  Token import_tok = previous();
  std::string full_path = std::get<std::string>(
      consume(TokenType::IDENTIFIER, "Expected package or type name after 'import'").value);
  bool is_wildcard = false;
  while (match(TokenType::PUNCTUATION_DOT) || match(TokenType::PUNCTUATION_DOUBLE_COLON)) {
    if (match(TokenType::OPERATOR_MULTIPLY)) {
      is_wildcard = true;
      break;
    }
    full_path += ".";
    full_path += std::get<std::string>(
        consume(TokenType::IDENTIFIER, "Expected identifier or '*' after '.' or '::'").value);
  }
  consume(TokenType::PUNCTUATION_SEMICOLON, "Expected ';' after import statement");

  if (is_wildcard) {
    log_debug("Declared wildcard import: '{}.*'", full_path);
    return std::make_unique<ImportStatement>(import_tok, full_path, "*");
  }

  size_t last_dot = full_path.rfind('.');
  if (last_dot != std::string::npos) {
    std::string pkg = full_path.substr(0, last_dot);
    std::string sym = full_path.substr(last_dot + 1);
    log_debug("Declared symbol import: '{}' from '{}'", sym, pkg);
    return std::make_unique<ImportStatement>(import_tok, pkg, sym);
  }

  log_debug("Declared import: '{}'", full_path);
  return std::make_unique<ImportStatement>(import_tok, full_path, "");
}

std::unique_ptr<Node> ParserState::parse_enum_declaration(TokenType modifier) {
  Token enum_tok = previous();
  Token name = consume(TokenType::IDENTIFIER, "Expected enum name");
  std::string e_name = std::get<std::string>(name.value);
  log_trace("Parsing enum '{}' at line {}", e_name, enum_tok.line);

  auto decl = std::make_unique<EnumDeclaration>(enum_tok, e_name);
  decl->access_modifier = modifier;

  consume(TokenType::PUNCTUATION_OPEN_BRACE, "Expected '{' before enum body");
  while (!check(TokenType::PUNCTUATION_CLOSE_BRACE) && !is_at_end()) {
    Token member = consume(TokenType::IDENTIFIER, "Expected enum member");
    std::string mem_name = std::get<std::string>(member.value);
    log_trace("Enum member: {}", mem_name);
    decl->members.push_back(mem_name);
    if (!match(TokenType::PUNCTUATION_COMMA))
      break;
  }
  consume(TokenType::PUNCTUATION_CLOSE_BRACE, "Expected '}' after enum body");
  return decl;
}

std::unique_ptr<Node> ParserState::parse_class_declaration(TokenType modifier) {
  Token class_tok = previous();
  Token name = consume(TokenType::IDENTIFIER, "Expected class name");
  std::string cls_name = std::get<std::string>(name.value);
  log_trace("Parsing class '{}' at line {}", cls_name, class_tok.line);

  auto decl = std::make_unique<ClassDeclaration>(class_tok, cls_name);

  if (match(TokenType::OPERATOR_LESS_THAN)) {
    do {
      std::string t_param = std::get<std::string>(
          consume(TokenType::IDENTIFIER, "Expected template parameter name")
              .value);
      decl->template_parameters.push_back(t_param);
    } while (match(TokenType::PUNCTUATION_COMMA));
    consume(TokenType::OPERATOR_GREATER_THAN,
            "Expected '>' after template parameters");
    log_debug("Class '{}' template parameters count: {}", cls_name,
              decl->template_parameters.size());
  }

  if (match(TokenType::KEYWORD_EXTENDS) || match(TokenType::PUNCTUATION_COLON)) {
    std::string base_name = std::get<std::string>(
        consume(TokenType::IDENTIFIER,
                "Expected base class name after 'extends' or ':'")
            .value);
    while (match(TokenType::PUNCTUATION_DOT) || match(TokenType::PUNCTUATION_DOUBLE_COLON)) {
      base_name += "." + std::get<std::string>(
          consume(TokenType::IDENTIFIER, "Expected identifier after '.' or '::' in base class name").value);
    }
    decl->base_class_name = base_name;
    log_debug("Class '{}' extends '{}'", cls_name, decl->base_class_name);
  }
  decl->access_modifier = modifier;

  consume(TokenType::PUNCTUATION_OPEN_BRACE, "Expected '{' before class body");
  while (!check(TokenType::PUNCTUATION_CLOSE_BRACE) && !is_at_end()) {
    TokenType field_mod = TokenType::KEYWORD_PRIVATE;
    bool is_static = false, is_inline = false, is_native = false,
         is_const = false, is_virtual = false, is_override = false,
         is_weak = false, is_abstract = false;
    while (true) {
      if (match({TokenType::KEYWORD_PUBLIC, TokenType::KEYWORD_PRIVATE,
                 TokenType::KEYWORD_PROTECTED, TokenType::KEYWORD_INTERNAL})) {
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

    if (match(TokenType::KEYWORD_CLASS)) {
      decl->children.push_back(parse_class_declaration(field_mod));
      continue;
    }
    if (match(TokenType::KEYWORD_ENUM)) {
      decl->children.push_back(parse_enum_declaration(field_mod));
      continue;
    }

    // Constructor check
    if (check(TokenType::IDENTIFIER) &&
        std::get<std::string>(peek().value) == decl->class_name &&
        tokens[current + 1]->type == TokenType::PUNCTUATION_OPEN_PAREN) {
      Token ctor_name = advance();
      log_debug("Parsing constructor for '{}' at line {}", decl->class_name,
                ctor_name.line);
      auto ctor =
          std::make_unique<ConstructorDeclaration>(ctor_name, decl->class_name);
      ctor->access_modifier = field_mod;

      consume(TokenType::PUNCTUATION_OPEN_PAREN,
              "Expected '(' after constructor name");
      while (!check(TokenType::PUNCTUATION_CLOSE_PAREN) && !is_at_end()) {
        TypeInfo p_type = parse_type_info();
        bool p_ref = match(TokenType::PUNCTUATION_AMPERSAND);
        Token p_name =
            consume(TokenType::IDENTIFIER, "Expected parameter name");
        auto param = std::make_unique<VariableDeclaration>(
            p_name, std::get<std::string>(p_name.value), std::move(p_type));
        param->is_reference_type = p_ref;
        ctor->parameters.push_back(std::move(param));
        if (!match(TokenType::PUNCTUATION_COMMA))
          break;
      }
      consume(TokenType::PUNCTUATION_CLOSE_PAREN,
              "Expected ')' after constructor parameters");

      std::vector<std::unique_ptr<Node>> injected_initializers;
      if (match(TokenType::PUNCTUATION_COLON)) {
        log_trace("Parsing constructor member initializer list for '{}'",
                  decl->class_name);
        do {
          if (match(TokenType::KEYWORD_SUPER)) {
            Token super_tok = previous();
            consume(TokenType::PUNCTUATION_OPEN_PAREN,
                    "Expected '(' after super");
            std::vector<std::unique_ptr<Node>> args;
            while (!check(TokenType::PUNCTUATION_CLOSE_PAREN) && !is_at_end()) {
              args.push_back(parse_expression());
              if (!match(TokenType::PUNCTUATION_COMMA))
                break;
            }
            consume(TokenType::PUNCTUATION_CLOSE_PAREN,
                    "Expected ')' after super arguments");
            auto super_id =
                std::make_unique<IdentifierNode>(super_tok, "super");
            auto super_call = std::make_unique<MethodCallExpression>(
                super_tok, std::move(super_id));
            super_call->arguments = std::move(args);
            auto expr_stmt = std::make_unique<ExpressionStatement>(
                super_tok, std::move(super_call));
            injected_initializers.push_back(std::move(expr_stmt));
          } else {
            Token field_name =
                consume(TokenType::IDENTIFIER,
                        "Expected 'super' or field name in initializer list");
            consume(TokenType::PUNCTUATION_OPEN_PAREN,
                    "Expected '(' after field name");
            auto val = parse_expression();
            consume(TokenType::PUNCTUATION_CLOSE_PAREN,
                    "Expected ')' after field value");

            auto this_id = std::make_unique<IdentifierNode>(field_name, "this");
            auto field_acc = std::make_unique<MemberAccessExpression>(
                field_name, std::move(this_id),
                std::get<std::string>(field_name.value));
            auto assign = std::make_unique<AssignmentExpression>(
                field_name, std::move(field_acc), TokenType::OPERATOR_ASSIGN,
                std::move(val));
            auto expr_stmt2 = std::make_unique<ExpressionStatement>(
                field_name, std::move(assign));
            injected_initializers.push_back(std::move(expr_stmt2));
          }
        } while (match(TokenType::PUNCTUATION_COMMA));
      }

      auto body = parse_block();
      if (auto* b = dynamic_cast<BlockStatement*>(body.get())) {
          b->block_kind = BlockKind::FUNCTION_BODY;
      }
      for (auto it = injected_initializers.rbegin();
           it != injected_initializers.rend(); ++it) {
        (*it)->parent = body.get();
        body->children.insert(body->children.begin(), std::move(*it));
      }
      ctor->children.push_back(std::move(body));
      decl->children.push_back(std::move(ctor));
    } else {
      decl->children.push_back(parse_field_or_method(
          field_mod, is_static, is_inline, is_native, is_const, is_virtual,
          is_override, is_weak, is_abstract));
    }
  }
  consume(TokenType::PUNCTUATION_CLOSE_BRACE, "Expected '}' after class body");

  for (auto &child : decl->children) {
    child->parent = decl.get();
  }

  log_debug("Completed parsing class '{}' with {} members", cls_name,
            decl->children.size());
  return decl;
}

std::unique_ptr<Node> ParserState::parse_field_or_method(
    TokenType modifier, bool is_static, bool is_inline, bool is_native,
    bool is_const, bool is_virtual, bool is_override, bool is_weak,
    bool is_abstract) {
  TypeInfo type = parse_type_info();
  bool is_ref = match(TokenType::PUNCTUATION_AMPERSAND);

  Token name;
  std::string name_str;
  if (match(TokenType::KEYWORD_OPERATOR)) {
    name = previous();
    Token op_token = peek();
    advance();
    name_str = "operator";
    if (op_token.type == TokenType::OPERATOR_PLUS)
      name_str += "+";
    else if (op_token.type == TokenType::OPERATOR_MINUS)
      name_str += "-";
    else if (op_token.type == TokenType::OPERATOR_MULTIPLY)
      name_str += "*";
    else if (op_token.type == TokenType::OPERATOR_DIVIDE)
      name_str += "/";
    else if (op_token.type == TokenType::OPERATOR_ASSIGN)
      name_str += "=";
    else
      throw ParseError("Invalid operator for overloading");
  } else {
    name = consume(TokenType::IDENTIFIER, "Expected field or method name");
    name_str = std::get<std::string>(name.value);
  }

  std::vector<std::string> tparams;
  bool is_specialization = false;
  std::vector<TypeInfo> spec_args;

  if (match(TokenType::OPERATOR_LESS_THAN)) {
    log_trace("Parsing template signature for method/field '{}'", name_str);
    do {
      TypeInfo t = parse_type_info();
      spec_args.push_back(t);
      if (t.name == "void" || t.name == "bool" ||
          t.name.find("int") != std::string::npos ||
          t.name.find("float") != std::string::npos || t.name == "char" ||
          t.array_depth > 0 || !t.type_args.empty()) {
        is_specialization = true;
      }
      tparams.push_back(t.name);
    } while (match(TokenType::PUNCTUATION_COMMA));
    consume(TokenType::OPERATOR_GREATER_THAN,
            "Expected '>' after template parameters");
  }

  if (is_specialization) {
    name_str += "<";
    for (size_t i = 0; i < spec_args.size(); ++i) {
      name_str += spec_args[i].to_string();
      if (i < spec_args.size() - 1)
        name_str += ",";
    }
    name_str += ">";
    tparams.clear();
    log_debug("Detected explicit template specialization method signature: {}",
              name_str);
  }

  if (match(TokenType::PUNCTUATION_OPEN_PAREN)) {
    log_debug("Parsing method declaration '{}' returning '{}' at line {}",
              name_str, type.to_string(), name.line);
    auto method =
        std::make_unique<MethodDeclaration>(name, name_str, std::move(type));
    method->template_parameters = tparams;
    method->access_modifier = modifier;
    method->is_static = is_static;
    method->is_inline = is_inline;
    method->is_native = is_native;
    method->is_virtual = is_virtual;
    method->is_abstract = is_abstract;
    method->is_override = is_override;

    while (!check(TokenType::PUNCTUATION_CLOSE_PAREN) && !is_at_end()) {
      TypeInfo p_type = parse_type_info();
      bool p_ref = match(TokenType::PUNCTUATION_AMPERSAND);
      Token p_name = consume(TokenType::IDENTIFIER, "Expected parameter name");
      auto param = std::make_unique<VariableDeclaration>(
          p_name, std::get<std::string>(p_name.value), std::move(p_type));
      param->is_reference_type = p_ref;
      method->parameters.push_back(std::move(param));
      if (!match(TokenType::PUNCTUATION_COMMA))
        break;
    }
    consume(TokenType::PUNCTUATION_CLOSE_PAREN,
            "Expected ')' after method parameters");

    if (is_native || is_abstract) {
      consume(TokenType::PUNCTUATION_SEMICOLON,
              "Expected ';' after native or abstract method declaration");
      log_trace("Finished native/abstract method header for '{}'", name_str);
    } else {
      auto block = parse_block();
      if (auto* b = dynamic_cast<BlockStatement*>(block.get())) {
          b->block_kind = BlockKind::FUNCTION_BODY;
      }
      method->children.push_back(std::move(block));
    }
    return method;
  } else {
    log_debug("Parsing field declaration '{}' of type '{}' at line {}",
              name_str, type.to_string(), name.line);
    auto field =
        std::make_unique<FieldDeclaration>(name, name_str, std::move(type));
    field->access_modifier = modifier;
    field->is_static = is_static;
    field->is_const = is_const;
    field->is_reference_type = is_ref;
    field->is_weak = is_weak;
    if (match(TokenType::OPERATOR_ASSIGN)) {
      log_trace("Parsing field initializer for '{}'", name_str);
      field->initializer = parse_expression();
    }
    consume(TokenType::PUNCTUATION_SEMICOLON,
            "Expected ';' after field declaration");
    return field;
  }
}

void Parser::execute() {
  log_debug("Starting Syntax Analysis (Parsing)...");

  for (const auto &[source, token_lists] : context.tokens) {
    if (token_lists.empty())
      continue;

    const TokenList &tokens = token_lists.front();
    if (tokens.empty())
      continue;

    ParserState state(tokens, this, &source);

    std::string source_name;
    if (std::holds_alternative<std::filesystem::path>(source)) {
      source_name = std::get<std::filesystem::path>(source).string();
    } else {
      source_name = std::get<std::string>(source);
    }

    log_debug("Parsing source file: {}", source_name);

    try {
      while (!state.is_at_end()) {
        context.nodes[source].push_back(state.parse_top_level_declaration());
      }
    } catch (const ParseError &e) {
      log_error("Syntax Error in {}: {}", source_name, e.what());
      if (context.diagnostic) {
        Report report;
        report.severity = ReportSeverity::ERROR;
        report.code = "E_PARSE";
        report.message = e.what();
        report.source_path = source_name;
        report.line = e.line;
        report.column = e.column;
        context.diagnostic->record_report(report);
      }
    } catch (const std::exception &e) {
      log_error("Syntax Error in {}: {}", source_name, e.what());
      if (context.diagnostic) {
        Report report;
        report.severity = ReportSeverity::ERROR;
        report.code = "E_PARSE";
        report.message = e.what();
        report.source_path = source_name;
        report.line = 0;
        report.column = 0;
        context.diagnostic->record_report(report);
      }
    }

    log_debug("Completed parsing {}: generated {} top-level AST nodes",
              source_name, context.nodes[source].size());
  }

  log_debug("Syntax Analysis completed.");
}


std::unique_ptr<Node> ParserState::parse_try_statement() {
  Token try_tok = previous();
  log_trace("Parsing 'try' statement at line {}", try_tok.line);

  std::unique_ptr<Node> try_block = nullptr;
  if (check(TokenType::PUNCTUATION_OPEN_BRACE)) {
      try_block = parse_block();
      if (auto* b = dynamic_cast<BlockStatement*>(try_block.get())) {
          b->block_kind = BlockKind::TRY_BODY;
      }
  } else {
      throw ParseError("Expected '{' after 'try'");
  }

  std::vector<std::unique_ptr<Node>> catches;
  while (match(TokenType::KEYWORD_CATCH)) {
      Token catch_tok = previous();
      consume(TokenType::PUNCTUATION_OPEN_PAREN, "Expected '(' after 'catch'");
      
      TypeInfo exc_type = parse_type_info();
      consume(TokenType::IDENTIFIER, "Expected exception variable name");
      std::string var_name = std::get<std::string>(previous().value);
      
      consume(TokenType::PUNCTUATION_CLOSE_PAREN, "Expected ')' after catch variable");
      
      std::unique_ptr<Node> catch_block = nullptr;
      if (check(TokenType::PUNCTUATION_OPEN_BRACE)) {
          catch_block = parse_block();
      } else {
          throw ParseError("Expected '{' after catch clause");
      }
      
      catches.push_back(std::make_unique<CatchClause>(catch_tok, var_name, exc_type, std::move(catch_block)));
  }

  std::unique_ptr<Node> finally_block = nullptr;
  if (match(TokenType::KEYWORD_FINALLY)) {
      if (check(TokenType::PUNCTUATION_OPEN_BRACE)) {
          finally_block = parse_block();
      } else {
          throw ParseError("Expected '{' after 'finally'");
      }
  }

  if (catches.empty() && !finally_block) {
      throw ParseError("'try' statement must have at least one 'catch' or 'finally' clause");
  }

  return std::make_unique<TryStatement>(try_tok, std::move(try_block), std::move(catches), std::move(finally_block));
}

std::unique_ptr<Node> ParserState::parse_throw_statement() {
    Token throw_tok = previous();
    log_trace("Parsing 'throw' statement at line {}", throw_tok.line);
    
    std::unique_ptr<Node> expr = parse_expression();
    consume(TokenType::PUNCTUATION_SEMICOLON, "Expected ';' after throw expression");
    
    return std::make_unique<ThrowStatement>(throw_tok, std::move(expr));
}
} // namespace solix
