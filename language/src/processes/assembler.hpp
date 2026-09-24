#pragma once
#include "utilities/ast_visitor.hpp"
#include "solix/compilation.hpp"
#include "processes/process.hpp"
#include "utilities/optcodes.hpp"
#include "utilities/statements.hpp"
#include <vector>
#include <string>
#include <unordered_map>
#include <utility>

namespace solix {

struct Assembler : public CompilationProcess, public NodeVisitor {
    using CompilationProcess::CompilationProcess;
    
    void execute() override;
    std::string disassemble() const;

private:
    std::vector<uint8_t>& bytecode() { return context.bytecode; }
    
    // Track where functions start in the flat bytecode array
    std::unordered_map<Node*, uint32_t> function_ips;
    
    // Linker Phase patches: Map from <Byte_Index_Of_0xFFFFFFFF_Hole> to <Function_Node>
    std::vector<std::pair<size_t, Node*>> linker_patches;
    
    std::vector<std::vector<uint32_t>> loop_break_patches;
    std::vector<std::vector<uint32_t>> loop_continue_patches;
    
    uint32_t native_id_counter = 1;

    void emit_byte(uint8_t byte);
    void emit_int32(uint32_t value);
    void emit_int64(uint64_t value);
    void emit_float32(float value);
    void emit_float64(double value);
    void emit_string(const std::string& value);

    void compile_boot_sequence();
    void compile_class(ClassDeclaration* class_node);
    void compile_function(Node* function_node);

    void compile_node(Node* node);
    void compile_expression(Node* expr);
    
    void emit_cleanup_for_node(Node* node);
    void emit_cleanup_for_function(Node* func_node);
    
    void apply_linker_patches();
    void throw_error(Node* node, const std::string& msg);

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
    void visit(TryStatement& node) override;
    void visit(CatchClause& node) override;
    void visit(ThrowStatement& node) override;
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
};

} // namespace solix
