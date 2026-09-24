#include "solix/runtime.hpp"
#include "natives/console.hpp"
#include "natives/utilities.hpp"

namespace solix {

const std::unordered_map<std::string, NativeFunction> &get_builtin_natives() {
  static const std::unordered_map<std::string, NativeFunction> builtin_natives = []() {
    std::unordered_map<std::string, NativeFunction> native_registry;
    register_console_natives(native_registry);
    register_utilities_natives(native_registry);
    return native_registry;
  }();

  return builtin_natives;
}

} // namespace solix
