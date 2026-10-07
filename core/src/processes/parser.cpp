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
  uint32_t end_line = 0;
  uint32_t end_column = 0;
  ParseError(const std::string &msg, uint32_t line = 0, uint32_t column = 0,
             uint32_t end_line = 0, uint32_t end_column = 0)
      : std::runtime_error(msg), line(line), column(column),
        end_line(end_line ? end_line : line),
        end_column(end_column ? end_column : column) {}
};

static std::string operator_token_to_string(TokenType type) {
  switch (type) {
    case TokenType::OPERATOR_PLUS: return "+";
    case TokenType::OPERATOR_MINUS: return "-";
    case TokenType::OPERATOR_MULTIPLY: return "*";
    case TokenType::OPERATOR_DIVIDE: return "/";
    case TokenType::OPERATOR_ASSIGN: return "=";
    case TokenType::OPERATOR_LOGICAL_AND: return "&&";
    case TokenType::OPERATOR_LOGICAL_OR: return "||";
    case TokenType::OPERATOR_EQUAL: return "==";
    case TokenType::OPERATOR_NOT_EQUAL: return "!=";
    case TokenType::OPERATOR_LESS_THAN: return "<";
    case TokenType::OPERATOR_GREATER_THAN: return ">";
    default: return "?";
  }
}

static std::string token_type_to_string(TokenType type) {
  switch (type) {
    case TokenType::PUNCTUATION_SEMICOLON: return "';'";
    case TokenType::PUNCTUATION_COMMA: return "','";
    case TokenType::PUNCTUATION_OPEN_PAREN: return "'('";
    case TokenType::PUNCTUATION_CLOSE_PAREN: return "')'";
    case TokenType::PUNCTUATION_OPEN_BRACE: return "'{'";
    case TokenType::PUNCTUATION_CLOSE_BRACE: return "'}'";
    case TokenType::PUNCTUATION_OPEN_BRACKET: return "'['";
    case TokenType::PUNCTUATION_CLOSE_BRACKET: return "']'";
    case TokenType::EOF_TOKEN: return "<EOF>";
    default: return std::to_string((int)type);
  }
}

class ParserState {
  std::vector<const Token *> tokens;
  size_t current = 0;
  Parser *process;
  const Source *current_source;
  bool has_parsed_declaration = false;
  bool has_package_statement = false;
  bool has_parsed_type_declaration = false;

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
    throw ParseError(message, peek().line, peek().column, peek().end_line, peek().end_column);
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
  std::unique_ptr<Node> parse_assert_statement();
  std::unique_ptr<Node> parse_exit_statement();
  std::unique_ptr<Node> parse_expression_statement();
  std::unique_ptr<Node> parse_variable_declaration(bool is_const, bool is_ref);

  // Top Level parsing
  std::unique_ptr<Node> parse_top_level_declaration();
  std::unique_ptr<Node> parse_class_declaration(TokenType modifier, bool is_abstract = false);
  std::unique_ptr<Node> parse_interface_declaration(TokenType modifier);
  std::unique_ptr<Node> parse_enum_declaration(TokenType modifier);
  std::unique_ptr<Node> parse_package_statement();
  std::unique_ptr<Node> parse_alias_statement();
  std::unique_ptr<Node> parse_import_statement();
  std::unique_ptr<Node> parse_field_or_method(TokenType modifier,
                                              bool is_static, bool is_inline,
                                              bool is_native, bool is_const,
                                              bool is_virtual, bool is_override,
                                              bool is_weak, bool is_abstract,
                                              bool is_inside_class = false);

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
    throw ParseError("Expected a type name", peek().line, peek().column);
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

  while (check(TokenType::PUNCTUATION_OPEN_PAREN) && current + 2 < tokens.size() &&
         tokens[current + 1]->type == TokenType::OPERATOR_MULTIPLY &&
         tokens[current + 2]->type == TokenType::PUNCTUATION_CLOSE_PAREN) {
    advance(); // '('
    advance(); // '*'
    advance(); // ')'
    TypeInfo fp;
    fp.is_function_pointer = true;
    fp.return_type = std::make_shared<TypeInfo>(type);
    consume(TokenType::PUNCTUATION_OPEN_PAREN, "Expected '(' for function pointer parameters");
    if (!check(TokenType::PUNCTUATION_CLOSE_PAREN)) {
      do {
        fp.param_types.push_back(parse_type_info());
      } while (match(TokenType::PUNCTUATION_COMMA));
    }
    consume(TokenType::PUNCTUATION_CLOSE_PAREN, "Expected ')' after function pointer parameters");

    while (match(TokenType::PUNCTUATION_ARRAY_BRACKETS)) {
      fp.array_depth++;
    }

    while (check(TokenType::PUNCTUATION_OPEN_BRACKET) &&
           current + 1 < tokens.size() &&
           tokens[current + 1]->type == TokenType::PUNCTUATION_CLOSE_BRACKET) {
      advance();
      advance();
      fp.array_depth++;
    }

    fp.name = fp.to_string();
    type = std::move(fp);
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
    throw ParseError("Invalid assignment target", equals.line, equals.column);
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
    if (op.type == TokenType::OPERATOR_INCREMENT || op.type == TokenType::OPERATOR_DECREMENT) {
      if (right && (right->node_type != NodeType::IDENTIFIER &&
                    right->node_type != NodeType::MEMBER_ACCESS &&
                    right->node_type != NodeType::ARRAY_ACCESS)) {
        throw ParseError("Invalid operand for increment operator: expected lvalue", op.line, op.column);
      }
    }
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
        } else if (tokens[temp]->type == TokenType::PUNCTUATION_OPEN_BRACE ||
                   tokens[temp]->type == TokenType::OPERATOR_LOGICAL_OR ||
                   tokens[temp]->type == TokenType::OPERATOR_LOGICAL_AND) {
          valid_template = false;
          break;
        }
        temp++;
      }
      if (valid_template && temp < tokens.size() &&
          tokens[temp]->type == TokenType::PUNCTUATION_OPEN_PAREN) {
        log_trace(
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
          std::make_unique<MemberAccessExpression>(name, std::move(expr), prop);
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
    if (expr && (expr->node_type != NodeType::IDENTIFIER &&
                 expr->node_type != NodeType::MEMBER_ACCESS &&
                 expr->node_type != NodeType::ARRAY_ACCESS)) {
      throw ParseError("Invalid operand for increment operator: expected lvalue", op.line, op.column);
    }
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

  if (match(TokenType::KEYWORD_SUPER)) {
    return std::make_unique<IdentifierNode>(previous(), "super");
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
      if (type.array_depth > 0) {
        throw ParseError("Array allocation requires size expression", new_tok.line, new_tok.column);
      }
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

  auto is_lambda_ahead = [&]() -> bool {
    if (current >= tokens.size()) return false;
    if (tokens[current]->type == TokenType::PUNCTUATION_ARRAY_BRACKETS) {
      if (current + 1 < tokens.size() && tokens[current + 1]->type == TokenType::PUNCTUATION_OPEN_PAREN) {
        size_t p = current + 2;
        int paren_depth = 1;
        while (p < tokens.size() && paren_depth > 0) {
          if (tokens[p]->type == TokenType::PUNCTUATION_OPEN_PAREN) paren_depth++;
          else if (tokens[p]->type == TokenType::PUNCTUATION_CLOSE_PAREN) paren_depth--;
          p++;
        }
        if (p < tokens.size()) {
          if (tokens[p]->type == TokenType::OPERATOR_FAT_ARROW ||
              tokens[p]->type == TokenType::PUNCTUATION_COLON ||
              tokens[p]->type == TokenType::PUNCTUATION_OPEN_BRACE) {
            return true;
          }
          if (tokens[p]->type == TokenType::OPERATOR_ASSIGN && p + 1 < tokens.size() && tokens[p + 1]->type == TokenType::OPERATOR_GREATER_THAN) {
            return true;
          }
        }
      }
      return false;
    }
    if (tokens[current]->type == TokenType::PUNCTUATION_OPEN_BRACKET) {
      size_t p = current + 1;
      while (p < tokens.size() && tokens[p]->type != TokenType::PUNCTUATION_CLOSE_BRACKET) {
        if (tokens[p]->type != TokenType::IDENTIFIER &&
            tokens[p]->type != TokenType::PUNCTUATION_COMMA) {
          return false;
        }
        p++;
      }
      if (p >= tokens.size() || tokens[p]->type != TokenType::PUNCTUATION_CLOSE_BRACKET) return false;
      p++; // skip ']'
      if (p >= tokens.size() || tokens[p]->type != TokenType::PUNCTUATION_OPEN_PAREN) return false;
      p++; // skip '('
      int paren_depth = 1;
      while (p < tokens.size() && paren_depth > 0) {
        if (tokens[p]->type == TokenType::PUNCTUATION_OPEN_PAREN) paren_depth++;
        else if (tokens[p]->type == TokenType::PUNCTUATION_CLOSE_PAREN) paren_depth--;
        p++;
      }
      if (p < tokens.size()) {
        if (tokens[p]->type == TokenType::OPERATOR_FAT_ARROW ||
            tokens[p]->type == TokenType::PUNCTUATION_COLON ||
            tokens[p]->type == TokenType::PUNCTUATION_OPEN_BRACE) {
          return true;
        }
        if (tokens[p]->type == TokenType::OPERATOR_ASSIGN && p + 1 < tokens.size() && tokens[p + 1]->type == TokenType::OPERATOR_GREATER_THAN) {
          return true;
        }
      }
      return false;
    }
    return false;
  };

  if (is_lambda_ahead()) {
    Token start_tok = peek();
    auto lambda = std::make_unique<LambdaExpression>(start_tok);

    if (match(TokenType::PUNCTUATION_ARRAY_BRACKETS)) {
      // Stateless lambda []
    } else if (match(TokenType::PUNCTUATION_OPEN_BRACKET)) {
      if (!check(TokenType::PUNCTUATION_CLOSE_BRACKET)) {
        do {
          if (match(TokenType::IDENTIFIER)) {
            lambda->capture_names.push_back(std::get<std::string>(previous().value));
          } else {
            throw ParseError("Expected variable identifier or 'this' in capture list", peek().line, peek().column);
          }
        } while (match(TokenType::PUNCTUATION_COMMA));
      }
      consume(TokenType::PUNCTUATION_CLOSE_BRACKET, "Expected ']' after capture list");
    }

    consume(TokenType::PUNCTUATION_OPEN_PAREN, "Expected '(' after lambda capture list");
    if (!check(TokenType::PUNCTUATION_CLOSE_PAREN)) {
      do {
        TypeInfo param_type = parse_type_info();
        Token name_tok = consume(TokenType::IDENTIFIER, "Expected parameter name in lambda");
        auto param_decl = std::make_unique<VariableDeclaration>(name_tok, std::get<std::string>(name_tok.value), param_type);
        param_decl->parent = lambda.get();
        lambda->parameters.push_back(std::move(param_decl));
      } while (match(TokenType::PUNCTUATION_COMMA));
    }
    consume(TokenType::PUNCTUATION_CLOSE_PAREN, "Expected ')' after lambda parameters");

    if (match(TokenType::PUNCTUATION_COLON)) {
      lambda->explicit_return_type = std::make_shared<TypeInfo>(parse_type_info());
    }

    if (match(TokenType::OPERATOR_FAT_ARROW)) {
      // ok
    } else if (match(TokenType::OPERATOR_ASSIGN) && match(TokenType::OPERATOR_GREATER_THAN)) {
      // '=' followed by '>'
    } else if (check(TokenType::PUNCTUATION_OPEN_BRACE)) {
      // ok, block body directly without fat arrow
    } else {
      throw ParseError("Expected '=>' in lambda expression", peek().line, peek().column);
    }

    if (check(TokenType::PUNCTUATION_OPEN_BRACE)) {
      lambda->body = parse_block();
    } else {
      lambda->body = parse_expression();
    }
    if (lambda->body) {
      lambda->body->parent = lambda.get();
    }

    return lambda;
  }

  if (match({TokenType::PUNCTUATION_OPEN_BRACE, TokenType::PUNCTUATION_OPEN_BRACKET})) {
    Token open_tok = previous();
    TokenType closing_tok = (open_tok.type == TokenType::PUNCTUATION_OPEN_BRACE)
                                ? TokenType::PUNCTUATION_CLOSE_BRACE
                                : TokenType::PUNCTUATION_CLOSE_BRACKET;
    log_trace("Parsing array literal at line {}", open_tok.line);
    auto arr_lit = std::make_unique<ArrayLiteralExpression>(open_tok);
    if (!check(closing_tok)) {
      do {
        arr_lit->elements.push_back(parse_expression());
      } while (match(TokenType::PUNCTUATION_COMMA));
    }
    consume(closing_tok,
            open_tok.type == TokenType::PUNCTUATION_OPEN_BRACE
                ? "Expected '}' at end of array literal"
                : "Expected ']' at end of array literal");
    return arr_lit;
  }

  if (match(TokenType::KEYWORD_SIZEOF)) {
    Token sizeof_tok = previous();
    consume(TokenType::PUNCTUATION_OPEN_PAREN, "Expected '(' after 'sizeof'");

    if (peek().type >= TokenType::PRIMITIVE_VOID && peek().type <= TokenType::PRIMITIVE_CHAR) {
      TypeInfo type = parse_type_info();
      consume(TokenType::PUNCTUATION_CLOSE_PAREN, "Expected ')' after sizeof argument");
      return std::make_unique<SizeOfExpression>(sizeof_tok, std::make_shared<TypeInfo>(type), nullptr);
    }

    if (peek().type == TokenType::IDENTIFIER &&
        (tokens[current + 1]->type == TokenType::PUNCTUATION_ARRAY_BRACKETS ||
         tokens[current + 1]->type == TokenType::PUNCTUATION_OPEN_BRACKET)) {
      size_t restore = current;
      try {
        TypeInfo type = parse_type_info();
        if (match(TokenType::PUNCTUATION_CLOSE_PAREN)) {
          return std::make_unique<SizeOfExpression>(sizeof_tok, std::make_shared<TypeInfo>(type), nullptr);
        }
      } catch (...) {
        current = restore;
      }
      current = restore;
    }

    std::unique_ptr<Node> expr = parse_expression();
    consume(TokenType::PUNCTUATION_CLOSE_PAREN, "Expected ')' after sizeof argument");
    return std::make_unique<SizeOfExpression>(sizeof_tok, nullptr, std::move(expr));
  }

  if (match(TokenType::KEYWORD_DEFAULT)) {
    Token default_tok = previous();
    consume(TokenType::PUNCTUATION_OPEN_PAREN, "Expected '(' after 'default'");
    TypeInfo type = parse_type_info();
    consume(TokenType::PUNCTUATION_CLOSE_PAREN, "Expected ')' after default type argument");
    return std::make_unique<DefaultExpression>(default_tok, std::make_shared<TypeInfo>(type));
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
    if (check(TokenType::IDENTIFIER) || check(TokenType::NUMBER) || check(TokenType::STRING)) {
      throw ParseError("Expected type name in cast expression", paren.line, paren.column);
    }
    return expr;
  }

  throw ParseError("Expected expression, got " +
                   token_type_to_string(peek().type), peek().line, peek().column);
}

std::unique_ptr<Node> ParserState::parse_block() {
  Token brace =
      consume(TokenType::PUNCTUATION_OPEN_BRACE, "Expected '{' before block");
  log_trace("Entering block statement at line {}", brace.line);
  auto block = std::make_unique<BlockStatement>(brace);
  while (!check(TokenType::PUNCTUATION_CLOSE_BRACE) && !is_at_end()) {
    auto stmt = parse_statement();
    if (stmt) {
      if (stmt->node_type == NodeType::BLOCK &&
          static_cast<BlockStatement*>(stmt.get())->block_kind == BlockKind::TRANSPARENT) {
        for (auto &child : stmt->children) {
          child->parent = block.get();
          block->children.push_back(std::move(child));
        }
      } else {
        stmt->parent = block.get();
        block->children.push_back(std::move(stmt));
      }
    }
  }
  Token close_brace = consume(TokenType::PUNCTUATION_CLOSE_BRACE, "Expected '}' after block");
  block->end_line = close_brace.line;
  block->end_column = close_brace.column;
  log_trace("Exiting block statement at line {} with {} statements", brace.line,
            block->children.size());
  return block;
}

std::unique_ptr<Node> ParserState::parse_statement() {
  if (match(TokenType::KEYWORD_IMPORT)) {
    throw ParseError("Import statements must appear before class declarations", previous().line, previous().column);
  }
  if (match(TokenType::KEYWORD_PACKAGE)) {
    throw ParseError("'package' statement must be the first statement in the file", previous().line, previous().column);
  }
  if (match(TokenType::KEYWORD_CLASS)) {
    throw ParseError("Classes cannot be declared inside a function or method body", previous().line, previous().column);
  }
  if (match(TokenType::KEYWORD_ENUM)) {
    throw ParseError("Enums cannot be declared inside a function or method body", previous().line, previous().column);
  }
  if (match(TokenType::KEYWORD_INTERFACE)) {
    throw ParseError("Interfaces cannot be declared inside a function or method body", previous().line, previous().column);
  }
  if (match({TokenType::KEYWORD_PUBLIC, TokenType::KEYWORD_PRIVATE,
             TokenType::KEYWORD_PROTECTED, TokenType::KEYWORD_INTERNAL})) {
    throw ParseError("Access modifiers ('public', 'private', 'protected') are not allowed on local variables", previous().line, previous().column);
  }
  if (match(TokenType::KEYWORD_OPERATOR)) {
    throw ParseError("Operator overloads can only be declared inside a class body", previous().line, previous().column);
  }

  if (check(TokenType::IDENTIFIER) && current + 1 < tokens.size() &&
      tokens[current + 1]->type == TokenType::PUNCTUATION_OPEN_PAREN) {
    size_t scan = current + 2;
    int depth = 1;
    while (scan < tokens.size() && depth > 0) {
      if (tokens[scan]->type == TokenType::PUNCTUATION_OPEN_PAREN) depth++;
      else if (tokens[scan]->type == TokenType::PUNCTUATION_CLOSE_PAREN) depth--;
      scan++;
    }
    if (depth == 0 && scan < tokens.size() && tokens[scan]->type == TokenType::PUNCTUATION_OPEN_BRACE) {
      throw ParseError("Constructors can only be declared inside a class body", peek().line, peek().column);
    }
  }

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
  if (match(TokenType::KEYWORD_ASSERT))
    return parse_assert_statement();
  if (match(TokenType::KEYWORD_EXIT))
    return parse_exit_statement();

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
    bool looks_like_var_decl = false;
    try {
      TypeInfo type = parse_type_info();
      bool is_ref = match(TokenType::PUNCTUATION_AMPERSAND);
      if (check(TokenType::IDENTIFIER)) {
        looks_like_var_decl = true;
      }
    } catch (...) {
      looks_like_var_decl = false;
    }
    current = restore;
    if (looks_like_var_decl) {
      return parse_variable_declaration(false, false);
    }
  }

  return parse_expression_statement();
}

static bool is_solitary_var_decl(const Node *node) {
  if (!node) return false;
  if (node->node_type == NodeType::VAR_DECL) return true;
  if (node->node_type == NodeType::BLOCK &&
      static_cast<const BlockStatement *>(node)->block_kind == BlockKind::TRANSPARENT) {
    return true;
  }
  return false;
}

std::unique_ptr<Node> ParserState::parse_if_statement() {
  Token if_tok = previous();
  log_trace("Parsing 'if' statement at line {}", if_tok.line);
  consume(TokenType::PUNCTUATION_OPEN_PAREN, "Expected '(' after 'if'");
  std::unique_ptr<Node> condition = parse_expression();
  consume(TokenType::PUNCTUATION_CLOSE_PAREN,
          "Expected ')' after if condition");

  std::unique_ptr<Node> then_branch = parse_statement();
  if (is_solitary_var_decl(then_branch.get())) {
    throw ParseError("Variable declarations are not allowed as immediate solitary branch statements without a block",
                     then_branch->line, then_branch->column);
  }
  std::unique_ptr<Node> else_branch = nullptr;
  if (match(TokenType::KEYWORD_ELSE)) {
    log_trace("Parsing 'else' branch for 'if' at line {}", if_tok.line);
    else_branch = parse_statement();
    if (is_solitary_var_decl(else_branch.get())) {
      throw ParseError("Variable declarations are not allowed as immediate solitary branch statements without a block",
                       else_branch->line, else_branch->column);
    }
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
  if (is_solitary_var_decl(body.get())) {
    throw ParseError("Variable declarations are not allowed as immediate solitary loop statements without a block",
                     body->line, body->column);
  }

  auto stmt = std::make_unique<WhileStatement>(while_tok);
  stmt->condition = std::move(condition);
  stmt->body = std::move(body);
  return stmt;
}

std::unique_ptr<Node> ParserState::parse_do_while_statement() {
  Token do_tok = previous();
  log_trace("Parsing 'do-while' loop at line {}", do_tok.line);
  std::unique_ptr<Node> body = parse_statement();
  if (is_solitary_var_decl(body.get())) {
    throw ParseError("Variable declarations are not allowed as immediate solitary loop statements without a block",
                     body->line, body->column);
  }
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
  if (is_solitary_var_decl(stmt->body.get())) {
    throw ParseError("Variable declarations are not allowed as immediate solitary loop statements without a block",
                     stmt->body->line, stmt->body->column);
  }
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
      if (case_stmt->case_value->node_type == NodeType::IDENTIFIER) {
        throw ParseError("Case label must be a constant literal", case_stmt->case_value->line, case_stmt->case_value->column);
      }
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

  std::vector<std::unique_ptr<VariableDeclaration>> decls;

  do {
    Token name_tok = consume(TokenType::IDENTIFIER, "Expected variable name");
    if (check(TokenType::PUNCTUATION_OPEN_PAREN)) {
      throw ParseError("Methods cannot be declared inside another method", name_tok.line, name_tok.column);
    }
    std::string v_name = std::get<std::string>(name_tok.value);
    log_trace("Parsing local variable declaration '{}' of type '{}' at line {}",
              v_name, type.to_string(), start.line);

    auto decl =
        std::make_unique<VariableDeclaration>(name_tok, v_name, type);
    decl->is_const = is_const;
    decl->is_reference_type = is_ref;

    if (match(TokenType::OPERATOR_ASSIGN)) {
      log_trace("Parsing initializer for variable '{}'", v_name);
      decl->initializer = parse_expression();
    }
    decls.push_back(std::move(decl));
  } while (match(TokenType::PUNCTUATION_COMMA));

  Token semi = consume(TokenType::PUNCTUATION_SEMICOLON,
          "Expected ';' after variable declaration");

  for (auto &d : decls) {
    d->end_line = semi.end_line;
    d->end_column = semi.end_column;
  }

  if (decls.size() == 1) {
    return std::move(decls[0]);
  }

  auto block = std::make_unique<BlockStatement>(start);
  block->block_kind = BlockKind::TRANSPARENT;
  for (auto &d : decls) {
    d->parent = block.get();
    block->children.push_back(std::move(d));
  }
  return block;
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
  if (match(TokenType::KEYWORD_IMPORT)) {
    if (has_parsed_type_declaration) {
      throw ParseError("Import statements must appear before class declarations", previous().line, previous().column);
    }
    return parse_import_statement();
  }
  if (match(TokenType::KEYWORD_ALIAS))
    return parse_alias_statement();

  if (match(TokenType::KEYWORD_RETURN)) {
    throw ParseError("'return' statement outside of function or method body", previous().line, previous().column);
  }

  has_parsed_type_declaration = true;

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
    } else if (match(TokenType::KEYWORD_ABSTRACT)) {
      is_abstract = true;
    } else if (match(TokenType::KEYWORD_VIRTUAL)) {
      is_virtual = true;
    } else if (match(TokenType::KEYWORD_OVERRIDE)) {
      is_override = true;
    } else {
      break;
    }
  }

  if (match(TokenType::KEYWORD_CLASS))
    return parse_class_declaration(modifier, is_abstract);
  if (match(TokenType::KEYWORD_INTERFACE))
    return parse_interface_declaration(modifier);
  if (match(TokenType::KEYWORD_ENUM))
    return parse_enum_declaration(modifier);

  if (check(TokenType::PUNCTUATION_CLOSE_BRACE)) {
    throw ParseError(fmt::format("Syntax error: unexpected token '}}' at line {}", peek().line));
  }

  return parse_field_or_method(modifier, is_static, is_inline, is_native,
                               is_const, false, false, false, false);
}

std::unique_ptr<Node> ParserState::parse_package_statement() {
  Token pkg = previous();
  std::string name_str = std::get<std::string>(
      consume(TokenType::IDENTIFIER, "Expected identifier in package statement, got number").value);
  while (match(TokenType::PUNCTUATION_DOT)) {
    name_str += ".";
    name_str += std::get<std::string>(
        consume(TokenType::IDENTIFIER, "Expected sub-package name after '.'")
            .value);
  }
  Token semi = consume(TokenType::PUNCTUATION_SEMICOLON,
          "Expected ';' after package declaration");
  log_trace("Declared package: {}", name_str);
  auto stmt = std::make_unique<PackageStatement>(pkg, name_str);
  stmt->end_line = semi.end_line;
  stmt->end_column = semi.end_column;
  return stmt;
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
  Token semi = consume(TokenType::PUNCTUATION_SEMICOLON,
          "Expected ';' after alias declaration");

  log_trace("Declared alias '{}' = '{}' (generics count: {})", a_name,
            type.to_string(), tparams.size());
  auto decl = std::make_unique<AliasStatement>(alias, a_name, std::move(type));
  decl->template_parameters = tparams;
  decl->end_line = semi.end_line;
  decl->end_column = semi.end_column;
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
  Token semi = consume(TokenType::PUNCTUATION_SEMICOLON, "Expected ';' after import statement");

  if (is_wildcard) {
    log_trace("Declared wildcard import: '{}.*'", full_path);
    auto stmt = std::make_unique<ImportStatement>(import_tok, full_path, "*");
    stmt->end_line = semi.end_line;
    stmt->end_column = semi.end_column;
    return stmt;
  }

  size_t last_dot = full_path.rfind('.');
  if (last_dot != std::string::npos) {
    std::string pkg = full_path.substr(0, last_dot);
    std::string sym = full_path.substr(last_dot + 1);
    log_trace("Declared symbol import: '{}' from '{}'", sym, pkg);
    auto stmt = std::make_unique<ImportStatement>(import_tok, pkg, sym);
    stmt->end_line = semi.end_line;
    stmt->end_column = semi.end_column;
    return stmt;
  }

  log_trace("Declared import: '{}'", full_path);
  auto stmt = std::make_unique<ImportStatement>(import_tok, full_path, "");
  stmt->end_line = semi.end_line;
  stmt->end_column = semi.end_column;
  return stmt;
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

std::unique_ptr<Node> ParserState::parse_interface_declaration(TokenType modifier) {
  Token iface_tok = previous();
  Token name = consume(TokenType::IDENTIFIER, "Expected interface name");
  std::string iface_name = std::get<std::string>(name.value);
  log_trace("Parsing interface '{}' at line {}", iface_name, iface_tok.line);

  auto decl = std::make_unique<ClassDeclaration>(iface_tok, iface_name);
  decl->is_interface = true;
  decl->is_abstract = true;
  decl->access_modifier = modifier;

  if (match(TokenType::OPERATOR_LESS_THAN)) {
    do {
      std::string t_param = std::get<std::string>(
          consume(TokenType::IDENTIFIER, "Expected template parameter name")
              .value);
      decl->template_parameters.push_back(t_param);
    } while (match(TokenType::PUNCTUATION_COMMA));
    consume(TokenType::OPERATOR_GREATER_THAN,
            "Expected '>' after template parameters");
    log_trace("Interface '{}' template parameters count: {}", iface_name,
              decl->template_parameters.size());
  }

  if (match(TokenType::KEYWORD_EXTENDS) || match(TokenType::PUNCTUATION_COLON)) {
    do {
      TypeInfo base_type = parse_type_info();
      if (decl->base_class_name.empty()) {
        decl->base_class_name = base_type.to_string();
        decl->base_class_type = base_type;
      }
      decl->implemented_interfaces.push_back(base_type.to_string());
      decl->interface_types.push_back(base_type);
    } while (match(TokenType::PUNCTUATION_COMMA));
  }

  consume(TokenType::PUNCTUATION_OPEN_BRACE, "Expected '{' before interface body");
  while (!check(TokenType::PUNCTUATION_CLOSE_BRACE) && !is_at_end()) {
    TypeInfo type = parse_type_info();
    Token m_name = consume(TokenType::IDENTIFIER, "Expected method name");
    std::string m_name_str = std::get<std::string>(m_name.value);
    if (!check(TokenType::PUNCTUATION_OPEN_PAREN)) {
      throw ParseError("Interfaces must not declare instance fields", m_name.line, m_name.column);
    }
    consume(TokenType::PUNCTUATION_OPEN_PAREN, "Expected '(' after method name");
    auto method = std::make_unique<MethodDeclaration>(m_name, m_name_str, std::move(type));
    method->is_abstract = true;
    method->access_modifier = TokenType::KEYWORD_PUBLIC;
    while (!check(TokenType::PUNCTUATION_CLOSE_PAREN) && !is_at_end()) {
      TypeInfo p_type = parse_type_info();
      Token p_name = consume(TokenType::IDENTIFIER, "Expected parameter name");
      method->parameters.push_back(std::make_unique<VariableDeclaration>(p_name, std::get<std::string>(p_name.value), std::move(p_type)));
      if (!match(TokenType::PUNCTUATION_COMMA)) break;
    }
    consume(TokenType::PUNCTUATION_CLOSE_PAREN, "Expected ')' after parameters");
    if (check(TokenType::PUNCTUATION_OPEN_BRACE)) {
      throw ParseError("Interface methods cannot have a body", peek().line, peek().column);
    }
    consume(TokenType::PUNCTUATION_SEMICOLON, "Expected ';' after interface method declaration");
    decl->children.push_back(std::move(method));
  }
  consume(TokenType::PUNCTUATION_CLOSE_BRACE, "Expected '}' after interface body");
  for (auto &child : decl->children) {
    child->parent = decl.get();
  }
  return decl;
}

std::unique_ptr<Node> ParserState::parse_class_declaration(TokenType modifier, bool is_abstract) {
  Token class_tok = previous();
  Token name = consume(TokenType::IDENTIFIER, "Expected class name");
  std::string cls_name = std::get<std::string>(name.value);
  log_trace("Parsing class '{}' at line {}", cls_name, class_tok.line);

  auto decl = std::make_unique<ClassDeclaration>(class_tok, cls_name);
  decl->is_abstract = is_abstract;
  decl->access_modifier = modifier;

  if (match(TokenType::OPERATOR_LESS_THAN)) {
    do {
      std::string t_param = std::get<std::string>(
          consume(TokenType::IDENTIFIER, "Expected template parameter name")
              .value);
      decl->template_parameters.push_back(t_param);
    } while (match(TokenType::PUNCTUATION_COMMA));
    consume(TokenType::OPERATOR_GREATER_THAN,
            "Expected '>' after template parameters");
    log_trace("Class '{}' template parameters count: {}", cls_name,
              decl->template_parameters.size());
  }

  if (match(TokenType::KEYWORD_EXTENDS) || match(TokenType::PUNCTUATION_COLON)) {
    TypeInfo base_type = parse_type_info();
    decl->base_class_name = base_type.to_string();
    decl->base_class_type = base_type;
    log_trace("Class '{}' extends '{}'", cls_name, decl->base_class_name);
  }
  if (match(TokenType::KEYWORD_IMPLEMENTS)) {
    do {
      TypeInfo iface_type = parse_type_info();
      decl->implemented_interfaces.push_back(iface_type.to_string());
      decl->interface_types.push_back(iface_type);
    } while (match(TokenType::PUNCTUATION_COMMA));
  }
  decl->access_modifier = modifier;

  consume(TokenType::PUNCTUATION_OPEN_BRACE, "Expected '{' after class inheritance clause");
  while (!check(TokenType::PUNCTUATION_CLOSE_BRACE) && !is_at_end()) {
    if (match(TokenType::KEYWORD_IMPORT)) {
      throw ParseError("Import statements must appear before class declarations", previous().line, previous().column);
    }
    if (match(TokenType::KEYWORD_PACKAGE)) {
      throw ParseError("'package' statement must be the first statement in the file", previous().line, previous().column);
    }
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

    if (check(TokenType::PUNCTUATION_OPEN_BRACE)) {
      throw ParseError("Executable blocks are not allowed directly in class body", peek().line, peek().column);
    }

    if (peek().type == TokenType::NUMBER ||
        peek().type == TokenType::STRING ||
        peek().type == TokenType::CHAR ||
        peek().type == TokenType::KEYWORD_BREAK ||
        peek().type == TokenType::KEYWORD_CONTINUE ||
        peek().type == TokenType::KEYWORD_RETURN ||
        peek().type == TokenType::KEYWORD_IF ||
        peek().type == TokenType::KEYWORD_WHILE ||
        peek().type == TokenType::KEYWORD_FOR ||
        peek().type == TokenType::KEYWORD_DO ||
        peek().type == TokenType::KEYWORD_SWITCH ||
        peek().type == TokenType::KEYWORD_TRY ||
        peek().type == TokenType::KEYWORD_THROW) {
      throw ParseError("Statements are not allowed directly in class body", peek().line, peek().column);
    }

    if (match(TokenType::KEYWORD_CLASS)) {
      decl->children.push_back(parse_class_declaration(field_mod, is_abstract));
      continue;
    }
    if (match(TokenType::KEYWORD_INTERFACE)) {
      decl->children.push_back(parse_interface_declaration(field_mod));
      continue;
    }
    if (match(TokenType::KEYWORD_ENUM)) {
      decl->children.push_back(parse_enum_declaration(field_mod));
      continue;
    }

    // Constructor check
    if (check(TokenType::IDENTIFIER) &&
        tokens[current + 1]->type == TokenType::PUNCTUATION_OPEN_PAREN &&
        !(current + 2 < tokens.size() && tokens[current + 2]->type == TokenType::OPERATOR_MULTIPLY)) {
      if (std::get<std::string>(peek().value) != decl->class_name) {
        throw ParseError("Constructor name '" + std::get<std::string>(peek().value) + "' does not match enclosing class '" + decl->class_name + "'", peek().line, peek().column);
      }
      Token ctor_name = advance();
      log_trace("Parsing constructor for '{}' at line {}", decl->class_name,
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
      ctor->end_line = body->end_line;
      ctor->end_column = body->end_column;
      ctor->children.push_back(std::move(body));
      decl->children.push_back(std::move(ctor));
    } else {
      auto member = parse_field_or_method(
          field_mod, is_static, is_inline, is_native, is_const, is_virtual,
          is_override, is_weak, is_abstract, true);
      if (member && member->node_type == NodeType::METHOD_DECL) {
        auto *m = static_cast<MethodDeclaration *>(member.get());
        if (m->method_name == decl->class_name) {
          throw ParseError("Constructors must not specify a return type", m->line, m->column);
        }
      }
      decl->children.push_back(std::move(member));
    }
  }
  Token close_brace = consume(TokenType::PUNCTUATION_CLOSE_BRACE, "Expected '}' after class body");
  decl->end_line = close_brace.line;
  decl->end_column = close_brace.column;

  for (auto &child : decl->children) {
    child->parent = decl.get();
  }

  log_trace("Completed parsing class '{}' with {} members", cls_name,
            decl->children.size());
  return decl;
}

std::unique_ptr<Node> ParserState::parse_field_or_method(
    TokenType modifier, bool is_static, bool is_inline, bool is_native,
    bool is_const, bool is_virtual, bool is_override, bool is_weak,
    bool is_abstract, bool is_inside_class) {
  TypeInfo type = parse_type_info();
  bool is_ref = match(TokenType::PUNCTUATION_AMPERSAND);

  if (check(TokenType::PUNCTUATION_OPEN_PAREN)) {
    throw ParseError("Constructors can only be declared inside a class body", peek().line, peek().column);
  }

  Token name;
  std::string name_str;
  if (match(TokenType::KEYWORD_OPERATOR)) {
    if (!is_inside_class) {
      throw ParseError("Operator overloads can only be declared inside a class body", previous().line, previous().column);
    }
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
    else if (op_token.type == TokenType::OPERATOR_EQUAL)
      name_str += "==";
    else if (op_token.type == TokenType::OPERATOR_NOT_EQUAL)
      name_str += "!=";
    else if (op_token.type == TokenType::OPERATOR_LESS_THAN)
      name_str += "<";
    else if (op_token.type == TokenType::OPERATOR_GREATER_THAN)
      name_str += ">";
    else if (op_token.type == TokenType::OPERATOR_LESS_EQUAL)
      name_str += "<=";
    else if (op_token.type == TokenType::OPERATOR_GREATER_EQUAL)
      name_str += ">=";
    else
      throw ParseError(fmt::format("Operator '{}' cannot be overloaded", operator_token_to_string(op_token.type)));
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
    log_trace("Detected explicit template specialization method signature: {}",
              name_str);
  }

  if (match(TokenType::PUNCTUATION_OPEN_PAREN)) {
    log_trace("Parsing method declaration '{}' returning '{}' at line {}",
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
      if (check(TokenType::PUNCTUATION_OPEN_BRACE)) {
        if (is_abstract) {
          throw ParseError(fmt::format("Abstract method '{}' cannot have a body", name_str));
        } else {
          throw ParseError(fmt::format("Native method '{}' cannot have a body", name_str));
        }
      }
      consume(TokenType::PUNCTUATION_SEMICOLON,
              "Expected ';' after native or abstract method declaration");
      log_trace("Finished native/abstract method header for '{}'", name_str);
    } else {
      auto block = parse_block();
      if (auto* b = dynamic_cast<BlockStatement*>(block.get())) {
          b->block_kind = BlockKind::FUNCTION_BODY;
      }
      method->end_line = block->end_line;
      method->end_column = block->end_column;
      block->parent = method.get();
      method->children.push_back(std::move(block));
    }
    return method;
  } else {
    if (!is_inside_class && modifier != TokenType::KEYWORD_INTERNAL) {
      throw ParseError("Variable declarations with access modifiers must be inside a class body", name.line, name.column);
    }
    log_trace("Parsing field declaration '{}' of type '{}' at line {}",
              name_str, type.to_string(), name.line);
    auto field =
        std::make_unique<FieldDeclaration>(name, name_str, std::move(type));
    field->access_modifier = modifier;
    field->is_static = is_static || is_const;
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
  log_trace("Starting Syntax Analysis (Parsing)...");

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

    log_trace("Parsing source file: {}", source_name);

    try {
      while (!state.is_at_end()) {
        context.nodes[source].push_back(state.parse_top_level_declaration());
      }
    } catch (const ParseError &e) {
      log_error("[{}:{}:{}] Syntax Error: {}", source_name, e.line, e.column, e.what());
      if (context.diagnostic) {
        Report report;
        report.severity = ReportSeverity::ERROR;
        report.code = "E_PARSE";
        report.message = e.what();
        report.source_path = source_name;
        report.line = e.line;
        report.column = e.column;
        report.end_line = e.end_line;
        report.end_column = e.end_column;
        context.diagnostic->record_report(report);
      }
    } catch (const std::exception &e) {
      log_error("[{}:0:0] Syntax Error: {}", source_name, e.what());
      if (context.diagnostic) {
        Report report;
        report.severity = ReportSeverity::ERROR;
        report.code = "E_PARSE";
        report.message = e.what();
        report.source_path = source_name;
        report.line = 0;
        report.column = 0;
        report.end_line = 0;
        report.end_column = 0;
        context.diagnostic->record_report(report);
      }
    }

    log_trace("Completed parsing {}: generated {} top-level AST nodes",
              source_name, context.nodes[source].size());
  }

  log_trace("Syntax Analysis completed.");
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

std::unique_ptr<Node> ParserState::parse_assert_statement() {
  Token assert_tok = previous();
  log_trace("Parsing 'assert' statement at line {}", assert_tok.line);

  bool has_paren = match(TokenType::PUNCTUATION_OPEN_PAREN);
  auto condition = parse_expression();
  std::unique_ptr<Node> message = nullptr;
  if (match(TokenType::PUNCTUATION_COMMA)) {
    message = parse_expression();
  }
  if (has_paren) {
    consume(TokenType::PUNCTUATION_CLOSE_PAREN, "Expected ')' after assert condition");
  }
  consume(TokenType::PUNCTUATION_SEMICOLON, "Expected ';' after assert statement");

  return std::make_unique<AssertStatement>(assert_tok, std::move(condition), std::move(message));
}

std::unique_ptr<Node> ParserState::parse_exit_statement() {
  Token exit_tok = previous();
  log_trace("Parsing 'exit' statement at line {}", exit_tok.line);

  bool has_paren = match(TokenType::PUNCTUATION_OPEN_PAREN);
  auto code = parse_expression();
  if (has_paren) {
    consume(TokenType::PUNCTUATION_CLOSE_PAREN, "Expected ')' after exit code");
  }
  consume(TokenType::PUNCTUATION_SEMICOLON, "Expected ';' after exit statement");

  return std::make_unique<ExitStatement>(exit_tok, std::move(code));
}
} // namespace solix
