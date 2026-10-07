# Solix Standard Library: Cryptography & Encodings (`solix.crypto`)

## Overview

The `solix.crypto` module provides binary encoding formats (Base64, Hexadecimal) and cryptographic hash algorithms (SHA-256, SHA-1, MD5) for data validation, digests, integrity verification, and binary serialization.

---

## 1. Class: `solix.crypto.Base64`

RFC 4648 standard Base64 encoding and decoding for byte buffers and textual strings.

```solix
package solix.crypto;

public class Base64 {
    public static String encode(int8[] data);
    public static String encode_string(String text);
    public static int8[] decode(String base64);
    public static String decode_to_string(String base64);
}
```

### Methods

- `String encode(int8[] data)`: Encodes a byte array into standard Base64 representation with standard padding (`=`). Returns an empty string if `data` is `null` or empty.
- `String encode_string(String text)`: Converts `text` into UTF-8 bytes and encodes to Base64.
- `int8[] decode(String base64)`: Decodes a Base64 string back into a byte array. Throws `FormatException` if the input is malformed, has invalid length, or contains illegal characters.
- `String decode_to_string(String base64)`: Decodes a Base64 string directly into a Solix `String`. Throws `FormatException` on invalid format.

---

## 2. Class: `solix.crypto.Hex`

Hexadecimal (base16) lower-case encoding and decoding.

```solix
package solix.crypto;

public class Hex {
    public static String encode(int8[] data);
    public static int8[] decode(String hex_string);
}
```

### Methods

- `String encode(int8[] data)`: Encodes a byte array into lowercase hexadecimal characters.
- `int8[] decode(String hex_string)`: Decodes an even-length hexadecimal string into a byte array. Throws `FormatException` if the string length is odd or contains characters outside `[0-9a-fA-F]`.

---

## 3. Class: `solix.crypto.Hash`

Cryptographic digest computation implementing standard FIPS 180-4 SHA-256, FIPS 180-1 SHA-1, and RFC 1321 MD5 algorithms.

```solix
package solix.crypto;

public class Hash {
    public static int8[] sha256(int8[] data);
    public static String sha256_hex(String text);
    public static int8[] sha1(int8[] data);
    public static String sha1_hex(String text);
    public static int8[] md5(int8[] data);
    public static String md5_hex(String text);
}
```

### Methods

- `int8[] sha256(int8[] data)`: Computes 32-byte (256-bit) SHA-256 binary digest.
- `String sha256_hex(String text)`: Computes SHA-256 of string content and returns 64-character lowercase hex string.
- `int8[] sha1(int8[] data)`: Computes 20-byte (160-bit) SHA-1 binary digest.
- `String sha1_hex(String text)`: Computes SHA-1 of string content and returns 40-character lowercase hex string.
- `int8[] md5(int8[] data)`: Computes 16-byte (128-bit) MD5 binary digest.
- `String md5_hex(String text)`: Computes MD5 of string content and returns 32-character lowercase hex string.
