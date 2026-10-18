#!/usr/bin/env python3
"""
Solix Raylib Binding Generator
Parses api/raylib_api.json and generates:
1. Solix type classes (Vector2, Vector3, Vector4, Color, Rectangle)
2. Solix enum classes (KeyboardKey, MouseButton, ConfigFlags, etc.)
3. Solix API class (Raylib.slx)
4. C++ bridge code (native/src/generated_raylib.cpp)
"""

import json
import os
import re
from pathlib import Path

def to_snake_case(name):
    # Special cases for acronyms
    name = re.sub(r'FPS', 'Fps', name)
    name = re.sub(r'DPI', 'Dpi', name)
    name = re.sub(r'RGB', 'Rgb', name)
    name = re.sub(r'HSV', 'Hsv', name)
    name = re.sub(r'2D', '2d', name)
    name = re.sub(r'3D', '3d', name)
    s1 = re.sub(r'(.)([A-Z][a-z]+)', r'\1_\2', name)
    return re.sub(r'([a-z0-9])([A-Z])', r'\1_\2', s1).lower()

TYPE_MAP_SOLIX = {
    'void': 'void',
    'bool': 'bool',
    'int': 'int32',
    'int32_t': 'int32',
    'unsigned int': 'uint32',
    'uint32_t': 'uint32',
    'float': 'float32',
    'double': 'float64',
    'char': 'char',
    'unsigned char': 'uint8',
    'const char *': 'String',
    'Vector2': 'Vector2',
    'Vector3': 'Vector3',
    'Vector4': 'Vector4',
    'Color': 'Color',
    'Rectangle': 'Rectangle',
}

SUPPORTED_TYPES = set(TYPE_MAP_SOLIX.keys())

def is_func_supported(func):
    ret = func.get('returnType', 'void')
    if ret not in SUPPORTED_TYPES:
        return False
    for p in func.get('params', []):
        ptype = p['type']
        if ptype not in SUPPORTED_TYPES:
            return False
    return True

def generate_types(api_dir, out_types_dir):
    out_types_dir.mkdir(parents=True, exist_ok=True)
    
    # 1. Vector2
    vec2_code = """package raylib.types;

public class Vector2 {
    public float32 x;
    public float32 y;

    public Vector2() {
        this.x = 0.0;
        this.y = 0.0;
    }

    public Vector2(float32 x, float32 y) {
        this.x = x;
        this.y = y;
    }
}
"""
    (out_types_dir / "Vector2.slx").write_text(vec2_code)

    # 2. Vector3
    vec3_code = """package raylib.types;

public class Vector3 {
    public float32 x;
    public float32 y;
    public float32 z;

    public Vector3() {
        this.x = 0.0;
        this.y = 0.0;
        this.z = 0.0;
    }

    public Vector3(float32 x, float32 y, float32 z) {
        this.x = x;
        this.y = y;
        this.z = z;
    }
}
"""
    (out_types_dir / "Vector3.slx").write_text(vec3_code)

    # 3. Vector4
    vec4_code = """package raylib.types;

public class Vector4 {
    public float32 x;
    public float32 y;
    public float32 z;
    public float32 w;

    public Vector4() {
        this.x = 0.0;
        this.y = 0.0;
        this.z = 0.0;
        this.w = 0.0;
    }

    public Vector4(float32 x, float32 y, float32 z, float32 w) {
        this.x = x;
        this.y = y;
        this.z = z;
        this.w = w;
    }
}
"""
    (out_types_dir / "Vector4.slx").write_text(vec4_code)

    # 4. Rectangle
    rec_code = """package raylib.types;

public class Rectangle {
    public float32 x;
    public float32 y;
    public float32 width;
    public float32 height;

    public Rectangle() {
        this.x = 0.0;
        this.y = 0.0;
        this.width = 0.0;
        this.height = 0.0;
    }

    public Rectangle(float32 x, float32 y, float32 width, float32 height) {
        this.x = x;
        this.y = y;
        this.width = width;
        this.height = height;
    }
}
"""
    (out_types_dir / "Rectangle.slx").write_text(rec_code)

    # 5. Color with predefined constants
    color_code = """package raylib.types;

public class Color {
    public uint8 r;
    public uint8 g;
    public uint8 b;
    public uint8 a;

    public Color() {
        this.r = 0;
        this.g = 0;
        this.b = 0;
        this.a = 255;
    }

    public Color(uint8 r, uint8 g, uint8 b, uint8 a) {
        this.r = r;
        this.g = g;
        this.b = b;
        this.a = a;
    }

    // Official Raylib Predefined Colors
    public static Color LIGHTGRAY = new Color(200, 200, 200, 255);
    public static Color GRAY = new Color(130, 130, 130, 255);
    public static Color DARKGRAY = new Color(80, 80, 80, 255);
    public static Color YELLOW = new Color(253, 249, 0, 255);
    public static Color GOLD = new Color(255, 203, 0, 255);
    public static Color ORANGE = new Color(255, 161, 0, 255);
    public static Color PINK = new Color(255, 109, 194, 255);
    public static Color RED = new Color(230, 41, 55, 255);
    public static Color MAROON = new Color(190, 33, 55, 255);
    public static Color GREEN = new Color(0, 228, 48, 255);
    public static Color LIME = new Color(0, 158, 47, 255);
    public static Color DARKGREEN = new Color(0, 117, 44, 255);
    public static Color SKYBLUE = new Color(102, 191, 255, 255);
    public static Color BLUE = new Color(0, 121, 241, 255);
    public static Color DARKBLUE = new Color(0, 82, 172, 255);
    public static Color PURPLE = new Color(200, 122, 255, 255);
    public static Color VIOLET = new Color(135, 60, 190, 255);
    public static Color DARKPURPLE = new Color(112, 31, 126, 255);
    public static Color BEIGE = new Color(211, 176, 131, 255);
    public static Color BROWN = new Color(127, 106, 79, 255);
    public static Color DARKBROWN = new Color(76, 63, 47, 255);
    public static Color WHITE = new Color(255, 255, 255, 255);
    public static Color BLACK = new Color(0, 0, 0, 255);
    public static Color BLANK = new Color(0, 0, 0, 0);
    public static Color MAGENTA = new Color(255, 0, 255, 255);
    public static Color RAYWHITE = new Color(245, 245, 245, 255);
}
"""
    (out_types_dir / "Color.slx").write_text(color_code)
    print("Generated types in src/raylib/types/")

def generate_enums(api, out_enums_dir):
    out_enums_dir.mkdir(parents=True, exist_ok=True)
    for enum in api.get('enums', []):
        name = enum['name']
        values = enum.get('values', [])
        lines = [f"package raylib.enums;\n", f"public class {name} {{"]
        for v in values:
            vname = v['name']
            vval = v['value']
            lines.append(f"    public static int32 {vname} = {vval};")
        lines.append("}\n")
        (out_enums_dir / f"{name}.slx").write_text("\n".join(lines))
    print(f"Generated {len(api.get('enums', []))} enum classes in src/raylib/enums/")

def generate_solix_raylib(api, out_path):
    funcs = [f for f in api['functions'] if is_func_supported(f)]
    
    lines = [
        "package raylib;\n",
        "import raylib.types.Vector2;",
        "import raylib.types.Vector3;",
        "import raylib.types.Vector4;",
        "import raylib.types.Color;",
        "import raylib.types.Rectangle;\n",
        "public class Raylib {"
    ]

    # Generate public wrapper methods
    for f in funcs:
        c_name = f['name']
        slx_name = to_snake_case(c_name)
        ret_c = f.get('returnType', 'void')
        ret_slx = TYPE_MAP_SOLIX[ret_c]
        
        params = f.get('params', [])
        slx_params = []
        call_args = []
        for p in params:
            pname = p['name']
            if pname in ('this', 'class', 'struct', 'default'):
                pname = "_" + pname
            ptype_slx = TYPE_MAP_SOLIX[p['type']]
            slx_params.append(f"{ptype_slx} {pname}")
            call_args.append(pname)
            
        param_sig = ", ".join(slx_params)
        arg_list = ", ".join(call_args)
        
        # Check if function returns a struct (handled by out parameter)
        if ret_slx in ('Vector2', 'Vector3', 'Vector4', 'Color', 'Rectangle'):
            lines.append(f"    public static {ret_slx} {slx_name}({param_sig}) {{")
            lines.append(f"        {ret_slx} __res = new {ret_slx}();")
            if arg_list:
                lines.append(f"        native_{slx_name}({arg_list}, __res);")
            else:
                lines.append(f"        native_{slx_name}(__res);")
            lines.append(f"        return __res;")
            lines.append(f"    }}\n")
        elif ret_slx == 'void':
            lines.append(f"    public static void {slx_name}({param_sig}) {{")
            lines.append(f"        native_{slx_name}({arg_list});")
            lines.append(f"    }}\n")
        else:
            lines.append(f"    public static {ret_slx} {slx_name}({param_sig}) {{")
            lines.append(f"        return native_{slx_name}({arg_list});")
            lines.append(f"    }}\n")

    lines.append("    // -------------------------------------------------------------------------")
    lines.append("    // Native Declarations")
    lines.append("    // -------------------------------------------------------------------------")
    for f in funcs:
        c_name = f['name']
        slx_name = to_snake_case(c_name)
        ret_c = f.get('returnType', 'void')
        ret_slx = TYPE_MAP_SOLIX[ret_c]
        
        params = f.get('params', [])
        slx_params = []
        for p in params:
            pname = p['name']
            if pname in ('this', 'class', 'struct', 'default'):
                pname = "_" + pname
            ptype_slx = TYPE_MAP_SOLIX[p['type']]
            slx_params.append(f"{ptype_slx} {pname}")
            
        if ret_slx in ('Vector2', 'Vector3', 'Vector4', 'Color', 'Rectangle'):
            slx_params.append(f"{ret_slx} __out")
            param_sig = ", ".join(slx_params)
            lines.append(f"    private static native void native_{slx_name}({param_sig});")
        else:
            param_sig = ", ".join(slx_params)
            lines.append(f"    private static native {ret_slx} native_{slx_name}({param_sig});")

    lines.append("}\n")
    out_path.write_text("\n".join(lines))
    print(f"Generated {len(funcs)} methods in src/raylib/Raylib.slx")

def generate_cpp_bridge(api, out_cpp_path):
    funcs = [f for f in api['functions'] if is_func_supported(f)]

    cpp_lines = [
        '#include "raylib.h"',
        '#include "solix/native.h"',
        '#include "solix/runtime.hpp"',
        '#include "solix/native_registry.hpp"',
        '#include <cstring>',
        '#include <string>',
        '#include <cstdint>',
        '#include <cmath>',
        '',
        'namespace {',
        '',
        'template <typename T> inline uint64_t bit_cast_to_u64(T value) {',
        '    uint64_t result = 0;',
        '    std::memcpy(&result, &value, sizeof(T));',
        '    return result;',
        '}',
        '',
        'template <typename T> inline T bit_cast_from_u64(uint64_t value) {',
        '    T result;',
        '    std::memcpy(&result, &value, sizeof(T));',
        '    return result;',
        '}',
        '',
        'inline float unpack_f32_arg(uint64_t arg) {',
        '    double d = bit_cast_from_u64<double>(arg);',
        '    if (arg != 0 && (std::isnan(d) || std::abs(d) < 1e-35 || std::abs(d) > 1e35)) {',
        '        uint32_t raw = static_cast<uint32_t>(arg & 0xFFFFFFFF);',
        '        float f;',
        '        std::memcpy(&f, &raw, 4);',
        '        return f;',
        '    }',
        '    return static_cast<float>(d);',
        '}',
        '',
        'inline uint64_t pack_f32_ret(float res) {',
        '    return bit_cast_to_u64<double>(static_cast<double>(res));',
        '}',
        '',
        'static std::string read_solix_chars(solix::RuntimeContext &vm, uint64_t addr) {',
        '    if (addr == 0) return "";',
        '    uint32_t len = vm.memory.get_array_length(static_cast<solix::Address>(addr));',
        '    std::string s;',
        '    s.reserve(len);',
        '    for (uint32_t i = 0; i < len; ++i) {',
        '        s.push_back(vm.memory.read_char(static_cast<solix::Address>(addr), i));',
        '    }',
        '    return s;',
        '}',
        '',
        '// Struct Unpackers (reading from Solix object offset)',
        '// Solix class layout: [0..3: vtable_id]',
        'inline ::Color unpack_color(solix::RuntimeContext &vm, uint64_t addr) {',
        '    if (addr == 0) return {0, 0, 0, 255};',
        '    uint32_t a = static_cast<uint32_t>(addr);',
        '    uint8_t r = vm.memory.read_u8(a, 4);',
        '    uint8_t g = vm.memory.read_u8(a, 5);',
        '    uint8_t b = vm.memory.read_u8(a, 6);',
        '    uint8_t av = vm.memory.read_u8(a, 7);',
        '    return ::Color{r, g, b, av};',
        '}',
        '',
        'inline ::Vector2 unpack_vector2(solix::RuntimeContext &vm, uint64_t addr) {',
        '    if (addr == 0) return {0.0f, 0.0f};',
        '    uint32_t a = static_cast<uint32_t>(addr);',
        '    uint32_t rx = vm.memory.read_u32(a, 4);',
        '    uint32_t ry = vm.memory.read_u32(a, 8);',
        '    float x, y;',
        '    std::memcpy(&x, &rx, 4);',
        '    std::memcpy(&y, &ry, 4);',
        '    return ::Vector2{x, y};',
        '}',
        '',
        'inline ::Vector3 unpack_vector3(solix::RuntimeContext &vm, uint64_t addr) {',
        '    if (addr == 0) return {0.0f, 0.0f, 0.0f};',
        '    uint32_t a = static_cast<uint32_t>(addr);',
        '    uint32_t rx = vm.memory.read_u32(a, 4);',
        '    uint32_t ry = vm.memory.read_u32(a, 8);',
        '    uint32_t rz = vm.memory.read_u32(a, 12);',
        '    float x, y, z;',
        '    std::memcpy(&x, &rx, 4);',
        '    std::memcpy(&y, &ry, 4);',
        '    std::memcpy(&z, &rz, 4);',
        '    return ::Vector3{x, y, z};',
        '}',
        '',
        'inline ::Vector4 unpack_vector4(solix::RuntimeContext &vm, uint64_t addr) {',
        '    if (addr == 0) return {0.0f, 0.0f, 0.0f, 0.0f};',
        '    uint32_t a = static_cast<uint32_t>(addr);',
        '    uint32_t rx = vm.memory.read_u32(a, 4);',
        '    uint32_t ry = vm.memory.read_u32(a, 8);',
        '    uint32_t rz = vm.memory.read_u32(a, 12);',
        '    uint32_t rw = vm.memory.read_u32(a, 16);',
        '    float x, y, z, w;',
        '    std::memcpy(&x, &rx, 4);',
        '    std::memcpy(&y, &ry, 4);',
        '    std::memcpy(&z, &rz, 4);',
        '    std::memcpy(&w, &rw, 4);',
        '    return ::Vector4{x, y, z, w};',
        '}',
        '',
        'inline ::Rectangle unpack_rectangle(solix::RuntimeContext &vm, uint64_t addr) {',
        '    if (addr == 0) return {0.0f, 0.0f, 0.0f, 0.0f};',
        '    uint32_t a = static_cast<uint32_t>(addr);',
        '    uint32_t rx = vm.memory.read_u32(a, 4);',
        '    uint32_t ry = vm.memory.read_u32(a, 8);',
        '    uint32_t rw = vm.memory.read_u32(a, 12);',
        '    uint32_t rh = vm.memory.read_u32(a, 16);',
        '    float x, y, w, h;',
        '    std::memcpy(&x, &rx, 4);',
        '    std::memcpy(&y, &ry, 4);',
        '    std::memcpy(&w, &rw, 4);',
        '    std::memcpy(&h, &rh, 4);',
        '    return ::Rectangle{x, y, w, h};',
        '}',
        '',
        '// Struct Packers (writing result into Solix object)',
        'inline void pack_vector2(solix::RuntimeContext &vm, uint64_t addr, const ::Vector2 &v) {',
        '    uint32_t a = static_cast<uint32_t>(addr);',
        '    uint32_t rx, ry;',
        '    std::memcpy(&rx, &v.x, 4);',
        '    std::memcpy(&ry, &v.y, 4);',
        '    vm.memory.write_u32(a, 4, rx);',
        '    vm.memory.write_u32(a, 8, ry);',
        '}',
        '',
        'inline void pack_vector3(solix::RuntimeContext &vm, uint64_t addr, const ::Vector3 &v) {',
        '    uint32_t a = static_cast<uint32_t>(addr);',
        '    uint32_t rx, ry, rz;',
        '    std::memcpy(&rx, &v.x, 4);',
        '    std::memcpy(&ry, &v.y, 4);',
        '    std::memcpy(&rz, &v.z, 4);',
        '    vm.memory.write_u32(a, 4, rx);',
        '    vm.memory.write_u32(a, 8, ry);',
        '    vm.memory.write_u32(a, 12, rz);',
        '}',
        '',
        'inline void pack_vector4(solix::RuntimeContext &vm, uint64_t addr, const ::Vector4 &v) {',
        '    uint32_t a = static_cast<uint32_t>(addr);',
        '    uint32_t rx, ry, rz, rw;',
        '    std::memcpy(&rx, &v.x, 4);',
        '    std::memcpy(&ry, &v.y, 4);',
        '    std::memcpy(&rz, &v.z, 4);',
        '    std::memcpy(&rw, &v.w, 4);',
        '    vm.memory.write_u32(a, 4, rx);',
        '    vm.memory.write_u32(a, 8, ry);',
        '    vm.memory.write_u32(a, 12, rz);',
        '    vm.memory.write_u32(a, 16, rw);',
        '}',
        '',
        'inline void pack_color(solix::RuntimeContext &vm, uint64_t addr, const ::Color &c) {',
        '    uint32_t a = static_cast<uint32_t>(addr);',
        '    vm.memory.write_u8(a, 4, c.r);',
        '    vm.memory.write_u8(a, 5, c.g);',
        '    vm.memory.write_u8(a, 6, c.b);',
        '    vm.memory.write_u8(a, 7, c.a);',
        '}',
        '',
        'inline void pack_rectangle(solix::RuntimeContext &vm, uint64_t addr, const ::Rectangle &r) {',
        '    uint32_t a = static_cast<uint32_t>(addr);',
        '    uint32_t rx, ry, rw, rh;',
        '    std::memcpy(&rx, &r.x, 4);',
        '    std::memcpy(&ry, &r.y, 4);',
        '    std::memcpy(&rw, &r.width, 4);',
        '    std::memcpy(&rh, &r.height, 4);',
        '    vm.memory.write_u32(a, 4, rx);',
        '    vm.memory.write_u32(a, 8, ry);',
        '    vm.memory.write_u32(a, 12, rw);',
        '    vm.memory.write_u32(a, 16, rh);',
        '}',
        ''
    ]

    for f in funcs:
        c_name = f['name']
        slx_name = to_snake_case(c_name)
        ret_c = f.get('returnType', 'void')
        params = f.get('params', [])
        
        cpp_lines.append(f"static uint64_t wrap_{slx_name}(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {{")
        cpp_lines.append("    (void)self; (void)argc;")
        
        # Prepare argument conversions
        c_args = []
        for i, p in enumerate(params):
            ptype = p['type']
            arg_var = f"a{i}"
            if ptype in ('int', 'int32_t'):
                cpp_lines.append(f"    int {arg_var} = static_cast<int>(args[{i}]);")
                c_args.append(arg_var)
            elif ptype in ('unsigned int', 'uint32_t'):
                cpp_lines.append(f"    unsigned int {arg_var} = static_cast<unsigned int>(args[{i}]);")
                c_args.append(arg_var)
            elif ptype == 'float':
                cpp_lines.append(f"    float {arg_var} = unpack_f32_arg(args[{i}]);")
                c_args.append(arg_var)
            elif ptype == 'double':
                cpp_lines.append(f"    double {arg_var} = bit_cast_from_u64<double>(args[{i}]);")
                c_args.append(arg_var)
            elif ptype == 'bool':
                cpp_lines.append(f"    bool {arg_var} = (args[{i}] != 0);")
                c_args.append(arg_var)
            elif ptype == 'const char *':
                cpp_lines.append(f"    std::string s_{arg_var} = read_solix_chars(vm, args[{i}]);")
                cpp_lines.append(f"    const char *{arg_var} = s_{arg_var}.c_str();")
                c_args.append(arg_var)
            elif ptype == 'Color':
                cpp_lines.append(f"    ::Color {arg_var} = unpack_color(vm, args[{i}]);")
                c_args.append(arg_var)
            elif ptype == 'Vector2':
                cpp_lines.append(f"    ::Vector2 {arg_var} = unpack_vector2(vm, args[{i}]);")
                c_args.append(arg_var)
            elif ptype == 'Vector3':
                cpp_lines.append(f"    ::Vector3 {arg_var} = unpack_vector3(vm, args[{i}]);")
                c_args.append(arg_var)
            elif ptype == 'Vector4':
                cpp_lines.append(f"    ::Vector4 {arg_var} = unpack_vector4(vm, args[{i}]);")
                c_args.append(arg_var)
            elif ptype == 'Rectangle':
                cpp_lines.append(f"    ::Rectangle {arg_var} = unpack_rectangle(vm, args[{i}]);")
                c_args.append(arg_var)

        c_call = f"::{c_name}({', '.join(c_args)})"
        
        # Return handling
        if ret_c in ('Vector2', 'Vector3', 'Vector4', 'Color', 'Rectangle'):
            out_idx = len(params)
            cpp_lines.append(f"    auto res = {c_call};")
            cpp_lines.append(f"    pack_{ret_c.lower()}(vm, args[{out_idx}], res);")
            cpp_lines.append("    return 0;")
        elif ret_c == 'void':
            cpp_lines.append(f"    {c_call};")
            cpp_lines.append("    return 0;")
        elif ret_c == 'bool':
            cpp_lines.append(f"    return {c_call} ? 1 : 0;")
        elif ret_c in ('int', 'int32_t', 'unsigned int', 'uint32_t'):
            cpp_lines.append(f"    return static_cast<uint64_t>({c_call});")
        elif ret_c == 'float':
            cpp_lines.append(f"    float res = {c_call};")
            cpp_lines.append("    return pack_f32_ret(res);")
        elif ret_c == 'double':
            cpp_lines.append(f"    double res = {c_call};")
            cpp_lines.append("    return bit_cast_to_u64<double>(res);")
        else:
            cpp_lines.append(f"    {c_call};")
            cpp_lines.append("    return 0;")

        cpp_lines.append("}\n")

    cpp_lines.append("} // namespace\n")
    cpp_lines.append("void register_raylib_natives(solix::NativeRegistry &registry) {")
    for f in funcs:
        c_name = f['name']
        slx_name = to_snake_case(c_name)
        cpp_lines.append(f'    registry.register_function("Raylib_native_{slx_name}", wrap_{slx_name});')
        cpp_lines.append(f'    registry.register_function("raylib_Raylib_native_{slx_name}", wrap_{slx_name});')
        cpp_lines.append(f'    registry.register_function("native_{slx_name}", wrap_{slx_name});')
    cpp_lines.append("}\n")

    out_cpp_path.write_text("\n".join(cpp_lines))
    print(f"Generated C++ bridge in {out_cpp_path}")

def update_manifest(root):
    manifest_path = root / "manifest.json"
    if not manifest_path.exists():
        return
    with open(manifest_path) as f:
        mf = json.load(f)

    src_files = sorted([s for s in root.glob("src/**/*.slx") if s.name != "main.slx"])
    deps = [d for d in mf.get("dependencies", []) if d.get("type") != "source"]
    for s in src_files:
        rel = str(s.relative_to(root))
        deps.append({"type": "source", "path": rel})

    mf["dependencies"] = deps
    with open(manifest_path, "w") as f:
        json.dump(mf, f, indent=2)
    print(f"Updated manifest.json with {len(src_files)} source files.")

def main():
    root = Path(__file__).resolve().parent.parent
    api_path = root / "api" / "raylib_api.json"
    if not api_path.exists():
        print(f"Error: {api_path} not found.")
        return 1

    with open(api_path) as f:
        api = json.load(f)

    types_dir = root / "src" / "raylib" / "types"
    enums_dir = root / "src" / "raylib" / "enums"
    solix_raylib_path = root / "src" / "raylib" / "Raylib.slx"
    cpp_bridge_path = root / "native" / "src" / "generated_raylib.cpp"

    generate_types(api_path, types_dir)
    generate_enums(api, enums_dir)
    generate_solix_raylib(api, solix_raylib_path)
    generate_cpp_bridge(api, cpp_bridge_path)
    update_manifest(root)

    print("Code generation completed successfully!")
    return 0

if __name__ == "__main__":
    main()
