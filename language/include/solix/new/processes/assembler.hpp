#pragma once
#include "solix/new/compilation.hpp"
#include "solix/new/processes/process.hpp"
#include "solix/new/utilities/optcodes.hpp"
#include "solix/new/statements.hpp"
#include <vector>
#include <string>
#include <unordered_map>
#include <utility>

namespace solix {

struct Assembler : public CompilationProcess {
    using CompilationProcess::CompilationProcess;
    
    void execute() override;

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
    
    void apply_linker_patches();
    void throw_error(Node* node, const std::string& msg);
};

} // namespace solix
