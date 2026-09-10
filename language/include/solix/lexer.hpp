#pragma once
#include <cstddef>
#include <filesystem>
#include <optional>
#include <string_view>
#include <string>
#include <vector>

namespace solix::lexer {
  enum class TokenType {
    // Special
    EOF_TOKEN,
    UNKNOWN_TOKEN,

    // Basic tokens
    IDENTIFIER,
    NUMBER,
    STRING,

    // Primitive types
    PRIMITIVE_VOID,
    PRIMITIVE_BOOL,

    PRIMITIVE_INT8,
    PRIMITIVE_INT16,
    PRIMITIVE_INT32,
    PRIMITIVE_INT64,

    PRIMITIVE_UINT8,
    PRIMITIVE_UINT16,
    PRIMITIVE_UINT32,
    PRIMITIVE_UINT64,

    PRIMITIVE_FLOAT32,
    PRIMITIVE_FLOAT64,

    PRIMITIVE_CHAR,
    PRIMITIVE_STRING,
    PRIMITIVE_ARRAY,

    // Keywords
    KEYWORD_IF,
    KEYWORD_ELSE,
    KEYWORD_FOR,
    KEYWORD_WHILE,
    KEYWORD_RETURN,
    KEYWORD_BREAK,
    KEYWORD_CONTINUE,

    KEYWORD_SWITCH,
    KEYWORD_CASE,
    KEYWORD_DEFAULT,

    KEYWORD_CONST,

    KEYWORD_CLASS,
    KEYWORD_ENUM,

    KEYWORD_STATIC,
    KEYWORD_INLINE,

    KEYWORD_ALIAS, // create alias eg: alias int = int32;

    KEYWORD_PUBLIC,
    KEYWORD_PROTECTED,
    KEYWORD_PRIVATE,
    KEYWORD_INTERNAL, // visible in the same package

    KEYWORD_PACKAGE,
    
    KEYWORD_NEW,

    // Operators
    OPERATOR_ASSIGN,

    OPERATOR_PLUS,
    OPERATOR_MINUS,
    OPERATOR_MULTIPLY,
    OPERATOR_DIVIDE,
    OPERATOR_MODULO,

    OPERATOR_INCREMENT,
    OPERATOR_DECREMENT,

    OPERATOR_EQUAL,
    OPERATOR_NOT_EQUAL,

    OPERATOR_LESS_THAN,
    OPERATOR_GREATER_THAN,
    OPERATOR_LESS_EQUAL,
    OPERATOR_GREATER_EQUAL,

    OPERATOR_LOGICAL_AND,
    OPERATOR_LOGICAL_OR,
    OPERATOR_LOGICAL_NOT,

    // Punctuation
    PUNCTUATION_SEMICOLON,
    PUNCTUATION_COMMA,
    PUNCTUATION_DOT,
    PUNCTUATION_COLON,

    PUNCTUATION_OPEN_PAREN,
    PUNCTUATION_CLOSE_PAREN,

    PUNCTUATION_OPEN_BRACE,
    PUNCTUATION_CLOSE_BRACE,

    PUNCTUATION_OPEN_BRACKET,
    PUNCTUATION_CLOSE_BRACKET,

    // Comments
    COMMENT_SINGLE_LINE,
    COMMENT_MULTI_LINE
  };

  struct Token {
    TokenType type;
    std::optional<std::string> value;
    std::optional<std::filesystem::path> path;
    size_t line;
    size_t column;
  };

  std::vector<Token> tokenize(std::string_view source);
  std::vector<Token> tokenize_file(const std::filesystem::path& path);

} // solix::lexer
