#include "solix/native_registry.hpp"
#include <iostream>

namespace solix {

NativeRegistry &NativeRegistry::global() {
  static NativeRegistry instance;
  return instance;
}


NativeFunctionPtr NativeRegistry::find_function(const std::string &name) const {
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  auto it = functions_.find(name);
  if (it != functions_.end()) {
    return it->second;
  }
  return nullptr;
}

bool NativeRegistry::load_library(const std::filesystem::path &path) {
  std::lock_guard<std::recursive_mutex> lock(mutex_);

  std::error_code ec;
  for (const auto &lib : loaded_libraries_) {
    if (lib->path() == path || (std::filesystem::exists(path, ec) && std::filesystem::exists(lib->path(), ec) && std::filesystem::equivalent(lib->path(), path, ec))) {
      return true;
    }
  }

  auto library = std::make_unique<SharedLibrary>(path);

  // Check for batch registration hook: solix_register_natives
  auto hook = library->get_symbol<NativeRegisterHook>("solix_register_natives");
  if (hook) {
    hook(*this);
  }

  loaded_libraries_.push_back(std::move(library));
  return true;
}

NativeFunctionPtr NativeRegistry::find_symbol_in_loaded_libraries(const std::string &symbol_name) const {
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  for (const auto &lib : loaded_libraries_) {
    void *sym = lib->get_symbol(symbol_name);
    if (sym) {
      NativeFunctionPtr func = reinterpret_cast<NativeFunctionPtr>(sym);
      functions_[symbol_name] = func;
      return func;
    }
  }
  return nullptr;
}

void NativeRegistry::clear() {
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  functions_.clear();
  loaded_libraries_.clear();
}

} // namespace solix
