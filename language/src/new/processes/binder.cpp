#include "solix/new/processes/binder.hpp"
#include "solix/new/compilation.hpp"

namespace solix {

void Binder::throw_error(Node* node, const std::string& msg) {
    std::string err = "";
    if (node && node->source) {
        if (std::holds_alternative<std::filesystem::path>(*node->source)) {
            err += std::get<std::filesystem::path>(*node->source).string() + ":";
        } else {
            err += std::get<std::string>(*node->source) + ":";
        }
        err += std::to_string(node->line) + ":" + std::to_string(node->column) + " - ";
    }
    err += msg;
    throw BindError(err);
}

void Binder::setup_builtins() {
    // Create dummy nodes for builtins. We don't store them in AST, just as pointers.
    // To avoid memory leaks in the compiler, we could store them in context.nodes, but 
    // it's easier to just allocate them as part of Binder's lifecycle or global singletons.
    // For simplicity, we just use static instances or new them up and leak them (compiler is short lived).
    // Let's allocate them dynamically and leak them for now.
    
    auto make_builtin = [&](const std::string& name) {
        auto* decl = new ClassDeclaration(Token{}, name);
        decl->is_primitive = true;
        decl->mangled_name = name;
        global_scope.define(name, decl);
        return decl;
    };
    
    builtin_void = make_builtin("void");
    builtin_int8 = make_builtin("int8");
    builtin_int16 = make_builtin("int16");
    builtin_int32 = make_builtin("int32");
    builtin_int64 = make_builtin("int64");
    builtin_uint8 = make_builtin("uint8");
    builtin_uint16 = make_builtin("uint16");
    builtin_uint32 = make_builtin("uint32");
    builtin_uint64 = make_builtin("uint64");
    builtin_float32 = make_builtin("float32");
    builtin_float64 = make_builtin("float64");
    builtin_bool = make_builtin("bool");
    builtin_char = make_builtin("char");
    builtin_string = make_builtin("string");
}

std::string Binder::mangle_method(MethodDeclaration* method) {
    if (method->is_native) return method->method_name; // native methods maintain pure name
    std::string mangled = method->method_name;
    for (const auto& param : method->parameters) {
        auto* var_decl = static_cast<VariableDeclaration*>(param.get());
        mangled += "_" + var_decl->type_info.to_string();
        if (var_decl->is_reference_type) mangled += "R";
    }
    return mangled;
}

std::string Binder::mangle_method_call(const std::string& base_name, const std::vector<TypeInfo>& arg_types) {
    std::string mangled = base_name;
    for (const auto& arg : arg_types) {
        mangled += "_" + arg.to_string();
    }
    return mangled;
}

std::string Binder::mangle_constructor(const std::string& class_name, const std::vector<TypeInfo>& arg_types) {
    std::string mangled = class_name + "_ctor";
    for (const auto& arg : arg_types) {
        mangled += "_" + arg.to_string();
    }
    return mangled;
}

void Binder::enter_scope(SymbolTable* new_scope) {
    new_scope->parent = current_scope;
    current_scope = new_scope;
}

void Binder::exit_scope() {
    if (current_scope->parent) {
        current_scope = current_scope->parent;
    }
}

void Binder::declare_local(const std::string& name, Node* node) {
    if (current_scope->symbols.count(name)) {
        throw_error(node, "Variable '" + name + "' is already defined in this scope.");
    }
    current_scope->define(name, node);
}


void Binder::register_global_symbols(Node* node, const std::string& prefix) {
    if (!node) return;
    std::string my_prefix = prefix;
    
    if (node->node_type == NodeType::PACKAGE_STMT) {
        auto* pkg = static_cast<PackageStatement*>(node);
        current_package = pkg->package_name + ".";
        pkg->mangled_name = pkg->package_name;
        global_scope.define(pkg->mangled_name, pkg);
    } else if (node->node_type == NodeType::CLASS_DECL) {
        auto* class_decl = static_cast<ClassDeclaration*>(node);
        std::string full_name = prefix + class_decl->class_name;
        if (global_scope.symbols.count(full_name)) throw_error(node, "Duplicate global symbol: " + full_name);
        class_decl->mangled_name = full_name;
        global_scope.define(full_name, class_decl);
        my_prefix = full_name + ".";
    } else if (node->node_type == NodeType::ENUM_DECL) {
        auto* enm = static_cast<EnumDeclaration*>(node);
        std::string full_name = prefix + enm->enum_name;
        if (global_scope.symbols.count(full_name)) throw_error(node, "Duplicate global symbol: " + full_name);
        enm->mangled_name = full_name;
        global_scope.define(full_name, enm);
        my_prefix = full_name + ".";
    } else if (node->node_type == NodeType::ALIAS_STMT) {
        auto* alias = static_cast<AliasStatement*>(node);
        std::string full_name = prefix + alias->alias_name;
        if (global_scope.symbols.count(full_name)) throw_error(node, "Duplicate global symbol: " + full_name);
        alias->mangled_name = full_name;
        global_scope.define(full_name, alias);
    } else if (node->node_type == NodeType::FIELD_DECL) {
        auto* field = static_cast<FieldDeclaration*>(node);
        std::string full_name = prefix + field->field_name;
        if (global_scope.symbols.count(full_name)) throw_error(node, "Duplicate field symbol: " + full_name);
        field->mangled_name = full_name;
        global_scope.define(full_name, field);
    } else if (node->node_type == NodeType::METHOD_DECL) {
        auto* method = static_cast<MethodDeclaration*>(node);
        std::string full_name = prefix + mangle_method(method);
        if (global_scope.symbols.count(full_name) && !method->is_native) { // native methods can collide if overloaded poorly, warn maybe
            throw_error(node, "Duplicate method signature: " + full_name);
        }
        method->mangled_name = full_name;
        global_scope.define(full_name, method);
        my_prefix = prefix + method->method_name + ".";
    } else if (node->node_type == NodeType::CONSTRUCTOR_DECL) {
        auto* ctor = static_cast<ConstructorDeclaration*>(node);
        std::string full_name = prefix + "ctor";
        for (const auto& param : ctor->parameters) {
            auto* var_decl = static_cast<VariableDeclaration*>(param.get());
            full_name += "_" + var_decl->type_info.to_string();
        }
        ctor->mangled_name = full_name;
        global_scope.define(full_name, ctor);
        my_prefix = prefix + "ctor.";
    }
    
    if (node->node_type == NodeType::CLASS_DECL || node->node_type == NodeType::ENUM_DECL) {
        for (const auto& child : node->children) {
            register_global_symbols(child.get(), my_prefix);
        }
    }
}

TypeInfo Binder::resolve_type(const TypeInfo& raw_type, Node* error_node) {
    if (raw_type.name == "") return raw_type;
    
    Node* resolved = global_scope.resolve(raw_type.name);
    if (!resolved && current_package != "") {
        resolved = global_scope.resolve(current_package + raw_type.name);
    }
    
    if (!resolved) {
        throw_error(error_node, "Unknown type: " + raw_type.name);
    }
    
    TypeInfo result = raw_type;
    if (resolved->node_type == NodeType::ALIAS_STMT) {
        auto* alias = static_cast<AliasStatement*>(resolved);
        result.name = alias->target_type.name;
        result.array_depth += alias->target_type.array_depth;
        // recursively resolve aliases
        return resolve_type(result, error_node);
    } else if (resolved->node_type == NodeType::CLASS_DECL || resolved->node_type == NodeType::ENUM_DECL) {
        result.name = resolved->mangled_name; // Use fully qualified name
    }
    
    return result;
}

void Binder::bind_types_and_memory() {
    static_variable_index = 1;
    
    // First, map all static fields
    for (const auto& [name, node] : global_scope.symbols) {
        if (node->node_type == NodeType::FIELD_DECL) {
            auto* field = static_cast<FieldDeclaration*>(node);
            field->type_info = resolve_type(field->type_info, field);
            
            Node* type_decl = global_scope.resolve(field->type_info.name);
            if (type_decl && type_decl->is_primitive && field->type_info.array_depth == 0) {
                field->is_reference_type = false;
            } else {
                field->is_reference_type = true;
            }
            
            if (field->is_static || !field->parent) {
                field->memory_index = static_variable_index++;
            }
        }
    }
    
    // Second, calculate instance size for classes
    for (const auto& [name, node] : global_scope.symbols) {
        if (node->node_type == NodeType::CLASS_DECL) {
            auto* class_decl = static_cast<ClassDeclaration*>(node);
            int field_offset = 0;
            for (const auto& child : class_decl->children) {
                if (child->node_type == NodeType::FIELD_DECL) {
                    auto* field = static_cast<FieldDeclaration*>(child.get());
                    if (!field->is_static) {
                        field->memory_index = field_offset++;
                    }
                }
            }
            class_decl->instance_size = field_offset;
        }
    }
}

void Binder::execute() {
    log_info("Starting Semantic Analysis (Binding)...");
    
    setup_builtins();
    
    // Pass 1: Global Outline
    for (const auto& [source, nodes] : context.nodes) {
        current_package = "";
        for (const auto& node : nodes) {
            if (node->node_type == NodeType::PACKAGE_STMT) {
                auto* pkg = static_cast<PackageStatement*>(node.get());
                current_package = pkg->package_name + ".";
                pkg->mangled_name = pkg->package_name;
                global_scope.define(pkg->mangled_name, pkg);
            } else {
                register_global_symbols(node.get(), current_package);
            }
        }
    }
    
    log_debug("Registered {} global symbols.", global_scope.symbols.size());
    
    // Pass 2: Type and Memory Binding
    bind_types_and_memory();
    
    log_debug("Memory mapping complete. Static variables: {}", static_variable_index);
    
    // Pass 3: Execution Logic Binding
    for (const auto& [source, nodes] : context.nodes) {
        current_package = "";
        for (const auto& node : nodes) {
            if (node->node_type == NodeType::PACKAGE_STMT) {
                current_package = static_cast<PackageStatement*>(node.get())->package_name + ".";
            } else {
                bind_tree(node.get());
            }
        }
    }
    
    log_info("Semantic Analysis completed successfully.");
}

void Binder::bind_tree(Node* root) {
    if (!root) return;
    
    if (root->node_type == NodeType::CLASS_DECL) {
        current_class = static_cast<ClassDeclaration*>(root);
    } else if (root->node_type == NodeType::METHOD_DECL) {
        current_method = static_cast<MethodDeclaration*>(root);
        local_variable_index = 0; // reset locals
        
        // Ensure return type is valid
        current_method->return_type = resolve_type(current_method->return_type, current_method);
        
        SymbolTable method_scope;
        enter_scope(&method_scope);
        
        // Define 'this' if not static
        if (!current_method->is_static && current_class) {
            auto* this_decl = new VariableDeclaration(Token{}, "this", TypeInfo{current_class->mangled_name, 0});
            this_decl->memory_index = local_variable_index++;
            this_decl->is_reference_type = true;
            declare_local("this", this_decl);
            // intentionally leaked for simplicity like builtins
        }
        
        for (const auto& param : current_method->parameters) {
            auto* p = static_cast<VariableDeclaration*>(param.get());
            p->type_info = resolve_type(p->type_info, p);
            
            Node* type_decl = global_scope.resolve(p->type_info.name);
            if (type_decl && type_decl->is_primitive && p->type_info.array_depth == 0) p->is_reference_type = false;
            else p->is_reference_type = true;
            
            p->memory_index = local_variable_index++;
            declare_local(p->var_name, p);
        }
        
        for (const auto& child : current_method->children) {
            bind_node(child.get());
        }
        
        exit_scope();
        current_method = nullptr;
        return; // already processed children
    } else if (root->node_type == NodeType::CONSTRUCTOR_DECL) {
        auto* ctor = static_cast<ConstructorDeclaration*>(root);
        local_variable_index = 0;
        
        SymbolTable method_scope;
        enter_scope(&method_scope);
        
        if (current_class) {
            auto* this_decl = new VariableDeclaration(Token{}, "this", TypeInfo{current_class->mangled_name, 0});
            this_decl->memory_index = local_variable_index++;
            this_decl->is_reference_type = true;
            declare_local("this", this_decl);
        }
        
        for (const auto& param : ctor->parameters) {
            auto* p = static_cast<VariableDeclaration*>(param.get());
            p->type_info = resolve_type(p->type_info, p);
            
            Node* type_decl = global_scope.resolve(p->type_info.name);
            if (type_decl && type_decl->is_primitive && p->type_info.array_depth == 0) p->is_reference_type = false;
            else p->is_reference_type = true;
            
            p->memory_index = local_variable_index++;
            declare_local(p->var_name, p);
        }
        
        for (const auto& child : ctor->children) {
            bind_node(child.get());
        }
        
        exit_scope();
        return;
    }
    
    // Default: recurse
    for (const auto& child : root->children) {
        if (child) bind_tree(child.get());
    }
    
    if (root->node_type == NodeType::CLASS_DECL) {
        current_class = nullptr;
    }
}


void Binder::bind_node(Node* node) {
    if (!node) return;
    
    if (node->node_type == NodeType::BLOCK) {
        SymbolTable block_scope;
        enter_scope(&block_scope);
        for (const auto& child : node->children) {
            bind_node(child.get());
        }
        exit_scope();
    } else if (node->node_type == NodeType::VAR_DECL) {
        auto* var = static_cast<VariableDeclaration*>(node);
        var->type_info = resolve_type(var->type_info, var);
        
        Node* type_decl = global_scope.resolve(var->type_info.name);
        if (type_decl && type_decl->is_primitive && var->type_info.array_depth == 0) var->is_reference_type = false;
        else var->is_reference_type = true;
        
        if (var->initializer) {
            TypeInfo init_type = evaluate_expression(var->initializer.get());
            if (init_type != var->type_info) {
                throw_error(node, "Type mismatch in variable declaration");
            }
        }
        var->memory_index = local_variable_index++;
        declare_local(var->var_name, var);
    } else if (node->node_type == NodeType::IF_STMT) {
        auto* if_stmt = static_cast<IfStatement*>(node);
        TypeInfo cond = evaluate_expression(if_stmt->condition.get());
        if (cond.name != "bool") throw_error(node, "Condition must be bool");
        bind_node(if_stmt->then_branch.get());
        if (if_stmt->else_branch) bind_node(if_stmt->else_branch.get());
    } else if (node->node_type == NodeType::WHILE_STMT) {
        auto* w_stmt = static_cast<WhileStatement*>(node);
        TypeInfo cond = evaluate_expression(w_stmt->condition.get());
        if (cond.name != "bool") throw_error(node, "Condition must be bool");
        loop_depth++;
        bind_node(w_stmt->body.get());
        loop_depth--;
    } else if (node->node_type == NodeType::DO_WHILE_STMT) {
        auto* dw_stmt = static_cast<DoWhileStatement*>(node);
        loop_depth++;
        bind_node(dw_stmt->body.get());
        loop_depth--;
        TypeInfo cond = evaluate_expression(dw_stmt->condition.get());
        if (cond.name != "bool") throw_error(node, "Condition must be bool");
    } else if (node->node_type == NodeType::FOR_STMT) {
        auto* f_stmt = static_cast<ForStatement*>(node);
        SymbolTable for_scope;
        enter_scope(&for_scope);
        if (f_stmt->initialization) bind_node(f_stmt->initialization.get());
        if (f_stmt->condition) {
            TypeInfo cond = evaluate_expression(f_stmt->condition.get());
            if (cond.name != "bool") throw_error(node, "Condition must be bool");
        }
        if (f_stmt->iteration) evaluate_expression(f_stmt->iteration.get());
        loop_depth++;
        bind_node(f_stmt->body.get());
        loop_depth--;
        exit_scope();
    } else if (node->node_type == NodeType::RETURN_STMT) {
        auto* r_stmt = static_cast<ReturnStatement*>(node);
        if (r_stmt->value) {
            TypeInfo ret = evaluate_expression(r_stmt->value.get());
            if (current_method && ret != current_method->return_type) {
                throw_error(node, "Return type mismatch");
            }
        } else if (current_method && current_method->return_type.name != "void") {
            throw_error(node, "Must return a value");
        }
    } else if (node->node_type == NodeType::BREAK_STMT || node->node_type == NodeType::CONTINUE_STMT) {
        if (loop_depth == 0) throw_error(node, "Break/Continue must be inside a loop");
    } else if (node->node_type == NodeType::EXPR_STMT) {
        auto* e_stmt = static_cast<ExpressionStatement*>(node);
        evaluate_expression(e_stmt->expression.get());
    }
}


TypeInfo Binder::evaluate_expression(Node* expr) {
    if (!expr) return {"void", 0};
    
    if (expr->node_type == NodeType::LITERAL) {
        auto* lit = static_cast<LiteralNode*>(expr);
        if (std::holds_alternative<int64_t>(lit->value)) expr->expression_type = {"int32", 0}; // default to int32, runtime handles cast
        else if (std::holds_alternative<double>(lit->value)) expr->expression_type = {"float64", 0};
        else if (std::holds_alternative<std::string>(lit->value)) expr->expression_type = {"string", 0};
        else expr->expression_type = {"void", 0};
        return expr->expression_type;
    } else if (expr->node_type == NodeType::IDENTIFIER) {
        auto* id = static_cast<IdentifierNode*>(expr);
        Node* decl = current_scope->resolve(id->name);
        if (!decl && current_class) { // fallback to class fields
            decl = global_scope.resolve(current_class->mangled_name + "." + id->name);
        }
        if (!decl) decl = global_scope.resolve(current_package + id->name);
        if (!decl) decl = global_scope.resolve(id->name); // absolute
        
        if (!decl) throw_error(expr, "Undefined identifier: " + id->name);
        expr->resolved_declaration = decl;
        
        if (decl->node_type == NodeType::VAR_DECL) {
            expr->expression_type = static_cast<VariableDeclaration*>(decl)->type_info;
        } else if (decl->node_type == NodeType::FIELD_DECL) {
            expr->expression_type = static_cast<FieldDeclaration*>(decl)->type_info;
        } else if (decl->node_type == NodeType::CLASS_DECL || decl->node_type == NodeType::ENUM_DECL) {
            expr->expression_type = {decl->mangled_name, 0};
        } else {
            throw_error(expr, "Invalid identifier usage");
        }
        return expr->expression_type;
    } else if (expr->node_type == NodeType::BINARY_EXPR) {
        auto* bin = static_cast<BinaryExpression*>(expr);
        TypeInfo left = evaluate_expression(bin->left.get());
        TypeInfo right = evaluate_expression(bin->right.get());
        if (left != right) throw_error(expr, "Binary operands type mismatch");
        
        if (bin->op >= TokenType::OPERATOR_EQUAL && bin->op <= TokenType::OPERATOR_GREATER_EQUAL) {
            expr->expression_type = {"bool", 0};
        } else {
            expr->expression_type = left;
        }
        return expr->expression_type;
    } else if (expr->node_type == NodeType::ASSIGNMENT_EXPR) {
        auto* assign = static_cast<AssignmentExpression*>(expr);
        TypeInfo target = evaluate_expression(assign->target.get());
        TypeInfo value = evaluate_expression(assign->value.get());
        if (target != value) throw_error(expr, "Assignment type mismatch");
        expr->expression_type = target;
        return expr->expression_type;
    } else if (expr->node_type == NodeType::MEMBER_ACCESS) {
        auto* acc = static_cast<MemberAccessExpression*>(expr);
        TypeInfo target = evaluate_expression(acc->object.get());
        
        if (target.array_depth > 0) {
            if (acc->member_name == "length") {
                expr->expression_type = {"int32", 0};
                return expr->expression_type;
            }
            throw_error(expr, "Arrays only have 'length' property");
        }
        
        Node* type_decl = global_scope.resolve(target.name);
        if (!type_decl) throw_error(expr, "Cannot access members on unknown type");
        
        if (type_decl->node_type == NodeType::CLASS_DECL) {
            auto* c = static_cast<ClassDeclaration*>(type_decl);
            Node* member_decl = global_scope.resolve(c->mangled_name + "." + acc->member_name);
            if (!member_decl) throw_error(expr, "Member not found: " + acc->member_name);
            expr->resolved_declaration = member_decl;
            
            if (member_decl->node_type == NodeType::FIELD_DECL) {
                expr->expression_type = static_cast<FieldDeclaration*>(member_decl)->type_info;
                return expr->expression_type;
            }
        } else if (type_decl->node_type == NodeType::ENUM_DECL) {
            // Enum members
            expr->expression_type = target;
            return target;
        }
        
        throw_error(expr, "Invalid member access");
    } else if (expr->node_type == NodeType::METHOD_CALL) {
        auto* call = static_cast<MethodCallExpression*>(expr);
        
        std::vector<TypeInfo> arg_types;
        for (const auto& arg : call->arguments) {
            arg_types.push_back(evaluate_expression(arg.get()));
        }
        
        if (call->callee->node_type == NodeType::MEMBER_ACCESS) {
            auto* acc = static_cast<MemberAccessExpression*>(call->callee.get());
            TypeInfo target_type = evaluate_expression(acc->object.get());
            std::string mangled = target_type.name + "." + acc->member_name;
            for (const auto& at : arg_types) mangled += "_" + at.to_string();
            
            Node* method_decl = global_scope.resolve(mangled);
            if (!method_decl) throw_error(expr, "No matching method found");
            call->resolved_declaration = method_decl;
            expr->expression_type = static_cast<MethodDeclaration*>(method_decl)->return_type;
            return expr->expression_type;
        } else {
            // It's a local function call, likely in same class
            if (!current_class) throw_error(expr, "Local function calls must be inside a class");
            auto* id = static_cast<IdentifierNode*>(call->callee.get());
            std::string mangled = current_class->mangled_name + "." + id->name;
            for (const auto& at : arg_types) mangled += "_" + at.to_string();
            
            Node* method_decl = global_scope.resolve(mangled);
            if (!method_decl) throw_error(expr, "No matching method found");
            call->resolved_declaration = method_decl;
            expr->expression_type = static_cast<MethodDeclaration*>(method_decl)->return_type;
            return expr->expression_type;
        }
    } else if (expr->node_type == NodeType::NEW_INSTANCE) {
        auto* inst = static_cast<NewInstanceExpression*>(expr);
        inst->type_info = resolve_type(inst->type_info, inst);
        
        std::vector<TypeInfo> arg_types;
        for (const auto& arg : inst->arguments) {
            arg_types.push_back(evaluate_expression(arg.get()));
        }
        
        std::string mangled_ctor = mangle_constructor(inst->type_info.name, arg_types);
        Node* ctor = global_scope.resolve(mangled_ctor);
        if (!ctor && !arg_types.empty()) {
            throw_error(expr, "No matching constructor found");
        }
        inst->resolved_declaration = global_scope.resolve(inst->type_info.name);
        expr->expression_type = inst->type_info;
        return expr->expression_type;
    } else if (expr->node_type == NodeType::ARRAY_CREATION) {
        auto* arr = static_cast<ArrayCreationExpression*>(expr);
        arr->type_info = resolve_type(arr->type_info, arr);
        TypeInfo size = evaluate_expression(arr->size.get());
        if (size.name != "int32") throw_error(expr, "Array size must be int32");
        expr->expression_type = arr->type_info;
        expr->expression_type.array_depth++;
        return expr->expression_type;
    } else if (expr->node_type == NodeType::ARRAY_ACCESS) {
        auto* acc = static_cast<ArrayAccessExpression*>(expr);
        TypeInfo arr = evaluate_expression(acc->array.get());
        if (arr.array_depth == 0) throw_error(expr, "Cannot index non-array");
        TypeInfo idx = evaluate_expression(acc->index.get());
        if (idx.name != "int32") throw_error(expr, "Array index must be int32");
        expr->expression_type = arr;
        expr->expression_type.array_depth--;
        return expr->expression_type;
    } else if (expr->node_type == NodeType::CAST_EXPR) {
        auto* cst = static_cast<CastExpression*>(expr);
        evaluate_expression(cst->expression.get());
        cst->target_type = resolve_type(cst->target_type, cst);
        expr->expression_type = cst->target_type;
        return expr->expression_type;
    } else if (expr->node_type == NodeType::UNARY_EXPR) {
        auto* uny = static_cast<UnaryExpression*>(expr);
        expr->expression_type = evaluate_expression(uny->operand.get());
        return expr->expression_type;
    }
    
    expr->expression_type = {"void", 0};
    return expr->expression_type;
}

} // namespace solix
