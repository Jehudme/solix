#pragma once

#include "solix/runtime.hpp"
#include "solix/shared_library.hpp"
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace solix {

class NativeRegistry;

// Hook signature exported by native plugins:
// extern "C" SOLIX_EXPORT void solix_register_natives(solix::NativeRegistry &registry);
typedef void (*NativeRegisterHook)(NativeRegistry &registry);

class NativeRegistry {
public:
  static NativeRegistry &global();

  NativeRegistry() = default;
  ~NativeRegistry() = default;

  NativeRegistry(const NativeRegistry &) = delete;
  NativeRegistry &operator=(const NativeRegistry &) = delete;

  inline void register_function(const std::string &name, NativeFunctionPtr func) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    functions_[name] = func;
  }
  NativeFunctionPtr find_function(const std::string &name) const;

  // Loads a shared library, keeping its handle resident in memory.
  // If the library exports 'solix_register_natives', it is automatically called.
  bool load_library(const std::filesystem::path &path);

  // Searches loaded libraries for an exported C symbol matching the given name.
  NativeFunctionPtr find_symbol_in_loaded_libraries(const std::string &symbol_name) const;

  void clear();

private:
  mutable std::recursive_mutex mutex_;
  mutable std::unordered_map<std::string, NativeFunctionPtr> functions_;
  std::vector<std::unique_ptr<SharedLibrary>> loaded_libraries_;
};

} // namespace solix
