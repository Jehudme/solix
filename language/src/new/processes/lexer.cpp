#include "solix/new/processes/lexer.hpp"
#include "solix/new/compilation.hpp"
#include "solix/new/utilities/token.hpp"
#include <fstream>
#include <sstream>
#include <cctype>

namespace solix {

class LexerState {
    const std::string& source_code;
    const Source* source_ref;
    uint32_t current_pos = 0;
    uint32_t start_pos = 0;
    uint32_t current_line = 1;
    uint32_t current_column = 1;
    uint32_t start_column = 1;

    TokenList tokens;

    char peek() const {
        if (is_at_end()) return '\0';
        return source_code[current_pos];
    }

    char peek_next() const {
        if (current_pos + 1 >= source_code.length()) return '\0';
        return source_code[current_pos + 1];
    }

    char advance() {
        current_column++;
        return source_code[current_pos++];
    }

    bool is_at_end() const {
        return current_pos >= source_code.length();
    }

    bool match(char expected) {
        if (is_at_end()) return false;
        if (source_code[current_pos] != expected) return false;
        current_column++;
        current_pos++;
        return true;
    }

    void add_token(TokenType type) {
        tokens.push_back({type, current_line, start_column, source_ref, std::nullptr_t{}});
    }

    void add_token(TokenType type, Value value) {
        tokens.push_back({type, current_line, start_column, source_ref, std::move(value)});
    }

    void handle_string() {
        char quote_type = source_code[current_pos - 1]; // " or '
        while (peek() != quote_type && !is_at_end()) {
            if (peek() == '\n') {
                current_line++;
                current_column = 1;
            }
            advance();
        }

        if (is_at_end()) {
            add_token(TokenType::UNKNOWN_TOKEN, std::string("Unterminated string"));
            return;
        }

        advance(); // consume closing quote
        std::string parsed_string = source_code.substr(start_pos + 1, current_pos - start_pos - 2);
        add_token(TokenType::STRING, parsed_string);
    }

    void handle_number() {
        while (std::isdigit(peek())) advance();

        if (peek() == '.' && std::isdigit(peek_next())) {
            advance(); // Consume the "."
            while (std::isdigit(peek())) advance();
            
            std::string text = source_code.substr(start_pos, current_pos - start_pos);
            add_token(TokenType::NUMBER, std::stod(text));
        } else {
            std::string text = source_code.substr(start_pos, current_pos - start_pos);
            add_token(TokenType::NUMBER, (int64_t)std::stoll(text));
        }
    }

    void handle_identifier() {
        while (std::isalnum(peek()) || peek() == '_') advance();

        std::string text = source_code.substr(start_pos, current_pos - start_pos);
        auto it = keywords.find(text);
        if (it != keywords.end()) {
            add_token(it->second);
        } else {
            add_token(TokenType::IDENTIFIER, text);
        }
    }

public:
    LexerState(const std::string& src_content, const Source* ref) 
        : source_code(src_content), source_ref(ref) {}

    TokenList tokenize() {
        while (!is_at_end()) {
            char current_character = peek();
            if (current_character == ' ' || current_character == '\r' || current_character == '\t') {
                advance();
                continue;
            } else if (current_character == '\n') {
                current_pos++;
                current_line++;
                current_column = 1;
                continue;
            }

            start_pos = current_pos;
            start_column = current_column;
            current_character = advance();

            if (std::isalpha(current_character) || current_character == '_') {
                handle_identifier();
            } else if (std::isdigit(current_character)) {
                handle_number();
            } else if (current_character == '"' || current_character == '\'') {
                handle_string();
            } else {
                switch (current_character) {
                    case '(': add_token(TokenType::PUNCTUATION_OPEN_PAREN); break;
                    case ')': add_token(TokenType::PUNCTUATION_CLOSE_PAREN); break;
                    case '{': add_token(TokenType::PUNCTUATION_OPEN_BRACE); break;
                    case '}': add_token(TokenType::PUNCTUATION_CLOSE_BRACE); break;
                    case '[': 
                        if (match(']')) {
                            add_token(TokenType::PUNCTUATION_ARRAY_BRACKETS);
                        } else {
                            add_token(TokenType::PUNCTUATION_OPEN_BRACKET);
                        }
                        break;
                    case ']': add_token(TokenType::PUNCTUATION_CLOSE_BRACKET); break;
                    case ';': add_token(TokenType::PUNCTUATION_SEMICOLON); break;
                    case ',': add_token(TokenType::PUNCTUATION_COMMA); break;
                    case '.': add_token(TokenType::PUNCTUATION_DOT); break;
                    case ':': add_token(TokenType::PUNCTUATION_COLON); break;
                    case '?': add_token(TokenType::OPERATOR_QUESTION); break;
                    
                    case '=': add_token(match('=') ? TokenType::OPERATOR_EQUAL : TokenType::OPERATOR_ASSIGN); break;
                    case '!': add_token(match('=') ? TokenType::OPERATOR_NOT_EQUAL : TokenType::OPERATOR_LOGICAL_NOT); break;
                    case '<': add_token(match('=') ? TokenType::OPERATOR_LESS_EQUAL : TokenType::OPERATOR_LESS_THAN); break;
                    case '>': add_token(match('=') ? TokenType::OPERATOR_GREATER_EQUAL : TokenType::OPERATOR_GREATER_THAN); break;
                    
                    case '&': 
                        if (match('&')) add_token(TokenType::OPERATOR_LOGICAL_AND);
                        else add_token(TokenType::UNKNOWN_TOKEN, std::string("&"));
                        break;
                    case '|': 
                        if (match('|')) add_token(TokenType::OPERATOR_LOGICAL_OR);
                        else add_token(TokenType::UNKNOWN_TOKEN, std::string("|"));
                        break;
                        
                    case '+': add_token(match('+') ? TokenType::OPERATOR_INCREMENT : (match('=') ? TokenType::OPERATOR_PLUS_ASSIGN : TokenType::OPERATOR_PLUS)); break;
                    case '-': add_token(match('-') ? TokenType::OPERATOR_DECREMENT : (match('=') ? TokenType::OPERATOR_MINUS_ASSIGN : TokenType::OPERATOR_MINUS)); break;
                    case '*': add_token(match('=') ? TokenType::OPERATOR_MULTIPLY_ASSIGN : TokenType::OPERATOR_MULTIPLY); break;
                    case '%': add_token(match('=') ? TokenType::OPERATOR_MODULO_ASSIGN : TokenType::OPERATOR_MODULO); break;
                    
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
                        
                    default:
                        add_token(TokenType::UNKNOWN_TOKEN, std::string(1, current_character));
                        break;
                }
            }
        }
        
        tokens.push_back({TokenType::EOF_TOKEN, current_line, current_column, source_ref, std::nullptr_t{}});
        return std::move(tokens);
    }
};

void Lexer::execute() {
    log_info("Starting Lexical Analysis...");
    size_t total_files = context.options.sources.size();
    size_t file_index = 0;

    for (const auto& [source, opt_content] : context.options.sources) {
        file_index++;
        std::string source_name;
        if (std::holds_alternative<std::filesystem::path>(source)) {
            source_name = std::get<std::filesystem::path>(source).string();
        } else {
            source_name = std::get<std::string>(source);
        }

        log_trace("Processing source {}/{} : {}", file_index, total_files, source_name);

        std::string content;
        if (opt_content.has_value()) {
            content = opt_content.value();
        } else {
            if (std::holds_alternative<std::filesystem::path>(source)) {
                std::ifstream file(std::get<std::filesystem::path>(source));
                if (file) {
                    std::ostringstream ss;
                    ss << file.rdbuf();
                    content = ss.str();
                } else {
                    log_error("Failed to read file: {}", source_name);
                    continue;
                }
            } else {
                log_error("Source '{}' is a string identifier but no content was provided", source_name);
                continue;
            }
        }

        LexerState state(content, &source);
        TokenList tokens = state.tokenize();
        
        log_debug("Tokenized {} with {} tokens", source_name, tokens.size());
        context.tokens[source].push_back(std::move(tokens));
    }
    
    log_info("Lexical Analysis completed.");
}

} // namespace solix
