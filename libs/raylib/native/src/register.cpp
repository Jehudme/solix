#include "solix/native.h"
#include "solix/native_registry.hpp"

void register_raylib_natives(solix::NativeRegistry &registry);

extern "C" SOLIX_EXPORT void solix_register_natives(solix::NativeRegistry &registry) {
    register_raylib_natives(registry);
}
