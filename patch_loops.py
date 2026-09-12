import sys

with open("language/src/semantic.cpp", "r") as f:
    code = f.read()

for_old = """    } else if (root->node_type == parser::NodeType::FOR_STATEMENT) {
        auto for_statement = static_cast<parser::ForStatement*>(root);
        if (for_statement->initialization) resolveAndCheck(tree, for_statement->initialization.get());
        if (for_statement->condition) evaluateExpression(tree, for_statement->condition.get());
        if (for_statement->iteration) evaluateExpression(tree, for_statement->iteration.get());
        if (for_statement->body) resolveAndCheck(tree, for_statement->body.get());
    }"""
for_new = """    } else if (root->node_type == parser::NodeType::FOR_STATEMENT) {
        auto for_statement = static_cast<parser::ForStatement*>(root);
        if (for_statement->initialization) resolveAndCheck(tree, for_statement->initialization.get());
        if (for_statement->condition) evaluateExpression(tree, for_statement->condition.get());
        if (for_statement->iteration) evaluateExpression(tree, for_statement->iteration.get());
        loop_depth++;
        if (for_statement->body) resolveAndCheck(tree, for_statement->body.get());
        loop_depth--;
    }"""
code = code.replace(for_old, for_new)

while_old = """    } else if (root->node_type == parser::NodeType::WHILE_STATEMENT) {
        auto while_statement = static_cast<parser::WhileStatement*>(root);
        evaluateExpression(tree, while_statement->condition.get());
        if (while_statement->body) resolveAndCheck(tree, while_statement->body.get());
    }"""
while_new = """    } else if (root->node_type == parser::NodeType::WHILE_STATEMENT) {
        auto while_statement = static_cast<parser::WhileStatement*>(root);
        evaluateExpression(tree, while_statement->condition.get());
        loop_depth++;
        if (while_statement->body) resolveAndCheck(tree, while_statement->body.get());
        loop_depth--;
    }"""
code = code.replace(while_old, while_new)

do_while_old = """    } else if (root->node_type == parser::NodeType::DO_WHILE_STATEMENT) {
        auto do_while_statement = static_cast<parser::DoWhileStatement*>(root);
        if (do_while_statement->body) resolveAndCheck(tree, do_while_statement->body.get());
        evaluateExpression(tree, do_while_statement->condition.get());
    }"""
do_while_new = """    } else if (root->node_type == parser::NodeType::DO_WHILE_STATEMENT) {
        auto do_while_statement = static_cast<parser::DoWhileStatement*>(root);
        loop_depth++;
        if (do_while_statement->body) resolveAndCheck(tree, do_while_statement->body.get());
        loop_depth--;
        evaluateExpression(tree, do_while_statement->condition.get());
    }"""
code = code.replace(do_while_old, do_while_new)

with open("language/src/semantic.cpp", "w") as f:
    f.write(code)

