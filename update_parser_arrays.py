with open("language/src/parser.cpp", "r") as f:
    code = f.read()

# Replace consumeType
bad_consume = """size_t consumeType(const std::vector<lexer::Token>& tokens, size_t start_index) {
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
        tokens[i].type != lexer::TokenType::PRIMITIVE_VOID &&
        tokens[i].type != lexer::TokenType::PRIMITIVE_CHAR) {
        return i; // Not a type
    }
    
    // Check for namespaced types (e.g. Engine.CoreProcessor)
    while (i < tokens.size() && (tokens[i].type == lexer::TokenType::IDENTIFIER || 
           (tokens[i].type >= lexer::TokenType::PRIMITIVE_VOID && tokens[i].type <= lexer::TokenType::PRIMITIVE_STRING))) {
        i++;
        if (i < tokens.size() && tokens[i].type == lexer::TokenType::PUNCTUATION_DOT) {
            i++;
        } else {
            break;
        }
    }
    return i;
}"""

good_consume = """std::string extractTypeName(const std::vector<lexer::Token>& tokens, size_t start, size_t end) {
    std::string result = "";
    for (size_t j = start; j < end; j++) {
        std::string val = tokens[j].value.value_or("");
        if (val == "[") {
            result += "[]";
            j++; // skip the closing ]
        } else if (val == ".") {
            if (!result.empty() && result.back() == ' ') result.pop_back();
            result += ".";
        } else {
            if (!result.empty() && result.back() == '.') result += val;
            else result += val + " ";
        }
    }
    while (!result.empty() && result.back() == ' ') result.pop_back();
    return result;
}

size_t consumeType(const std::vector<lexer::Token>& tokens, size_t start_index) {
    size_t i = start_index;
    if (i >= tokens.size()) return i;
    
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
        tokens[i].type != lexer::TokenType::PRIMITIVE_VOID &&
        tokens[i].type != lexer::TokenType::PRIMITIVE_CHAR) {
        return i; // Not a type
    }
    
    // Check for namespaced types (e.g. Engine.CoreProcessor)
    while (i < tokens.size() && (tokens[i].type == lexer::TokenType::IDENTIFIER || 
           (tokens[i].type >= lexer::TokenType::PRIMITIVE_VOID && tokens[i].type <= lexer::TokenType::PRIMITIVE_STRING))) {
        i++;
        if (i < tokens.size() && tokens[i].type == lexer::TokenType::PUNCTUATION_DOT) {
            i++;
        } else {
            break;
        }
    }
    
    // Now look for postfix array brackets []
    while (i + 1 < tokens.size() && 
           tokens[i].type == lexer::TokenType::PUNCTUATION_OPEN_BRACKET && 
           tokens[i+1].type == lexer::TokenType::PUNCTUATION_CLOSE_BRACKET) {
        i += 2;
    }
    
    return i;
}"""
code = code.replace(bad_consume, good_consume)

# Update places that construct type_name
code = code.replace(
    'for (size_t j = start_idx; j < i; j++) alias_type += std::string(tokens[j].value.value_or("")) + (tokens[j].type == lexer::TokenType::PUNCTUATION_DOT ? "" : " ");',
    'alias_type = extractTypeName(tokens, start_idx, i);'
)
code = code.replace(
    'for (size_t j = type_start; j < i; j++) type_name += std::string(tokens[j].value.value_or("")) + (tokens[j].type == lexer::TokenType::PUNCTUATION_DOT ? "" : " ");',
    'type_name = extractTypeName(tokens, type_start, i);'
)
code = code.replace(
    'for (size_t j = type_start; j < i; j++) return_type += std::string(tokens[j].value.value_or("")) + " ";',
    'return_type = extractTypeName(tokens, type_start, i);'
)
code = code.replace(
    'for (size_t k = param_start; k < i; k++) param.type += std::string(tokens[k].value.value_or("")) + " ";',
    'param.type = extractTypeName(tokens, param_start, i);'
)
code = code.replace(
    'for (size_t j = start; j < i; j++) type_name += std::string(tokens[j].value.value_or("")) + (tokens[j].type == lexer::TokenType::PUNCTUATION_DOT ? "" : " ");',
    'type_name = extractTypeName(tokens, start, i);'
)

# Replace ArrayCreationExpression parsing
bad_array = """ArrayCreationExpression::ArrayCreationExpression(const std::vector<lexer::Token>& tokens, Node* parent) : Node(tokens, NodeType::ARRAY_CREATION_EXPRESSION, parent) {
    // new array int32[5]
    if (tokens.size() < 5) throw_parse_error(tokens, "Invalid array creation syntax");
    if (tokens[0].type != lexer::TokenType::KEYWORD_NEW) throw_parse_error(tokens, "Expected 'new' for array creation");
    
    int open_idx = -1;
    for (int i = 1; i < tokens.size(); i++) {
        if (tokens[i].type == lexer::TokenType::PUNCTUATION_OPEN_BRACKET) {
            open_idx = i;
            break;
        }
    }
    
    if (open_idx <= 1) throw_parse_error(tokens, "Invalid array creation syntax");
    for (int i = 1; i < open_idx; i++) {
        if (tokens[i].type == lexer::TokenType::PRIMITIVE_ARRAY) continue;
        type_name += std::string(tokens[i].value.value_or("")) + " ";
    }
    while (!type_name.empty() && type_name.back() == ' ') type_name.pop_back();"""

good_array = """ArrayCreationExpression::ArrayCreationExpression(const std::vector<lexer::Token>& tokens, Node* parent) : Node(tokens, NodeType::ARRAY_CREATION_EXPRESSION, parent) {
    // new int32[5][]
    if (tokens.size() < 4) throw_parse_error(tokens, "Invalid array creation syntax");
    if (tokens[0].type != lexer::TokenType::KEYWORD_NEW) throw_parse_error(tokens, "Expected 'new' for array creation");
    
    int open_idx = -1;
    for (int i = 1; i < tokens.size(); i++) {
        if (tokens[i].type == lexer::TokenType::PUNCTUATION_OPEN_BRACKET) {
            open_idx = i;
            break;
        }
    }
    
    if (open_idx <= 1) throw_parse_error(tokens, "Invalid array creation syntax");
    type_name = extractTypeName(tokens, 1, open_idx);
    
    // Count trailing empty brackets for multidimensional array initialization like new int32[5][]
    for (int i = tokens.size() - 2; i > open_idx; i -= 2) {
        if (tokens[i].type == lexer::TokenType::PUNCTUATION_OPEN_BRACKET && tokens[i+1].type == lexer::TokenType::PUNCTUATION_CLOSE_BRACKET) {
            type_name += "[]";
        } else {
            break;
        }
    }"""
code = code.replace(bad_array, good_array)

with open("language/src/parser.cpp", "w") as f:
    f.write(code)

