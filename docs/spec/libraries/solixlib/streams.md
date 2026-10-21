# Solix Standard Library: Streams & Object-Oriented Path (`solix.io`)

## Overview

The `solix.io` module provides high-performance stream abstractions for sequential and random-access binary and text I/O, preventing out-of-memory errors when processing large files. In addition, the filesystem `Path` abstraction supports an intuitive object-oriented API featuring operator overloading for cross-platform path composition.

---

## 1. Interface: `solix.io.IStream`

The foundational contract for stream-based byte I/O operations.

```solix
package solix.io;

public interface IStream {
    public int32 read(byte[] buffer, int32 offset, int32 count);
    public int32 read_byte();
    public void write(byte[] buffer, int32 offset, int32 count);
    public void write_byte(int32 b);
    public int64 seek(int64 offset, int32 origin);
    public void flush();
    public void close();
    public int64 length();
    public int64 position();
}
```

### Methods

- `int32 read(byte[] buffer, int32 offset, int32 count)`: Reads a block of bytes from the stream and writes data in a given buffer. Returns the total number of bytes read, or `-1` if end of stream is reached.
- `int32 read_byte()`: Reads a single byte from the stream, advancing position by 1. Returns `-1` if at EOF.
- `void write(byte[] buffer, int32 offset, int32 count)`: Writes a sequence of bytes to the stream.
- `void write_byte(int32 b)`: Writes a single byte to the stream at current position.
- `int64 seek(int64 offset, int32 origin)`: Sets the position within the current stream. `origin`: 0 = Begin, 1 = Current, 2 = End.
- `void flush()`: Clears all buffers and causes any buffered data to be written to the underlying device.
- `void close()`: Closes the stream and releases any system resources.
- `int64 length()`: Returns the length in bytes of the stream.
- `int64 position()`: Returns the current position within the stream.

---

## 2. Class: `solix.io.FileMode`

Enumeration constants for opening file streams.

- `public static int32 READ = 0;`: Opens existing file for reading.
- `public static int32 WRITE = 1;`: Creates or truncates file for writing.
- `public static int32 APPEND = 2;`: Opens or creates file, positioning writes at end.
- `public static int32 READ_WRITE = 3;`: Opens existing file for both reading and writing.

---

## 3. Class: `solix.io.FileStream`

File-backed stream implementation implementing `IStream`.

### Constructors
- `public FileStream(String path, int32 mode)`: Opens the file at `path` using the specified `FileMode`.

### Methods
- Implements all methods of `IStream` (`read`, `read_byte`, `write`, `write_byte`, `seek`, `flush`, `close`, `length`, `position`).

---

## 4. Class: `solix.io.MemoryStream`

In-memory resizable byte array stream implementing `IStream`.

### Constructors
- `public MemoryStream()`: Initializes a memory stream with default initial capacity (256 bytes).
- `public MemoryStream(int32 capacity)`: Initializes a memory stream with explicit initial capacity.

### Methods
- Implements all methods of `IStream`.
- `public byte[] to_array()`: Returns a newly allocated byte array containing the entire content of the stream.

---

## 5. Class: `solix.io.BufferedReader`

High-performance buffered reader designed for line-by-line reading from an underlying `IStream` without high heap allocation overhead.

### Constructors
- `public BufferedReader(IStream stream)`: Wraps an `IStream` with buffered line reader capability.

### Methods
- `public String read_line()`: Reads the next line of text, stripping CRLF or LF terminators. Returns `null` when EOF is reached.
- `public void close()`: Closes the reader and underlying stream.

---

## 6. Class: `solix.io.StreamReader` & `solix.io.StreamWriter`

Sequential character and text stream readers and writers.

### `StreamReader`
- `public StreamReader(IStream stream)`
- `public String read_line()`: Reads a line of text.
- `public String read_to_end()`: Reads all characters to end of stream.
- `public void close()`: Closes the reader and stream.

### `StreamWriter`
- `public StreamWriter(IStream stream)`
- `public void write(String s)`: Writes text to stream.
- `public void write_line(String s)`: Writes text followed by newline.
- `public void flush()`: Flushes underlying stream.
- `public void close()`: Flushes and closes underlying stream.

---

## 7. Class: `solix.io.filesystem.Path` (Object-Oriented API & Operators)

`Path` can be instantiated as an object representing an immutable path string, supporting operators for composition.

### Constructors
- `public Path()`: Default empty path.
- `public Path(String path)`: Instantiates a path wrapping the specified string.
- `public Path(char[] characters)`: Instantiates a path from character array.

### Operators
- `public Path operator/(Path other)`: Combines this path with `other` using system separator.
- `public Path operator/(String other)`: Combines this path with string `other`.
- `public bool operator==(Path other)`: Path equality comparison.
- `public bool operator!=(Path other)`: Path inequality comparison.

### Instance Methods
- `public String value()`: Returns underlying path string.
- `public String to_string()`: Implements `IStringable`.
- `public bool is_empty()`: Returns whether path is empty.
- `public Path parent()`: Returns parent directory as a `Path`.
- `public Path filename()`: Returns file name as a `Path`.
- `public String extension()`: Returns extension.
- `public String filename_without_extension()`: Returns file name without extension.
- `public bool is_absolute_path()`: Returns whether path is absolute.
- `public Path normalized()`: Returns canonicalized path.

---

## Error Handling

Streams integrate with `solix.exceptions`:
- `IOException`: Thrown on native read, write, seek, or flush errors.
- `FileNotFoundException`: Thrown when opening non-existent files for reading.
- `ArgumentException` / `IndexOutOfBoundsException`: Thrown on invalid buffers or seek offsets.
