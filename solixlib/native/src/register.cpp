#include "solix/native.h"
#include "solix/native_registry.hpp"

void register_console_natives(solix::NativeRegistry &registry);
void register_string_natives(solix::NativeRegistry &registry);
void register_primitives_natives(solix::NativeRegistry &registry);
void register_math_natives(solix::NativeRegistry &registry);
void register_time_natives(solix::NativeRegistry &registry);
void register_io_fs_natives(solix::NativeRegistry &registry);
void register_io_stream_natives(solix::NativeRegistry &registry);
void register_system_natives(solix::NativeRegistry &registry);
void register_crypto_natives(solix::NativeRegistry &registry);

extern "C" SOLIX_EXPORT void solix_register_natives(solix::NativeRegistry &registry) {
    register_console_natives(registry);
    register_string_natives(registry);
    register_primitives_natives(registry);
    register_math_natives(registry);
    register_time_natives(registry);
    register_io_fs_natives(registry);
    register_io_stream_natives(registry);
    register_system_natives(registry);
    register_crypto_natives(registry);
}



