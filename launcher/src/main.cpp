#include "solix/new/compilation.hpp"
#include "solix/new/processes/lexer.hpp"
#include "solix/new/processes/parser.hpp"
#include "solix/new/processes/binder.hpp"
#include "solix/new/processes/assembler.hpp"
#include "solix/new/utilities/diagnostic.hpp"
#include "solix/new/utilities/optcodes.hpp"
#include "solix/new/runtime.hpp"
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

  std::cout << "--- Compilation Successful ---" << std::endl;

  solix::register_native_function("com.solix.advanced.test.Engine.print", [](RuntimeContext& ctx, uint64_t self_address, uint64_t* args, size_t arg_count) {
      // In the old system, the native function manually popped args.
      // But now we pass the signature required.
      // Wait, since we don't pass args from VM (args = nullptr), we will manually pop.
      uint64_t val = ctx.pop();
      Address addr = static_cast<Address>(val);
      
      uint32_t len = static_cast<uint32_t>(ctx.memory.heap[addr]);
      std::string str = "";
      for (uint32_t i = 0; i < len; ++i) {
          str += static_cast<char>(ctx.memory.heap[addr + 1 + i]);
      }
      
      std::cout << "NATIVE PRINT: " << str << std::endl;
      
      ctx.push(0);
  });

  RuntimeOptions run_opts;
  run_opts.bytecode_source = context.bytecode;
  RuntimeContext vm(run_opts);

  std::cout << "--- Executing ---" << std::endl;
  try {
      vm.execute();
  } catch(const std::exception& e) {
      std::cerr << "Runtime Exception: " << e.what() << std::endl;
      return 1;
  }
  std::cout << "--- Execution Finished ---" << std::endl;

  return 0;
}
