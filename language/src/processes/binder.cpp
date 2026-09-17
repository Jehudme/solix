#include <unordered_set>
#include "solix/processes/binder.hpp"
#include "solix/compilation.hpp"
#include "solix/utilities/diagnostic.hpp"

namespace solix {

// ─── Error Reporting ────────────────────────────────────────────────────────

void Binder::record_error(Node *node, const std::string &msg) {
  Report r;
  r.severity = ReportSeverity::ERROR;
  r.code = "E_BIND";
  r.message = msg;
  r.source_path = "";
  r.line = node ? node->line : 0;
  r.column = node ? node->column : 0;
  if (node && node->source) {
    if (std::holds_alternative<std::filesystem::path>(*node->source)) {
      r.source_path = std::get<std::filesystem::path>(*node->source).string();
    } else {
      r.source_path = std::get<std::string>(*node->source);
    }
  }
  context.diagnostic->record_report(r);
}

// ─── Builtin Primitives Setup ────────────────────────────────────────────────

void Binder::setup_builtins() {
  // Allocate dummy ClassDeclaration nodes for primitive types.
  // These are intentionally leaked — the compiler is short-lived.
  auto make_builtin = [&](const std::string &name) {
    auto *decl = new ClassDeclaration(Token{}, name);
    decl->is_primitive = true;
    decl->mangled_name = name;
    global_scope.define(name, decl);
    return decl;
  };

  builtin_void = make_builtin("void");
  builtin_bool = make_builtin("bool");
  builtin_char = make_builtin("char");
  builtin_int8 = make_builtin("int8");
  builtin_uint8 = make_builtin("uint8");
  builtin_int16 = make_builtin("int16");
  builtin_uint16 = make_builtin("uint16");
  builtin_int32 = make_builtin("int32");
  builtin_uint32 = make_builtin("uint32");
  builtin_int64 = make_builtin("int64");
  builtin_uint64 = make_builtin("uint64");
  builtin_float32 = make_builtin("float32");
  builtin_float64 = make_builtin("float64");
}

// ─── Name Mangling ───────────────────────────────────────────────────────────

std::string Binder::mangle_method(MethodDeclaration *method) {
  if (method->is_native) return method->method_name;
  std::string mangled_name = method->method_name + "(";
  for (size_t i = 0; i < method->parameters.size(); ++i) {
    auto *var_decl = static_cast<VariableDeclaration *>(method->parameters[i].get());
    mangled_name += var_decl->type_info.to_string();
    if (i < method->parameters.size() - 1) mangled_name += ",";
  }
  mangled_name += ")";
  return mangled_name;}

std::string Binder::mangle_method_call(const std::string &base_name, const std::vector<TypeInfo> &arg_types) {
  std::string mangled_name = base_name + "(";
  for (size_t i = 0; i < arg_types.size(); ++i) {
    mangled_name += arg_types[i].to_string();
    if (i < arg_types.size() - 1) mangled_name += ",";
  }
  mangled_name += ")";
  return mangled_name;}

std::string Binder::mangle_constructor(const std::string &class_name, const std::vector<TypeInfo> &arg_types) {
  std::string mangled_name = class_name + ".ctor(";
  for (size_t i = 0; i < arg_types.size(); ++i) {
    mangled_name += arg_types[i].to_string();
    if (i < arg_types.size() - 1) mangled_name += ",";
  }
  mangled_name += ")";
  return mangled_name;}

// ─── Scope Management ────────────────────────────────────────────────────────

void Binder::enter_scope(SymbolTable *new_scope) {
  new_scope->parent = current_scope;
  current_scope = new_scope;
}

void Binder::exit_scope() {
  if (current_scope->parent) {
    current_scope = current_scope->parent;
  }
}

void Binder::declare_local(const std::string &name, Node *node) {
  if (current_scope->symbols.count(name)) {
    record_error(node,
                "Variable '" + name + "' is already defined in this scope.");
  }
  current_scope->define(name, node);
}

// ─── Pass 1a: Register Global Types & Aliases ────────────────────────────────
// Only registers packages, classes, enums, and aliases.
// Methods/fields/ctors are skipped here — they need types resolved first.

void Binder::register_global_symbols(Node *node, const std::string &prefix) {
  if (!node)
    return;
  std::string my_prefix = prefix;

  if (node->node_type == NodeType::PACKAGE_STMT) {
    auto *pkg = static_cast<PackageStatement *>(node);
    current_package = pkg->package_name + ".";
    pkg->mangled_name = pkg->package_name;
    global_scope.define(pkg->mangled_name, pkg);
  } else if (node->node_type == NodeType::ALIAS_STMT) {
    auto *alias = static_cast<AliasStatement *>(node);
    std::string full_name = prefix + alias->alias_name;
    if (global_scope.symbols.count(full_name)) {
      record_error(node, "Duplicate global symbol: " + full_name);
    }
    alias->mangled_name = full_name;
    global_scope.define(full_name, alias);
  } else if (node->node_type == NodeType::CLASS_DECL) {
    auto *class_decl = static_cast<ClassDeclaration *>(node);
    std::string full_name = prefix + class_decl->class_name;

    if (!class_decl->base_class_name.empty()) {
        class_decl->base_class_name = prefix + class_decl->base_class_name;
    }
    if (global_scope.symbols.count(full_name)) {
      record_error(node, "Duplicate global symbol: " + full_name);
    }
    class_decl->mangled_name = full_name;
    global_scope.define(full_name, class_decl);
    my_prefix = full_name + ".";
  } else if (node->node_type == NodeType::ENUM_DECL) {
    auto *enum_decl = static_cast<EnumDeclaration *>(node);
    std::string full_name = prefix + enum_decl->enum_name;
    if (global_scope.symbols.count(full_name)) {
      record_error(node, "Duplicate global symbol: " + full_name);
    }
    enum_decl->mangled_name = full_name;
    global_scope.define(full_name, enum_decl);
    my_prefix = full_name + ".";
  }

  // Recurse into class/enum bodies so nested types are also registered.
  if (node->node_type == NodeType::CLASS_DECL ||
      node->node_type == NodeType::ENUM_DECL) {
    for (const auto &child : node->children) {
      if (child)
        child->parent = node;
      register_global_symbols(child.get(), my_prefix);
    }
  }
}

// ─── Pass 1b: Register Members ───────────────────────────────────────────────
// Runs after all types/aliases are known so parameter types can be resolved
// before generating mangled method/constructor signatures.

void Binder::register_members(Node *node, const std::string &prefix) {
  if (!node)
    return;
  std::string my_prefix = prefix;

  if (node->node_type == NodeType::PACKAGE_STMT) {
    my_prefix = static_cast<PackageStatement *>(node)->package_name + ".";
  } else if (node->node_type == NodeType::CLASS_DECL) {
    auto *class_decl = static_cast<ClassDeclaration *>(node);
    my_prefix = prefix + class_decl->class_name + ".";
    current_class = class_decl;
  } else if (node->node_type == NodeType::ENUM_DECL) {
    my_prefix = prefix + static_cast<EnumDeclaration *>(node)->enum_name + ".";
  } else if (node->node_type == NodeType::FIELD_DECL) {
    auto *field = static_cast<FieldDeclaration *>(node);
    std::string full_name = prefix + field->field_name;
    field->mangled_name = full_name;
    global_scope.define(full_name, field);
  } else if (node->node_type == NodeType::METHOD_DECL) {
    auto *method = static_cast<MethodDeclaration *>(node);
    // Resolve parameter types now, so the mangled name uses canonical names.
    for (const auto &param : method->parameters) {
      auto *var_decl = static_cast<VariableDeclaration *>(param.get());
      var_decl->type_info = resolve_type(var_decl->type_info, var_decl);
    }
    std::string full_name = prefix + mangle_method(method);
    if (global_scope.symbols.count(full_name) && !method->is_native) {
      record_error(node, "Duplicate method signature: " + full_name);
    }
    method->mangled_name = full_name;
    global_scope.define(full_name, method);
  } else if (node->node_type == NodeType::CONSTRUCTOR_DECL) {
    auto *ctor = static_cast<ConstructorDeclaration *>(node);
    // Resolve parameter types now, so the mangled name uses canonical names.
    std::string full_name = prefix + "ctor(";
    for (size_t i = 0; i < ctor->parameters.size(); ++i) {
      auto *var_decl = static_cast<VariableDeclaration *>(ctor->parameters[i].get());
      var_decl->type_info = resolve_type(var_decl->type_info, var_decl);
      full_name += var_decl->type_info.to_string();
      if (i < ctor->parameters.size() - 1) full_name += ",";
    }
    full_name += ")";
    ctor->mangled_name = full_name;    global_scope.define(full_name, ctor);
  }

  if (node->node_type == NodeType::CLASS_DECL ||
      node->node_type == NodeType::ENUM_DECL) {
    for (const auto &child : node->children) {
      register_members(child.get(), my_prefix);
    }
    if (node->node_type == NodeType::CLASS_DECL) {
      current_class = nullptr;
    }
  }
}

// ─── Type Resolution ─────────────────────────────────────────────────────────

TypeInfo Binder::resolve_type(const TypeInfo &raw_type, Node *error_node) {
  if (raw_type.name.empty())
    return raw_type;

  Node *resolved = global_scope.resolve(raw_type.name);
  if (!resolved && !current_package.empty()) {
    resolved = global_scope.resolve(current_package + raw_type.name);
  }
  if (!resolved && current_class) {
    resolved =
        global_scope.resolve(current_class->mangled_name + "." + raw_type.name);
  }

  if (!resolved) {
    record_error(error_node, "Unknown type: " + raw_type.name);
  }

  TypeInfo result = raw_type;
  if (resolved->node_type == NodeType::ALIAS_STMT) {
    auto *alias = static_cast<AliasStatement *>(resolved);
    result.name = alias->target_type.name;
    result.array_depth += alias->target_type.array_depth;
    return resolve_type(result, error_node); // Recursively resolve aliases
  } else if (resolved->node_type == NodeType::CLASS_DECL ||
             resolved->node_type == NodeType::ENUM_DECL) {
    result.name = resolved->mangled_name; // Use fully qualified name
  }

  return result;
}

// ─── Pass 2: Type & Memory Binding ──────────────────────────────────────────

void Binder::bind_types_and_memory() {
  static_variable_index = 1; // 0 is reserved for null

  // Resolve all field types and assign static field indices.
  for (const auto &[name, node] : global_scope.symbols) {
    if (node->node_type == NodeType::FIELD_DECL) {
      auto *field = static_cast<FieldDeclaration *>(node);
      current_class =
          field->parent && field->parent->node_type == NodeType::CLASS_DECL
              ? static_cast<ClassDeclaration *>(field->parent)
              : nullptr;
      field->type_info = resolve_type(field->type_info, field);

      Node *type_decl = global_scope.resolve(field->type_info.name);
      field->is_reference_type = !(type_decl && type_decl->is_primitive &&
                                   field->type_info.array_depth == 0);

      if (field->is_static || !field->parent) {
        field->memory_index = static_variable_index++;
      }
      current_class = nullptr;
    }
  }

  // Resolve method return types.
  for (const auto &[name, node] : global_scope.symbols) {
    if (node->node_type == NodeType::METHOD_DECL) {
      auto *method = static_cast<MethodDeclaration *>(node);
      current_class =
          method->parent && method->parent->node_type == NodeType::CLASS_DECL
              ? static_cast<ClassDeclaration *>(method->parent)
              : nullptr;
      method->return_type = resolve_type(method->return_type, method);
      current_class = nullptr;
    }
  }

  // Calculate instance field offsets for each class with inheritance

  // Calculate vtables for each class
  std::unordered_map<std::string, std::vector<MethodDeclaration*>> vtables;
  std::unordered_set<std::string> vtable_calculated;
  
  std::function<void(ClassDeclaration*)> calculate_vtable = [&](ClassDeclaration* cls) {
      if (vtable_calculated.count(cls->mangled_name)) return;
      
      std::vector<MethodDeclaration*> vtable;
      if (!cls->base_class_name.empty()) {
          Node* base_node = global_scope.resolve(cls->base_class_name);
          if (base_node && base_node->node_type == NodeType::CLASS_DECL) {
              auto* base_cls = static_cast<ClassDeclaration*>(base_node);
              calculate_vtable(base_cls);
              vtable = vtables[base_cls->mangled_name];
          }
      }
      
      for (const auto &child : cls->children) {
          if (child->node_type == NodeType::METHOD_DECL) {
              auto* method = static_cast<MethodDeclaration*>(child.get());
              if (method->is_override) {
                  // Find the method in the vtable with the same signature
                  bool found = false;
                  for (size_t i = 0; i < vtable.size(); ++i) {
                      // Compare signature (method name and parameter types)
                      // A simplistic check: just compare the suffix after the class name
                      std::string base_sig = vtable[i]->mangled_name.substr(vtable[i]->mangled_name.rfind('.') + 1);
                      std::string drv_sig = method->mangled_name.substr(method->mangled_name.rfind('.') + 1);
                      if (base_sig == drv_sig) {
                          vtable[i] = method; // Override
                          method->vtable_index = i;
                          method->is_virtual = true;
                          found = true;
                          break;
                      }
                  }
                  if (!found) throw std::runtime_error("Method marked override but no base method found: " + method->mangled_name);
              } else if (method->is_virtual) {
                  method->vtable_index = vtable.size();
                  vtable.push_back(method);
              }
          }
      }
      vtables[cls->mangled_name] = vtable; cls->vtable = vtable;
      vtable_calculated.insert(cls->mangled_name);
  };

  for (const auto &[name, node] : global_scope.symbols) {
    if (node->node_type == NodeType::CLASS_DECL) {
      calculate_vtable(static_cast<ClassDeclaration *>(node));
    }
  }

  // Assign a unique vtable_id to each class that has a vtable
  int next_vtable_id = 0;
  for (const auto &[name, node] : global_scope.symbols) {
      if (node->node_type == NodeType::CLASS_DECL) {
          auto* cls = static_cast<ClassDeclaration*>(node);
          if (!vtables[cls->mangled_name].empty()) {
              cls->vtable_id = next_vtable_id++;
          }
      }
  }
  std::unordered_set<std::string> layout_calculated;
  std::function<int(ClassDeclaration*)> calculate_layout = [&](ClassDeclaration* cls) -> int {
      if (layout_calculated.count(cls->mangled_name)) return cls->instance_size;
      int offset = (!cls->base_class_name.empty() ? 0 : 1);
      if (!cls->base_class_name.empty()) {
          Node* base_node = global_scope.resolve(cls->base_class_name);
          if (base_node && base_node->node_type == NodeType::CLASS_DECL) {
              offset = calculate_layout(static_cast<ClassDeclaration*>(base_node));
          } else {
              throw std::runtime_error("Base class not found: " + cls->base_class_name);
          }
      }
      for (const auto &child : cls->children) {
        if (child->node_type == NodeType::FIELD_DECL) {
          auto *field = static_cast<FieldDeclaration *>(child.get());
          if (!field->is_static) {
            field->memory_index = offset++;
          }
        }
      }
      cls->instance_size = offset;
      layout_calculated.insert(cls->mangled_name);
      return offset;
  };

  for (const auto &[name, node] : global_scope.symbols) {
    if (node->node_type == NodeType::CLASS_DECL) {
      calculate_layout(static_cast<ClassDeclaration *>(node));
    }
  }


}
// ─── Binder::execute ─────────────────────────────────────────────────────────

void Binder::execute() {
  log_info("Starting Semantic Analysis (Binding)...");

  setup_builtins();

  // Pass 1a: Register packages, classes, enums, and aliases.
  for (const auto &[source, nodes] : context.nodes) {
    current_package = "";
    for (const auto &node : nodes) {
      if (node->node_type == NodeType::PACKAGE_STMT) {
        auto *pkg = static_cast<PackageStatement *>(node.get());
        current_package = pkg->package_name + ".";
        pkg->mangled_name = pkg->package_name;
        global_scope.define(pkg->mangled_name, pkg);
      } else {
        register_global_symbols(node.get(), current_package);
      }
    }
  }

  // Pass 1b: Register fields, methods, and constructors.
  // Now aliases are resolved, so parameter types can be correctly mangled.
  for (const auto &[source, nodes] : context.nodes) {
    current_package = "";
    for (const auto &node : nodes) {
      if (node->node_type == NodeType::PACKAGE_STMT) {
        current_package =
            static_cast<PackageStatement *>(node.get())->package_name + ".";
      } else {
        register_members(node.get(), current_package);
      }
    }
  }

  log_debug("Registered {} global symbols.", global_scope.symbols.size());

  // Pass 2: Resolve field/method return types and calculate memory layout.
  bind_types_and_memory();

  log_debug("Memory mapping complete. Static variables: {}",
            static_variable_index);

  // Pass 3: Bind execution logic (statement/expression bodies).
  for (const auto &[source, nodes] : context.nodes) {
    current_package = "";
    for (const auto &node : nodes) {
      if (node->node_type == NodeType::PACKAGE_STMT) {
        current_package =
            static_cast<PackageStatement *>(node.get())->package_name + ".";
      } else {
        bind_tree(node.get());
      }
    }
  }

  if (context.diagnostic->has_errors()) {
      std::string all_errors = "Semantic Analysis failed with errors:\n";
      for (const auto& r : context.diagnostic->get_reports()) {
          all_errors += r.message + "\n";
      }
      throw BindError(all_errors);
  }

  log_info("Semantic Analysis completed successfully.");
}

// ─── Pass 3 Tree Traversal ───────────────────────────────────────────────────

void Binder::bind_tree(Node *root) {
  if (!root)
    return;

  ClassDeclaration *previous_class = current_class;

  if (root->node_type == NodeType::CLASS_DECL) {
    current_class = static_cast<ClassDeclaration *>(root);

  } else if (root->node_type == NodeType::METHOD_DECL) {
    current_method = static_cast<MethodDeclaration *>(root);
    local_variable_index = 0;

    current_method->return_type =
        resolve_type(current_method->return_type, current_method);

    SymbolTable method_scope;
    enter_scope(&method_scope);

    // Inject implicit 'this' for non-static methods.
    if (!current_method->is_static && current_class) {
      auto *this_decl = new VariableDeclaration(
          Token{}, "this", TypeInfo{current_class->mangled_name, 0});
      this_decl->memory_index = local_variable_index++;
      this_decl->is_reference_type = true;
      declare_local("this", this_decl);
    }

    for (const auto &param : current_method->parameters) {
      auto *param_var = static_cast<VariableDeclaration *>(param.get());
      param_var->type_info = resolve_type(param_var->type_info, param_var);

      Node *type_decl = global_scope.resolve(param_var->type_info.name);
      param_var->is_reference_type = !(type_decl && type_decl->is_primitive &&
                                       param_var->type_info.array_depth == 0);

      param_var->memory_index = local_variable_index++;
      declare_local(param_var->var_name, param_var);
    }

    for (const auto &child : current_method->children) {
      bind_node(child.get());
    }

    current_method->frame_size = local_variable_index;
    exit_scope();
    current_method = nullptr;
    current_class = previous_class;
    return; // Children already handled above.

  } else if (root->node_type == NodeType::CONSTRUCTOR_DECL) {
    auto *ctor = static_cast<ConstructorDeclaration *>(root);
    local_variable_index = 0;

    SymbolTable constructor_scope;
    enter_scope(&constructor_scope);

    // Inject implicit 'this' for constructors.
    if (current_class) {
      auto *this_decl = new VariableDeclaration(
          Token{}, "this", TypeInfo{current_class->mangled_name, 0});
      this_decl->memory_index = local_variable_index++;
      this_decl->is_reference_type = true;
      declare_local("this", this_decl);
    }

    for (const auto &param : ctor->parameters) {
      auto *param_var = static_cast<VariableDeclaration *>(param.get());
      param_var->type_info = resolve_type(param_var->type_info, param_var);

      Node *type_decl = global_scope.resolve(param_var->type_info.name);
      param_var->is_reference_type = !(type_decl && type_decl->is_primitive &&
                                       param_var->type_info.array_depth == 0);

      param_var->memory_index = local_variable_index++;
      declare_local(param_var->var_name, param_var);
    }

    for (const auto &child : ctor->children) {
      bind_node(child.get());
    }

    ctor->frame_size = local_variable_index;
    exit_scope();
    current_class = previous_class;
    return; // Children already handled above.
  }

  // Default: recurse into children.
  for (const auto &child : root->children) {
    if (child)
      bind_tree(child.get());
  }

  current_class = previous_class;
}

// ─── Statement Binding ───────────────────────────────────────────────────────

void Binder::bind_node(Node *node) {
  if (!node)
    return;

  if (node->node_type == NodeType::BLOCK) {
    SymbolTable block_scope;
    enter_scope(&block_scope);
    for (const auto &child : node->children) {
      bind_node(child.get());
    }
    exit_scope();

  } else if (node->node_type == NodeType::VAR_DECL) {
    auto *var = static_cast<VariableDeclaration *>(node);
    var->type_info = resolve_type(var->type_info, var);

    Node *type_decl = global_scope.resolve(var->type_info.name);
    var->is_reference_type = !(type_decl && type_decl->is_primitive &&
                               var->type_info.array_depth == 0);

    if (var->initializer) {
      TypeInfo initializer_type = evaluate_expression(var->initializer.get());
      if (!is_assignable(var->type_info, initializer_type)) {
        record_error(node, "Type mismatch in variable declaration: expected '" +
                              var->type_info.name + "', got '" +
                              initializer_type.name + "'");
      }
    }
    var->memory_index = local_variable_index++;
    declare_local(var->var_name, var);

  } else if (node->node_type == NodeType::IF_STMT) {
    auto *if_stmt = static_cast<IfStatement *>(node);
    TypeInfo condition_type = evaluate_expression(if_stmt->condition.get());
    if (condition_type.name != "bool")
      record_error(node, "Condition must be bool");
    bind_node(if_stmt->then_branch.get());
    if (if_stmt->else_branch)
      bind_node(if_stmt->else_branch.get());

  } else if (node->node_type == NodeType::WHILE_STMT) {
    auto *while_stmt = static_cast<WhileStatement *>(node);
    TypeInfo condition_type = evaluate_expression(while_stmt->condition.get());
    if (condition_type.name != "bool")
      record_error(node, "Condition must be bool");
    loop_depth++;
    bind_node(while_stmt->body.get());
    loop_depth--;

  } else if (node->node_type == NodeType::DO_WHILE_STMT) {
    auto *do_while_stmt = static_cast<DoWhileStatement *>(node);
    loop_depth++;
    bind_node(do_while_stmt->body.get());
    loop_depth--;
    TypeInfo condition_type =
        evaluate_expression(do_while_stmt->condition.get());
    if (condition_type.name != "bool")
      record_error(node, "Condition must be bool");

  } else if (node->node_type == NodeType::FOR_STMT) {
    auto *for_stmt = static_cast<ForStatement *>(node);
    SymbolTable for_scope;
    enter_scope(&for_scope);
    if (for_stmt->initialization)
      bind_node(for_stmt->initialization.get());
    if (for_stmt->condition) {
      TypeInfo condition_type = evaluate_expression(for_stmt->condition.get());
      if (condition_type.name != "bool")
        record_error(node, "Condition must be bool");
    }
    if (for_stmt->iteration)
      evaluate_expression(for_stmt->iteration.get());
    loop_depth++;
    bind_node(for_stmt->body.get());
    loop_depth--;
    exit_scope();

  } else if (node->node_type == NodeType::SWITCH_STMT) {
    auto *switch_stmt = static_cast<SwitchStatement *>(node);
    evaluate_expression(switch_stmt->condition.get());
    switch_depth++;
    for (const auto &case_node : switch_stmt->children) {
      bind_node(case_node.get());
    }
    switch_depth--;

  } else if (node->node_type == NodeType::CASE_STMT) {
    auto *case_stmt = static_cast<CaseStatement *>(node);
    if (case_stmt->case_value)
      evaluate_expression(case_stmt->case_value.get());
    for (const auto &child : case_stmt->children) {
      bind_node(child.get());
    }

  } else if (node->node_type == NodeType::RETURN_STMT) {
    auto *return_stmt = static_cast<ReturnStatement *>(node);
    if (return_stmt->value) {
      TypeInfo return_type = evaluate_expression(return_stmt->value.get());
      if (current_method && !is_assignable(current_method->return_type, return_type)) {
        record_error(node, "Return type mismatch: expected '" +
                              current_method->return_type.name + "', got '" +
                              return_type.name + "'");
      }
    } else if (current_method && current_method->return_type.name != "void") {
      record_error(node, "Must return a value from non-void method");
    }

  } else if (node->node_type == NodeType::BREAK_STMT) {
    if (loop_depth == 0 && switch_depth == 0) {
      record_error(node, "Break must be inside a loop or switch");
    }
  } else if (node->node_type == NodeType::CONTINUE_STMT) {
    if (loop_depth == 0)
      record_error(node, "Continue must be inside a loop");

  } else if (node->node_type == NodeType::EXPR_STMT) {
    auto *expr_stmt = static_cast<ExpressionStatement *>(node);
    evaluate_expression(expr_stmt->expression.get());
  }
}

// ─── Expression Evaluation ───────────────────────────────────────────────────

bool Binder::is_assignable(const TypeInfo& target, const TypeInfo& source) {
    if (target == source) return true;
    if (target.array_depth != source.array_depth) return false;
    
    // Check if source inherits from target
    Node* src_node = global_scope.resolve(source.name);
    while (src_node && src_node->node_type == NodeType::CLASS_DECL) {
        auto* cls = static_cast<ClassDeclaration*>(src_node);
        if (cls->mangled_name == target.name) return true;
        if (cls->base_class_name.empty()) break;
        src_node = global_scope.resolve(cls->base_class_name);
    }
    return false;
}

TypeInfo Binder::evaluate_expression(Node *expr) {
  if (!expr)
    return {"void", 0};

  if (expr->node_type == NodeType::LITERAL) {
    auto *lit = static_cast<LiteralNode *>(expr);
    if (std::holds_alternative<int64_t>(lit->value))
      expr->expression_type = {"int32", 0};
    else if (std::holds_alternative<double>(lit->value))
      expr->expression_type = {"float64", 0};
    else if (std::holds_alternative<std::string>(lit->value))
      expr->expression_type = {"char", 1};
    else
      expr->expression_type = {"void", 0};
    return expr->expression_type;

  } else if (expr->node_type == NodeType::IDENTIFIER) {
    auto *id = static_cast<IdentifierNode *>(expr);

    // Intrinsic boolean/null keywords are lexed as identifiers.
    if (id->name == "true" || id->name == "false") {
      expr->expression_type = {"bool", 0};
      return expr->expression_type;
    }
    if (id->name == "null") {
      expr->expression_type = {"void", 0};
      return expr->expression_type;
    }

    // Look up from innermost to outermost scope, then global.
    Node *declaration = current_scope->resolve(id->name);
    if (!declaration && current_class) {
      declaration =
          global_scope.resolve(current_class->mangled_name + "." + id->name);
    }
    if (!declaration && !current_package.empty()) {
      declaration = global_scope.resolve(current_package + id->name);
    }
    if (!declaration) {
      declaration = global_scope.resolve(id->name);
    }

    if (!declaration)
      { record_error(expr, "Undefined identifier: " + id->name); return {"void", 0}; }
    expr->resolved_declaration = declaration;

    if (declaration->node_type == NodeType::VAR_DECL) {
      expr->expression_type =
          static_cast<VariableDeclaration *>(declaration)->type_info;
    } else if (declaration->node_type == NodeType::FIELD_DECL) {
      expr->expression_type =
          static_cast<FieldDeclaration *>(declaration)->type_info;
    } else if (declaration->node_type == NodeType::CLASS_DECL ||
               declaration->node_type == NodeType::ENUM_DECL) {
      expr->expression_type = {declaration->mangled_name, 0};
    } else {
      { record_error(expr, "Invalid identifier usage: " + id->name); return {"void", 0}; }
    }
    return expr->expression_type;

  } else if (expr->node_type == NodeType::BINARY_EXPR) {
    auto *bin = static_cast<BinaryExpression *>(expr);
    TypeInfo left_type = evaluate_expression(bin->left.get());
    TypeInfo right_type = evaluate_expression(bin->right.get());

    Node *left_decl = global_scope.resolve(left_type.name);
    if (left_decl && left_decl->node_type == NodeType::CLASS_DECL) {
        std::string op_name = "operator";
        if (bin->op == TokenType::OPERATOR_PLUS) op_name += "+";
        else if (bin->op == TokenType::OPERATOR_MINUS) op_name += "-";
        else if (bin->op == TokenType::OPERATOR_MULTIPLY) op_name += "*";
        else if (bin->op == TokenType::OPERATOR_DIVIDE) op_name += "/";
        else goto primitive_fallback;
        
        std::string base_name = left_decl->mangled_name + "." + op_name;
        std::vector<TypeInfo> args = {right_type};
        std::string mangled = mangle_method_call(base_name, args);
        Node* method_decl = global_scope.resolve(mangled);
        if (!method_decl) goto primitive_fallback;
        
        bin->overloaded_operator = method_decl;
        expr->expression_type = static_cast<MethodDeclaration*>(method_decl)->return_type;
        return expr->expression_type;
    }
    
    primitive_fallback:
    if (left_type != right_type) {
      record_error(expr, "Binary operands type mismatch: '" + left_type.name +
                            "' vs '" + right_type.name + "'");
    }
    if (bin->op >= TokenType::OPERATOR_EQUAL &&
        bin->op <= TokenType::OPERATOR_GREATER_EQUAL) {
      expr->expression_type = {"bool", 0};
    } else {
      expr->expression_type = left_type;
    }
    return expr->expression_type;

  } else if (expr->node_type == NodeType::UNARY_EXPR) {
    auto *unary = static_cast<UnaryExpression *>(expr);
    expr->expression_type = evaluate_expression(unary->operand.get());
    return expr->expression_type;

  } else if (expr->node_type == NodeType::TERNARY_EXPR) {
    auto *ternary = static_cast<TernaryExpression *>(expr);
    TypeInfo condition_type = evaluate_expression(ternary->condition.get());
    if (condition_type.name != "bool")
      record_error(expr, "Ternary condition must be bool");
    TypeInfo true_type = evaluate_expression(ternary->true_branch.get());
    TypeInfo false_type = evaluate_expression(ternary->false_branch.get());
    if (true_type != false_type) {
      record_error(expr, "Ternary branches must have the same type");
    }
    expr->expression_type = true_type;
    return expr->expression_type;

  } else if (expr->node_type == NodeType::ASSIGNMENT_EXPR) {
    auto *assign = static_cast<AssignmentExpression *>(expr);
    TypeInfo target_type = evaluate_expression(assign->target.get());
    TypeInfo value_type = evaluate_expression(assign->value.get());

    Node *left_decl = global_scope.resolve(target_type.name);
    if (left_decl && left_decl->node_type == NodeType::CLASS_DECL) {
        std::string op_name = "operator=";
        std::string base_name = left_decl->mangled_name + "." + op_name;
        std::vector<TypeInfo> args = {value_type};
        std::string mangled = mangle_method_call(base_name, args);
        Node* method_decl = global_scope.resolve(mangled);
        if (method_decl) {
            assign->overloaded_operator = method_decl;
            expr->expression_type = static_cast<MethodDeclaration*>(method_decl)->return_type;
            return expr->expression_type;
        }
    }
    if (!is_assignable(target_type, value_type)) {
      record_error(expr, "Assignment type mismatch: '" + target_type.name +
                            "' = '" + value_type.name + "'");
    }
    expr->expression_type = target_type;
    return expr->expression_type;

  } else if (expr->node_type == NodeType::ARRAY_LITERAL) {
    auto *array_lit = static_cast<ArrayLiteralExpression *>(expr);
    TypeInfo element_type = {"void", 0};
    for (const auto &element : array_lit->elements) {
      TypeInfo current_element_type = evaluate_expression(element.get());
      if (element_type.name == "void") {
        element_type = current_element_type;
      } else if (element_type != current_element_type) {
        record_error(expr, "Mixed types in array literal");
      }
    }
    element_type.array_depth++;
    expr->expression_type = element_type;
    return expr->expression_type;

  } else if (expr->node_type == NodeType::MEMBER_ACCESS) {
    auto *member_access = static_cast<MemberAccessExpression *>(expr);
    TypeInfo object_type = evaluate_expression(member_access->object.get());

    // Array length property.
    if (object_type.array_depth > 0) {
      if (member_access->member_name == "length") {
        expr->expression_type = {"int32", 0};
        return expr->expression_type;
      }
      record_error(expr, "Arrays only have the 'length' property");
    }

    Node *type_decl = global_scope.resolve(object_type.name);
    if (!type_decl)
      { record_error(expr, "Cannot access members on unknown type: " +  object_type.name); return {"void", 0}; }

    if (type_decl->node_type == NodeType::CLASS_DECL) {
      auto *class_decl = static_cast<ClassDeclaration *>(type_decl);
      Node *member_decl = global_scope.resolve(class_decl->mangled_name + "." + member_access->member_name);
      ClassDeclaration* current_resolve_class = class_decl;
      while (!member_decl && current_resolve_class && !current_resolve_class->base_class_name.empty()) {
          Node* base_node = global_scope.resolve(current_resolve_class->base_class_name);
          if (!base_node) break;
          current_resolve_class = static_cast<ClassDeclaration*>(base_node);
          member_decl = global_scope.resolve(current_resolve_class->mangled_name + "." + member_access->member_name);
      }
      if (!member_decl) {
        { record_error(expr, "Member not found: " +  member_access->member_name +
                              " on " + class_decl->mangled_name); return {"void", 0}; }
      }
      if (!check_access(member_decl, current_resolve_class, expr)) return {"void", 0};
      expr->resolved_declaration = member_decl;
      if (member_decl->node_type == NodeType::FIELD_DECL) {
        expr->expression_type =
            static_cast<FieldDeclaration *>(member_decl)->type_info;
        return expr->expression_type;
      }
      // Nested class or enum access — return the type itself so further chained
      // access works.
      if (member_decl->node_type == NodeType::CLASS_DECL ||
          member_decl->node_type == NodeType::ENUM_DECL) {
        expr->expression_type = {member_decl->mangled_name, 0};
        return expr->expression_type;
      }
    } else if (type_decl->node_type == NodeType::ENUM_DECL) {
      auto* enum_decl = static_cast<EnumDeclaration*>(type_decl);
      int32_t e_val = -1;
      for (size_t i = 0; i < enum_decl->members.size(); ++i) {
          if (enum_decl->members[i] == member_access->member_name) {
              e_val = i;
              break;
          }
      }
      if (e_val == -1) {
          record_error(expr, "Undefined enum member: " + member_access->member_name);
      }
      member_access->resolved_declaration = enum_decl;
      member_access->enum_value = e_val;
      expr->expression_type = object_type;
      return expr->expression_type;
    }

    record_error(expr, "Invalid member access on type: " + object_type.name);

  } else if (expr->node_type == NodeType::METHOD_CALL) {
    auto *method_call = static_cast<MethodCallExpression *>(expr);

    std::vector<TypeInfo> argument_types;
    for (const auto &arg : method_call->arguments) {
      argument_types.push_back(evaluate_expression(arg.get()));
    }

    if (method_call->callee->node_type == NodeType::MEMBER_ACCESS) {
      auto *member_access =
          static_cast<MemberAccessExpression *>(method_call->callee.get());
      TypeInfo receiver_type = evaluate_expression(member_access->object.get());
      ClassDeclaration* current_resolve_class = nullptr;
      Node* type_decl = global_scope.resolve(receiver_type.name);
      if (type_decl && type_decl->node_type == NodeType::CLASS_DECL) {
          current_resolve_class = static_cast<ClassDeclaration*>(type_decl);
      }
      
      Node *method_decl = nullptr;
      std::string base_name;
      std::string mangled_name;
      
      while (!method_decl && current_resolve_class) {
          base_name = current_resolve_class->mangled_name + "." + member_access->member_name;
          mangled_name = mangle_method_call(base_name, argument_types);
          method_decl = global_scope.resolve(mangled_name);
          if (!method_decl) method_decl = global_scope.resolve(base_name);
          
          if (!method_decl && !current_resolve_class->base_class_name.empty()) {
              Node* base_node = global_scope.resolve(current_resolve_class->base_class_name);
              if (base_node) current_resolve_class = static_cast<ClassDeclaration*>(base_node);
              else break;
          } else {
              break;
          }
      }

      if (!method_decl) {
          record_error(expr, "No matching method: " +  member_access->member_name + " on " + receiver_type.name); 
          return {"void", 0}; 
      }
      if (!check_access(method_decl, current_resolve_class, expr)) return {"void", 0};
      method_call->resolved_declaration = method_decl;
      if (method_decl->node_type == NodeType::METHOD_DECL) {
          auto* m = static_cast<MethodDeclaration*>(method_decl);
          if (m->is_virtual && member_access && !member_access->is_scope_resolution) {
              method_call->is_virtual_call = true;
          }
          expr->expression_type = m->return_type;
      } else {
          expr->expression_type = {"void", 0};
      }
      return expr->expression_type;

    } else {
      // Local method call — must be inside a class.
      if (!current_class) {
        record_error(expr, "Local method calls must be inside a class");
        return {"void", 0};
      }
      auto *id = static_cast<IdentifierNode *>(method_call->callee.get());
      std::string base_name;
      std::string mangled_name;
      Node *method_decl = nullptr;
      ClassDeclaration* current_resolve_class = current_class;

      if (id->name == "super") {
          if (current_class->base_class_name.empty()) { record_error(expr, "Cannot call super() in a class without a base class"); return {"void", 0}; }
          Node* base_node = global_scope.resolve(current_class->base_class_name);
          if (!base_node) { record_error(expr, "Base class not found"); return {"void", 0}; }
          current_resolve_class = static_cast<ClassDeclaration*>(base_node);
          base_name = current_resolve_class->mangled_name + ".ctor";
          mangled_name = mangle_method_call(base_name, argument_types);
          method_decl = global_scope.resolve(mangled_name);
          if (!method_decl) method_decl = global_scope.resolve(base_name);
      } else {
          while (!method_decl && current_resolve_class) {
              base_name = current_resolve_class->mangled_name + "." + id->name;
              mangled_name = mangle_method_call(base_name, argument_types);
              method_decl = global_scope.resolve(mangled_name);
              if (!method_decl) method_decl = global_scope.resolve(base_name);
              
              if (!method_decl && !current_resolve_class->base_class_name.empty()) {
                  Node* base_node = global_scope.resolve(current_resolve_class->base_class_name);
                  if (base_node) current_resolve_class = static_cast<ClassDeclaration*>(base_node);
                  else break;
              } else {
                  break;
              }
          }
      }

      if (!method_decl) {
          record_error(expr, "No matching method: " + id->name); 
          return {"void", 0}; 
      }
      if (!check_access(method_decl, current_resolve_class, expr)) return {"void", 0};
      method_call->resolved_declaration = method_decl;
      if (method_decl->node_type == NodeType::METHOD_DECL) {
          auto* m = static_cast<MethodDeclaration*>(method_decl);
          if (m->is_virtual && id->name != "super") {
              method_call->is_virtual_call = true;
          }
          expr->expression_type = m->return_type;
      } else {
          expr->expression_type = {"void", 0};
      }
      return expr->expression_type;
    }

  } else if (expr->node_type == NodeType::NEW_INSTANCE) {
    auto *new_instance = static_cast<NewInstanceExpression *>(expr);
    new_instance->type_info =
        resolve_type(new_instance->type_info, new_instance);

    std::vector<TypeInfo> argument_types;
    for (const auto &arg : new_instance->arguments) {
      argument_types.push_back(evaluate_expression(arg.get()));
    }

    // Try exact constructor match first.
    std::string mangled_ctor =
        mangle_constructor(new_instance->type_info.name, argument_types);
    Node *ctor = global_scope.resolve(mangled_ctor);

    // Fallback: find any constructor with the same arity (allows numeric
    // coercion).
    if (!ctor && !argument_types.empty()) {
      std::string ctor_prefix = new_instance->type_info.name + ".ctor(";
      int expected_param_count = static_cast<int>(argument_types.size());
      for (const auto &[sym_name, sym_node] : global_scope.symbols) {
        if (sym_node->node_type == NodeType::CONSTRUCTOR_DECL &&
            sym_name.find(ctor_prefix) == 0) {
          auto *candidate = static_cast<ConstructorDeclaration *>(sym_node);
          if (static_cast<int>(candidate->parameters.size()) ==
              expected_param_count) {
            ctor = sym_node;
            break;
          }
        }
      }
    }

    if (!ctor && !argument_types.empty()) {
      record_error(expr, "No matching constructor: " + mangled_ctor);
    }
    if (ctor) new_instance->resolved_declaration = ctor; else new_instance->resolved_declaration =
        global_scope.resolve(new_instance->type_info.name);
    expr->expression_type = new_instance->type_info;
    return expr->expression_type;

  } else if (expr->node_type == NodeType::ARRAY_CREATION) {
    auto *array_creation = static_cast<ArrayCreationExpression *>(expr);
    array_creation->type_info =
        resolve_type(array_creation->type_info, array_creation);
    TypeInfo size_type = evaluate_expression(array_creation->size.get());
    if (size_type.name != "int32")
      record_error(expr, "Array size must be int32");
    expr->expression_type = array_creation->type_info;
    expr->expression_type.array_depth++;
    return expr->expression_type;

  } else if (expr->node_type == NodeType::ARRAY_ACCESS) {
    auto *array_access = static_cast<ArrayAccessExpression *>(expr);
    TypeInfo array_type = evaluate_expression(array_access->array.get());
    if (array_type.array_depth == 0)
      record_error(expr, "Cannot index a non-array value");
    TypeInfo index_type = evaluate_expression(array_access->index.get());
    if (index_type.name != "int32")
      record_error(expr, "Array index must be int32");
    expr->expression_type = array_type;
    expr->expression_type.array_depth--;
    return expr->expression_type;

  } else if (expr->node_type == NodeType::CAST_EXPR) {
    auto *cast_expr = static_cast<CastExpression *>(expr);
    TypeInfo source_type = evaluate_expression(cast_expr->expression.get());
    cast_expr->target_type = resolve_type(cast_expr->target_type, cast_expr);
    
    auto is_primitive = [&](const TypeInfo& t) {
        if (t.array_depth > 0) return false;
        Node* decl = global_scope.resolve(t.name);
        return !decl || decl->is_primitive;
    };
    
    bool target_prim = is_primitive(cast_expr->target_type);
    bool source_prim = is_primitive(source_type);
    
    if (target_prim && source_prim) {
        // primitive to primitive allowed
    } else if (target_prim != source_prim) {
        record_error(expr, "Cannot cast between primitive and class types");
    } else {
        if (is_assignable(cast_expr->target_type, source_type)) {
            // Upcast: allowed
        } else if (is_assignable(source_type, cast_expr->target_type)) {
            // Downcast
            log_info("NOTE: Downcast from '" + source_type.name + "' to '" + cast_expr->target_type.name + "' is unchecked until Phase 10.");
        } else {
            record_error(expr, "Cannot cast '" + source_type.name + "' to '" + cast_expr->target_type.name + "': no inheritance relationship");
        }
    }
    
    expr->expression_type = cast_expr->target_type;
    return expr->expression_type;
  }

  expr->expression_type = {"void", 0};
  return expr->expression_type;
}



bool Binder::check_access(Node* member_decl, Node* owner_class_node, Node* expr) {
    if (!owner_class_node || owner_class_node->node_type != NodeType::CLASS_DECL) return true;
    ClassDeclaration* owner_class = static_cast<ClassDeclaration*>(owner_class_node);

    TokenType access = TokenType::KEYWORD_PUBLIC;
    if (member_decl->node_type == NodeType::FIELD_DECL) access = static_cast<FieldDeclaration*>(member_decl)->access_modifier;
    else if (member_decl->node_type == NodeType::METHOD_DECL) access = static_cast<MethodDeclaration*>(member_decl)->access_modifier;
    
    if (access == TokenType::KEYWORD_PUBLIC) return true;

    if (access == TokenType::KEYWORD_PRIVATE) {
        if (current_class == owner_class) return true;
        record_error(expr, "Cannot access private member of class '" + owner_class->class_name + "'");
        return false;
    }

    if (access == TokenType::KEYWORD_PROTECTED) {
        if (!current_class) {
            record_error(expr, "Cannot access protected member outside a class");
            return false;
        }
        ClassDeclaration* iter = current_class;
        while (iter) {
            if (iter == owner_class) return true;
            if (iter->base_class_name.empty()) break;
            Node* base_node = global_scope.resolve(iter->base_class_name);
            if (!base_node) break;
            iter = static_cast<ClassDeclaration*>(base_node);
        }
        record_error(expr, "Cannot access protected member of class '" + owner_class->class_name + "'");
        return false;
    }

    if (access == TokenType::KEYWORD_INTERNAL) {
        auto get_package = [](const std::string& name) {
            size_t last_dot = name.rfind('.');
            if (last_dot != std::string::npos) return name.substr(0, last_dot + 1);
            return std::string("");
        };
        std::string owner_pkg = get_package(owner_class->mangled_name);
        if (owner_pkg == current_package) return true;
        
        record_error(expr, "Cannot access internal member of class '" + owner_class->class_name + "' from a different package");
        return false;
    }

    return true;
}
} // namespace solix
