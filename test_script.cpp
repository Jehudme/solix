#include "solix/compilation.hpp"
#include "solix/processes/lexer.hpp"
#include "solix/processes/parser.hpp"
#include "solix/statements.hpp"
#include <iostream>

using namespace solix;
int main() {
  CompilationOptions options;
  options.log_level = CompilationOptions::LogLevel::ERR;
  Source src_key = std::string("test");
  options.sources[src_key] = "class Map<K, V> { K key; V value; } alias IntMap<V> = Map<int32, V>; void main() { Map<int32, float32> map; }";
  CompilationContext context(options);
  Lexer lexer(context, "Lexer");
  lexer.execute();
  Parser parser(context, "Parser");
  parser.execute();
  std::cout << context.nodes[src_key].size() << std::endl;
}
