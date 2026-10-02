#include "solix/native.h"
#include "solix/native_registry.hpp"

void register_console_natives(solix::NativeRegistry &registry);
void register_string_natives(solix::NativeRegistry &registry);
void register_primitives_natives(solix::NativeRegistry &registry);
void register_math_natives(solix::NativeRegistry &registry);

extern "C" SOLIX_EXPORT void solix_register_natives(solix::NativeRegistry &registry) {
    register_console_natives(registry);
    register_string_natives(registry);
    register_primitives_natives(registry);
    register_math_natives(registry);
}


