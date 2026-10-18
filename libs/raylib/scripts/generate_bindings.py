#!/usr/bin/env python3
import json
import re
from pathlib import Path

def to_snake_case(name):
    name = re.sub(r'FPS', 'Fps', name)
    name = re.sub(r'DPI', 'Dpi', name)
    name = re.sub(r'RGB', 'Rgb', name)
    name = re.sub(r'HSV', 'Hsv', name)
    name = re.sub(r'2D', '2d', name)
    name = re.sub(r'3D', '3d', name)
    name = re.sub(r'VAO', 'Vao', name)
    name = re.sub(r'VBO', 'Vbo', name)
    s1 = re.sub(r'(.)([A-Z][a-z]+)', r'\1_\2', name)
    return re.sub(r'([a-z0-9])([A-Z])', r'\1_\2', s1).lower()

ALIASES = {
    'Quaternion': 'Vector4',
    'Texture': 'Texture2D',
    'TextureCubemap': 'Texture2D',
    'RenderTexture': 'RenderTexture2D',
    'Camera': 'Camera3D',
    'ModelAnimPose': 'Transform',
}

PRIMITIVES_SOLIX = {
    'void': 'void',
    'bool': 'bool',
    'char': 'char',
    'int': 'int32',
    'int32_t': 'int32',
    'unsigned int': 'uint32',
    'uint32_t': 'uint32',
    'unsigned short': 'uint16',
    'unsigned char': 'uint8',
    'long': 'int64',
    'float': 'float32',
    'double': 'float64',
    'const char *': 'String',
    'char *': 'String',
    'void *': 'uint64',
    'const void *': 'uint64',
}

KNOWN_STRUCTS = {
    'Vector2', 'Vector3', 'Vector4', 'Matrix', 'Color', 'Rectangle',
    'Image', 'Texture', 'Texture2D', 'RenderTexture', 'RenderTexture2D',
    'NPatchInfo', 'GlyphInfo', 'Font', 'Camera3D', 'Camera2D', 'Mesh',
    'Shader', 'MaterialMap', 'Material', 'Transform', 'BoneInfo',
    'ModelSkeleton', 'Model', 'ModelAnimation', 'Ray', 'RayCollision',
    'BoundingBox', 'Wave', 'AudioStream', 'Sound', 'Music', 'VrDeviceInfo',
    'VrStereoConfig', 'FilePathList', 'AutomationEvent', 'AutomationEventList',
    'Camera', 'Quaternion', 'TextureCubemap'
}


SOLIX_KEYWORDS = {
    'void', 'bool', 'int8', 'int16', 'int32', 'int64', 'uint8', 'uint16',
    'uint32', 'uint64', 'float32', 'float64', 'char', 'if', 'else', 'for',
    'while', 'do', 'return', 'break', 'continue', 'switch', 'case', 'default',
    'try', 'catch', 'finally', 'throw', 'const', 'class', 'enum', 'static',
    'inline', 'alias', 'import', 'public', 'native', 'protected', 'private',
    'internal', 'package', 'new', 'operator', 'extends', 'super', 'virtual',
    'override', 'weak', 'abstract', 'interface', 'implements', 'instanceof',
    'sizeof', 'assert', 'exit', 'this', 'null', 'true', 'false', 'struct'
}

def map_c_type_to_solix(c_type):
    c_clean = c_type.strip()
    if c_clean in ALIASES:
        return ALIASES[c_clean]
    if c_clean in PRIMITIVES_SOLIX:
        return PRIMITIVES_SOLIX[c_clean]
    if c_clean in KNOWN_STRUCTS:
        return ALIASES.get(c_clean, c_clean)
    if c_clean.endswith('*'):
        inner = c_clean[:-1].strip()
        if inner.startswith('const '):
            inner = inner[6:].strip()
        if inner in KNOWN_STRUCTS:
            return f"{ALIASES.get(inner, inner)}[]"
        if inner in ('int', 'int32_t'):
            return "int32[]"
        if inner == 'float':
            return "float32[]"
        if inner in ('unsigned char', 'uint8_t'):
            return "uint8[]"
        if inner in ('unsigned int', 'uint32_t'):
            return "uint32[]"
        if inner in ('char *', 'char'):
            return "String[]"
        return "uint64"
    return None

def map_struct_fields(struct_name, raw_fields):
    mapped = []
    for f in raw_fields:
        ftype = f['type'].strip()
        fname = f['name'].strip()
        if fname in SOLIX_KEYWORDS:
            fname = '_' + fname
            
        if ftype == 'float':
            mapped.append((fname, 'float32', '0.0', 4, 'float', ftype))
        elif ftype == 'double':
            mapped.append((fname, 'float64', '0.0', 8, 'double', ftype))
        elif ftype in ('int', 'int32_t'):
            mapped.append((fname, 'int32', '0', 4, 'int', ftype))
        elif ftype in ('unsigned int', 'uint32_t'):
            mapped.append((fname, 'uint32', '0', 4, 'uint', ftype))
        elif ftype == 'unsigned short':
            mapped.append((fname, 'uint16', '0', 2, 'ushort', ftype))
        elif ftype == 'unsigned char':
            mapped.append((fname, 'uint8', '0', 1, 'uchar', ftype))
        elif ftype == 'char':
            mapped.append((fname, 'char', "'\\0'", 1, 'char', ftype))
        elif ftype == 'bool':
            mapped.append((fname, 'bool', 'false', 1, 'bool', ftype))
        elif ftype in ('const char *', 'char *'):
            mapped.append((fname, 'String', 'null', 8, 'string', ftype))
        elif ftype in ('void *', 'const void *') or ftype.endswith('*'):
            mapped.append((fname, 'uint64', '0', 8, 'pointer', ftype))
        elif ftype in KNOWN_STRUCTS:
            slx_type = ALIASES.get(ftype, ftype)
            mapped.append((fname, slx_type, f'new {slx_type}()', 8, f'struct_{slx_type}', ftype))
        elif ftype == 'char[32]':
            mapped.append((fname, 'String', '""', 8, 'char_array_32', ftype))
        elif ftype == 'float[4]':
            for idx in range(4):
                mapped.append((f'{fname}_{idx}', 'float32', '0.0', 4, f'float4_{idx}_{fname}', 'float'))
        elif ftype == 'int[4]':
            for idx in range(4):
                mapped.append((f'{fname}_{idx}', 'int32', '0', 4, f'int4_{idx}_{fname}', 'int'))
        elif ftype == 'float[2]':
            mapped.append((fname, 'Vector2', 'new Vector2()', 8, 'float2_' + fname, 'float[2]'))
        elif ftype == 'Matrix[2]':
            for idx in range(2):
                mapped.append((f'{fname}_{idx}', 'Matrix', 'new Matrix()', 8, f'matrix2_{idx}_{fname}', 'Matrix'))
        else:
            mapped.append((fname, 'uint64', '0', 8, 'pointer', ftype))

    # Compute Solix memory offsets
    offset = 4 # after vtable_id
    field_records = []
    for fname, slx_type, def_val, size, kind, orig_t in mapped:
        offset = (offset + size - 1) & ~(size - 1)
        field_records.append({
            'name': fname,
            'type': slx_type,
            'default': def_val,
            'size': size,
            'kind': kind,
            'orig_type': orig_t,
            'offset': offset,
        })
        offset += size
    return field_records

def main():
    root = Path(__file__).resolve().parent.parent
    api_path = root / "api" / "raylib_api.json"
    types_dir = root / "src" / "raylib" / "types"
    enums_dir = root / "src" / "raylib" / "enums"
    solix_raylib_path = root / "src" / "raylib" / "Raylib.slx"
    cpp_bridge_path = root / "native" / "src" / "generated_raylib.cpp"
    manifest_path = root / "manifest.json"

    with open(api_path) as f:
        api = json.load(f)

    # 1. Generate types
    types_dir.mkdir(parents=True, exist_ok=True)
    structs = api['structs']
    for s in structs:
        name = s['name']
        if name == 'Texture': name = 'Texture2D'
        elif name == 'RenderTexture': name = 'RenderTexture2D'
        
        fields = map_struct_fields(name, s['fields'])
        imports = set()
        for f in fields:
            ft = f['type']
            if ft in KNOWN_STRUCTS and ft != name:
                imports.add(f"import raylib.types.{ft};")
            elif ft == 'String':
                imports.add("import solix.core.String;")

        lines = ["package raylib.types;\n"]
        for imp in sorted(imports):
            lines.append(imp)
        if imports:
            lines.append("")

        lines.append(f"public class {name} {{")
        for f in fields:
            lines.append(f"    public {f['type']} {f['name']};")
        lines.append("")

        # Default constructor
        lines.append(f"    public {name}() {{")
        for f in fields:
            lines.append(f"        this.{f['name']} = {f['default']};")
        lines.append("    }")
        lines.append("")

        # Parameterized constructor
        if len(fields) <= 16:
            params = [f"{f['type']} {f['name']}" for f in fields]
            lines.append(f"    public {name}({', '.join(params)}) {{")
            for f in fields:
                lines.append(f"        this.{f['name']} = {f['name']};")
            lines.append("    }")
            lines.append("")

        if name == 'Color':
            colors = [
                ("LIGHTGRAY", "200, 200, 200, 255"),
                ("GRAY", "130, 130, 130, 255"),
                ("DARKGRAY", "80, 80, 80, 255"),
                ("YELLOW", "253, 249, 0, 255"),
                ("GOLD", "255, 203, 0, 255"),
                ("ORANGE", "255, 161, 0, 255"),
                ("PINK", "255, 109, 194, 255"),
                ("RED", "230, 41, 55, 255"),
                ("MAROON", "190, 33, 55, 255"),
                ("GREEN", "0, 228, 48, 255"),
                ("LIME", "0, 158, 47, 255"),
                ("DARKGREEN", "0, 117, 44, 255"),
                ("SKYBLUE", "102, 191, 255, 255"),
                ("BLUE", "0, 121, 241, 255"),
                ("DARKBLUE", "0, 82, 172, 255"),
                ("PURPLE", "200, 122, 255, 255"),
                ("VIOLET", "135, 60, 190, 255"),
                ("DARKPURPLE", "112, 31, 126, 255"),
                ("BEIGE", "211, 176, 131, 255"),
                ("BROWN", "127, 106, 79, 255"),
                ("DARKBROWN", "76, 63, 47, 255"),
                ("WHITE", "255, 255, 255, 255"),
                ("BLACK", "0, 0, 0, 255"),
                ("BLANK", "0, 0, 0, 0"),
                ("MAGENTA", "255, 0, 255, 255"),
                ("RAYWHITE", "245, 245, 245, 255"),
            ]
            lines.append("    // Official Raylib Predefined Colors")
            for cname, cargs in colors:
                lines.append(f"    public static Color {cname} = new Color({cargs});")

        lines.append("}\n")
        (types_dir / f"{name}.slx").write_text("\n".join(lines))

    # Aliases
    aliases_to_gen = [
        ('Texture', 'Texture2D'),
        ('RenderTexture', 'RenderTexture2D'),
        ('Camera', 'Camera3D'),
        ('Quaternion', 'Vector4'),
    ]
    for alias_name, target_name in aliases_to_gen:
        content = f"""package raylib.types;

public class {alias_name} : {target_name} {{
    public {alias_name}() : super() {{}}
}}
"""
        (types_dir / f"{alias_name}.slx").write_text(content)
    print(f"Generated {len(structs)} structs + {len(aliases_to_gen)} aliases in src/raylib/types/")

    # 2. Generate enums
    enums_dir.mkdir(parents=True, exist_ok=True)
    for enum in api.get('enums', []):
        name = enum['name']
        values = enum.get('values', [])
        lines = [f"package raylib.enums;\n", f"public class {name} {{"]
        for v in values:
            vname = v['name']
            vval = v['value']
            lines.append(f"    public static int32 {vname} = {vval};")
        lines.append("}\n")
        (enums_dir / f"{name}.slx").write_text("\n".join(lines))
    print(f"Generated {len(api.get('enums', []))} enums in src/raylib/enums/")

    # 3. Filter supported functions
    def is_func_supported(f):
        params = f.get('params', [])
        ret = f.get('returnType', 'void')
        if any(p['type'] == '...' for p in params):
            return False
        if any('Callback' in p['type'] for p in params):
            return False
        if any(p['type'] == 'Rectangle **' for p in params):
            return False
        if map_c_type_to_solix(ret) is None:
            return False
        for p in params:
            if map_c_type_to_solix(p['type']) is None:
                return False
        return True

    funcs = [f for f in api['functions'] if is_func_supported(f)]
    print(f"Supported functions: {len(funcs)} / {len(api['functions'])}")

    # 4. Generate Raylib.slx
    r_lines = [
        "package raylib;\n",
        "import solix.core.String;\n",
    ]
    for s in structs:
        sname = s['name']
        if sname == 'Texture': sname = 'Texture2D'
        elif sname == 'RenderTexture': sname = 'RenderTexture2D'
        r_lines.append(f"import raylib.types.{sname};")
    r_lines.append("import raylib.types.Camera;")
    r_lines.append("import raylib.types.Texture;")
    r_lines.append("import raylib.types.RenderTexture;")
    r_lines.append("import raylib.types.Quaternion;\n")
    r_lines.append("public class Raylib {")

    struct_names_set = set(s['name'] for s in structs)
    struct_names_set.update(['Texture2D', 'RenderTexture2D', 'Camera', 'Quaternion'])

    for f in funcs:
        c_name = f['name']
        slx_name = to_snake_case(c_name)
        ret_c = f.get('returnType', 'void')
        ret_slx = map_c_type_to_solix(ret_c)
        params = f.get('params', [])
        
        slx_params = []
        call_args = []
        for p in params:
            pname = p['name']
            if pname in SOLIX_KEYWORDS:
                pname = "_" + pname
            ptype_slx = map_c_type_to_solix(p['type'])
            slx_params.append(f"{ptype_slx} {pname}")
            call_args.append(pname)

        param_sig = ", ".join(slx_params)
        arg_list = ", ".join(call_args)

        is_ret_struct = ret_slx in struct_names_set

        if is_ret_struct:
            r_lines.append(f"    public static {ret_slx} {slx_name}({param_sig}) {{")
            r_lines.append(f"        {ret_slx} __res = new {ret_slx}();")
            if arg_list:
                r_lines.append(f"        native_{slx_name}({arg_list}, __res);")
            else:
                r_lines.append(f"        native_{slx_name}(__res);")
            r_lines.append(f"        return __res;")
            r_lines.append(f"    }}\n")
        elif ret_slx == 'void':
            r_lines.append(f"    public static void {slx_name}({param_sig}) {{")
            r_lines.append(f"        native_{slx_name}({arg_list});")
            r_lines.append(f"    }}\n")
        elif ret_slx == 'String':
            r_lines.append(f"    public static String {slx_name}({param_sig}) {{")
            r_lines.append(f"        char[] __chars = native_{slx_name}({arg_list});")
            r_lines.append(f"        if (__chars == null) return null;")
            r_lines.append(f"        return new String(__chars);")
            r_lines.append(f"    }}\n")
        else:
            r_lines.append(f"    public static {ret_slx} {slx_name}({param_sig}) {{")
            r_lines.append(f"        return native_{slx_name}({arg_list});")
            r_lines.append(f"    }}\n")

    r_lines.append("    // -------------------------------------------------------------------------")
    r_lines.append("    // Native Declarations")
    r_lines.append("    // -------------------------------------------------------------------------")

    for f in funcs:
        c_name = f['name']
        slx_name = to_snake_case(c_name)
        ret_c = f.get('returnType', 'void')
        ret_slx = map_c_type_to_solix(ret_c)
        params = f.get('params', [])
        
        slx_params = []
        for p in params:
            pname = p['name']
            if pname in SOLIX_KEYWORDS:
                pname = "_" + pname
            ptype_slx = map_c_type_to_solix(p['type'])
            slx_params.append(f"{ptype_slx} {pname}")

        is_ret_struct = ret_slx in struct_names_set

        if is_ret_struct:
            slx_params.append(f"{ret_slx} __out")
            param_sig = ", ".join(slx_params)
            r_lines.append(f"    private static native void native_{slx_name}({param_sig});")
        elif ret_slx == 'String':
            param_sig = ", ".join(slx_params)
            r_lines.append(f"    private static native char[] native_{slx_name}({param_sig});")
        else:
            param_sig = ", ".join(slx_params)
            r_lines.append(f"    private static native {ret_slx} native_{slx_name}({param_sig});")

    r_lines.append("}\n")
    solix_raylib_path.write_text("\n".join(r_lines))
    print(f"Generated {len(funcs)} methods in src/raylib/Raylib.slx")

    # 5. Generate C++ bridge
    cpp = [
        '#include "raylib.h"',
        '#include "solix/native.h"',
        '#include "solix/runtime.hpp"',
        '#include "solix/native_registry.hpp"',
        '#include <cstring>',
        '#include <string>',
        '#include <vector>',
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
        'static uint64_t allocate_solix_chars(solix::RuntimeContext &vm, const char *str) {',
        '    if (!str) return 0;',
        '    size_t len = std::strlen(str);',
        '    uint64_t addr = vm.memory.dynamic_allocation(static_cast<uint32_t>(len), solix::BlockTag::ARRAY_U8);',
        '    for (size_t i = 0; i < len; ++i) {',
        '        vm.memory.write_u8(static_cast<solix::Address>(addr), static_cast<uint32_t>(i), static_cast<uint8_t>(str[i]));',
        '    }',
        '    return addr;',
        '}',
        '',
    ]

    # Forward-declare all unpackers and packers
    cpp.append('// Forward Declarations of Struct Unpackers and Packers')
    for s in structs:
        sname = s['name']
        fn_suffix = sname.lower()
        c_type = f"::{sname}"
        cpp.append(f"inline {c_type} unpack_{fn_suffix}(solix::RuntimeContext &vm, uint64_t addr);")
        cpp.append(f"inline void pack_{fn_suffix}(solix::RuntimeContext &vm, uint64_t addr, const {c_type} &res);")
    cpp.append("inline ::Texture2D unpack_texture2d(solix::RuntimeContext &vm, uint64_t addr);")
    cpp.append("inline void pack_texture2d(solix::RuntimeContext &vm, uint64_t addr, const ::Texture2D &res);")
    cpp.append("inline ::RenderTexture2D unpack_rendertexture2d(solix::RuntimeContext &vm, uint64_t addr);")
    cpp.append("inline void pack_rendertexture2d(solix::RuntimeContext &vm, uint64_t addr, const ::RenderTexture2D &res);")
    cpp.append("inline ::Camera3D unpack_camera(solix::RuntimeContext &vm, uint64_t addr);")
    cpp.append("inline void pack_camera(solix::RuntimeContext &vm, uint64_t addr, const ::Camera3D &res);")
    cpp.append("inline ::Vector4 unpack_quaternion(solix::RuntimeContext &vm, uint64_t addr);")
    cpp.append("inline void pack_quaternion(solix::RuntimeContext &vm, uint64_t addr, const ::Vector4 &res);")
    cpp.append("")

    # Implement unpackers and packers
    for s in structs:
        sname = s['name']
        c_type = f"::{sname}"
        fn_suffix = sname.lower()
        fields = map_struct_fields(sname, s['fields'])

        # Unpacker
        cpp.append(f"inline {c_type} unpack_{fn_suffix}(solix::RuntimeContext &vm, uint64_t addr) {{")
        cpp.append(f"    if (addr == 0) return {c_type}{{}};")
        cpp.append(f"    uint32_t a = static_cast<uint32_t>(addr);")
        cpp.append(f"    {c_type} res{{}};")
        for f in fields:
            off = f['offset']
            fname = f['name']
            kind = f['kind']
            orig_t = f['orig_type']
            
            if kind == 'float':
                cpp.append(f"    {{ uint32_t raw = vm.memory.read_u32(a, {off}); std::memcpy(&res.{fname}, &raw, 4); }}")
            elif kind == 'double':
                cpp.append(f"    {{ uint64_t raw = vm.memory.read_u64(a, {off}); res.{fname} = bit_cast_from_u64<double>(raw); }}")
            elif kind == 'int':
                cpp.append(f"    res.{fname} = static_cast<int>(vm.memory.read_u32(a, {off}));")
            elif kind == 'uint':
                cpp.append(f"    res.{fname} = vm.memory.read_u32(a, {off});")
            elif kind == 'ushort':
                cpp.append(f"    res.{fname} = vm.memory.read_u16(a, {off});")
            elif kind == 'uchar':
                cpp.append(f"    res.{fname} = vm.memory.read_u8(a, {off});")
            elif kind == 'char':
                cpp.append(f"    res.{fname} = static_cast<char>(vm.memory.read_u8(a, {off}));")
            elif kind == 'bool':
                cpp.append(f"    res.{fname} = (vm.memory.read_u8(a, {off}) != 0);")
            elif kind == 'pointer':
                cpp.append(f"    res.{fname} = reinterpret_cast<{orig_t}>(vm.memory.read_u64(a, {off}));")
            elif kind == 'char_array_32':
                cpp.append(f"    {{ std::string s = read_solix_chars(vm, vm.memory.read_u64(a, {off})); std::strncpy(res.{fname}, s.c_str(), 31); res.{fname}[31] = '\\0'; }}")
            elif kind.startswith('struct_'):
                sub_t = kind[7:]
                cpp.append(f"    res.{fname} = unpack_{sub_t.lower()}(vm, vm.memory.read_u64(a, {off}));")
            elif kind.startswith('float4_'):
                parts = kind.split('_')
                idx = int(parts[1])
                real_f = parts[2]
                cpp.append(f"    {{ uint32_t raw = vm.memory.read_u32(a, {off}); std::memcpy(&res.{real_f}[{idx}], &raw, 4); }}")
            elif kind.startswith('int4_'):
                parts = kind.split('_')
                idx = int(parts[1])
                real_f = parts[2]
                cpp.append(f"    res.{real_f}[{idx}] = static_cast<int>(vm.memory.read_u32(a, {off}));")
            elif kind.startswith('float2_'):
                real_f = kind[7:]
                cpp.append(f"    {{ ::Vector2 tmp = unpack_vector2(vm, vm.memory.read_u64(a, {off})); res.{real_f}[0] = tmp.x; res.{real_f}[1] = tmp.y; }}")
            elif kind.startswith('matrix2_'):
                parts = kind.split('_')
                idx = int(parts[1])
                real_f = parts[2]
                cpp.append(f"    res.{real_f}[{idx}] = unpack_matrix(vm, vm.memory.read_u64(a, {off}));")
        cpp.append(f"    return res;")
        cpp.append(f"}}\n")

        # Packer
        cpp.append(f"inline void pack_{fn_suffix}(solix::RuntimeContext &vm, uint64_t addr, const {c_type} &res) {{")
        cpp.append(f"    if (addr == 0) return;")
        cpp.append(f"    uint32_t a = static_cast<uint32_t>(addr);")
        for f in fields:
            off = f['offset']
            fname = f['name']
            kind = f['kind']
            orig_t = f['orig_type']
            
            if kind == 'float':
                cpp.append(f"    {{ uint32_t raw; std::memcpy(&raw, &res.{fname}, 4); vm.memory.write_u32(a, {off}, raw); }}")
            elif kind == 'double':
                cpp.append(f"    vm.memory.write_u64(a, {off}, bit_cast_to_u64(res.{fname}));")
            elif kind == 'int':
                cpp.append(f"    vm.memory.write_u32(a, {off}, static_cast<uint32_t>(res.{fname}));")
            elif kind == 'uint':
                cpp.append(f"    vm.memory.write_u32(a, {off}, res.{fname});")
            elif kind == 'ushort':
                cpp.append(f"    vm.memory.write_u16(a, {off}, res.{fname});")
            elif kind == 'uchar':
                cpp.append(f"    vm.memory.write_u8(a, {off}, res.{fname});")
            elif kind == 'bool':
                cpp.append(f"    vm.memory.write_u8(a, {off}, res.{fname} ? 1 : 0);")
            elif kind == 'pointer':
                cpp.append(f"    vm.memory.write_u64(a, {off}, reinterpret_cast<uint64_t>(res.{fname}));")
            elif kind.startswith('struct_'):
                sub_t = kind[7:]
                cpp.append(f"    {{ uint64_t sub_addr = vm.memory.read_u64(a, {off}); if (sub_addr != 0) pack_{sub_t.lower()}(vm, sub_addr, res.{fname}); }}")
            elif kind.startswith('float4_'):
                parts = kind.split('_')
                idx = int(parts[1])
                real_f = parts[2]
                cpp.append(f"    {{ uint32_t raw; std::memcpy(&raw, &res.{real_f}[{idx}], 4); vm.memory.write_u32(a, {off}, raw); }}")
            elif kind.startswith('int4_'):
                parts = kind.split('_')
                idx = int(parts[1])
                real_f = parts[2]
                cpp.append(f"    vm.memory.write_u32(a, {off}, static_cast<uint32_t>(res.{real_f}[{idx}]));")
            elif kind.startswith('float2_'):
                real_f = kind[7:]
                cpp.append(f"    {{ uint64_t sub_addr = vm.memory.read_u64(a, {off}); if (sub_addr != 0) pack_vector2(vm, sub_addr, ::Vector2{{res.{real_f}[0], res.{real_f}[1]}}); }}")
            elif kind.startswith('matrix2_'):
                parts = kind.split('_')
                idx = int(parts[1])
                real_f = parts[2]
                cpp.append(f"    {{ uint64_t sub_addr = vm.memory.read_u64(a, {off}); if (sub_addr != 0) pack_matrix(vm, sub_addr, res.{real_f}[{idx}]); }}")
        cpp.append(f"}}\n")

    # Alias unpackers/packers
    cpp.append("inline ::Texture2D unpack_texture2d(solix::RuntimeContext &vm, uint64_t addr) { return unpack_texture(vm, addr); }")
    cpp.append("inline void pack_texture2d(solix::RuntimeContext &vm, uint64_t addr, const ::Texture2D &res) { pack_texture(vm, addr, res); }")
    cpp.append("inline ::RenderTexture2D unpack_rendertexture2d(solix::RuntimeContext &vm, uint64_t addr) { return unpack_rendertexture(vm, addr); }")
    cpp.append("inline void pack_rendertexture2d(solix::RuntimeContext &vm, uint64_t addr, const ::RenderTexture2D &res) { pack_rendertexture(vm, addr, res); }")
    cpp.append("inline ::Camera3D unpack_camera(solix::RuntimeContext &vm, uint64_t addr) { return unpack_camera3d(vm, addr); }")
    cpp.append("inline void pack_camera(solix::RuntimeContext &vm, uint64_t addr, const ::Camera3D &res) { pack_camera3d(vm, addr, res); }")
    cpp.append("inline ::Vector4 unpack_quaternion(solix::RuntimeContext &vm, uint64_t addr) { return unpack_vector4(vm, addr); }")
    cpp.append("inline void pack_quaternion(solix::RuntimeContext &vm, uint64_t addr, const ::Vector4 &res) { pack_vector4(vm, addr, res); }")
    cpp.append("")

    # Array unpackers
    cpp.append("""
static std::vector<::Vector2> unpack_vector2_array(solix::RuntimeContext &vm, uint64_t addr, int count) {
    if (count <= 0 && addr != 0) count = static_cast<int>(vm.memory.get_array_length(static_cast<solix::Address>(addr)));
    std::vector<::Vector2> res(count);
    if (addr == 0) return res;
    uint32_t a = static_cast<uint32_t>(addr);
    for (int i = 0; i < count; ++i) {
        uint64_t elem = vm.memory.read_u64(a, i * 8);
        res[i] = unpack_vector2(vm, elem);
    }
    return res;
}

static std::vector<::Vector3> unpack_vector3_array(solix::RuntimeContext &vm, uint64_t addr, int count) {
    if (count <= 0 && addr != 0) count = static_cast<int>(vm.memory.get_array_length(static_cast<solix::Address>(addr)));
    std::vector<::Vector3> res(count);
    if (addr == 0) return res;
    uint32_t a = static_cast<uint32_t>(addr);
    for (int i = 0; i < count; ++i) {
        uint64_t elem = vm.memory.read_u64(a, i * 8);
        res[i] = unpack_vector3(vm, elem);
    }
    return res;
}

static std::vector<::Color> unpack_color_array(solix::RuntimeContext &vm, uint64_t addr, int count) {
    if (count <= 0 && addr != 0) count = static_cast<int>(vm.memory.get_array_length(static_cast<solix::Address>(addr)));
    std::vector<::Color> res(count);
    if (addr == 0) return res;
    uint32_t a = static_cast<uint32_t>(addr);
    for (int i = 0; i < count; ++i) {
        uint64_t elem = vm.memory.read_u64(a, i * 8);
        res[i] = unpack_color(vm, elem);
    }
    return res;
}

static std::vector<int> unpack_int32_array(solix::RuntimeContext &vm, uint64_t addr, int count) {
    if (count <= 0 && addr != 0) count = static_cast<int>(vm.memory.get_array_length(static_cast<solix::Address>(addr)));
    std::vector<int> res(count);
    if (addr == 0) return res;
    uint32_t a = static_cast<uint32_t>(addr);
    for (int i = 0; i < count; ++i) {
        res[i] = static_cast<int>(vm.memory.read_u32(a, i * 4));
    }
    return res;
}

static std::vector<float> unpack_float32_array(solix::RuntimeContext &vm, uint64_t addr, int count) {
    if (count <= 0 && addr != 0) count = static_cast<int>(vm.memory.get_array_length(static_cast<solix::Address>(addr)));
    std::vector<float> res(count);
    if (addr == 0) return res;
    uint32_t a = static_cast<uint32_t>(addr);
    for (int i = 0; i < count; ++i) {
        uint32_t raw = vm.memory.read_u32(a, i * 4);
        std::memcpy(&res[i], &raw, 4);
    }
    return res;
}

static std::vector<unsigned char> unpack_uint8_array(solix::RuntimeContext &vm, uint64_t addr, int count) {
    if (count <= 0 && addr != 0) count = static_cast<int>(vm.memory.get_array_length(static_cast<solix::Address>(addr)));
    std::vector<unsigned char> res(count);
    if (addr == 0) return res;
    uint32_t a = static_cast<uint32_t>(addr);
    for (int i = 0; i < count; ++i) {
        res[i] = vm.memory.read_u8(a, i);
    }
    return res;
}


static std::vector<::Matrix> unpack_matrix_array(solix::RuntimeContext &vm, uint64_t addr, int count) {
    if (count <= 0 && addr != 0) count = static_cast<int>(vm.memory.get_array_length(static_cast<solix::Address>(addr)));
    std::vector<::Matrix> res(count);
    if (addr == 0) return res;
    uint32_t a = static_cast<uint32_t>(addr);
    for (int i = 0; i < count; ++i) {
        uint64_t elem = vm.memory.read_u64(a, i * 8);
        res[i] = unpack_matrix(vm, elem);
    }
    return res;
}

static std::vector<unsigned int> unpack_uint32_array(solix::RuntimeContext &vm, uint64_t addr, int count) {
    if (count <= 0 && addr != 0) count = static_cast<int>(vm.memory.get_array_length(static_cast<solix::Address>(addr)));
    std::vector<unsigned int> res(count);
    if (addr == 0) return res;
    uint32_t a = static_cast<uint32_t>(addr);
    for (int i = 0; i < count; ++i) {
        res[i] = vm.memory.read_u32(a, i * 4);
    }
    return res;
}
""")

    # Generate C++ wrapper functions
    for f in funcs:
        c_name = f['name']
        slx_name = to_snake_case(c_name)
        ret_c = f.get('returnType', 'void')
        ret_slx = map_c_type_to_solix(ret_c)
        params = f.get('params', [])

        cpp.append(f"static uint64_t wrap_{slx_name}(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {{")
        cpp.append("    (void)self; (void)argc;")

        c_args = []
        post_calls = []

        # Find count parameter if any for arrays
        for i, p in enumerate(params):
            ptype = p['type'].strip()
            arg_var = f"a{i}"
            
            # 1. Primitives
            if ptype in ('int', 'int32_t'):
                cpp.append(f"    int {arg_var} = static_cast<int>(args[{i}]);")
                c_args.append(arg_var)
            elif ptype in ('unsigned int', 'uint32_t'):
                cpp.append(f"    unsigned int {arg_var} = static_cast<unsigned int>(args[{i}]);")
                c_args.append(arg_var)
            elif ptype == 'unsigned short':
                cpp.append(f"    unsigned short {arg_var} = static_cast<unsigned short>(args[{i}]);")
                c_args.append(arg_var)
            elif ptype == 'unsigned char':
                cpp.append(f"    unsigned char {arg_var} = static_cast<unsigned char>(args[{i}]);")
                c_args.append(arg_var)
            elif ptype == 'float':
                cpp.append(f"    float {arg_var} = unpack_f32_arg(args[{i}]);")
                c_args.append(arg_var)
            elif ptype == 'double':
                cpp.append(f"    double {arg_var} = bit_cast_from_u64<double>(args[{i}]);")
                c_args.append(arg_var)
            elif ptype == 'bool':
                cpp.append(f"    bool {arg_var} = (args[{i}] != 0);")
                c_args.append(arg_var)
            elif ptype == 'const char *':
                cpp.append(f"    std::string s_{arg_var} = read_solix_chars(vm, args[{i}]);")
                cpp.append(f"    const char *{arg_var} = s_{arg_var}.c_str();")
                c_args.append(arg_var)
            elif ptype == 'char *':
                cpp.append(f"    std::string s_{arg_var} = read_solix_chars(vm, args[{i}]);")
                cpp.append(f"    char *{arg_var} = const_cast<char *>(s_{arg_var}.c_str());")
                c_args.append(arg_var)
            elif ptype in ('void *', 'const void *'):
                cpp.append(f"    void *{arg_var} = reinterpret_cast<void *>(args[{i}]);")
                c_args.append(arg_var)
            
            # 2. In-place struct pointers (Image *, Camera *, Camera3D *, etc.)
            elif ptype.endswith('*') and ptype[:-1].strip() in KNOWN_STRUCTS and ptype[:-1].strip() in ('Image', 'Camera', 'Camera3D', 'Camera2D', 'Wave', 'Mesh', 'Model', 'AutomationEventList', 'Texture', 'Texture2D'):
                st = ptype[:-1].strip()
                c_type_name = f"::{st}"
                fn_suf = st.lower()
                cpp.append(f"    {c_type_name} {arg_var} = unpack_{fn_suf}(vm, args[{i}]);")
                c_args.append(f"&{arg_var}")
                post_calls.append(f"    pack_{fn_suf}(vm, args[{i}], {arg_var});")

            # 3. Value structs
            elif ptype in KNOWN_STRUCTS:
                fn_suf = ptype.lower()
                cpp.append(f"    ::{ptype} {arg_var} = unpack_{fn_suf}(vm, args[{i}]);")
                c_args.append(arg_var)

            # 4. Arrays
            elif ptype in ('const Vector2 *', 'Vector2 *'):
                cpp.append(f"    auto v_{arg_var} = unpack_vector2_array(vm, args[{i}], 0);")
                c_args.append(f"v_{arg_var}.data()")
            elif ptype in ('const Vector3 *', 'Vector3 *'):
                cpp.append(f"    auto v_{arg_var} = unpack_vector3_array(vm, args[{i}], 0);")
                c_args.append(f"v_{arg_var}.data()")
            elif ptype in ('const Matrix *', 'Matrix *'):
                cpp.append(f"    auto v_{arg_var} = unpack_matrix_array(vm, args[{i}], 0);")
                c_args.append(f"v_{arg_var}.data()")
            elif ptype in ('const Color *', 'Color *'):
                cpp.append(f"    auto v_{arg_var} = unpack_color_array(vm, args[{i}], 0);")
                c_args.append(f"v_{arg_var}.data()")
            elif ptype in ('const int *', 'int *'):
                cpp.append(f"    auto v_{arg_var} = unpack_int32_array(vm, args[{i}], 0);")
                c_args.append(f"v_{arg_var}.data()")
            elif ptype in ('const float *', 'float *'):
                cpp.append(f"    auto v_{arg_var} = unpack_float32_array(vm, args[{i}], 0);")
                c_args.append(f"v_{arg_var}.data()")
            elif ptype in ('const unsigned char *', 'unsigned char *'):
                cpp.append(f"    auto v_{arg_var} = unpack_uint8_array(vm, args[{i}], 0);")
                c_args.append(f"v_{arg_var}.data()")
            elif ptype in ('const unsigned int *', 'unsigned int *'):
                cpp.append(f"    auto v_{arg_var} = unpack_uint32_array(vm, args[{i}], 0);")
                c_args.append(f"v_{arg_var}.data()")
            elif ptype in ('char **', 'const char **'):
                cpp.append(f"    char **{arg_var} = nullptr; // string array slice")
                c_args.append(arg_var)
            else:
                if ptype.endswith('*'):
                    base_t = ptype[:-1].strip()
                    if base_t in KNOWN_STRUCTS:
                        cpp.append(f"    ::{base_t} *{arg_var} = reinterpret_cast<::{base_t} *>(args[{i}]);")
                    else:
                        cpp.append(f"    {ptype} {arg_var} = reinterpret_cast<{ptype}>(args[{i}]);")
                else:
                    cpp.append(f"    uint64_t {arg_var} = args[{i}];")
                c_args.append(arg_var)

        c_call = f"::{c_name}({', '.join(c_args)})"

        # Return handling
        is_ret_struct = ret_slx in struct_names_set
        if is_ret_struct:
            out_idx = len(params)
            cpp.append(f"    auto res = {c_call};")
            for pc in post_calls:
                cpp.append(pc)
            fn_suf = ret_slx.lower()
            cpp.append(f"    pack_{fn_suf}(vm, args[{out_idx}], res);")
            cpp.append("    return 0;")
        elif ret_c == 'void':
            cpp.append(f"    {c_call};")
            for pc in post_calls:
                cpp.append(pc)
            cpp.append("    return 0;")
        elif ret_c in ('const char *', 'char *'):
            cpp.append(f"    const char *res = {c_call};")
            for pc in post_calls:
                cpp.append(pc)
            cpp.append("    return allocate_solix_chars(vm, res);")
        elif ret_c == 'bool':
            cpp.append(f"    bool res = {c_call};")
            for pc in post_calls:
                cpp.append(pc)
            cpp.append("    return res ? 1 : 0;")
        elif ret_c in ('int', 'int32_t', 'unsigned int', 'uint32_t', 'unsigned short', 'unsigned char', 'long'):
            cpp.append(f"    auto res = {c_call};")
            for pc in post_calls:
                cpp.append(pc)
            cpp.append("    return static_cast<uint64_t>(res);")
        elif ret_c == 'float':
            cpp.append(f"    float res = {c_call};")
            for pc in post_calls:
                cpp.append(pc)
            cpp.append("    return pack_f32_ret(res);")
        elif ret_c == 'double':
            cpp.append(f"    double res = {c_call};")
            for pc in post_calls:
                cpp.append(pc)
            cpp.append("    return bit_cast_to_u64<double>(res);")
        else:
            cpp.append(f"    auto res = {c_call};")
            for pc in post_calls:
                cpp.append(pc)
            cpp.append("    return reinterpret_cast<uint64_t>(res);")

        cpp.append("}\n")

    cpp.append("} // namespace\n")
    cpp.append("void register_raylib_natives(solix::NativeRegistry &registry) {")
    for f in funcs:
        c_name = f['name']
        slx_name = to_snake_case(c_name)
        cpp.append(f'    registry.register_function("Raylib_native_{slx_name}", wrap_{slx_name});')
        cpp.append(f'    registry.register_function("raylib_Raylib_native_{slx_name}", wrap_{slx_name});')
        cpp.append(f'    registry.register_function("native_{slx_name}", wrap_{slx_name});')
    cpp.append("}\n")

    cpp_bridge_path.write_text("\n".join(cpp))
    print(f"Generated C++ bridge in {cpp_bridge_path}")

    # 6. Update manifest.json
    with open(manifest_path) as mf_file:
        mf = json.load(mf_file)
    src_files = sorted([s for s in root.glob("src/**/*.slx") if s.name != "main.slx"])
    deps = [d for d in mf.get("dependencies", []) if d.get("type") != "source"]
    for s in src_files:
        rel = str(s.relative_to(root))
        deps.append({"type": "source", "path": rel})
    mf["dependencies"] = deps
    with open(manifest_path, "w") as mf_file:
        json.dump(mf, mf_file, indent=2)
    print(f"Updated manifest.json with {len(src_files)} files.")

if __name__ == "__main__":
    main()

