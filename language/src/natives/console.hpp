#pragma once

#include "solix/runtime.hpp"
#include <string>
#include <unordered_map>

namespace solix {

void register_console_natives(std::unordered_map<std::string, NativeFunction> &native_registry);

} // namespace solix
