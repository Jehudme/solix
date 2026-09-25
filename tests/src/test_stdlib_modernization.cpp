#include "solix/compilation.hpp"
#include "solix/runtime.hpp"
#include <catch2/catch_test_macros.hpp>
#include <filesystem>

using namespace solix;

static void load_stdlib_into_options(CompilationOptions &opts) {
    std::vector<std::filesystem::path> candidates = {
        "launcher/rsc/lib/solix",
        "../launcher/rsc/lib/solix",
        "../../launcher/rsc/lib/solix",
        "build/launcher/lib/solix",
        "launcher/lib/solix"
    };

    std::filesystem::path found_path;
    for (const auto &cand : candidates) {
        if (std::filesystem::exists(cand)) {
            found_path = cand;
            break;
        }
    }

    if (!found_path.empty()) {
        for (const auto &entry : std::filesystem::recursive_directory_iterator(found_path)) {
            if (entry.is_regular_file() && entry.path().extension() == ".slx") {
                if (std::filesystem::file_size(entry.path()) > 0) {
                    opts.sources[std::filesystem::canonical(entry.path())] = std::nullopt;
                }
            }
        }
    }
}

TEST_CASE("Phase 6 - Standard Library API Modernization & Import Syntax", "[phase6][stdlib_modernization]") {
    SECTION("Import syntax supports both single-symbol and wildcard imports") {
        CompilationOptions comp_opts;
        comp_opts.log_level = CompilationOptions::LogLevel::OFF;
        load_stdlib_into_options(comp_opts);

        Source src_key = std::string("test_imports.slx");
        comp_opts.sources[src_key] = R"(
            package test.pkg;

            import solix.core.String;
            import solix.core.*;

            public class CustomData {
                public int32 id = 123;
            }

            public class ImportTest {
                public static int32 main(char[][] args) {
                    String str = new String("Testing Imports");
                    CustomData d = new CustomData();
                    if (!Objects.non_null<CustomData>(d)) return 1;
                    if (Objects.is_null<CustomData>(null)) {
                        return 0;
                    }
                    return 2;
                }
            }
        )";

        Bytecode bytecode;
        REQUIRE_NOTHROW(bytecode = solix::run(comp_opts));

        RuntimeOptions run_opts;
        run_opts.bytecode_source = bytecode;
        for (const auto &[name, func] : solix::get_builtin_natives()) {
            run_opts.native_functions[name] = func;
        }
        REQUIRE(solix::run(run_opts) == 0);
    }

    SECTION("Objects utility class supports generic null checking and equality") {
        CompilationOptions comp_opts;
        comp_opts.log_level = CompilationOptions::LogLevel::OFF;
        load_stdlib_into_options(comp_opts);

        Source src_key = std::string("test_generic_objects.slx");
        comp_opts.sources[src_key] = R"(
            package test.pkg;

            import solix.core.*;

            public class Box {
                public int32 val;
                public Box(int32 v) { this.val = v; }
                public bool equals(Box other) {
                    if (other == null) return false;
                    return this.val == other.val;
                }
                public int32 hash_code() {
                    return this.val;
                }
            }

            public class ObjectsTest {
                public static int32 main(char[][] args) {
                    Box b1 = new Box(42);
                    Box b2 = new Box(42);
                    Box b3 = new Box(99);

                    // Null and non-null validation
                    if (!Objects.non_null<Box>(b1)) return 1;
                    if (Objects.is_null<Box>(b1)) return 2;
                    Box req = Objects.require_non_null<Box>(b1);
                    if (req.val != 42) return 3;

                    // Equals and hash_code
                    if (!Objects.equals<Box>(b1, b2)) return 4;
                    if (Objects.equals<Box>(b1, b3)) return 5;
                    if (Objects.hash_code<Box>(b1) != 42) return 6;

                    // Primitive hashing & equality overloads
                    if (!Objects.equals(100, 100)) return 7;
                    if (Objects.equals(100, 200)) return 8;
                    if (Objects.hash_code(555) != 555) return 9;

                    return 0;
                }
            }
        )";

        Bytecode bytecode;
        REQUIRE_NOTHROW(bytecode = solix::run(comp_opts));

        RuntimeOptions run_opts;
        run_opts.bytecode_source = bytecode;
        for (const auto &[name, func] : solix::get_builtin_natives()) {
            run_opts.native_functions[name] = func;
        }
        REQUIRE(solix::run(run_opts) == 0);
    }

    SECTION("Arrays utility class supports generic swap, reverse, copy, and typed sorting") {
        CompilationOptions comp_opts;
        comp_opts.log_level = CompilationOptions::LogLevel::OFF;
        load_stdlib_into_options(comp_opts);

        Source src_key = std::string("test_arrays_modern.slx");
        comp_opts.sources[src_key] = R"(
            package test.pkg;

            import solix.core.*;

            public class ArraysTest {
                public static int32 main(char[][] args) {
                    // 1. Generic swap and reverse
                    int32[] nums = new int32[3];
                    nums[0] = 10;
                    nums[1] = 20;
                    nums[2] = 30;

                    Arrays.swap<int32>(nums, 0, 2);
                    if (nums[0] != 30 || nums[2] != 10) return 1;

                    Arrays.reverse<int32>(nums);
                    if (nums[0] != 10 || nums[2] != 30) return 2;

                    // 2. Generic copy and equals
                    int32[] dst = new int32[3];
                    Arrays.copy<int32>(nums, 0, dst, 0, 3);
                    if (!Arrays.equals<int32>(nums, dst)) return 3;

                    // 3. Typed sort and binary search for int64
                    int64[] bigs = new int64[4];
                    bigs[0] = (int64)400;
                    bigs[1] = (int64)100;
                    bigs[2] = (int64)300;
                    bigs[3] = (int64)200;

                    Arrays.sort(bigs);
                    if (bigs[0] != (int64)100 || bigs[3] != (int64)400) return 4;
                    int32 found = Arrays.binary_search(bigs, (int64)300);
                    if (found != 2) return 5;

                    // 4. Overloaded fill
                    Arrays.fill(dst, 77);
                    if (dst[0] != 77 || dst[1] != 77 || dst[2] != 77) return 6;

                    return 0;
                }
            }
        )";

        Bytecode bytecode;
        REQUIRE_NOTHROW(bytecode = solix::run(comp_opts));

        RuntimeOptions run_opts;
        run_opts.bytecode_source = bytecode;
        for (const auto &[name, func] : solix::get_builtin_natives()) {
            run_opts.native_functions[name] = func;
        }
        REQUIRE(solix::run(run_opts) == 0);
    }

    SECTION("Bucketed HashMap and HashSet operate with collision resolution") {
        CompilationOptions comp_opts;
        comp_opts.log_level = CompilationOptions::LogLevel::OFF;
        load_stdlib_into_options(comp_opts);

        Source src_key = std::string("test_hash_collections.slx");
        comp_opts.sources[src_key] = R"(
            package test.pkg;

            import solix.core.*;
            import solix.collections.*;

            public class HashCollectionsTest {
                public static int32 main(char[][] args) {
                    // Test HashMap with int32 keys and values
                    HashMap<int32, int32> map = new HashMap<int32, int32>();
                    map.put(1, 100);
                    map.put(2, 200);
                    map.put(3, 300);

                    if (map.size() != 3) return 1;
                    if (!map.contains_key(2)) return 2;
                    if (map.get(2) != 200) return 3;

                    // Update existing key
                    map.put(2, 250);
                    if (map.get(2) != 250) return 4;
                    if (map.size() != 3) return 5;

                    // Remove key
                    int32 removed = map.remove(2);
                    if (removed != 250) return 6;
                    if (map.contains_key(2)) return 7;
                    if (map.size() != 2) return 8;

                    // Test HashSet
                    HashSet<int32> set = new HashSet<int32>();
                    set.add(10);
                    set.add(20);
                    set.add(30);

                    if (set.size() != 3) return 9;
                    if (!set.contains(20)) return 10;

                    // Duplicate add returns false
                    bool added_again = set.add(20);
                    if (added_again) return 11;
                    if (set.size() != 3) return 12;

                    // Remove item
                    bool was_removed = set.remove(20);
                    if (!was_removed) return 13;
                    if (set.contains(20)) return 14;
                    if (set.size() != 2) return 15;

                    return 0;
                }
            }
        )";

        Bytecode bytecode;
        REQUIRE_NOTHROW(bytecode = solix::run(comp_opts));

        RuntimeOptions run_opts;
        run_opts.bytecode_source = bytecode;
        for (const auto &[name, func] : solix::get_builtin_natives()) {
            run_opts.native_functions[name] = func;
        }
        REQUIRE(solix::run(run_opts) == 0);
    }
}
