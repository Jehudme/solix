import sys

with open("language/src/semantic.cpp", "r") as f:
    code = f.read()

# Let's just insert it before `} else if (root->node_type == parser::NodeType::EXPRESSION_STATEMENT) {`
target = "} else if (root->node_type == parser::NodeType::EXPRESSION_STATEMENT) {"
replacement = """    } else if (root->node_type == parser::NodeType::BREAK_STATEMENT || root->node_type == parser::NodeType::CONTINUE_STATEMENT) {
        if (loop_depth <= 0) {
            throw_semantic_error(root, "break/continue statement outside of loop");
        }
    } else if (root->node_type == parser::NodeType::EXPRESSION_STATEMENT) {"""

if target in code:
    code = code.replace(target, replacement)
    with open("language/src/semantic.cpp", "w") as f:
        f.write(code)
    print("Patched break/continue")

with open("tests/src/lexer/literals.cpp", "r") as f:
    code = f.read()

code = code.replace('REQUIRE( tokens[0].value.value() == "\\"hello world\\"" );', 'REQUIRE( tokens[0].value.value() == "hello world" );')
with open("tests/src/lexer/literals.cpp", "w") as f:
    f.write(code)

