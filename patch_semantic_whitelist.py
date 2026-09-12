import sys

with open("language/src/semantic.cpp", "r") as f:
    code = f.read()

unhandled_old = """               root->node_type == parser::NodeType::ARRAY_LITERAL_EXPRESSION ||
               root->node_type == parser::NodeType::PACKAGE_STATEMENT) {"""

unhandled_new = """               root->node_type == parser::NodeType::ARRAY_LITERAL_EXPRESSION ||
               root->node_type == parser::NodeType::PACKAGE_STATEMENT ||
               root->node_type == parser::NodeType::BLOCK_STATEMENT ||
               root->node_type == parser::NodeType::FIELD_DECLARATION ||
               root->node_type == parser::NodeType::CLASS_DECLARATION ||
               root->node_type == parser::NodeType::METHOD_DECLARATION ||
               root->node_type == parser::NodeType::CONSTRUCTOR_DECLARATION ||
               root->node_type == parser::NodeType::ENUM_DECLARATION ||
               root->node_type == parser::NodeType::ALIAS_STATEMENT ||
               root->node_type == parser::NodeType::FOR_STATEMENT ||
               root->node_type == parser::NodeType::WHILE_STATEMENT ||
               root->node_type == parser::NodeType::DO_WHILE_STATEMENT ||
               root->node_type == parser::NodeType::IF_STATEMENT ||
               root->node_type == parser::NodeType::SWITCH_STATEMENT ||
               root->node_type == parser::NodeType::CASE_STATEMENT ||
               root->node_type == parser::NodeType::VARIABLE_DECLARATION) {"""

if unhandled_old in code:
    code = code.replace(unhandled_old, unhandled_new)
    with open("language/src/semantic.cpp", "w") as f:
        f.write(code)
    print("Whitelisted node types")
else:
    print("Could not find unhandled nodes list")
