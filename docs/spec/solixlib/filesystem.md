# Solix Standard Library: Filesystem (`solix.io.filesystem`)

## Overview

The `solix.io.filesystem` module provides cross-platform filesystem paths, file input/output, and directory operations:
1. `Path`: Cross-platform path string manipulation, delimiter normalization, extraction of directories, file names, and extensions.
2. `File`: Static helpers for synchronous file I/O, text reading/writing, line-by-line streaming into collections, copying, moving, and deleting files.
3. `Directory`: Directory management, hierarchical path creation, enumeration of directory contents (files and subdirectories), and recursive/non-recursive deletion.

---

## 1. Class: `solix.io.filesystem.Path`

Static path manipulation utilities. Normalizes path separators and provides cross-platform path resolution.

### Static Methods

- `static String combine(String path1, String path2)`: Concatenates two path segments with a platform-appropriate directory separator.
- `static String get_directory_name(String path)`: Returns the parent directory portion of the path.
- `static String get_file_name(String path)`: Returns the file name and extension of the specified path string.
- `static String get_extension(String path)`: Returns the file extension of the specified path (including the leading dot).
- `static String get_file_name_without_extension(String path)`: Returns the file name of the specified path string without its extension.
- `static bool is_absolute(String path)`: Determines whether the specified path string contains an absolute or relative path reference.
- `static String get_temp_path()`: Returns the absolute path of the current system's temporary directory.
- `static String normalize(String path)`: Resolves redundant separators, `.` and `..` components to generate a canonical path representation.

---

## 2. Class: `solix.io.filesystem.File`

Static file system utilities for reading, writing, and managing files.

### Static Methods

- `static bool exists(String path)`: Checks whether the specified file exists and is a regular file.
- `static String read_all_text(String path)`: Opens a text file, reads all contents into a `String`, and closes the file. Throws `FileNotFoundException` if the file does not exist, or `IOException` on read failure.
- `static void write_all_text(String path, String content)`: Creates a new file, writes the specified string to the file, and closes the file. Overwrites existing files. Throws `IOException` on failure.
- `static void append_all_text(String path, String content)`: Opens a file, appends the specified string to the end of the file, and closes the file. Creates the file if it does not exist. Throws `IOException` on failure.
- `static List read_all_lines(String path)`: Reads all lines from the specified file and returns them as a `List` of `String` entries.
- `static void delete(String path)`: Deletes the specified file. Throws `FileNotFoundException` if the file does not exist, or `IOException` on deletion failure.
- `static void copy(String source, String destination)`: Copies an existing file to a new file location. Defaults to not overwriting. Throws `FileNotFoundException` or `IOException`.
- `static void copy_with_overwrite(String source, String destination, bool overwrite)`: Copies an existing file to a new location with optional overwrite semantics.
- `static void move(String source, String destination)`: Moves a specified file to a new location, with optional renaming. Throws `FileNotFoundException` or `IOException`.
- `static int64 get_size(String path)`: Returns the size of the specified file in bytes. Throws `FileNotFoundException` or `IOException`.
- `static int64 get_last_modified_time(String path)`: Returns the epoch timestamp in milliseconds when the file was last modified. Throws `FileNotFoundException` or `IOException`.

---

## 3. Class: `solix.io.filesystem.Directory`

Static directory utilities for directory creation, enumeration, and deletion.

### Static Methods

- `static bool exists(String path)`: Checks whether the specified directory exists.
- `static void create_directory(String path)`: Creates all directories and subdirectories in the specified path unless they already exist. Throws `IOException` on failure.
- `static void delete(String path)`: Deletes the specified empty directory. Throws `DirectoryNotFoundException` if the directory does not exist, or `IOException` if the directory is not empty.
- `static void delete_with_recursive(String path, bool recursive)`: Deletes the specified directory and, if `recursive` is `true`, all subdirectories and files in the path.
- `static List list_files(String path)`: Returns a `List` containing the names of all regular files located directly inside the specified directory. Throws `DirectoryNotFoundException` or `IOException`.
- `static List list_directories(String path)`: Returns a `List` containing the names of all subdirectories located directly inside the specified directory. Throws `DirectoryNotFoundException` or `IOException`.
- `static String get_current_directory()`: Returns the absolute path of the current working directory of the application.
- `static void set_current_directory(String path)`: Sets the application's current working directory to the specified path. Throws `IOException` on failure.

---

## Error Handling & Exceptions

The filesystem APIs integrate with `solix.exceptions`:
- `FileNotFoundException`: Thrown when a file operation targets a path that does not exist.
- `DirectoryNotFoundException`: Thrown when a directory operation targets a path that does not exist.
- `IOException`: Thrown on I/O read/write errors, permission errors, or attempt to delete a non-empty directory non-recursively.
- `ArgumentNullException`: Thrown if a null path or argument is supplied.
