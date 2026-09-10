#pragma once

#include "solix/parser.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <stdexcept>

namespace solix {
namespace semantic {

// A structure representing a fully resolved type in Solix.
// For example: "int32", "array array float64", or "com.solix.Engine"
struct TypeInfo {
    std::string base_name;             // The core type (e.g., "int32" or "com.solix.Engine")
    int array_depth = 0;               // 0 for scalar, 1 for "array", 2 for "array array"
    bool is_primitive = false;         // True for int32, float64, bool, string, etc.
    parser::Node* class_ref = nullptr; // If it's a custom Class or Enum, this points directly to its declaration
    
    bool operator==(const TypeInfo& other) const {
        return base_name == other.base_name && array_depth == other.array_depth;
    }
    bool operator!=(const TypeInfo& other) const {
        return !(*this == other);
    }
    
    // Helper to print the type for error messages (e.g., returns "array int32")
    std::string to_string() const;
};

// Represents a single block scope level (e.g., inside {} braces)
struct Scope {
    // Maps a local variable/parameter name to its Declaration Node
    std::unordered_map<std::string, parser::Node*> symbols;
};

// The core Semantic Analyzer that orchestrates the multi-pass validation of the AST.
class SemanticAnalyzer {
public:
    // Main entry point. Modifies the tree in-place. Throws an error on semantic violations.
    void analyze(parser::AstTree& tree);

private:
    // ==========================================
    // Context & State Trackers
    // ==========================================
    
    // Tracks nested block scopes for variable shadowing and lifecycle
    std::vector<Scope> scope_stack;
    
    // Context flags to enforce structural rules
    int loop_depth = 0;                                 // >0 means we are inside a loop (break/continue are valid)
    parser::ClassDeclaration* current_class = nullptr;  // Tracks the 'this' context and visibility boundaries
    parser::MethodDeclaration* current_method = nullptr;// Tracks expected return type for 'return' statements
    std::string current_package;                        // Tracks the namespace prefix

    // ==========================================
    // Multi-Pass Executors
    // ==========================================
    
    // Pass 1: Global Outline
    // Harvests all classes, enums, fields, and method signatures so forward-references work.
    void registerGlobalSymbols(parser::AstTree& tree, parser::Node* root, const std::string& prefix);
    
    // Pass 2 & 3: Deep Dive & Enforcement
    // Binds identifiers, evaluates expression types, and enforces OOP rules.
    void resolveAndCheck(parser::AstTree& tree, parser::Node* root);

    // ==========================================
    // Helper Utilities
    // ==========================================
    
    // Scope Management
    void pushScope();
    void popScope();
    
    // Safely registers a variable in the current scope. Throws if it already exists.
    void declareLocal(const std::string& name, parser::Node* node, const std::vector<lexer::Token>& tokens);
    
    // Searches the scope stack (bottom-up), then class fields, then global symbols.
    parser::Node* lookupSymbol(parser::AstTree& tree, const std::string& name);

    // Type Resolution & AST Decoration
    TypeInfo resolveType(parser::AstTree& tree, const std::string& raw_type_name, const std::vector<lexer::Token>& tokens);
    TypeInfo evaluateExpression(parser::AstTree& tree, parser::Node* expr);

    // OOP Rule Enforcement
    void enforceAccessModifier(parser::Node* target_node, const std::vector<lexer::Token>& tokens);
    void enforceLValue(parser::Node* expr, const std::vector<lexer::Token>& tokens);
};

} // namespace semantic
} // namespace solix
