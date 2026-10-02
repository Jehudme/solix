#ifndef SOLIX_NATIVE_H
#define SOLIX_NATIVE_H

#include <stdint.h>
#include <stddef.h>

#if defined(_WIN32) || defined(__CYGWIN__)
  #define SOLIX_EXPORT __declspec(dllexport)
#elif defined(__GNUC__) || defined(__clang__)
  #define SOLIX_EXPORT __attribute__((visibility("default")))
#else
  #define SOLIX_EXPORT
#endif

#endif // SOLIX_NATIVE_H
