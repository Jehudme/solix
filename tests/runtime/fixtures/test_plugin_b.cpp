#include "solix/native.h"
#include "solix/runtime.hpp"
#include "solix/native_registry.hpp"

static uint64_t lib_b_func_b(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
  (void)vm;
  (void)self;
  (void)args;
  (void)argc;
  return 58;
}

extern "C" SOLIX_EXPORT void solix_register_natives(solix::NativeRegistry &registry) {
  registry.register_function("LibB_funcB", lib_b_func_b);
}
