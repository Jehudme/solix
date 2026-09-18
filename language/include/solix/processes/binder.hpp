#include <unordered_set>
#pragma once
#include "solix/processes/process.hpp"
#include "solix/statements.hpp"
#include "solix/ast_visitor.hpp"
#include <unordered_map>
#include <string>
#include <vector>
#include <stdexcept>

namespace solix {

struct SymbolTable {
    SymbolTable* parent = nullptr;
    std::unordered_map<std::string, Node*> symbols;

    void define(const std::string& name, Node* node) {
        symbols[name] = node;
    }

    Node* resolve(const std::string& name) {
        auto it = symbols.find(name);
        if (it != symbols.end()) return it->second;
        if (parent) return parent->resolve(name);
        return nullptr;
    }
};

class BindError : public std::runtime_error {
public:
    BindError(const std::string& msg) : std::runtime_error(msg) {}
};

enum class BinderPass { REGISTER_GLOBALS, REGISTER_MEMBERS, BIND_EXECUTION, EVALUATE_EXPRESSION };

class Binder : public CompilationProcess, public NodeVisitor {
public:
    Binder(CompilationContext& context, const std::string& name) : CompilationProcess(context, name) {}
    void execute() override;

    void visit(IdentifierNode& node) override;
    void visit(LiteralNode& node) override;
    void visit(BinaryExpression& node) override;
    void visit(UnaryExpression& node) override;
    void visit(AssignmentExpression& node) override;
    void visit(ArrayAccessExpression& node) override;
    void visit(MemberAccessExpression& node) override;
    void visit(MethodCallExpression& node) override;
    void visit(NewInstanceExpression& node) override;
    void visit(ArrayCreationExpression& node) override;
    void visit(ArrayLiteralExpression& node) override;
    void visit(CastExpression& node) override;
    void visit(InstanceofExpression& node) override;
    void visit(TernaryExpression& node) override;
    void visit(BlockStatement& node) override;
    void visit(IfStatement& node) override;
    void visit(ForStatement& node) override;
    void visit(WhileStatement& node) override;
    void visit(DoWhileStatement& node) override;
    void visit(SwitchStatement& node) override;
    void visit(CaseStatement& node) override;
    void visit(VariableDeclaration& node) override;
    void visit(ExpressionStatement& node) override;
    void visit(ReturnStatement& node) override;
    void visit(BreakStatement& node) override;
    void visit(ContinueStatement& node) override;
    void visit(PackageStatement& node) override;
    void visit(AliasStatement& node) override;
    void visit(EnumDeclaration& node) override;
    void visit(ClassDeclaration& node) override;
    void visit(FieldDeclaration& node) override;
    void visit(ConstructorDeclaration& node) override;
    void visit(MethodDeclaration& node) override;

private:
public:
    SymbolTable global_scope;
  std::unordered_map<std::string, Node*> template_registry;
  std::unordered_set<std::string> instantiated_templates;
    SymbolTable* current_scope = &global_scope;
    
    // Builtin primitives
    Node* builtin_int8;
    Node* builtin_int16;
    Node* builtin_int32;
    Node* builtin_int64;
    Node* builtin_uint8;
    Node* builtin_uint16;
    Node* builtin_uint32;
    Node* builtin_uint64;
    Node* builtin_float32;
    Node* builtin_float64;
    Node* builtin_bool;
    Node* builtin_char;
    Node* builtin_void;
    
    int static_variable_index = 1; // 0 reserved for null
    int local_variable_index = 0;
    int loop_depth = 0;
    int switch_depth = 0;
    
    ClassDeclaration* current_class = nullptr;
    MethodDeclaration* current_method = nullptr;
    std::string current_package;
    
    BinderPass current_pass;
    std::string current_prefix;
    TypeInfo evaluated_type;
    
    // Pass 1: Global Symbol Outline
    void register_global_symbols(Node* node, const std::string& prefix);
    void register_members(Node* node, const std::string& prefix);
    
    // Pass 2: Type and Memory Binding
    void bind_types_and_memory();
    
    // Pass 3: Execution Logic Binding
    void bind_tree(Node* root);
    void bind_node(Node* node);
    
    // Scoping helpers
    void enter_scope(SymbolTable* new_scope);
    void exit_scope();
    void declare_local(const std::string& name, Node* node);
    
    // Mangling
    std::string mangle_method(MethodDeclaration* method);
    std::string mangle_method_call(const std::string& base_name, const std::vector<TypeInfo>& arg_types);
    std::string mangle_constructor(const std::string& class_name, const std::vector<TypeInfo>& arg_types);
    
    // Evaluators
    TypeInfo resolve_type(const TypeInfo& raw_type, Node* error_node);
    Node* instantiate_template(const std::string& template_name, const std::vector<TypeInfo>& type_args, Node* error_node);
    bool is_assignable(const TypeInfo& target, const TypeInfo& source);
    TypeInfo evaluate_expression(Node* expr);
    
    void record_error(Node* node, const std::string& msg);
    
    void setup_builtins();
    bool check_access(Node* member_decl, Node* owner_class, Node* expr);
};

} // namespace solix
