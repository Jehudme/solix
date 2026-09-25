#include "natives/utilities.hpp"
#include "solix/runtime.hpp"
#include <bit>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>

namespace solix {

namespace {

inline std::string read_solix_char_array_to_string(RuntimeContext &runtime_context, Address array_address) {
  if (array_address == 0) {
    return "";
  }
  uint32_t character_length = static_cast<uint32_t>(runtime_context.memory.heap[array_address - 1] >> 32);
  std::string result_string;
  result_string.reserve(character_length);
  for (uint32_t character_index = 0; character_index < character_length; ++character_index) {
    char character_value = static_cast<char>(runtime_context.memory.heap[array_address + character_index]);
    if (character_value == '\0') {
      break;
    }
    result_string.push_back(character_value);
  }
  return result_string;
}

inline Address allocate_solix_char_array_from_string(RuntimeContext &runtime_context, const std::string &source_string) {
  size_t character_length = source_string.length();
  Address allocated_address = runtime_context.memory.dynamic_allocation(character_length);
  for (size_t character_index = 0; character_index < character_length; ++character_index) {
    runtime_context.memory.heap[allocated_address + character_index] = static_cast<uint64_t>(source_string[character_index]);
  }
  return allocated_address;
}

} // namespace

void register_utilities_natives(std::unordered_map<std::string, NativeFunction> &native_registry) {
  native_registry["solix.NativeUtilities.copy_chars(char[],int32,char[],int32,int32)"] =
      [](RuntimeContext &runtime_context, uint64_t instance_id, uint64_t *arguments, size_t argument_count) -> uint64_t {
        (void)instance_id;
        (void)argument_count;
        Address source_address = static_cast<Address>(arguments[0]);
        int32_t source_offset = static_cast<int32_t>(arguments[1]);
        Address destination_address = static_cast<Address>(arguments[2]);
        int32_t destination_offset = static_cast<int32_t>(arguments[3]);
        int32_t copy_count = static_cast<int32_t>(arguments[4]);

        if (source_address == 0 || destination_address == 0 || copy_count <= 0) {
          return 0;
        }

        uint32_t source_length = static_cast<uint32_t>(runtime_context.memory.heap[source_address - 1] >> 32);
        uint32_t destination_length = static_cast<uint32_t>(runtime_context.memory.heap[destination_address - 1] >> 32);

        if (source_offset < 0 || destination_offset < 0 ||
            static_cast<uint32_t>(source_offset + copy_count) > source_length ||
            static_cast<uint32_t>(destination_offset + copy_count) > destination_length) {
          return 0;
        }

        for (int32_t copy_index = 0; copy_index < copy_count; ++copy_index) {
          runtime_context.memory.heap[destination_address + destination_offset + copy_index] =
              runtime_context.memory.heap[source_address + source_offset + copy_index];
        }
        return 0;
      };

  native_registry["solix.NativeUtilities.float64_to_chars(float64,int32)"] =
      [](RuntimeContext &runtime_context, uint64_t instance_id, uint64_t *arguments, size_t argument_count) -> uint64_t {
        (void)instance_id;
        (void)argument_count;
        double float_value = std::bit_cast<double>(arguments[0]);
        int32_t precision_digits = static_cast<int32_t>(arguments[1]);

        char formatted_buffer[64];
        if (precision_digits < 0) {
          std::snprintf(formatted_buffer, sizeof(formatted_buffer), "%g", float_value);
        } else {
          std::snprintf(formatted_buffer, sizeof(formatted_buffer), "%.*f", precision_digits, float_value);
        }

        std::string formatted_string(formatted_buffer);
        Address result_address = allocate_solix_char_array_from_string(runtime_context, formatted_string);
        return static_cast<uint64_t>(result_address);
      };

  native_registry["solix.NativeUtilities.chars_to_float64(char[])"] =
      [](RuntimeContext &runtime_context, uint64_t instance_id, uint64_t *arguments, size_t argument_count) -> uint64_t {
        (void)instance_id;
        (void)argument_count;
        Address array_address = static_cast<Address>(arguments[0]);
        std::string number_string = read_solix_char_array_to_string(runtime_context, array_address);
        double parsed_value = 0.0;
        try {
          if (!number_string.empty()) {
            parsed_value = std::stod(number_string);
          }
        } catch (...) {
          parsed_value = 0.0;
        }
        return std::bit_cast<uint64_t>(parsed_value);
      };

  native_registry["solix.NativeUtilities.int64_to_chars(int64)"] =
      [](RuntimeContext &runtime_context, uint64_t instance_id, uint64_t *arguments, size_t argument_count) -> uint64_t {
        (void)instance_id;
        (void)argument_count;
        int64_t integer_value = static_cast<int64_t>(arguments[0]);
        std::string integer_string = std::to_string(integer_value);
        Address result_address = allocate_solix_char_array_from_string(runtime_context, integer_string);
        return static_cast<uint64_t>(result_address);
      };

  native_registry["solix.NativeUtilities.chars_to_int64(char[])"] =
      [](RuntimeContext &runtime_context, uint64_t instance_id, uint64_t *arguments, size_t argument_count) -> uint64_t {
        (void)instance_id;
        (void)argument_count;
        Address array_address = static_cast<Address>(arguments[0]);
        std::string integer_string = read_solix_char_array_to_string(runtime_context, array_address);
        int64_t parsed_value = 0;
        try {
          if (!integer_string.empty()) {
            parsed_value = std::stoll(integer_string);
          }
        } catch (...) {
          parsed_value = 0;
        }
        return static_cast<uint64_t>(parsed_value);
      };

  native_registry["solix.NativeUtilities.current_time_millis()"] =
      [](RuntimeContext &runtime_context, uint64_t instance_id, uint64_t *arguments, size_t argument_count) -> uint64_t {
        (void)runtime_context;
        (void)instance_id;
        (void)arguments;
        (void)argument_count;
        auto current_epoch_duration = std::chrono::system_clock::now().time_since_epoch();
        auto milliseconds_count = std::chrono::duration_cast<std::chrono::milliseconds>(current_epoch_duration).count();
        return static_cast<uint64_t>(milliseconds_count);
      };

  native_registry["solix.NativeUtilities.current_time_nanos()"] =
      [](RuntimeContext &runtime_context, uint64_t instance_id, uint64_t *arguments, size_t argument_count) -> uint64_t {
        (void)runtime_context;
        (void)instance_id;
        (void)arguments;
        (void)argument_count;
        auto current_high_res_duration = std::chrono::high_resolution_clock::now().time_since_epoch();
        auto nanoseconds_count = std::chrono::duration_cast<std::chrono::nanoseconds>(current_high_res_duration).count();
        return static_cast<uint64_t>(nanoseconds_count);
      };

  // Also register solix.core.NativeUtilities.* variants
  std::vector<std::pair<std::string, NativeFunction>> core_entries;
  for (const auto &[name, func] : native_registry) {
    if (name.rfind("solix.NativeUtilities.", 0) == 0) {
      std::string core_name = "solix.core.NativeUtilities." + name.substr(std::string("solix.NativeUtilities.").length());
      core_entries.emplace_back(core_name, func);
    }
  }
  for (auto &pair : core_entries) {
    native_registry[pair.first] = pair.second;
  }
}

} // namespace solix
