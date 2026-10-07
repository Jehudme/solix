#include "solix/native.h"
#include "solix/runtime.hpp"
#include "solix/native_registry.hpp"

#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <sstream>
#include <thread>
#include <unordered_map>
#include <mutex>
#include <iostream>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
extern char **environ;
#endif

namespace {

static std::string read_solix_chars(solix::RuntimeContext &vm, uint64_t addr) {
    if (addr == 0) return "";
    uint32_t len = static_cast<uint32_t>(vm.memory.heap[addr - 1] >> 32);
    std::string s;
    s.reserve(len);
    for (uint32_t i = 0; i < len; ++i) {
        s.push_back(static_cast<char>(vm.memory.heap[addr + i]));
    }
    return s;
}

static uint64_t allocate_solix_chars(solix::RuntimeContext &vm, const std::string &s) {
    uint64_t addr = vm.memory.dynamic_allocation(s.length());
    for (size_t i = 0; i < s.length(); ++i) {
        vm.memory.heap[addr + i] = static_cast<uint64_t>(static_cast<unsigned char>(s[i]));
    }
    return addr;
}

// --- Environment Native Functions ---

static uint64_t env_native_get_env(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string key = read_solix_chars(vm, args[0]);
    const char *val = std::getenv(key.c_str());
    if (val == nullptr) {
        return 0ULL; // null
    }
    return allocate_solix_chars(vm, std::string(val));
}

static uint64_t env_native_set_env(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string key = read_solix_chars(vm, args[0]);
    std::string val = read_solix_chars(vm, args[1]);
#if defined(_WIN32)
    _putenv_s(key.c_str(), val.c_str());
#else
    setenv(key.c_str(), val.c_str(), 1);
#endif
    return 0ULL;
}

static uint64_t env_native_get_all_env(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)args; (void)argc;
    std::string all_env;
#if defined(_WIN32)
    LPCH env_block = GetEnvironmentStringsA();
    if (env_block != nullptr) {
        LPCH cur = env_block;
        bool first = true;
        while (*cur != '\0') {
            std::string entry(cur);
            if (!entry.empty() && entry[0] != '=') {
                if (!first) all_env.push_back('\n');
                all_env.append(entry);
                first = false;
            }
            cur += entry.length() + 1;
        }
        FreeEnvironmentStringsA(env_block);
    }
#else
    if (environ != nullptr) {
        bool first = true;
        for (char **env = environ; *env != nullptr; ++env) {
            if (!first) all_env.push_back('\n');
            all_env.append(*env);
            first = false;
        }
    }
#endif
    return allocate_solix_chars(vm, all_env);
}

static uint64_t env_native_get_args(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)args; (void)argc;
    std::string joined;
    bool first = true;
    for (const auto &arg : vm.options.program_args) {
        if (!first) joined.push_back('\n');
        joined.append(arg);
        first = false;
    }
    return allocate_solix_chars(vm, joined);
}

static uint64_t env_native_os_name(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)args; (void)argc;
#if defined(_WIN32)
    return allocate_solix_chars(vm, "Windows");
#elif defined(__APPLE__)
    return allocate_solix_chars(vm, "macOS");
#elif defined(__linux__)
    return allocate_solix_chars(vm, "Linux");
#else
    return allocate_solix_chars(vm, "Unknown");
#endif
}

static uint64_t env_native_os_version(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)args; (void)argc;
#if defined(_WIN32)
    return allocate_solix_chars(vm, "Windows NT");
#elif defined(__APPLE__) || defined(__linux__)
    struct utsname uts;
    if (uname(&uts) == 0) {
        return allocate_solix_chars(vm, std::string(uts.release));
    }
    return allocate_solix_chars(vm, "Unknown");
#else
    return allocate_solix_chars(vm, "Unknown");
#endif
}

static uint64_t env_native_is_windows(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm; (void)self; (void)args; (void)argc;
#if defined(_WIN32)
    return 1ULL;
#else
    return 0ULL;
#endif
}

static uint64_t env_native_is_linux(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm; (void)self; (void)args; (void)argc;
#if defined(__linux__)
    return 1ULL;
#else
    return 0ULL;
#endif
}

static uint64_t env_native_is_macos(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm; (void)self; (void)args; (void)argc;
#if defined(__APPLE__)
    return 1ULL;
#else
    return 0ULL;
#endif
}

static uint64_t env_native_processor_count(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm; (void)self; (void)args; (void)argc;
    unsigned int c = std::thread::hardware_concurrency();
    return c > 0 ? static_cast<uint64_t>(c) : 1ULL;
}

// --- Process Management ---

struct ProcessInfo {
#if defined(_WIN32)
    HANDLE hProcess = NULL;
    HANDLE hStdOutRead = NULL;
    HANDLE hStdErrRead = NULL;
#else
    pid_t pid = -1;
    int stdout_fd = -1;
    int stderr_fd = -1;
#endif
    std::string stdout_content;
    std::string stderr_content;
    int32_t exit_code = -1;
    bool has_waited = false;
};

static std::unordered_map<int64_t, ProcessInfo> g_processes;
static int64_t g_next_process_handle = 1;
static std::mutex g_proc_mutex;

static uint64_t process_native_start(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string cmd = read_solix_chars(vm, args[0]);
    std::string args_tsv = read_solix_chars(vm, args[1]);

    std::vector<std::string> arg_list;
    arg_list.push_back(cmd);
    if (!args_tsv.empty()) {
        std::stringstream ss(args_tsv);
        std::string item;
        while (std::getline(ss, item, '\t')) {
            arg_list.push_back(item);
        }
    }

    std::lock_guard<std::mutex> lock(g_proc_mutex);
    int64_t handle = g_next_process_handle++;
    ProcessInfo info;

#if defined(_WIN32)
    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    HANDLE hChildStdOutWr = NULL;
    HANDLE hChildStdErrWr = NULL;

    CreatePipe(&info.hStdOutRead, &hChildStdOutWr, &saAttr, 0);
    SetHandleInformation(info.hStdOutRead, HANDLE_FLAG_INHERIT, 0);

    CreatePipe(&info.hStdErrRead, &hChildStdErrWr, &saAttr, 0);
    SetHandleInformation(info.hStdErrRead, HANDLE_FLAG_INHERIT, 0);

    std::string cmd_line;
    for (size_t i = 0; i < arg_list.size(); ++i) {
        if (i > 0) cmd_line += " ";
        cmd_line += "\"" + arg_list[i] + "\"";
    }

    STARTUPINFOA siStartInfo;
    ZeroMemory(&siStartInfo, sizeof(STARTUPINFOA));
    siStartInfo.cb = sizeof(STARTUPINFOA);
    siStartInfo.hStdError = hChildStdErrWr;
    siStartInfo.hStdOutput = hChildStdOutWr;
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

    PROCESS_INFORMATION piProcInfo;
    ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

    BOOL success = CreateProcessA(NULL, const_cast<char*>(cmd_line.c_str()), NULL, NULL, TRUE, 0, NULL, NULL, &siStartInfo, &piProcInfo);
    CloseHandle(hChildStdOutWr);
    CloseHandle(hChildStdErrWr);

    if (success) {
        info.hProcess = piProcInfo.hProcess;
        CloseHandle(piProcInfo.hThread);
    } else {
        info.exit_code = 127;
        info.has_waited = true;
    }
#else
    int out_pipe[2];
    int err_pipe[2];
    if (pipe(out_pipe) < 0 || pipe(err_pipe) < 0) {
        info.exit_code = -1;
        info.has_waited = true;
        g_processes[handle] = info;
        return static_cast<uint64_t>(handle);
    }

    pid_t pid = fork();
    if (pid == 0) {
        // Child
        close(out_pipe[0]);
        close(err_pipe[0]);
        dup2(out_pipe[1], STDOUT_FILENO);
        dup2(err_pipe[1], STDERR_FILENO);
        close(out_pipe[1]);
        close(err_pipe[1]);

        std::vector<char*> c_argv;
        for (auto &a : arg_list) {
            c_argv.push_back(const_cast<char*>(a.c_str()));
        }
        c_argv.push_back(nullptr);

        execvp(c_argv[0], c_argv.data());
        _exit(127); // Command not found or exec failure
    } else if (pid > 0) {
        // Parent
        close(out_pipe[1]);
        close(err_pipe[1]);
        info.pid = pid;
        info.stdout_fd = out_pipe[0];
        info.stderr_fd = err_pipe[0];
    } else {
        close(out_pipe[0]); close(out_pipe[1]);
        close(err_pipe[0]); close(err_pipe[1]);
        info.exit_code = -1;
        info.has_waited = true;
    }
#endif

    g_processes[handle] = info;
    return static_cast<uint64_t>(handle);
}

static void read_process_output_internal(ProcessInfo &info) {
#if defined(_WIN32)
    if (info.hStdOutRead != NULL) {
        DWORD bytesRead;
        CHAR buffer[4096];
        while (ReadFile(info.hStdOutRead, buffer, sizeof(buffer), &bytesRead, NULL) && bytesRead > 0) {
            info.stdout_content.append(buffer, bytesRead);
        }
        CloseHandle(info.hStdOutRead);
        info.hStdOutRead = NULL;
    }
    if (info.hStdErrRead != NULL) {
        DWORD bytesRead;
        CHAR buffer[4096];
        while (ReadFile(info.hStdErrRead, buffer, sizeof(buffer), &bytesRead, NULL) && bytesRead > 0) {
            info.stderr_content.append(buffer, bytesRead);
        }
        CloseHandle(info.hStdErrRead);
        info.hStdErrRead = NULL;
    }
#else
    if (info.stdout_fd >= 0) {
        char buffer[4096];
        ssize_t bytes;
        while ((bytes = read(info.stdout_fd, buffer, sizeof(buffer))) > 0) {
            info.stdout_content.append(buffer, bytes);
        }
        close(info.stdout_fd);
        info.stdout_fd = -1;
    }
    if (info.stderr_fd >= 0) {
        char buffer[4096];
        ssize_t bytes;
        while ((bytes = read(info.stderr_fd, buffer, sizeof(buffer))) > 0) {
            info.stderr_content.append(buffer, bytes);
        }
        close(info.stderr_fd);
        info.stderr_fd = -1;
    }
#endif
}

static uint64_t process_native_wait_for_exit(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm; (void)self; (void)argc;
    int64_t handle = static_cast<int64_t>(args[0]);

    std::lock_guard<std::mutex> lock(g_proc_mutex);
    auto it = g_processes.find(handle);
    if (it == g_processes.end()) {
        return static_cast<uint64_t>(-1);
    }

    ProcessInfo &info = it->second;
    if (info.has_waited) {
        return static_cast<uint64_t>(info.exit_code);
    }

    read_process_output_internal(info);

#if defined(_WIN32)
    if (info.hProcess != NULL) {
        WaitForSingleObject(info.hProcess, INFINITE);
        DWORD exitCode = 0;
        if (GetExitCodeProcess(info.hProcess, &exitCode)) {
            info.exit_code = static_cast<int32_t>(exitCode);
        }
        CloseHandle(info.hProcess);
        info.hProcess = NULL;
    }
#else
    if (info.pid > 0) {
        int status = 0;
        waitpid(info.pid, &status, 0);
        if (WIFEXITED(status)) {
            info.exit_code = WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            info.exit_code = 128 + WTERMSIG(status);
        } else {
            info.exit_code = -1;
        }
        info.pid = -1;
    }
#endif

    info.has_waited = true;
    return static_cast<uint64_t>(info.exit_code);
}

static uint64_t process_native_get_stdout(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int64_t handle = static_cast<int64_t>(args[0]);

    std::lock_guard<std::mutex> lock(g_proc_mutex);
    auto it = g_processes.find(handle);
    if (it == g_processes.end()) {
        return allocate_solix_chars(vm, "");
    }
    return allocate_solix_chars(vm, it->second.stdout_content);
}

static uint64_t process_native_get_stderr(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int64_t handle = static_cast<int64_t>(args[0]);

    std::lock_guard<std::mutex> lock(g_proc_mutex);
    auto it = g_processes.find(handle);
    if (it == g_processes.end()) {
        return allocate_solix_chars(vm, "");
    }
    return allocate_solix_chars(vm, it->second.stderr_content);
}

static uint64_t process_native_kill(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm; (void)self; (void)argc;
    int64_t handle = static_cast<int64_t>(args[0]);

    std::lock_guard<std::mutex> lock(g_proc_mutex);
    auto it = g_processes.find(handle);
    if (it == g_processes.end()) {
        return 0ULL;
    }

    ProcessInfo &info = it->second;
    if (info.has_waited) {
        return 0ULL;
    }

#if defined(_WIN32)
    if (info.hProcess != NULL) {
        TerminateProcess(info.hProcess, 1);
    }
#else
    if (info.pid > 0) {
        kill(info.pid, SIGKILL);
    }
#endif
    return 0ULL;
}

} // namespace

void register_system_natives(solix::NativeRegistry &registry) {
    auto reg = [&](const std::string &cls, const std::string &name, solix::NativeFunctionPtr func) {
        registry.register_function(cls + "_" + name, func);
        registry.register_function("solix_system_" + cls + "_" + name, func);
        registry.register_function("solix.system." + cls + "." + name, func);
    };

    // Environment
    reg("Environment", "native_get_env", env_native_get_env);
    reg("Environment", "native_set_env", env_native_set_env);
    reg("Environment", "native_get_all_env", env_native_get_all_env);
    reg("Environment", "native_get_args", env_native_get_args);
    reg("Environment", "native_os_name", env_native_os_name);
    reg("Environment", "native_os_version", env_native_os_version);
    reg("Environment", "native_is_windows", env_native_is_windows);
    reg("Environment", "native_is_linux", env_native_is_linux);
    reg("Environment", "native_is_macos", env_native_is_macos);
    reg("Environment", "native_processor_count", env_native_processor_count);

    // Process
    reg("Process", "native_start", process_native_start);
    reg("Process", "native_wait_for_exit", process_native_wait_for_exit);
    reg("Process", "native_get_stdout", process_native_get_stdout);
    reg("Process", "native_get_stderr", process_native_get_stderr);
    reg("Process", "native_kill", process_native_kill);
}
