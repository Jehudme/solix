#pragma once
#include "solix/ast_visitor.hpp"
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include "solix/utilities/token.hpp"

namespace solix {

enum class NodeType {
    UNKNOWN,
    
    // Expressions
    IDENTIFIER, LITERAL, BINARY_EXPR, UNARY_EXPR, ASSIGNMENT_EXPR,
    ARRAY_ACCESS, MEMBER_ACCESS, METHOD_CALL, NEW_INSTANCE, ARRAY_CREATION,
    ARRAY_LITERAL, CAST_EXPR,
    INSTANCEOF_EXPR, TERNARY_EXPR,

    // Statements
    BLOCK, IF_STMT, FOR_STMT, WHILE_STMT, DO_WHILE_STMT, SWITCH_STMT, CASE_STMT,
    VAR_DECL, EXPR_STMT, RETURN_STMT, BREAK_STMT, CONTINUE_STMT,

    // Top Level
    PACKAGE_STMT, ALIAS_STMT, ENUM_DECL, CLASS_DECL, FIELD_DECL, CONSTRUCTOR_DECL, METHOD_DECL
};

struct TypeInfo {
    std::string name;
    int array_depth = 0;
    std::vector<TypeInfo> type_args;
    bool operator==(const TypeInfo& other) const {
        if (name != other.name || array_depth != other.array_depth || type_args.size() != other.type_args.size()) return false;
        for (size_t i = 0; i < type_args.size(); i++) if (type_args[i] != other.type_args[i]) return false;
        return true;
    }
    bool operator!=(const TypeInfo& other) const {
        return !(*this == other);
    }
    std::string to_string() const {
        std::string res = name;
        if (!type_args.empty()) {
            res += "<";
            for (size_t i = 0; i < type_args.size(); i++) {
                res += type_args[i].to_string();
                if (i < type_args.size() - 1) res += ",";
            }
            res += ">";
        }
        for (int i = 0; i < array_depth; i++) res += "[]";
        return res;
    }
};

struct Node {
    NodeType node_type;
    uint32_t line;
    uint32_t column;
    const Source* source;
    
    Node* parent = nullptr;
    // --- Semantic Binding Fields ---
    TypeInfo expression_type;
    Node* resolved_declaration = nullptr;
    int memory_index = -1;
    std::string mangled_name = "";
    int instance_size = 0;
    bool is_primitive = false;
    bool is_reference_type = false;
    bool is_weak = false;
    std::string package_context = "";  // Stamped during Pass 1 to preserve namespace context
    std::vector<std::unique_ptr<Node>> children;

    Node(NodeType type, const Token& token) 
        : node_type(type), line(token.line), column(token.column), source(token.source) {}
    virtual ~Node() = default;
    virtual void accept(NodeVisitor& v) = 0;
    virtual std::unique_ptr<Node> clone() const = 0;
};

// ==========================================
// Expressions
// ==========================================

struct IdentifierNode : public Node {
    std::unique_ptr<Node> clone() const override;

    void accept(NodeVisitor& v) override { v.visit(*this); }

    std::string name;
    IdentifierNode(const Token& t, std::string n) : Node(NodeType::IDENTIFIER, t), name(std::move(n)) {}
};

struct LiteralNode : public Node {
    std::unique_ptr<Node> clone() const override;

    void accept(NodeVisitor& v) override { v.visit(*this); }

    Value value;
    LiteralNode(const Token& t, Value v) : Node(NodeType::LITERAL, t), value(std::move(v)) {}
};

struct BinaryExpression : public Node {
    std::unique_ptr<Node> clone() const override;

    void accept(NodeVisitor& v) override { v.visit(*this); }

    std::unique_ptr<Node> left;
    Node* overloaded_operator = nullptr;
    TokenType op;
    std::unique_ptr<Node> right;
    BinaryExpression(const Token& t, std::unique_ptr<Node> l, TokenType o, std::unique_ptr<Node> r) 
        : Node(NodeType::BINARY_EXPR, t), left(std::move(l)), op(o), right(std::move(r)) {
            if(left) left->parent = this;
            if(right) right->parent = this;
        }
};

struct UnaryExpression : public Node {
    std::unique_ptr<Node> clone() const override;

    void accept(NodeVisitor& v) override { v.visit(*this); }

    Node* overloaded_operator = nullptr;
    TokenType op;
    std::unique_ptr<Node> operand;
    bool is_prefix;
    UnaryExpression(const Token& t, TokenType o, std::unique_ptr<Node> opnd, bool prefix)
        : Node(NodeType::UNARY_EXPR, t), op(o), operand(std::move(opnd)), is_prefix(prefix) {
            if(operand) operand->parent = this;
        }
};

struct AssignmentExpression : public Node {
    std::unique_ptr<Node> clone() const override;

    void accept(NodeVisitor& v) override { v.visit(*this); }

    std::unique_ptr<Node> target;
    Node* overloaded_operator = nullptr;
    TokenType op;
    std::unique_ptr<Node> value;
    AssignmentExpression(const Token& t, std::unique_ptr<Node> tgt, TokenType o, std::unique_ptr<Node> val)
        : Node(NodeType::ASSIGNMENT_EXPR, t), target(std::move(tgt)), op(o), value(std::move(val)) {
            if(target) target->parent = this;
            if(value) value->parent = this;
        }
};

struct ArrayAccessExpression : public Node {
    std::unique_ptr<Node> clone() const override;

    void accept(NodeVisitor& v) override { v.visit(*this); }

    std::unique_ptr<Node> array;
    std::unique_ptr<Node> index;
    ArrayAccessExpression(const Token& t, std::unique_ptr<Node> arr, std::unique_ptr<Node> idx)
        : Node(NodeType::ARRAY_ACCESS, t), array(std::move(arr)), index(std::move(idx)) {
            if(array) array->parent = this;
            if(index) index->parent = this;
        }
};

struct MemberAccessExpression : public Node {
    std::unique_ptr<Node> clone() const override;

    void accept(NodeVisitor& v) override { v.visit(*this); }

    std::unique_ptr<Node> object;
    std::string member_name;
    bool is_scope_resolution = false;
    int32_t enum_value = -1;
    MemberAccessExpression(const Token& t, std::unique_ptr<Node> obj, std::string mem)
        : Node(NodeType::MEMBER_ACCESS, t), object(std::move(obj)), member_name(std::move(mem)) {
            if(object) object->parent = this;
        }
};

struct MethodCallExpression : public Node {
    std::unique_ptr<Node> clone() const override;

    void accept(NodeVisitor& v) override { v.visit(*this); }

    std::unique_ptr<Node> callee;
    std::vector<std::unique_ptr<Node>> arguments;
    bool is_virtual_call = false;
    std::vector<TypeInfo> type_args;
    MethodCallExpression(const Token& t, std::unique_ptr<Node> cal)
        : Node(NodeType::METHOD_CALL, t), callee(std::move(cal)) {
            if(callee) callee->parent = this;
        }
};

struct NewInstanceExpression : public Node {
    std::unique_ptr<Node> clone() const override;

    void accept(NodeVisitor& v) override { v.visit(*this); }

    TypeInfo type_info;
    std::vector<std::unique_ptr<Node>> arguments;
    bool is_virtual_call = false;
    std::vector<TypeInfo> type_args;
    NewInstanceExpression(const Token& t, TypeInfo type)
        : Node(NodeType::NEW_INSTANCE, t), type_info(std::move(type)) {}
};

struct ArrayCreationExpression : public Node {
    std::unique_ptr<Node> clone() const override;

    void accept(NodeVisitor& v) override { v.visit(*this); }

    TypeInfo type_info;
    std::unique_ptr<Node> size;
    ArrayCreationExpression(const Token& t, TypeInfo type, std::unique_ptr<Node> sz)
        : Node(NodeType::ARRAY_CREATION, t), type_info(std::move(type)), size(std::move(sz)) {
            if(size) size->parent = this;
        }
};

struct ArrayLiteralExpression : public Node {
    std::unique_ptr<Node> clone() const override;

    void accept(NodeVisitor& v) override { v.visit(*this); }

    std::vector<std::unique_ptr<Node>> elements;
    ArrayLiteralExpression(const Token& t) : Node(NodeType::ARRAY_LITERAL, t) {}
};

struct CastExpression : public Node {
    std::unique_ptr<Node> clone() const override;

    void accept(NodeVisitor& v) override { v.visit(*this); }

    TypeInfo target_type;
    std::unique_ptr<Node> expression;
    int32_t target_vtable_id = -1;
    CastExpression(const Token& t, TypeInfo tgt, std::unique_ptr<Node> expr)
        : Node(NodeType::CAST_EXPR, t), target_type(std::move(tgt)), expression(std::move(expr)) {
            if(expression) expression->parent = this;
        }
};

struct InstanceofExpression : public Node {
    std::unique_ptr<Node> clone() const override;

    void accept(NodeVisitor& v) override { v.visit(*this); }

    std::unique_ptr<Node> expression;
    TypeInfo target_type;
    int32_t target_vtable_id = -1;
    InstanceofExpression(const Token& t, std::unique_ptr<Node> expr, TypeInfo tgt)
        : Node(NodeType::INSTANCEOF_EXPR, t), expression(std::move(expr)), target_type(std::move(tgt)) {
            if(expression) expression->parent = this;
        }
};

struct TernaryExpression : public Node {
    std::unique_ptr<Node> clone() const override;

    void accept(NodeVisitor& v) override { v.visit(*this); }

    std::unique_ptr<Node> condition;
    std::unique_ptr<Node> true_branch;
    std::unique_ptr<Node> false_branch;
    TernaryExpression(const Token& t, std::unique_ptr<Node> cond, std::unique_ptr<Node> tbr, std::unique_ptr<Node> fbr)
        : Node(NodeType::TERNARY_EXPR, t), condition(std::move(cond)), true_branch(std::move(tbr)), false_branch(std::move(fbr)) {
            if(condition) condition->parent = this;
            if(true_branch) true_branch->parent = this;
            if(false_branch) false_branch->parent = this;
        }
};

// ==========================================
// Statements
// ==========================================

struct BlockStatement : public Node {
    std::unique_ptr<Node> clone() const override;

    void accept(NodeVisitor& v) override { v.visit(*this); }

    BlockStatement(const Token& t) : Node(NodeType::BLOCK, t) {}
};

struct IfStatement : public Node {
    std::unique_ptr<Node> clone() const override;

    void accept(NodeVisitor& v) override { v.visit(*this); }

    std::unique_ptr<Node> condition;
    std::unique_ptr<Node> then_branch;
    std::unique_ptr<Node> else_branch;
    IfStatement(const Token& t, std::unique_ptr<Node> cond, std::unique_ptr<Node> then_br, std::unique_ptr<Node> else_br)
        : Node(NodeType::IF_STMT, t), condition(std::move(cond)), then_branch(std::move(then_br)), else_branch(std::move(else_br)) {
            if(condition) condition->parent = this;
            if(then_branch) then_branch->parent = this;
            if(else_branch) else_branch->parent = this;
        }
};

struct ForStatement : public Node {
    std::unique_ptr<Node> clone() const override;

    void accept(NodeVisitor& v) override { v.visit(*this); }

    std::unique_ptr<Node> initialization;
    std::unique_ptr<Node> condition;
    std::unique_ptr<Node> iteration;
    std::unique_ptr<Node> body;
    ForStatement(const Token& t) : Node(NodeType::FOR_STMT, t) {}
};

struct WhileStatement : public Node {
    std::unique_ptr<Node> clone() const override;

    void accept(NodeVisitor& v) override { v.visit(*this); }

    std::unique_ptr<Node> condition;
    std::unique_ptr<Node> body;
    WhileStatement(const Token& t) : Node(NodeType::WHILE_STMT, t) {}
};

struct DoWhileStatement : public Node {
    std::unique_ptr<Node> clone() const override;

    void accept(NodeVisitor& v) override { v.visit(*this); }

    std::unique_ptr<Node> body;
    std::unique_ptr<Node> condition;
    DoWhileStatement(const Token& t) : Node(NodeType::DO_WHILE_STMT, t) {}
};

struct SwitchStatement : public Node {
    std::unique_ptr<Node> clone() const override;

    void accept(NodeVisitor& v) override { v.visit(*this); }

    std::unique_ptr<Node> condition;
    SwitchStatement(const Token& t) : Node(NodeType::SWITCH_STMT, t) {}
};

struct CaseStatement : public Node {
    std::unique_ptr<Node> clone() const override;

    void accept(NodeVisitor& v) override { v.visit(*this); }

    std::unique_ptr<Node> case_value;
    bool is_default = false;
    CaseStatement(const Token& t) : Node(NodeType::CASE_STMT, t) {}
};

struct VariableDeclaration : public Node {
    std::unique_ptr<Node> clone() const override;

    void accept(NodeVisitor& v) override { v.visit(*this); }

    std::string var_name;
    TypeInfo type_info;
    bool is_const = false;
    bool is_reference_type = false;
    bool is_weak = false;
    std::unique_ptr<Node> initializer;
    VariableDeclaration(const Token& t, std::string name, TypeInfo type)
        : Node(NodeType::VAR_DECL, t), var_name(std::move(name)), type_info(std::move(type)) {}
};

struct ExpressionStatement : public Node {
    std::unique_ptr<Node> clone() const override;

    void accept(NodeVisitor& v) override { v.visit(*this); }

    std::unique_ptr<Node> expression;
    ExpressionStatement(const Token& t, std::unique_ptr<Node> expr)
        : Node(NodeType::EXPR_STMT, t), expression(std::move(expr)) {
            if(expression) expression->parent = this;
        }
};

struct ReturnStatement : public Node {
    std::unique_ptr<Node> clone() const override;

    void accept(NodeVisitor& v) override { v.visit(*this); }

    std::unique_ptr<Node> value;
    ReturnStatement(const Token& t, std::unique_ptr<Node> val) : Node(NodeType::RETURN_STMT, t), value(std::move(val)) {
        if(value) value->parent = this;
    }
};

struct BreakStatement : public Node {
    std::unique_ptr<Node> clone() const override;

    void accept(NodeVisitor& v) override { v.visit(*this); }

    BreakStatement(const Token& t) : Node(NodeType::BREAK_STMT, t) {}
};

struct ContinueStatement : public Node {
    std::unique_ptr<Node> clone() const override;

    void accept(NodeVisitor& v) override { v.visit(*this); }

    ContinueStatement(const Token& t) : Node(NodeType::CONTINUE_STMT, t) {}
};

// ==========================================
// Top Level Declarations
// ==========================================

struct PackageStatement : public Node {
    std::unique_ptr<Node> clone() const override;

    void accept(NodeVisitor& v) override { v.visit(*this); }

    std::string package_name;
    PackageStatement(const Token& t, std::string name) : Node(NodeType::PACKAGE_STMT, t), package_name(std::move(name)) {}
};

struct AliasStatement : public Node {
    std::unique_ptr<Node> clone() const override;

    void accept(NodeVisitor& v) override { v.visit(*this); }

    std::string alias_name;
    std::vector<std::string> template_parameters;
    TypeInfo target_type;
    AliasStatement(const Token& t, std::string alias, TypeInfo tgt)
        : Node(NodeType::ALIAS_STMT, t), alias_name(std::move(alias)), target_type(std::move(tgt)) {}
};

struct EnumDeclaration : public Node {
    std::unique_ptr<Node> clone() const override;

    void accept(NodeVisitor& v) override { v.visit(*this); }

    std::string enum_name;
    TokenType access_modifier = TokenType::KEYWORD_INTERNAL;
    std::vector<std::string> members;
    EnumDeclaration(const Token& t, std::string name) : Node(NodeType::ENUM_DECL, t), enum_name(std::move(name)) {}
};

struct MethodDeclaration;
struct ClassDeclaration : public Node {
    std::unique_ptr<Node> clone() const override;

    void accept(NodeVisitor& v) override { v.visit(*this); }

    int vtable_id = -1;
    int base_vtable_id = -1;
    std::vector<MethodDeclaration*> vtable;
    std::string class_name;
    std::vector<std::string> template_parameters;
    std::string base_class_name;
    TokenType access_modifier = TokenType::KEYWORD_INTERNAL;
    int instance_size = 0;
    ClassDeclaration(const Token& t, std::string name) : Node(NodeType::CLASS_DECL, t), class_name(std::move(name)) {}
};

struct FieldDeclaration : public Node {
    std::unique_ptr<Node> clone() const override;

    void accept(NodeVisitor& v) override { v.visit(*this); }

    std::string field_name;
    TypeInfo type_info;
    TokenType access_modifier = TokenType::KEYWORD_PRIVATE;
    bool is_static = false;
    bool is_const = false;
    bool is_reference_type = false;
    bool is_weak = false;
    std::unique_ptr<Node> initializer;
    FieldDeclaration(const Token& t, std::string name, TypeInfo type)
        : Node(NodeType::FIELD_DECL, t), field_name(std::move(name)), type_info(std::move(type)) {}
};

struct ConstructorDeclaration : public Node {
    std::unique_ptr<Node> clone() const override;

    void accept(NodeVisitor& v) override { v.visit(*this); }

    TokenType access_modifier = TokenType::KEYWORD_PUBLIC;
    std::string class_name;
    std::vector<std::string> template_parameters;
    std::string base_class_name;
    std::vector<std::unique_ptr<VariableDeclaration>> parameters;
    int frame_size = 0;
    ConstructorDeclaration(const Token& t, std::string name) : Node(NodeType::CONSTRUCTOR_DECL, t), class_name(std::move(name)) {}
};

struct MethodDeclaration : public Node {
    std::unique_ptr<Node> clone() const override;

    void accept(NodeVisitor& v) override { v.visit(*this); }

    TokenType access_modifier = TokenType::KEYWORD_PRIVATE;
    bool is_static = false;
    bool is_native = false;
    bool is_inline = false;
    bool is_virtual = false;
    bool is_override = false;
    bool is_abstract = false;
    int vtable_index = -1;
    TypeInfo return_type;
    std::string method_name;
    std::vector<std::string> template_parameters;
    std::optional<uint64_t> native_id;
    std::vector<std::unique_ptr<VariableDeclaration>> parameters;
    int frame_size = 0;
    MethodDeclaration(const Token& t, std::string name, TypeInfo ret_type)
        : Node(NodeType::METHOD_DECL, t), method_name(std::move(name)), return_type(std::move(ret_type)) {}
};

} // namespace solix
