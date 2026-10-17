# Solix Standard Library: Application Resources & Dual-Location Filesystem (`solix.io.filesystem.Resource`)

## Overview

The `solix.io.filesystem.Resource` class and accompanying dual-location filesystem APIs provide first-class access to packaged application resources (`resources/` directory within `.slxpac` application bundles or local project development root).

Solix programs can selectively read from either:
1. **Application Resources**: Internal assets bundled inside the `.slxpac` package (e.g., config templates, UI shaders, default databases, localization catalogs).
2. **Host Filesystem**: External files and directories located on the host operating system.

---

## 1. Class: `solix.io.filesystem.Resource`

Static utilities for inspecting, locating, and reading bundled application resources.

### Static Methods

- `static String get_resource_directory()`:
  Returns the absolute path to the active application resource root. When executing from a `.slxpac` bundle, this points to the unpacked cache directory for bundled resources. In development mode (`solix run .`), it falls back to `<project_root>/resources`.
- `static String get_path(String rel_path)`:
  Resolves a relative resource path against the application resource root, returning a normalized absolute path string. If `rel_path` is `null`, returns `get_resource_directory()`.
- `static bool exists(String rel_path)`:
  Checks whether the specified resource exists and is a regular file within the application resources directory.
- `static String read_all_text(String rel_path)`:
  Reads all text from the specified resource. Throws `FileNotFoundException` if the resource does not exist, or `IOException` if reading fails.
- `static List<String> read_all_lines(String rel_path)`:
  Reads all lines from the specified resource, returning them as a `List<String>`.

---

## 2. Dual-Location Overloads in `solix.io.filesystem.File`

`File` includes overloaded methods and dedicated helpers to select between host filesystem and resource locations:

- `static bool exists(String path, bool is_resource)`:
  When `is_resource` is `true`, checks existence in application resources (`Resource.exists(path)`). Otherwise, checks the host filesystem.
- `static String read_all_text(String path, bool is_resource)`:
  When `is_resource` is `true`, reads from application resources (`Resource.read_all_text(path)`). Otherwise, reads from host filesystem.
- `static List<String> read_all_lines(String path, bool is_resource)`:
  When `is_resource` is `true`, reads lines from application resources. Otherwise, reads from host filesystem.
- `static bool resource_exists(String rel_path)`:
  Convenience alias for `Resource.exists(rel_path)`.
- `static String read_resource_text(String rel_path)`:
  Convenience alias for `Resource.read_all_text(rel_path)`.
- `static List<String> read_resource_lines(String rel_path)`:
  Convenience alias for `Resource.read_all_lines(rel_path)`.

---

## 3. Virtual URI Scheme: `res://`

All file reading and inspection functions (`File.exists`, `File.read_all_text`, `File.get_size`, `File.get_last_modified_time`) transparently support the `res://` virtual URI scheme.

When a path begins with `res://`:
- The `res://` prefix is stripped.
- The path is resolved relative to the application resource root directory.

### Example

```solix
import solix.io.filesystem.File;
import solix.io.filesystem.Path;
import solix.io.filesystem.Resource;
import solix.system.Console;

class App {
    public static int32 main() {
        // Direct Resource API
        if (Resource.exists(new String("config.json"))) {
            String cfg = Resource.read_all_text(new String("config.json"));
            Console.println(cfg);
        }

        // Virtual URI scheme via standard File API
        String banner = File.read_all_text(new String("res://assets/banner.txt"));
        Console.println(banner);

        // Path resource inspection
        Path res_path = new Path("res://shaders/vertex.glsl");
        if (res_path.is_resource()) {
            Console.println(res_path.to_string());
        }

        return 0;
    }
}
```
