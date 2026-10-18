#!/usr/bin/env python3
"""
Native compilation script for solix-raylib companion library.
Compiles generated C++ bindings and statically links Raylib.
"""

import os
import sys
import subprocess
from pathlib import Path

def build():
    root = Path(__file__).resolve().parent.parent
    native_dir = root / "native"
    output_dir = root / "lib"
    output_dir.mkdir(parents=True, exist_ok=True)

    sources = [
        str(native_dir / "src" / "generated_raylib.cpp"),
        str(native_dir / "src" / "register.cpp")
    ]

    include_dirs = [
        "-I", str(native_dir / "include"),
        "-I", str(native_dir / "include" / "solix"),
        "-I", str(root.parent.parent / "core" / "include")
    ]

    libraylib_a = native_dir / "lib" / "linux" / "libraylib.a"
    if not libraylib_a.exists():
        print(f"[build_native] {libraylib_a} not found, fetching Raylib 6.2 static library...")
        import urllib.request, tarfile, io
        libraylib_a.parent.mkdir(parents=True, exist_ok=True)
        url = "https://github.com/raysan5/raylib/releases/download/6.2/raylib-6.2_linux_amd64.tar.gz"
        try:
            data = urllib.request.urlopen(url).read()
            with tarfile.open(fileobj=io.BytesIO(data), mode="r:gz") as tar:
                for member in tar.getmembers():
                    if member.name.endswith("libraylib.a"):
                        f = tar.extractfile(member)
                        with open(libraylib_a, "wb") as out:
                            out.write(f.read())
            print(f"[build_native] Successfully fetched {libraylib_a}")
        except Exception as e:
            print(f"[build_native] Failed to download Raylib archive: {e}", file=sys.stderr)
            return 1

    out_so = output_dir / "libsolix_raylib.so"

    cmd = [
        "g++", "-shared", "-fPIC", "-std=c++17", "-O2",
        *include_dirs,
        *sources,
        str(libraylib_a),
        "-lGL", "-lm", "-lpthread", "-ldl", "-lrt", "-lX11",
        "-o", str(out_so)
    ]

    print(f"[build_native] Compiling {out_so.name}...")
    res = subprocess.run(cmd)
    if res.returncode != 0:
        print("[build_native] Compilation failed!", file=sys.stderr)
        return res.returncode

    print(f"[build_native] Successfully built {out_so}")
    return 0

if __name__ == "__main__":
    sys.exit(build())
