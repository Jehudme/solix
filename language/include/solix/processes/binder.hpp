#pragma once
#include "solix/processes/process.hpp"
#include "solix/statements.hpp"
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

class Binder : public CompilationProcess {
public:
    Binder(CompilationContext& context, const std::string& name) : CompilationProcess(context, name) {}
    void execute() override;

private:
    SymbolTable global_scope;
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
    bool is_assignable(const TypeInfo& target, const TypeInfo& source);
    TypeInfo evaluate_expression(Node* expr);
    
    void throw_error(Node* node, const std::string& msg);
    
    void setup_builtins();
};

} // namespace solix
