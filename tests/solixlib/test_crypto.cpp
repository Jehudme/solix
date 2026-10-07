#include "solixlib_test_helper.hpp"

using namespace solix::test;

TEST_CASE("Standard Library - solix.crypto", "[crypto][base64][hex][hash][solixlib]") {
    SECTION("Case 15.1: Base64 round-trip encoding and decoding across byte arrays and strings") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.crypto.Base64;
            import solix.core.String;

            class Main {
                public static int32 main() {
                    // String encoding / decoding
                    String text = new String("Hello, Solix Crypto!");
                    String encoded = Base64.encode_string(text);
                    if (encoded == null || encoded.length() == 0) return 1;

                    String decoded = Base64.decode_to_string(encoded);
                    if (decoded == null || !decoded.equals(text)) return 2;

                    // Byte array encoding / decoding
                    int8[] bytes = new int8[4];
                    bytes[0] = (int8)10;
                    bytes[1] = (int8)20;
                    bytes[2] = (int8)30;
                    bytes[3] = (int8)40;

                    String b64 = Base64.encode(bytes);
                    if (b64 == null || b64.length() == 0) return 3;

                    int8[] dec_bytes = Base64.decode(b64);
                    if (dec_bytes == null || dec_bytes.length != 4) return 4;
                    if (dec_bytes[0] != (int8)10 || dec_bytes[1] != (int8)20 ||
                        dec_bytes[2] != (int8)30 || dec_bytes[3] != (int8)40) return 5;

                    return 0;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 15.2: Hexadecimal round-trip encoding and decoding") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.crypto.Hex;
            import solix.core.String;

            class Main {
                public static int32 main() {
                    int8[] bytes = new int8[3];
                    bytes[0] = (int8)0xDE;
                    bytes[1] = (int8)0xAD;
                    bytes[2] = (int8)0xBE;

                    String hex = Hex.encode(bytes);
                    if (hex == null || !hex.equals(new String("deadbe"))) return 1;

                    int8[] decoded = Hex.decode(hex);
                    if (decoded == null || decoded.length != 3) return 2;
                    if (decoded[0] != (int8)0xDE || decoded[1] != (int8)0xAD || decoded[2] != (int8)0xBE) return 3;

                    return 0;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 15.3: Cryptographic digest validation: SHA-256, SHA-1, and MD5 matching NIST and RFC standard test vectors") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.crypto.Hash;
            import solix.core.String;

            class Main {
                public static int32 main() {
                    // NIST SHA-256("abc") = ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad
                    String s256 = Hash.sha256_hex(new String("abc"));
                    if (!s256.equals(new String("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"))) return 1;

                    // NIST SHA-1("abc") = a9993e364706816aba3e25717850c26c9cd0d89d
                    String s1 = Hash.sha1_hex(new String("abc"));
                    if (!s1.equals(new String("a9993e364706816aba3e25717850c26c9cd0d89d"))) return 2;

                    // RFC 1321 MD5("abc") = 900150983cd24fb0d6963f7d28e17f72
                    String m5 = Hash.md5_hex(new String("abc"));
                    if (!m5.equals(new String("900150983cd24fb0d6963f7d28e17f72"))) return 3;

                    // Raw byte digest check for SHA-256("abc")
                    int8[] data = new int8[3];
                    data[0] = (int8)97; // 'a'
                    data[1] = (int8)98; // 'b'
                    data[2] = (int8)99; // 'c'
                    int8[] digest256 = Hash.sha256(data);
                    if (digest256 == null || digest256.length != 32) return 4;
                    if (digest256[0] != (int8)0xBA || digest256[1] != (int8)0x78) return 5;

                    return 0;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 15.4: Negative: Malformed Base64 payload or padding throws FormatException") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.crypto.Base64;
            import solix.core.String;
            import solix.exceptions.FormatException;

            class Main {
                public static int32 main() {
                    bool caught = false;
                    try {
                        // Length not multiple of 4
                        Base64.decode(new String("abc"));
                    } catch (FormatException e) {
                        caught = true;
                    }
                    if (!caught) return 1;

                    caught = false;
                    try {
                        // Invalid base64 character
                        Base64.decode(new String("ab!@"));
                    } catch (FormatException e) {
                        caught = true;
                    }
                    if (!caught) return 2;

                    return 0;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 15.5: Negative: Odd-length or invalid non-hexadecimal character string throws FormatException") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.crypto.Hex;
            import solix.core.String;
            import solix.exceptions.FormatException;

            class Main {
                public static int32 main() {
                    bool caught = false;
                    try {
                        // Odd length
                        Hex.decode(new String("abc"));
                    } catch (FormatException e) {
                        caught = true;
                    }
                    if (!caught) return 1;

                    caught = false;
                    try {
                        // Invalid hex character 'z'
                        Hex.decode(new String("123z"));
                    } catch (FormatException e) {
                        caught = true;
                    }
                    if (!caught) return 2;

                    return 0;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }
}
