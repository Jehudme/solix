import sys

with open("language/src/parser.cpp", "r") as f:
    code = f.read()

# Fix parsing native modifier
target_modifier = """        tokens[index].type == lexer::TokenType::KEYWORD_CONST ||
        tokens[index].type == lexer::TokenType::KEYWORD_INLINE)) {
        if (tokens[index].type == lexer::TokenType::KEYWORD_STATIC) is_static = true;
        else if (tokens[index].type == lexer::TokenType::KEYWORD_INLINE) is_inline = true;"""

replacement_modifier = """        tokens[index].type == lexer::TokenType::KEYWORD_CONST ||
        tokens[index].type == lexer::TokenType::KEYWORD_NATIVE ||
        tokens[index].type == lexer::TokenType::KEYWORD_INLINE)) {
        if (tokens[index].type == lexer::TokenType::KEYWORD_STATIC) is_static = true;
        else if (tokens[index].type == lexer::TokenType::KEYWORD_INLINE) is_inline = true;
        else if (tokens[index].type == lexer::TokenType::KEYWORD_NATIVE) is_native = true;"""

code = code.replace(target_modifier, replacement_modifier)

# Fix body check for native methods
target_body = """    if (param_end < tokens.size() - 1 && tokens[param_end + 1].type == lexer::TokenType::PUNCTUATION_OPEN_BRACE) {
        std::vector<lexer::Token> body_tokens(tokens.begin() + param_end + 1, tokens.end());
        children.push_back(parseTokensToNode(body_tokens, this));
    }
}"""

replacement_body = """    if (param_end < tokens.size() - 1 && tokens[param_end + 1].type == lexer::TokenType::PUNCTUATION_OPEN_BRACE) {
        if (is_native) throw_parse_error(tokens, "Native functions cannot have a body.");
        std::vector<lexer::Token> body_tokens(tokens.begin() + param_end + 1, tokens.end());
        children.push_back(parseTokensToNode(body_tokens, this));
    } else if (is_native && param_end < tokens.size() - 1 && tokens[param_end + 1].type != lexer::TokenType::PUNCTUATION_SEMICOLON) {
        throw_parse_error(tokens, "Expected ';' after native function declaration.");
    }
}"""

# Need to replace the last occurrence which is the MethodDeclaration constructor
parts = code.rsplit(target_body, 1)
if len(parts) == 2:
    code = replacement_body.join(parts)

with open("language/src/parser.cpp", "w") as f:
    f.write(code)
