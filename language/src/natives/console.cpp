#include "natives/console.hpp"
#include "solix/runtime.hpp"
#include <bit>
#include <cstring>
#include <iostream>
#include <string>
#include <type_traits>

namespace solix {

namespace {

inline void print_char_array_to_stream(
    RuntimeContext &runtime_context,
    std::ostream &output_stream,
    Address array_address,
    bool append_newline,
    const char *prefix_label = nullptr) {
  if (prefix_label != nullptr) {
    output_stream << prefix_label;
  }
  if (array_address == 0) {
    output_stream << "null";
  } else {
    uint32_t character_length = static_cast<uint32_t>(runtime_context.memory.heap[array_address - 1] >> 32);
    for (uint32_t character_index = 0; character_index < character_length; ++character_index) {
      char character_value = static_cast<char>(runtime_context.memory.heap[array_address + character_index]);
      if (character_value == '\0') {
        break;
      }
      output_stream << character_value;
    }
  }
  if (append_newline) {
    output_stream << std::endl;
  }
}

inline void print_solix_string_to_stream(
    RuntimeContext &runtime_context,
    std::ostream &output_stream,
    Address string_object_address,
    bool append_newline,
    const char *prefix_label = nullptr) {
  if (prefix_label != nullptr) {
    output_stream << prefix_label;
  }
  if (string_object_address == 0) {
    output_stream << "null";
  } else {
    // solix.String instance layout:
    // field 1 (offset 1): char[] character_buffer
    // field 2 (offset 2): int32 character_count
    Address buffer_address = static_cast<Address>(runtime_context.memory.heap[string_object_address + 1]);
    if (buffer_address == 0) {
      output_stream << "";
    } else {
      uint32_t character_length = static_cast<uint32_t>(runtime_context.memory.heap[buffer_address - 1] >> 32);
      for (uint32_t character_index = 0; character_index < character_length; ++character_index) {
        char character_value = static_cast<char>(runtime_context.memory.heap[buffer_address + character_index]);
        if (character_value == '\0') {
          break;
        }
        output_stream << character_value;
      }
    }
  }
  if (append_newline) {
    output_stream << std::endl;
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

inline const char *get_ansi_foreground_color(uint64_t color_index) {
  switch (color_index) {
    case 0: return "\033[39m"; // DEFAULT
    case 1: return "\033[30m"; // BLACK
    case 2: return "\033[31m"; // RED
    case 3: return "\033[32m"; // GREEN
    case 4: return "\033[33m"; // YELLOW
    case 5: return "\033[34m"; // BLUE
    case 6: return "\033[35m"; // MAGENTA
    case 7: return "\033[36m"; // CYAN
    case 8: return "\033[37m"; // WHITE
    default: return "\033[39m";
  }
}

inline const char *get_ansi_background_color(uint64_t color_index) {
  switch (color_index) {
    case 0: return "\033[49m"; // DEFAULT
    case 1: return "\033[40m"; // BLACK
    case 2: return "\033[41m"; // RED
    case 3: return "\033[42m"; // GREEN
    case 4: return "\033[43m"; // YELLOW
    case 5: return "\033[44m"; // BLUE
    case 6: return "\033[45m"; // MAGENTA
    case 7: return "\033[46m"; // CYAN
    case 8: return "\033[47m"; // WHITE
    default: return "\033[49m";
  }
}

} // namespace

void register_console_natives(std::unordered_map<std::string, NativeFunction> &native_registry) {
  // ---------------------------------------------------------------------------
  // Standard Output: Println Blank Line
  // ---------------------------------------------------------------------------
  native_registry["solix.systems.Console.println()"] = [](RuntimeContext &runtime_context, uint64_t instance_id, uint64_t *arguments, size_t argument_count) -> uint64_t {
    (void)runtime_context;
    (void)instance_id;
    (void)arguments;
    (void)argument_count;
    std::cout << std::endl;
    return 0;
  };

  // ---------------------------------------------------------------------------
  // Standard Output: Primitives Print & Println
  // ---------------------------------------------------------------------------
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

  // ---------------------------------------------------------------------------
  // Standard Output: char[] Array (Text)
  // ---------------------------------------------------------------------------
  native_registry["solix.systems.Console.print(char[])"] = [](RuntimeContext &runtime_context, uint64_t instance_id, uint64_t *arguments, size_t argument_count) -> uint64_t {
    (void)instance_id;
    (void)argument_count;
    print_char_array_to_stream(runtime_context, std::cout, static_cast<Address>(arguments[0]), false);
    return 0;
  };
  native_registry["solix.systems.Console.println(char[])"] = [](RuntimeContext &runtime_context, uint64_t instance_id, uint64_t *arguments, size_t argument_count) -> uint64_t {
    (void)instance_id;
    (void)argument_count;
    print_char_array_to_stream(runtime_context, std::cout, static_cast<Address>(arguments[0]), true);
    return 0;
  };

  // ---------------------------------------------------------------------------
  // Standard Output: Other Primitive Arrays Formatted as [elem1, elem2, ...]
  // ---------------------------------------------------------------------------
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

  // ---------------------------------------------------------------------------
  // Standard Output: Solix String
  // ---------------------------------------------------------------------------
  native_registry["solix.systems.Console.print(solix.String)"] = [](RuntimeContext &runtime_context, uint64_t instance_id, uint64_t *arguments, size_t argument_count) -> uint64_t {
    (void)instance_id;
    (void)argument_count;
    print_solix_string_to_stream(runtime_context, std::cout, static_cast<Address>(arguments[0]), false);
    return 0;
  };
  native_registry["solix.systems.Console.println(solix.String)"] = [](RuntimeContext &runtime_context, uint64_t instance_id, uint64_t *arguments, size_t argument_count) -> uint64_t {
    (void)instance_id;
    (void)argument_count;
    print_solix_string_to_stream(runtime_context, std::cout, static_cast<Address>(arguments[0]), true);
    return 0;
  };

  // ---------------------------------------------------------------------------
  // Error Stream (stderr)
  // ---------------------------------------------------------------------------
  native_registry["solix.systems.Console.error(char[])"] = [](RuntimeContext &runtime_context, uint64_t instance_id, uint64_t *arguments, size_t argument_count) -> uint64_t {
    (void)instance_id;
    (void)argument_count;
    print_char_array_to_stream(runtime_context, std::cerr, static_cast<Address>(arguments[0]), false);
    return 0;
  };
  native_registry["solix.systems.Console.error(solix.String)"] = [](RuntimeContext &runtime_context, uint64_t instance_id, uint64_t *arguments, size_t argument_count) -> uint64_t {
    (void)instance_id;
    (void)argument_count;
    print_solix_string_to_stream(runtime_context, std::cerr, static_cast<Address>(arguments[0]), false);
    return 0;
  };
  native_registry["solix.systems.Console.errorln(char[])"] = [](RuntimeContext &runtime_context, uint64_t instance_id, uint64_t *arguments, size_t argument_count) -> uint64_t {
    (void)instance_id;
    (void)argument_count;
    print_char_array_to_stream(runtime_context, std::cerr, static_cast<Address>(arguments[0]), true);
    return 0;
  };
  native_registry["solix.systems.Console.errorln(solix.String)"] = [](RuntimeContext &runtime_context, uint64_t instance_id, uint64_t *arguments, size_t argument_count) -> uint64_t {
    (void)instance_id;
    (void)argument_count;
    print_solix_string_to_stream(runtime_context, std::cerr, static_cast<Address>(arguments[0]), true);
    return 0;
  };
  native_registry["solix.systems.Console.errorln()"] = [](RuntimeContext &runtime_context, uint64_t instance_id, uint64_t *arguments, size_t argument_count) -> uint64_t {
    (void)runtime_context;
    (void)instance_id;
    (void)arguments;
    (void)argument_count;
    std::cerr << std::endl;
    return 0;
  };

  // ---------------------------------------------------------------------------
  // Warning Stream
  // ---------------------------------------------------------------------------
  native_registry["solix.systems.Console.warn(char[])"] = [](RuntimeContext &runtime_context, uint64_t instance_id, uint64_t *arguments, size_t argument_count) -> uint64_t {
    (void)instance_id;
    (void)argument_count;
    print_char_array_to_stream(runtime_context, std::cerr, static_cast<Address>(arguments[0]), false, "[WARN] ");
    return 0;
  };
  native_registry["solix.systems.Console.warn(solix.String)"] = [](RuntimeContext &runtime_context, uint64_t instance_id, uint64_t *arguments, size_t argument_count) -> uint64_t {
    (void)instance_id;
    (void)argument_count;
    print_solix_string_to_stream(runtime_context, std::cerr, static_cast<Address>(arguments[0]), false, "[WARN] ");
    return 0;
  };
  native_registry["solix.systems.Console.warnln(char[])"] = [](RuntimeContext &runtime_context, uint64_t instance_id, uint64_t *arguments, size_t argument_count) -> uint64_t {
    (void)instance_id;
    (void)argument_count;
    print_char_array_to_stream(runtime_context, std::cerr, static_cast<Address>(arguments[0]), true, "[WARN] ");
    return 0;
  };
  native_registry["solix.systems.Console.warnln(solix.String)"] = [](RuntimeContext &runtime_context, uint64_t instance_id, uint64_t *arguments, size_t argument_count) -> uint64_t {
    (void)instance_id;
    (void)argument_count;
    print_solix_string_to_stream(runtime_context, std::cerr, static_cast<Address>(arguments[0]), true, "[WARN] ");
    return 0;
  };

  // ---------------------------------------------------------------------------
  // Info & Debug Streams
  // ---------------------------------------------------------------------------
  native_registry["solix.systems.Console.info(char[])"] = [](RuntimeContext &runtime_context, uint64_t instance_id, uint64_t *arguments, size_t argument_count) -> uint64_t {
    (void)instance_id;
    (void)argument_count;
    print_char_array_to_stream(runtime_context, std::cout, static_cast<Address>(arguments[0]), false, "[INFO] ");
    return 0;
  };
  native_registry["solix.systems.Console.info(solix.String)"] = [](RuntimeContext &runtime_context, uint64_t instance_id, uint64_t *arguments, size_t argument_count) -> uint64_t {
    (void)instance_id;
    (void)argument_count;
    print_solix_string_to_stream(runtime_context, std::cout, static_cast<Address>(arguments[0]), false, "[INFO] ");
    return 0;
  };
  native_registry["solix.systems.Console.infoln(char[])"] = [](RuntimeContext &runtime_context, uint64_t instance_id, uint64_t *arguments, size_t argument_count) -> uint64_t {
    (void)instance_id;
    (void)argument_count;
    print_char_array_to_stream(runtime_context, std::cout, static_cast<Address>(arguments[0]), true, "[INFO] ");
    return 0;
  };
  native_registry["solix.systems.Console.infoln(solix.String)"] = [](RuntimeContext &runtime_context, uint64_t instance_id, uint64_t *arguments, size_t argument_count) -> uint64_t {
    (void)instance_id;
    (void)argument_count;
    print_solix_string_to_stream(runtime_context, std::cout, static_cast<Address>(arguments[0]), true, "[INFO] ");
    return 0;
  };

  native_registry["solix.systems.Console.debug(char[])"] = [](RuntimeContext &runtime_context, uint64_t instance_id, uint64_t *arguments, size_t argument_count) -> uint64_t {
    (void)instance_id;
    (void)argument_count;
    print_char_array_to_stream(runtime_context, std::cout, static_cast<Address>(arguments[0]), false, "[DEBUG] ");
    return 0;
  };
  native_registry["solix.systems.Console.debug(solix.String)"] = [](RuntimeContext &runtime_context, uint64_t instance_id, uint64_t *arguments, size_t argument_count) -> uint64_t {
    (void)instance_id;
    (void)argument_count;
    print_solix_string_to_stream(runtime_context, std::cout, static_cast<Address>(arguments[0]), false, "[DEBUG] ");
    return 0;
  };
  native_registry["solix.systems.Console.debugln(char[])"] = [](RuntimeContext &runtime_context, uint64_t instance_id, uint64_t *arguments, size_t argument_count) -> uint64_t {
    (void)instance_id;
    (void)argument_count;
    print_char_array_to_stream(runtime_context, std::cout, static_cast<Address>(arguments[0]), true, "[DEBUG] ");
    return 0;
  };
  native_registry["solix.systems.Console.debugln(solix.String)"] = [](RuntimeContext &runtime_context, uint64_t instance_id, uint64_t *arguments, size_t argument_count) -> uint64_t {
    (void)instance_id;
    (void)argument_count;
    print_solix_string_to_stream(runtime_context, std::cout, static_cast<Address>(arguments[0]), true, "[DEBUG] ");
    return 0;
  };

  // ---------------------------------------------------------------------------
  // Terminal Control
  // ---------------------------------------------------------------------------
  native_registry["solix.systems.Console.clear()"] = [](RuntimeContext &runtime_context, uint64_t instance_id, uint64_t *arguments, size_t argument_count) -> uint64_t {
    (void)runtime_context;
    (void)instance_id;
    (void)arguments;
    (void)argument_count;
    std::cout << "\033[2J\033[H";
    std::cout.flush();
    return 0;
  };
  native_registry["solix.systems.Console.flush()"] = [](RuntimeContext &runtime_context, uint64_t instance_id, uint64_t *arguments, size_t argument_count) -> uint64_t {
    (void)runtime_context;
    (void)instance_id;
    (void)arguments;
    (void)argument_count;
    std::cout.flush();
    std::cerr.flush();
    return 0;
  };
  native_registry["solix.systems.Console.beep()"] = [](RuntimeContext &runtime_context, uint64_t instance_id, uint64_t *arguments, size_t argument_count) -> uint64_t {
    (void)runtime_context;
    (void)instance_id;
    (void)arguments;
    (void)argument_count;
    std::cout << '\a';
    std::cout.flush();
    return 0;
  };

  // ---------------------------------------------------------------------------
  // Colors & Styling
  // ---------------------------------------------------------------------------
  native_registry["solix.systems.Console.set_color(solix.systems.ConsoleColor)"] = [](RuntimeContext &runtime_context, uint64_t instance_id, uint64_t *arguments, size_t argument_count) -> uint64_t {
    (void)runtime_context;
    (void)instance_id;
    (void)argument_count;
    std::cout << get_ansi_foreground_color(arguments[0]);
    return 0;
  };
  native_registry["solix.systems.Console.set_background(solix.systems.ConsoleColor)"] = [](RuntimeContext &runtime_context, uint64_t instance_id, uint64_t *arguments, size_t argument_count) -> uint64_t {
    (void)runtime_context;
    (void)instance_id;
    (void)argument_count;
    std::cout << get_ansi_background_color(arguments[0]);
    return 0;
  };
  native_registry["solix.systems.Console.reset_color()"] = [](RuntimeContext &runtime_context, uint64_t instance_id, uint64_t *arguments, size_t argument_count) -> uint64_t {
    (void)runtime_context;
    (void)instance_id;
    (void)arguments;
    (void)argument_count;
    std::cout << "\033[0m";
    return 0;
  };

  // ---------------------------------------------------------------------------
  // User Input Functions (Returns values directly to the VM)
  // ---------------------------------------------------------------------------
  native_registry["solix.systems.Console.input_char()"] = [](RuntimeContext &runtime_context, uint64_t instance_id, uint64_t *arguments, size_t argument_count) -> uint64_t {
    (void)runtime_context;
    (void)instance_id;
    (void)arguments;
    (void)argument_count;
    char character_input = 0;
    if (std::cin.get(character_input)) {
      return static_cast<uint64_t>(character_input);
    }
    std::cin.clear();
    return 0;
  };

  native_registry["solix.systems.Console.input_bool()"] = [](RuntimeContext &runtime_context, uint64_t instance_id, uint64_t *arguments, size_t argument_count) -> uint64_t {
    (void)runtime_context;
    (void)instance_id;
    (void)arguments;
    (void)argument_count;
    std::string token_string;
    if (std::cin >> token_string) {
      if (token_string == "true" || token_string == "1" || token_string == "True" || token_string == "TRUE") {
        return 1;
      }
      return 0;
    }
    std::cin.clear();
    return 0;
  };

  native_registry["solix.systems.Console.input_int8()"] = [](RuntimeContext &runtime_context, uint64_t instance_id, uint64_t *arguments, size_t argument_count) -> uint64_t {
    (void)runtime_context;
    (void)instance_id;
    (void)arguments;
    (void)argument_count;
    int64_t integer_value = 0;
    if (std::cin >> integer_value) {
      return static_cast<uint64_t>(static_cast<int8_t>(integer_value));
    }
    std::cin.clear();
    return 0;
  };

  native_registry["solix.systems.Console.input_int16()"] = [](RuntimeContext &runtime_context, uint64_t instance_id, uint64_t *arguments, size_t argument_count) -> uint64_t {
    (void)runtime_context;
    (void)instance_id;
    (void)arguments;
    (void)argument_count;
    int64_t integer_value = 0;
    if (std::cin >> integer_value) {
      return static_cast<uint64_t>(static_cast<int16_t>(integer_value));
    }
    std::cin.clear();
    return 0;
  };

  native_registry["solix.systems.Console.input_int32()"] = [](RuntimeContext &runtime_context, uint64_t instance_id, uint64_t *arguments, size_t argument_count) -> uint64_t {
    (void)runtime_context;
    (void)instance_id;
    (void)arguments;
    (void)argument_count;
    int64_t integer_value = 0;
    if (std::cin >> integer_value) {
      return static_cast<uint64_t>(static_cast<int32_t>(integer_value));
    }
    std::cin.clear();
    return 0;
  };

  native_registry["solix.systems.Console.input_int64()"] = [](RuntimeContext &runtime_context, uint64_t instance_id, uint64_t *arguments, size_t argument_count) -> uint64_t {
    (void)runtime_context;
    (void)instance_id;
    (void)arguments;
    (void)argument_count;
    int64_t integer_value = 0;
    if (std::cin >> integer_value) {
      return static_cast<uint64_t>(integer_value);
    }
    std::cin.clear();
    return 0;
  };

  native_registry["solix.systems.Console.input_uint8()"] = [](RuntimeContext &runtime_context, uint64_t instance_id, uint64_t *arguments, size_t argument_count) -> uint64_t {
    (void)runtime_context;
    (void)instance_id;
    (void)arguments;
    (void)argument_count;
    uint64_t unsigned_value = 0;
    if (std::cin >> unsigned_value) {
      return static_cast<uint64_t>(static_cast<uint8_t>(unsigned_value));
    }
    std::cin.clear();
    return 0;
  };

  native_registry["solix.systems.Console.input_uint16()"] = [](RuntimeContext &runtime_context, uint64_t instance_id, uint64_t *arguments, size_t argument_count) -> uint64_t {
    (void)runtime_context;
    (void)instance_id;
    (void)arguments;
    (void)argument_count;
    uint64_t unsigned_value = 0;
    if (std::cin >> unsigned_value) {
      return static_cast<uint64_t>(static_cast<uint16_t>(unsigned_value));
    }
    std::cin.clear();
    return 0;
  };

  native_registry["solix.systems.Console.input_uint32()"] = [](RuntimeContext &runtime_context, uint64_t instance_id, uint64_t *arguments, size_t argument_count) -> uint64_t {
    (void)runtime_context;
    (void)instance_id;
    (void)arguments;
    (void)argument_count;
    uint64_t unsigned_value = 0;
    if (std::cin >> unsigned_value) {
      return static_cast<uint64_t>(static_cast<uint32_t>(unsigned_value));
    }
    std::cin.clear();
    return 0;
  };

  native_registry["solix.systems.Console.input_uint64()"] = [](RuntimeContext &runtime_context, uint64_t instance_id, uint64_t *arguments, size_t argument_count) -> uint64_t {
    (void)runtime_context;
    (void)instance_id;
    (void)arguments;
    (void)argument_count;
    uint64_t unsigned_value = 0;
    if (std::cin >> unsigned_value) {
      return unsigned_value;
    }
    std::cin.clear();
    return 0;
  };

  native_registry["solix.systems.Console.input_float32()"] = [](RuntimeContext &runtime_context, uint64_t instance_id, uint64_t *arguments, size_t argument_count) -> uint64_t {
    (void)runtime_context;
    (void)instance_id;
    (void)arguments;
    (void)argument_count;
    float float_value = 0.0f;
    if (std::cin >> float_value) {
      uint32_t float_representation = std::bit_cast<uint32_t>(float_value);
      return static_cast<uint64_t>(float_representation);
    }
    std::cin.clear();
    return 0;
  };

  native_registry["solix.systems.Console.input_float64()"] = [](RuntimeContext &runtime_context, uint64_t instance_id, uint64_t *arguments, size_t argument_count) -> uint64_t {
    (void)runtime_context;
    (void)instance_id;
    (void)arguments;
    (void)argument_count;
    double double_value = 0.0;
    if (std::cin >> double_value) {
      return std::bit_cast<uint64_t>(double_value);
    }
    std::cin.clear();
    return 0;
  };

  native_registry["solix.systems.Console.input_line()"] = [](RuntimeContext &runtime_context, uint64_t instance_id, uint64_t *arguments, size_t argument_count) -> uint64_t {
    (void)instance_id;
    (void)arguments;
    (void)argument_count;
    std::string input_line_string;
    if (!std::getline(std::cin, input_line_string)) {
      std::cin.clear();
      input_line_string = "";
    }
    size_t character_length = input_line_string.length();
    Address array_address = runtime_context.memory.dynamic_allocation(character_length);
    for (size_t character_index = 0; character_index < character_length; ++character_index) {
      runtime_context.memory.heap[array_address + character_index] = static_cast<uint64_t>(input_line_string[character_index]);
    }
    return static_cast<uint64_t>(array_address);
  };

  // Also register solix.core.String variants for console print methods
  std::vector<std::pair<std::string, NativeFunction>> core_entries;
  for (const auto &[name, func] : native_registry) {
    if (name.find("(solix.String)") != std::string::npos) {
      std::string core_name = name;
      size_t pos = core_name.find("(solix.String)");
      core_name.replace(pos, 14, "(solix.core.String)");
      core_entries.emplace_back(core_name, func);
    }
  }
  for (auto &pair : core_entries) {
    native_registry[pair.first] = pair.second;
  }
}

} // namespace solix
