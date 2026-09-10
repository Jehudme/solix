#include "solix/lexer.hpp"
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <cctype>

namespace solix::lexer {

namespace {

    const std::unordered_map<std::string_view, TokenType> keywords = {
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
        {"string", TokenType::PRIMITIVE_STRING},
        
        {"if", TokenType::KEYWORD_IF},
        {"else", TokenType::KEYWORD_ELSE},
        {"for", TokenType::KEYWORD_FOR},
        {"while", TokenType::KEYWORD_WHILE},
        {"return", TokenType::KEYWORD_RETURN},
        {"break", TokenType::KEYWORD_BREAK},
        {"continue", TokenType::KEYWORD_CONTINUE},
        {"switch", TokenType::KEYWORD_SWITCH},
        {"case", TokenType::KEYWORD_CASE},
        {"default", TokenType::KEYWORD_DEFAULT},
        {"const", TokenType::KEYWORD_CONST},
        {"class", TokenType::KEYWORD_CLASS},
        {"enum", TokenType::KEYWORD_ENUM},
        {"static", TokenType::KEYWORD_STATIC},
        {"inline", TokenType::KEYWORD_INLINE},
        {"alias", TokenType::KEYWORD_ALIAS},
        {"public", TokenType::KEYWORD_PUBLIC},
        {"protected", TokenType::KEYWORD_PROTECTED},
        {"private", TokenType::KEYWORD_PRIVATE},
        {"internal", TokenType::KEYWORD_INTERNAL},
        {"package", TokenType::KEYWORD_PACKAGE},
        {"new", TokenType::KEYWORD_NEW}
    };

    class LexerContext {
    private:
        std::string_view source;
        std::optional<std::filesystem::path> file_path;
        
        size_t start_pos = 0;
        size_t current_pos = 0;
        
        size_t current_line = 1;
        size_t current_column = 1;
        size_t start_column = 1;

        std::vector<Token> tokens;

        bool is_at_end() const { 
            return current_pos >= source.length(); 
        }
        
        char peek() const { 
            return is_at_end() ? '\0' : source[current_pos]; 
        }
        
        char peek_next() const { 
            return (current_pos + 1 >= source.length()) ? '\0' : source[current_pos + 1]; 
        }
        
        char advance() {
            current_column++;
            return source[current_pos++];
        }

        bool match(char expected) {
            if (is_at_end() || source[current_pos] != expected) return false;
            current_pos++;
            current_column++;
            return true;
        }

        void add_token(TokenType type) {
            std::string text = std::string(source.substr(start_pos, current_pos - start_pos));
            tokens.push_back(Token{type, text, file_path, current_line, start_column});
        }

        void handle_string() {
            char quote_type = source[start_pos];
            while (peek() != quote_type && !is_at_end()) {
                if (peek() == '\\' && peek_next() != '\0') {
                    advance(); // Consume the backslash
                    advance(); // Consume the escaped character
                    continue;
                }
                if (peek() == '\n') {
                    current_line++;
                    current_column = 0; // will be 1 after advance()
                }
                advance();
            }

            if (is_at_end()) {
                std::string text = std::string(source.substr(start_pos, current_pos - start_pos));
                tokens.push_back(Token{TokenType::UNKNOWN_TOKEN, text, file_path, current_line, start_column});
                return;
            }

            advance(); // Consume the closing quote
            add_token(TokenType::STRING);
        }

        void handle_number() {
            while (std::isdigit(peek())) advance();

            // Look for a fractional part
            if (peek() == '.' && std::isdigit(peek_next())) {
                advance(); // Consume the "."
                while (std::isdigit(peek())) advance();
            }

            add_token(TokenType::NUMBER);
        }

        void handle_identifier() {
            while (std::isalnum(peek()) || peek() == '_') advance();

            std::string text = std::string(source.substr(start_pos, current_pos - start_pos));
            auto it = keywords.find(text);
            
            if (it != keywords.end()) {
                add_token(it->second); // It's a keyword
            } else {
                add_token(TokenType::IDENTIFIER); // It's a regular identifier
            }
        }

    public:
        LexerContext(std::string_view src, std::optional<std::filesystem::path> path = std::nullopt) 
            : source(src), file_path(path) {}

        std::vector<Token> tokenize() {
            while (!is_at_end()) {
                // Handle whitespace manually to keep track of lines and columns accurately
                char c = peek();
                if (c == ' ' || c == '\r' || c == '\t') {
                    advance();
                    continue;
                } else if (c == '\n') {
                    current_pos++;
                    current_line++;
                    current_column = 1;
                    continue;
                }

                start_pos = current_pos;
                start_column = current_column;
                c = advance();

                if (std::isalpha(c) || c == '_') {
                    handle_identifier();
                } else if (std::isdigit(c)) {
                    handle_number();
                } else if (c == '"' || c == '\'') {
                    handle_string();
                } else {
                    switch (c) {
                        // Punctuation
                        case '(': add_token(TokenType::PUNCTUATION_OPEN_PAREN); break;
                        case ')': add_token(TokenType::PUNCTUATION_CLOSE_PAREN); break;
                        case '{': add_token(TokenType::PUNCTUATION_OPEN_BRACE); break;
                        case '}': add_token(TokenType::PUNCTUATION_CLOSE_BRACE); break;
                        case '[': add_token(TokenType::PUNCTUATION_OPEN_BRACKET); break;
                        case ']': add_token(TokenType::PUNCTUATION_CLOSE_BRACKET); break;
                        case ';': add_token(TokenType::PUNCTUATION_SEMICOLON); break;
                        case ',': add_token(TokenType::PUNCTUATION_COMMA); break;
                        case '.': add_token(TokenType::PUNCTUATION_DOT); break;
                        case ':': add_token(TokenType::PUNCTUATION_COLON); break;
                        
                        // Operators (1 or 2 characters)
                        case '=': add_token(match('=') ? TokenType::OPERATOR_EQUAL : TokenType::OPERATOR_ASSIGN); break;
                        case '!': add_token(match('=') ? TokenType::OPERATOR_NOT_EQUAL : TokenType::OPERATOR_LOGICAL_NOT); break;
                        case '<': add_token(match('=') ? TokenType::OPERATOR_LESS_EQUAL : TokenType::OPERATOR_LESS_THAN); break;
                        case '>': add_token(match('=') ? TokenType::OPERATOR_GREATER_EQUAL : TokenType::OPERATOR_GREATER_THAN); break;
                        
                        case '&': 
                            if (match('&')) add_token(TokenType::OPERATOR_LOGICAL_AND);
                            else {
                                std::string text = std::string(source.substr(start_pos, current_pos - start_pos));
                                tokens.push_back(Token{TokenType::UNKNOWN_TOKEN, text, file_path, current_line, start_column});
                            }
                            break;
                        case '|': 
                            if (match('|')) add_token(TokenType::OPERATOR_LOGICAL_OR);
                            else {
                                std::string text = std::string(source.substr(start_pos, current_pos - start_pos));
                                tokens.push_back(Token{TokenType::UNKNOWN_TOKEN, text, file_path, current_line, start_column});
                            }
                            break;
                            
                        case '+': add_token(match('+') ? TokenType::OPERATOR_INCREMENT : (match('=') ? TokenType::OPERATOR_PLUS_ASSIGN : TokenType::OPERATOR_PLUS)); break;
                        case '-': add_token(match('-') ? TokenType::OPERATOR_DECREMENT : (match('=') ? TokenType::OPERATOR_MINUS_ASSIGN : TokenType::OPERATOR_MINUS)); break;
                        case '*': add_token(match('=') ? TokenType::OPERATOR_MULTIPLY_ASSIGN : TokenType::OPERATOR_MULTIPLY); break;
                        case '%': add_token(match('=') ? TokenType::OPERATOR_MODULO_ASSIGN : TokenType::OPERATOR_MODULO); break;
                        
                        // Division or Comments
                        case '/':
                            if (match('=')) {
                                add_token(TokenType::OPERATOR_DIVIDE_ASSIGN);
                            } else if (match('/')) {
                                while (peek() != '\n' && !is_at_end()) advance();
                                add_token(TokenType::COMMENT_SINGLE_LINE);
                            } else if (match('*')) {
                                while (!is_at_end()) {
                                    if (peek() == '\n') {
                                        current_line++;
                                        current_column = 0; // will be 1 after advance
                                    }
                                    if (peek() == '*' && peek_next() == '/') {
                                        advance(); // consume *
                                        advance(); // consume /
                                        break;
                                    }
                                    advance();
                                }
                                add_token(TokenType::COMMENT_MULTI_LINE);
                            } else {
                                add_token(TokenType::OPERATOR_DIVIDE);
                            }
                            break;
                            
                        default: {
                            std::string text = std::string(source.substr(start_pos, current_pos - start_pos));
                            tokens.push_back(Token{TokenType::UNKNOWN_TOKEN, text, file_path, current_line, start_column});
                            break;
                        }
                    }
                }
            }
            
            tokens.push_back(Token{TokenType::EOF_TOKEN, std::nullopt, file_path, current_line, current_column});
            return std::move(tokens);
        }
    };

} // anonymous namespace

std::vector<Token> tokenize(std::string_view source) {
    LexerContext context(source);
    return context.tokenize();
}

std::vector<Token> tokenize_file(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (file) {
        std::ostringstream ss;
        ss << file.rdbuf();
        std::string file_content = ss.str();
        LexerContext context(file_content, path);
        return context.tokenize();
    }
    return {};
}

} // namespace solix::lexer
