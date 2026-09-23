#include "solix/runtime.hpp"
#include <bit>
#include <cstring>
#include <iostream>
#include <string>
#include <type_traits>
#include <unordered_map>

namespace solix {

namespace {

inline void print_char_array(RuntimeContext &ctx, Address addr, bool newline) {
  if (addr == 0) {
    std::cout << "null";
  } else {
    uint32_t len = static_cast<uint32_t>(ctx.memory.heap[addr - 1] >> 32);
    for (uint32_t i = 0; i < len; ++i) {
      char c = static_cast<char>(ctx.memory.heap[addr + i]);
      if (c == '\0') break;
      std::cout << c;
    }
  }
  if (newline) {
    std::cout << std::endl;
  }
}

inline void print_solix_string(RuntimeContext &ctx, Address str_obj_addr, bool newline) {
  if (str_obj_addr == 0) {
    std::cout << "null";
  } else {
    // solix.String instance layout:
    // field 1 (offset 1): char[] character_buffer
    // field 2 (offset 2): int32 character_count
    Address buf_addr = static_cast<Address>(ctx.memory.heap[str_obj_addr + 1]);
    if (buf_addr == 0) {
      std::cout << "";
    } else {
      uint32_t len = static_cast<uint32_t>(ctx.memory.heap[buf_addr - 1] >> 32);
      for (uint32_t i = 0; i < len; ++i) {
        char c = static_cast<char>(ctx.memory.heap[buf_addr + i]);
        if (c == '\0') break;
        std::cout << c;
      }
    }
  }
  if (newline) {
    std::cout << std::endl;
  }
}

template <typename T>
inline void print_primitive_value(uint64_t raw_val, bool newline) {
  if constexpr (std::is_same_v<T, bool>) {
    std::cout << (raw_val ? "true" : "false");
  } else if constexpr (std::is_same_v<T, char>) {
    std::cout << static_cast<char>(raw_val);
  } else if constexpr (std::is_same_v<T, float>) {
    uint32_t u32 = static_cast<uint32_t>(raw_val);
    float f = std::bit_cast<float>(u32);
    std::cout << f;
  } else if constexpr (std::is_same_v<T, double>) {
    double d = std::bit_cast<double>(raw_val);
    std::cout << d;
  } else if constexpr (std::is_signed_v<T>) {
    std::cout << static_cast<int64_t>(static_cast<T>(raw_val));
  } else {
    std::cout << static_cast<uint64_t>(static_cast<T>(raw_val));
  }
  if (newline) {
    std::cout << std::endl;
  }
}

template <typename T>
inline void print_primitive_array(RuntimeContext &ctx, Address addr, bool newline) {
  if (addr == 0) {
    std::cout << "null";
  } else {
    uint32_t len = static_cast<uint32_t>(ctx.memory.heap[addr - 1] >> 32);
    std::cout << "[";
    for (uint32_t i = 0; i < len; ++i) {
      if (i > 0) {
        std::cout << ", ";
      }
      uint64_t raw = ctx.memory.heap[addr + i];
      if constexpr (std::is_same_v<T, bool>) {
        std::cout << (raw ? "true" : "false");
      } else if constexpr (std::is_same_v<T, char>) {
        std::cout << "'" << static_cast<char>(raw) << "'";
      } else if constexpr (std::is_same_v<T, float>) {
        uint32_t u32 = static_cast<uint32_t>(raw);
        float f = std::bit_cast<float>(u32);
        std::cout << f;
      } else if constexpr (std::is_same_v<T, double>) {
        double d = std::bit_cast<double>(raw);
        std::cout << d;
      } else if constexpr (std::is_signed_v<T>) {
        std::cout << static_cast<int64_t>(static_cast<T>(raw));
      } else {
        std::cout << static_cast<uint64_t>(static_cast<T>(raw));
      }
    }
    std::cout << "]";
  }
  if (newline) {
    std::cout << std::endl;
  }
}

template <typename T>
inline NativeFunction make_print_prim(bool newline) {
  return [newline](RuntimeContext &, uint64_t, uint64_t *args, size_t) -> uint64_t {
    print_primitive_value<T>(args[0], newline);
    return 0;
  };
}

template <typename T>
inline NativeFunction make_print_arr(bool newline) {
  return [newline](RuntimeContext &ctx, uint64_t, uint64_t *args, size_t) -> uint64_t {
    print_primitive_array<T>(ctx, static_cast<Address>(args[0]), newline);
    return 0;
  };
}

} // namespace

const std::unordered_map<std::string, NativeFunction> &get_builtin_natives() {
  static const std::unordered_map<std::string, NativeFunction> builtin_natives = []() {
    std::unordered_map<std::string, NativeFunction> map;

    // Parameterless println
    auto fn_println_empty = [](RuntimeContext &, uint64_t, uint64_t *, size_t) -> uint64_t {
      std::cout << std::endl;
      return 0;
    };
    map["solix.systems.Console.println()"] = fn_println_empty;

    // Primitives: bool, char, int8..int64, uint8..uint64, float32, float64
    map["solix.systems.Console.print(bool)"] = make_print_prim<bool>(false);
    map["solix.systems.Console.println(bool)"] = make_print_prim<bool>(true);
    map["solix.systems.Console.print(char)"] = make_print_prim<char>(false);
    map["solix.systems.Console.println(char)"] = make_print_prim<char>(true);

    map["solix.systems.Console.print(int8)"] = make_print_prim<int8_t>(false);
    map["solix.systems.Console.println(int8)"] = make_print_prim<int8_t>(true);
    map["solix.systems.Console.print(int16)"] = make_print_prim<int16_t>(false);
    map["solix.systems.Console.println(int16)"] = make_print_prim<int16_t>(true);
    map["solix.systems.Console.print(int32)"] = make_print_prim<int32_t>(false);
    map["solix.systems.Console.println(int32)"] = make_print_prim<int32_t>(true);
    map["solix.systems.Console.print(int64)"] = make_print_prim<int64_t>(false);
    map["solix.systems.Console.println(int64)"] = make_print_prim<int64_t>(true);

    map["solix.systems.Console.print(uint8)"] = make_print_prim<uint8_t>(false);
    map["solix.systems.Console.println(uint8)"] = make_print_prim<uint8_t>(true);
    map["solix.systems.Console.print(uint16)"] = make_print_prim<uint16_t>(false);
    map["solix.systems.Console.println(uint16)"] = make_print_prim<uint16_t>(true);
    map["solix.systems.Console.print(uint32)"] = make_print_prim<uint32_t>(false);
    map["solix.systems.Console.println(uint32)"] = make_print_prim<uint32_t>(true);
    map["solix.systems.Console.print(uint64)"] = make_print_prim<uint64_t>(false);
    map["solix.systems.Console.println(uint64)"] = make_print_prim<uint64_t>(true);

    map["solix.systems.Console.print(float32)"] = make_print_prim<float>(false);
    map["solix.systems.Console.println(float32)"] = make_print_prim<float>(true);
    map["solix.systems.Console.print(float64)"] = make_print_prim<double>(false);
    map["solix.systems.Console.println(float64)"] = make_print_prim<double>(true);

    // char[] is printed as text
    map["solix.systems.Console.print(char[])"] = [](RuntimeContext &ctx, uint64_t, uint64_t *args, size_t) -> uint64_t {
      print_char_array(ctx, static_cast<Address>(args[0]), false);
      return 0;
    };
    map["solix.systems.Console.println(char[])"] = [](RuntimeContext &ctx, uint64_t, uint64_t *args, size_t) -> uint64_t {
      print_char_array(ctx, static_cast<Address>(args[0]), true);
      return 0;
    };

    // Other primitive arrays are formatted as [x, y, z]
    map["solix.systems.Console.print(bool[])"] = make_print_arr<bool>(false);
    map["solix.systems.Console.println(bool[])"] = make_print_arr<bool>(true);

    map["solix.systems.Console.print(int8[])"] = make_print_arr<int8_t>(false);
    map["solix.systems.Console.println(int8[])"] = make_print_arr<int8_t>(true);
    map["solix.systems.Console.print(int16[])"] = make_print_arr<int16_t>(false);
    map["solix.systems.Console.println(int16[])"] = make_print_arr<int16_t>(true);
    map["solix.systems.Console.print(int32[])"] = make_print_arr<int32_t>(false);
    map["solix.systems.Console.println(int32[])"] = make_print_arr<int32_t>(true);
    map["solix.systems.Console.print(int64[])"] = make_print_arr<int64_t>(false);
    map["solix.systems.Console.println(int64[])"] = make_print_arr<int64_t>(true);

    map["solix.systems.Console.print(uint8[])"] = make_print_arr<uint8_t>(false);
    map["solix.systems.Console.println(uint8[])"] = make_print_arr<uint8_t>(true);
    map["solix.systems.Console.print(uint16[])"] = make_print_arr<uint16_t>(false);
    map["solix.systems.Console.println(uint16[])"] = make_print_arr<uint16_t>(true);
    map["solix.systems.Console.print(uint32[])"] = make_print_arr<uint32_t>(false);
    map["solix.systems.Console.println(uint32[])"] = make_print_arr<uint32_t>(true);
    map["solix.systems.Console.print(uint64[])"] = make_print_arr<uint64_t>(false);
    map["solix.systems.Console.println(uint64[])"] = make_print_arr<uint64_t>(true);

    map["solix.systems.Console.print(float32[])"] = make_print_arr<float>(false);
    map["solix.systems.Console.println(float32[])"] = make_print_arr<float>(true);
    map["solix.systems.Console.print(float64[])"] = make_print_arr<double>(false);
    map["solix.systems.Console.println(float64[])"] = make_print_arr<double>(true);

    // solix.String
    map["solix.systems.Console.print(solix.String)"] = [](RuntimeContext &ctx, uint64_t, uint64_t *args, size_t) -> uint64_t {
      print_solix_string(ctx, static_cast<Address>(args[0]), false);
      return 0;
    };
    map["solix.systems.Console.println(solix.String)"] = [](RuntimeContext &ctx, uint64_t, uint64_t *args, size_t) -> uint64_t {
      print_solix_string(ctx, static_cast<Address>(args[0]), true);
      return 0;
    };

    return map;
  }();

  return builtin_natives;
}

} // namespace solix
