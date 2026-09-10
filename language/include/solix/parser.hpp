#pragma once

#include "solix/lexer.hpp"
#include <filesystem>
#include <unordered_map>
#include <string_view>
#include <vector>
#include <string>
#include <memory>
#include <cstddef>

namespace solix::parser {

enum class NodeType {
    // Expressions
    ASSIGNMENT_EXPRESSION,
    BINARY_EXPRESSION,
    UNARY_EXPRESSION,
    LITERAL_EXPRESSION,
    IDENTIFIER_EXPRESSION,
    CALL_EXPRESSION,
    ARRAY_ACCESS_EXPRESSION,
    MEMBER_ACCESS_EXPRESSION,
    NEW_INSTANCE_EXPRESSION,
    ARRAY_CREATION_EXPRESSION,
    ARRAY_LITERAL_EXPRESSION, // For {1, 2, 3}
    CAST_EXPRESSION,
    TERNARY_EXPRESSION,

    // Top-Level Statements
    PACKAGE_STATEMENT,
    ALIAS_STATEMENT,
    ENUM_DECLARATION,
    CLASS_DECLARATION,

    // Class-Level Declarations
    FIELD_DECLARATION,
    METHOD_DECLARATION,
    CONSTRUCTOR_DECLARATION,

    // Control Flow
    BLOCK_STATEMENT,
    IF_STATEMENT,
    FOR_STATEMENT,
    WHILE_STATEMENT,
    DO_WHILE_STATEMENT,
    SWITCH_STATEMENT,
    CASE_STATEMENT,

    // Actions
    VARIABLE_DECLARATION,
    EXPRESSION_STATEMENT,
    RETURN_STATEMENT,
    BREAK_STATEMENT,
    CONTINUE_STATEMENT,
    
    // Base/Generic
    GENERIC
};


// ==========================================
// Base AST Node
// ==========================================
struct Node {
    NodeType node_type = NodeType::GENERIC;
    
    std::filesystem::path file_path;
    size_t line = 0;
    size_t column = 0;

    // --- Semantic Analyzer Resolved Data ---
    std::string resolved_type;
    int resolved_array_depth = 0;
    Node* resolved_declaration = nullptr;
    std::string symbol_name;

    // Pointer back to the parent node
    Node* parent_node = nullptr;
    
    // Child nodes (used by blocks, classes, functions, etc.)
    std::vector<std::unique_ptr<Node>> children;

    Node(const std::vector<lexer::Token>& tokens, NodeType type = NodeType::GENERIC, Node* parent = nullptr);
    virtual ~Node() = default;
};


// ==========================================
// Expressions
// ==========================================

struct AssignmentExpression : public Node {
    std::unique_ptr<Node> target; // What is being assigned to (L-Value)
    std::unique_ptr<Node> value;  // The value being assigned
    
    AssignmentExpression(const std::vector<lexer::Token>& tokens, Node* parent = nullptr);
};

struct BinaryExpression : public Node {
    std::unique_ptr<Node> left;
    lexer::TokenType op; // e.g., OPERATOR_PLUS
    std::unique_ptr<Node> right;
    
    BinaryExpression(const std::vector<lexer::Token>& tokens, Node* parent = nullptr);
};

struct UnaryExpression : public Node {
    lexer::TokenType op; // e.g., OPERATOR_LOGICAL_NOT
    std::unique_ptr<Node> operand;
    bool is_postfix = false; // true for counter++, false for !true
    
    UnaryExpression(const std::vector<lexer::Token>& tokens, Node* parent = nullptr);
};

struct LiteralExpression : public Node {
    lexer::Token token; // Holds the NUMBER, STRING, or BOOL token
    
    LiteralExpression(const std::vector<lexer::Token>& tokens, Node* parent = nullptr);
};

struct IdentifierExpression : public Node {
    std::string name;
    
    IdentifierExpression(const std::vector<lexer::Token>& tokens, Node* parent = nullptr);
};

struct CallExpression : public Node {
    std::unique_ptr<Node> callee; // usually an IdentifierExpression or MemberAccessExpression
    std::vector<std::unique_ptr<Node>> arguments;
    
    CallExpression(const std::vector<lexer::Token>& tokens, Node* parent = nullptr);
};

struct ArrayAccessExpression : public Node {
    std::unique_ptr<Node> array;
    std::unique_ptr<Node> index;
    
    ArrayAccessExpression(const std::vector<lexer::Token>& tokens, Node* parent = nullptr);
};

struct MemberAccessExpression : public Node {
    std::unique_ptr<Node> object;
    std::string member_name; // e.g., 'add' in MathUtils.add()
    
    MemberAccessExpression(const std::vector<lexer::Token>& tokens, Node* parent = nullptr);
};

struct NewInstanceExpression : public Node {
    std::string class_name;
    std::vector<std::unique_ptr<Node>> arguments;
    
    NewInstanceExpression(const std::vector<lexer::Token>& tokens, Node* parent = nullptr);
};

struct ArrayCreationExpression : public Node {
    std::string type_name;
    std::unique_ptr<Node> size; // e.g., the '5' in new array int32[5]
    
    ArrayCreationExpression(const std::vector<lexer::Token>& tokens, Node* parent = nullptr);
};

struct ArrayLiteralExpression : public Node {
    std::vector<std::unique_ptr<Node>> elements; // e.g. the 1, 2, 3 in {1, 2, 3}
    
    ArrayLiteralExpression(const std::vector<lexer::Token>& tokens, Node* parent = nullptr);
};

struct CastExpression : public Node {
    std::string target_type;
    std::unique_ptr<Node> expression; // The value being casted
    
    CastExpression(const std::vector<lexer::Token>& tokens, Node* parent = nullptr);
};

struct TernaryExpression : public Node {
    std::unique_ptr<Node> condition;
    std::unique_ptr<Node> true_branch;
    std::unique_ptr<Node> false_branch;
    
    TernaryExpression(const std::vector<lexer::Token>& tokens, Node* parent = nullptr);
};


// ==========================================
// Statements
// ==========================================

// 1. Global / Top-Level Declarations

struct PackageStatement : public Node {
    std::string package_name;
    
    PackageStatement(const std::vector<lexer::Token>& tokens, Node* parent = nullptr);
};

struct AliasStatement : public Node {
    std::string alias_name;
    std::string target_type; // e.g., "int32"
    
    AliasStatement(const std::vector<lexer::Token>& tokens, Node* parent = nullptr);
};

struct EnumDeclaration : public Node {
    std::string enum_name;
    lexer::TokenType access_modifier; // e.g., KEYWORD_PUBLIC
    std::vector<std::string> members; // e.g., "START", "STOP"
    
    EnumDeclaration(const std::vector<lexer::Token>& tokens, Node* parent = nullptr);
};

struct ClassDeclaration : public Node {
    std::string class_name;
    lexer::TokenType access_modifier; // e.g., KEYWORD_INTERNAL
    
    // Note: Methods and fields will be stored in the 'children' vector
    ClassDeclaration(const std::vector<lexer::Token>& tokens, Node* parent = nullptr);
};


// 2. Class-Level Declarations

struct FieldDeclaration : public Node {
    std::string field_name;
    std::string type_name;
    lexer::TokenType access_modifier; // e.g., KEYWORD_PRIVATE
    bool is_static = false;
    bool is_const = false;
    
    std::unique_ptr<Node> initializer; // The value assigned
    
    FieldDeclaration(const std::vector<lexer::Token>& tokens, Node* parent = nullptr);
};

struct ConstructorDeclaration : public Node {
    lexer::TokenType access_modifier;
    
    struct Parameter {
        std::string name;
        std::string type;
    };
    std::vector<Parameter> parameters;
    
    // Note: The constructor body (BlockStatement) will be stored in 'children'
    ConstructorDeclaration(const std::vector<lexer::Token>& tokens, Node* parent = nullptr);
};

struct MethodDeclaration : public Node {
    std::string method_name;
    std::string return_type;
    lexer::TokenType access_modifier;
    bool is_static = false;
    bool is_inline = false;
    
    struct Parameter {
        std::string name;
        std::string type;
    };
    std::vector<Parameter> parameters;
    
    // Note: The method body (BlockStatement) will be stored in 'children'
    MethodDeclaration(const std::vector<lexer::Token>& tokens, Node* parent = nullptr);
};


// 3. Block & Control Flow Statements

// Represents { ... }. The actual statements inside it are in the 'children' vector!
struct BlockStatement : public Node {
    BlockStatement(const std::vector<lexer::Token>& tokens, Node* parent = nullptr);
};

struct IfStatement : public Node {
    std::unique_ptr<Node> condition;
    std::unique_ptr<Node> then_branch; // usually a BlockStatement
    std::unique_ptr<Node> else_branch; // optional BlockStatement or another IfStatement
    
    IfStatement(const std::vector<lexer::Token>& tokens, Node* parent = nullptr);
};

struct ForStatement : public Node {
    std::unique_ptr<Node> initialization;
    std::unique_ptr<Node> condition;
    std::unique_ptr<Node> iteration;
    
    // The for-loop body
    std::unique_ptr<Node> body; 
    
    ForStatement(const std::vector<lexer::Token>& tokens, Node* parent = nullptr);
};

struct WhileStatement : public Node {
    std::unique_ptr<Node> condition;
    
    // The while-loop body
    std::unique_ptr<Node> body;
    
    WhileStatement(const std::vector<lexer::Token>& tokens, Node* parent = nullptr);
};

struct DoWhileStatement : public Node {
    std::unique_ptr<Node> body;
    std::unique_ptr<Node> condition;
    
    DoWhileStatement(const std::vector<lexer::Token>& tokens, Node* parent = nullptr);
};

struct SwitchStatement : public Node {
    std::unique_ptr<Node> condition;
    
    // Note: The CaseStatements are stored in 'children'
    SwitchStatement(const std::vector<lexer::Token>& tokens, Node* parent = nullptr);
};

struct CaseStatement : public Node {
    std::unique_ptr<Node> case_value; // null if this is a default: case
    bool is_default = false;
    
    // Note: The statements executed inside this case are stored in 'children'
    CaseStatement(const std::vector<lexer::Token>& tokens, Node* parent = nullptr);
};


// 4. Action Statements

struct VariableDeclaration : public Node {
    std::string var_name;
    bool is_const = false;

    std::string type_name;
    
    std::unique_ptr<Node> initializer; // optional
    
    VariableDeclaration(const std::vector<lexer::Token>& tokens, Node* parent = nullptr);
};

// Represents things like: counter = counter + 1; OR nums[0] = 5;
struct ExpressionStatement : public Node {
    std::unique_ptr<Node> expression;
    
    ExpressionStatement(const std::vector<lexer::Token>& tokens, Node* parent = nullptr);
};

struct ReturnStatement : public Node {
    std::unique_ptr<Node> value; // optional (null if 'return;')
    
    ReturnStatement(const std::vector<lexer::Token>& tokens, Node* parent = nullptr);
};

struct BreakStatement : public Node {
    BreakStatement(const std::vector<lexer::Token>& tokens, Node* parent = nullptr);
};

struct ContinueStatement : public Node {
    ContinueStatement(const std::vector<lexer::Token>& tokens, Node* parent = nullptr);
};

struct AstTree {
  // Memory ownership of all top-level statements/declarations
  std::vector<std::unique_ptr<Node>> nodes;

  // symbole tables for quick lookup of top-level declarations

  // A single global symbol table for all top-level declarations (Classes, Enums, Functions, Aliases, etc.)
  std::unordered_map<std::string, Node*> symbols;

  AstTree();

  void include(std::filesystem::path file_path);
  void include(std::string_view source_code, std::optional<std::filesystem::path> file_path = std::nullopt);

  // Resolves a variable or symbol by searching local scopes upwards, falling back to the global symbols map
  const Node* resolveDeclaration(const Node* current_scope, const std::string& name) const;
};

std::unique_ptr<Node> parseTokensToNode(const std::vector<lexer::Token>& tokens, Node* parent = nullptr);

} // namespace solix::parser
