#include "solix/shared_library.hpp"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace solix {

namespace {

std::string get_last_error_string() {
#ifdef _WIN32
  DWORD error_code = GetLastError();
  if (error_code == 0) return "No error";
  LPSTR message_buffer = nullptr;
  size_t size = FormatMessageA(
      FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
      NULL, error_code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      (LPSTR)&message_buffer, 0, NULL);
  std::string message(message_buffer, size);
  LocalFree(message_buffer);
  while (!message.empty() && (message.back() == '\r' || message.back() == '\n')) {
    message.pop_back();
  }
  return message + " (Error code: " + std::to_string(error_code) + ")";
#else
  const char *err = dlerror();
  return err ? std::string(err) : "Unknown dynamic library error";
#endif
}

} // namespace

SharedLibrary::SharedLibrary(const std::filesystem::path &path) {
  load(path);
}

SharedLibrary::~SharedLibrary() {
  unload();
}

SharedLibrary::SharedLibrary(SharedLibrary &&other) noexcept
    : handle_(other.handle_), path_(std::move(other.path_)) {
  other.handle_ = nullptr;
}

SharedLibrary &SharedLibrary::operator=(SharedLibrary &&other) noexcept {
  if (this != &other) {
    unload();
    handle_ = other.handle_;
    path_ = std::move(other.path_);
    other.handle_ = nullptr;
  }
  return *this;
}

bool SharedLibrary::load(const std::filesystem::path &path) {
  unload();
  path_ = path;

#ifdef _WIN32
  handle_ = LoadLibraryW(path.wstring().c_str());
#else
  handle_ = dlopen(path.string().c_str(), RTLD_NOW | RTLD_LOCAL);
#endif

  if (!handle_) {
    std::string err = get_last_error_string();
    throw SharedLibraryException("Failed to load shared library '" + path.string() + "': " + err);
  }

  return true;
}

void SharedLibrary::unload() {
  if (handle_) {
#ifdef _WIN32
    FreeLibrary(static_cast<HMODULE>(handle_));
#else
    dlclose(handle_);
#endif
    handle_ = nullptr;
  }
}

bool SharedLibrary::is_loaded() const {
  return handle_ != nullptr;
}

void *SharedLibrary::get_symbol(const std::string &symbol_name) const {
  if (!handle_) return nullptr;

#ifdef _WIN32
  return reinterpret_cast<void *>(GetProcAddress(static_cast<HMODULE>(handle_), symbol_name.c_str()));
#else
  dlerror(); // Clear previous error
  return dlsym(handle_, symbol_name.c_str());
#endif
}

} // namespace solix
