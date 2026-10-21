# Solix Standard Library: System Environment & Processes (`solix.system`)

## Overview

The `solix.system` module provides cross-platform abstractions for querying and mutating the operating system environment, inspecting host platform hardware, and orchestrating child process lifecycle execution.

---

## 1. Class: `solix.system.Environment`

The static class for environment variable management and host platform queries.

```solix
package solix.system;

public class Environment {
    public static String get_env(String key);
    public static void set_env(String key, String value);
    public static HashMap<String, String> get_all_env();
    public static List<String> get_args();
    public static String os_name();
    public static String os_version();
    public static bool is_windows();
    public static bool is_linux();
    public static bool is_macos();
    public static int32 processor_count();
}
```

### Methods

- `String get_env(String key)`: Retrieves the value of the environment variable specified by `key`. Returns `null` if the variable does not exist.
- `void set_env(String key, String value)`: Creates or updates the environment variable specified by `key` to `value`.
- `HashMap<String, String> get_all_env()`: Returns all environment variables defined in the current process space as key-value pairs.
- `List<String> get_args()`: Retrieves command-line arguments passed to the running Solix application.
- `String os_name()`: Returns the operating system friendly name (e.g., `"Linux"`, `"Windows"`, `"macOS"`).
- `String os_version()`: Returns the host operating system kernel / release version string.
- `bool is_windows()`: Returns `true` if executing on Microsoft Windows.
- `bool is_linux()`: Returns `true` if executing on a Linux kernel.
- `bool is_macos()`: Returns `true` if executing on Apple macOS / Darwin.
- `int32 processor_count()`: Returns the number of logical processor cores available on the system.

---

## 2. Class: `solix.system.ProcessResult`

Data transfer object containing execution outcomes of a terminated process.

```solix
package solix.system;

public class ProcessResult {
    public int32 exit_code;
    public String standard_output;
    public String standard_error;

    public ProcessResult(int32 exit_code, String stdout_str, String stderr_str);
}
```

### Fields

- `int32 exit_code`: The termination status code returned by the child process (0 indicates success).
- `String standard_output`: Captured textual standard output produced by the process.
- `String standard_error`: Captured textual standard error output produced by the process.

---

## 3. Class: `solix.system.Process`

Child process manager supporting both synchronous execution and lifecycle management.

```solix
package solix.system;

public class Process {
    public Process(String command, List<String> args);
    public void start();
    public int32 wait_for_exit();
    public void kill();
    public bool has_exited();
    public int32 exit_code();
    public String get_standard_output();
    public String get_standard_error();

    public static ProcessResult run(String command, List<String> args);
}
```

### Methods

- `Process(String command, List<String> args)`: Initializes a process configuration with command path and argument list.
- `void start()`: Asynchronously spawns the child process and attaches I/O pipes.
- `int32 wait_for_exit()`: Blocks until the spawned process terminates and returns its exit code.
- `void kill()`: Forces termination of the active child process.
- `bool has_exited()`: Returns `true` if the child process has completed execution.
- `int32 exit_code()`: Returns the exit code if terminated; returns `-1` if still active.
- `String get_standard_output()`: Returns captured standard output content.
- `String get_standard_error()`: Returns captured standard error content.
- `static ProcessResult run(String command, List<String> args)`: Synchronously executes the specified command and returns a `ProcessResult` with exit code, stdout, and stderr.
