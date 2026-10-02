#include "solixlib_test_helper.hpp"

using namespace solix::test;

TEST_CASE("Standard Library - solix.io.filesystem", "[solixlib][io][filesystem]") {
    SECTION("Case 12.1: Path manipulation") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.io.filesystem.Path;
            import solix.core.String;

            class Main {
                public static int32 main() {
                    String p1 = new String("foo");
                    String p2 = new String("bar");
                    String combined = Path.combine(p1, p2);
                    if (!combined.equals_chars("foo/bar") && !combined.equals_chars("foo\\bar")) return 1;

                    String full_path = new String("a/b/c.txt");
                    String dir = Path.get_directory_name(full_path);
                    if (!dir.equals_chars("a/b")) return 2;

                    String fn = Path.get_file_name(full_path);
                    if (!fn.equals_chars("c.txt")) return 3;

                    String ext = Path.get_extension(full_path);
                    if (!ext.equals_chars(".txt")) return 4;

                    String stem = Path.get_file_name_without_extension(full_path);
                    if (!stem.equals_chars("c")) return 5;

                    String abs_path = new String("/usr/bin");
                    if (!Path.is_absolute(abs_path)) return 6;

                    String rel_path = new String("src/main.slx");
                    if (Path.is_absolute(rel_path)) return 7;

                    String temp_path = Path.get_temp_path();
                    if (temp_path.length() == 0) return 8;

                    String dirty = new String("a/b/../c");
                    String normalized = Path.normalize(dirty);
                    if (!normalized.equals_chars("a/c")) return 9;

                    return 0;
                }
            }
        )";

        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 12.2: File text I/O and size retrieval") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.io.filesystem.Path;
            import solix.io.filesystem.File;
            import solix.core.String;

            class Main {
                public static int32 main() {
                    String temp_dir = Path.get_temp_path();
                    String file_name = new String("solix_test_file_12_2.txt");
                    String path = Path.combine(temp_dir, file_name);

                    if (File.exists(path)) {
                        File.delete(path);
                    }

                    if (File.exists(path)) return 1;

                    String content = new String("Hello Solix Filesystem!");
                    File.write_all_text(path, content);

                    if (!File.exists(path)) return 2;

                    String read_back = File.read_all_text(path);
                    if (!read_back.equals(content)) return 3;

                    int64 sz = File.get_size(path);
                    if (sz != 23L) return 4;

                    String append_content = new String(" Appended.");
                    File.append_all_text(path, append_content);

                    String full_content = File.read_all_text(path);
                    if (!full_content.equals_chars("Hello Solix Filesystem! Appended.")) return 5;

                    File.delete(path);
                    if (File.exists(path)) return 6;

                    return 0;
                }
            }
        )";

        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 12.3: File line reading") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.io.filesystem.Path;
            import solix.io.filesystem.File;
            import solix.core.String;
            import solix.collections.List;

            class Main {
                public static int32 main() {
                    String temp_dir = Path.get_temp_path();
                    String file_name = new String("solix_test_lines_12_3.txt");
                    String path = Path.combine(temp_dir, file_name);

                    if (File.exists(path)) {
                        File.delete(path);
                    }

                    String text = new String("line1\nline2\nline3");
                    File.write_all_text(path, text);

                    List<String> lines = File.read_all_lines(path);
                    if (lines.size() != 3) return 1;

                    String l1 = lines.get(0);
                    String l2 = lines.get(1);
                    String l3 = lines.get(2);

                    if (!l1.equals_chars("line1")) return 2;
                    if (!l2.equals_chars("line2")) return 3;
                    if (!l3.equals_chars("line3")) return 4;

                    File.delete(path);
                    return 0;
                }
            }
        )";

        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 12.4: File copy and move") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.io.filesystem.Path;
            import solix.io.filesystem.File;
            import solix.core.String;

            class Main {
                public static int32 main() {
                    String temp_dir = Path.get_temp_path();
                    String src_name = new String("solix_test_src_12_4.txt");
                    String dst_name = new String("solix_test_dst_12_4.txt");
                    String mv_name = new String("solix_test_mv_12_4.txt");

                    String src = Path.combine(temp_dir, src_name);
                    String dst = Path.combine(temp_dir, dst_name);
                    String mv = Path.combine(temp_dir, mv_name);

                    if (File.exists(src)) File.delete(src);
                    if (File.exists(dst)) File.delete(dst);
                    if (File.exists(mv)) File.delete(mv);

                    String text = new String("Copy and Move Payload");
                    File.write_all_text(src, text);

                    File.copy(src, dst);
                    if (!File.exists(src) || !File.exists(dst)) return 1;

                    File.move(dst, mv);
                    if (File.exists(dst)) return 2;
                    if (!File.exists(mv)) return 3;

                    String mv_content = File.read_all_text(mv);
                    if (!mv_content.equals(text)) return 4;

                    File.delete(src);
                    File.delete(mv);
                    return 0;
                }
            }
        )";

        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 12.5: Directory operations and recursive deletion") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.io.filesystem.Path;
            import solix.io.filesystem.File;
            import solix.io.filesystem.Directory;
            import solix.core.String;
            import solix.collections.List;
            import solix.system.Console;
            import solix.exceptions.Exception;

            class Main {
                public static int32 main() {
                    String temp_dir = Path.get_temp_path();
                    String base_name = new String("solix_test_dir_12_5");
                    String base_dir = Path.combine(temp_dir, base_name);

                    if (Directory.exists(base_dir)) {
                        Directory.delete_with_recursive(base_dir, true);
                    }

                    String sub_name = new String("subdir");
                    String sub_dir = Path.combine(base_dir, sub_name);
                    Directory.create_directory(sub_dir);

                    if (!Directory.exists(sub_dir)) return 1;

                    String f1_name = new String("sample.txt");
                    String f1_path = Path.combine(sub_dir, f1_name);
                    String hello = new String("hello");
                    File.write_all_text(f1_path, hello);

                    List<String> files = Directory.list_files(sub_dir);
                    if (files.size() != 1) return 2;
                    String found_file = files.get(0);
                    if (!found_file.equals(f1_name)) return 3;

                    List<String> dirs = Directory.list_directories(base_dir);
                    if (dirs.size() != 1) return 4;
                    String found_dir = dirs.get(0);
                    if (!found_dir.equals(sub_name)) return 5;

                    Directory.delete_with_recursive(base_dir, true);
                    if (Directory.exists(base_dir)) return 6;

                    return 0;
                }
            }
        )";

        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 12.6: Negative: File.read_all_text on non-existent file") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.io.filesystem.File;
            import solix.core.String;
            import solix.exceptions.FileNotFoundException;

            class Main {
                public static int32 main() {
                    String non_existent = new String("/non/existent/path/file_12_6.txt");
                    bool caught = false;
                    try {
                        File.read_all_text(non_existent);
                    } catch (FileNotFoundException ex) {
                        caught = true;
                    }
                    return caught ? 0 : 1;
                }
            }
        )";

        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 12.7: Negative: File.delete on non-existent file") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.io.filesystem.File;
            import solix.core.String;
            import solix.exceptions.FileNotFoundException;

            class Main {
                public static int32 main() {
                    String non_existent = new String("/non/existent/path/file_12_7.txt");
                    bool caught = false;
                    try {
                        File.delete(non_existent);
                    } catch (FileNotFoundException ex) {
                        caught = true;
                    }
                    return caught ? 0 : 1;
                }
            }
        )";

        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 12.8: Negative: Directory.delete on non-empty directory without recursive flag") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.io.filesystem.Path;
            import solix.io.filesystem.File;
            import solix.io.filesystem.Directory;
            import solix.core.String;
            import solix.exceptions.IOException;

            class Main {
                public static int32 main() {
                    String temp_dir = Path.get_temp_path();
                    String dir_name = new String("solix_test_nonempty_12_8");
                    String test_dir = Path.combine(temp_dir, dir_name);

                    if (Directory.exists(test_dir)) {
                        Directory.delete_with_recursive(test_dir, true);
                    }

                    Directory.create_directory(test_dir);
                    String fn = new String("dummy.txt");
                    String file_path = Path.combine(test_dir, fn);
                    String dummy_data = new String("data");
                    File.write_all_text(file_path, dummy_data);

                    bool caught = false;
                    try {
                        // Non-recursive delete should throw IOException because directory is not empty
                        Directory.delete(test_dir);
                    } catch (IOException ex) {
                        caught = true;
                    }

                    // Clean up
                    Directory.delete_with_recursive(test_dir, true);
                    return caught ? 0 : 1;
                }
            }
        )";

        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 12.9: Negative: Directory.list_files on non-existent directory") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.io.filesystem.Directory;
            import solix.core.String;
            import solix.exceptions.DirectoryNotFoundException;

            class Main {
                public static int32 main() {
                    String non_existent = new String("/non/existent/directory_12_9");
                    bool caught = false;
                    try {
                        Directory.list_files(non_existent);
                    } catch (DirectoryNotFoundException ex) {
                        caught = true;
                    }
                    return caught ? 0 : 1;
                }
            }
        )";

        REQUIRE(run_solixlib_sources(sources) == 0);
    }
}
