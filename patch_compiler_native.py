import sys

with open("language/src/compiler.cpp", "r") as f:
    code = f.read()

# Update disassemble
target_disasm = """            case OpCode::CALL: {
                uint32_t val;
                std::memcpy(&val, &bytecode[ip], sizeof(val));
                ip += sizeof(val);
                result += "CALL " + std::to_string(val) + "\\n";
                break;
            }"""

replacement_disasm = """            case OpCode::CALL: {
                uint32_t val;
                std::memcpy(&val, &bytecode[ip], sizeof(val));
                ip += sizeof(val);
                result += "CALL " + std::to_string(val) + "\\n";
                break;
            }
            case OpCode::CALL_NATIVE: {
                uint32_t val;
                std::memcpy(&val, &bytecode[ip], sizeof(val));
                ip += sizeof(val);
                result += "CALL_NATIVE " + std::to_string(val) + "\\n";
                break;
            }
            case OpCode::DEFINE_NATIVE: {
                uint32_t val;
                std::memcpy(&val, &bytecode[ip], sizeof(val));
                ip += sizeof(val);
                
                uint32_t length;
                std::memcpy(&length, &bytecode[ip], sizeof(length));
                ip += sizeof(length);
                
                std::string str(reinterpret_cast<const char*>(&bytecode[ip]), length);
                ip += length;
                
                result += "DEFINE_NATIVE " + std::to_string(val) + " \\"" + str + "\\"\\n";
                break;
            }"""

code = code.replace(target_disasm, replacement_disasm)

# Update compileBootSequence
target_boot = """void Compiler::compileBootSequence(std::string_view entry_point) {
    // Collect all static fields to know how much memory to allocate
    uint32_t static_count = 0;"""

replacement_boot = """void Compiler::compileBootSequence(std::string_view entry_point) {
    // Link Native Functions first
    uint32_t native_id_counter = 0;
    for (auto* native_method : ast_tree.native_methods) {
        native_method->memory_index = native_id_counter++;
        emitByte(static_cast<uint8_t>(OpCode::DEFINE_NATIVE));
        emitInt32(native_method->memory_index);
        emitString(native_method->symbol_name);
    }

    // Collect all static fields to know how much memory to allocate
    uint32_t static_count = 0;"""

code = code.replace(target_boot, replacement_boot)

# Update compileExpression (CALL_EXPRESSION)
target_call = """            if (expr->resolved_declaration) {
                // If it's a method call, we patch it later
                emitByte(static_cast<uint8_t>(OpCode::CALL));
                linker_patches.push_back(std::make_pair(bytecode.size(), expr->resolved_declaration));
                emitInt32(0xFFFFFFFF);
            } else {
                throw_compile_error(expr, "Unresolved function call");
            }"""

replacement_call = """            if (expr->resolved_declaration) {
                if (expr->resolved_declaration->node_type == parser::NodeType::METHOD_DECLARATION) {
                    auto method = static_cast<parser::MethodDeclaration*>(expr->resolved_declaration);
                    if (method->is_native) {
                        emitByte(static_cast<uint8_t>(OpCode::CALL_NATIVE));
                        emitInt32(method->memory_index);
                    } else {
                        // Standard method call, we patch it later
                        emitByte(static_cast<uint8_t>(OpCode::CALL));
                        linker_patches.push_back(std::make_pair(bytecode.size(), expr->resolved_declaration));
                        emitInt32(0xFFFFFFFF);
                    }
                } else {
                    emitByte(static_cast<uint8_t>(OpCode::CALL));
                    linker_patches.push_back(std::make_pair(bytecode.size(), expr->resolved_declaration));
                    emitInt32(0xFFFFFFFF);
                }
            } else {
                throw_compile_error(expr, "Unresolved function call");
            }"""

code = code.replace(target_call, replacement_call)

with open("language/src/compiler.cpp", "w") as f:
    f.write(code)
