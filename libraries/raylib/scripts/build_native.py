#!/usr/bin/env python3
"""
Solix Companion Native Library Cross-Compilation Script for Raylib
Builds native companion C++ libraries with static Raylib integration
using CMake and Zig toolchains for cross-platform support.
"""

import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path

TARGET_MAP = {
    "linux": {
        "system_name": "Linux",
        "zig_target": "x86_64-linux-gnu",
        "output_filename": "libsolix_raylib.so",
    },
    "windows": {
        "system_name": "Windows",
        "zig_target": "x86_64-windows-gnu",
        "output_filename": "libsolix_raylib.dll",
    },
    "macos": {
        "system_name": "Darwin",
        "zig_target": "x86_64-macos-none",
        "output_filename": "libsolix_raylib.dylib",
    },
}


def find_zig():
    return shutil.which("zig")


def get_host_target_key():
    plat = sys.platform
    if plat.startswith("linux"):
        return "linux"
    elif plat in ("win32", "cygwin"):
        return "windows"
    elif plat == "darwin":
        return "macos"
    return "linux"


def build_target(native_dir, build_dir, target_key, output_path, release=True, use_zig=False):
    target_info = TARGET_MAP[target_key]
    build_type = "Release" if release else "Debug"

    cmd = [
        "cmake",
        "-S", str(native_dir),
        "-B", str(build_dir),
        f"-DCMAKE_BUILD_TYPE={build_type}",
    ]

    if use_zig:
        zig_target = target_info["zig_target"]
        system_name = target_info["system_name"]
        cmd.extend([
            f"-DCMAKE_SYSTEM_NAME={system_name}",
            "-DCMAKE_C_COMPILER=zig",
            f"-DCMAKE_C_COMPILER_ARG1=cc -target {zig_target}",
            "-DCMAKE_CXX_COMPILER=zig",
            f"-DCMAKE_CXX_COMPILER_ARG1=c++ -target {zig_target}",
        ])

    print(f"[build_native] Configuring {target_key} (use_zig={use_zig})...")
    res = subprocess.run(cmd)
    if res.returncode != 0:
        print(f"[build_native] CMake configuration failed for {target_key}", file=sys.stderr)
        return False

    print(f"[build_native] Compiling {target_key}...")
    res_build = subprocess.run(["cmake", "--build", str(build_dir), "-j8"])
    if res_build.returncode != 0:
        print(f"[build_native] Compilation failed for {target_key}", file=sys.stderr)
        return False

    compiled_lib = output_path.parent / target_info["output_filename"]
    if target_key == "windows":
        alt_name = "solix_raylib.dll" if target_info["output_filename"].startswith("lib") else "libsolix_raylib.dll"
        alt_lib = output_path.parent / alt_name
        if alt_lib.exists() and not compiled_lib.exists():
            shutil.copy2(alt_lib, compiled_lib)
        elif compiled_lib.exists() and not alt_lib.exists():
            shutil.copy2(compiled_lib, alt_lib)
    if compiled_lib.exists():
        print(f"[build_native] Successfully produced {compiled_lib.name}")
        return True

    return False


def main():
    parser = argparse.ArgumentParser(description="Cross-compile Solix companion native libraries for Raylib")
    parser.add_argument("--output-dir", "-o", help="Target output directory for compiled binaries")
    parser.add_argument(
        "--targets",
        "-t",
        default="all",
        help="Comma-separated list of targets ('linux', 'windows', 'macos', or 'all')",
    )
    parser.add_argument("--debug", action="store_true", help="Compile with debug symbols")
    parser.add_argument("--force", "-f", action="store_true", help="Force rebuild even if output already exists")
    args = parser.parse_args()

    script_dir = Path(__file__).resolve().parent
    raylib_dir = script_dir.parent
    native_dir = raylib_dir / "native"
    output_dir = Path(args.output_dir) if args.output_dir else (raylib_dir / "lib")
    output_dir.mkdir(parents=True, exist_ok=True)

    build_root = raylib_dir / "build" / "native"

    zig_bin = find_zig()
    release = not args.debug

    if args.targets == "all":
        target_keys = ["linux", "windows"] if zig_bin else [get_host_target_key()]
    else:
        target_keys = [t.strip().lower() for t in args.targets.split(",") if t.strip()]

    host_target = get_host_target_key()
    success_count = 0

    for target_key in target_keys:
        if target_key not in TARGET_MAP:
            print(f"[build_native] Warning: Unknown target '{target_key}', skipping", file=sys.stderr)
            continue

        target_info = TARGET_MAP[target_key]
        dest_file = output_dir / target_info["output_filename"]
        target_build_dir = build_root / target_key

        if target_key == "windows":
            alt_name = "solix_raylib.dll" if target_info["output_filename"].startswith("lib") else "libsolix_raylib.dll"
            alt_file = output_dir / alt_name
            if (dest_file.exists() or alt_file.exists()) and not args.force:
                if alt_file.exists() and not dest_file.exists():
                    shutil.copy2(alt_file, dest_file)
                elif dest_file.exists() and not alt_file.exists():
                    shutil.copy2(dest_file, alt_file)
                print(f"[build_native] {dest_file.name} is already built. Use --force to rebuild.")
                success_count += 1
                continue
        elif dest_file.exists() and not args.force:
            print(f"[build_native] {dest_file.name} is already built. Use --force to rebuild.")
            success_count += 1
            continue

        is_host = (target_key == host_target)
        if is_host:
            ok = build_target(native_dir, target_build_dir, target_key, dest_file, release=release, use_zig=False)
            if not ok and zig_bin:
                ok = build_target(native_dir, target_build_dir, target_key, dest_file, release=release, use_zig=True)
        else:
            if zig_bin:
                ok = build_target(native_dir, target_build_dir, target_key, dest_file, release=release, use_zig=True)
            else:
                print(f"[build_native] Zig not available, cannot cross-compile for {target_key}")
                ok = False

        if ok:
            success_count += 1

    # Clean up auxiliary linker artifacts (.lib, .exp, .pdb) in output directory
    for ext in ("*.exp", "*.pdb"):
        for aux in output_dir.glob(ext):
            try:
                aux.unlink()
            except OSError:
                pass

    print(f"[build_native] Done: {success_count}/{len(target_keys)} native target(s) ready")
    if success_count == 0:
        sys.exit(1)


if __name__ == "__main__":
    main()
