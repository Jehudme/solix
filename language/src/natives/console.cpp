#include "natives/console.hpp"
#include "solix/runtime.hpp"
#include <bit>
#include <cstring>
#include <iostream>
#include <string>
#include <type_traits>

namespace solix {

namespace {

inline void print_char_array(RuntimeContext &runtime_context, Address array_address, bool append_newline) {
  if (array_address == 0) {
    std::cout << "null";
  } else {
    uint32_t character_length = static_cast<uint32_t>(runtime_context.memory.heap[array_address - 1] >> 32);
    for (uint32_t character_index = 0; character_index < character_length; ++character_index) {
      char character_value = static_cast<char>(runtime_context.memory.heap[array_address + character_index]);
      if (character_value == '\0') {
        break;
      }
      std::cout << character_value;
    }
  }
  if (append_newline) {
    std::cout << std::endl;
  }
}

inline void print_solix_string(RuntimeContext &runtime_context, Address string_object_address, bool append_newline) {
  if (string_object_address == 0) {
    std::cout << "null";
  } else {
    // solix.String instance layout:
    // field 1 (offset 1): char[] character_buffer
    // field 2 (offset 2): int32 character_count
    Address buffer_address = static_cast<Address>(runtime_context.memory.heap[string_object_address + 1]);
    if (buffer_address == 0) {
      std::cout << "";
    } else {
      uint32_t character_length = static_cast<uint32_t>(runtime_context.memory.heap[buffer_address - 1] >> 32);
      for (uint32_t character_index = 0; character_index < character_length; ++character_index) {
        char character_value = static_cast<char>(runtime_context.memory.heap[buffer_address + character_index]);
        if (character_value == '\0') {
          break;
        }
        std::cout << character_value;
      }
    }
  }
  if (append_newline) {
    std::cout << std::endl;
  }
}

template <typename PrimitiveType>
inline void print_primitive_value(uint64_t raw_value, bool append_newline) {
  if constexpr (std::is_same_v<PrimitiveType, bool>) {
    std::cout << (raw_value ? "true" : "false");
  } else if constexpr (std::is_same_v<PrimitiveType, char>) {
    std::cout << static_cast<char>(raw_value);
  } else if constexpr (std::is_same_v<PrimitiveType, float>) {
    uint32_t unsigned_32bit_value = static_cast<uint32_t>(raw_value);
    float float_value = std::bit_cast<float>(unsigned_32bit_value);
    std::cout << float_value;
  } else if constexpr (std::is_same_v<PrimitiveType, double>) {
    double double_value = std::bit_cast<double>(raw_value);
    std::cout << double_value;
  } else if constexpr (std::is_signed_v<PrimitiveType>) {
    std::cout << static_cast<int64_t>(static_cast<PrimitiveType>(raw_value));
  } else {
    std::cout << static_cast<uint64_t>(static_cast<PrimitiveType>(raw_value));
  }
  if (append_newline) {
    std::cout << std::endl;
  }
}

template <typename PrimitiveType>
inline void print_primitive_array(RuntimeContext &runtime_context, Address array_address, bool append_newline) {
  if (array_address == 0) {
    std::cout << "null";
  } else {
    uint32_t element_count = static_cast<uint32_t>(runtime_context.memory.heap[array_address - 1] >> 32);
    std::cout << "[";
    for (uint32_t element_index = 0; element_index < element_count; ++element_index) {
      if (element_index > 0) {
        std::cout << ", ";
      }
      uint64_t raw_element_value = runtime_context.memory.heap[array_address + element_index];
      if constexpr (std::is_same_v<PrimitiveType, bool>) {
        std::cout << (raw_element_value ? "true" : "false");
      } else if constexpr (std::is_same_v<PrimitiveType, char>) {
        std::cout << "'" << static_cast<char>(raw_element_value) << "'";
      } else if constexpr (std::is_same_v<PrimitiveType, float>) {
        uint32_t unsigned_32bit_value = static_cast<uint32_t>(raw_element_value);
        float float_value = std::bit_cast<float>(unsigned_32bit_value);
        std::cout << float_value;
      } else if constexpr (std::is_same_v<PrimitiveType, double>) {
        double double_value = std::bit_cast<double>(raw_element_value);
        std::cout << double_value;
      } else if constexpr (std::is_signed_v<PrimitiveType>) {
        std::cout << static_cast<int64_t>(static_cast<PrimitiveType>(raw_element_value));
      } else {
        std::cout << static_cast<uint64_t>(static_cast<PrimitiveType>(raw_element_value));
      }
    }
    std::cout << "]";
  }
  if (append_newline) {
    std::cout << std::endl;
  }
}

template <typename PrimitiveType>
inline NativeFunction make_print_primitive(bool append_newline) {
  return [append_newline](RuntimeContext &runtime_context, uint64_t instance_id, uint64_t *arguments, size_t argument_count) -> uint64_t {
    (void)runtime_context;
    (void)instance_id;
    (void)argument_count;
    print_primitive_value<PrimitiveType>(arguments[0], append_newline);
    return 0;
  };
}

template <typename PrimitiveType>
inline NativeFunction make_print_primitive_array(bool append_newline) {
  return [append_newline](RuntimeContext &runtime_context, uint64_t instance_id, uint64_t *arguments, size_t argument_count) -> uint64_t {
    (void)instance_id;
    (void)argument_count;
    print_primitive_array<PrimitiveType>(runtime_context, static_cast<Address>(arguments[0]), append_newline);
    return 0;
  };
}

} // namespace

void register_console_natives(std::unordered_map<std::string, NativeFunction> &native_registry) {
  // Parameterless println
  native_registry["solix.systems.Console.println()"] = [](RuntimeContext &runtime_context, uint64_t instance_id, uint64_t *arguments, size_t argument_count) -> uint64_t {
    (void)runtime_context;
    (void)instance_id;
    (void)arguments;
    (void)argument_count;
    std::cout << std::endl;
    return 0;
  };

  // Primitives: bool, char, int8..int64, uint8..uint64, float32, float64
  native_registry["solix.systems.Console.print(bool)"] = make_print_primitive<bool>(false);
  native_registry["solix.systems.Console.println(bool)"] = make_print_primitive<bool>(true);
  native_registry["solix.systems.Console.print(char)"] = make_print_primitive<char>(false);
  native_registry["solix.systems.Console.println(char)"] = make_print_primitive<char>(true);

  native_registry["solix.systems.Console.print(int8)"] = make_print_primitive<int8_t>(false);
  native_registry["solix.systems.Console.println(int8)"] = make_print_primitive<int8_t>(true);
  native_registry["solix.systems.Console.print(int16)"] = make_print_primitive<int16_t>(false);
  native_registry["solix.systems.Console.println(int16)"] = make_print_primitive<int16_t>(true);
  native_registry["solix.systems.Console.print(int32)"] = make_print_primitive<int32_t>(false);
  native_registry["solix.systems.Console.println(int32)"] = make_print_primitive<int32_t>(true);
  native_registry["solix.systems.Console.print(int64)"] = make_print_primitive<int64_t>(false);
  native_registry["solix.systems.Console.println(int64)"] = make_print_primitive<int64_t>(true);

  native_registry["solix.systems.Console.print(uint8)"] = make_print_primitive<uint8_t>(false);
  native_registry["solix.systems.Console.println(uint8)"] = make_print_primitive<uint8_t>(true);
  native_registry["solix.systems.Console.print(uint16)"] = make_print_primitive<uint16_t>(false);
  native_registry["solix.systems.Console.println(uint16)"] = make_print_primitive<uint16_t>(true);
  native_registry["solix.systems.Console.print(uint32)"] = make_print_primitive<uint32_t>(false);
  native_registry["solix.systems.Console.println(uint32)"] = make_print_primitive<uint32_t>(true);
  native_registry["solix.systems.Console.print(uint64)"] = make_print_primitive<uint64_t>(false);
  native_registry["solix.systems.Console.println(uint64)"] = make_print_primitive<uint64_t>(true);

  native_registry["solix.systems.Console.print(float32)"] = make_print_primitive<float>(false);
  native_registry["solix.systems.Console.println(float32)"] = make_print_primitive<float>(true);
  native_registry["solix.systems.Console.print(float64)"] = make_print_primitive<double>(false);
  native_registry["solix.systems.Console.println(float64)"] = make_print_primitive<double>(true);

  // char[] is printed as text
  native_registry["solix.systems.Console.print(char[])"] = [](RuntimeContext &runtime_context, uint64_t instance_id, uint64_t *arguments, size_t argument_count) -> uint64_t {
    (void)instance_id;
    (void)argument_count;
    print_char_array(runtime_context, static_cast<Address>(arguments[0]), false);
    return 0;
  };
  native_registry["solix.systems.Console.println(char[])"] = [](RuntimeContext &runtime_context, uint64_t instance_id, uint64_t *arguments, size_t argument_count) -> uint64_t {
    (void)instance_id;
    (void)argument_count;
    print_char_array(runtime_context, static_cast<Address>(arguments[0]), true);
    return 0;
  };

  // Other primitive arrays are formatted as [elem1, elem2, ...]
  native_registry["solix.systems.Console.print(bool[])"] = make_print_primitive_array<bool>(false);
  native_registry["solix.systems.Console.println(bool[])"] = make_print_primitive_array<bool>(true);

  native_registry["solix.systems.Console.print(int8[])"] = make_print_primitive_array<int8_t>(false);
  native_registry["solix.systems.Console.println(int8[])"] = make_print_primitive_array<int8_t>(true);
  native_registry["solix.systems.Console.print(int16[])"] = make_print_primitive_array<int16_t>(false);
  native_registry["solix.systems.Console.println(int16[])"] = make_print_primitive_array<int16_t>(true);
  native_registry["solix.systems.Console.print(int32[])"] = make_print_primitive_array<int32_t>(false);
  native_registry["solix.systems.Console.println(int32[])"] = make_print_primitive_array<int32_t>(true);
  native_registry["solix.systems.Console.print(int64[])"] = make_print_primitive_array<int64_t>(false);
  native_registry["solix.systems.Console.println(int64[])"] = make_print_primitive_array<int64_t>(true);

  native_registry["solix.systems.Console.print(uint8[])"] = make_print_primitive_array<uint8_t>(false);
  native_registry["solix.systems.Console.println(uint8[])"] = make_print_primitive_array<uint8_t>(true);
  native_registry["solix.systems.Console.print(uint16[])"] = make_print_primitive_array<uint16_t>(false);
  native_registry["solix.systems.Console.println(uint16[])"] = make_print_primitive_array<uint16_t>(true);
  native_registry["solix.systems.Console.print(uint32[])"] = make_print_primitive_array<uint32_t>(false);
  native_registry["solix.systems.Console.println(uint32[])"] = make_print_primitive_array<uint32_t>(true);
  native_registry["solix.systems.Console.print(uint64[])"] = make_print_primitive_array<uint64_t>(false);
  native_registry["solix.systems.Console.println(uint64[])"] = make_print_primitive_array<uint64_t>(true);

  native_registry["solix.systems.Console.print(float32[])"] = make_print_primitive_array<float>(false);
  native_registry["solix.systems.Console.println(float32[])"] = make_print_primitive_array<float>(true);
  native_registry["solix.systems.Console.print(float64[])"] = make_print_primitive_array<double>(false);
  native_registry["solix.systems.Console.println(float64[])"] = make_print_primitive_array<double>(true);

  // solix.String
  native_registry["solix.systems.Console.print(solix.String)"] = [](RuntimeContext &runtime_context, uint64_t instance_id, uint64_t *arguments, size_t argument_count) -> uint64_t {
    (void)instance_id;
    (void)argument_count;
    print_solix_string(runtime_context, static_cast<Address>(arguments[0]), false);
    return 0;
  };
  native_registry["solix.systems.Console.println(solix.String)"] = [](RuntimeContext &runtime_context, uint64_t instance_id, uint64_t *arguments, size_t argument_count) -> uint64_t {
    (void)instance_id;
    (void)argument_count;
    print_solix_string(runtime_context, static_cast<Address>(arguments[0]), true);
    return 0;
  };
}

} // namespace solix
