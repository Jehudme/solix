#include "processes/binder.hpp"
#include <functional>
#include <algorithm>

namespace solix {

bool Binder::deduce_template_arguments(const std::vector<TypeInfo>& param_types,
                                       const std::vector<TypeInfo>& arg_types,
                                       const std::vector<std::string>& tparams,
                                       std::vector<TypeInfo>& deduced_args) {
    bool is_variadic = (!tparams.empty() && tparams.back().length() > 3 && tparams.back().substr(tparams.back().length() - 3) == "...") ||
                       (!param_types.empty() && (param_types.back().is_variadic_pack || (param_types.back().name == "tuple" && !tparams.empty() && tparams.back().find("...") != std::string::npos)));
    
    if (!is_variadic) {
        if (param_types.size() != arg_types.size()) return false;
    } else {
        if (arg_types.size() + 1 < param_types.size()) return false;
    }
    
    std::unordered_map<std::string, TypeInfo> deductions;
    
    std::function<bool(const TypeInfo&, const TypeInfo&)> deduce = [&](const TypeInfo& param, const TypeInfo& arg) -> bool {
        std::string p_name = param.name;
        bool matches_tparam = (std::find(tparams.begin(), tparams.end(), p_name) != tparams.end());
        if (!matches_tparam && (std::find(tparams.begin(), tparams.end(), p_name + "...") != tparams.end())) {
            matches_tparam = true;
            p_name = p_name + "...";
        }
        if (matches_tparam) {
            if (param.array_depth > arg.array_depth) return false;
            TypeInfo deduced_t = arg;
            deduced_t.array_depth -= param.array_depth;
            
            if (deductions.count(p_name)) {
                if (deductions[p_name] != deduced_t) return false;
            } else {
                deductions[p_name] = deduced_t;
            }
            return true;
        }
        
        // Check if the argument is an instantiated generic (e.g. com.solix.Box<int32>)
        if (!param.type_args.empty()) {
            std::string expected_prefix = param.name + "<";
            if (arg.name.find(expected_prefix) != std::string::npos || arg.name.find("." + expected_prefix) != std::string::npos) {
                // We must extract the type args from the mangled name!
                size_t start = arg.name.find("<") + 1;
                size_t end = arg.name.rfind(">");
                std::string generic_content = arg.name.substr(start, end - start);
                
                // Depth-aware split by comma (respecting nested template <...>)
                std::vector<std::string> extracted_args;
                std::string current_arg;
                int depth = 0;
                for (char ch : generic_content) {
                    if (ch == '<') {
                        depth++;
                        current_arg += ch;
                    } else if (ch == '>') {
                        depth--;
                        current_arg += ch;
                    } else if (ch == ',' && depth == 0) {
                        extracted_args.push_back(current_arg);
                        current_arg.clear();
                    } else {
                        current_arg += ch;
                    }
                }
                if (!current_arg.empty()) {
                    extracted_args.push_back(current_arg);
                }
                
                if (param.type_args.size() == extracted_args.size() && param.array_depth == arg.array_depth) {
                    for (size_t i = 0; i < param.type_args.size(); ++i) {
                        TypeInfo arg_inner;
                        arg_inner.name = extracted_args[i];
                        if (!deduce(param.type_args[i], arg_inner)) return false;
                    }
                    return true;
                }
            }
        }
        
        if (!param.type_args.empty() && param.name == arg.name && param.array_depth == arg.array_depth && param.type_args.size() == arg.type_args.size()) {
            for (size_t i = 0; i < param.type_args.size(); ++i) {
                if (!deduce(param.type_args[i], arg.type_args[i])) return false;
            }
            return true;
        }
        
        return true;
    };
    
    size_t fixed_count = is_variadic ? (param_types.size() - 1) : param_types.size();
    for (size_t i = 0; i < fixed_count; ++i) {
        if (!deduce(param_types[i], arg_types[i])) return false;
    }
    
    if (is_variadic) {
        std::string var_param_name = tparams.back();
        TypeInfo tuple_pack("tuple", 0);
        tuple_pack.is_tuple = true;
        for (size_t i = fixed_count; i < arg_types.size(); ++i) {
            tuple_pack.type_args.push_back(arg_types[i]);
        }
        deductions[var_param_name] = tuple_pack;
        if (var_param_name.length() > 3 && var_param_name.substr(var_param_name.length() - 3) == "...") {
            deductions[var_param_name.substr(0, var_param_name.length() - 3)] = tuple_pack;
        }
    }
    
    for (const auto& tparam : tparams) {
        if (deductions.count(tparam)) {
            deduced_args.push_back(deductions[tparam]);
        } else {
            std::string plain = tparam;
            if (plain.length() > 3 && plain.substr(plain.length() - 3) == "...") {
                plain = plain.substr(0, plain.length() - 3);
            }
            if (deductions.count(plain)) {
                deduced_args.push_back(deductions[plain]);
            } else {
                return false;
            }
        }
    }
    
    return true;
}

} // namespace solix
