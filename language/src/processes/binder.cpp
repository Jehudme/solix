#include "processes/binder.hpp"
#include "solix/compilation.hpp"
#include "utilities/diagnostic.hpp"
#include <unordered_set>

#include "processes/template_substitution.hpp"

namespace solix {

Node *Binder::instantiate_template(const std::string &template_name,
                                   const std::vector<TypeInfo> &type_args,
                                   Node *error_node) {
  log_debug("Attempting to instantiate template '{}' with {} type arguments",
            template_name, type_args.size());

  if (!template_registry.count(template_name)) {
    log_error(
        "Failed template instantiation: blueprint '{}' not found in registry",
        template_name);
    record_error(error_node, "Unknown template: " + template_name);
    return nullptr;
  }

  Node *blueprint = template_registry[template_name];

  std::string mangled_name = template_name + "<";
  for (size_t i = 0; i < type_args.size(); ++i) {
    mangled_name += type_args[i].to_string();
    if (i < type_args.size() - 1)
      mangled_name += ",";
  }
  mangled_name += ">";

  if (global_scope.symbols.count(mangled_name)) {
    log_debug(
        "Template instantiation for '{}' retrieved from cache/global_scope",
        mangled_name);
    return global_scope.symbols[mangled_name];
  }

  struct TemplateInstantiationGuard {
    Binder *b;
    ClassDeclaration *saved_class;
    MethodDeclaration *saved_method;
    SymbolTable *saved_scope;
    uint32_t saved_local_var_idx;
    std::string saved_pkg;
    std::string saved_prefix;
    BinderPass saved_pass;

    TemplateInstantiationGuard(Binder *b)
        : b(b), saved_class(b->current_class), saved_method(b->current_method),
          saved_scope(b->current_scope), saved_local_var_idx(b->local_variable_index),
          saved_pkg(b->current_package), saved_prefix(b->current_prefix),
          saved_pass(b->current_pass) {
      b->current_scope = &b->global_scope;
      b->current_method = nullptr;
      b->current_class = nullptr;
      b->local_variable_index = 0;
    }

    ~TemplateInstantiationGuard() {
      b->current_class = saved_class;
      b->current_method = saved_method;
      b->current_scope = saved_scope;
      b->local_variable_index = saved_local_var_idx;
      b->current_package = saved_pkg;
      b->current_prefix = saved_prefix;
      b->current_pass = saved_pass;
    }
  } guard(this);

  std::vector<std::string> tparams;
  if (blueprint->node_type == NodeType::CLASS_DECL)
    tparams = static_cast<ClassDeclaration *>(blueprint)->template_parameters;
  else if (blueprint->node_type == NodeType::ALIAS_STMT)
    tparams = static_cast<AliasStatement *>(blueprint)->template_parameters;
  else if (blueprint->node_type == NodeType::METHOD_DECL)
    tparams = static_cast<MethodDeclaration *>(blueprint)->template_parameters;

  if (type_args.size() != tparams.size()) {
    log_error("Template '{}' arity mismatch: expected {}, got {}",
              template_name, tparams.size(), type_args.size());
    record_error(error_node, "Template " + template_name + " expects " +
                                 std::to_string(tparams.size()) +
                                 " arguments, got " +
                                 std::to_string(type_args.size()));
    return nullptr;
  }

  std::unordered_map<std::string, TypeInfo> substitutions;
  for (size_t i = 0; i < tparams.size(); ++i) {
    substitutions[tparams[i]] = type_args[i];
    log_trace("Substitution mapping: {} -> {}", tparams[i],
              type_args[i].to_string());
  }

  log_debug("Cloning AST blueprint for template '{}' -> '{}'", template_name,
            mangled_name);
  auto clone_ptr = blueprint->clone();
  Node *clone = clone_ptr.get();

  if (clone->node_type == NodeType::CLASS_DECL) {
    static_cast<ClassDeclaration *>(clone)->template_parameters.clear();
    static_cast<ClassDeclaration *>(clone)->class_name = mangled_name;
  } else if (clone->node_type == NodeType::ALIAS_STMT) {
    static_cast<AliasStatement *>(clone)->template_parameters.clear();
    static_cast<AliasStatement *>(clone)->alias_name = mangled_name;
  } else if (clone->node_type == NodeType::METHOD_DECL) {
    static_cast<MethodDeclaration *>(clone)->template_parameters.clear();
    static_cast<MethodDeclaration *>(clone)->method_name = mangled_name;
  }

  log_trace("Executing template substitution visitor on clone for '{}'",
            mangled_name);
  TemplateSubstitutionVisitor substitutor(substitutions);
  substitutor.execute(clone);

  context.nodes[std::string("__instantiated_templates")].push_back(
      std::move(clone_ptr));
  instantiated_templates.insert(mangled_name);

  std::string current_pkg_copy = current_package;
  std::string my_prefix = "";
  auto last_dot = template_name.rfind('.');
  if (last_dot != std::string::npos)
    my_prefix = template_name.substr(0, last_dot + 1);

  log_debug("Running mini-pipeline passes on newly instantiated template '{}'",
            mangled_name);

  if (clone->node_type == NodeType::CLASS_DECL) {
    auto *cls = static_cast<ClassDeclaration *>(clone);
    cls->class_name = mangled_name.substr(my_prefix.length());
    cls->package_context = my_prefix;
    current_package = my_prefix;
    current_prefix = my_prefix;
    register_global_symbols(clone, my_prefix);
    register_members(clone, my_prefix);

    ClassDeclaration* current_class_copy = current_class;
    current_class = cls;

    int offset = (!cls->base_class_name.empty() ? 0 : 1);
    if (!cls->base_class_name.empty()) {
      Node *base_node = global_scope.resolve(cls->base_class_name);
      if (base_node && base_node->node_type == NodeType::CLASS_DECL) {
        offset = static_cast<ClassDeclaration *>(base_node)->instance_size;
      }
    }
    for (const auto &child : cls->children) {
      if (child && child->node_type == NodeType::FIELD_DECL) {
        auto *field = static_cast<FieldDeclaration *>(child.get());
        field->type_info = resolve_type(field->type_info, field);
        Node *type_decl = global_scope.resolve(field->type_info.name);
        field->is_reference_type = !(type_decl && type_decl->is_primitive &&
                                     field->type_info.array_depth == 0);
        if (!field->is_static) {
          field->memory_index = offset++;
        }
      } else if (child && child->node_type == NodeType::METHOD_DECL) {
        auto *method = static_cast<MethodDeclaration *>(child.get());
        method->return_type = resolve_type(method->return_type, method);
      }
    }
    cls->instance_size = offset;

    current_class = current_class_copy;

    if (current_pass == BinderPass::BIND_EXECUTION ||
        current_pass == BinderPass::EVALUATE_EXPRESSION) {
      BinderPass old = current_pass;
      current_pass = BinderPass::BIND_EXECUTION;
      current_package = my_prefix;
      bind_tree(clone);
      current_pass = old;
    }
  } else if (clone->node_type == NodeType::ALIAS_STMT) {
    static_cast<AliasStatement *>(clone)->alias_name =
        mangled_name.substr(my_prefix.length());
    register_global_symbols(clone, my_prefix);
    if (current_pass == BinderPass::BIND_EXECUTION ||
        current_pass == BinderPass::EVALUATE_EXPRESSION) {
      BinderPass old = current_pass;
      current_pass = BinderPass::BIND_EXECUTION;
      current_package = my_prefix;
      bind_tree(clone);
      current_pass = old;
    }
  } else if (clone->node_type == NodeType::METHOD_DECL) {
    current_package = my_prefix;
    current_prefix = my_prefix;
    static_cast<MethodDeclaration *>(clone)->package_context = my_prefix;
    static_cast<MethodDeclaration *>(clone)->method_name =
        mangled_name.substr(my_prefix.length());
    register_members(clone, my_prefix);
    if (!global_scope.symbols.count(mangled_name)) {
      global_scope.define(mangled_name, clone);
    }
    if (current_pass == BinderPass::BIND_EXECUTION ||
        current_pass == BinderPass::EVALUATE_EXPRESSION) {
      BinderPass old = current_pass;
      current_pass = BinderPass::BIND_EXECUTION;
      current_package = my_prefix;
      bind_tree(clone);
      current_pass = old;
    }
  }
  current_package = current_pkg_copy;

  log_debug("Successfully instantiated template: {}", mangled_name);
  return global_scope.resolve(mangled_name);
}

// ─── Error & Warning Reporting ───────────────────────────────────────────────

void Binder::record_error(Node *node, const std::string &msg) {
  uint32_t line = node ? node->line : 0;
  uint32_t col = node ? node->column : 0;
  log_error("[line {}, col {}] {}", line, col, msg);

  Report report;
  report.severity = ReportSeverity::ERROR;
  report.code = "E_BIND";
  report.message = msg;
  report.source_path = "";
  report.line = line;
  report.column = col;
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

void Binder::record_warning(Node *node, const std::string &msg, const std::string &code) {
  uint32_t line = node ? node->line : 0;
  uint32_t col = node ? node->column : 0;
  log_warn("[line {}, col {}] {}", line, col, msg);

  Report report;
  report.severity = ReportSeverity::WARNING;
  report.code = code;
  report.message = msg;
  report.source_path = "";
  report.line = line;
  report.column = col;
  if (node && node->source) {
    if (std::holds_alternative<std::filesystem::path>(*node->source)) {
      report.source_path =
          std::get<std::filesystem::path>(*node->source).string();
    } else {
      report.source_path = std::get<std::string>(*node->source);
    }
  }
  if (context.diagnostic) {
    context.diagnostic->record_report(report);
  }
}

// ─── Builtin Primitives Setup ────────────────────────────────────────────────

void Binder::setup_builtins() {
  log_trace(
      "Registering language builtin primitive types into global scope...");
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
  log_trace("Entering new local scope");
  new_scope->parent = current_scope;
  current_scope = new_scope;
}

void Binder::exit_scope() {
  log_trace("Exiting current local scope");
  if (current_scope->parent) {
    current_scope = current_scope->parent;
  }
}

void Binder::declare_local(const std::string &name, Node *node) {
  log_trace("Declaring local variable '{}' in current scope", name);
  if (current_scope->symbols.count(name)) {
    record_error(node,
                 "Variable '" + name + "' is already defined in this scope.");
  } else if (current_scope->parent && current_scope->parent->resolve(name) != nullptr) {
    if (name != "this" && node && node->node_type == NodeType::VAR_DECL) {
      record_warning(node, "Variable '" + name + "' shadows a variable in an outer scope", "W_SHADOW");
    }
  }
  current_scope->define(name, node);
}

// ─── Pass Drivers ────────────────────────────────────────────────────────────

void Binder::register_global_symbols(Node *node, const std::string &prefix) {
  if (!node)
    return;
  BinderPass old_pass = current_pass;
  std::string old_prefix = current_prefix;
  current_pass = BinderPass::REGISTER_GLOBALS;
  current_prefix = prefix;
  node->accept(*this);
  current_pass = old_pass;
  current_prefix = old_prefix;
}

void Binder::register_members(Node *node, const std::string &prefix) {
  if (!node)
    return;
  BinderPass old_pass = current_pass;
  std::string old_prefix = current_prefix;
  current_pass = BinderPass::REGISTER_MEMBERS;
  current_prefix = prefix;
  node->accept(*this);
  current_pass = old_pass;
  current_prefix = old_prefix;
}

void Binder::bind_node(Node *node) {
  if (!node)
    return;
  BinderPass old_pass = current_pass;
  current_pass = BinderPass::BIND_EXECUTION;
  node->accept(*this);
  current_pass = old_pass;
}

TypeInfo Binder::evaluate_expression(Node *expr) {
  if (!expr)
    return {"void", 0};
  BinderPass old_pass = current_pass;
  current_pass = BinderPass::EVALUATE_EXPRESSION;
  expr->accept(*this);
  current_pass = old_pass;
  return evaluated_type;
}

// ─── Symbol Resolution Helpers ───────────────────────────────────────────────

bool Binder::extract_symbol_path(Node *node, std::string &out_path,
                                 std::string &out_root_name) {
  if (!node)
    return false;
  if (node->node_type == NodeType::IDENTIFIER) {
    auto *id = static_cast<IdentifierNode *>(node);
    out_path = id->name;
    out_root_name = id->name;
    return true;
  }
  if (node->node_type == NodeType::MEMBER_ACCESS) {
    auto *mem = static_cast<MemberAccessExpression *>(node);
    std::string prefix;
    if (!extract_symbol_path(mem->object.get(), prefix, out_root_name)) {
      return false;
    }
    out_path = prefix + "." + mem->member_name;
    return true;
  }
  return false;
}

Node *Binder::resolve_symbol(const std::string &name, Node *error_node,
                             bool report_ambiguity) {
  if (name.empty())
    return nullptr;

  // 1. Direct local scope and class hierarchy (only for unqualified names)
  if (name.find('.') == std::string::npos) {
    if (current_scope) {
      Node *declaration = current_scope->resolve(name);
      if (declaration)
        return declaration;
    }
    if (current_class) {
      ClassDeclaration *cls_iter = current_class;
      while (cls_iter) {
        Node *decl = global_scope.resolve(cls_iter->mangled_name + "." + name);
        if (decl)
          return decl;
        if (!cls_iter->base_class_name.empty()) {
          Node *base_node = global_scope.resolve(cls_iter->base_class_name);
          cls_iter = (base_node && base_node->node_type == NodeType::CLASS_DECL)
                         ? static_cast<ClassDeclaration *>(base_node)
                         : nullptr;
        } else {
          break;
        }
      }
    }
  }

  // 2. Check imported symbols table
  auto it = imported_symbols.find(name);
  if (it != imported_symbols.end()) {
    Node *decl = global_scope.resolve(it->second);
    if (decl)
      return decl;
  }

  // 3. Current package context
  std::string node_pkg = (error_node && !error_node->package_context.empty())
                             ? error_node->package_context
                             : current_package;
  if (!node_pkg.empty() && node_pkg.back() != '.')
    node_pkg += '.';
  if (!node_pkg.empty()) {
    Node *decl = global_scope.resolve(node_pkg + name);
    if (decl)
      return decl;
  }

  // 4. Exact match in global scope (e.g. fully qualified "solix.core.String")
  Node *decl = global_scope.resolve(name);
  if (decl)
    return decl;

  // 5. Sub-namespace suffix matching across all registered symbols (Class, Enum, Alias)
  std::vector<std::pair<std::string, Node *>> matches;
  std::string suffix = "." + name;
  for (const auto &[sym_name, sym_node] : global_scope.symbols) {
    if (sym_node->node_type != NodeType::CLASS_DECL &&
        sym_node->node_type != NodeType::ENUM_DECL &&
        sym_node->node_type != NodeType::ALIAS_STMT) {
      continue;
    }
    if (sym_name == name ||
        (sym_name.length() > name.length() &&
         sym_name.compare(sym_name.length() - suffix.length(), suffix.length(),
                          suffix) == 0)) {
      matches.push_back({sym_name, sym_node});
    }
  }

  if (matches.empty()) {
    return nullptr;
  }

  if (matches.size() == 1) {
    return matches[0].second;
  }

  // If multiple matches, prefer current package if present
  if (!node_pkg.empty()) {
    for (const auto &m : matches) {
      if (m.first.rfind(node_pkg, 0) == 0) {
        return m.second;
      }
    }
  }

  // Unify candidates that resolve to the same underlying declaration (e.g. class vs re-export alias)
  auto unwrap = [&](Node *sym) -> Node * {
    while (sym && sym->node_type == NodeType::ALIAS_STMT) {
      auto *al = static_cast<AliasStatement *>(sym);
      if (al->resolved_declaration) {
        sym = al->resolved_declaration;
      } else {
        sym = global_scope.resolve(al->target_type.name);
      }
    }
    return sym;
  };

  std::vector<Node *> unique_decls;
  for (const auto &m : matches) {
    Node *underlying = unwrap(m.second);
    if (!underlying) underlying = m.second;
    bool found = false;
    for (Node *u : unique_decls) {
      if (u == underlying) {
        found = true;
        break;
      }
    }
    if (!found) {
      unique_decls.push_back(underlying);
    }
  }

  if (unique_decls.size() == 1) {
    return unique_decls[0];
  }

  // Ambiguity detected across multiple packages
  if (report_ambiguity && error_node) {
    std::string cand_str;
    for (size_t i = 0; i < matches.size(); ++i) {
      if (i > 0)
        cand_str += ", ";
      cand_str += matches[i].first;
    }
    record_error(error_node, "Ambiguous symbol '" + name +
                                 "': multiple candidates found (" + cand_str +
                                 "). Specify full package or use import to disambiguate.");
  }
  return nullptr;
}

std::string Binder::resolve_template_name(const std::string &template_name,
                                          Node *error_node) {
  if (template_name.empty())
    return "";

  // 1. Check imported symbols
  auto it = imported_symbols.find(template_name);
  if (it != imported_symbols.end() && template_registry.count(it->second)) {
    return it->second;
  }

  // 2. Exact match in template_registry
  if (template_registry.count(template_name)) {
    return template_name;
  }

  // 3. Current package context
  std::string node_pkg = (error_node && !error_node->package_context.empty())
                             ? error_node->package_context
                             : current_package;
  if (!node_pkg.empty() && node_pkg.back() != '.')
    node_pkg += '.';
  if (!node_pkg.empty() && template_registry.count(node_pkg + template_name)) {
    return node_pkg + template_name;
  }

  // 4. Sub-namespace suffix matching across template_registry
  std::vector<std::string> matches;
  std::string suffix = "." + template_name;
  for (const auto &[key, node] : template_registry) {
    if (key == template_name ||
        (key.length() > template_name.length() &&
         key.compare(key.length() - suffix.length(), suffix.length(), suffix) ==
             0)) {
      matches.push_back(key);
    }
  }

  if (matches.empty()) {
    return "";
  }

  if (matches.size() == 1) {
    return matches[0];
  }

  if (!node_pkg.empty()) {
    for (const auto &m : matches) {
      if (m.rfind(node_pkg, 0) == 0) {
        return m;
      }
    }
  }

  if (error_node) {
    std::string cand_str;
    for (size_t i = 0; i < matches.size(); ++i) {
      if (i > 0)
        cand_str += ", ";
      cand_str += matches[i];
    }
    record_error(error_node, "Ambiguous template symbol '" + template_name +
                                 "': multiple candidates found (" + cand_str +
                                 ").");
  }
  return "";
}

void Binder::process_imports() {
  for (auto *n : pending_imports) {
    if (n->symbol_name == "*") {
      std::string target_pkg = n->package_name;
      if (target_pkg.empty())
        continue;
      if (target_pkg.back() != '.')
        target_pkg += '.';
      if (known_packages.count(target_pkg)) {
        // Already exact match
      } else {
        std::vector<std::string> matches;
        for (const auto &pkg : known_packages) {
          if (pkg.length() > target_pkg.length() &&
              pkg.substr(pkg.length() - target_pkg.length()) == target_pkg) {
            matches.push_back(pkg);
          }
        }
        if (matches.size() == 1) {
          known_packages.insert(matches[0]);
          log_debug("Wildcard import resolved '{}' -> '{}'", target_pkg,
                    matches[0]);
        } else if (matches.size() > 1) {
          std::string cand_str;
          for (const auto &m : matches) {
            if (!cand_str.empty())
              cand_str += ", ";
            cand_str += m;
          }
          record_error(n, "Ambiguous wildcard import '" + n->package_name +
                              ".*': matches multiple packages (" + cand_str +
                              ")");
        } else {
          known_packages.insert(target_pkg);
        }
      }
    } else {
      std::string query = n->package_name + "." + n->symbol_name;
      Node *sym = resolve_symbol(query, n, true);
      if (sym) {
        imported_symbols[n->symbol_name] = sym->mangled_name;
        size_t dot = sym->mangled_name.rfind('.');
        if (dot != std::string::npos) {
          known_packages.insert(sym->mangled_name.substr(0, dot + 1));
        }
        log_debug("Symbol import resolved: '{}' -> '{}'", n->symbol_name,
                  sym->mangled_name);
      } else {
        std::string tmpl = resolve_template_name(query, n);
        if (!tmpl.empty()) {
          imported_symbols[n->symbol_name] = tmpl;
          size_t dot = tmpl.rfind('.');
          if (dot != std::string::npos) {
            known_packages.insert(tmpl.substr(0, dot + 1));
          }
          log_debug("Template import resolved: '{}' -> '{}'", n->symbol_name,
                    tmpl);
        } else {
          record_error(n, "Cannot resolve imported symbol: " + query);
        }
      }
    }
  }
  pending_imports.clear();
}

TypeInfo Binder::resolve_type(const TypeInfo &raw_type, Node *error_node) {
  if (raw_type.name.empty()) {
    return raw_type;
  }
  if (raw_type.name == "void" || raw_type.name == "bool" ||
      raw_type.name == "char" || raw_type.name == "int8" ||
      raw_type.name == "uint8" || raw_type.name == "int16" ||
      raw_type.name == "uint16" || raw_type.name == "int32" ||
      raw_type.name == "uint32" || raw_type.name == "int64" ||
      raw_type.name == "uint64" || raw_type.name == "float32" ||
      raw_type.name == "float64") {
    return raw_type;
  }

  log_trace("Resolving type '{}' (depth: {}, type_args: {})", raw_type.name,
            raw_type.array_depth, raw_type.type_args.size());

  Node *resolved = resolve_symbol(raw_type.name, error_node, true);

  TypeInfo result = raw_type;

  if (!resolved && !result.type_args.empty()) {
    std::string template_name = resolve_template_name(result.name, error_node);
    if (!template_name.empty()) {
      log_debug("Resolving type arguments for potential template '{}'",
                template_name);
      std::vector<TypeInfo> resolved_args;
      for (const auto &arg : result.type_args) {
        resolved_args.push_back(resolve_type(arg, error_node));
      }
      result.type_args = resolved_args;

      resolved = instantiate_template(template_name, resolved_args, error_node);
      if (resolved) {
        result.name = resolved->mangled_name;
        result.type_args.clear();
        log_debug("Resolved instantiated template type to '{}'", result.name);
      }
    }
  }

  if (!resolved) {
    record_error(error_node, "Unknown type: " + raw_type.name);
  }
  if (resolved && resolved->node_type == NodeType::ALIAS_STMT) {
    auto *alias = static_cast<AliasStatement *>(resolved);
    log_trace("Resolving alias '{}' -> '{}'", alias->alias_name,
              alias->target_type.to_string());
    result.name = alias->target_type.name;
    result.array_depth += alias->target_type.array_depth;
    result.type_args = alias->target_type.type_args;
    return resolve_type(result, error_node);
  } else if (resolved && (resolved->node_type == NodeType::CLASS_DECL ||
                          resolved->node_type == NodeType::ENUM_DECL)) {
    result.name = resolved->mangled_name;
  }

  return result;
}

// ─── Pass 2: Type & Memory Binding ──────────────────────────────────────────

void Binder::bind_types_and_memory() {
  log_debug("Starting Pass 2: Type and Memory Binding...");
  static_variable_index = 1;

  for (const auto &[name, node] : global_scope.symbols) {
    if (node->node_type == NodeType::ALIAS_STMT) {
      auto *alias = static_cast<AliasStatement *>(node);
      Node *target = resolve_symbol(alias->target_type.name, alias, false);
      if (target) {
        alias->resolved_declaration = target;
      } else {
        std::string tmpl = resolve_template_name(alias->target_type.name, alias);
        if (!tmpl.empty() && template_registry.count(tmpl)) {
          alias->resolved_declaration = template_registry[tmpl];
        }
      }
    }
  }

  for (const auto &[name, node] : global_scope.symbols) {
    if (node->node_type == NodeType::ALIAS_STMT) {
      std::unordered_set<Node *> visited;
      Node *curr = node;
      while (curr && curr->node_type == NodeType::ALIAS_STMT) {
        if (visited.count(curr)) {
          record_error(node, "Circular alias detected in '" + static_cast<AliasStatement *>(node)->alias_name + "'");
          break;
        }
        visited.insert(curr);
        curr = static_cast<AliasStatement *>(curr)->resolved_declaration;
      }
    }
  }

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
        log_debug("Bound static field '{}' to static index {}", name,
                  field->memory_index);
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
      log_trace("Resolved return type for method '{}' -> '{}'", name,
                method->return_type.to_string());
      current_class = nullptr;
    }
  }

  auto unwrap_alias = [&](Node *n) -> Node * {
    while (n && n->node_type == NodeType::ALIAS_STMT) {
      auto *alias_stmt = static_cast<AliasStatement *>(n);
      if (alias_stmt->resolved_declaration) {
        n = alias_stmt->resolved_declaration;
      } else {
        n = resolve_symbol(alias_stmt->target_type.name, alias_stmt, false);
        alias_stmt->resolved_declaration = n;
      }
    }
    return n;
  };

  auto resolve_base_class = [&](ClassDeclaration *cls) -> Node * {
    if (cls->base_class_name.empty()) return nullptr;
    Node *node = resolve_symbol(cls->base_class_name, cls, true);
    node = unwrap_alias(node);
    if (node && node->node_type == NodeType::CLASS_DECL) {
      cls->base_class_name = static_cast<ClassDeclaration *>(node)->mangled_name;
      return node;
    }
    record_error(cls, "Base class not found: " + cls->base_class_name);
    return nullptr;
  };

  for (const auto &[name, node] : global_scope.symbols) {
    if (node->node_type == NodeType::CLASS_DECL) {
      resolve_base_class(static_cast<ClassDeclaration *>(node));
    }
  }

  std::unordered_map<std::string, std::vector<MethodDeclaration *>> vtables;
  std::unordered_set<std::string> vtable_calculated;
  std::unordered_set<std::string> vtable_in_progress;

  auto get_method_sig = [](const std::string &mangled) -> std::string {
    size_t paren = mangled.find('(');
    if (paren == std::string::npos) {
      size_t last_dot = mangled.rfind('.');
      return (last_dot == std::string::npos) ? mangled : mangled.substr(last_dot + 1);
    }
    size_t last_dot = mangled.rfind('.', paren);
    if (last_dot == std::string::npos) {
      return mangled;
    }
    return mangled.substr(last_dot + 1);
  };

  std::function<void(ClassDeclaration *)> calculate_vtable =
      [&](ClassDeclaration *cls) {
        if (vtable_calculated.count(cls->mangled_name))
          return;

        if (vtable_in_progress.count(cls->mangled_name)) {
          record_error(cls, "Circular inheritance detected for class '" + cls->class_name + "'");
          return;
        }
        vtable_in_progress.insert(cls->mangled_name);

        log_trace("Calculating vtable for class '{}'", cls->mangled_name);
        std::vector<MethodDeclaration *> vtable;
        if (!cls->base_class_name.empty()) {
          Node *base_node = unwrap_alias(global_scope.resolve(cls->base_class_name));
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
                std::string base_sig = get_method_sig(vtable[i]->mangled_name);
                std::string drv_sig = get_method_sig(method->mangled_name);
                if (base_sig == drv_sig) {
                  vtable[i] = method;
                  method->vtable_index = i;
                  method->is_virtual = true;
                  found = true;
                  log_trace("VTable override: {} at index {}",
                            method->mangled_name, i);
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
              log_trace("VTable addition: {} assigned slot {}",
                        method->mangled_name, method->vtable_index);
            }
          }
        }
        vtable_in_progress.erase(cls->mangled_name);
        vtables[cls->mangled_name] = vtable;
        cls->vtable = vtable;
        vtable_calculated.insert(cls->mangled_name);

        if (!cls->is_abstract) {
          for (MethodDeclaration *m : cls->vtable) {
            if (m->is_abstract) {
              std::string parent_name = (m->parent && m->parent->node_type == NodeType::CLASS_DECL)
                                            ? static_cast<ClassDeclaration *>(m->parent)->class_name
                                            : "";
              record_error(cls, "Class '" + cls->class_name + "' must implement abstract method '" + m->method_name + "()' from '" + parent_name + "'");
            }
          }
        }
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

  auto is_exception_class = [&](ClassDeclaration *cls) -> bool {
    std::unordered_set<std::string> visited;
    std::string current_name = cls->mangled_name;
    while (!current_name.empty()) {
      if (visited.count(current_name)) break;
      visited.insert(current_name);
      if (current_name == "Exception" || current_name == "Throwable" ||
          current_name.ends_with(".Exception") || current_name.ends_with(".Throwable")) return true;
      Node *node = global_scope.resolve(current_name);
      if (node && node->node_type == NodeType::CLASS_DECL) {
        current_name = static_cast<ClassDeclaration *>(node)->base_class_name;
      } else {
        break;
      }
    }
    return false;
  };

  int next_vtable_id = 0;
  for (const auto &[name, node] : global_scope.symbols) {
    if (node->node_type == NodeType::CLASS_DECL) {
      auto *cls = static_cast<ClassDeclaration *>(node);
      if (!vtables[cls->mangled_name].empty() || is_exception_class(cls)) {
        cls->vtable_id = next_vtable_id++;
        log_debug("Assigned vtable_id {} to class '{}'", cls->vtable_id,
                  cls->mangled_name);
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
          log_trace("Class '{}' linked to base vtable_id {}", cls->mangled_name,
                    cls->base_vtable_id);
        }
      }
    }
  }

  std::unordered_set<std::string> layout_calculated;
  std::unordered_set<std::string> layout_in_progress;
  std::function<int(ClassDeclaration *)> calculate_layout =
      [&](ClassDeclaration *cls) -> int {
    if (layout_calculated.count(cls->mangled_name))
      return cls->instance_size;
    if (layout_in_progress.count(cls->mangled_name))
      return 1;
    layout_in_progress.insert(cls->mangled_name);

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
          log_trace("Field '{}' in '{}' assigned instance offset {}",
                    field->field_name, cls->mangled_name, field->memory_index);
        }
      }
    }
    layout_in_progress.erase(cls->mangled_name);
    cls->instance_size = offset;
    layout_calculated.insert(cls->mangled_name);
    log_debug("Class '{}' instance layout computed: size = {} words",
              cls->mangled_name, cls->instance_size);
    return cls->instance_size;
  };

  for (const auto &[name, node] : global_scope.symbols) {
    if (node->node_type == NodeType::CLASS_DECL) {
      calculate_layout(static_cast<ClassDeclaration *>(node));
    }
  }
}

// ─── Binder::execute ─────────────────────────────────────────────────────────

void Binder::execute() {
  log_debug("Starting Semantic Analysis (Binding)...");

  setup_builtins();

  log_debug("Pass 1a: Registering package and top-level symbols...");
  for (const auto &[source, nodes] : context.nodes) {
    current_package = "";
    current_prefix = "";
    for (const auto &node : nodes) {
      if (node->node_type == NodeType::PACKAGE_STMT) {
        // PackageStatement is a file-level context directive — call the visitor
        // directly so the package state persists across all sibling nodes in
        // this file (wrapper functions save/restore state, which would undo it).
        current_pass = BinderPass::REGISTER_GLOBALS;
        node->accept(*this);
      } else {
        register_global_symbols(node.get(), current_prefix);
      }
    }
  }

  process_imports();

  log_debug("Pass 1b: Registering class members and signatures...");
  for (const auto &[source, nodes] : context.nodes) {
    current_package = "";
    current_prefix = "";
    for (const auto &node : nodes) {
      if (node->node_type == NodeType::PACKAGE_STMT) {
        current_pass = BinderPass::REGISTER_MEMBERS;
        node->accept(*this);
      } else {
        register_members(node.get(), current_prefix);
      }
    }
  }

  log_debug("Pass 1 complete. Registered {} global symbols, {} templates in "
            "registry.",
            global_scope.symbols.size(), template_registry.size());

  bind_types_and_memory();

  log_debug("Memory mapping complete. Total static variables: {}",
            static_variable_index);

  log_debug("Pass 3: Binding statement execution logic and bodies...");
  for (const auto &[source, nodes] : context.nodes) {
    current_package = "";
    for (const auto &node : nodes) {
      if (node->node_type == NodeType::PACKAGE_STMT) {
        current_pass = BinderPass::BIND_EXECUTION;
        node->accept(*this);
      } else {
        bind_tree(node.get());
      }
    }
  }

  if (context.diagnostic->has_errors()) {
    log_error("Semantic Analysis completed with errors.");
    std::string all_errors = "Semantic Analysis failed with errors:\n";
    for (const auto &r : context.diagnostic->get_reports()) {
      all_errors += r.message + "\n";
    }
    throw BindError(all_errors);
  }

  log_debug("Semantic Analysis completed successfully.");
}

// ─── Pass 3 Tree Traversal ───────────────────────────────────────────────────

void Binder::bind_tree(Node *root) {
  if (!root)
    return;

  struct StateGuard {
    Binder *b;
    ClassDeclaration *pc;
    MethodDeclaration *pm;
    uint32_t pl;
    std::string ppkg;
    StateGuard(Binder *b)
        : b(b), pc(b->current_class), pm(b->current_method),
          pl(b->local_variable_index), ppkg(b->current_package) {}
    ~StateGuard() {
      b->current_class = pc;
      b->current_method = pm;
      b->local_variable_index = pl;
      b->current_package = ppkg;
    }
  } guard(this);

  if (root->node_type == NodeType::CLASS_DECL) {
    auto *cls = static_cast<ClassDeclaration *>(root);
    if (!cls->template_parameters.empty()) {
      log_trace(
          "Skipping uninstantiated template class blueprint '{}' in bind_tree",
          cls->class_name);
      return;
    }
    if (!cls->package_context.empty()) {
      current_package = cls->package_context;
    }
    log_debug("Binding AST tree for class '{}'", cls->class_name);
    current_class = cls;
  } else if (root->node_type == NodeType::METHOD_DECL) {
    auto *mth = static_cast<MethodDeclaration *>(root);
    if (!mth->template_parameters.empty()) {
      log_trace(
          "Skipping uninstantiated template method blueprint '{}' in bind_tree",
          mth->method_name);
      return;
    }
    current_method = static_cast<MethodDeclaration *>(root);
    local_variable_index = 0;

    current_method->return_type =
        resolve_type(current_method->return_type, current_method);
    log_debug("Binding method '{}' (return type: '{}')",
              current_method->method_name,
              current_method->return_type.to_string());

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
      log_trace("Bound parameter '{}' to frame slot {}", param_var->var_name,
                param_var->memory_index);
    }

    for (const auto &child : current_method->children) {
      bind_node(child.get());
    }

    current_method->frame_size = local_variable_index;
    log_debug("Method '{}' frame size resolved to {} words",
              current_method->method_name, current_method->frame_size);
    exit_scope();
    return;
  } else if (root->node_type == NodeType::CONSTRUCTOR_DECL) {
    auto *ctor = static_cast<ConstructorDeclaration *>(root);
    local_variable_index = 0;
    log_debug("Binding constructor for class '{}'", ctor->class_name);

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
      log_trace("Bound ctor parameter '{}' to frame slot {}",
                param_var->var_name, param_var->memory_index);
    }

    for (const auto &child : ctor->children) {
      bind_node(child.get());
    }

    ctor->frame_size = local_variable_index;
    log_debug("Constructor for '{}' frame size resolved to {} words",
              ctor->class_name, ctor->frame_size);
    exit_scope();
    return;
  } else if (root->node_type == NodeType::FIELD_DECL) {
    bind_node(root);
    return;
  }

  for (const auto &child : root->children) {
    if (child)
      bind_tree(child.get());
  }
}

// ─── Helpers ─────────────────────────────────────────────────────────────────

bool Binder::is_assignable(const TypeInfo &target, const TypeInfo &source) {
  if (target == source)
    return true;
  // null (void) is assignable to any reference type or array
  if (source.name == "void" && source.array_depth == 0) {
    // Allow assigning null to arrays or to classes (reference types)
    if (target.array_depth > 0) return true;
    Node *tgt_node = global_scope.resolve(target.name);
    if (tgt_node && tgt_node->node_type == NodeType::CLASS_DECL) return true;
  }
  // Allow numeric conversions
  auto is_integer = [](const std::string &name) {
    return name == "int8" || name == "int16" || name == "int32" || name == "int64" ||
           name == "uint8" || name == "uint16" || name == "uint32" || name == "uint64" ||
           name == "char";
  };
  auto is_floating = [](const std::string &name) {
    return name == "float32" || name == "float64";
  };
  if (target.array_depth == 0 && source.array_depth == 0) {
    if (is_integer(target.name) && is_integer(source.name))
      return true;
    if (is_floating(target.name) && is_floating(source.name))
      return true;
  }
  // Allow char[] to String object assignment / parameter passing
  if (target.array_depth == 0 && (target.name == "String" || target.name.ends_with(".String"))) {
    if (source.array_depth == 1 && source.name == "char") {
      return true;
    }
  }

  if (target.array_depth != source.array_depth)
    return false;

  Node *target_node = global_scope.resolve(target.name);
  Node *src_node = global_scope.resolve(source.name);
  while (src_node && src_node->node_type == NodeType::CLASS_DECL) {
    auto *cls = static_cast<ClassDeclaration *>(src_node);
    if (src_node == target_node || cls->mangled_name == target.name || cls->class_name == target.name)
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

    Node *declaration = resolve_symbol(n.name, &n, true);

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
    } else if (declaration->node_type == NodeType::ALIAS_STMT) {
      auto *alias = static_cast<AliasStatement *>(declaration);
      n.expression_type = alias->target_type;
      Node *target_decl = global_scope.resolve(alias->target_type.name);
      if (target_decl) {
        n.resolved_declaration = target_decl;
      }
    } else {
      record_error(&n, "Invalid identifier usage: " + n.name);
      evaluated_type = {"void", 0};
      return;
    }
    evaluated_type = n.expression_type;
    log_trace("Evaluated identifier '{}' -> type '{}'", n.name,
              evaluated_type.to_string());
  }
}

void Binder::visit(LiteralNode &n) {
  if (current_pass == BinderPass::EVALUATE_EXPRESSION) {
    if (n.token_type == TokenType::CHAR)
      n.expression_type = {"char", 0};
    else if (std::holds_alternative<int64_t>(n.value))
      n.expression_type = {"int32", 0};
    else if (std::holds_alternative<double>(n.value))
      n.expression_type = {"float64", 0};
    else if (std::holds_alternative<std::string>(n.value)) {
      n.expression_type = {"char", 1};
      std::string str = std::get<std::string>(n.value);
      if (context.string_pool.count(str)) {
        n.memory_index = context.string_pool[str];
      } else {
        int idx = static_variable_index++;
        context.string_pool[str] = idx;
        n.memory_index = idx;
      }
    } else
      n.expression_type = {"void", 0};
    evaluated_type = n.expression_type;
    log_trace("Evaluated literal -> type '{}'", evaluated_type.to_string());
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
      log_debug("Resolved overloaded binary operator: {}", mangled);
      return;
    }

  primitive_fallback:
    {
      bool is_null_compare = (n.op == TokenType::OPERATOR_EQUAL || n.op == TokenType::OPERATOR_NOT_EQUAL) &&
                             (left_type.name == "void" || right_type.name == "void");
      auto is_integer = [](const std::string &name) {
        return name == "int8" || name == "int16" || name == "int32" || name == "int64" ||
               name == "uint8" || name == "uint16" || name == "uint32" || name == "uint64" ||
               name == "char";
      };
      bool is_integer_math = left_type.array_depth == 0 && right_type.array_depth == 0 &&
                             is_integer(left_type.name) && is_integer(right_type.name);
      if (left_type != right_type && !is_null_compare && !is_integer_math) {
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
      log_trace("Evaluated binary operation -> '{}'", evaluated_type.to_string());
    }
  }
}

void Binder::visit(UnaryExpression &n) {
  if (current_pass == BinderPass::EVALUATE_EXPRESSION) {
    n.expression_type = evaluate_expression(n.operand.get());
    evaluated_type = n.expression_type;
    log_trace("Evaluated unary operation -> '{}'", evaluated_type.to_string());
  }
}

void Binder::visit(AssignmentExpression &n) {
  if (current_pass == BinderPass::EVALUATE_EXPRESSION) {
    TypeInfo target_type = evaluate_expression(n.target.get());
    TypeInfo value_type = evaluate_expression(n.value.get());

    Node *left_decl = global_scope.resolve(target_type.name);
    if (left_decl && left_decl->node_type == NodeType::CLASS_DECL &&
        n.value->node_type != NodeType::NEW_INSTANCE &&
        n.target->node_type != NodeType::MEMBER_ACCESS) {
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
        log_debug("Resolved overloaded assignment operator: {}", mangled);
        return;
      }
    }
    if (!is_assignable(target_type, value_type)) {
      record_error(&n, "Assignment type mismatch: '" + target_type.name +
                           "' = '" + value_type.name + "'");
    }
    n.expression_type = target_type;
    evaluated_type = n.expression_type;
    log_trace("Evaluated assignment expression -> '{}'",
              evaluated_type.to_string());
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
    log_trace("Evaluated array access -> element type '{}'",
              evaluated_type.to_string());
  }
}

void Binder::visit(MemberAccessExpression &n) {
  if (current_pass == BinderPass::EVALUATE_EXPRESSION) {
    TypeInfo object_type;
    Node *type_decl = nullptr;

    std::string sym_path, root_name;
    bool is_static_path = false;
    if (extract_symbol_path(n.object.get(), sym_path, root_name)) {
      bool is_local_var = false;
      if (current_scope) {
        Node *local_sym = current_scope->resolve(root_name);
        if (local_sym && local_sym->node_type == NodeType::VAR_DECL) {
          is_local_var = true;
        }
      }
      if (!is_local_var && current_class) {
        Node *cls_field = global_scope.resolve(current_class->mangled_name + "." + root_name);
        if (cls_field && cls_field->node_type == NodeType::FIELD_DECL &&
            !static_cast<FieldDeclaration *>(cls_field)->is_static) {
          is_local_var = true;
        }
      }
      if (!is_local_var) {
        Node *target = resolve_symbol(sym_path, &n, true);
        while (target && target->node_type == NodeType::ALIAS_STMT) {
          auto *al = static_cast<AliasStatement *>(target);
          target = al->resolved_declaration ? al->resolved_declaration : resolve_symbol(al->target_type.name, al, false);
        }
        if (target && (target->node_type == NodeType::CLASS_DECL || target->node_type == NodeType::ENUM_DECL)) {
          type_decl = target;
          object_type = {type_decl->mangled_name, 0};
          is_static_path = true;
        }
      }
    }

    if (!is_static_path) {
      object_type = evaluate_expression(n.object.get());
      if (object_type.array_depth > 0) {
        if (n.member_name == "length") {
          n.expression_type = {"int32", 0};
          evaluated_type = n.expression_type;
          return;
        }
        record_error(&n, "Arrays only have the 'length' property");
      }
      type_decl = global_scope.resolve(object_type.name);
    }

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
        log_trace("Resolved field member access '{}.{}' -> '{}'",
                  object_type.name, n.member_name, evaluated_type.to_string());
        return;
      }
      if (member_decl->node_type == NodeType::CLASS_DECL ||
          member_decl->node_type == NodeType::ENUM_DECL) {
        n.expression_type = {member_decl->mangled_name, 0};
        evaluated_type = n.expression_type;
        log_trace("Resolved nested type access '{}.{}' -> '{}'",
                  object_type.name, n.member_name, evaluated_type.to_string());
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
      log_trace("Resolved enum member access '{}.{}' = {}", object_type.name,
                n.member_name, e_val);
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
      TypeInfo receiver_type = {"void", 0};
      ClassDeclaration *current_resolve_class = nullptr;

      std::string sym_path, root_name;
      bool is_static_path = false;
      if (extract_symbol_path(member_access->object.get(), sym_path, root_name)) {
        bool is_local_var = false;
        if (current_scope) {
          Node *local_sym = current_scope->resolve(root_name);
          if (local_sym && local_sym->node_type == NodeType::VAR_DECL) {
            is_local_var = true;
          }
        }
        if (!is_local_var && current_class) {
          Node *cls_field = global_scope.resolve(current_class->mangled_name + "." + root_name);
          if (cls_field && cls_field->node_type == NodeType::FIELD_DECL &&
              !static_cast<FieldDeclaration *>(cls_field)->is_static) {
            is_local_var = true;
          }
        }
        if (!is_local_var) {
          Node *target = resolve_symbol(sym_path, &n, true);
          while (target && target->node_type == NodeType::ALIAS_STMT) {
            auto *al = static_cast<AliasStatement *>(target);
            target = al->resolved_declaration ? al->resolved_declaration : resolve_symbol(al->target_type.name, al, false);
          }
          if (target && target->node_type == NodeType::CLASS_DECL) {
            current_resolve_class = static_cast<ClassDeclaration *>(target);
            receiver_type = {current_resolve_class->mangled_name, 0};
            is_static_path = true;
          }
        }
      }

      if (!is_static_path) {
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
          while (type_decl && type_decl->node_type == NodeType::ALIAS_STMT) {
            auto *alias_stmt = static_cast<AliasStatement *>(type_decl);
            if (alias_stmt->resolved_declaration) {
              type_decl = alias_stmt->resolved_declaration;
            } else {
              type_decl = resolve_symbol(alias_stmt->target_type.name, alias_stmt, false);
            }
          }
          if (type_decl && type_decl->node_type == NodeType::CLASS_DECL) {
            current_resolve_class = static_cast<ClassDeclaration *>(type_decl);
          }
        }
      }

      Node *method_decl = nullptr;
      std::string base_name;
      std::string mangled_name;

      while (!method_decl && current_resolve_class) {
        base_name = current_resolve_class->mangled_name + "." +
                    member_access->member_name;
        if (!n.type_args.empty() && template_registry.count(base_name)) {
          std::vector<TypeInfo> resolved_targs;
          for (auto &t : n.type_args)
            resolved_targs.push_back(resolve_type(t, &n));
          instantiate_template(base_name, resolved_targs, &n);
          base_name += "<";
          for (size_t i = 0; i < resolved_targs.size(); ++i) {
            base_name += resolved_targs[i].to_string();
            if (i < resolved_targs.size() - 1)
              base_name += ",";
          }
          base_name += ">";
        }
        mangled_name = mangle_method_call(base_name, argument_types);
        method_decl = global_scope.resolve(mangled_name);
        if (!method_decl)
          method_decl = global_scope.resolve(base_name);

        // Implicit generic deduction for member method templates
        if (!method_decl && n.type_args.empty() &&
            template_registry.count(base_name)) {
          Node *blueprint = template_registry[base_name];
          if (blueprint->node_type == NodeType::METHOD_DECL) {
            auto *method_bp = static_cast<MethodDeclaration *>(blueprint);
            std::vector<TypeInfo> param_types;
            for (const auto &p : method_bp->parameters) {
              param_types.push_back(
                  static_cast<VariableDeclaration *>(p.get())->type_info);
            }

            std::vector<TypeInfo> deduced_args;
            if (deduce_template_arguments(param_types, argument_types,
                                          method_bp->template_parameters,
                                          deduced_args)) {
              std::string instantiated_base = base_name + "<";
              for (size_t i = 0; i < deduced_args.size(); ++i) {
                instantiated_base += deduced_args[i].to_string();
                if (i < deduced_args.size() - 1)
                  instantiated_base += ",";
              }
              instantiated_base += ">";
              mangled_name =
                  mangle_method_call(instantiated_base, argument_types);
              method_decl = global_scope.resolve(mangled_name);
              if (!method_decl) {
                instantiate_template(base_name, deduced_args, &n);
                method_decl = global_scope.resolve(mangled_name);
              }
            }
          }
        }

        if (!method_decl) {
          for (const auto &child : current_resolve_class->children) {
            if (child && child->node_type == NodeType::METHOD_DECL) {
              auto *m = static_cast<MethodDeclaration *>(child.get());
              if (m->method_name == member_access->member_name &&
                  m->parameters.size() == argument_types.size()) {
                bool match = true;
                for (size_t i = 0; i < argument_types.size(); ++i) {
                  auto *p_var = static_cast<VariableDeclaration *>(m->parameters[i].get());
                  if (!is_assignable(p_var->type_info, argument_types[i])) {
                    match = false;
                    break;
                  }
                }
                if (match) {
                  method_decl = m;
                  break;
                }
              }
            }
          }
        }

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
      log_debug("Resolved member method call: '{}' (virtual: {})", mangled_name,
                n.is_virtual_call);
      return;
    } else {
      auto *id = static_cast<IdentifierNode *>(n.callee.get());

      std::string base_name;
      std::string mangled_name;
      Node *method_decl = nullptr;
      ClassDeclaration *current_resolve_class = current_class;

      // 1. Try resolving against the class hierarchy first
      if (current_resolve_class) {
        if (id->name == "super") {
          if (current_class->base_class_name.empty()) {
            record_error(&n,
                         "Cannot call super() in a class without a base class");
            evaluated_type = {"void", 0};
            return;
          }
          Node *base_node =
              global_scope.resolve(current_class->base_class_name);
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
          if (!method_decl) {
            for (const auto &child : current_resolve_class->children) {
              if (child && child->node_type == NodeType::CONSTRUCTOR_DECL) {
                auto *c = static_cast<ConstructorDeclaration *>(child.get());
                if (c->parameters.size() == argument_types.size()) {
                  bool match = true;
                  for (size_t i = 0; i < argument_types.size(); ++i) {
                    auto *p_var = static_cast<VariableDeclaration *>(c->parameters[i].get());
                    if (!is_assignable(p_var->type_info, argument_types[i])) {
                      match = false;
                      break;
                    }
                  }
                  if (match) {
                    method_decl = c;
                    break;
                  }
                }
              }
            }
          }
        } else {
          while (!method_decl && current_resolve_class) {
            base_name = current_resolve_class->mangled_name + "." + id->name;
            if (!n.type_args.empty() && template_registry.count(base_name)) {
              std::vector<TypeInfo> resolved_targs;
              for (auto &t : n.type_args)
                resolved_targs.push_back(resolve_type(t, &n));
              instantiate_template(base_name, resolved_targs, &n);
              base_name += "<";
              for (size_t i = 0; i < resolved_targs.size(); ++i) {
                base_name += resolved_targs[i].to_string();
                if (i < resolved_targs.size() - 1)
                  base_name += ",";
              }
              base_name += ">";
            }
            mangled_name = mangle_method_call(base_name, argument_types);
            method_decl = global_scope.resolve(mangled_name);
            if (!method_decl)
              method_decl = global_scope.resolve(base_name);

            if (!method_decl) {
              for (const auto &child : current_resolve_class->children) {
                if (child && child->node_type == NodeType::METHOD_DECL) {
                  auto *m = static_cast<MethodDeclaration *>(child.get());
                  if (m->method_name == id->name &&
                      m->parameters.size() == argument_types.size()) {
                    bool match = true;
                    for (size_t i = 0; i < argument_types.size(); ++i) {
                      auto *p_var = static_cast<VariableDeclaration *>(m->parameters[i].get());
                      if (!is_assignable(p_var->type_info, argument_types[i])) {
                        match = false;
                        break;
                      }
                    }
                    if (match) {
                      method_decl = m;
                      break;
                    }
                  }
                }
              }
            }

            if (!method_decl &&
                !current_resolve_class->base_class_name.empty()) {
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
      }

      // 2. If not found in class hierarchy, fallback to global resolution
      if (!method_decl && id->name != "super") {
        current_resolve_class = nullptr; // Reset to indicate global lookup
        base_name = id->name;

        // Prepend the active package namespace to find templates and global
        // functions properly
        if (!template_registry.count(base_name) && !current_package.empty() &&
            template_registry.count(current_package + base_name)) {
          base_name = current_package + base_name;
        } else if (!global_scope.symbols.count(base_name) &&
                   !current_package.empty() &&
                   global_scope.symbols.count(current_package + base_name)) {
          base_name = current_package + base_name;
        } else if (imported_symbols.count(base_name)) {
          base_name = imported_symbols[base_name];
        } else {
          for (const auto &pkg : known_packages) {
            std::string p = (pkg.empty() || pkg.back() == '.') ? pkg : pkg + '.';
            if (template_registry.count(p + base_name)) {
              base_name = p + base_name;
              break;
            }
            if (global_scope.symbols.count(p + base_name)) {
              base_name = p + base_name;
              break;
            }
          }
        }

        if (!n.type_args.empty()) {
          std::vector<TypeInfo> resolved_targs;
          for (auto &t : n.type_args)
            resolved_targs.push_back(resolve_type(t, &n));
          instantiate_template(base_name, resolved_targs, &n);
          base_name += "<";
          for (size_t i = 0; i < resolved_targs.size(); ++i) {
            base_name += resolved_targs[i].to_string();
            if (i < resolved_targs.size() - 1)
              base_name += ",";
          }
          base_name += ">";
        }
        mangled_name = mangle_method_call(base_name, argument_types);
        method_decl = global_scope.resolve(mangled_name);
        if (!method_decl)
          method_decl = global_scope.resolve(base_name);

        // Implicit generic deduction
        if (!method_decl && n.type_args.empty() &&
            template_registry.count(base_name)) {
          Node *blueprint = template_registry[base_name];
          if (blueprint->node_type == NodeType::METHOD_DECL) {
            auto *method_bp = static_cast<MethodDeclaration *>(blueprint);
            std::vector<TypeInfo> param_types;
            for (const auto &p : method_bp->parameters) {
              param_types.push_back(
                  static_cast<VariableDeclaration *>(p.get())->type_info);
            }

            std::vector<TypeInfo> deduced_args;
            if (deduce_template_arguments(param_types, argument_types,
                                          method_bp->template_parameters,
                                          deduced_args)) {
              std::string instantiated_base = base_name + "<";
              for (size_t i = 0; i < deduced_args.size(); ++i) {
                instantiated_base += deduced_args[i].to_string();
                if (i < deduced_args.size() - 1)
                  instantiated_base += ",";
              }
              instantiated_base += ">";
              mangled_name =
                  mangle_method_call(instantiated_base, argument_types);
              method_decl = global_scope.resolve(mangled_name);
              if (!method_decl) {
                instantiate_template(base_name, deduced_args, &n);
                method_decl = global_scope.resolve(mangled_name);
              }
            }
          }
        }

        if (!method_decl) {
          std::string search_prefix = base_name + "(";
          for (const auto &[sym_name, sym_node] : global_scope.symbols) {
            if (sym_node->node_type == NodeType::METHOD_DECL &&
                sym_name.find(search_prefix) == 0) {
              auto *candidate = static_cast<MethodDeclaration *>(sym_node);
              if (candidate->parameters.size() == argument_types.size()) {
                bool match = true;
                for (size_t i = 0; i < argument_types.size(); ++i) {
                  auto *p_var = static_cast<VariableDeclaration *>(candidate->parameters[i].get());
                  if (!is_assignable(p_var->type_info, argument_types[i])) {
                    match = false;
                    break;
                  }
                }
                if (match) {
                  method_decl = candidate;
                  break;
                }
              }
            }
          }
        }
      }

      if (!method_decl) {
        record_error(&n, "No matching method or global function: " + id->name);
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
      log_debug("Resolved local/global method call: '{}' (virtual: {})",
                mangled_name.empty() ? base_name : mangled_name,
                n.is_virtual_call);
      return;
    }
  }
}

void Binder::visit(NewInstanceExpression &n) {
  if (current_pass == BinderPass::EVALUATE_EXPRESSION) {
    n.type_info = resolve_type(n.type_info, &n);
    log_debug("Binding new instance instantiation for type '{}'",
              n.type_info.to_string());
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
            bool match = true;
            for (size_t p = 0; p < argument_types.size(); ++p) {
              auto *p_var = static_cast<VariableDeclaration *>(candidate->parameters[p].get());
              if (!is_assignable(p_var->type_info, argument_types[p])) {
                match = false;
                break;
              }
            }
            if (match) {
              ctor = sym_node;
              break;
            }
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
    log_trace("Resolved new instance constructor -> '{}'", mangled_ctor);
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
    log_trace("Evaluated array creation -> '{}'", evaluated_type.to_string());
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
        if (is_assignable(element_type, current_element_type)) {
          // current_element_type is assignable to element_type (e.g. Dog to Animal)
        } else if (is_assignable(current_element_type, element_type)) {
          // element_type is assignable to current_element_type (e.g. Animal to Dog)
          element_type = current_element_type;
        } else {
          bool found_common = false;
          TypeInfo ancestor = element_type;
          while (!ancestor.name.empty()) {
            Node *decl = global_scope.resolve(ancestor.name);
            if (decl && decl->node_type == NodeType::CLASS_DECL) {
              auto *cls = static_cast<ClassDeclaration *>(decl);
              if (!cls->base_class_name.empty()) {
                ancestor.name = cls->base_class_name;
                if (is_assignable(ancestor, current_element_type)) {
                  element_type = ancestor;
                  found_common = true;
                  break;
                }
              } else {
                break;
              }
            } else {
              break;
            }
          }
          if (!found_common) {
            record_error(&n, "Mixed types in array literal");
          }
        }
      }
    }
    element_type.array_depth++;
    n.expression_type = element_type;
    evaluated_type = n.expression_type;
    log_trace("Evaluated array literal -> '{}'", evaluated_type.to_string());
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
    // (T)null — casting from null (void) to any type is always valid
    bool source_is_null = (source_type.name == "void" && source_type.array_depth == 0);
    if (source_is_null) {
      // null can be cast to any reference type; emit as null
    } else if (target_prim && source_prim) {
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
    log_trace("Evaluated explicit cast: '{}' -> '{}'", source_type.to_string(),
              n.target_type.to_string());
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
    log_trace("Evaluated instanceof expression: checking '{}' against '{}'",
              source_type.to_string(), n.target_type.to_string());
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
    log_trace("Evaluated ternary expression -> result type '{}'",
              evaluated_type.to_string());
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
    log_trace("Bound local variable '{}' of type '{}' to frame index {}",
              n.var_name, n.type_info.to_string(), n.memory_index);
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
    current_prefix = current_package;
    known_packages.insert(current_package);
    n.mangled_name = n.package_name;
    global_scope.define(n.mangled_name, &n);
    log_debug("Configured active package: '{}'", n.package_name);
  } else if (current_pass == BinderPass::REGISTER_MEMBERS) {
    current_package = n.package_name + ".";
    current_prefix = current_package;
  } else if (current_pass == BinderPass::BIND_EXECUTION) {
    current_package = n.package_name + ".";
  }
}

void Binder::visit(AliasStatement &n) {
  if (!n.template_parameters.empty()) {
    if (current_pass == BinderPass::REGISTER_GLOBALS) {
      std::string full_name = current_prefix + n.alias_name;
      n.package_context = current_prefix;
      template_registry[full_name] = &n;
      log_debug("Registered alias template blueprint: '{}' (params: {})",
                full_name, n.template_parameters.size());
    }
    return;
  }
  if (current_pass == BinderPass::REGISTER_GLOBALS) {
    std::string full_name = current_prefix + n.alias_name;
    n.package_context = current_prefix;
    if (global_scope.symbols.count(full_name))
      record_error(&n, "Duplicate global symbol: " + full_name);
    n.mangled_name = full_name;
    global_scope.define(full_name, &n);
    log_debug("Registered global alias: '{}'", full_name);
  }
}

void Binder::visit(ImportStatement &n) {
  if (current_pass == BinderPass::REGISTER_GLOBALS) {
    pending_imports.push_back(&n);
  }
}

void Binder::visit(EnumDeclaration &n) {
  if (current_pass == BinderPass::REGISTER_GLOBALS) {
    std::string full_name = current_prefix + n.enum_name;
    n.package_context = current_prefix;
    if (global_scope.symbols.count(full_name))
      record_error(&n, "Duplicate global symbol: " + full_name);
    n.mangled_name = full_name;
    global_scope.define(full_name, &n);
    log_debug("Registered global enum: '{}'", full_name);
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
      n.package_context = current_prefix;
      template_registry[full_name] = &n;
      log_debug("Registered class template blueprint: '{}' (params: {})",
                full_name, n.template_parameters.size());
    }
    return;
  }
  if (current_pass == BinderPass::REGISTER_GLOBALS) {
    std::string full_name = current_prefix + n.class_name;
    n.package_context = current_prefix;
    if (global_scope.symbols.count(full_name))
      record_error(&n, "Duplicate global symbol: " + full_name);
    n.mangled_name = full_name;
    global_scope.define(full_name, &n);
    log_debug("Registered global class: '{}'", full_name);

    bool has_ctor = false;
    for (const auto &child : n.children) {
      if (child && child->node_type == NodeType::CONSTRUCTOR_DECL) {
        has_ctor = true;
        break;
      }
    }
    if (!has_ctor) {
      Token tok{TokenType::IDENTIFIER, n.line, n.column, n.source, n.class_name};
      auto default_ctor = std::make_unique<ConstructorDeclaration>(tok, n.class_name);
      default_ctor->parent = &n;
      auto empty_body = std::make_unique<BlockStatement>(tok);
      empty_body->parent = default_ctor.get();
      default_ctor->children.push_back(std::move(empty_body));
      n.children.push_back(std::move(default_ctor));
    }

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
    n.package_context = current_package;
    global_scope.define(full_name, &n);
    log_trace("Registered class field: '{}'", full_name);
  } else if (current_pass == BinderPass::BIND_EXECUTION) {
    if (n.initializer) {
      TypeInfo init_type = evaluate_expression(n.initializer.get());
      if (!is_assignable(n.type_info, init_type)) {
        record_error(&n, "Type mismatch in field initialization: expected '" +
                              n.type_info.name + "', got '" +
                              init_type.name + "'");
      }
    }
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
    n.package_context = current_package;
    global_scope.define(full_name, &n);
    log_debug("Registered class constructor: '{}'", full_name);
  }
}

void Binder::visit(MethodDeclaration &n) {
  if (!n.template_parameters.empty()) {
    if (current_pass == BinderPass::REGISTER_MEMBERS) {
      std::string full_name = current_prefix + n.method_name;
      n.package_context = current_package;
      template_registry[full_name] = &n;
      log_debug("Registered method template blueprint: '{}' (params: {})",
                full_name, n.template_parameters.size());
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
    n.package_context = current_package;
    global_scope.define(full_name, &n);
    log_debug("Registered method signature: '{}'", full_name);
  }
}


void Binder::visit(TryStatement& n) {
    if (current_pass == BinderPass::REGISTER_GLOBALS) {
        if (n.try_block) register_global_symbols(n.try_block.get(), current_prefix);
        for (auto& c : n.catch_clauses) if (c) register_global_symbols(c.get(), current_prefix);
        if (n.finally_block) register_global_symbols(n.finally_block.get(), current_prefix);
    } else if (current_pass == BinderPass::REGISTER_MEMBERS) {
        if (n.try_block) register_members(n.try_block.get(), current_prefix);
        for (auto& c : n.catch_clauses) if (c) register_members(c.get(), current_prefix);
        if (n.finally_block) register_members(n.finally_block.get(), current_prefix);
    } else if (current_pass == BinderPass::BIND_EXECUTION) {
        if (n.try_block) bind_node(n.try_block.get());
        for (auto& c : n.catch_clauses) if (c) bind_node(c.get());
        if (n.finally_block) bind_node(n.finally_block.get());
    }
}

void Binder::visit(CatchClause& n) {
    if (current_pass == BinderPass::REGISTER_GLOBALS) {
        if (n.body) register_global_symbols(n.body.get(), current_prefix);
    } else if (current_pass == BinderPass::REGISTER_MEMBERS) {
        if (n.body) register_members(n.body.get(), current_prefix);
    } else if (current_pass == BinderPass::BIND_EXECUTION) {
        TypeInfo exc_type = resolve_type(n.exception_type, &n);
        n.exception_type = exc_type;
        
        Node *target_class = global_scope.resolve(n.exception_type.name);
        if (target_class && target_class->node_type == NodeType::CLASS_DECL) {
            n.target_vtable_id = static_cast<ClassDeclaration *>(target_class)->vtable_id;
        } else {
            record_error(&n, "Catch clause exception type '" + exc_type.name + "' is not a class.");
        }
        
        Token dummy_tok;
        dummy_tok.type = TokenType::UNKNOWN_TOKEN;
        n.catch_param_decl = std::make_unique<VariableDeclaration>(dummy_tok, n.variable_name, exc_type);
        auto* decl = static_cast<VariableDeclaration*>(n.catch_param_decl.get());
        decl->is_reference_type = true;
        decl->resolved_declaration = decl;
        decl->memory_index = local_variable_index++;
        n.variable_memory_index = decl->memory_index;
        
        SymbolTable catch_scope;
        enter_scope(&catch_scope);
        declare_local(n.variable_name, decl);
        
        if (n.body) bind_node(n.body.get());
        
        exit_scope();
    }
}

void Binder::visit(ThrowStatement& n) {
    if (current_pass == BinderPass::REGISTER_GLOBALS) {
        if (n.exception_expression) register_global_symbols(n.exception_expression.get(), current_prefix);
    } else if (current_pass == BinderPass::REGISTER_MEMBERS) {
        if (n.exception_expression) register_members(n.exception_expression.get(), current_prefix);
    } else if (current_pass == BinderPass::BIND_EXECUTION) {
        if (n.exception_expression) {
            evaluate_expression(n.exception_expression.get());
        }
    }
}
} // namespace solix
