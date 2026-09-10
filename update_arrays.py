import re

# 1. semantic.cpp
with open("language/src/semantic.cpp", "r") as f:
    semantic = f.read()

bad = """    while (type_str.find("array ") == 0) {
        info.array_depth++;
        type_str = type_str.substr(6);
    }"""
good = """    while (type_str.length() >= 2 && type_str.substr(type_str.length() - 2) == "[]") {
        info.array_depth++;
        type_str = type_str.substr(0, type_str.length() - 2);
    }"""
semantic = semantic.replace(bad, good)

# Also fix TypeInfo::to_string()
bad_t = """    for (int i = 0; i < array_depth; i++) res += "array ";
    res += base_name;"""
good_t = """    res += base_name;
    for (int i = 0; i < array_depth; i++) res += "[]";"""
semantic = semantic.replace(bad_t, good_t)
with open("language/src/semantic.cpp", "w") as f:
    f.write(semantic)

# 2. lexer.hpp and lexer.cpp (remove array keyword)
with open("language/include/solix/lexer.hpp", "r") as f:
    lexer_hpp = f.read()
lexer_hpp = lexer_hpp.replace("    PRIMITIVE_STRING,\n    PRIMITIVE_ARRAY,", "    PRIMITIVE_STRING,")
with open("language/include/solix/lexer.hpp", "w") as f:
    f.write(lexer_hpp)

with open("language/src/lexer.cpp", "r") as f:
    lexer_cpp = f.read()
lexer_cpp = lexer_cpp.replace('        {"array", TokenType::PRIMITIVE_ARRAY},\n', '')
with open("language/src/lexer.cpp", "w") as f:
    f.write(lexer_cpp)

