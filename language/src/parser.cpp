#include "solix/parser.hpp"
#include "solix/lexer.hpp"
#include <stdexcept>
#include <iostream>

namespace solix::parser {

// ==========================================
// Error Handling & Helpers
// ==========================================

/*
 * throw_parse_error
 * 
 * ALGORITHM:
 * 1. Takes a `Token` and a `std::string` message.
 * 2. Extracts the exact file path, line number, and column number from the `Token` (or its `Span` metadata).
 * 3. Formats a highly professional, compiler-grade error message:
 *    "[Solix Parser Error] <file_path>:<line>:<col> - <msg> (At token: '<value>')"
 * 4. Throws a std::runtime_error with the formatted string.
 * Note: Provide an overloaded version that takes a vector of tokens, grabs the first token,
 * and calls the main throw_parse_error. Handle empty token vectors by throwing an "Unexpected EOF" error.
 */
[[noreturn]] void throw_parse_error(const lexer::Token& tok, const std::string& msg) {
    throw std::runtime_error("Parse Error: " + msg);
}
[[noreturn]] void throw_parse_error(const std::vector<lexer::Token>& tokens, const std::string& msg) {
    if (tokens.empty()) throw std::runtime_error("Parse Error: " + msg + " (Unexpected EOF)");
    throw_parse_error(tokens[0], msg);
}

/*
 * strip_parentheses (Helper)
 * ALGORITHM:
 * 1. Checks if the first token is `(` and the last token is `)`.
 * 2. If so, iterate through the tokens to ensure that these outer parentheses actually encapsulate 
 *    the *entire* sequence, meaning the parenthesis depth only drops to 0 at the very last token.
 * 3. If they wrap the whole expression, remove the first and last token, and recursively call strip_parentheses again.
 * 4. Return the stripped token vector.
 * ERROR DETECTION:
 * - If parenthesis depth goes negative at any point, throw "Mismatched parentheses: unexpected ')'".
 * - If parenthesis depth is > 0 at the end of the expression, throw "Mismatched parentheses: expected ')'".
 */

/*
 * divideTokensIntoStatements (Helper)
 * ALGORITHM:
 * 1. Iterate through a vector of tokens keeping track of scope depth (`{}` and `()`).
 * 2. Push tokens into a temporary current_statement buffer.
 * 3. If depth == 0 AND we hit a `;`, push current_statement into a list of statements and clear the buffer.
 * 4. If depth == 0 AND we hit a `}` (e.g., end of an `if` or `while` block), push current_statement into the list.
 * 5. Return the `std::vector<std::vector<lexer::Token>>` representing individual statements.
 * ERROR DETECTION:
 * - If depth != 0 at the end of iteration, throw "Mismatched braces or parentheses in block".
 */

/*
 * consumeType (Helper)
 * ALGORITHM:
 * 1. Takes a vector of tokens and a `start_index`.
 * 2. Eagerly consumes consecutive type-related keywords (like `array`).
 * 3. Consumes an identifier.
 * 4. Checks if the next token is a `.` (dot). If so, it consumes the dot and the following identifier,
 *    repeating this to build a full compound type (e.g., `Engine.CoreProcessor`).
 * 5. Returns the `end_index` pointing to the token immediately following the complete type.
 * ERROR DETECTION:
 * - If a dot `.` is found but not followed by an identifier, throw "Expected identifier after '.' in type".
 * - If `array` is found but not followed by a valid type, throw "Expected type after 'array' keyword".
 */
size_t consumeType(const std::vector<lexer::Token>& tokens, size_t start_index) {
    return start_index;
}

// ==========================================
// Core Parser Logic
// ==========================================

/*
 * determineNodeType
 * 
 * ALGORITHM (Order is critical):
 * 1. Call `strip_parentheses` on the input tokens to get a clean baseline.
 * 
 * 2. Top-Level Declarations Check:
 *    - Check `tokens[0].type` for `KEYWORD_PACKAGE`, `KEYWORD_ALIAS`.
 *    - Check if ANY token at depth 0 is `KEYWORD_ENUM` or `KEYWORD_CLASS`.
 * 
 * 3. Control Flow & Keyword Check:
 *    - Check `tokens[0].type` for `KEYWORD_IF`, `KEYWORD_FOR`, `KEYWORD_WHILE`, `KEYWORD_SWITCH`, 
 *      `KEYWORD_CASE`, `KEYWORD_DEFAULT`, `KEYWORD_BREAK`, `KEYWORD_CONTINUE`, `KEYWORD_RETURN`.
 *    - Check if `tokens[0].type` is `IDENTIFIER` with value "do" (for DoWhileStatement).
 * 
 * 4. Method & Constructor Declarations:
 *    - If the sequence ends with `}`, iterate backwards to find the matching `{`. 
 *    - Just before the `{`, verify there is a closing parenthesis `)`. Match it backwards to find `(`.
 *    - If there is an identifier before `(`, and optional type/modifiers before that, it's a Method/Constructor.
 *    - If there's a return type, it's `METHOD_DECLARATION`. Otherwise, `CONSTRUCTOR_DECLARATION`.
 * 
 * 5. Block Statement:
 *    - If `tokens[0] == '{'` and `tokens.back() == '}'`, it's `BLOCK_STATEMENT`.
 * 
 * 6. Variable Declaration / Field Declaration:
 *    - First, skip any access modifiers or specifiers (`public`, `static`, etc.).
 *    - Use `consumeType()` to fully parse the type (e.g., handling `array array float64` or `Engine.CoreProcessor`).
 *    - If the token immediately following the consumed type is an identifier, it is a declaration.
 *    - If it has an `=` or just ends with `;`.
 *    - Differentiate Field from Variable based on presence of modifiers or scope level.
 * 
 * 7. Expressions (Order of checking is critical for precedence):
 *    - Assignment: Search LEFT-TO-RIGHT at depth 0 for `=`. First one found -> `ASSIGNMENT_EXPRESSION`.
 *    - Ternary: Search RIGHT-TO-LEFT at depth 0 for `?` and `:`. -> `TERNARY_EXPRESSION`.
 *    - Binary: Search RIGHT-TO-LEFT at depth 0 for the operator with the LOWEST precedence (e.g., `+`, `-`, `*`, `/`).
 *      (Right-to-left ensures left-associativity). -> `BINARY_EXPRESSION`.
 *    - Unary: Check if `tokens[0]` is `!`, `-`, `++`, `--` OR if `tokens.back()` is `++`, `--`. -> `UNARY_EXPRESSION`.
 *    - Call: If `tokens.back() == ')'`, traverse backwards to match `(`. Tokens before `(` exist -> `CALL_EXPRESSION`.
 *    - Array Access: If `tokens.back() == ']'`, traverse backwards to match `[`. Tokens before `[` exist -> `ARRAY_ACCESS_EXPRESSION`.
 *    - Member Access: Search RIGHT-TO-LEFT at depth 0 for `.`. -> `MEMBER_ACCESS_EXPRESSION`.
 *    - New Instance: Check for `KEYWORD_NEW`. 
 *    - Array Literal: Starts with `{` and ends with `}`, contains commas at depth 0, no semicolons.
 *    - Cast: Check for `(Type) expression` pattern.
 *    - Literal/Identifier: If size == 1, check if NUMBER, STRING, BOOL ("true"/"false"), or IDENTIFIER.
 * 
 * 8. Fallback:
 *    - If `tokens.back()` is `;` and none of the above matched, return `EXPRESSION_STATEMENT`.
 *    - Otherwise, throw "Unable to determine AST Node Type".
 */
const NodeType determineNodeType(const std::vector<lexer::Token>& tokens) {
    return NodeType::GENERIC;
}

/*
 * parseTokensToNode
 * 
 * ALGORITHM:
 * 1. Check if `tokens` is empty. If so, return `nullptr`.
 * 2. Call `determineNodeType(tokens)`.
 * 3. Use a switch statement on the returned `NodeType`.
 * 4. Instantiate the matching node class using `std::make_unique<...>(tokens, parent)`.
 * 5. Return the resulting `unique_ptr<Node>`.
 * ERROR DETECTION:
 * - If `determineNodeType` throws, catch the error (optional) or let it bubble up, ensuring line numbers are preserved.
 */
std::unique_ptr<Node> parseTokensToNode(const std::vector<lexer::Token>& tokens, Node* parent) {
    return nullptr;
}


// ==========================================
// Expression Nodes
// ==========================================

/*
 * LiteralExpression
 * 1. Set `this->token` to `tokens[0]`.
 * ERROR DETECTION:
 * - If token is not a NUMBER, STRING, or BOOL, throw "Expected literal value".
 */
LiteralExpression::LiteralExpression(const std::vector<lexer::Token>& tokens, Node* parent) : Node(tokens, NodeType::LITERAL_EXPRESSION, parent) {}

/*
 * IdentifierExpression
 * 1. Set `this->token` to `tokens[0]`.
 * ERROR DETECTION:
 * - If token is not an IDENTIFIER, throw "Expected identifier".
 */
IdentifierExpression::IdentifierExpression(const std::vector<lexer::Token>& tokens, Node* parent) : Node(tokens, NodeType::IDENTIFIER_EXPRESSION, parent) {}

/*
 * BinaryExpression
 * 1. Iterate RIGHT-TO-LEFT at depth 0 (tracking parens/braces).
 * 2. Identify the operator with the LOWEST precedence (e.g., `+` is lower than `*`).
 * 3. Set `this->op` to that token's type.
 * 4. Split tokens into `left_tokens` and `right_tokens`.
 * 5. Assign `this->left = parseTokensToNode(left_tokens, this)`.
 * 6. Assign `this->right = parseTokensToNode(right_tokens, this)`.
 * ERROR DETECTION:
 * - If `left_tokens` is empty, throw "Missing left operand for binary operator".
 * - If `right_tokens` is empty, throw "Missing right operand for binary operator".
 */
BinaryExpression::BinaryExpression(const std::vector<lexer::Token>& tokens, Node* parent) : Node(tokens, NodeType::BINARY_EXPRESSION, parent) {}

/*
 * UnaryExpression
 * 1. Check if `tokens[0]` is an operator (`!`, `-`, `++`, `--`). If so, `is_postfix = false`, `op = tokens[0].type`.
 * 2. Otherwise, check `tokens.back()`. If operator, `is_postfix = true`, `op = tokens.back().type`.
 * 3. Extract the remaining tokens (the operand).
 * 4. Assign `this->operand = parseTokensToNode(remaining_tokens, this)`.
 * ERROR DETECTION:
 * - If remaining operand tokens are empty, throw "Missing operand for unary operator".
 */
UnaryExpression::UnaryExpression(const std::vector<lexer::Token>& tokens, Node* parent) : Node(tokens, NodeType::UNARY_EXPRESSION, parent) {}

/*
 * AssignmentExpression
 * 1. Iterate LEFT-TO-RIGHT at depth 0.
 * 2. Find the FIRST `=` operator. (Left-to-right ensures proper right-associativity grouping).
 * 3. Split tokens into `target_tokens` (left of `=`) and `value_tokens` (right of `=`).
 * 4. `this->target = parseTokensToNode(target_tokens, this)`.
 * 5. `this->value = parseTokensToNode(value_tokens, this)`.
 * ERROR DETECTION:
 * - If `target_tokens` is empty, throw "Missing assignment target (L-Value)".
 * - If `value_tokens` is empty, throw "Missing expression value on right side of assignment".
 */
AssignmentExpression::AssignmentExpression(const std::vector<lexer::Token>& tokens, Node* parent) : Node(tokens, NodeType::ASSIGNMENT_EXPRESSION, parent) {}

/*
 * CallExpression
 * 1. Find the LAST `)` at depth 0.
 * 2. Traverse backwards to find the matching `(`.
 * 3. The tokens before `(` are the `callee`. `this->callee = parseTokensToNode(callee_tokens, this)`.
 * 4. The tokens inside `()` are the arguments.
 * 5. Split the argument tokens by `,` at depth 0.
 * 6. For each arg token sequence, call `parseTokensToNode()` and push to `this->arguments`.
 * ERROR DETECTION:
 * - If `callee_tokens` is empty, throw "Missing target for function call".
 * - If splitting by `,` results in an empty sequence between commas (e.g., `foo(a, , b)`), throw "Empty argument in function call".
 */
CallExpression::CallExpression(const std::vector<lexer::Token>& tokens, Node* parent) : Node(tokens, NodeType::CALL_EXPRESSION, parent) {}

/*
 * ArrayAccessExpression
 * 1. Find the LAST `]` at depth 0. Match it backwards to `[`.
 * 2. Tokens before `[` are the `array` target. Parse it.
 * 3. Tokens inside `[]` are the `index`. Parse it.
 * ERROR DETECTION:
 * - If `array` target tokens are empty, throw "Missing array target for index access".
 * - If `index` tokens are empty (e.g., `arr[]`), throw "Missing index expression in array access".
 */
ArrayAccessExpression::ArrayAccessExpression(const std::vector<lexer::Token>& tokens, Node* parent) : Node(tokens, NodeType::ARRAY_ACCESS_EXPRESSION, parent) {}

/*
 * MemberAccessExpression
 * 1. Find the LAST `.` at depth 0.
 * 2. The single token to the right of `.` is the `member_name`.
 * 3. The tokens to the left of `.` form the `object`. Parse it.
 * ERROR DETECTION:
 * - If tokens right of `.` do not form exactly one valid identifier, throw "Expected single identifier after '.'".
 * - If `object` tokens are empty, throw "Missing object reference before '.'".
 */
MemberAccessExpression::MemberAccessExpression(const std::vector<lexer::Token>& tokens, Node* parent) : Node(tokens, NodeType::MEMBER_ACCESS_EXPRESSION, parent) {}

/*
 * NewInstanceExpression
 * 1. `tokens[0]` is `KEYWORD_NEW`.
 * 2. Find the last `(...)`. The tokens before it (excluding `new`) are the `class_name`.
 * 3. Tokens inside `(...)` are split by `,` and parsed into `this->arguments`.
 * ERROR DETECTION:
 * - If `class_name` tokens are empty, throw "Missing class name after 'new'".
 * - If argument splitting reveals empty segments, throw "Empty argument in constructor call".
 */
NewInstanceExpression::NewInstanceExpression(const std::vector<lexer::Token>& tokens, Node* parent) : Node(tokens, NodeType::NEW_INSTANCE_EXPRESSION, parent) {}

/*
 * ArrayCreationExpression
 * 1. `tokens[0]` is `KEYWORD_NEW`.
 * 2. Find the `[...]`. Tokens before it (excluding `new`) are the `type_name`.
 * 3. Tokens inside `[...]` are parsed into `this->size`.
 * ERROR DETECTION:
 * - If `type_name` is empty, throw "Missing type for array creation".
 * - If `size` tokens are empty, throw "Array size expression cannot be empty".
 */
ArrayCreationExpression::ArrayCreationExpression(const std::vector<lexer::Token>& tokens, Node* parent) : Node(tokens, NodeType::ARRAY_CREATION_EXPRESSION, parent) {}

/*
 * ArrayLiteralExpression
 * 1. Verify `tokens[0]` is `{` and `tokens.back()` is `}`.
 * 2. Extract inner tokens, split by `,` at depth 0.
 * 3. Parse each segment and push to `this->elements`.
 * ERROR DETECTION:
 * - If segment between commas is empty (except trailing comma support, if language allows), throw "Empty element in array literal".
 */
ArrayLiteralExpression::ArrayLiteralExpression(const std::vector<lexer::Token>& tokens, Node* parent) : Node(tokens, NodeType::ARRAY_LITERAL_EXPRESSION, parent) {}

CastExpression::CastExpression(const std::vector<lexer::Token>& tokens, Node* parent) : Node(tokens, NodeType::CAST_EXPRESSION, parent) {}
TernaryExpression::TernaryExpression(const std::vector<lexer::Token>& tokens, Node* parent) : Node(tokens, NodeType::TERNARY_EXPRESSION, parent) {}


// ==========================================
// Top-Level Statements
// ==========================================

/*
 * PackageStatement
 * 1. Skip `package`. Concatenate remaining identifiers/dots into `this->package_name`.
 * ERROR DETECTION:
 * - If no tokens after `package`, throw "Expected package name".
 * - If package name is malformed (e.g., consecutive dots), throw "Invalid package name format".
 */
PackageStatement::PackageStatement(const std::vector<lexer::Token>& tokens, Node* parent) : Node(tokens, NodeType::PACKAGE_STATEMENT, parent) {}


/*
 * AliasStatement
 * 1. Skip `alias`. `this->alias_name = tokens[1]`.
 * 2. Skip `=`. `this->target_type = tokens[3]`.
 * ERROR DETECTION:
 * - If missing `=`, throw "Expected '=' in alias declaration".
 * - If missing target type, throw "Expected target type after '='".
 */
AliasStatement::AliasStatement(const std::vector<lexer::Token>& tokens, Node* parent) : Node(tokens, NodeType::ALIAS_STATEMENT, parent) {}

/*
 * EnumDeclaration
 * 1. Extract modifiers (`public`, `internal`, etc.). Assign to `access_modifier`.
 * 2. Skip `enum`. Next token is `enum_name`.
 * 3. Extract tokens inside `{ ... }`.
 * 4. Split by `,` and push each identifier to `members`.
 * ERROR DETECTION:
 * - If missing `enum_name`, throw "Expected identifier for enum name".
 * - If block is missing or malformed, throw "Expected '{' for enum body".
 * - If any member is not a single identifier, throw "Enum members must be valid identifiers".
 */
EnumDeclaration::EnumDeclaration(const std::vector<lexer::Token>& tokens, Node* parent) : Node(tokens, NodeType::ENUM_DECLARATION, parent) {}

/*
 * ClassDeclaration
 * 1. Extract modifiers up to `class`. Assign to `access_modifier`.
 * 2. Token after `class` is `class_name`.
 * 3. Extract tokens inside `{ ... }`.
 * 4. Use `divideTokensIntoStatements` to split the class body into distinct field/method declarations.
 * 5. For each statement, call `parseTokensToNode()` and push the result to `this->children`.
 * ERROR DETECTION:
 * - If `class_name` is missing, throw "Expected identifier for class name".
 * - If body is missing, throw "Expected '{' for class body".
 */
ClassDeclaration::ClassDeclaration(const std::vector<lexer::Token>& tokens, Node* parent) : Node(tokens, NodeType::CLASS_DECLARATION, parent) {}


// ==========================================
// Class-Level Declarations
// ==========================================

/*
 * FieldDeclaration
 * 1. Extract modifiers (`public`, `static`, `const`). Set corresponding fields.
 * 2. Extract `type_name` and `field_name`.
 * 3. STRICT RULE: Iterate through `parent->children` to ensure no sibling shares this exact `field_name`. Throw duplicate error if found.
 * 4. If `=` exists, everything after it (up to `;`) is passed to `parseTokensToNode` as `initializer`.
 * ERROR DETECTION:
 * - If missing `type_name` or `field_name`, throw "Invalid field declaration syntax".
 * - If initializer tokens are empty but `=` is present, throw "Missing expression after '='".
 * - If parent is not a ClassDeclaration, throw "Fields must be declared inside a class".
 */
FieldDeclaration::FieldDeclaration(const std::vector<lexer::Token>& tokens, Node* parent) : Node(tokens, NodeType::FIELD_DECLARATION, parent) {}

/*
 * ConstructorDeclaration
 * 1. Extract modifiers.
 * 2. Find `(...)`. Split contents by `,` at depth 0. For each segment, extract Type and Name. Push to `parameters`.
 * 3. Find `{...}`. Extract contents.
 * 4. Pass body contents to `parseTokensToNode()` (which should yield a BlockStatement). Push to `children`.
 * ERROR DETECTION:
 * - If constructor name does not match class name, throw "Constructor name must match class name".
 * - If parameter segment is malformed (missing type or name), throw "Invalid parameter syntax".
 * - If body `{...}` is missing, throw "Constructors must have a body".
 */
ConstructorDeclaration::ConstructorDeclaration(const std::vector<lexer::Token>& tokens, Node* parent) : Node(tokens, NodeType::CONSTRUCTOR_DECLARATION, parent) {}

/*
 * MethodDeclaration
 * 1. Extract modifiers, set flags (`is_static`, `is_inline`, access modifiers).
 * 2. Type token before the identifier is `return_type`. The identifier before `(` is `method_name`.
 * 3. STRICT RULE: Check if `parent` is a `ClassDeclaration`. If not, call `throw_parse_error` because Solix methods MUST be inside a class.
 * 4. STRICT RULE: Iterate through `parent->children` to ensure no sibling shares this exact `method_name` (unless supporting overloading, in which case check signatures). Throw a duplicate identifier error if found.
 * 5. Parse parameters inside `(...)` just like Constructor.
 * 6. Parse body inside `{...}` into a BlockStatement, push to `children`.
 * ERROR DETECTION:
 * - If `return_type` or `method_name` are missing, throw "Invalid method declaration syntax".
 * - If body `{...}` is missing (unless in interface/abstract), throw "Method must have a body".
 * - If parameter segment is malformed, throw "Invalid parameter syntax".
 */
MethodDeclaration::MethodDeclaration(const std::vector<lexer::Token>& tokens, Node* parent) : Node(tokens, NodeType::METHOD_DECLARATION, parent) {}


// ==========================================
// Control Flow & Statements
// ==========================================

/*
 * BlockStatement
 * 1. Ensure starts with `{` and ends with `}`. Strip them.
 * 2. Use `divideTokensIntoStatements` on inner tokens.
 * 3. Iterate through statement token vectors. Call `parseTokensToNode` on each.
 * 4. Push resulting nodes to `this->children`.
 * ERROR DETECTION:
 * - If not starting with `{` or not ending with `}`, throw "Block must be enclosed in braces".
 */
BlockStatement::BlockStatement(const std::vector<lexer::Token>& tokens, Node* parent) : Node(tokens, NodeType::BLOCK_STATEMENT, parent) {}

/*
 * IfStatement
 * 1. `tokens[0]` is `if`.
 * 2. Extract condition from the first `(...)`. Parse and assign to `condition`.
 * 3. Find the `then` body (could be a block `{...}` or a single statement up to `;`). Parse and assign to `then_branch`.
 * 4. If there are tokens remaining, check if they start with `else`.
 * 5. If `else` is followed immediately by `if`, extract everything from `if` to the end, parse it (it will become another IfStatement), and assign to `else_branch`.
 * 6. Otherwise, extract the `else` body (block or single statement), parse, and assign to `else_branch`.
 * ERROR DETECTION:
 * - If condition `(...)` is missing or empty, throw "Expected condition in if statement".
 * - If `then` branch is missing, throw "Expected body for if statement".
 * - If `else` keyword is present but no branch follows, throw "Expected body after else".
 */
IfStatement::IfStatement(const std::vector<lexer::Token>& tokens, Node* parent) : Node(tokens, NodeType::IF_STATEMENT, parent) {}

/*
 * ForStatement
 * 1. `tokens[0]` is `for`.
 * 2. Extract contents of first `(...)`.
 * 3. Split contents precisely by `;` at depth 0 into 3 parts: `init`, `cond`, `iter`.
 * 4. Parse each non-empty part and assign to `initialization`, `condition`, `iteration`.
 * 5. Extract the remaining tokens as the body (block or single statement). Parse and assign to `body`.
 * ERROR DETECTION:
 * - If `(...)` does not contain exactly two semicolons, throw "Invalid for-loop syntax: expected two semicolons".
 * - If body is missing, throw "Expected body for for-loop".
 */
ForStatement::ForStatement(const std::vector<lexer::Token>& tokens, Node* parent) : Node(tokens, NodeType::FOR_STATEMENT, parent) {}

/*
 * WhileStatement
 * 1. `tokens[0]` is `while`.
 * 2. Extract condition from `(...)`. Parse to `condition`.
 * 3. Extract body. Parse to `body`.
 * ERROR DETECTION:
 * - If condition `(...)` is missing or empty, throw "Expected condition in while statement".
 * - If body is missing, throw "Expected body for while statement".
 */
WhileStatement::WhileStatement(const std::vector<lexer::Token>& tokens, Node* parent) : Node(tokens, NodeType::WHILE_STATEMENT, parent) {}

/*
 * DoWhileStatement
 * 1. `tokens[0]` is `do`.
 * 2. Extract the block body. Parse to `body`.
 * 3. The token after the block is `while`. Extract condition from `(...)` after it. Parse to `condition`.
 * ERROR DETECTION:
 * - If `while` is missing after the block, throw "Expected 'while' after do block".
 * - If condition `(...)` is missing, throw "Expected condition in do-while statement".
 */
DoWhileStatement::DoWhileStatement(const std::vector<lexer::Token>& tokens, Node* parent) : Node(tokens, NodeType::DO_WHILE_STATEMENT, parent) {}

/*
 * SwitchStatement
 * 1. `tokens[0]` is `switch`.
 * 2. Extract condition from `(...)`. Parse to `condition`.
 * 3. Extract inner block `{...}`. Use `divideTokensIntoStatements` to split the inner block into case statements.
 * 4. Parse each case statement and push to `children`.
 * ERROR DETECTION:
 * - If condition is empty, throw "Expected condition in switch statement".
 * - If block `{}` is missing, throw "Expected block for switch statement".
 */
SwitchStatement::SwitchStatement(const std::vector<lexer::Token>& tokens, Node* parent) : Node(tokens, NodeType::SWITCH_STATEMENT, parent) {}

/*
 * CaseStatement
 * 1. Check if `tokens[0]` is `case` or `default`.
 * 2. If `case`, extract tokens between `case` and `:` as `case_value`. Parse it.
 * 3. The tokens after `:` are the body. Use `divideTokensIntoStatements` to split them.
 * 4. Parse each body statement and push to `children`.
 * ERROR DETECTION:
 * - If missing `:`, throw "Expected ':' after case or default".
 * - If `case` has no value before `:`, throw "Expected value for case".
 */
CaseStatement::CaseStatement(const std::vector<lexer::Token>& tokens, Node* parent) : Node(tokens, NodeType::CASE_STATEMENT, parent) {}

/*
 * VariableDeclaration
 * 1. Extract `type_name` and `var_name`.
 * 2. STRICT RULE: Iterate through `parent->children` to ensure no sibling shares this exact `var_name`. If found, throw a Duplicate Identifier error.
 * 3. If `=` is present, parse the right side (up to `;`) as `initializer`.
 * ERROR DETECTION:
 * - If `type_name` or `var_name` is missing, throw "Invalid variable declaration syntax".
 * - If `=` is present but right side is empty, throw "Expected expression after '='".
 */
VariableDeclaration::VariableDeclaration(const std::vector<lexer::Token>& tokens, Node* parent) : Node(tokens, NodeType::VARIABLE_DECLARATION, parent) {}

/*
 * ExpressionStatement
 * 1. Strip the trailing `;`.
 * 2. Pass the remaining tokens to `parseTokensToNode`.
 * 3. Assign the resulting node to `expression`.
 * ERROR DETECTION:
 * - If trailing `;` is missing, throw "Expected ';' at end of expression statement".
 * - If tokens before `;` are empty, throw "Empty expression statement".
 */
ExpressionStatement::ExpressionStatement(const std::vector<lexer::Token>& tokens, Node* parent) : Node(tokens, NodeType::EXPRESSION_STATEMENT, parent) {}

/*
 * ReturnStatement
 * 1. Verify `tokens[0]` is `return`.
 * 2. If length > 2 (i.e. not just `return;`), extract tokens between `return` and `;`.
 * 3. Parse them and assign to `value`.
 * ERROR DETECTION:
 * - If missing trailing `;`, throw "Expected ';' at end of return statement".
 */
ReturnStatement::ReturnStatement(const std::vector<lexer::Token>& tokens, Node* parent) : Node(tokens, NodeType::RETURN_STATEMENT, parent) {}

/*
 * Break/Continue
 * 1. Verify token is `break` or `continue`. No children required.
 * ERROR DETECTION:
 * - If missing trailing `;`, throw "Expected ';' at end of statement".
 * - If not inside a loop (or switch for break), throw "Break/Continue outside of allowed control flow" (requires parent traversal check).
 */
BreakStatement::BreakStatement(const std::vector<lexer::Token>& tokens, Node* parent) : Node(tokens, NodeType::BREAK_STATEMENT, parent) {}
ContinueStatement::ContinueStatement(const std::vector<lexer::Token>& tokens, Node* parent) : Node(tokens, NodeType::CONTINUE_STATEMENT, parent) {}


// ==========================================
// AstTree & Helper Implementation
// ==========================================

AstTree::AstTree() {}

/*
 * AstTree::resolveDeclaration
 * 
 * ALGORITHM:
 * 1. Take a `current_scope` pointer and a `name` string.
 * 2. Track a `previous_node` pointer (initially `nullptr`).
 * 3. Loop `while (current_scope != nullptr)`:
 *    a. If `current_scope` is a `BlockStatement`:
 *       - Loop through its `children`.
 *       - Stop searching if we reach a child where `child == previous_node` 
 *         (meaning we shouldn't resolve variables declared *after* our current line).
 *       - Check if the child is a VariableDeclaration. If its `var_name` == `name`, return it.
 *    b. If `current_scope` is a `MethodDeclaration` or `ConstructorDeclaration`:
 *       - Check its `parameters`. (We will need to ensure parameters can be queried by name).
 *    c. If `current_scope` is a `ClassDeclaration`:
 *       - Loop through ALL its `children` (order doesn't matter for classes).
 *       - Check FieldDeclarations and MethodDeclarations for a name match. Return if found.
 *    d. `previous_node = current_scope;`
 *    e. `current_scope = current_scope->parent;` // Move up to the next scope.
 * 4. If the loop finishes without returning, check the `symbols` unordered_map (global scope).
 *    - To check the global scope, we search the `symbols` map for the fully qualified name.
 *    - `auto it = symbols.find(name); if (it != symbols.end()) return it->second;`
 * 5. Return `nullptr` if not found anywhere.
 * ERROR DETECTION:
 * - If resolution is specifically required for compilation but returns nullptr, the calling site must throw "Undefined symbol: <name>".
 */
const Node* AstTree::resolveDeclaration(const Node* current_scope, const std::string& name) const {
    return nullptr;
}

/*
 * generateUniqueSymbolName (Helper for AstTree)
 * ALGORITHM:
 * 1. Takes a Node and its name.
 * 2. Traverses up the `parent` pointers to build a fully qualified name string.
 * 3. Prepend the package name, class name, etc. separated by dots or colons (e.g., `math.utils.MathUtils.add`).
 * 4. For method overloads, append parameter types (e.g., `MathUtils.add(int32,int32)`).
 * 5. Return the resulting unique string.
 */

/*
 * AstTree::include
 * 
 * ALGORITHM:
 * 1. Tokenize the source code using the lexer.
 * 2. Use `divideTokensIntoStatements` to split the top-level tokens.
 * 3. For each top-level statement vector, call `parseTokensToNode`.
 * 4. Push the resulting Node to the `nodes` vector.
 * 5. Recursively walk the newly parsed tree to find all Global Declarations:
 *    - `PackageStatement`, `ClassDeclaration`, `EnumDeclaration`, `AliasStatement`
 *    - `MethodDeclaration` (global or static methods)
 *    - `VariableDeclaration` or `FieldDeclaration` (if marked static/global)
 * 6. For each found declaration, generate a unique symbol key using `generateUniqueSymbolName`.
 * 7. Check if the unique key already exists in `symbols`. If it does, throw a "Duplicate Symbol" error.
 * 8. Otherwise, insert it into the `symbols` unordered_map for O(1) global resolution.
 * ERROR DETECTION:
 * - Check if the lexer threw any errors, bubble them up.
 * - Throw "Duplicate global symbol: <name>" if key insertion fails.
 */
void AstTree::include(std::filesystem::path file_path) {}
void AstTree::include(std::string_view source_code, std::optional<std::filesystem::path> file_path) {}

} // namespace solix::parser
