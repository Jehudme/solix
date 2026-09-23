#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>
#include <chrono>

namespace solix {

struct Token;
struct Node;

using TokenList = std::vector<Token>;
using NodeList = std::vector<std::unique_ptr<Node>>;
using Source = std::variant<std::string, std::filesystem::path>;

class CompilationProcess;
struct Lexer;
struct Parser;
struct Binder;
struct Assembler;
class Diagnostic;

struct CompilationOptions {
  enum class LogLevel { TRACE, DEBUG, INFO, WARN, ERR, CRITICAL, OFF };
  enum class LogSinkType {
    STDOUT,
    STDERR,
    BASIC_FILE,
    CONSOLE_AND_FILE
  };

  LogLevel log_level = LogLevel::INFO;
  LogLevel flush_level = LogLevel::ERR;
  LogSinkType sink_type = LogSinkType::STDOUT;

  // The pattern [Level]   [ProcessName]   Message
  std::string log_pattern = "[%^%-8l%$] [%-12n] %v";

  std::optional<std::filesystem::path> log_file_path;
  std::optional<std::filesystem::path> assembly_output_path;

  bool use_multithreading = false;
  std::chrono::seconds flush_every_seconds{0};

  std::string entry_point = "main";

  std::unordered_map<Source, std::optional<std::string>> sources;
};

struct CompilationContext {
  const CompilationOptions &options;

  std::unique_ptr<Lexer> lexer;
  std::unique_ptr<Parser> parser;
  std::unique_ptr<Binder> binder;
  std::unique_ptr<Assembler> assembler;
  std::unique_ptr<Diagnostic> diagnostic;

  std::unordered_map<Source, std::vector<TokenList>> tokens;
  std::unordered_map<Source, NodeList> nodes;
  std::unordered_map<std::string, Node *> symbols;
  std::unordered_map<std::string, int> string_pool;
  std::vector<uint8_t> bytecode;
  std::string assembly;
  
  CompilationContext(const CompilationOptions& opts);
  ~CompilationContext();
};

std::vector<uint8_t> run(CompilationOptions& options);
} // namespace solix
