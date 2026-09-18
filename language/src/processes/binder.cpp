
#include "solix/processes/binder.hpp"
#include "solix/compilation.hpp"
#include "solix/utilities/diagnostic.hpp"
#include <unordered_set>

#include "solix/processes/template_substitution.hpp"

namespace solix {
Node* Binder::instantiate_template(const std::string& template_name, const std::vector<TypeInfo>& type_args, Node* error_node) {
    if (!template_registry.count(template_name)) {
        record_error(error_node, "Unknown template: " + template_name);
        return nullptr;
    }
    
    Node* blueprint = template_registry[template_name];
    
    std::string mangled_name = template_name + "<";
    for (size_t i = 0; i < type_args.size(); ++i) {
        mangled_name += type_args[i].to_string();
        if (i < type_args.size() - 1) mangled_name += ",";
    }
    mangled_name += ">";
    
    if (global_scope.symbols.count(mangled_name)) {
        return global_scope.symbols[mangled_name];
    }
    
    std::vector<std::string> tparams;
    if (blueprint->node_type == NodeType::CLASS_DECL) tparams = static_cast<ClassDeclaration*>(blueprint)->template_parameters;
    else if (blueprint->node_type == NodeType::ALIAS_STMT) tparams = static_cast<AliasStatement*>(blueprint)->template_parameters;
    else if (blueprint->node_type == NodeType::METHOD_DECL) tparams = static_cast<MethodDeclaration*>(blueprint)->template_parameters;
    
    if (type_args.size() != tparams.size()) {
        record_error(error_node, "Template " + template_name + " expects " + std::to_string(tparams.size()) + " arguments, got " + std::to_string(type_args.size()));
        return nullptr;
    }
    
    std::unordered_map<std::string, TypeInfo> substitutions;
    for (size_t i = 0; i < tparams.size(); ++i) {
        substitutions[tparams[i]] = type_args[i];
    }
    
    auto clone_ptr = blueprint->clone();
    Node* clone = clone_ptr.get();
    
    if (clone->node_type == NodeType::CLASS_DECL) {
        static_cast<ClassDeclaration*>(clone)->template_parameters.clear();
        static_cast<ClassDeclaration*>(clone)->class_name = mangled_name;
    } else if (clone->node_type == NodeType::ALIAS_STMT) {
        static_cast<AliasStatement*>(clone)->template_parameters.clear();
        static_cast<AliasStatement*>(clone)->alias_name = mangled_name;
    } else if (clone->node_type == NodeType::METHOD_DECL) {
        static_cast<MethodDeclaration*>(clone)->template_parameters.clear();
        static_cast<MethodDeclaration*>(clone)->method_name = mangled_name;
    }
    
    TemplateSubstitutionVisitor substitutor(substitutions);
    substitutor.execute(clone);
    
    // Inject the newly minted concrete node into context.nodes and global_scope!
    context.nodes[std::string("__instantiated_templates")].push_back(std::move(clone_ptr));
    instantiated_templates.insert(mangled_name);
    
    // Run Pass 1 (REGISTER_GLOBALS)
    // Run Pass 2 (REGISTER_MEMBERS)
    // Run Pass 3 (BIND_EXECUTION)
    
    std::string current_pkg_copy = current_package;
    std::string my_prefix = ""; 
    auto last_dot = template_name.rfind('.');
    if (last_dot != std::string::npos) my_prefix = template_name.substr(0, last_dot + 1);

    if (clone->node_type == NodeType::CLASS_DECL) {
        static_cast<ClassDeclaration*>(clone)->class_name = mangled_name.substr(my_prefix.length());
        register_global_symbols(clone, my_prefix);
        register_members(clone, my_prefix);
        if (current_pass == BinderPass::BIND_EXECUTION) {
            bind_tree(clone);
        }
    } else if (clone->node_type == NodeType::ALIAS_STMT) {
        static_cast<AliasStatement*>(clone)->alias_name = mangled_name.substr(my_prefix.length());
        register_global_symbols(clone, my_prefix);
        if (current_pass == BinderPass::BIND_EXECUTION) {
            bind_tree(clone);
        }
    } else if (clone->node_type == NodeType::METHOD_DECL) {
        static_cast<MethodDeclaration*>(clone)->method_name = mangled_name.substr(my_prefix.length());
        // For methods, register_members defines the symbol
        register_members(clone, my_prefix);
        if (current_pass == BinderPass::BIND_EXECUTION) {
            bind_tree(clone);
        }
    }
    
    return global_scope.resolve(mangled_name);
}
} // namespace solix


namespace solix {

// ─── Error Reporting ────────────────────────────────────────────────────────

void Binder::record_error(Node *node, const std::string &msg) {
  Report report;
  report.severity = ReportSeverity::ERROR;
  report.code = "E_BIND";
  report.message = msg;
  report.source_path = "";
  report.line = node ? node->line : 0;
  report.column = node ? node->column : 0;
  if (node && node->source) {
    if (std::holds_alternative<std::filesystem::path>(*node->source)) {
      report.source_path =
          std::get<std::filesystem::path>(*node->source).string();
    } else {
      report.source_path = std::get<std::string>(*node->source);
    }
  }
  context.diagnostic->record_report(report);
}

// ─── Builtin Primitives Setup ────────────────────────────────────────────────

void Binder::setup_builtins() {
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
  std::string mangled_name = method->method_name + "(";
  for (size_t i = 0; i < method->parameters.size(); ++i) {
    auto *var_decl =
        static_cast<VariableDeclaration *>(method->parameters[i].get());
    mangled_name += var_decl->type_info.to_string();
    if (i < method->parameters.size() - 1)
      mangled_name += ",";
  }
  mangled_name += ")";
  return mangled_name;
}

std::string Binder::mangle_method_call(const std::string &base_name,
                                       const std::vector<TypeInfo> &arg_types) {
  std::string mangled_name = base_name + "(";
  for (size_t i = 0; i < arg_types.size(); ++i) {
    mangled_name += arg_types[i].to_string();
    if (i < arg_types.size() - 1)
      mangled_name += ",";
  }
  mangled_name += ")";
  return mangled_name;
}

std::string Binder::mangle_constructor(const std::string &class_name,
                                       const std::vector<TypeInfo> &arg_types) {
  std::string mangled_name = class_name + ".ctor(";
  for (size_t i = 0; i < arg_types.size(); ++i) {
    mangled_name += arg_types[i].to_string();
    if (i < arg_types.size() - 1)
      mangled_name += ",";
  }
  mangled_name += ")";
  return mangled_name;
}

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

// ─── Pass Drivers ────────────────────────────────────────────────────────────

void Binder::register_global_symbols(Node *node, const std::string &prefix) {
  if (!node)
    return;
  current_pass = BinderPass::REGISTER_GLOBALS;
  current_prefix = prefix;
  node->accept(*this);
}

void Binder::register_members(Node *node, const std::string &prefix) {
  if (!node)
    return;
  current_pass = BinderPass::REGISTER_MEMBERS;
  current_prefix = prefix;
  node->accept(*this);
}

void Binder::bind_node(Node *node) {
  if (!node)
    return;
  current_pass = BinderPass::BIND_EXECUTION;
  node->accept(*this);
}

TypeInfo Binder::evaluate_expression(Node *expr) {
  if (!expr)
    return {"void", 0};
  current_pass = BinderPass::EVALUATE_EXPRESSION;
  expr->accept(*this);
  return evaluated_type;
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

  TypeInfo result = raw_type;

  if (!resolved && !result.type_args.empty()) {
      // It might be an uninstantiated template!
      std::string template_name = result.name;
      if (template_registry.count(template_name)) {
          // It's a root template name
      } else if (!current_package.empty() && template_registry.count(current_package + template_name)) {
          template_name = current_package + template_name;
      }
      
      // Resolve the type arguments
      std::vector<TypeInfo> resolved_args;
      for (const auto& arg : result.type_args) {
          resolved_args.push_back(resolve_type(arg, error_node));
      }
      result.type_args = resolved_args;
      
      resolved = instantiate_template(template_name, resolved_args, error_node);
      if (resolved) {
          result.name = resolved->mangled_name;
          result.type_args.clear(); // Important! The name itself contains the generic info now.
      }
  }

  if (!resolved) {
    record_error(error_node, "Unknown type: " + raw_type.name);
  }
  if (resolved && resolved->node_type == NodeType::ALIAS_STMT) {
    auto *alias = static_cast<AliasStatement *>(resolved);
    result.name = alias->target_type.name;
    result.array_depth += alias->target_type.array_depth;
    result.type_args = alias->target_type.type_args;
    return resolve_type(result, error_node); // Recursively resolve aliases
  } else if (resolved && (resolved->node_type == NodeType::CLASS_DECL ||
                          resolved->node_type == NodeType::ENUM_DECL)) {
    result.name = resolved->mangled_name; // Use fully qualified name
  }

  return result;
}

// ─── Pass 2: Type & Memory Binding ──────────────────────────────────────────

void Binder::bind_types_and_memory() {
  static_variable_index = 1;

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

  std::unordered_map<std::string, std::vector<MethodDeclaration *>> vtables;
  std::unordered_set<std::string> vtable_calculated;

  std::function<void(ClassDeclaration *)> calculate_vtable =
      [&](ClassDeclaration *cls) {
        if (vtable_calculated.count(cls->mangled_name))
          return;

        std::vector<MethodDeclaration *> vtable;
        if (!cls->base_class_name.empty()) {
          Node *base_node = global_scope.resolve(cls->base_class_name);
          if (base_node && base_node->node_type == NodeType::CLASS_DECL) {
            auto *base_cls = static_cast<ClassDeclaration *>(base_node);
            calculate_vtable(base_cls);
            vtable = vtables[base_cls->mangled_name];
          }
        }

        for (const auto &child : cls->children) {
          if (child->node_type == NodeType::METHOD_DECL) {
            auto *method = static_cast<MethodDeclaration *>(child.get());
            if (method->is_override) {
              bool found = false;
              for (size_t i = 0; i < vtable.size(); ++i) {
                std::string base_sig = vtable[i]->mangled_name.substr(
                    vtable[i]->mangled_name.rfind('.') + 1);
                std::string drv_sig = method->mangled_name.substr(
                    method->mangled_name.rfind('.') + 1);
                if (base_sig == drv_sig) {
                  vtable[i] = method;
                  method->vtable_index = i;
                  method->is_virtual = true;
                  found = true;
                  break;
                }
              }
              if (!found)
                throw std::runtime_error(
                    "Method marked override but no base method found: " +
                    method->mangled_name);
            } else if (method->is_virtual || method->is_abstract) {
              method->is_virtual = true;
              method->vtable_index = vtable.size();
              vtable.push_back(method);
            }
          }
        }
        vtables[cls->mangled_name] = vtable;
        cls->vtable = vtable;
        vtable_calculated.insert(cls->mangled_name);
      };

  for (const auto &[name, node] : global_scope.symbols) {
    if (node->node_type == NodeType::CLASS_DECL) {
      calculate_vtable(static_cast<ClassDeclaration *>(node));
    }
  }

  for (const auto &[name, node] : global_scope.symbols) {
    if (node->node_type == NodeType::CLASS_DECL) {
      auto *cls = static_cast<ClassDeclaration *>(node);
      bool has_abstract = false;
      for (auto *m : cls->vtable) {
        if (m->is_abstract) {
          has_abstract = true;
          break;
        }
      }
    }
  }

  int next_vtable_id = 0;
  for (const auto &[name, node] : global_scope.symbols) {
    if (node->node_type == NodeType::CLASS_DECL) {
      auto *cls = static_cast<ClassDeclaration *>(node);
      if (!vtables[cls->mangled_name].empty()) {
        cls->vtable_id = next_vtable_id++;
      }
    }
  }

  for (const auto &[name, node] : global_scope.symbols) {
    if (node->node_type == NodeType::CLASS_DECL) {
      auto *cls = static_cast<ClassDeclaration *>(node);
      if (!cls->base_class_name.empty()) {
        Node *base_node = global_scope.resolve(cls->base_class_name);
        if (base_node && base_node->node_type == NodeType::CLASS_DECL) {
          cls->base_vtable_id =
              static_cast<ClassDeclaration *>(base_node)->vtable_id;
        }
      }
    }
  }
  std::unordered_set<std::string> layout_calculated;
  std::function<int(ClassDeclaration *)> calculate_layout =
      [&](ClassDeclaration *cls) -> int {
    if (layout_calculated.count(cls->mangled_name))
      return cls->instance_size;
    int offset = (!cls->base_class_name.empty() ? 0 : 1);
    if (!cls->base_class_name.empty()) {
      Node *base_node = global_scope.resolve(cls->base_class_name);
      if (base_node && base_node->node_type == NodeType::CLASS_DECL) {
        offset = calculate_layout(static_cast<ClassDeclaration *>(base_node));
      } else {
        throw std::runtime_error("Base class not found: " +
                                 cls->base_class_name);
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

  bind_types_and_memory();

  log_debug("Memory mapping complete. Static variables: {}",
            static_variable_index);

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
    for (const auto &r : context.diagnostic->get_reports()) {
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
    auto* cls = static_cast<ClassDeclaration *>(root);
    if (!cls->template_parameters.empty()) return;
    current_class = cls;
  } else if (root->node_type == NodeType::METHOD_DECL) {
    auto* mth = static_cast<MethodDeclaration *>(root);
    if (!mth->template_parameters.empty()) return;
    current_method = static_cast<MethodDeclaration *>(root);
    local_variable_index = 0;

    current_method->return_type =
        resolve_type(current_method->return_type, current_method);

    SymbolTable method_scope;
    enter_scope(&method_scope);

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
    return;
  } else if (root->node_type == NodeType::CONSTRUCTOR_DECL) {
    auto *ctor = static_cast<ConstructorDeclaration *>(root);
    local_variable_index = 0;

    SymbolTable constructor_scope;
    enter_scope(&constructor_scope);

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
    return;
  }

  for (const auto &child : root->children) {
    if (child)
      bind_tree(child.get());
  }

  current_class = previous_class;
}

// ─── Helpers ─────────────────────────────────────────────────────────────────

bool Binder::is_assignable(const TypeInfo &target, const TypeInfo &source) {
  if (target == source)
    return true;
  if (target.array_depth != source.array_depth)
    return false;

  Node *src_node = global_scope.resolve(source.name);
  while (src_node && src_node->node_type == NodeType::CLASS_DECL) {
    auto *cls = static_cast<ClassDeclaration *>(src_node);
    if (cls->mangled_name == target.name)
      return true;
    if (cls->base_class_name.empty())
      break;
    src_node = global_scope.resolve(cls->base_class_name);
  }
  return false;
}

bool Binder::check_access(Node *member_decl, Node *owner_class_node,
                          Node *expr) {
  if (!owner_class_node || owner_class_node->node_type != NodeType::CLASS_DECL)
    return true;
  ClassDeclaration *owner_class =
      static_cast<ClassDeclaration *>(owner_class_node);

  TokenType access = TokenType::KEYWORD_PUBLIC;
  if (member_decl->node_type == NodeType::FIELD_DECL)
    access = static_cast<FieldDeclaration *>(member_decl)->access_modifier;
  else if (member_decl->node_type == NodeType::METHOD_DECL)
    access = static_cast<MethodDeclaration *>(member_decl)->access_modifier;

  if (access == TokenType::KEYWORD_PUBLIC)
    return true;

  if (current_class &&
      current_class->mangled_name.find(owner_class->mangled_name + ".") == 0) {
    return true;
  }

  if (access == TokenType::KEYWORD_PRIVATE) {
    if (current_class == owner_class)
      return true;
    record_error(expr, "Cannot access private member of class '" +
                           owner_class->class_name + "'");
    return false;
  }

  if (access == TokenType::KEYWORD_PROTECTED) {
    if (!current_class) {
      record_error(expr, "Cannot access protected member outside a class");
      return false;
    }
    ClassDeclaration *iter = current_class;
    while (iter) {
      if (iter == owner_class)
        return true;
      if (iter->base_class_name.empty())
        break;
      Node *base_node = global_scope.resolve(iter->base_class_name);
      if (!base_node)
        break;
      iter = static_cast<ClassDeclaration *>(base_node);
    }
    record_error(expr, "Cannot access protected member of class '" +
                           owner_class->class_name + "'");
    return false;
  }

  if (access == TokenType::KEYWORD_INTERNAL) {
    auto get_package = [](const std::string &name) {
      size_t last_dot = name.rfind('.');
      if (last_dot != std::string::npos)
        return name.substr(0, last_dot + 1);
      return std::string("");
    };
    std::string owner_pkg = get_package(owner_class->mangled_name);
    if (owner_pkg == current_package)
      return true;

    record_error(expr, "Cannot access internal member of class '" +
                           owner_class->class_name +
                           "' from a different package");
    return false;
  }

  return true;
}

// ─── Visitors ────────────────────────────────────────────────────────────────

void Binder::visit(IdentifierNode &n) {
  if (current_pass == BinderPass::EVALUATE_EXPRESSION) {
    if (n.name == "true" || n.name == "false") {
      n.expression_type = {"bool", 0};
      evaluated_type = n.expression_type;
      return;
    }
    if (n.name == "null") {
      n.expression_type = {"void", 0};
      evaluated_type = n.expression_type;
      return;
    }

    Node *declaration = current_scope->resolve(n.name);
    if (!declaration && current_class) {
      declaration =
          global_scope.resolve(current_class->mangled_name + "." + n.name);
    }
    if (!declaration && !current_package.empty()) {
      declaration = global_scope.resolve(current_package + n.name);
    }
    if (!declaration) {
      declaration = global_scope.resolve(n.name);
    }

    if (!declaration) {
      record_error(&n, "Undefined identifier: " + n.name);
      evaluated_type = {"void", 0};
      return;
    }
    n.resolved_declaration = declaration;

    if (declaration->node_type == NodeType::VAR_DECL) {
      n.expression_type =
          static_cast<VariableDeclaration *>(declaration)->type_info;
    } else if (declaration->node_type == NodeType::FIELD_DECL) {
      n.expression_type =
          static_cast<FieldDeclaration *>(declaration)->type_info;
    } else if (declaration->node_type == NodeType::CLASS_DECL ||
               declaration->node_type == NodeType::ENUM_DECL) {
      n.expression_type = {declaration->mangled_name, 0};
    } else {
      record_error(&n, "Invalid identifier usage: " + n.name);
      evaluated_type = {"void", 0};
      return;
    }
    evaluated_type = n.expression_type;
  }
}

void Binder::visit(LiteralNode &n) {
  if (current_pass == BinderPass::EVALUATE_EXPRESSION) {
    if (std::holds_alternative<int64_t>(n.value))
      n.expression_type = {"int32", 0};
    else if (std::holds_alternative<double>(n.value))
      n.expression_type = {"float64", 0};
    else if (std::holds_alternative<std::string>(n.value))
      n.expression_type = {"char", 1};
    else
      n.expression_type = {"void", 0};
    evaluated_type = n.expression_type;
  }
}

void Binder::visit(BinaryExpression &n) {
  if (current_pass == BinderPass::EVALUATE_EXPRESSION) {
    TypeInfo left_type = evaluate_expression(n.left.get());
    TypeInfo right_type = evaluate_expression(n.right.get());

    Node *left_decl = global_scope.resolve(left_type.name);
    if (left_decl && left_decl->node_type == NodeType::CLASS_DECL) {
      std::string op_name = "operator";
      if (n.op == TokenType::OPERATOR_PLUS)
        op_name += "+";
      else if (n.op == TokenType::OPERATOR_MINUS)
        op_name += "-";
      else if (n.op == TokenType::OPERATOR_MULTIPLY)
        op_name += "*";
      else if (n.op == TokenType::OPERATOR_DIVIDE)
        op_name += "/";
      else
        goto primitive_fallback;

      std::string base_name = left_decl->mangled_name + "." + op_name;
      std::vector<TypeInfo> args = {right_type};
      std::string mangled = mangle_method_call(base_name, args);
      Node *method_decl = global_scope.resolve(mangled);
      if (!method_decl)
        goto primitive_fallback;

      n.overloaded_operator = method_decl;
      n.expression_type =
          static_cast<MethodDeclaration *>(method_decl)->return_type;
      evaluated_type = n.expression_type;
      return;
    }

  primitive_fallback:
    if (left_type != right_type) {
      record_error(&n, "Binary operands type mismatch: '" + left_type.name +
                           "' vs '" + right_type.name + "'");
    }
    if (n.op >= TokenType::OPERATOR_EQUAL &&
        n.op <= TokenType::OPERATOR_GREATER_EQUAL) {
      n.expression_type = {"bool", 0};
    } else {
      n.expression_type = left_type;
    }
    evaluated_type = n.expression_type;
  }
}

void Binder::visit(UnaryExpression &n) {
  if (current_pass == BinderPass::EVALUATE_EXPRESSION) {
    n.expression_type = evaluate_expression(n.operand.get());
    evaluated_type = n.expression_type;
  }
}

void Binder::visit(AssignmentExpression &n) {
  if (current_pass == BinderPass::EVALUATE_EXPRESSION) {
    TypeInfo target_type = evaluate_expression(n.target.get());
    TypeInfo value_type = evaluate_expression(n.value.get());

    Node *left_decl = global_scope.resolve(target_type.name);
    if (left_decl && left_decl->node_type == NodeType::CLASS_DECL) {
      std::string op_name = "operator=";
      std::string base_name = left_decl->mangled_name + "." + op_name;
      std::vector<TypeInfo> args = {value_type};
      std::string mangled = mangle_method_call(base_name, args);
      Node *method_decl = global_scope.resolve(mangled);
      if (method_decl) {
        n.overloaded_operator = method_decl;
        n.expression_type =
            static_cast<MethodDeclaration *>(method_decl)->return_type;
        evaluated_type = n.expression_type;
        return;
      }
    }
    if (!is_assignable(target_type, value_type)) {
      record_error(&n, "Assignment type mismatch: '" + target_type.name +
                           "' = '" + value_type.name + "'");
    }
    n.expression_type = target_type;
    evaluated_type = n.expression_type;
  }
}

void Binder::visit(ArrayAccessExpression &n) {
  if (current_pass == BinderPass::EVALUATE_EXPRESSION) {
    TypeInfo array_type = evaluate_expression(n.array.get());
    if (array_type.array_depth == 0)
      record_error(&n, "Cannot index a non-array value");
    TypeInfo index_type = evaluate_expression(n.index.get());
    if (index_type.name != "int32")
      record_error(&n, "Array index must be int32");
    n.expression_type = array_type;
    n.expression_type.array_depth--;
    evaluated_type = n.expression_type;
  }
}

void Binder::visit(MemberAccessExpression &n) {
  if (current_pass == BinderPass::EVALUATE_EXPRESSION) {
    TypeInfo object_type = evaluate_expression(n.object.get());
    if (object_type.array_depth > 0) {
      if (n.member_name == "length") {
        n.expression_type = {"int32", 0};
        evaluated_type = n.expression_type;
        return;
      }
      record_error(&n, "Arrays only have the 'length' property");
    }
    Node *type_decl = global_scope.resolve(object_type.name);
    if (!type_decl) {
      record_error(&n, "Cannot access members on unknown type: " +
                           object_type.name);
      evaluated_type = {"void", 0};
      return;
    }
    if (type_decl->node_type == NodeType::CLASS_DECL) {
      auto *class_decl = static_cast<ClassDeclaration *>(type_decl);
      Node *member_decl =
          global_scope.resolve(class_decl->mangled_name + "." + n.member_name);
      ClassDeclaration *current_resolve_class = class_decl;
      while (!member_decl && current_resolve_class &&
             !current_resolve_class->base_class_name.empty()) {
        Node *base_node =
            global_scope.resolve(current_resolve_class->base_class_name);
        if (!base_node)
          break;
        current_resolve_class = static_cast<ClassDeclaration *>(base_node);
        member_decl = global_scope.resolve(current_resolve_class->mangled_name +
                                           "." + n.member_name);
      }
      if (!member_decl) {
        record_error(&n, "Member not found: " + n.member_name + " on " +
                             class_decl->mangled_name);
        evaluated_type = {"void", 0};
        return;
      }
      if (!check_access(member_decl, current_resolve_class, &n)) {
        evaluated_type = {"void", 0};
        return;
      }
      n.resolved_declaration = member_decl;
      if (member_decl->node_type == NodeType::FIELD_DECL) {
        n.expression_type =
            static_cast<FieldDeclaration *>(member_decl)->type_info;
        evaluated_type = n.expression_type;
        return;
      }
      if (member_decl->node_type == NodeType::CLASS_DECL ||
          member_decl->node_type == NodeType::ENUM_DECL) {
        n.expression_type = {member_decl->mangled_name, 0};
        evaluated_type = n.expression_type;
        return;
      }
    } else if (type_decl->node_type == NodeType::ENUM_DECL) {
      auto *enum_decl = static_cast<EnumDeclaration *>(type_decl);
      int32_t e_val = -1;
      for (size_t i = 0; i < enum_decl->members.size(); ++i) {
        if (enum_decl->members[i] == n.member_name) {
          e_val = i;
          break;
        }
      }
      if (e_val == -1)
        record_error(&n, "Undefined enum member: " + n.member_name);
      n.resolved_declaration = enum_decl;
      n.enum_value = e_val;
      n.expression_type = object_type;
      evaluated_type = n.expression_type;
      return;
    }
    record_error(&n, "Invalid member access on type: " + object_type.name);
    evaluated_type = {"void", 0};
  }
}

void Binder::visit(MethodCallExpression &n) {
  if (current_pass == BinderPass::EVALUATE_EXPRESSION) {
    std::vector<TypeInfo> argument_types;
    for (const auto &arg : n.arguments) {
      argument_types.push_back(evaluate_expression(arg.get()));
    }

    if (n.callee->node_type == NodeType::MEMBER_ACCESS) {
      auto *member_access =
          static_cast<MemberAccessExpression *>(n.callee.get());
      TypeInfo receiver_type;
      ClassDeclaration *current_resolve_class = nullptr;

      if (member_access->is_scope_resolution) {
        if (member_access->object->node_type == NodeType::IDENTIFIER) {
          auto *id = static_cast<IdentifierNode *>(member_access->object.get());
          std::string class_name = resolve_type(TypeInfo{id->name, 0}, &n).name;
          Node *decl = global_scope.resolve(class_name);
          if (!decl || decl->node_type != NodeType::CLASS_DECL) {
            record_error(&n, "Invalid class name for scope resolution: " +
                                 class_name);
            evaluated_type = {"void", 0};
            return;
          }
          current_resolve_class = static_cast<ClassDeclaration *>(decl);
          receiver_type = {"void", 0};
        } else if (member_access->object->node_type ==
                   NodeType::MEMBER_ACCESS) {
          auto *inner_access = static_cast<MemberAccessExpression *>(
              member_access->object.get());
          receiver_type = evaluate_expression(inner_access->object.get());
          std::string class_name =
              resolve_type(TypeInfo{inner_access->member_name, 0}, &n).name;
          Node *decl = global_scope.resolve(class_name);
          if (!decl || decl->node_type != NodeType::CLASS_DECL) {
            record_error(&n, "Invalid class name for scope resolution: " +
                                 class_name);
            evaluated_type = {"void", 0};
            return;
          }
          current_resolve_class = static_cast<ClassDeclaration *>(decl);
          member_access->object = std::move(inner_access->object);
        } else {
          record_error(&n, "Invalid syntax for scope resolution");
          evaluated_type = {"void", 0};
          return;
        }
      } else {
        receiver_type = evaluate_expression(member_access->object.get());
        Node *type_decl = global_scope.resolve(receiver_type.name);
        if (type_decl && type_decl->node_type == NodeType::CLASS_DECL) {
          current_resolve_class = static_cast<ClassDeclaration *>(type_decl);
        }
      }

      Node *method_decl = nullptr;
      std::string base_name;
      std::string mangled_name;

      while (!method_decl && current_resolve_class) {
        base_name = current_resolve_class->mangled_name + "." +
                    member_access->member_name;
        mangled_name = mangle_method_call(base_name, argument_types);
        method_decl = global_scope.resolve(mangled_name);
        if (!method_decl)
          method_decl = global_scope.resolve(base_name);

        if (!method_decl && !current_resolve_class->base_class_name.empty() &&
            !member_access->is_scope_resolution) {
          Node *base_node =
              global_scope.resolve(current_resolve_class->base_class_name);
          if (base_node)
            current_resolve_class = static_cast<ClassDeclaration *>(base_node);
          else
            break;
        } else
          break;
      }

      if (!method_decl) {
        record_error(&n, "No matching method: " + member_access->member_name +
                             " on " + receiver_type.name);
        evaluated_type = {"void", 0};
        return;
      }
      if (!check_access(method_decl, current_resolve_class, &n)) {
        evaluated_type = {"void", 0};
        return;
      }
      n.resolved_declaration = method_decl;
      if (method_decl->node_type == NodeType::METHOD_DECL) {
        auto *m = static_cast<MethodDeclaration *>(method_decl);
        if (m->is_virtual && member_access &&
            !member_access->is_scope_resolution) {
          n.is_virtual_call = true;
        }
        n.expression_type = m->return_type;
      } else {
        n.expression_type = {"void", 0};
      }
      evaluated_type = n.expression_type;
      return;
    } else {
      auto *id = static_cast<IdentifierNode *>(n.callee.get());
      if (!current_class) {
          // Allow global method calls
          std::string base_name = id->name;
          std::string mangled_name;
          Node *method_decl = nullptr;
          if (!n.type_args.empty()) {
              std::vector<TypeInfo> resolved_targs;
              for (auto& t : n.type_args) resolved_targs.push_back(resolve_type(t, &n));
              instantiate_template(base_name, resolved_targs, &n);
              base_name += "<";
              for (size_t i = 0; i < resolved_targs.size(); ++i) {
                  base_name += resolved_targs[i].to_string();
                  if (i < resolved_targs.size() - 1) base_name += ",";
              }
              base_name += ">";
          }
          mangled_name = mangle_method_call(base_name, argument_types);
          method_decl = global_scope.resolve(mangled_name);
          if (!method_decl) method_decl = global_scope.resolve(base_name);
          
          if (!method_decl && n.type_args.empty() && template_registry.count(base_name)) {
              Node* blueprint = template_registry[base_name];
              if (blueprint->node_type == NodeType::METHOD_DECL) {
                  auto* method_bp = static_cast<MethodDeclaration*>(blueprint);
                  std::vector<TypeInfo> param_types;
                  for (const auto& p : method_bp->parameters) param_types.push_back(static_cast<VariableDeclaration*>(p.get())->type_info);
                  std::vector<TypeInfo> deduced_args;
                  if (deduce_template_arguments(param_types, argument_types, method_bp->template_parameters, deduced_args)) {
                      std::string instantiated_base = base_name + "<";
                      for (size_t i = 0; i < deduced_args.size(); ++i) {
                          instantiated_base += deduced_args[i].to_string();
                          if (i < deduced_args.size() - 1) instantiated_base += ",";
                      }
                      instantiated_base += ">";
                      mangled_name = mangle_method_call(instantiated_base, argument_types);
                      method_decl = global_scope.resolve(mangled_name);
                      if (!method_decl) {
                          instantiate_template(base_name, deduced_args, &n);
                          method_decl = global_scope.resolve(mangled_name);
                      }
                  }
              }
          }
          
          if (!method_decl) {
            record_error(&n, "No matching global function: " + id->name);
            evaluated_type = {"void", 0};
            return;
          }
          n.resolved_declaration = method_decl;
          if (method_decl->node_type == NodeType::METHOD_DECL) {
            n.expression_type = static_cast<MethodDeclaration *>(method_decl)->return_type;
          } else {
            n.expression_type = {"void", 0};
          }
          evaluated_type = n.expression_type;
          return;
      }
      std::string base_name;
      std::string mangled_name;
      Node *method_decl = nullptr;
      ClassDeclaration *current_resolve_class = current_class;

      if (id->name == "super") {
        if (current_class->base_class_name.empty()) {
          record_error(&n,
                       "Cannot call super() in a class without a base class");
          evaluated_type = {"void", 0};
          return;
        }
        Node *base_node = global_scope.resolve(current_class->base_class_name);
        if (!base_node) {
          record_error(&n, "Base class not found");
          evaluated_type = {"void", 0};
          return;
        }
        current_resolve_class = static_cast<ClassDeclaration *>(base_node);
        base_name = current_resolve_class->mangled_name + ".ctor";
        mangled_name = mangle_method_call(base_name, argument_types);
        method_decl = global_scope.resolve(mangled_name);
        if (!method_decl)
          method_decl = global_scope.resolve(base_name);
      } else {
        while (!method_decl && current_resolve_class) {
          base_name = current_resolve_class->mangled_name + "." + id->name;
          if (!n.type_args.empty()) {
              std::vector<TypeInfo> resolved_targs;
              for (auto& t : n.type_args) resolved_targs.push_back(resolve_type(t, &n));
              instantiate_template(base_name, resolved_targs, &n);
              base_name += "<";
              for (size_t i = 0; i < resolved_targs.size(); ++i) {
                  base_name += resolved_targs[i].to_string();
                  if (i < resolved_targs.size() - 1) base_name += ",";
              }
              base_name += ">";
          }
          mangled_name = mangle_method_call(base_name, argument_types);
          method_decl = global_scope.resolve(mangled_name);
          if (!method_decl)
            method_decl = global_scope.resolve(base_name);

          if (!method_decl && !current_resolve_class->base_class_name.empty()) {
            Node *base_node =
                global_scope.resolve(current_resolve_class->base_class_name);
            if (base_node)
              current_resolve_class =
                  static_cast<ClassDeclaration *>(base_node);
            else
              break;
          } else
            break;
        }
      }

      if (!method_decl) {
        record_error(&n, "No matching method: " + id->name);
        evaluated_type = {"void", 0};
        return;
      }
      if (!check_access(method_decl, current_resolve_class, &n)) {
        evaluated_type = {"void", 0};
        return;
      }
      n.resolved_declaration = method_decl;
      if (method_decl->node_type == NodeType::METHOD_DECL) {
        auto *m = static_cast<MethodDeclaration *>(method_decl);
        if (m->is_virtual && id->name != "super") {
          n.is_virtual_call = true;
        }
        n.expression_type = m->return_type;
      } else {
        n.expression_type = {"void", 0};
      }
      evaluated_type = n.expression_type;
      return;
    }
  }
}

void Binder::visit(NewInstanceExpression &n) {
  if (current_pass == BinderPass::EVALUATE_EXPRESSION) {
    n.type_info = resolve_type(n.type_info, &n);
    Node *resolved_cls = global_scope.resolve(n.type_info.name);
    if (resolved_cls && resolved_cls->node_type == NodeType::CLASS_DECL) {
      auto *cls = static_cast<ClassDeclaration *>(resolved_cls);
      for (auto *m : cls->vtable) {
        if (m->is_abstract) {
          record_error(&n, "Cannot instantiate abstract class '" +
                               cls->class_name +
                               "' (abstract method: " + m->method_name + ")");
          evaluated_type = {"void", 0};
          return;
        }
      }
    }
    std::vector<TypeInfo> argument_types;
    for (const auto &arg : n.arguments) {
      argument_types.push_back(evaluate_expression(arg.get()));
    }
    std::string mangled_ctor =
        mangle_constructor(n.type_info.name, argument_types);
    Node *ctor = global_scope.resolve(mangled_ctor);
    if (!ctor && !argument_types.empty()) {
      std::string ctor_prefix = n.type_info.name + ".ctor(";
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
    if (!ctor && !argument_types.empty())
      record_error(&n, "No matching constructor: " + mangled_ctor);
    if (ctor)
      n.resolved_declaration = ctor;
    else
      n.resolved_declaration = global_scope.resolve(n.type_info.name);
    n.expression_type = n.type_info;
    evaluated_type = n.expression_type;
  }
}

void Binder::visit(ArrayCreationExpression &n) {
  if (current_pass == BinderPass::EVALUATE_EXPRESSION) {
    n.type_info = resolve_type(n.type_info, &n);
    TypeInfo size_type = evaluate_expression(n.size.get());
    if (size_type.name != "int32")
      record_error(&n, "Array size must be int32");
    n.expression_type = n.type_info;
    n.expression_type.array_depth++;
    evaluated_type = n.expression_type;
  }
}

void Binder::visit(ArrayLiteralExpression &n) {
  if (current_pass == BinderPass::EVALUATE_EXPRESSION) {
    TypeInfo element_type = {"void", 0};
    for (const auto &element : n.elements) {
      TypeInfo current_element_type = evaluate_expression(element.get());
      if (element_type.name == "void") {
        element_type = current_element_type;
      } else if (element_type != current_element_type) {
        record_error(&n, "Mixed types in array literal");
      }
    }
    element_type.array_depth++;
    n.expression_type = element_type;
    evaluated_type = n.expression_type;
  }
}

void Binder::visit(CastExpression &n) {
  if (current_pass == BinderPass::EVALUATE_EXPRESSION) {
    TypeInfo source_type = evaluate_expression(n.expression.get());
    n.target_type = resolve_type(n.target_type, &n);
    auto is_primitive = [&](const TypeInfo &t) {
      if (t.array_depth > 0)
        return false;
      Node *decl = global_scope.resolve(t.name);
      return !decl || decl->is_primitive;
    };
    bool target_prim = is_primitive(n.target_type);
    bool source_prim = is_primitive(source_type);
    if (target_prim && source_prim) {
    } else if (target_prim != source_prim) {
      record_error(&n, "Cannot cast between primitive and class types");
    } else {
      if (is_assignable(n.target_type, source_type)) {
      } else if (is_assignable(source_type, n.target_type)) {
        Node *target_class = global_scope.resolve(n.target_type.name);
        if (target_class && target_class->node_type == NodeType::CLASS_DECL) {
          n.target_vtable_id =
              static_cast<ClassDeclaration *>(target_class)->vtable_id;
        }
      } else {
        record_error(&n, "Cannot cast '" + source_type.name + "' to '" +
                             n.target_type.name +
                             "': no inheritance relationship");
      }
    }
    n.expression_type = n.target_type;
    evaluated_type = n.expression_type;
  }
}

void Binder::visit(InstanceofExpression &n) {
  if (current_pass == BinderPass::EVALUATE_EXPRESSION) {
    TypeInfo source_type = evaluate_expression(n.expression.get());
    n.target_type = resolve_type(n.target_type, &n);
    Node *target_class = global_scope.resolve(n.target_type.name);
    if (target_class && target_class->node_type == NodeType::CLASS_DECL) {
      n.target_vtable_id =
          static_cast<ClassDeclaration *>(target_class)->vtable_id;
    }
    n.expression_type = {"bool", 0};
    evaluated_type = n.expression_type;
  }
}

void Binder::visit(TernaryExpression &n) {
  if (current_pass == BinderPass::EVALUATE_EXPRESSION) {
    TypeInfo condition_type = evaluate_expression(n.condition.get());
    if (condition_type.name != "bool")
      record_error(&n, "Ternary condition must be bool");
    TypeInfo true_type = evaluate_expression(n.true_branch.get());
    TypeInfo false_type = evaluate_expression(n.false_branch.get());
    if (true_type != false_type)
      record_error(&n, "Ternary branches must have the same type");
    n.expression_type = true_type;
    evaluated_type = n.expression_type;
  }
}

void Binder::visit(BlockStatement &n) {
  if (current_pass == BinderPass::BIND_EXECUTION) {
    SymbolTable block_scope;
    enter_scope(&block_scope);
    for (const auto &child : n.children)
      bind_node(child.get());
    exit_scope();
  }
}

void Binder::visit(IfStatement &n) {
  if (current_pass == BinderPass::BIND_EXECUTION) {
    TypeInfo condition_type = evaluate_expression(n.condition.get());
    if (condition_type.name != "bool")
      record_error(&n, "Condition must be bool");
    bind_node(n.then_branch.get());
    if (n.else_branch)
      bind_node(n.else_branch.get());
  }
}

void Binder::visit(ForStatement &n) {
  if (current_pass == BinderPass::BIND_EXECUTION) {
    SymbolTable for_scope;
    enter_scope(&for_scope);
    if (n.initialization)
      bind_node(n.initialization.get());
    if (n.condition) {
      TypeInfo condition_type = evaluate_expression(n.condition.get());
      if (condition_type.name != "bool")
        record_error(&n, "Condition must be bool");
    }
    if (n.iteration)
      evaluate_expression(n.iteration.get());
    loop_depth++;
    bind_node(n.body.get());
    loop_depth--;
    exit_scope();
  }
}

void Binder::visit(WhileStatement &n) {
  if (current_pass == BinderPass::BIND_EXECUTION) {
    TypeInfo condition_type = evaluate_expression(n.condition.get());
    if (condition_type.name != "bool")
      record_error(&n, "Condition must be bool");
    loop_depth++;
    bind_node(n.body.get());
    loop_depth--;
  }
}

void Binder::visit(DoWhileStatement &n) {
  if (current_pass == BinderPass::BIND_EXECUTION) {
    loop_depth++;
    bind_node(n.body.get());
    loop_depth--;
    TypeInfo condition_type = evaluate_expression(n.condition.get());
    if (condition_type.name != "bool")
      record_error(&n, "Condition must be bool");
  }
}

void Binder::visit(SwitchStatement &n) {
  if (current_pass == BinderPass::BIND_EXECUTION) {
    evaluate_expression(n.condition.get());
    switch_depth++;
    for (const auto &case_node : n.children)
      bind_node(case_node.get());
    switch_depth--;
  }
}

void Binder::visit(CaseStatement &n) {
  if (current_pass == BinderPass::BIND_EXECUTION) {
    if (n.case_value)
      evaluate_expression(n.case_value.get());
    for (const auto &child : n.children)
      bind_node(child.get());
  }
}

void Binder::visit(VariableDeclaration &n) {
  if (current_pass == BinderPass::BIND_EXECUTION) {
    n.type_info = resolve_type(n.type_info, &n);
    Node *type_decl = global_scope.resolve(n.type_info.name);
    n.is_reference_type =
        !(type_decl && type_decl->is_primitive && n.type_info.array_depth == 0);
    if (n.initializer) {
      TypeInfo initializer_type = evaluate_expression(n.initializer.get());
      if (!is_assignable(n.type_info, initializer_type)) {
        record_error(&n, "Type mismatch in variable declaration: expected '" +
                             n.type_info.name + "', got '" +
                             initializer_type.name + "'");
      }
    }
    n.memory_index = local_variable_index++;
    declare_local(n.var_name, &n);
  }
}

void Binder::visit(ExpressionStatement &n) {
  if (current_pass == BinderPass::BIND_EXECUTION) {
    evaluate_expression(n.expression.get());
  }
}

void Binder::visit(ReturnStatement &n) {
  if (current_pass == BinderPass::BIND_EXECUTION) {
    if (n.value) {
      TypeInfo return_type = evaluate_expression(n.value.get());
      if (current_method &&
          !is_assignable(current_method->return_type, return_type)) {
        record_error(&n, "Return type mismatch: expected '" +
                             current_method->return_type.name + "', got '" +
                             return_type.name + "'");
      }
    } else if (current_method && current_method->return_type.name != "void") {
      record_error(&n, "Must return a value from non-void method");
    }
  }
}

void Binder::visit(BreakStatement &n) {
  if (current_pass == BinderPass::BIND_EXECUTION) {
    if (loop_depth == 0 && switch_depth == 0)
      record_error(&n, "Break must be inside a loop or switch");
  }
}

void Binder::visit(ContinueStatement &n) {
  if (current_pass == BinderPass::BIND_EXECUTION) {
    if (loop_depth == 0)
      record_error(&n, "Continue must be inside a loop");
  }
}

void Binder::visit(PackageStatement &n) {
  if (current_pass == BinderPass::REGISTER_GLOBALS) {
    current_package = n.package_name + ".";
    n.mangled_name = n.package_name;
    global_scope.define(n.mangled_name, &n);
  } else if (current_pass == BinderPass::REGISTER_MEMBERS) {
    current_prefix = n.package_name + ".";
  }
}

void Binder::visit(AliasStatement &n) {
  if (!n.template_parameters.empty()) {
      if (current_pass == BinderPass::REGISTER_GLOBALS) {
          std::string full_name = current_prefix + n.alias_name;
          template_registry[full_name] = &n;
      }
      return;
  }
  if (current_pass == BinderPass::REGISTER_GLOBALS) {
    std::string full_name = current_prefix + n.alias_name;
    if (global_scope.symbols.count(full_name))
      record_error(&n, "Duplicate global symbol: " + full_name);
    n.mangled_name = full_name;
    global_scope.define(full_name, &n);
  }
}

void Binder::visit(EnumDeclaration &n) {
  if (current_pass == BinderPass::REGISTER_GLOBALS) {
    std::string full_name = current_prefix + n.enum_name;
    if (global_scope.symbols.count(full_name))
      record_error(&n, "Duplicate global symbol: " + full_name);
    n.mangled_name = full_name;
    global_scope.define(full_name, &n);
    std::string my_prefix = full_name + ".";
    for (const auto &child : n.children) {
      if (child)
        child->parent = &n;
      register_global_symbols(child.get(), my_prefix);
    }
  } else if (current_pass == BinderPass::REGISTER_MEMBERS) {
    std::string my_prefix = current_prefix + n.enum_name + ".";
    for (const auto &child : n.children)
      register_members(child.get(), my_prefix);
  }
}

void Binder::visit(ClassDeclaration &n) {
  if (!n.template_parameters.empty()) {
      if (current_pass == BinderPass::REGISTER_GLOBALS) {
          std::string full_name = current_prefix + n.class_name;
          template_registry[full_name] = &n;
      }
      return;
  }
  if (current_pass == BinderPass::REGISTER_GLOBALS) {
    std::string full_name = current_prefix + n.class_name;
    if (!n.base_class_name.empty())
      n.base_class_name = current_prefix + n.base_class_name;
    if (global_scope.symbols.count(full_name))
      record_error(&n, "Duplicate global symbol: " + full_name);
    n.mangled_name = full_name;
    global_scope.define(full_name, &n);
    std::string my_prefix = full_name + ".";
    for (const auto &child : n.children) {
      if (child)
        child->parent = &n;
      register_global_symbols(child.get(), my_prefix);
    }
  } else if (current_pass == BinderPass::REGISTER_MEMBERS) {
    std::string my_prefix = current_prefix + n.class_name + ".";
    current_class = &n;
    for (const auto &child : n.children)
      register_members(child.get(), my_prefix);
    current_class = nullptr;
  }
}

void Binder::visit(FieldDeclaration &n) {
  if (current_pass == BinderPass::REGISTER_MEMBERS) {
    std::string full_name = current_prefix + n.field_name;
    n.mangled_name = full_name;
    global_scope.define(full_name, &n);
  }
}

void Binder::visit(ConstructorDeclaration &n) {
  if (current_pass == BinderPass::REGISTER_MEMBERS) {
    std::string full_name = current_prefix + "ctor(";
    for (size_t i = 0; i < n.parameters.size(); ++i) {
      auto *var_decl =
          static_cast<VariableDeclaration *>(n.parameters[i].get());
      var_decl->type_info = resolve_type(var_decl->type_info, var_decl);
      full_name += var_decl->type_info.to_string();
      if (i < n.parameters.size() - 1)
        full_name += ",";
    }
    full_name += ")";
    n.mangled_name = full_name;
    global_scope.define(full_name, &n);
  }
}

void Binder::visit(MethodDeclaration &n) {
  if (!n.template_parameters.empty()) {
      if (current_pass == BinderPass::REGISTER_MEMBERS) {
          std::string full_name = current_prefix + n.method_name;
          template_registry[full_name] = &n;
      }
      return;
  }
  if (current_pass == BinderPass::REGISTER_MEMBERS) {
    for (const auto &param : n.parameters) {
      auto *var_decl = static_cast<VariableDeclaration *>(param.get());
      var_decl->type_info = resolve_type(var_decl->type_info, var_decl);
    }
    std::string full_name = current_prefix + mangle_method(&n);
    if (global_scope.symbols.count(full_name) && !n.is_native)
      record_error(&n, "Duplicate method signature: " + full_name);
    n.mangled_name = full_name;
    global_scope.define(full_name, &n);
  }
}

} // namespace solix
