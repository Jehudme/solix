#include "solixlib_test_helper.hpp"

using namespace solix::test;

TEST_CASE("Standard Library - solix.io.Streams", "[io][streams][solixlib]") {
    SECTION("Case 13.1: Path object-oriented operations and division operator") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.io.filesystem.Path;
            import solix.core.String;

            class Main {
                public static int32 main() {
                    Path p1 = new Path(new String("root"));
                    Path p2 = new Path(new String("sub"));

                    Path combined = p1 / p2;
                    if (combined.is_empty()) return 1;

                    Path combined_str = p1 / new String("file.txt");
                    if (combined_str.is_empty()) return 2;

                    Path fn = combined_str.filename();
                    if (!fn.to_string().equals(new String("file.txt"))) return 3;

                    Path p_copy = new Path(p1.to_string());
                    if (p1 != p_copy) return 4;
                    if (!(p1 == p_copy)) return 5;

                    return 0;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 13.2: MemoryStream reading, writing, seeking, position, length, and to_array") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.io.MemoryStream;
            import solix.io.IStream;

            class Main {
                public static int32 main() {
                    MemoryStream ms = new MemoryStream();
                    if (ms.length() != 0L) return 1;
                    if (ms.position() != 0L) return 2;

                    ms.write_byte(65);
                    ms.write_byte(66);
                    ms.write_byte(67);

                    if (ms.length() != 3L) return 3;
                    if (ms.position() != 3L) return 4;

                    ms.seek(0L, 0); // Seek to begin
                    if (ms.position() != 0L) return 5;

                    if (ms.read_byte() != 65) return 6;
                    if (ms.read_byte() != 66) return 7;
                    if (ms.read_byte() != 67) return 8;
                    if (ms.read_byte() != -1) return 9; // EOF

                    ms.seek(1L, 0);
                    if (ms.read_byte() != 66) return 10;

                    ms.close();
                    return 0;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 13.3: FileStream reading, writing, seeking, position, and lifecycle") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.io.FileStream;
            import solix.io.FileMode;
            import solix.io.filesystem.Path;
            import solix.io.filesystem.File;
            import solix.core.String;

            class Main {
                public static int32 main() {
                    String temp_dir = Path.get_temp_path();
                    String file_path = Path.combine(temp_dir, new String("solix_filestream_test.bin"));

                    if (File.exists(file_path)) {
                        File.delete(file_path);
                    }

                    FileStream fs_write = new FileStream(file_path, FileMode.WRITE);
                    fs_write.write_byte(42);
                    fs_write.write_byte(99);
                    fs_write.flush();
                    fs_write.close();

                    if (!File.exists(file_path)) return 1;

                    FileStream fs_read = new FileStream(file_path, FileMode.READ);
                    if (fs_read.length() != 2L) return 2;
                    if (fs_read.read_byte() != 42) return 3;
                    if (fs_read.read_byte() != 99) return 4;
                    if (fs_read.read_byte() != -1) return 5;
                    fs_read.close();

                    File.delete(file_path);
                    return 0;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 13.4: BufferedReader line-by-line reading without OOM memory hazards") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.io.FileStream;
            import solix.io.FileMode;
            import solix.io.BufferedReader;
            import solix.io.filesystem.Path;
            import solix.io.filesystem.File;
            import solix.core.String;

            class Main {
                public static int32 main() {
                    String temp_dir = Path.get_temp_path();
                    String file_path = Path.combine(temp_dir, new String("solix_buffered_reader_test.txt"));

                    File.write_all_text(file_path, new String("alpha\nbeta\ngamma\n"));

                    FileStream fs = new FileStream(file_path, FileMode.READ);
                    BufferedReader reader = new BufferedReader(fs);

                    String l1 = reader.read_line();
                    if (l1 == null || !l1.equals(new String("alpha"))) return 1;

                    String l2 = reader.read_line();
                    if (l2 == null || !l2.equals(new String("beta"))) return 2;

                    String l3 = reader.read_line();
                    if (l3 == null || !l3.equals(new String("gamma"))) return 3;

                    String l4 = reader.read_line();
                    if (l4 != null) return 4;

                    reader.close();
                    File.delete(file_path);
                    return 0;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 13.5: StreamReader and StreamWriter sequential text reading and writing") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.io.FileStream;
            import solix.io.FileMode;
            import solix.io.StreamReader;
            import solix.io.StreamWriter;
            import solix.io.filesystem.Path;
            import solix.io.filesystem.File;
            import solix.core.String;

            class Main {
                public static int32 main() {
                    String temp_dir = Path.get_temp_path();
                    String file_path = Path.combine(temp_dir, new String("solix_stream_rw_test.txt"));

                    FileStream fs_w = new FileStream(file_path, FileMode.WRITE);
                    StreamWriter sw = new StreamWriter(fs_w);
                    sw.write_line(new String("Hello Solix"));
                    sw.write_line(new String("Streaming IO"));
                    sw.flush();
                    sw.close();

                    FileStream fs_r = new FileStream(file_path, FileMode.READ);
                    StreamReader sr = new StreamReader(fs_r);
                    String l1 = sr.read_line();
                    if (l1 == null || !l1.equals(new String("Hello Solix"))) return 1;

                    String l2 = sr.read_line();
                    if (l2 == null || !l2.equals(new String("Streaming IO"))) return 2;

                    sr.close();
                    File.delete(file_path);
                    return 0;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }
}
