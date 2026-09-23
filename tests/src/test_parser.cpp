#include "solix/compilation.hpp"
#include "processes/lexer.hpp"
#include "processes/parser.hpp"
#include "utilities/statements.hpp"
#include "utilities/diagnostic.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace solix;

inline std::vector<std::unique_ptr<Node>> test_parse(const std::string &code) {
  CompilationOptions options;
  options.log_level = CompilationOptions::LogLevel::ERR;
  Source src_key = std::string("test");
  options.sources[src_key] = code;

  CompilationContext context(options);
  context.diagnostic = std::make_unique<Diagnostic>(context);

  Lexer lexer(context, "Lexer");
  lexer.execute();

  Parser parser(context, "Parser");
  parser.execute();

  return std::move(context.nodes[src_key]);
}

inline std::vector<std::unique_ptr<Node>>
test_parse_file(const std::filesystem::path &path) {
  CompilationOptions options;
  options.log_level = CompilationOptions::LogLevel::ERR;
  Source src_key = path;
  options.sources[src_key] = std::nullopt;

  CompilationContext context(options);
  context.diagnostic = std::make_unique<Diagnostic>(context);

  Lexer lexer(context, "Lexer");
  lexer.execute();

  Parser parser(context, "Parser");
  parser.execute();

  return std::move(context.nodes[src_key]);
}

TEST_CASE("New Parser - Variable Declarations", "[new_parser]") {
  REQUIRE_NOTHROW(test_parse("class Map<K, V> { K key; V value; } alias IntMap<V> = Map<int32, V>; void main() { Map<int32, float32> map; }"));
    auto nodes = test_parse("const int32[] x = 5; char[] y;");
  REQUIRE(nodes.size() == 2);

  REQUIRE(nodes[0]->node_type == NodeType::FIELD_DECL);
  auto *x_decl = static_cast<FieldDeclaration *>(nodes[0].get());
  REQUIRE(x_decl->field_name == "x");
  REQUIRE(x_decl->type_info.name == "int32");
  REQUIRE(x_decl->type_info.array_depth == 1);
  REQUIRE(x_decl->is_const == true);

  REQUIRE(nodes[1]->node_type == NodeType::FIELD_DECL);
  auto *y_decl = static_cast<FieldDeclaration *>(nodes[1].get());
  REQUIRE(y_decl->field_name == "y");
  REQUIRE(y_decl->type_info.name == "char");
  REQUIRE(y_decl->type_info.array_depth == 1);
}

TEST_CASE("New Parser - Classes and Methods", "[new_parser]") {
  REQUIRE_NOTHROW(test_parse("class Map<K, V> { K key; V value; } alias IntMap<V> = Map<int32, V>; void main() { Map<int32, float32> map; }"));
    auto nodes = test_parse(
      "public class MyClass { public void test(int32 param) { return; } }");
  REQUIRE(nodes.size() == 1);

  REQUIRE(nodes[0]->node_type == NodeType::CLASS_DECL);
  auto *cls = static_cast<ClassDeclaration *>(nodes[0].get());
  REQUIRE(cls->class_name == "MyClass");
  REQUIRE(cls->access_modifier == TokenType::KEYWORD_PUBLIC);
  REQUIRE(cls->children.size() == 1);

  auto *method = static_cast<MethodDeclaration *>(cls->children[0].get());
  REQUIRE(method->method_name == "test");
  REQUIRE(method->return_type.name == "void");
  REQUIRE(method->parameters.size() == 1);
  REQUIRE(method->parameters[0]->var_name == "param");

  REQUIRE(method->children.size() == 1);
  auto *block = static_cast<BlockStatement *>(method->children[0].get());
  REQUIRE(block->children.size() == 1);
  REQUIRE(block->children[0]->node_type == NodeType::RETURN_STMT);
}

TEST_CASE("New Parser - Full test.slx script", "[new_parser]") {
  auto nodes =
      test_parse_file("/home/jehud/Projects/solix/tests/resources/test.slx");
  // Ensure we parse successfully without exceptions
  REQUIRE(!nodes.empty());

  // Check that we got a valid top level construct
  REQUIRE((nodes[0]->node_type == NodeType::CLASS_DECL ||
           nodes[0]->node_type == NodeType::PACKAGE_STMT ||
           nodes[0]->node_type == NodeType::ALIAS_STMT));
}

TEST_CASE("Phase 18 - Templates Parser", "[templates_parser]") {
    solix::CompilationOptions opts;
    opts.sources[std::string("test")] = "class Map<K, V> { K key; V value; } alias IntMap<V> = Map<int32, V>; void main() { Map<int32, float32> map; }";
    solix::CompilationContext ctx(opts);
    
    solix::Lexer lexer(ctx, "Lexer");
    lexer.execute();
    
    solix::Parser parser(ctx, "Parser");
    REQUIRE_NOTHROW(parser.execute());
    
    auto& nodes = ctx.nodes[std::string("test")];
    REQUIRE(nodes.size() == 3);
    
    // Check Class
    REQUIRE(nodes[0]->node_type == solix::NodeType::CLASS_DECL);
    auto* cls = static_cast<solix::ClassDeclaration*>(nodes[0].get());
    REQUIRE(cls->class_name == "Map");
    REQUIRE(cls->template_parameters.size() == 2);
    REQUIRE(cls->template_parameters[0] == "K");
    REQUIRE(cls->template_parameters[1] == "V");
    
    // Check Alias
    REQUIRE(nodes[1]->node_type == solix::NodeType::ALIAS_STMT);
    auto* alias = static_cast<solix::AliasStatement*>(nodes[1].get());
    REQUIRE(alias->alias_name == "IntMap");
    REQUIRE(alias->template_parameters.size() == 1);
    REQUIRE(alias->template_parameters[0] == "V");
    REQUIRE(alias->target_type.name == "Map");
    REQUIRE(alias->target_type.type_args.size() == 2);
    REQUIRE(alias->target_type.type_args[0].name == "int32");
    REQUIRE(alias->target_type.type_args[1].name == "V");
}
