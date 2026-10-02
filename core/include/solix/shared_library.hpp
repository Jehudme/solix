#pragma once

#include <filesystem>
#include <string>
#include <stdexcept>

namespace solix {

class SharedLibraryException : public std::runtime_error {
public:
  explicit SharedLibraryException(const std::string &message)
      : std::runtime_error(message) {}
};

class SharedLibrary {
public:
  SharedLibrary() = default;
  explicit SharedLibrary(const std::filesystem::path &path);
  ~SharedLibrary();

  SharedLibrary(const SharedLibrary &) = delete;
  SharedLibrary &operator=(const SharedLibrary &) = delete;
  SharedLibrary(SharedLibrary &&other) noexcept;
  SharedLibrary &operator=(SharedLibrary &&other) noexcept;

  bool load(const std::filesystem::path &path);
  void unload();
  bool is_loaded() const;

  void *get_symbol(const std::string &symbol_name) const;

  template <typename T>
  T get_symbol(const std::string &symbol_name) const {
    return reinterpret_cast<T>(get_symbol(symbol_name));
  }

  const std::filesystem::path &path() const { return path_; }

private:
  void *handle_{nullptr};
  std::filesystem::path path_;
};

} // namespace solix
