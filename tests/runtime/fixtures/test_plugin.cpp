#include "solix/native.h"
#include "solix/runtime.hpp"
#include "solix/native_registry.hpp"

static uint64_t native_math_add(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
  (void)vm;
  (void)self;
  (void)argc;
  return args[0] + args[1];
}

static uint64_t counter_increment(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
  (void)args;
  (void)argc;
  uint64_t current = vm.memory.read_u64(static_cast<solix::Address>(self), 1);
  current += 1;
  vm.memory.write_u64(static_cast<solix::Address>(self), 1, current);
  return current;
}

static uint64_t batch_multiply(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
  (void)vm;
  (void)self;
  (void)argc;
  return args[0] * args[1];
}

static uint64_t batch_subtract(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
  (void)vm;
  (void)self;
  (void)argc;
  return args[0] - args[1];
}

static uint64_t lib_a_func_a(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
  (void)vm;
  (void)self;
  (void)args;
  (void)argc;
  return 42;
}

extern "C" SOLIX_EXPORT void solix_register_natives(solix::NativeRegistry &registry) {
  registry.register_function("NativeMath_add", native_math_add);
  registry.register_function("Counter_increment", counter_increment);
  registry.register_function("BatchPlugin_multiply", batch_multiply);
  registry.register_function("BatchPlugin_subtract", batch_subtract);
  registry.register_function("LibA_funcA", lib_a_func_a);
}

extern "C" SOLIX_EXPORT uint64_t direct_export(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
  (void)vm;
  (void)self;
  (void)argc;
  return args[0] + 1;
}
