#pragma once
#include <filesystem>
#include <string>
#include <variant>
#include <vector>
#include <unordered_map>
#include <string_view>

namespace solix {
struct Token;

using TokenList = std::vector<Token>;
using Source = std::variant<std::string, std::filesystem::path>;
using Value = std::variant<int64_t, double, std::string, std::nullptr_t>;

enum class TokenType {
  EOF_TOKEN, UNKNOWN_TOKEN,
  IDENTIFIER, NUMBER, STRING, CHAR,
  PRIMITIVE_VOID, PRIMITIVE_BOOL,
  PRIMITIVE_INT8, PRIMITIVE_INT16, PRIMITIVE_INT32, PRIMITIVE_INT64,
  PRIMITIVE_UINT8, PRIMITIVE_UINT16, PRIMITIVE_UINT32, PRIMITIVE_UINT64,
  PRIMITIVE_FLOAT32, PRIMITIVE_FLOAT64,
  PRIMITIVE_CHAR,

  KEYWORD_IF, KEYWORD_ELSE, KEYWORD_FOR, KEYWORD_WHILE, KEYWORD_DO,
  KEYWORD_RETURN, KEYWORD_BREAK, KEYWORD_CONTINUE,
  KEYWORD_SWITCH, KEYWORD_CASE, KEYWORD_DEFAULT,
  KEYWORD_CONST, KEYWORD_CLASS, KEYWORD_ENUM,
  KEYWORD_TRY, KEYWORD_CATCH, KEYWORD_FINALLY, KEYWORD_THROW,
  KEYWORD_STATIC, KEYWORD_INLINE, KEYWORD_ALIAS, KEYWORD_IMPORT,
  KEYWORD_PUBLIC, KEYWORD_NATIVE, KEYWORD_PROTECTED, KEYWORD_PRIVATE, KEYWORD_INTERNAL,
  KEYWORD_PACKAGE, KEYWORD_NEW, KEYWORD_OPERATOR, KEYWORD_EXTENDS, KEYWORD_SUPER, KEYWORD_VIRTUAL, KEYWORD_OVERRIDE, KEYWORD_WEAK, KEYWORD_ABSTRACT, KEYWORD_INTERFACE, KEYWORD_IMPLEMENTS, KEYWORD_INSTANCEOF,

  OPERATOR_ASSIGN, OPERATOR_QUESTION,
  OPERATOR_PLUS, OPERATOR_MINUS, OPERATOR_MULTIPLY, OPERATOR_DIVIDE, OPERATOR_MODULO,
  OPERATOR_PLUS_ASSIGN, OPERATOR_MINUS_ASSIGN, OPERATOR_MULTIPLY_ASSIGN, OPERATOR_DIVIDE_ASSIGN, OPERATOR_MODULO_ASSIGN,
  OPERATOR_INCREMENT, OPERATOR_DECREMENT,
  OPERATOR_EQUAL, OPERATOR_NOT_EQUAL,
  OPERATOR_LESS_THAN, OPERATOR_GREATER_THAN, OPERATOR_LESS_EQUAL, OPERATOR_GREATER_EQUAL,
  OPERATOR_LOGICAL_AND, OPERATOR_LOGICAL_OR, OPERATOR_LOGICAL_NOT,

  PUNCTUATION_SEMICOLON, PUNCTUATION_COMMA, PUNCTUATION_DOT, PUNCTUATION_COLON, PUNCTUATION_DOUBLE_COLON,
  PUNCTUATION_OPEN_PAREN, PUNCTUATION_CLOSE_PAREN,
  PUNCTUATION_OPEN_BRACE, PUNCTUATION_CLOSE_BRACE,
  PUNCTUATION_OPEN_BRACKET, PUNCTUATION_CLOSE_BRACKET,
  PUNCTUATION_ARRAY_BRACKETS, // The [] optimization!
  PUNCTUATION_AMPERSAND,

  COMMENT_SINGLE_LINE, COMMENT_MULTI_LINE
};

inline const std::unordered_map<std::string_view, TokenType> keywords = {
    {"void", TokenType::PRIMITIVE_VOID},
    {"bool", TokenType::PRIMITIVE_BOOL},
    {"int8", TokenType::PRIMITIVE_INT8},
    {"int16", TokenType::PRIMITIVE_INT16},
    {"int32", TokenType::PRIMITIVE_INT32},
    {"int64", TokenType::PRIMITIVE_INT64},
    {"uint8", TokenType::PRIMITIVE_UINT8},
    {"uint16", TokenType::PRIMITIVE_UINT16},
    {"uint32", TokenType::PRIMITIVE_UINT32},
    {"uint64", TokenType::PRIMITIVE_UINT64},
    {"float32", TokenType::PRIMITIVE_FLOAT32},
    {"float64", TokenType::PRIMITIVE_FLOAT64},
    {"char", TokenType::PRIMITIVE_CHAR},

    {"if", TokenType::KEYWORD_IF},
    {"else", TokenType::KEYWORD_ELSE},
    {"for", TokenType::KEYWORD_FOR},
    {"while", TokenType::KEYWORD_WHILE},
    {"do", TokenType::KEYWORD_DO},
    {"return", TokenType::KEYWORD_RETURN},
    {"break", TokenType::KEYWORD_BREAK},
    {"continue", TokenType::KEYWORD_CONTINUE},
    {"switch", TokenType::KEYWORD_SWITCH},
    {"case", TokenType::KEYWORD_CASE},
    {"default", TokenType::KEYWORD_DEFAULT},
    {"try", TokenType::KEYWORD_TRY},
    {"catch", TokenType::KEYWORD_CATCH},
    {"finally", TokenType::KEYWORD_FINALLY},
    {"throw", TokenType::KEYWORD_THROW},
    {"const", TokenType::KEYWORD_CONST},
    {"class", TokenType::KEYWORD_CLASS},
    {"enum", TokenType::KEYWORD_ENUM},
    {"static", TokenType::KEYWORD_STATIC},
    {"inline", TokenType::KEYWORD_INLINE},
    {"alias", TokenType::KEYWORD_ALIAS},
    {"import", TokenType::KEYWORD_IMPORT},
    {"public", TokenType::KEYWORD_PUBLIC},
    {"native", TokenType::KEYWORD_NATIVE},
    {"protected", TokenType::KEYWORD_PROTECTED},
    {"private", TokenType::KEYWORD_PRIVATE},
    {"internal", TokenType::KEYWORD_INTERNAL},
    {"package", TokenType::KEYWORD_PACKAGE},
    {"new", TokenType::KEYWORD_NEW},
    {"operator", TokenType::KEYWORD_OPERATOR},
    {"extends", TokenType::KEYWORD_EXTENDS},
    {"super", TokenType::KEYWORD_SUPER},
    {"virtual", TokenType::KEYWORD_VIRTUAL},
    {"override", TokenType::KEYWORD_OVERRIDE},
    {"weak", TokenType::KEYWORD_WEAK},
    {"abstract", TokenType::KEYWORD_ABSTRACT},
    {"interface", TokenType::KEYWORD_INTERFACE},
    {"implements", TokenType::KEYWORD_IMPLEMENTS},
    {"instanceof", TokenType::KEYWORD_INSTANCEOF},
};

struct Token {
  TokenType type;
  uint32_t line;
  uint32_t column;
  const Source* source;
  Value value;
};

} // namespace solix
