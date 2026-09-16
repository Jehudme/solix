#include "solix/new/compilation.hpp"
#include "solix/new/processes/lexer.hpp"
#include "solix/new/utilities/diagnostic.hpp"
#include "solix/new/utilities/token.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace solix;

inline TokenList test_tokenize(const std::string &code) {
  CompilationOptions options;
  options.log_level = CompilationOptions::LogLevel::OFF;
  Source src_key = std::string("test");
  options.sources[src_key] = code;

  CompilationContext context(options);
  context.diagnostic = std::make_unique<Diagnostic>(context);

  Lexer lexer(context, "Lexer");
  lexer.execute();

  return context.tokens[src_key].front();
}

inline TokenList test_tokenize_file(const std::filesystem::path &path) {
  CompilationOptions options;
  options.log_level = CompilationOptions::LogLevel::OFF;
  Source src_key = path;
  options.sources[src_key] = std::nullopt;

  CompilationContext context(options);
  context.diagnostic = std::make_unique<Diagnostic>(context);

  Lexer lexer(context, "Lexer");
  lexer.execute();

  return context.tokens[src_key].front();
}

TEST_CASE("New Lexer - Primitives and Strings", "[new_lexer]") {
  auto tokens =
      test_tokenize("int32 x = 5; char[] s = \"hello\"; string y = \"world\";");
  // int32, x, =, 5, ;, char, [], s, =, "hello", ;, string, y, =, "world", ;,
  // EOF
  REQUIRE(tokens.size() == 17);
  REQUIRE(tokens[0].type == TokenType::PRIMITIVE_INT32);
  REQUIRE(tokens[1].type == TokenType::IDENTIFIER);
  REQUIRE(std::get<std::string>(tokens[1].value) == "x");
  REQUIRE(tokens[3].type == TokenType::NUMBER);
  REQUIRE(std::get<int64_t>(tokens[3].value) == 5);

  REQUIRE(tokens[5].type == TokenType::PRIMITIVE_CHAR);
  REQUIRE(tokens[6].type == TokenType::PUNCTUATION_ARRAY_BRACKETS);
  REQUIRE(tokens[9].type == TokenType::STRING);
  REQUIRE(std::get<std::string>(tokens[9].value) == "hello");

  // 'string' is no longer a primitive, it's an IDENTIFIER
  REQUIRE(tokens[11].type == TokenType::IDENTIFIER);
  REQUIRE(std::get<std::string>(tokens[11].value) == "string");
}

TEST_CASE("New Lexer - Classes, Functions and Enums", "[new_lexer]") {
  auto tokens = test_tokenize(
      "public class MyClass { public void test() {} } enum State { A, B }");
  REQUIRE(tokens[0].type == TokenType::KEYWORD_PUBLIC);
  REQUIRE(tokens[1].type == TokenType::KEYWORD_CLASS);
  REQUIRE(tokens[2].type == TokenType::IDENTIFIER);
  REQUIRE(tokens[3].type == TokenType::PUNCTUATION_OPEN_BRACE);
  REQUIRE(tokens[10].type == TokenType::PUNCTUATION_CLOSE_BRACE);

  REQUIRE(tokens[12].type == TokenType::KEYWORD_ENUM);
  REQUIRE(tokens[13].type == TokenType::IDENTIFIER);
  REQUIRE(tokens[15].type == TokenType::IDENTIFIER);
  REQUIRE(std::get<std::string>(tokens[15].value) == "A");
}

TEST_CASE("New Lexer - Full test.slx script", "[new_lexer]") {
  auto tokens =
      test_tokenize_file("/home/jehud/Projects/solix/tests/resources/test.slx");
  REQUIRE(!tokens.empty());
  REQUIRE(tokens.back().type == TokenType::EOF_TOKEN);

  // Check that we lexed a reasonable number of tokens
  REQUIRE(tokens.size() > 50);
}
