#include "solix/compilation.hpp"
#include "processes/lexer.hpp"
#include "processes/parser.hpp"
#include "processes/binder.hpp"
#include "processes/assembler.hpp"
#include "utilities/diagnostic.hpp"
#include "utilities/optcodes.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace solix;

inline std::string test_assemble(const std::string &code) {
  CompilationOptions options;
  options.log_level = CompilationOptions::LogLevel::OFF;
  options.entry_point = "main";
  Source src_key = std::string("test");
  options.sources[src_key] = code;

  CompilationContext context(options);
  context.diagnostic = std::make_unique<Diagnostic>(context);

  Lexer lexer(context, "Lexer");
  lexer.execute();

  Parser parser(context, "Parser");
  parser.execute();

  Binder binder(context, "Binder");
  binder.execute();

  Assembler assembler(context, "Assembler");
  assembler.execute();

  // Disassemble to string for verification
  std::stringstream ss;
  auto& bcode = context.bytecode;
  for (size_t i = 0; i < bcode.size(); ) {
      uint8_t op = bcode[i];
      ss << opcode_to_string(op);
      
      switch (static_cast<OpCode>(op)) {
          case OpCode::INSTANCEOF:
          case OpCode::SET_VTABLE: {
              uint32_t val = (bcode[i+1] << 24) | (bcode[i+2] << 16) | (bcode[i+3] << 8) | bcode[i+4];
              ss << " " << val;
              i += 5;
              break;
          }
          case OpCode::REGISTER_RETURN_CLEANUP: {
              uint32_t ret_ip = (bcode[i+1] << 24) | (bcode[i+2] << 16) | (bcode[i+3] << 8) | bcode[i+4];
              uint32_t cleanup_ip = (bcode[i+5] << 24) | (bcode[i+6] << 16) | (bcode[i+7] << 8) | bcode[i+8];
              ss << " ret_ip=" << ret_ip << " cleanup_ip=" << cleanup_ip;
              i += 9;
              break;
          }
          case OpCode::THROW_EXCEPTION: {
              uint32_t cleanup_ip = (bcode[i+1] << 24) | (bcode[i+2] << 16) | (bcode[i+3] << 8) | bcode[i+4];
              ss << " cleanup_ip=" << cleanup_ip;
              i += 5;
              break;
          }
          case OpCode::DEFINE_VTABLE: {
              uint32_t vtable_id = (bcode[i+1] << 24) | (bcode[i+2] << 16) | (bcode[i+3] << 8) | bcode[i+4];
              uint32_t base_id = (bcode[i+5] << 24) | (bcode[i+6] << 16) | (bcode[i+7] << 8) | bcode[i+8];
              uint32_t count = (bcode[i+9] << 24) | (bcode[i+10] << 16) | (bcode[i+11] << 8) | bcode[i+12];
              ss << " vtable_id=" << vtable_id << " base=" << base_id << " count=" << count;
              i += 13 + count * 4;
              break;
          }
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
              ss << " " << val;
              i += 5;
              
              if (static_cast<OpCode>(op) == OpCode::DEFINE_NATIVE) {
                  uint32_t str_len = (bcode[i] << 24) | (bcode[i+1] << 16) | (bcode[i+2] << 8) | bcode[i+3];
                  i += 4;
                  std::string s(reinterpret_cast<char*>(&bcode[i]), str_len);
                  ss << " \"" << s << "\"";
                  i += str_len;
              }
              break;
          }
          case OpCode::PUSH_CONST_I64: {
              uint64_t val = ((uint64_t)bcode[i+1] << 56) | ((uint64_t)bcode[i+2] << 48) |
                             ((uint64_t)bcode[i+3] << 40) | ((uint64_t)bcode[i+4] << 32) |
                             ((uint64_t)bcode[i+5] << 24) | ((uint64_t)bcode[i+6] << 16) |
                             ((uint64_t)bcode[i+7] << 8) | bcode[i+8];
              ss << " " << val;
              i += 9;
              break;
          }
          case OpCode::PUSH_CONST_STRING: {
              uint32_t str_len = (bcode[i+1] << 24) | (bcode[i+2] << 16) | (bcode[i+3] << 8) | bcode[i+4];
              i += 5;
              std::string s(reinterpret_cast<char*>(&bcode[i]), str_len);
              ss << " \"" << s << "\"";
              i += str_len;
              break;
          }
          default:
              i += 1;
              break;
      }
      ss << "\n";
  }
  return ss.str();
}

TEST_CASE("New Assembler - Basic Variables", "[new_assembler]") {
    std::string asm_code = test_assemble(R"(
        public class Main {
            public static void main() {
                int32 x = 5;
            }
        }
    )");
    
    // We expect some boot sequence, then main function
    // Look for SET_LOCAL 0 and PUSH_CONST_I64 5 (since literals are 64-bit currently)
    REQUIRE(asm_code.find("PUSH_CONST_I64 5") != std::string::npos);
    REQUIRE(asm_code.find("SET_LOCAL") != std::string::npos);
}

TEST_CASE("Try-Catch Assembler test", "[assembler]") {
    std::string code = R"(
        class Exception {}
        class CustomException extends Exception {}
        
        class Thrower {
            public void do_throw() {
                throw new CustomException();
            }
        }
        
        public class Main {
            public static void main() {
                try {
                    Thrower thrower = new Thrower();
                    thrower.do_throw();
                } catch (CustomException e) {
                } catch (Exception e) {
                }
            }
        }
    )";
    
    // We just want to make sure it compiles without syntax/binder errors
    // and doesn't segfault the assembler.
    REQUIRE_NOTHROW(test_assemble(code));
}

TEST_CASE("Vtable optimization: Normal classes don't get vtables", "[assembler]") {
    std::string code = R"(
        class NormalClass {
            public int32 value;
        }
        
        class Exception {}
        class CustomException extends Exception {}
        
        public class Main {
            public static void main() {
                NormalClass obj = new NormalClass();
                CustomException exc = new CustomException();
            }
        }
    )";
    
    std::string asm_code = test_assemble(code);
    
    // NormalClass should not have a DEFINE_VTABLE instruction
    // Note: In Binder, vtable IDs start at 0 and go up.
    // We expect Exception to have one, CustomException to have one.
    // If NormalClass doesn't have one, it will not use SET_VTABLE.
    // Let's check the number of DEFINE_VTABLEs.
    // Wait, test_assemble dumps all DEFINE_VTABLEs. Let's see if NormalClass gets one.
    // If we count the number of DEFINE_VTABLE instructions...
    
    // The only classes that should get vtables are Exception and CustomException
    // Main has no virtual methods. NormalClass has no virtual methods.
    
    // Check that SET_VTABLE is emitted for CustomException but NOT for NormalClass
    // Since we can't easily parse the exact IDs, we can just check the number of SET_VTABLEs.
    // There should be exactly 1 SET_VTABLE in main() for CustomException.
    
    int set_vtable_count = 0;
    size_t pos = 0;
    while ((pos = asm_code.find("SET_VTABLE", pos)) != std::string::npos) {
        set_vtable_count++;
        pos += 10;
    }
    
    // 1 for CustomException in main
    REQUIRE(set_vtable_count == 1);
}
