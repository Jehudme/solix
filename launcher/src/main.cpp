#include "solix/new/compilation.hpp"
#include "solix/new/processes/lexer.hpp"
#include "solix/new/processes/parser.hpp"
#include "solix/new/processes/binder.hpp"
#include "solix/new/processes/assembler.hpp"
#include "solix/new/utilities/diagnostic.hpp"
#include "solix/new/utilities/optcodes.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

using namespace solix;

int main(int argc, char** argv) {
  if (argc < 2) {
      std::cerr << "Usage: " << argv[0] << " <file.slx>\n";
      return 1;
  }
  CompilationOptions options;
  options.log_level = CompilationOptions::LogLevel::OFF;
  options.entry_point = "main";
  Source src_key = std::string(argv[1]);

  std::ifstream t(argv[1]);
  if (!t.is_open()) {
      std::cerr << "Failed to open file.\n";
      return 1;
  }
  std::stringstream buffer;
  buffer << t.rdbuf();
  options.sources[src_key] = buffer.str();

  CompilationContext context(options);
  context.diagnostic = std::make_unique<Diagnostic>(context);

  try {
      Lexer lexer(context, "Lexer");
      lexer.execute();
      Parser parser(context, "Parser");
      parser.execute();
      Binder binder(context, "Binder");
      binder.execute();
      Assembler assembler(context, "Assembler");
      assembler.execute();
  } catch (const std::exception& e) {
      std::cerr << "Compilation error: " << e.what() << "\n";
      return 1;
  }

  auto& bcode = context.bytecode;
  for (size_t i = 0; i < bcode.size(); ) {
      uint8_t op = bcode[i];
      std::cout << opcode_to_string(op);
      switch (static_cast<OpCode>(op)) {
          case OpCode::PUSH_CONST_I32:
          case OpCode::GET_GLOBAL:
          case OpCode::SET_GLOBAL:
          case OpCode::GET_LOCAL:
          case OpCode::SET_LOCAL:
          case OpCode::GET_PROPERTY:
          case OpCode::SET_PROPERTY:
          case OpCode::JUMP:
          case OpCode::JUMP_IF_FALSE:
          case OpCode::JUMP_IF_TRUE:
          case OpCode::DEFINE_NATIVE:
          case OpCode::CALL_NATIVE: {
              uint32_t val = (bcode[i+1] << 24) | (bcode[i+2] << 16) | (bcode[i+3] << 8) | bcode[i+4];
              std::cout << " " << val;
              i += 5;
              if (static_cast<OpCode>(op) == OpCode::DEFINE_NATIVE) {
                  uint32_t str_len = (bcode[i] << 24) | (bcode[i+1] << 16) | (bcode[i+2] << 8) | bcode[i+3];
                  i += 4;
                  std::string s(reinterpret_cast<char*>(&bcode[i]), str_len);
                  std::cout << " \"" << s << "\"";
                  i += str_len;
              }
              break;
          }
          case OpCode::PUSH_CONST_I64: {
              uint64_t val = ((uint64_t)bcode[i+1] << 56) | ((uint64_t)bcode[i+2] << 48) |
                             ((uint64_t)bcode[i+3] << 40) | ((uint64_t)bcode[i+4] << 32) |
                             ((uint64_t)bcode[i+5] << 24) | ((uint64_t)bcode[i+6] << 16) |
                             ((uint64_t)bcode[i+7] << 8) | bcode[i+8];
              std::cout << " " << val;
              i += 9;
              break;
          }
          case OpCode::PUSH_CONST_STRING: {
              uint32_t str_len = (bcode[i+1] << 24) | (bcode[i+2] << 16) | (bcode[i+3] << 8) | bcode[i+4];
              i += 5;
              std::string s(reinterpret_cast<char*>(&bcode[i]), str_len);
              std::cout << " \"" << s << "\"";
              i += str_len;
              break;
          }
          default:
              i += 1;
              break;
      }
      std::cout << "\n";
  }
  return 0;
}
