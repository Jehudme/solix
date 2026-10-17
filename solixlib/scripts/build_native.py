#!/usr/bin/env python3
"""
Solix Companion Native Library Cross-Compilation Script
Cross-compiles native companion C++ libraries for multiple target platforms
(Linux, Windows, macOS) using Zig or host C++ toolchains.
"""

import argparse
import glob
import os
import platform
import shutil
import subprocess
import sys
from pathlib import Path

TARGET_MAP = {
    "linux": {
        "zig_target": "x86_64-linux-gnu",
        "output_filename": "libsolixlib_native.so",
    },
    "windows": {
        "zig_target": "x86_64-windows-gnu",
        "output_filename": "solixlib_native.dll",
    },
    "macos": {
        "zig_target": "x86_64-macos-none",
        "output_filename": "libsolixlib_native.dylib",
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


def build_with_zig(zig_bin, sources, include_dirs, target_key, output_path, release=True):
    target_info = TARGET_MAP[target_key]
    zig_target = target_info["zig_target"]

    cmd = [
        zig_bin,
        "c++",
        "-shared",
        "-fPIC",
        "-std=c++20",
        "-target",
        zig_target,
        "-Wno-nullability-completeness",
    ]

    if release:
        cmd.append("-O3")
    else:
        cmd.extend(["-O0", "-g"])

    for inc in include_dirs:
        cmd.extend(["-I", str(inc)])

    for src in sources:
        cmd.append(str(src))

    cmd.extend(["-o", str(output_path)])

    print(f"[build_native] Cross-compiling for {target_key} ({zig_target}) -> {output_path.name}")
    res = subprocess.run(cmd, capture_output=True, text=True)
    if res.returncode != 0:
        print(f"[build_native] Error building {target_key}:", file=sys.stderr)
        if res.stderr:
            print(res.stderr, file=sys.stderr)
        return False
    return True


def build_host_fallback(sources, include_dirs, output_path, release=True):
    cxx = os.environ.get("CXX") or shutil.which("g++") or shutil.which("clang++")
    if not cxx:
        print("[build_native] Error: No C++ compiler found (g++ or clang++)", file=sys.stderr)
        return False

    cmd = [cxx, "-shared", "-fPIC", "-std=c++20"]
    if release:
        cmd.append("-O3")
    else:
        cmd.extend(["-O0", "-g"])

    for inc in include_dirs:
        cmd.extend(["-I", str(inc)])

    for src in sources:
        cmd.append(str(src))

    cmd.extend(["-o", str(output_path)])

    print(f"[build_native] Building with host compiler {cxx} -> {output_path.name}")
    res = subprocess.run(cmd, capture_output=True, text=True)
    if res.returncode != 0:
        print(f"[build_native] Error building host fallback:", file=sys.stderr)
        if res.stderr:
            print(res.stderr, file=sys.stderr)
        return False
    return True


def main():
    parser = argparse.ArgumentParser(description="Cross-compile Solix companion native libraries")
    parser.add_argument("--output-dir", "-o", help="Target output directory for compiled binaries")
    parser.add_argument(
        "--targets",
        "-t",
        default="all",
        help="Comma-separated list of targets ('linux', 'windows', 'macos', or 'all')",
    )
    parser.add_argument("--debug", action="store_true", help="Compile with debug symbols (-O0 -g)")
    args = parser.parse_args()

    script_dir = Path(__file__).resolve().parent
    solixlib_dir = script_dir.parent
    repo_root = solixlib_dir.parent

    include_dirs = [
        repo_root / "core" / "include",
        repo_root / "core" / "src",
    ]

    native_src_dir = solixlib_dir / "native" / "src"
    sources = sorted(list(native_src_dir.glob("*.cpp")))
    if not sources:
        print(f"[build_native] Error: No C++ source files found in {native_src_dir}", file=sys.stderr)
        sys.exit(1)

    output_dir = Path(args.output_dir) if args.output_dir else (solixlib_dir / "lib")
    output_dir.mkdir(parents=True, exist_ok=True)

    zig_bin = find_zig()
    release = not args.debug

    if args.targets == "all":
        target_keys = ["linux", "windows", "macos"] if zig_bin else [get_host_target_key()]
    else:
        target_keys = [t.strip().lower() for t in args.targets.split(",") if t.strip()]

    success_count = 0
    for target_key in target_keys:
        if target_key not in TARGET_MAP:
            print(f"[build_native] Warning: Unknown target '{target_key}', skipping", file=sys.stderr)
            continue

        target_info = TARGET_MAP[target_key]
        dest_file = output_dir / target_info["output_filename"]

        is_host = (target_key == get_host_target_key())
        if is_host:
            # Build for host using host C++ compiler to preserve standard library ABI compatibility
            ok = build_host_fallback(sources, include_dirs, dest_file, release=release)
            if not ok and zig_bin:
                ok = build_with_zig(zig_bin, sources, include_dirs, target_key, dest_file, release=release)
        else:
            if zig_bin:
                ok = build_with_zig(zig_bin, sources, include_dirs, target_key, dest_file, release=release)
            else:
                print(
                    f"[build_native] Warning: Zig not installed; cannot cross-compile for foreign target '{target_key}'",
                    file=sys.stderr,
                )
                ok = False

        if ok:
            success_count += 1

    # Clean up Windows-specific linker artifacts (.lib, .exp, .pdb) in output directory
    for ext in ("*.lib", "*.exp", "*.pdb"):
        for aux in output_dir.glob(ext):
            try:
                aux.unlink()
            except OSError:
                pass

    print(f"[build_native] Successfully compiled {success_count}/{len(target_keys)} native target(s)")
    if success_count == 0:
        sys.exit(1)


if __name__ == "__main__":
    main()
