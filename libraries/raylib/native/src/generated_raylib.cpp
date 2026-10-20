#include "raylib.h"
#include "solix/native.h"
#include "solix/runtime.hpp"
#include "solix/native_registry.hpp"
#include <cstring>
#include <string>
#include <vector>
#include <cstdint>
#include <cmath>

namespace {

template <typename T> inline uint64_t bit_cast_to_u64(T value) {
    uint64_t result = 0;
    std::memcpy(&result, &value, sizeof(T));
    return result;
}

template <typename T> inline T bit_cast_from_u64(uint64_t value) {
    T result;
    std::memcpy(&result, &value, sizeof(T));
    return result;
}

inline float unpack_f32_arg(uint64_t arg) {
    double d = bit_cast_from_u64<double>(arg);
    if (arg != 0 && (std::isnan(d) || std::abs(d) < 1e-35 || std::abs(d) > 1e35)) {
        uint32_t raw = static_cast<uint32_t>(arg & 0xFFFFFFFF);
        float f;
        std::memcpy(&f, &raw, 4);
        return f;
    }
    return static_cast<float>(d);
}

inline uint64_t pack_f32_ret(float res) {
    return bit_cast_to_u64<double>(static_cast<double>(res));
}

static std::string read_solix_chars(solix::RuntimeContext &vm, uint64_t addr) {
    if (addr == 0) return "";
    uint32_t len = vm.memory.get_array_length(static_cast<solix::Address>(addr));
    std::string s;
    s.reserve(len);
    for (uint32_t i = 0; i < len; ++i) {
        s.push_back(vm.memory.read_char(static_cast<solix::Address>(addr), i));
    }
    return s;
}

static uint64_t allocate_solix_chars(solix::RuntimeContext &vm, const char *str) {
    if (!str) return 0;
    size_t len = std::strlen(str);
    uint64_t addr = vm.memory.dynamic_allocation(static_cast<uint32_t>(len), solix::BlockTag::ARRAY_U8);
    for (size_t i = 0; i < len; ++i) {
        vm.memory.write_u8(static_cast<solix::Address>(addr), static_cast<uint32_t>(i), static_cast<uint8_t>(str[i]));
    }
    return addr;
}

// Forward Declarations of Struct Unpackers and Packers
inline ::Vector2 unpack_vector2(solix::RuntimeContext &vm, uint64_t addr);
inline void pack_vector2(solix::RuntimeContext &vm, uint64_t addr, const ::Vector2 &res);
inline ::Vector3 unpack_vector3(solix::RuntimeContext &vm, uint64_t addr);
inline void pack_vector3(solix::RuntimeContext &vm, uint64_t addr, const ::Vector3 &res);
inline ::Vector4 unpack_vector4(solix::RuntimeContext &vm, uint64_t addr);
inline void pack_vector4(solix::RuntimeContext &vm, uint64_t addr, const ::Vector4 &res);
inline ::Matrix unpack_matrix(solix::RuntimeContext &vm, uint64_t addr);
inline void pack_matrix(solix::RuntimeContext &vm, uint64_t addr, const ::Matrix &res);
inline ::Color unpack_color(solix::RuntimeContext &vm, uint64_t addr);
inline void pack_color(solix::RuntimeContext &vm, uint64_t addr, const ::Color &res);
inline ::Rectangle unpack_rectangle(solix::RuntimeContext &vm, uint64_t addr);
inline void pack_rectangle(solix::RuntimeContext &vm, uint64_t addr, const ::Rectangle &res);
inline ::Image unpack_image(solix::RuntimeContext &vm, uint64_t addr);
inline void pack_image(solix::RuntimeContext &vm, uint64_t addr, const ::Image &res);
inline ::Texture unpack_texture(solix::RuntimeContext &vm, uint64_t addr);
inline void pack_texture(solix::RuntimeContext &vm, uint64_t addr, const ::Texture &res);
inline ::RenderTexture unpack_rendertexture(solix::RuntimeContext &vm, uint64_t addr);
inline void pack_rendertexture(solix::RuntimeContext &vm, uint64_t addr, const ::RenderTexture &res);
inline ::NPatchInfo unpack_npatchinfo(solix::RuntimeContext &vm, uint64_t addr);
inline void pack_npatchinfo(solix::RuntimeContext &vm, uint64_t addr, const ::NPatchInfo &res);
inline ::GlyphInfo unpack_glyphinfo(solix::RuntimeContext &vm, uint64_t addr);
inline void pack_glyphinfo(solix::RuntimeContext &vm, uint64_t addr, const ::GlyphInfo &res);
inline ::Font unpack_font(solix::RuntimeContext &vm, uint64_t addr);
inline void pack_font(solix::RuntimeContext &vm, uint64_t addr, const ::Font &res);
inline ::Camera3D unpack_camera3d(solix::RuntimeContext &vm, uint64_t addr);
inline void pack_camera3d(solix::RuntimeContext &vm, uint64_t addr, const ::Camera3D &res);
inline ::Camera2D unpack_camera2d(solix::RuntimeContext &vm, uint64_t addr);
inline void pack_camera2d(solix::RuntimeContext &vm, uint64_t addr, const ::Camera2D &res);
inline ::Mesh unpack_mesh(solix::RuntimeContext &vm, uint64_t addr);
inline void pack_mesh(solix::RuntimeContext &vm, uint64_t addr, const ::Mesh &res);
inline ::Shader unpack_shader(solix::RuntimeContext &vm, uint64_t addr);
inline void pack_shader(solix::RuntimeContext &vm, uint64_t addr, const ::Shader &res);
inline ::MaterialMap unpack_materialmap(solix::RuntimeContext &vm, uint64_t addr);
inline void pack_materialmap(solix::RuntimeContext &vm, uint64_t addr, const ::MaterialMap &res);
inline ::Material unpack_material(solix::RuntimeContext &vm, uint64_t addr);
inline void pack_material(solix::RuntimeContext &vm, uint64_t addr, const ::Material &res);
inline ::Transform unpack_transform(solix::RuntimeContext &vm, uint64_t addr);
inline void pack_transform(solix::RuntimeContext &vm, uint64_t addr, const ::Transform &res);
inline ::BoneInfo unpack_boneinfo(solix::RuntimeContext &vm, uint64_t addr);
inline void pack_boneinfo(solix::RuntimeContext &vm, uint64_t addr, const ::BoneInfo &res);
inline ::ModelSkeleton unpack_modelskeleton(solix::RuntimeContext &vm, uint64_t addr);
inline void pack_modelskeleton(solix::RuntimeContext &vm, uint64_t addr, const ::ModelSkeleton &res);
inline ::Model unpack_model(solix::RuntimeContext &vm, uint64_t addr);
inline void pack_model(solix::RuntimeContext &vm, uint64_t addr, const ::Model &res);
inline ::ModelAnimation unpack_modelanimation(solix::RuntimeContext &vm, uint64_t addr);
inline void pack_modelanimation(solix::RuntimeContext &vm, uint64_t addr, const ::ModelAnimation &res);
inline ::Ray unpack_ray(solix::RuntimeContext &vm, uint64_t addr);
inline void pack_ray(solix::RuntimeContext &vm, uint64_t addr, const ::Ray &res);
inline ::RayCollision unpack_raycollision(solix::RuntimeContext &vm, uint64_t addr);
inline void pack_raycollision(solix::RuntimeContext &vm, uint64_t addr, const ::RayCollision &res);
inline ::BoundingBox unpack_boundingbox(solix::RuntimeContext &vm, uint64_t addr);
inline void pack_boundingbox(solix::RuntimeContext &vm, uint64_t addr, const ::BoundingBox &res);
inline ::Wave unpack_wave(solix::RuntimeContext &vm, uint64_t addr);
inline void pack_wave(solix::RuntimeContext &vm, uint64_t addr, const ::Wave &res);
inline ::AudioStream unpack_audiostream(solix::RuntimeContext &vm, uint64_t addr);
inline void pack_audiostream(solix::RuntimeContext &vm, uint64_t addr, const ::AudioStream &res);
inline ::Sound unpack_sound(solix::RuntimeContext &vm, uint64_t addr);
inline void pack_sound(solix::RuntimeContext &vm, uint64_t addr, const ::Sound &res);
inline ::Music unpack_music(solix::RuntimeContext &vm, uint64_t addr);
inline void pack_music(solix::RuntimeContext &vm, uint64_t addr, const ::Music &res);
inline ::VrDeviceInfo unpack_vrdeviceinfo(solix::RuntimeContext &vm, uint64_t addr);
inline void pack_vrdeviceinfo(solix::RuntimeContext &vm, uint64_t addr, const ::VrDeviceInfo &res);
inline ::VrStereoConfig unpack_vrstereoconfig(solix::RuntimeContext &vm, uint64_t addr);
inline void pack_vrstereoconfig(solix::RuntimeContext &vm, uint64_t addr, const ::VrStereoConfig &res);
inline ::FilePathList unpack_filepathlist(solix::RuntimeContext &vm, uint64_t addr);
inline void pack_filepathlist(solix::RuntimeContext &vm, uint64_t addr, const ::FilePathList &res);
inline ::AutomationEvent unpack_automationevent(solix::RuntimeContext &vm, uint64_t addr);
inline void pack_automationevent(solix::RuntimeContext &vm, uint64_t addr, const ::AutomationEvent &res);
inline ::AutomationEventList unpack_automationeventlist(solix::RuntimeContext &vm, uint64_t addr);
inline void pack_automationeventlist(solix::RuntimeContext &vm, uint64_t addr, const ::AutomationEventList &res);
inline ::Texture2D unpack_texture2d(solix::RuntimeContext &vm, uint64_t addr);
inline void pack_texture2d(solix::RuntimeContext &vm, uint64_t addr, const ::Texture2D &res);
inline ::RenderTexture2D unpack_rendertexture2d(solix::RuntimeContext &vm, uint64_t addr);
inline void pack_rendertexture2d(solix::RuntimeContext &vm, uint64_t addr, const ::RenderTexture2D &res);
inline ::Camera3D unpack_camera(solix::RuntimeContext &vm, uint64_t addr);
inline void pack_camera(solix::RuntimeContext &vm, uint64_t addr, const ::Camera3D &res);
inline ::Vector4 unpack_quaternion(solix::RuntimeContext &vm, uint64_t addr);
inline void pack_quaternion(solix::RuntimeContext &vm, uint64_t addr, const ::Vector4 &res);

inline ::Vector2 unpack_vector2(solix::RuntimeContext &vm, uint64_t addr) {
    if (addr == 0) return ::Vector2{};
    uint32_t a = static_cast<uint32_t>(addr);
    ::Vector2 res{};
    { uint32_t raw = vm.memory.read_u32(a, 4); std::memcpy(&res.x, &raw, 4); }
    { uint32_t raw = vm.memory.read_u32(a, 8); std::memcpy(&res.y, &raw, 4); }
    return res;
}

inline void pack_vector2(solix::RuntimeContext &vm, uint64_t addr, const ::Vector2 &res) {
    if (addr == 0) return;
    uint32_t a = static_cast<uint32_t>(addr);
    { uint32_t raw; std::memcpy(&raw, &res.x, 4); vm.memory.write_u32(a, 4, raw); }
    { uint32_t raw; std::memcpy(&raw, &res.y, 4); vm.memory.write_u32(a, 8, raw); }
}

inline ::Vector3 unpack_vector3(solix::RuntimeContext &vm, uint64_t addr) {
    if (addr == 0) return ::Vector3{};
    uint32_t a = static_cast<uint32_t>(addr);
    ::Vector3 res{};
    { uint32_t raw = vm.memory.read_u32(a, 4); std::memcpy(&res.x, &raw, 4); }
    { uint32_t raw = vm.memory.read_u32(a, 8); std::memcpy(&res.y, &raw, 4); }
    { uint32_t raw = vm.memory.read_u32(a, 12); std::memcpy(&res.z, &raw, 4); }
    return res;
}

inline void pack_vector3(solix::RuntimeContext &vm, uint64_t addr, const ::Vector3 &res) {
    if (addr == 0) return;
    uint32_t a = static_cast<uint32_t>(addr);
    { uint32_t raw; std::memcpy(&raw, &res.x, 4); vm.memory.write_u32(a, 4, raw); }
    { uint32_t raw; std::memcpy(&raw, &res.y, 4); vm.memory.write_u32(a, 8, raw); }
    { uint32_t raw; std::memcpy(&raw, &res.z, 4); vm.memory.write_u32(a, 12, raw); }
}

inline ::Vector4 unpack_vector4(solix::RuntimeContext &vm, uint64_t addr) {
    if (addr == 0) return ::Vector4{};
    uint32_t a = static_cast<uint32_t>(addr);
    ::Vector4 res{};
    { uint32_t raw = vm.memory.read_u32(a, 4); std::memcpy(&res.x, &raw, 4); }
    { uint32_t raw = vm.memory.read_u32(a, 8); std::memcpy(&res.y, &raw, 4); }
    { uint32_t raw = vm.memory.read_u32(a, 12); std::memcpy(&res.z, &raw, 4); }
    { uint32_t raw = vm.memory.read_u32(a, 16); std::memcpy(&res.w, &raw, 4); }
    return res;
}

inline void pack_vector4(solix::RuntimeContext &vm, uint64_t addr, const ::Vector4 &res) {
    if (addr == 0) return;
    uint32_t a = static_cast<uint32_t>(addr);
    { uint32_t raw; std::memcpy(&raw, &res.x, 4); vm.memory.write_u32(a, 4, raw); }
    { uint32_t raw; std::memcpy(&raw, &res.y, 4); vm.memory.write_u32(a, 8, raw); }
    { uint32_t raw; std::memcpy(&raw, &res.z, 4); vm.memory.write_u32(a, 12, raw); }
    { uint32_t raw; std::memcpy(&raw, &res.w, 4); vm.memory.write_u32(a, 16, raw); }
}

inline ::Matrix unpack_matrix(solix::RuntimeContext &vm, uint64_t addr) {
    if (addr == 0) return ::Matrix{};
    uint32_t a = static_cast<uint32_t>(addr);
    ::Matrix res{};
    { uint32_t raw = vm.memory.read_u32(a, 4); std::memcpy(&res.m0, &raw, 4); }
    { uint32_t raw = vm.memory.read_u32(a, 8); std::memcpy(&res.m4, &raw, 4); }
    { uint32_t raw = vm.memory.read_u32(a, 12); std::memcpy(&res.m8, &raw, 4); }
    { uint32_t raw = vm.memory.read_u32(a, 16); std::memcpy(&res.m12, &raw, 4); }
    { uint32_t raw = vm.memory.read_u32(a, 20); std::memcpy(&res.m1, &raw, 4); }
    { uint32_t raw = vm.memory.read_u32(a, 24); std::memcpy(&res.m5, &raw, 4); }
    { uint32_t raw = vm.memory.read_u32(a, 28); std::memcpy(&res.m9, &raw, 4); }
    { uint32_t raw = vm.memory.read_u32(a, 32); std::memcpy(&res.m13, &raw, 4); }
    { uint32_t raw = vm.memory.read_u32(a, 36); std::memcpy(&res.m2, &raw, 4); }
    { uint32_t raw = vm.memory.read_u32(a, 40); std::memcpy(&res.m6, &raw, 4); }
    { uint32_t raw = vm.memory.read_u32(a, 44); std::memcpy(&res.m10, &raw, 4); }
    { uint32_t raw = vm.memory.read_u32(a, 48); std::memcpy(&res.m14, &raw, 4); }
    { uint32_t raw = vm.memory.read_u32(a, 52); std::memcpy(&res.m3, &raw, 4); }
    { uint32_t raw = vm.memory.read_u32(a, 56); std::memcpy(&res.m7, &raw, 4); }
    { uint32_t raw = vm.memory.read_u32(a, 60); std::memcpy(&res.m11, &raw, 4); }
    { uint32_t raw = vm.memory.read_u32(a, 64); std::memcpy(&res.m15, &raw, 4); }
    return res;
}

inline void pack_matrix(solix::RuntimeContext &vm, uint64_t addr, const ::Matrix &res) {
    if (addr == 0) return;
    uint32_t a = static_cast<uint32_t>(addr);
    { uint32_t raw; std::memcpy(&raw, &res.m0, 4); vm.memory.write_u32(a, 4, raw); }
    { uint32_t raw; std::memcpy(&raw, &res.m4, 4); vm.memory.write_u32(a, 8, raw); }
    { uint32_t raw; std::memcpy(&raw, &res.m8, 4); vm.memory.write_u32(a, 12, raw); }
    { uint32_t raw; std::memcpy(&raw, &res.m12, 4); vm.memory.write_u32(a, 16, raw); }
    { uint32_t raw; std::memcpy(&raw, &res.m1, 4); vm.memory.write_u32(a, 20, raw); }
    { uint32_t raw; std::memcpy(&raw, &res.m5, 4); vm.memory.write_u32(a, 24, raw); }
    { uint32_t raw; std::memcpy(&raw, &res.m9, 4); vm.memory.write_u32(a, 28, raw); }
    { uint32_t raw; std::memcpy(&raw, &res.m13, 4); vm.memory.write_u32(a, 32, raw); }
    { uint32_t raw; std::memcpy(&raw, &res.m2, 4); vm.memory.write_u32(a, 36, raw); }
    { uint32_t raw; std::memcpy(&raw, &res.m6, 4); vm.memory.write_u32(a, 40, raw); }
    { uint32_t raw; std::memcpy(&raw, &res.m10, 4); vm.memory.write_u32(a, 44, raw); }
    { uint32_t raw; std::memcpy(&raw, &res.m14, 4); vm.memory.write_u32(a, 48, raw); }
    { uint32_t raw; std::memcpy(&raw, &res.m3, 4); vm.memory.write_u32(a, 52, raw); }
    { uint32_t raw; std::memcpy(&raw, &res.m7, 4); vm.memory.write_u32(a, 56, raw); }
    { uint32_t raw; std::memcpy(&raw, &res.m11, 4); vm.memory.write_u32(a, 60, raw); }
    { uint32_t raw; std::memcpy(&raw, &res.m15, 4); vm.memory.write_u32(a, 64, raw); }
}

inline ::Color unpack_color(solix::RuntimeContext &vm, uint64_t addr) {
    if (addr == 0) return ::Color{};
    uint32_t a = static_cast<uint32_t>(addr);
    ::Color res{};
    res.r = vm.memory.read_u8(a, 4);
    res.g = vm.memory.read_u8(a, 5);
    res.b = vm.memory.read_u8(a, 6);
    res.a = vm.memory.read_u8(a, 7);
    return res;
}

inline void pack_color(solix::RuntimeContext &vm, uint64_t addr, const ::Color &res) {
    if (addr == 0) return;
    uint32_t a = static_cast<uint32_t>(addr);
    vm.memory.write_u8(a, 4, res.r);
    vm.memory.write_u8(a, 5, res.g);
    vm.memory.write_u8(a, 6, res.b);
    vm.memory.write_u8(a, 7, res.a);
}

inline ::Rectangle unpack_rectangle(solix::RuntimeContext &vm, uint64_t addr) {
    if (addr == 0) return ::Rectangle{};
    uint32_t a = static_cast<uint32_t>(addr);
    ::Rectangle res{};
    { uint32_t raw = vm.memory.read_u32(a, 4); std::memcpy(&res.x, &raw, 4); }
    { uint32_t raw = vm.memory.read_u32(a, 8); std::memcpy(&res.y, &raw, 4); }
    { uint32_t raw = vm.memory.read_u32(a, 12); std::memcpy(&res.width, &raw, 4); }
    { uint32_t raw = vm.memory.read_u32(a, 16); std::memcpy(&res.height, &raw, 4); }
    return res;
}

inline void pack_rectangle(solix::RuntimeContext &vm, uint64_t addr, const ::Rectangle &res) {
    if (addr == 0) return;
    uint32_t a = static_cast<uint32_t>(addr);
    { uint32_t raw; std::memcpy(&raw, &res.x, 4); vm.memory.write_u32(a, 4, raw); }
    { uint32_t raw; std::memcpy(&raw, &res.y, 4); vm.memory.write_u32(a, 8, raw); }
    { uint32_t raw; std::memcpy(&raw, &res.width, 4); vm.memory.write_u32(a, 12, raw); }
    { uint32_t raw; std::memcpy(&raw, &res.height, 4); vm.memory.write_u32(a, 16, raw); }
}

inline ::Image unpack_image(solix::RuntimeContext &vm, uint64_t addr) {
    if (addr == 0) return ::Image{};
    uint32_t a = static_cast<uint32_t>(addr);
    ::Image res{};
    res.data = reinterpret_cast<void *>(vm.memory.read_u64(a, 8));
    res.width = static_cast<int>(vm.memory.read_u32(a, 16));
    res.height = static_cast<int>(vm.memory.read_u32(a, 20));
    res.mipmaps = static_cast<int>(vm.memory.read_u32(a, 24));
    res.format = static_cast<int>(vm.memory.read_u32(a, 28));
    return res;
}

inline void pack_image(solix::RuntimeContext &vm, uint64_t addr, const ::Image &res) {
    if (addr == 0) return;
    uint32_t a = static_cast<uint32_t>(addr);
    vm.memory.write_u64(a, 8, reinterpret_cast<uint64_t>(res.data));
    vm.memory.write_u32(a, 16, static_cast<uint32_t>(res.width));
    vm.memory.write_u32(a, 20, static_cast<uint32_t>(res.height));
    vm.memory.write_u32(a, 24, static_cast<uint32_t>(res.mipmaps));
    vm.memory.write_u32(a, 28, static_cast<uint32_t>(res.format));
}

inline ::Texture unpack_texture(solix::RuntimeContext &vm, uint64_t addr) {
    if (addr == 0) return ::Texture{};
    uint32_t a = static_cast<uint32_t>(addr);
    ::Texture res{};
    res.id = vm.memory.read_u32(a, 4);
    res.width = static_cast<int>(vm.memory.read_u32(a, 8));
    res.height = static_cast<int>(vm.memory.read_u32(a, 12));
    res.mipmaps = static_cast<int>(vm.memory.read_u32(a, 16));
    res.format = static_cast<int>(vm.memory.read_u32(a, 20));
    return res;
}

inline void pack_texture(solix::RuntimeContext &vm, uint64_t addr, const ::Texture &res) {
    if (addr == 0) return;
    uint32_t a = static_cast<uint32_t>(addr);
    vm.memory.write_u32(a, 4, res.id);
    vm.memory.write_u32(a, 8, static_cast<uint32_t>(res.width));
    vm.memory.write_u32(a, 12, static_cast<uint32_t>(res.height));
    vm.memory.write_u32(a, 16, static_cast<uint32_t>(res.mipmaps));
    vm.memory.write_u32(a, 20, static_cast<uint32_t>(res.format));
}

inline ::RenderTexture unpack_rendertexture(solix::RuntimeContext &vm, uint64_t addr) {
    if (addr == 0) return ::RenderTexture{};
    uint32_t a = static_cast<uint32_t>(addr);
    ::RenderTexture res{};
    res.id = vm.memory.read_u32(a, 4);
    res.texture = unpack_texture2d(vm, vm.memory.read_u64(a, 8));
    res.depth = unpack_texture2d(vm, vm.memory.read_u64(a, 16));
    return res;
}

inline void pack_rendertexture(solix::RuntimeContext &vm, uint64_t addr, const ::RenderTexture &res) {
    if (addr == 0) return;
    uint32_t a = static_cast<uint32_t>(addr);
    vm.memory.write_u32(a, 4, res.id);
    { uint64_t sub_addr = vm.memory.read_u64(a, 8); if (sub_addr != 0) pack_texture2d(vm, sub_addr, res.texture); }
    { uint64_t sub_addr = vm.memory.read_u64(a, 16); if (sub_addr != 0) pack_texture2d(vm, sub_addr, res.depth); }
}

inline ::NPatchInfo unpack_npatchinfo(solix::RuntimeContext &vm, uint64_t addr) {
    if (addr == 0) return ::NPatchInfo{};
    uint32_t a = static_cast<uint32_t>(addr);
    ::NPatchInfo res{};
    res.source = unpack_rectangle(vm, vm.memory.read_u64(a, 8));
    res.left = static_cast<int>(vm.memory.read_u32(a, 16));
    res.top = static_cast<int>(vm.memory.read_u32(a, 20));
    res.right = static_cast<int>(vm.memory.read_u32(a, 24));
    res.bottom = static_cast<int>(vm.memory.read_u32(a, 28));
    res.layout = static_cast<int>(vm.memory.read_u32(a, 32));
    return res;
}

inline void pack_npatchinfo(solix::RuntimeContext &vm, uint64_t addr, const ::NPatchInfo &res) {
    if (addr == 0) return;
    uint32_t a = static_cast<uint32_t>(addr);
    { uint64_t sub_addr = vm.memory.read_u64(a, 8); if (sub_addr != 0) pack_rectangle(vm, sub_addr, res.source); }
    vm.memory.write_u32(a, 16, static_cast<uint32_t>(res.left));
    vm.memory.write_u32(a, 20, static_cast<uint32_t>(res.top));
    vm.memory.write_u32(a, 24, static_cast<uint32_t>(res.right));
    vm.memory.write_u32(a, 28, static_cast<uint32_t>(res.bottom));
    vm.memory.write_u32(a, 32, static_cast<uint32_t>(res.layout));
}

inline ::GlyphInfo unpack_glyphinfo(solix::RuntimeContext &vm, uint64_t addr) {
    if (addr == 0) return ::GlyphInfo{};
    uint32_t a = static_cast<uint32_t>(addr);
    ::GlyphInfo res{};
    res.value = static_cast<int>(vm.memory.read_u32(a, 4));
    res.offsetX = static_cast<int>(vm.memory.read_u32(a, 8));
    res.offsetY = static_cast<int>(vm.memory.read_u32(a, 12));
    res.advanceX = static_cast<int>(vm.memory.read_u32(a, 16));
    res.image = unpack_image(vm, vm.memory.read_u64(a, 24));
    return res;
}

inline void pack_glyphinfo(solix::RuntimeContext &vm, uint64_t addr, const ::GlyphInfo &res) {
    if (addr == 0) return;
    uint32_t a = static_cast<uint32_t>(addr);
    vm.memory.write_u32(a, 4, static_cast<uint32_t>(res.value));
    vm.memory.write_u32(a, 8, static_cast<uint32_t>(res.offsetX));
    vm.memory.write_u32(a, 12, static_cast<uint32_t>(res.offsetY));
    vm.memory.write_u32(a, 16, static_cast<uint32_t>(res.advanceX));
    { uint64_t sub_addr = vm.memory.read_u64(a, 24); if (sub_addr != 0) pack_image(vm, sub_addr, res.image); }
}

inline ::Font unpack_font(solix::RuntimeContext &vm, uint64_t addr) {
    if (addr == 0) return ::Font{};
    uint32_t a = static_cast<uint32_t>(addr);
    ::Font res{};
    res.baseSize = static_cast<int>(vm.memory.read_u32(a, 4));
    res.glyphCount = static_cast<int>(vm.memory.read_u32(a, 8));
    res.glyphPadding = static_cast<int>(vm.memory.read_u32(a, 12));
    res.texture = unpack_texture2d(vm, vm.memory.read_u64(a, 16));
    res.recs = reinterpret_cast<Rectangle *>(vm.memory.read_u64(a, 24));
    res.glyphs = reinterpret_cast<GlyphInfo *>(vm.memory.read_u64(a, 32));
    return res;
}

inline void pack_font(solix::RuntimeContext &vm, uint64_t addr, const ::Font &res) {
    if (addr == 0) return;
    uint32_t a = static_cast<uint32_t>(addr);
    vm.memory.write_u32(a, 4, static_cast<uint32_t>(res.baseSize));
    vm.memory.write_u32(a, 8, static_cast<uint32_t>(res.glyphCount));
    vm.memory.write_u32(a, 12, static_cast<uint32_t>(res.glyphPadding));
    { uint64_t sub_addr = vm.memory.read_u64(a, 16); if (sub_addr != 0) pack_texture2d(vm, sub_addr, res.texture); }
    vm.memory.write_u64(a, 24, reinterpret_cast<uint64_t>(res.recs));
    vm.memory.write_u64(a, 32, reinterpret_cast<uint64_t>(res.glyphs));
}

inline ::Camera3D unpack_camera3d(solix::RuntimeContext &vm, uint64_t addr) {
    if (addr == 0) return ::Camera3D{};
    uint32_t a = static_cast<uint32_t>(addr);
    ::Camera3D res{};
    res.position = unpack_vector3(vm, vm.memory.read_u64(a, 8));
    res.target = unpack_vector3(vm, vm.memory.read_u64(a, 16));
    res.up = unpack_vector3(vm, vm.memory.read_u64(a, 24));
    { uint32_t raw = vm.memory.read_u32(a, 32); std::memcpy(&res.fovy, &raw, 4); }
    res.projection = static_cast<int>(vm.memory.read_u32(a, 36));
    return res;
}

inline void pack_camera3d(solix::RuntimeContext &vm, uint64_t addr, const ::Camera3D &res) {
    if (addr == 0) return;
    uint32_t a = static_cast<uint32_t>(addr);
    { uint64_t sub_addr = vm.memory.read_u64(a, 8); if (sub_addr != 0) pack_vector3(vm, sub_addr, res.position); }
    { uint64_t sub_addr = vm.memory.read_u64(a, 16); if (sub_addr != 0) pack_vector3(vm, sub_addr, res.target); }
    { uint64_t sub_addr = vm.memory.read_u64(a, 24); if (sub_addr != 0) pack_vector3(vm, sub_addr, res.up); }
    { uint32_t raw; std::memcpy(&raw, &res.fovy, 4); vm.memory.write_u32(a, 32, raw); }
    vm.memory.write_u32(a, 36, static_cast<uint32_t>(res.projection));
}

inline ::Camera2D unpack_camera2d(solix::RuntimeContext &vm, uint64_t addr) {
    if (addr == 0) return ::Camera2D{};
    uint32_t a = static_cast<uint32_t>(addr);
    ::Camera2D res{};
    res.offset = unpack_vector2(vm, vm.memory.read_u64(a, 8));
    res.target = unpack_vector2(vm, vm.memory.read_u64(a, 16));
    { uint32_t raw = vm.memory.read_u32(a, 24); std::memcpy(&res.rotation, &raw, 4); }
    { uint32_t raw = vm.memory.read_u32(a, 28); std::memcpy(&res.zoom, &raw, 4); }
    return res;
}

inline void pack_camera2d(solix::RuntimeContext &vm, uint64_t addr, const ::Camera2D &res) {
    if (addr == 0) return;
    uint32_t a = static_cast<uint32_t>(addr);
    { uint64_t sub_addr = vm.memory.read_u64(a, 8); if (sub_addr != 0) pack_vector2(vm, sub_addr, res.offset); }
    { uint64_t sub_addr = vm.memory.read_u64(a, 16); if (sub_addr != 0) pack_vector2(vm, sub_addr, res.target); }
    { uint32_t raw; std::memcpy(&raw, &res.rotation, 4); vm.memory.write_u32(a, 24, raw); }
    { uint32_t raw; std::memcpy(&raw, &res.zoom, 4); vm.memory.write_u32(a, 28, raw); }
}

inline ::Mesh unpack_mesh(solix::RuntimeContext &vm, uint64_t addr) {
    if (addr == 0) return ::Mesh{};
    uint32_t a = static_cast<uint32_t>(addr);
    ::Mesh res{};
    res.vertexCount = static_cast<int>(vm.memory.read_u32(a, 4));
    res.triangleCount = static_cast<int>(vm.memory.read_u32(a, 8));
    res.vertices = reinterpret_cast<float *>(vm.memory.read_u64(a, 16));
    res.texcoords = reinterpret_cast<float *>(vm.memory.read_u64(a, 24));
    res.texcoords2 = reinterpret_cast<float *>(vm.memory.read_u64(a, 32));
    res.normals = reinterpret_cast<float *>(vm.memory.read_u64(a, 40));
    res.tangents = reinterpret_cast<float *>(vm.memory.read_u64(a, 48));
    res.colors = reinterpret_cast<unsigned char *>(vm.memory.read_u64(a, 56));
    res.indices = reinterpret_cast<unsigned short *>(vm.memory.read_u64(a, 64));
    res.boneCount = static_cast<int>(vm.memory.read_u32(a, 72));
    res.boneIndices = reinterpret_cast<unsigned char *>(vm.memory.read_u64(a, 80));
    res.boneWeights = reinterpret_cast<float *>(vm.memory.read_u64(a, 88));
    res.animVertices = reinterpret_cast<float *>(vm.memory.read_u64(a, 96));
    res.animNormals = reinterpret_cast<float *>(vm.memory.read_u64(a, 104));
    res.vaoId = vm.memory.read_u32(a, 112);
    res.vboId = reinterpret_cast<unsigned int *>(vm.memory.read_u64(a, 120));
    return res;
}

inline void pack_mesh(solix::RuntimeContext &vm, uint64_t addr, const ::Mesh &res) {
    if (addr == 0) return;
    uint32_t a = static_cast<uint32_t>(addr);
    vm.memory.write_u32(a, 4, static_cast<uint32_t>(res.vertexCount));
    vm.memory.write_u32(a, 8, static_cast<uint32_t>(res.triangleCount));
    vm.memory.write_u64(a, 16, reinterpret_cast<uint64_t>(res.vertices));
    vm.memory.write_u64(a, 24, reinterpret_cast<uint64_t>(res.texcoords));
    vm.memory.write_u64(a, 32, reinterpret_cast<uint64_t>(res.texcoords2));
    vm.memory.write_u64(a, 40, reinterpret_cast<uint64_t>(res.normals));
    vm.memory.write_u64(a, 48, reinterpret_cast<uint64_t>(res.tangents));
    vm.memory.write_u64(a, 56, reinterpret_cast<uint64_t>(res.colors));
    vm.memory.write_u64(a, 64, reinterpret_cast<uint64_t>(res.indices));
    vm.memory.write_u32(a, 72, static_cast<uint32_t>(res.boneCount));
    vm.memory.write_u64(a, 80, reinterpret_cast<uint64_t>(res.boneIndices));
    vm.memory.write_u64(a, 88, reinterpret_cast<uint64_t>(res.boneWeights));
    vm.memory.write_u64(a, 96, reinterpret_cast<uint64_t>(res.animVertices));
    vm.memory.write_u64(a, 104, reinterpret_cast<uint64_t>(res.animNormals));
    vm.memory.write_u32(a, 112, res.vaoId);
    vm.memory.write_u64(a, 120, reinterpret_cast<uint64_t>(res.vboId));
}

inline ::Shader unpack_shader(solix::RuntimeContext &vm, uint64_t addr) {
    if (addr == 0) return ::Shader{};
    uint32_t a = static_cast<uint32_t>(addr);
    ::Shader res{};
    res.id = vm.memory.read_u32(a, 4);
    res.locs = reinterpret_cast<int *>(vm.memory.read_u64(a, 8));
    return res;
}

inline void pack_shader(solix::RuntimeContext &vm, uint64_t addr, const ::Shader &res) {
    if (addr == 0) return;
    uint32_t a = static_cast<uint32_t>(addr);
    vm.memory.write_u32(a, 4, res.id);
    vm.memory.write_u64(a, 8, reinterpret_cast<uint64_t>(res.locs));
}

inline ::MaterialMap unpack_materialmap(solix::RuntimeContext &vm, uint64_t addr) {
    if (addr == 0) return ::MaterialMap{};
    uint32_t a = static_cast<uint32_t>(addr);
    ::MaterialMap res{};
    res.texture = unpack_texture2d(vm, vm.memory.read_u64(a, 8));
    res.color = unpack_color(vm, vm.memory.read_u64(a, 16));
    { uint32_t raw = vm.memory.read_u32(a, 24); std::memcpy(&res.value, &raw, 4); }
    return res;
}

inline void pack_materialmap(solix::RuntimeContext &vm, uint64_t addr, const ::MaterialMap &res) {
    if (addr == 0) return;
    uint32_t a = static_cast<uint32_t>(addr);
    { uint64_t sub_addr = vm.memory.read_u64(a, 8); if (sub_addr != 0) pack_texture2d(vm, sub_addr, res.texture); }
    { uint64_t sub_addr = vm.memory.read_u64(a, 16); if (sub_addr != 0) pack_color(vm, sub_addr, res.color); }
    { uint32_t raw; std::memcpy(&raw, &res.value, 4); vm.memory.write_u32(a, 24, raw); }
}

inline ::Material unpack_material(solix::RuntimeContext &vm, uint64_t addr) {
    if (addr == 0) return ::Material{};
    uint32_t a = static_cast<uint32_t>(addr);
    ::Material res{};
    res.shader = unpack_shader(vm, vm.memory.read_u64(a, 8));
    res.maps = reinterpret_cast<MaterialMap *>(vm.memory.read_u64(a, 16));
    { uint32_t raw = vm.memory.read_u32(a, 24); std::memcpy(&res.params[0], &raw, 4); }
    { uint32_t raw = vm.memory.read_u32(a, 28); std::memcpy(&res.params[1], &raw, 4); }
    { uint32_t raw = vm.memory.read_u32(a, 32); std::memcpy(&res.params[2], &raw, 4); }
    { uint32_t raw = vm.memory.read_u32(a, 36); std::memcpy(&res.params[3], &raw, 4); }
    return res;
}

inline void pack_material(solix::RuntimeContext &vm, uint64_t addr, const ::Material &res) {
    if (addr == 0) return;
    uint32_t a = static_cast<uint32_t>(addr);
    { uint64_t sub_addr = vm.memory.read_u64(a, 8); if (sub_addr != 0) pack_shader(vm, sub_addr, res.shader); }
    vm.memory.write_u64(a, 16, reinterpret_cast<uint64_t>(res.maps));
    { uint32_t raw; std::memcpy(&raw, &res.params[0], 4); vm.memory.write_u32(a, 24, raw); }
    { uint32_t raw; std::memcpy(&raw, &res.params[1], 4); vm.memory.write_u32(a, 28, raw); }
    { uint32_t raw; std::memcpy(&raw, &res.params[2], 4); vm.memory.write_u32(a, 32, raw); }
    { uint32_t raw; std::memcpy(&raw, &res.params[3], 4); vm.memory.write_u32(a, 36, raw); }
}

inline ::Transform unpack_transform(solix::RuntimeContext &vm, uint64_t addr) {
    if (addr == 0) return ::Transform{};
    uint32_t a = static_cast<uint32_t>(addr);
    ::Transform res{};
    res.translation = unpack_vector3(vm, vm.memory.read_u64(a, 8));
    res.rotation = unpack_vector4(vm, vm.memory.read_u64(a, 16));
    res.scale = unpack_vector3(vm, vm.memory.read_u64(a, 24));
    return res;
}

inline void pack_transform(solix::RuntimeContext &vm, uint64_t addr, const ::Transform &res) {
    if (addr == 0) return;
    uint32_t a = static_cast<uint32_t>(addr);
    { uint64_t sub_addr = vm.memory.read_u64(a, 8); if (sub_addr != 0) pack_vector3(vm, sub_addr, res.translation); }
    { uint64_t sub_addr = vm.memory.read_u64(a, 16); if (sub_addr != 0) pack_vector4(vm, sub_addr, res.rotation); }
    { uint64_t sub_addr = vm.memory.read_u64(a, 24); if (sub_addr != 0) pack_vector3(vm, sub_addr, res.scale); }
}

inline ::BoneInfo unpack_boneinfo(solix::RuntimeContext &vm, uint64_t addr) {
    if (addr == 0) return ::BoneInfo{};
    uint32_t a = static_cast<uint32_t>(addr);
    ::BoneInfo res{};
    { std::string s = read_solix_chars(vm, vm.memory.read_u64(a, 8)); std::strncpy(res.name, s.c_str(), 31); res.name[31] = '\0'; }
    res.parent = static_cast<int>(vm.memory.read_u32(a, 16));
    return res;
}

inline void pack_boneinfo(solix::RuntimeContext &vm, uint64_t addr, const ::BoneInfo &res) {
    if (addr == 0) return;
    uint32_t a = static_cast<uint32_t>(addr);
    vm.memory.write_u32(a, 16, static_cast<uint32_t>(res.parent));
}

inline ::ModelSkeleton unpack_modelskeleton(solix::RuntimeContext &vm, uint64_t addr) {
    if (addr == 0) return ::ModelSkeleton{};
    uint32_t a = static_cast<uint32_t>(addr);
    ::ModelSkeleton res{};
    res.boneCount = vm.memory.read_u32(a, 4);
    res.bones = reinterpret_cast<BoneInfo *>(vm.memory.read_u64(a, 8));
    res.bindPose = reinterpret_cast<ModelAnimPose>(vm.memory.read_u64(a, 16));
    return res;
}

inline void pack_modelskeleton(solix::RuntimeContext &vm, uint64_t addr, const ::ModelSkeleton &res) {
    if (addr == 0) return;
    uint32_t a = static_cast<uint32_t>(addr);
    vm.memory.write_u32(a, 4, res.boneCount);
    vm.memory.write_u64(a, 8, reinterpret_cast<uint64_t>(res.bones));
    vm.memory.write_u64(a, 16, reinterpret_cast<uint64_t>(res.bindPose));
}

inline ::Model unpack_model(solix::RuntimeContext &vm, uint64_t addr) {
    if (addr == 0) return ::Model{};
    uint32_t a = static_cast<uint32_t>(addr);
    ::Model res{};
    res.transform = unpack_matrix(vm, vm.memory.read_u64(a, 8));
    res.meshCount = static_cast<int>(vm.memory.read_u32(a, 16));
    res.materialCount = static_cast<int>(vm.memory.read_u32(a, 20));
    res.meshes = reinterpret_cast<Mesh *>(vm.memory.read_u64(a, 24));
    res.materials = reinterpret_cast<Material *>(vm.memory.read_u64(a, 32));
    res.meshMaterial = reinterpret_cast<int *>(vm.memory.read_u64(a, 40));
    res.skeleton = unpack_modelskeleton(vm, vm.memory.read_u64(a, 48));
    res.currentPose = reinterpret_cast<ModelAnimPose>(vm.memory.read_u64(a, 56));
    res.boneMatrices = reinterpret_cast<Matrix *>(vm.memory.read_u64(a, 64));
    return res;
}

inline void pack_model(solix::RuntimeContext &vm, uint64_t addr, const ::Model &res) {
    if (addr == 0) return;
    uint32_t a = static_cast<uint32_t>(addr);
    { uint64_t sub_addr = vm.memory.read_u64(a, 8); if (sub_addr != 0) pack_matrix(vm, sub_addr, res.transform); }
    vm.memory.write_u32(a, 16, static_cast<uint32_t>(res.meshCount));
    vm.memory.write_u32(a, 20, static_cast<uint32_t>(res.materialCount));
    vm.memory.write_u64(a, 24, reinterpret_cast<uint64_t>(res.meshes));
    vm.memory.write_u64(a, 32, reinterpret_cast<uint64_t>(res.materials));
    vm.memory.write_u64(a, 40, reinterpret_cast<uint64_t>(res.meshMaterial));
    { uint64_t sub_addr = vm.memory.read_u64(a, 48); if (sub_addr != 0) pack_modelskeleton(vm, sub_addr, res.skeleton); }
    vm.memory.write_u64(a, 56, reinterpret_cast<uint64_t>(res.currentPose));
    vm.memory.write_u64(a, 64, reinterpret_cast<uint64_t>(res.boneMatrices));
}

inline ::ModelAnimation unpack_modelanimation(solix::RuntimeContext &vm, uint64_t addr) {
    if (addr == 0) return ::ModelAnimation{};
    uint32_t a = static_cast<uint32_t>(addr);
    ::ModelAnimation res{};
    { std::string s = read_solix_chars(vm, vm.memory.read_u64(a, 8)); std::strncpy(res.name, s.c_str(), 31); res.name[31] = '\0'; }
    res.boneCount = vm.memory.read_u32(a, 16);
    res.keyframeCount = static_cast<int>(vm.memory.read_u32(a, 20));
    res.keyframePoses = reinterpret_cast<ModelAnimPose *>(vm.memory.read_u64(a, 24));
    return res;
}

inline void pack_modelanimation(solix::RuntimeContext &vm, uint64_t addr, const ::ModelAnimation &res) {
    if (addr == 0) return;
    uint32_t a = static_cast<uint32_t>(addr);
    vm.memory.write_u32(a, 16, res.boneCount);
    vm.memory.write_u32(a, 20, static_cast<uint32_t>(res.keyframeCount));
    vm.memory.write_u64(a, 24, reinterpret_cast<uint64_t>(res.keyframePoses));
}

inline ::Ray unpack_ray(solix::RuntimeContext &vm, uint64_t addr) {
    if (addr == 0) return ::Ray{};
    uint32_t a = static_cast<uint32_t>(addr);
    ::Ray res{};
    res.position = unpack_vector3(vm, vm.memory.read_u64(a, 8));
    res.direction = unpack_vector3(vm, vm.memory.read_u64(a, 16));
    return res;
}

inline void pack_ray(solix::RuntimeContext &vm, uint64_t addr, const ::Ray &res) {
    if (addr == 0) return;
    uint32_t a = static_cast<uint32_t>(addr);
    { uint64_t sub_addr = vm.memory.read_u64(a, 8); if (sub_addr != 0) pack_vector3(vm, sub_addr, res.position); }
    { uint64_t sub_addr = vm.memory.read_u64(a, 16); if (sub_addr != 0) pack_vector3(vm, sub_addr, res.direction); }
}

inline ::RayCollision unpack_raycollision(solix::RuntimeContext &vm, uint64_t addr) {
    if (addr == 0) return ::RayCollision{};
    uint32_t a = static_cast<uint32_t>(addr);
    ::RayCollision res{};
    res.hit = (vm.memory.read_u8(a, 4) != 0);
    { uint32_t raw = vm.memory.read_u32(a, 8); std::memcpy(&res.distance, &raw, 4); }
    res.point = unpack_vector3(vm, vm.memory.read_u64(a, 16));
    res.normal = unpack_vector3(vm, vm.memory.read_u64(a, 24));
    return res;
}

inline void pack_raycollision(solix::RuntimeContext &vm, uint64_t addr, const ::RayCollision &res) {
    if (addr == 0) return;
    uint32_t a = static_cast<uint32_t>(addr);
    vm.memory.write_u8(a, 4, res.hit ? 1 : 0);
    { uint32_t raw; std::memcpy(&raw, &res.distance, 4); vm.memory.write_u32(a, 8, raw); }
    { uint64_t sub_addr = vm.memory.read_u64(a, 16); if (sub_addr != 0) pack_vector3(vm, sub_addr, res.point); }
    { uint64_t sub_addr = vm.memory.read_u64(a, 24); if (sub_addr != 0) pack_vector3(vm, sub_addr, res.normal); }
}

inline ::BoundingBox unpack_boundingbox(solix::RuntimeContext &vm, uint64_t addr) {
    if (addr == 0) return ::BoundingBox{};
    uint32_t a = static_cast<uint32_t>(addr);
    ::BoundingBox res{};
    res.min = unpack_vector3(vm, vm.memory.read_u64(a, 8));
    res.max = unpack_vector3(vm, vm.memory.read_u64(a, 16));
    return res;
}

inline void pack_boundingbox(solix::RuntimeContext &vm, uint64_t addr, const ::BoundingBox &res) {
    if (addr == 0) return;
    uint32_t a = static_cast<uint32_t>(addr);
    { uint64_t sub_addr = vm.memory.read_u64(a, 8); if (sub_addr != 0) pack_vector3(vm, sub_addr, res.min); }
    { uint64_t sub_addr = vm.memory.read_u64(a, 16); if (sub_addr != 0) pack_vector3(vm, sub_addr, res.max); }
}

inline ::Wave unpack_wave(solix::RuntimeContext &vm, uint64_t addr) {
    if (addr == 0) return ::Wave{};
    uint32_t a = static_cast<uint32_t>(addr);
    ::Wave res{};
    res.frameCount = vm.memory.read_u32(a, 4);
    res.sampleRate = vm.memory.read_u32(a, 8);
    res.sampleSize = vm.memory.read_u32(a, 12);
    res.channels = vm.memory.read_u32(a, 16);
    res.data = reinterpret_cast<void *>(vm.memory.read_u64(a, 24));
    return res;
}

inline void pack_wave(solix::RuntimeContext &vm, uint64_t addr, const ::Wave &res) {
    if (addr == 0) return;
    uint32_t a = static_cast<uint32_t>(addr);
    vm.memory.write_u32(a, 4, res.frameCount);
    vm.memory.write_u32(a, 8, res.sampleRate);
    vm.memory.write_u32(a, 12, res.sampleSize);
    vm.memory.write_u32(a, 16, res.channels);
    vm.memory.write_u64(a, 24, reinterpret_cast<uint64_t>(res.data));
}

inline ::AudioStream unpack_audiostream(solix::RuntimeContext &vm, uint64_t addr) {
    if (addr == 0) return ::AudioStream{};
    uint32_t a = static_cast<uint32_t>(addr);
    ::AudioStream res{};
    res.buffer = reinterpret_cast<rAudioBuffer *>(vm.memory.read_u64(a, 8));
    res.processor = reinterpret_cast<rAudioProcessor *>(vm.memory.read_u64(a, 16));
    res.sampleRate = vm.memory.read_u32(a, 24);
    res.sampleSize = vm.memory.read_u32(a, 28);
    res.channels = vm.memory.read_u32(a, 32);
    return res;
}

inline void pack_audiostream(solix::RuntimeContext &vm, uint64_t addr, const ::AudioStream &res) {
    if (addr == 0) return;
    uint32_t a = static_cast<uint32_t>(addr);
    vm.memory.write_u64(a, 8, reinterpret_cast<uint64_t>(res.buffer));
    vm.memory.write_u64(a, 16, reinterpret_cast<uint64_t>(res.processor));
    vm.memory.write_u32(a, 24, res.sampleRate);
    vm.memory.write_u32(a, 28, res.sampleSize);
    vm.memory.write_u32(a, 32, res.channels);
}

inline ::Sound unpack_sound(solix::RuntimeContext &vm, uint64_t addr) {
    if (addr == 0) return ::Sound{};
    uint32_t a = static_cast<uint32_t>(addr);
    ::Sound res{};
    res.stream = unpack_audiostream(vm, vm.memory.read_u64(a, 8));
    res.frameCount = vm.memory.read_u32(a, 16);
    return res;
}

inline void pack_sound(solix::RuntimeContext &vm, uint64_t addr, const ::Sound &res) {
    if (addr == 0) return;
    uint32_t a = static_cast<uint32_t>(addr);
    { uint64_t sub_addr = vm.memory.read_u64(a, 8); if (sub_addr != 0) pack_audiostream(vm, sub_addr, res.stream); }
    vm.memory.write_u32(a, 16, res.frameCount);
}

inline ::Music unpack_music(solix::RuntimeContext &vm, uint64_t addr) {
    if (addr == 0) return ::Music{};
    uint32_t a = static_cast<uint32_t>(addr);
    ::Music res{};
    res.stream = unpack_audiostream(vm, vm.memory.read_u64(a, 8));
    res.frameCount = vm.memory.read_u32(a, 16);
    res.looping = (vm.memory.read_u8(a, 20) != 0);
    res.ctxType = static_cast<int>(vm.memory.read_u32(a, 24));
    res.ctxData = reinterpret_cast<void *>(vm.memory.read_u64(a, 32));
    return res;
}

inline void pack_music(solix::RuntimeContext &vm, uint64_t addr, const ::Music &res) {
    if (addr == 0) return;
    uint32_t a = static_cast<uint32_t>(addr);
    { uint64_t sub_addr = vm.memory.read_u64(a, 8); if (sub_addr != 0) pack_audiostream(vm, sub_addr, res.stream); }
    vm.memory.write_u32(a, 16, res.frameCount);
    vm.memory.write_u8(a, 20, res.looping ? 1 : 0);
    vm.memory.write_u32(a, 24, static_cast<uint32_t>(res.ctxType));
    vm.memory.write_u64(a, 32, reinterpret_cast<uint64_t>(res.ctxData));
}

inline ::VrDeviceInfo unpack_vrdeviceinfo(solix::RuntimeContext &vm, uint64_t addr) {
    if (addr == 0) return ::VrDeviceInfo{};
    uint32_t a = static_cast<uint32_t>(addr);
    ::VrDeviceInfo res{};
    res.hResolution = static_cast<int>(vm.memory.read_u32(a, 4));
    res.vResolution = static_cast<int>(vm.memory.read_u32(a, 8));
    { uint32_t raw = vm.memory.read_u32(a, 12); std::memcpy(&res.hScreenSize, &raw, 4); }
    { uint32_t raw = vm.memory.read_u32(a, 16); std::memcpy(&res.vScreenSize, &raw, 4); }
    { uint32_t raw = vm.memory.read_u32(a, 20); std::memcpy(&res.eyeToScreenDistance, &raw, 4); }
    { uint32_t raw = vm.memory.read_u32(a, 24); std::memcpy(&res.lensSeparationDistance, &raw, 4); }
    { uint32_t raw = vm.memory.read_u32(a, 28); std::memcpy(&res.interpupillaryDistance, &raw, 4); }
    { uint32_t raw = vm.memory.read_u32(a, 32); std::memcpy(&res.lensDistortionValues[0], &raw, 4); }
    { uint32_t raw = vm.memory.read_u32(a, 36); std::memcpy(&res.lensDistortionValues[1], &raw, 4); }
    { uint32_t raw = vm.memory.read_u32(a, 40); std::memcpy(&res.lensDistortionValues[2], &raw, 4); }
    { uint32_t raw = vm.memory.read_u32(a, 44); std::memcpy(&res.lensDistortionValues[3], &raw, 4); }
    { uint32_t raw = vm.memory.read_u32(a, 48); std::memcpy(&res.chromaAbCorrection[0], &raw, 4); }
    { uint32_t raw = vm.memory.read_u32(a, 52); std::memcpy(&res.chromaAbCorrection[1], &raw, 4); }
    { uint32_t raw = vm.memory.read_u32(a, 56); std::memcpy(&res.chromaAbCorrection[2], &raw, 4); }
    { uint32_t raw = vm.memory.read_u32(a, 60); std::memcpy(&res.chromaAbCorrection[3], &raw, 4); }
    return res;
}

inline void pack_vrdeviceinfo(solix::RuntimeContext &vm, uint64_t addr, const ::VrDeviceInfo &res) {
    if (addr == 0) return;
    uint32_t a = static_cast<uint32_t>(addr);
    vm.memory.write_u32(a, 4, static_cast<uint32_t>(res.hResolution));
    vm.memory.write_u32(a, 8, static_cast<uint32_t>(res.vResolution));
    { uint32_t raw; std::memcpy(&raw, &res.hScreenSize, 4); vm.memory.write_u32(a, 12, raw); }
    { uint32_t raw; std::memcpy(&raw, &res.vScreenSize, 4); vm.memory.write_u32(a, 16, raw); }
    { uint32_t raw; std::memcpy(&raw, &res.eyeToScreenDistance, 4); vm.memory.write_u32(a, 20, raw); }
    { uint32_t raw; std::memcpy(&raw, &res.lensSeparationDistance, 4); vm.memory.write_u32(a, 24, raw); }
    { uint32_t raw; std::memcpy(&raw, &res.interpupillaryDistance, 4); vm.memory.write_u32(a, 28, raw); }
    { uint32_t raw; std::memcpy(&raw, &res.lensDistortionValues[0], 4); vm.memory.write_u32(a, 32, raw); }
    { uint32_t raw; std::memcpy(&raw, &res.lensDistortionValues[1], 4); vm.memory.write_u32(a, 36, raw); }
    { uint32_t raw; std::memcpy(&raw, &res.lensDistortionValues[2], 4); vm.memory.write_u32(a, 40, raw); }
    { uint32_t raw; std::memcpy(&raw, &res.lensDistortionValues[3], 4); vm.memory.write_u32(a, 44, raw); }
    { uint32_t raw; std::memcpy(&raw, &res.chromaAbCorrection[0], 4); vm.memory.write_u32(a, 48, raw); }
    { uint32_t raw; std::memcpy(&raw, &res.chromaAbCorrection[1], 4); vm.memory.write_u32(a, 52, raw); }
    { uint32_t raw; std::memcpy(&raw, &res.chromaAbCorrection[2], 4); vm.memory.write_u32(a, 56, raw); }
    { uint32_t raw; std::memcpy(&raw, &res.chromaAbCorrection[3], 4); vm.memory.write_u32(a, 60, raw); }
}

inline ::VrStereoConfig unpack_vrstereoconfig(solix::RuntimeContext &vm, uint64_t addr) {
    if (addr == 0) return ::VrStereoConfig{};
    uint32_t a = static_cast<uint32_t>(addr);
    ::VrStereoConfig res{};
    res.projection[0] = unpack_matrix(vm, vm.memory.read_u64(a, 8));
    res.projection[1] = unpack_matrix(vm, vm.memory.read_u64(a, 16));
    res.viewOffset[0] = unpack_matrix(vm, vm.memory.read_u64(a, 24));
    res.viewOffset[1] = unpack_matrix(vm, vm.memory.read_u64(a, 32));
    { ::Vector2 tmp = unpack_vector2(vm, vm.memory.read_u64(a, 40)); res.leftLensCenter[0] = tmp.x; res.leftLensCenter[1] = tmp.y; }
    { ::Vector2 tmp = unpack_vector2(vm, vm.memory.read_u64(a, 48)); res.rightLensCenter[0] = tmp.x; res.rightLensCenter[1] = tmp.y; }
    { ::Vector2 tmp = unpack_vector2(vm, vm.memory.read_u64(a, 56)); res.leftScreenCenter[0] = tmp.x; res.leftScreenCenter[1] = tmp.y; }
    { ::Vector2 tmp = unpack_vector2(vm, vm.memory.read_u64(a, 64)); res.rightScreenCenter[0] = tmp.x; res.rightScreenCenter[1] = tmp.y; }
    { ::Vector2 tmp = unpack_vector2(vm, vm.memory.read_u64(a, 72)); res.scale[0] = tmp.x; res.scale[1] = tmp.y; }
    { ::Vector2 tmp = unpack_vector2(vm, vm.memory.read_u64(a, 80)); res.scaleIn[0] = tmp.x; res.scaleIn[1] = tmp.y; }
    return res;
}

inline void pack_vrstereoconfig(solix::RuntimeContext &vm, uint64_t addr, const ::VrStereoConfig &res) {
    if (addr == 0) return;
    uint32_t a = static_cast<uint32_t>(addr);
    { uint64_t sub_addr = vm.memory.read_u64(a, 8); if (sub_addr != 0) pack_matrix(vm, sub_addr, res.projection[0]); }
    { uint64_t sub_addr = vm.memory.read_u64(a, 16); if (sub_addr != 0) pack_matrix(vm, sub_addr, res.projection[1]); }
    { uint64_t sub_addr = vm.memory.read_u64(a, 24); if (sub_addr != 0) pack_matrix(vm, sub_addr, res.viewOffset[0]); }
    { uint64_t sub_addr = vm.memory.read_u64(a, 32); if (sub_addr != 0) pack_matrix(vm, sub_addr, res.viewOffset[1]); }
    { uint64_t sub_addr = vm.memory.read_u64(a, 40); if (sub_addr != 0) pack_vector2(vm, sub_addr, ::Vector2{res.leftLensCenter[0], res.leftLensCenter[1]}); }
    { uint64_t sub_addr = vm.memory.read_u64(a, 48); if (sub_addr != 0) pack_vector2(vm, sub_addr, ::Vector2{res.rightLensCenter[0], res.rightLensCenter[1]}); }
    { uint64_t sub_addr = vm.memory.read_u64(a, 56); if (sub_addr != 0) pack_vector2(vm, sub_addr, ::Vector2{res.leftScreenCenter[0], res.leftScreenCenter[1]}); }
    { uint64_t sub_addr = vm.memory.read_u64(a, 64); if (sub_addr != 0) pack_vector2(vm, sub_addr, ::Vector2{res.rightScreenCenter[0], res.rightScreenCenter[1]}); }
    { uint64_t sub_addr = vm.memory.read_u64(a, 72); if (sub_addr != 0) pack_vector2(vm, sub_addr, ::Vector2{res.scale[0], res.scale[1]}); }
    { uint64_t sub_addr = vm.memory.read_u64(a, 80); if (sub_addr != 0) pack_vector2(vm, sub_addr, ::Vector2{res.scaleIn[0], res.scaleIn[1]}); }
}

inline ::FilePathList unpack_filepathlist(solix::RuntimeContext &vm, uint64_t addr) {
    if (addr == 0) return ::FilePathList{};
    uint32_t a = static_cast<uint32_t>(addr);
    ::FilePathList res{};
    res.count = vm.memory.read_u32(a, 4);
    res.paths = reinterpret_cast<char **>(vm.memory.read_u64(a, 8));
    return res;
}

inline void pack_filepathlist(solix::RuntimeContext &vm, uint64_t addr, const ::FilePathList &res) {
    if (addr == 0) return;
    uint32_t a = static_cast<uint32_t>(addr);
    vm.memory.write_u32(a, 4, res.count);
    vm.memory.write_u64(a, 8, reinterpret_cast<uint64_t>(res.paths));
}

inline ::AutomationEvent unpack_automationevent(solix::RuntimeContext &vm, uint64_t addr) {
    if (addr == 0) return ::AutomationEvent{};
    uint32_t a = static_cast<uint32_t>(addr);
    ::AutomationEvent res{};
    res.frame = vm.memory.read_u32(a, 4);
    res.type = vm.memory.read_u32(a, 8);
    res.params[0] = static_cast<int>(vm.memory.read_u32(a, 12));
    res.params[1] = static_cast<int>(vm.memory.read_u32(a, 16));
    res.params[2] = static_cast<int>(vm.memory.read_u32(a, 20));
    res.params[3] = static_cast<int>(vm.memory.read_u32(a, 24));
    return res;
}

inline void pack_automationevent(solix::RuntimeContext &vm, uint64_t addr, const ::AutomationEvent &res) {
    if (addr == 0) return;
    uint32_t a = static_cast<uint32_t>(addr);
    vm.memory.write_u32(a, 4, res.frame);
    vm.memory.write_u32(a, 8, res.type);
    vm.memory.write_u32(a, 12, static_cast<uint32_t>(res.params[0]));
    vm.memory.write_u32(a, 16, static_cast<uint32_t>(res.params[1]));
    vm.memory.write_u32(a, 20, static_cast<uint32_t>(res.params[2]));
    vm.memory.write_u32(a, 24, static_cast<uint32_t>(res.params[3]));
}

inline ::AutomationEventList unpack_automationeventlist(solix::RuntimeContext &vm, uint64_t addr) {
    if (addr == 0) return ::AutomationEventList{};
    uint32_t a = static_cast<uint32_t>(addr);
    ::AutomationEventList res{};
    res.capacity = vm.memory.read_u32(a, 4);
    res.count = vm.memory.read_u32(a, 8);
    res.events = reinterpret_cast<AutomationEvent *>(vm.memory.read_u64(a, 16));
    return res;
}

inline void pack_automationeventlist(solix::RuntimeContext &vm, uint64_t addr, const ::AutomationEventList &res) {
    if (addr == 0) return;
    uint32_t a = static_cast<uint32_t>(addr);
    vm.memory.write_u32(a, 4, res.capacity);
    vm.memory.write_u32(a, 8, res.count);
    vm.memory.write_u64(a, 16, reinterpret_cast<uint64_t>(res.events));
}

inline ::Texture2D unpack_texture2d(solix::RuntimeContext &vm, uint64_t addr) { return unpack_texture(vm, addr); }
inline void pack_texture2d(solix::RuntimeContext &vm, uint64_t addr, const ::Texture2D &res) { pack_texture(vm, addr, res); }
inline ::RenderTexture2D unpack_rendertexture2d(solix::RuntimeContext &vm, uint64_t addr) { return unpack_rendertexture(vm, addr); }
inline void pack_rendertexture2d(solix::RuntimeContext &vm, uint64_t addr, const ::RenderTexture2D &res) { pack_rendertexture(vm, addr, res); }
inline ::Camera3D unpack_camera(solix::RuntimeContext &vm, uint64_t addr) { return unpack_camera3d(vm, addr); }
inline void pack_camera(solix::RuntimeContext &vm, uint64_t addr, const ::Camera3D &res) { pack_camera3d(vm, addr, res); }
inline ::Vector4 unpack_quaternion(solix::RuntimeContext &vm, uint64_t addr) { return unpack_vector4(vm, addr); }
inline void pack_quaternion(solix::RuntimeContext &vm, uint64_t addr, const ::Vector4 &res) { pack_vector4(vm, addr, res); }


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

static uint64_t wrap_init_window(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    int a1 = static_cast<int>(args[1]);
    std::string s_a2 = read_solix_chars(vm, args[2]);
    const char *a2 = s_a2.c_str();
    ::InitWindow(a0, a1, a2);
    return 0;
}

static uint64_t wrap_close_window(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::CloseWindow();
    return 0;
}

static uint64_t wrap_window_should_close(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    bool res = ::WindowShouldClose();
    return res ? 1 : 0;
}

static uint64_t wrap_is_window_ready(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    bool res = ::IsWindowReady();
    return res ? 1 : 0;
}

static uint64_t wrap_is_window_fullscreen(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    bool res = ::IsWindowFullscreen();
    return res ? 1 : 0;
}

static uint64_t wrap_is_window_hidden(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    bool res = ::IsWindowHidden();
    return res ? 1 : 0;
}

static uint64_t wrap_is_window_minimized(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    bool res = ::IsWindowMinimized();
    return res ? 1 : 0;
}

static uint64_t wrap_is_window_maximized(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    bool res = ::IsWindowMaximized();
    return res ? 1 : 0;
}

static uint64_t wrap_is_window_focused(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    bool res = ::IsWindowFocused();
    return res ? 1 : 0;
}

static uint64_t wrap_is_window_resized(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    bool res = ::IsWindowResized();
    return res ? 1 : 0;
}

static uint64_t wrap_is_window_state(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    unsigned int a0 = static_cast<unsigned int>(args[0]);
    bool res = ::IsWindowState(a0);
    return res ? 1 : 0;
}

static uint64_t wrap_set_window_state(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    unsigned int a0 = static_cast<unsigned int>(args[0]);
    ::SetWindowState(a0);
    return 0;
}

static uint64_t wrap_clear_window_state(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    unsigned int a0 = static_cast<unsigned int>(args[0]);
    ::ClearWindowState(a0);
    return 0;
}

static uint64_t wrap_toggle_fullscreen(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::ToggleFullscreen();
    return 0;
}

static uint64_t wrap_toggle_borderless_windowed(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::ToggleBorderlessWindowed();
    return 0;
}

static uint64_t wrap_maximize_window(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::MaximizeWindow();
    return 0;
}

static uint64_t wrap_minimize_window(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::MinimizeWindow();
    return 0;
}

static uint64_t wrap_restore_window(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::RestoreWindow();
    return 0;
}

static uint64_t wrap_set_window_icon(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    ::SetWindowIcon(a0);
    return 0;
}

static uint64_t wrap_set_window_icons(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    int a1 = static_cast<int>(args[1]);
    ::SetWindowIcons(&a0, a1);
    pack_image(vm, args[0], a0);
    return 0;
}

static uint64_t wrap_set_window_title(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    ::SetWindowTitle(a0);
    return 0;
}

static uint64_t wrap_set_window_position(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    int a1 = static_cast<int>(args[1]);
    ::SetWindowPosition(a0, a1);
    return 0;
}

static uint64_t wrap_set_window_monitor(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    ::SetWindowMonitor(a0);
    return 0;
}

static uint64_t wrap_set_window_min_size(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    int a1 = static_cast<int>(args[1]);
    ::SetWindowMinSize(a0, a1);
    return 0;
}

static uint64_t wrap_set_window_max_size(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    int a1 = static_cast<int>(args[1]);
    ::SetWindowMaxSize(a0, a1);
    return 0;
}

static uint64_t wrap_set_window_size(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    int a1 = static_cast<int>(args[1]);
    ::SetWindowSize(a0, a1);
    return 0;
}

static uint64_t wrap_set_window_opacity(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    float a0 = unpack_f32_arg(args[0]);
    ::SetWindowOpacity(a0);
    return 0;
}

static uint64_t wrap_set_window_focused(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::SetWindowFocused();
    return 0;
}

static uint64_t wrap_get_window_handle(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    auto res = ::GetWindowHandle();
    return reinterpret_cast<uint64_t>(res);
}

static uint64_t wrap_get_screen_width(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    auto res = ::GetScreenWidth();
    return static_cast<uint64_t>(res);
}

static uint64_t wrap_get_screen_height(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    auto res = ::GetScreenHeight();
    return static_cast<uint64_t>(res);
}

static uint64_t wrap_get_render_width(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    auto res = ::GetRenderWidth();
    return static_cast<uint64_t>(res);
}

static uint64_t wrap_get_render_height(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    auto res = ::GetRenderHeight();
    return static_cast<uint64_t>(res);
}

static uint64_t wrap_get_monitor_count(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    auto res = ::GetMonitorCount();
    return static_cast<uint64_t>(res);
}

static uint64_t wrap_get_current_monitor(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    auto res = ::GetCurrentMonitor();
    return static_cast<uint64_t>(res);
}

static uint64_t wrap_get_monitor_position(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    auto res = ::GetMonitorPosition(a0);
    pack_vector2(vm, args[1], res);
    return 0;
}

static uint64_t wrap_get_monitor_width(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    auto res = ::GetMonitorWidth(a0);
    return static_cast<uint64_t>(res);
}

static uint64_t wrap_get_monitor_height(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    auto res = ::GetMonitorHeight(a0);
    return static_cast<uint64_t>(res);
}

static uint64_t wrap_get_monitor_physical_width(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    auto res = ::GetMonitorPhysicalWidth(a0);
    return static_cast<uint64_t>(res);
}

static uint64_t wrap_get_monitor_physical_height(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    auto res = ::GetMonitorPhysicalHeight(a0);
    return static_cast<uint64_t>(res);
}

static uint64_t wrap_get_monitor_refresh_rate(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    auto res = ::GetMonitorRefreshRate(a0);
    return static_cast<uint64_t>(res);
}

static uint64_t wrap_get_window_position(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    auto res = ::GetWindowPosition();
    pack_vector2(vm, args[0], res);
    return 0;
}

static uint64_t wrap_get_window_scale_dpi(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    auto res = ::GetWindowScaleDPI();
    pack_vector2(vm, args[0], res);
    return 0;
}

static uint64_t wrap_get_monitor_name(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    const char *res = ::GetMonitorName(a0);
    return allocate_solix_chars(vm, res);
}

static uint64_t wrap_set_clipboard_text(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    ::SetClipboardText(a0);
    return 0;
}

static uint64_t wrap_get_clipboard_text(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    const char *res = ::GetClipboardText();
    return allocate_solix_chars(vm, res);
}

static uint64_t wrap_get_clipboard_image(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    auto res = ::GetClipboardImage();
    pack_image(vm, args[0], res);
    return 0;
}

static uint64_t wrap_enable_event_waiting(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::EnableEventWaiting();
    return 0;
}

static uint64_t wrap_disable_event_waiting(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::DisableEventWaiting();
    return 0;
}

static uint64_t wrap_show_cursor(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::ShowCursor();
    return 0;
}

static uint64_t wrap_hide_cursor(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::HideCursor();
    return 0;
}

static uint64_t wrap_is_cursor_hidden(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    bool res = ::IsCursorHidden();
    return res ? 1 : 0;
}

static uint64_t wrap_enable_cursor(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::EnableCursor();
    return 0;
}

static uint64_t wrap_disable_cursor(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::DisableCursor();
    return 0;
}

static uint64_t wrap_is_cursor_on_screen(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    bool res = ::IsCursorOnScreen();
    return res ? 1 : 0;
}

static uint64_t wrap_clear_background(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Color a0 = unpack_color(vm, args[0]);
    ::ClearBackground(a0);
    return 0;
}

static uint64_t wrap_begin_drawing(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::BeginDrawing();
    return 0;
}

static uint64_t wrap_end_drawing(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::EndDrawing();
    return 0;
}

static uint64_t wrap_begin_mode2d(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Camera2D a0 = unpack_camera2d(vm, args[0]);
    ::BeginMode2D(a0);
    return 0;
}

static uint64_t wrap_end_mode2d(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::EndMode2D();
    return 0;
}

static uint64_t wrap_begin_mode3d(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Camera3D a0 = unpack_camera3d(vm, args[0]);
    ::BeginMode3D(a0);
    return 0;
}

static uint64_t wrap_end_mode3d(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::EndMode3D();
    return 0;
}

static uint64_t wrap_begin_texture_mode(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::RenderTexture2D a0 = unpack_rendertexture2d(vm, args[0]);
    ::BeginTextureMode(a0);
    return 0;
}

static uint64_t wrap_end_texture_mode(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::EndTextureMode();
    return 0;
}

static uint64_t wrap_begin_shader_mode(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Shader a0 = unpack_shader(vm, args[0]);
    ::BeginShaderMode(a0);
    return 0;
}

static uint64_t wrap_end_shader_mode(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::EndShaderMode();
    return 0;
}

static uint64_t wrap_begin_blend_mode(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    ::BeginBlendMode(a0);
    return 0;
}

static uint64_t wrap_end_blend_mode(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::EndBlendMode();
    return 0;
}

static uint64_t wrap_begin_scissor_mode(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    int a1 = static_cast<int>(args[1]);
    int a2 = static_cast<int>(args[2]);
    int a3 = static_cast<int>(args[3]);
    ::BeginScissorMode(a0, a1, a2, a3);
    return 0;
}

static uint64_t wrap_end_scissor_mode(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::EndScissorMode();
    return 0;
}

static uint64_t wrap_begin_vr_stereo_mode(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::VrStereoConfig a0 = unpack_vrstereoconfig(vm, args[0]);
    ::BeginVrStereoMode(a0);
    return 0;
}

static uint64_t wrap_end_vr_stereo_mode(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::EndVrStereoMode();
    return 0;
}

static uint64_t wrap_load_vr_stereo_config(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::VrDeviceInfo a0 = unpack_vrdeviceinfo(vm, args[0]);
    auto res = ::LoadVrStereoConfig(a0);
    pack_vrstereoconfig(vm, args[1], res);
    return 0;
}

static uint64_t wrap_unload_vr_stereo_config(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::VrStereoConfig a0 = unpack_vrstereoconfig(vm, args[0]);
    ::UnloadVrStereoConfig(a0);
    return 0;
}

static uint64_t wrap_load_shader(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    std::string s_a1 = read_solix_chars(vm, args[1]);
    const char *a1 = s_a1.c_str();
    auto res = ::LoadShader(a0, a1);
    pack_shader(vm, args[2], res);
    return 0;
}

static uint64_t wrap_load_shader_from_memory(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    std::string s_a1 = read_solix_chars(vm, args[1]);
    const char *a1 = s_a1.c_str();
    auto res = ::LoadShaderFromMemory(a0, a1);
    pack_shader(vm, args[2], res);
    return 0;
}

static uint64_t wrap_is_shader_valid(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Shader a0 = unpack_shader(vm, args[0]);
    bool res = ::IsShaderValid(a0);
    return res ? 1 : 0;
}

static uint64_t wrap_get_shader_location(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Shader a0 = unpack_shader(vm, args[0]);
    std::string s_a1 = read_solix_chars(vm, args[1]);
    const char *a1 = s_a1.c_str();
    auto res = ::GetShaderLocation(a0, a1);
    return static_cast<uint64_t>(res);
}

static uint64_t wrap_get_shader_location_attrib(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Shader a0 = unpack_shader(vm, args[0]);
    std::string s_a1 = read_solix_chars(vm, args[1]);
    const char *a1 = s_a1.c_str();
    auto res = ::GetShaderLocationAttrib(a0, a1);
    return static_cast<uint64_t>(res);
}

static uint64_t wrap_set_shader_value(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Shader a0 = unpack_shader(vm, args[0]);
    int a1 = static_cast<int>(args[1]);
    void *a2 = reinterpret_cast<void *>(args[2]);
    int a3 = static_cast<int>(args[3]);
    ::SetShaderValue(a0, a1, a2, a3);
    return 0;
}

static uint64_t wrap_set_shader_value_v(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Shader a0 = unpack_shader(vm, args[0]);
    int a1 = static_cast<int>(args[1]);
    void *a2 = reinterpret_cast<void *>(args[2]);
    int a3 = static_cast<int>(args[3]);
    int a4 = static_cast<int>(args[4]);
    ::SetShaderValueV(a0, a1, a2, a3, a4);
    return 0;
}

static uint64_t wrap_set_shader_value_matrix(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Shader a0 = unpack_shader(vm, args[0]);
    int a1 = static_cast<int>(args[1]);
    ::Matrix a2 = unpack_matrix(vm, args[2]);
    ::SetShaderValueMatrix(a0, a1, a2);
    return 0;
}

static uint64_t wrap_set_shader_value_texture(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Shader a0 = unpack_shader(vm, args[0]);
    int a1 = static_cast<int>(args[1]);
    ::Texture2D a2 = unpack_texture2d(vm, args[2]);
    ::SetShaderValueTexture(a0, a1, a2);
    return 0;
}

static uint64_t wrap_unload_shader(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Shader a0 = unpack_shader(vm, args[0]);
    ::UnloadShader(a0);
    return 0;
}

static uint64_t wrap_get_screen_to_world_ray(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector2 a0 = unpack_vector2(vm, args[0]);
    ::Camera a1 = unpack_camera(vm, args[1]);
    auto res = ::GetScreenToWorldRay(a0, a1);
    pack_ray(vm, args[2], res);
    return 0;
}

static uint64_t wrap_get_screen_to_world_ray_ex(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector2 a0 = unpack_vector2(vm, args[0]);
    ::Camera a1 = unpack_camera(vm, args[1]);
    int a2 = static_cast<int>(args[2]);
    int a3 = static_cast<int>(args[3]);
    auto res = ::GetScreenToWorldRayEx(a0, a1, a2, a3);
    pack_ray(vm, args[4], res);
    return 0;
}

static uint64_t wrap_get_world_to_screen(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector3 a0 = unpack_vector3(vm, args[0]);
    ::Camera a1 = unpack_camera(vm, args[1]);
    auto res = ::GetWorldToScreen(a0, a1);
    pack_vector2(vm, args[2], res);
    return 0;
}

static uint64_t wrap_get_world_to_screen_ex(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector3 a0 = unpack_vector3(vm, args[0]);
    ::Camera a1 = unpack_camera(vm, args[1]);
    int a2 = static_cast<int>(args[2]);
    int a3 = static_cast<int>(args[3]);
    auto res = ::GetWorldToScreenEx(a0, a1, a2, a3);
    pack_vector2(vm, args[4], res);
    return 0;
}

static uint64_t wrap_get_world_to_screen2d(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector2 a0 = unpack_vector2(vm, args[0]);
    ::Camera2D a1 = unpack_camera2d(vm, args[1]);
    auto res = ::GetWorldToScreen2D(a0, a1);
    pack_vector2(vm, args[2], res);
    return 0;
}

static uint64_t wrap_get_screen_to_world2d(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector2 a0 = unpack_vector2(vm, args[0]);
    ::Camera2D a1 = unpack_camera2d(vm, args[1]);
    auto res = ::GetScreenToWorld2D(a0, a1);
    pack_vector2(vm, args[2], res);
    return 0;
}

static uint64_t wrap_get_camera_matrix(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Camera a0 = unpack_camera(vm, args[0]);
    auto res = ::GetCameraMatrix(a0);
    pack_matrix(vm, args[1], res);
    return 0;
}

static uint64_t wrap_get_camera_matrix2d(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Camera2D a0 = unpack_camera2d(vm, args[0]);
    auto res = ::GetCameraMatrix2D(a0);
    pack_matrix(vm, args[1], res);
    return 0;
}

static uint64_t wrap_set_target_fps(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    ::SetTargetFPS(a0);
    return 0;
}

static uint64_t wrap_get_frame_time(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    float res = ::GetFrameTime();
    return pack_f32_ret(res);
}

static uint64_t wrap_get_time(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    double res = ::GetTime();
    return bit_cast_to_u64<double>(res);
}

static uint64_t wrap_get_fps(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    auto res = ::GetFPS();
    return static_cast<uint64_t>(res);
}

static uint64_t wrap_swap_screen_buffer(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::SwapScreenBuffer();
    return 0;
}

static uint64_t wrap_poll_input_events(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::PollInputEvents();
    return 0;
}

static uint64_t wrap_wait_time(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    double a0 = bit_cast_from_u64<double>(args[0]);
    ::WaitTime(a0);
    return 0;
}

static uint64_t wrap_set_random_seed(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    unsigned int a0 = static_cast<unsigned int>(args[0]);
    ::SetRandomSeed(a0);
    return 0;
}

static uint64_t wrap_get_random_value(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    int a1 = static_cast<int>(args[1]);
    auto res = ::GetRandomValue(a0, a1);
    return static_cast<uint64_t>(res);
}

static uint64_t wrap_load_random_sequence(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    unsigned int a0 = static_cast<unsigned int>(args[0]);
    int a1 = static_cast<int>(args[1]);
    int a2 = static_cast<int>(args[2]);
    auto res = ::LoadRandomSequence(a0, a1, a2);
    return reinterpret_cast<uint64_t>(res);
}

static uint64_t wrap_unload_random_sequence(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    auto v_a0 = unpack_int32_array(vm, args[0], 0);
    ::UnloadRandomSequence(v_a0.data());
    return 0;
}

static uint64_t wrap_take_screenshot(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    ::TakeScreenshot(a0);
    return 0;
}

static uint64_t wrap_set_config_flags(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    unsigned int a0 = static_cast<unsigned int>(args[0]);
    ::SetConfigFlags(a0);
    return 0;
}

static uint64_t wrap_open_url(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    ::OpenURL(a0);
    return 0;
}

static uint64_t wrap_set_trace_log_level(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    ::SetTraceLogLevel(a0);
    return 0;
}

static uint64_t wrap_mem_alloc(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    unsigned int a0 = static_cast<unsigned int>(args[0]);
    auto res = ::MemAlloc(a0);
    return reinterpret_cast<uint64_t>(res);
}

static uint64_t wrap_mem_realloc(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    void *a0 = reinterpret_cast<void *>(args[0]);
    unsigned int a1 = static_cast<unsigned int>(args[1]);
    auto res = ::MemRealloc(a0, a1);
    return reinterpret_cast<uint64_t>(res);
}

static uint64_t wrap_mem_free(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    void *a0 = reinterpret_cast<void *>(args[0]);
    ::MemFree(a0);
    return 0;
}

static uint64_t wrap_load_file_data(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    auto v_a1 = unpack_int32_array(vm, args[1], 0);
    auto res = ::LoadFileData(a0, v_a1.data());
    return reinterpret_cast<uint64_t>(res);
}

static uint64_t wrap_unload_file_data(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    auto v_a0 = unpack_uint8_array(vm, args[0], 0);
    ::UnloadFileData(v_a0.data());
    return 0;
}

static uint64_t wrap_save_file_data(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    void *a1 = reinterpret_cast<void *>(args[1]);
    int a2 = static_cast<int>(args[2]);
    bool res = ::SaveFileData(a0, a1, a2);
    return res ? 1 : 0;
}

static uint64_t wrap_export_data_as_code(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    auto v_a0 = unpack_uint8_array(vm, args[0], 0);
    int a1 = static_cast<int>(args[1]);
    std::string s_a2 = read_solix_chars(vm, args[2]);
    const char *a2 = s_a2.c_str();
    bool res = ::ExportDataAsCode(v_a0.data(), a1, a2);
    return res ? 1 : 0;
}

static uint64_t wrap_load_file_text(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    const char *res = ::LoadFileText(a0);
    return allocate_solix_chars(vm, res);
}

static uint64_t wrap_unload_file_text(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    char *a0 = const_cast<char *>(s_a0.c_str());
    ::UnloadFileText(a0);
    return 0;
}

static uint64_t wrap_save_file_text(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    std::string s_a1 = read_solix_chars(vm, args[1]);
    const char *a1 = s_a1.c_str();
    bool res = ::SaveFileText(a0, a1);
    return res ? 1 : 0;
}

static uint64_t wrap_file_rename(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    std::string s_a1 = read_solix_chars(vm, args[1]);
    const char *a1 = s_a1.c_str();
    auto res = ::FileRename(a0, a1);
    return static_cast<uint64_t>(res);
}

static uint64_t wrap_file_remove(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    auto res = ::FileRemove(a0);
    return static_cast<uint64_t>(res);
}

static uint64_t wrap_file_copy(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    std::string s_a1 = read_solix_chars(vm, args[1]);
    const char *a1 = s_a1.c_str();
    auto res = ::FileCopy(a0, a1);
    return static_cast<uint64_t>(res);
}

static uint64_t wrap_file_move(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    std::string s_a1 = read_solix_chars(vm, args[1]);
    const char *a1 = s_a1.c_str();
    auto res = ::FileMove(a0, a1);
    return static_cast<uint64_t>(res);
}

static uint64_t wrap_file_text_replace(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    std::string s_a1 = read_solix_chars(vm, args[1]);
    const char *a1 = s_a1.c_str();
    std::string s_a2 = read_solix_chars(vm, args[2]);
    const char *a2 = s_a2.c_str();
    auto res = ::FileTextReplace(a0, a1, a2);
    return static_cast<uint64_t>(res);
}

static uint64_t wrap_file_text_find_index(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    std::string s_a1 = read_solix_chars(vm, args[1]);
    const char *a1 = s_a1.c_str();
    auto res = ::FileTextFindIndex(a0, a1);
    return static_cast<uint64_t>(res);
}

static uint64_t wrap_file_exists(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    bool res = ::FileExists(a0);
    return res ? 1 : 0;
}

static uint64_t wrap_directory_exists(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    bool res = ::DirectoryExists(a0);
    return res ? 1 : 0;
}

static uint64_t wrap_is_file_extension(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    std::string s_a1 = read_solix_chars(vm, args[1]);
    const char *a1 = s_a1.c_str();
    bool res = ::IsFileExtension(a0, a1);
    return res ? 1 : 0;
}

static uint64_t wrap_is_file_hidden(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    bool res = ::IsFileHidden(a0);
    return res ? 1 : 0;
}

static uint64_t wrap_get_file_length(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    auto res = ::GetFileLength(a0);
    return static_cast<uint64_t>(res);
}

static uint64_t wrap_get_file_mod_time(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    auto res = ::GetFileModTime(a0);
    return static_cast<uint64_t>(res);
}

static uint64_t wrap_get_file_extension(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    const char *res = ::GetFileExtension(a0);
    return allocate_solix_chars(vm, res);
}

static uint64_t wrap_get_file_name(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    const char *res = ::GetFileName(a0);
    return allocate_solix_chars(vm, res);
}

static uint64_t wrap_get_file_name_without_ext(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    const char *res = ::GetFileNameWithoutExt(a0);
    return allocate_solix_chars(vm, res);
}

static uint64_t wrap_get_directory_path(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    const char *res = ::GetDirectoryPath(a0);
    return allocate_solix_chars(vm, res);
}

static uint64_t wrap_get_prev_directory_path(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    const char *res = ::GetPrevDirectoryPath(a0);
    return allocate_solix_chars(vm, res);
}

static uint64_t wrap_get_working_directory(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    const char *res = ::GetWorkingDirectory();
    return allocate_solix_chars(vm, res);
}

static uint64_t wrap_get_application_directory(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    const char *res = ::GetApplicationDirectory();
    return allocate_solix_chars(vm, res);
}

static uint64_t wrap_make_directory(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    auto res = ::MakeDirectory(a0);
    return static_cast<uint64_t>(res);
}

static uint64_t wrap_change_directory(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    auto res = ::ChangeDirectory(a0);
    return static_cast<uint64_t>(res);
}

static uint64_t wrap_is_path_file(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    bool res = ::IsPathFile(a0);
    return res ? 1 : 0;
}

static uint64_t wrap_is_path_directory(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    bool res = ::IsPathDirectory(a0);
    return res ? 1 : 0;
}

static uint64_t wrap_is_path_absolute(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    bool res = ::IsPathAbsolute(a0);
    return res ? 1 : 0;
}

static uint64_t wrap_is_file_name_valid(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    bool res = ::IsFileNameValid(a0);
    return res ? 1 : 0;
}

static uint64_t wrap_load_directory_files(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    auto res = ::LoadDirectoryFiles(a0);
    pack_filepathlist(vm, args[1], res);
    return 0;
}

static uint64_t wrap_load_directory_files_ex(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    std::string s_a1 = read_solix_chars(vm, args[1]);
    const char *a1 = s_a1.c_str();
    bool a2 = (args[2] != 0);
    auto res = ::LoadDirectoryFilesEx(a0, a1, a2);
    pack_filepathlist(vm, args[3], res);
    return 0;
}

static uint64_t wrap_unload_directory_files(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::FilePathList a0 = unpack_filepathlist(vm, args[0]);
    ::UnloadDirectoryFiles(a0);
    return 0;
}

static uint64_t wrap_is_file_dropped(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    bool res = ::IsFileDropped();
    return res ? 1 : 0;
}

static uint64_t wrap_load_dropped_files(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    auto res = ::LoadDroppedFiles();
    pack_filepathlist(vm, args[0], res);
    return 0;
}

static uint64_t wrap_unload_dropped_files(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::FilePathList a0 = unpack_filepathlist(vm, args[0]);
    ::UnloadDroppedFiles(a0);
    return 0;
}

static uint64_t wrap_get_directory_file_count(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    auto res = ::GetDirectoryFileCount(a0);
    return static_cast<uint64_t>(res);
}

static uint64_t wrap_get_directory_file_count_ex(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    std::string s_a1 = read_solix_chars(vm, args[1]);
    const char *a1 = s_a1.c_str();
    bool a2 = (args[2] != 0);
    auto res = ::GetDirectoryFileCountEx(a0, a1, a2);
    return static_cast<uint64_t>(res);
}

static uint64_t wrap_compress_data(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    auto v_a0 = unpack_uint8_array(vm, args[0], 0);
    int a1 = static_cast<int>(args[1]);
    auto v_a2 = unpack_int32_array(vm, args[2], 0);
    auto res = ::CompressData(v_a0.data(), a1, v_a2.data());
    return reinterpret_cast<uint64_t>(res);
}

static uint64_t wrap_decompress_data(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    auto v_a0 = unpack_uint8_array(vm, args[0], 0);
    int a1 = static_cast<int>(args[1]);
    auto v_a2 = unpack_int32_array(vm, args[2], 0);
    auto res = ::DecompressData(v_a0.data(), a1, v_a2.data());
    return reinterpret_cast<uint64_t>(res);
}

static uint64_t wrap_encode_data_base64(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    auto v_a0 = unpack_uint8_array(vm, args[0], 0);
    int a1 = static_cast<int>(args[1]);
    auto v_a2 = unpack_int32_array(vm, args[2], 0);
    const char *res = ::EncodeDataBase64(v_a0.data(), a1, v_a2.data());
    return allocate_solix_chars(vm, res);
}

static uint64_t wrap_decode_data_base64(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    auto v_a1 = unpack_int32_array(vm, args[1], 0);
    auto res = ::DecodeDataBase64(a0, v_a1.data());
    return reinterpret_cast<uint64_t>(res);
}

static uint64_t wrap_compute_crc32(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    auto v_a0 = unpack_uint8_array(vm, args[0], 0);
    int a1 = static_cast<int>(args[1]);
    auto res = ::ComputeCRC32(v_a0.data(), a1);
    return static_cast<uint64_t>(res);
}

static uint64_t wrap_compute_md5(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    auto v_a0 = unpack_uint8_array(vm, args[0], 0);
    int a1 = static_cast<int>(args[1]);
    auto res = ::ComputeMD5(v_a0.data(), a1);
    return reinterpret_cast<uint64_t>(res);
}

static uint64_t wrap_compute_sha1(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    auto v_a0 = unpack_uint8_array(vm, args[0], 0);
    int a1 = static_cast<int>(args[1]);
    auto res = ::ComputeSHA1(v_a0.data(), a1);
    return reinterpret_cast<uint64_t>(res);
}

static uint64_t wrap_compute_sha256(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    auto v_a0 = unpack_uint8_array(vm, args[0], 0);
    int a1 = static_cast<int>(args[1]);
    auto res = ::ComputeSHA256(v_a0.data(), a1);
    return reinterpret_cast<uint64_t>(res);
}

static uint64_t wrap_load_automation_event_list(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    auto res = ::LoadAutomationEventList(a0);
    pack_automationeventlist(vm, args[1], res);
    return 0;
}

static uint64_t wrap_unload_automation_event_list(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::AutomationEventList a0 = unpack_automationeventlist(vm, args[0]);
    ::UnloadAutomationEventList(a0);
    return 0;
}

static uint64_t wrap_export_automation_event_list(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::AutomationEventList a0 = unpack_automationeventlist(vm, args[0]);
    std::string s_a1 = read_solix_chars(vm, args[1]);
    const char *a1 = s_a1.c_str();
    bool res = ::ExportAutomationEventList(a0, a1);
    return res ? 1 : 0;
}

static uint64_t wrap_set_automation_event_list(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::AutomationEventList a0 = unpack_automationeventlist(vm, args[0]);
    ::SetAutomationEventList(&a0);
    pack_automationeventlist(vm, args[0], a0);
    return 0;
}

static uint64_t wrap_set_automation_event_base_frame(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    ::SetAutomationEventBaseFrame(a0);
    return 0;
}

static uint64_t wrap_start_automation_event_recording(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::StartAutomationEventRecording();
    return 0;
}

static uint64_t wrap_stop_automation_event_recording(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::StopAutomationEventRecording();
    return 0;
}

static uint64_t wrap_play_automation_event(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::AutomationEvent a0 = unpack_automationevent(vm, args[0]);
    ::PlayAutomationEvent(a0);
    return 0;
}

static uint64_t wrap_is_key_pressed(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    bool res = ::IsKeyPressed(a0);
    return res ? 1 : 0;
}

static uint64_t wrap_is_key_pressed_repeat(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    bool res = ::IsKeyPressedRepeat(a0);
    return res ? 1 : 0;
}

static uint64_t wrap_is_key_down(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    bool res = ::IsKeyDown(a0);
    return res ? 1 : 0;
}

static uint64_t wrap_is_key_released(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    bool res = ::IsKeyReleased(a0);
    return res ? 1 : 0;
}

static uint64_t wrap_is_key_up(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    bool res = ::IsKeyUp(a0);
    return res ? 1 : 0;
}

static uint64_t wrap_get_key_pressed(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    auto res = ::GetKeyPressed();
    return static_cast<uint64_t>(res);
}

static uint64_t wrap_get_char_pressed(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    auto res = ::GetCharPressed();
    return static_cast<uint64_t>(res);
}

static uint64_t wrap_get_key_name(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    const char *res = ::GetKeyName(a0);
    return allocate_solix_chars(vm, res);
}

static uint64_t wrap_set_exit_key(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    ::SetExitKey(a0);
    return 0;
}

static uint64_t wrap_is_gamepad_available(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    bool res = ::IsGamepadAvailable(a0);
    return res ? 1 : 0;
}

static uint64_t wrap_get_gamepad_name(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    const char *res = ::GetGamepadName(a0);
    return allocate_solix_chars(vm, res);
}

static uint64_t wrap_is_gamepad_button_pressed(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    int a1 = static_cast<int>(args[1]);
    bool res = ::IsGamepadButtonPressed(a0, a1);
    return res ? 1 : 0;
}

static uint64_t wrap_is_gamepad_button_down(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    int a1 = static_cast<int>(args[1]);
    bool res = ::IsGamepadButtonDown(a0, a1);
    return res ? 1 : 0;
}

static uint64_t wrap_is_gamepad_button_released(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    int a1 = static_cast<int>(args[1]);
    bool res = ::IsGamepadButtonReleased(a0, a1);
    return res ? 1 : 0;
}

static uint64_t wrap_is_gamepad_button_up(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    int a1 = static_cast<int>(args[1]);
    bool res = ::IsGamepadButtonUp(a0, a1);
    return res ? 1 : 0;
}

static uint64_t wrap_get_gamepad_button_pressed(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    auto res = ::GetGamepadButtonPressed();
    return static_cast<uint64_t>(res);
}

static uint64_t wrap_get_gamepad_axis_count(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    auto res = ::GetGamepadAxisCount(a0);
    return static_cast<uint64_t>(res);
}

static uint64_t wrap_get_gamepad_axis_movement(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    int a1 = static_cast<int>(args[1]);
    float res = ::GetGamepadAxisMovement(a0, a1);
    return pack_f32_ret(res);
}

static uint64_t wrap_set_gamepad_mappings(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    auto res = ::SetGamepadMappings(a0);
    return static_cast<uint64_t>(res);
}

static uint64_t wrap_set_gamepad_vibration(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    float a1 = unpack_f32_arg(args[1]);
    float a2 = unpack_f32_arg(args[2]);
    float a3 = unpack_f32_arg(args[3]);
    ::SetGamepadVibration(a0, a1, a2, a3);
    return 0;
}

static uint64_t wrap_is_mouse_button_pressed(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    bool res = ::IsMouseButtonPressed(a0);
    return res ? 1 : 0;
}

static uint64_t wrap_is_mouse_button_down(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    bool res = ::IsMouseButtonDown(a0);
    return res ? 1 : 0;
}

static uint64_t wrap_is_mouse_button_released(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    bool res = ::IsMouseButtonReleased(a0);
    return res ? 1 : 0;
}

static uint64_t wrap_is_mouse_button_up(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    bool res = ::IsMouseButtonUp(a0);
    return res ? 1 : 0;
}

static uint64_t wrap_get_mouse_x(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    auto res = ::GetMouseX();
    return static_cast<uint64_t>(res);
}

static uint64_t wrap_get_mouse_y(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    auto res = ::GetMouseY();
    return static_cast<uint64_t>(res);
}

static uint64_t wrap_get_mouse_position(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    auto res = ::GetMousePosition();
    pack_vector2(vm, args[0], res);
    return 0;
}

static uint64_t wrap_get_mouse_delta(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    auto res = ::GetMouseDelta();
    pack_vector2(vm, args[0], res);
    return 0;
}

static uint64_t wrap_set_mouse_position(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    int a1 = static_cast<int>(args[1]);
    ::SetMousePosition(a0, a1);
    return 0;
}

static uint64_t wrap_set_mouse_offset(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    int a1 = static_cast<int>(args[1]);
    ::SetMouseOffset(a0, a1);
    return 0;
}

static uint64_t wrap_set_mouse_scale(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    float a0 = unpack_f32_arg(args[0]);
    float a1 = unpack_f32_arg(args[1]);
    ::SetMouseScale(a0, a1);
    return 0;
}

static uint64_t wrap_get_mouse_wheel_move(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    float res = ::GetMouseWheelMove();
    return pack_f32_ret(res);
}

static uint64_t wrap_get_mouse_wheel_move_v(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    auto res = ::GetMouseWheelMoveV();
    pack_vector2(vm, args[0], res);
    return 0;
}

static uint64_t wrap_set_mouse_cursor(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    ::SetMouseCursor(a0);
    return 0;
}

static uint64_t wrap_get_touch_x(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    auto res = ::GetTouchX();
    return static_cast<uint64_t>(res);
}

static uint64_t wrap_get_touch_y(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    auto res = ::GetTouchY();
    return static_cast<uint64_t>(res);
}

static uint64_t wrap_get_touch_position(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    auto res = ::GetTouchPosition(a0);
    pack_vector2(vm, args[1], res);
    return 0;
}

static uint64_t wrap_get_touch_point_id(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    auto res = ::GetTouchPointId(a0);
    return static_cast<uint64_t>(res);
}

static uint64_t wrap_get_touch_point_count(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    auto res = ::GetTouchPointCount();
    return static_cast<uint64_t>(res);
}

static uint64_t wrap_set_gestures_enabled(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    unsigned int a0 = static_cast<unsigned int>(args[0]);
    ::SetGesturesEnabled(a0);
    return 0;
}

static uint64_t wrap_is_gesture_detected(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    unsigned int a0 = static_cast<unsigned int>(args[0]);
    bool res = ::IsGestureDetected(a0);
    return res ? 1 : 0;
}

static uint64_t wrap_get_gesture_detected(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    auto res = ::GetGestureDetected();
    return static_cast<uint64_t>(res);
}

static uint64_t wrap_get_gesture_hold_duration(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    float res = ::GetGestureHoldDuration();
    return pack_f32_ret(res);
}

static uint64_t wrap_get_gesture_drag_vector(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    auto res = ::GetGestureDragVector();
    pack_vector2(vm, args[0], res);
    return 0;
}

static uint64_t wrap_get_gesture_drag_angle(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    float res = ::GetGestureDragAngle();
    return pack_f32_ret(res);
}

static uint64_t wrap_get_gesture_pinch_vector(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    auto res = ::GetGesturePinchVector();
    pack_vector2(vm, args[0], res);
    return 0;
}

static uint64_t wrap_get_gesture_pinch_angle(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    float res = ::GetGesturePinchAngle();
    return pack_f32_ret(res);
}

static uint64_t wrap_update_camera(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Camera a0 = unpack_camera(vm, args[0]);
    int a1 = static_cast<int>(args[1]);
    ::UpdateCamera(&a0, a1);
    pack_camera(vm, args[0], a0);
    return 0;
}

static uint64_t wrap_update_camera_pro(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Camera a0 = unpack_camera(vm, args[0]);
    ::Vector3 a1 = unpack_vector3(vm, args[1]);
    ::Vector3 a2 = unpack_vector3(vm, args[2]);
    float a3 = unpack_f32_arg(args[3]);
    ::UpdateCameraPro(&a0, a1, a2, a3);
    pack_camera(vm, args[0], a0);
    return 0;
}

static uint64_t wrap_set_shapes_texture(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Texture2D a0 = unpack_texture2d(vm, args[0]);
    ::Rectangle a1 = unpack_rectangle(vm, args[1]);
    ::SetShapesTexture(a0, a1);
    return 0;
}

static uint64_t wrap_get_shapes_texture(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    auto res = ::GetShapesTexture();
    pack_texture2d(vm, args[0], res);
    return 0;
}

static uint64_t wrap_get_shapes_texture_rectangle(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    auto res = ::GetShapesTextureRectangle();
    pack_rectangle(vm, args[0], res);
    return 0;
}

static uint64_t wrap_draw_pixel(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    int a1 = static_cast<int>(args[1]);
    ::Color a2 = unpack_color(vm, args[2]);
    ::DrawPixel(a0, a1, a2);
    return 0;
}

static uint64_t wrap_draw_pixel_v(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector2 a0 = unpack_vector2(vm, args[0]);
    ::Color a1 = unpack_color(vm, args[1]);
    ::DrawPixelV(a0, a1);
    return 0;
}

static uint64_t wrap_draw_line(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    int a1 = static_cast<int>(args[1]);
    int a2 = static_cast<int>(args[2]);
    int a3 = static_cast<int>(args[3]);
    ::Color a4 = unpack_color(vm, args[4]);
    ::DrawLine(a0, a1, a2, a3, a4);
    return 0;
}

static uint64_t wrap_draw_line_v(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector2 a0 = unpack_vector2(vm, args[0]);
    ::Vector2 a1 = unpack_vector2(vm, args[1]);
    ::Color a2 = unpack_color(vm, args[2]);
    ::DrawLineV(a0, a1, a2);
    return 0;
}

static uint64_t wrap_draw_line_ex(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector2 a0 = unpack_vector2(vm, args[0]);
    ::Vector2 a1 = unpack_vector2(vm, args[1]);
    float a2 = unpack_f32_arg(args[2]);
    ::Color a3 = unpack_color(vm, args[3]);
    ::DrawLineEx(a0, a1, a2, a3);
    return 0;
}

static uint64_t wrap_draw_line_strip(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    auto v_a0 = unpack_vector2_array(vm, args[0], 0);
    int a1 = static_cast<int>(args[1]);
    ::Color a2 = unpack_color(vm, args[2]);
    ::DrawLineStrip(v_a0.data(), a1, a2);
    return 0;
}

static uint64_t wrap_draw_line_bezier(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector2 a0 = unpack_vector2(vm, args[0]);
    ::Vector2 a1 = unpack_vector2(vm, args[1]);
    float a2 = unpack_f32_arg(args[2]);
    ::Color a3 = unpack_color(vm, args[3]);
    ::DrawLineBezier(a0, a1, a2, a3);
    return 0;
}

static uint64_t wrap_draw_line_dashed(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector2 a0 = unpack_vector2(vm, args[0]);
    ::Vector2 a1 = unpack_vector2(vm, args[1]);
    int a2 = static_cast<int>(args[2]);
    int a3 = static_cast<int>(args[3]);
    ::Color a4 = unpack_color(vm, args[4]);
    ::DrawLineDashed(a0, a1, a2, a3, a4);
    return 0;
}

static uint64_t wrap_draw_triangle(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector2 a0 = unpack_vector2(vm, args[0]);
    ::Vector2 a1 = unpack_vector2(vm, args[1]);
    ::Vector2 a2 = unpack_vector2(vm, args[2]);
    ::Color a3 = unpack_color(vm, args[3]);
    ::DrawTriangle(a0, a1, a2, a3);
    return 0;
}

static uint64_t wrap_draw_triangle_gradient(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector2 a0 = unpack_vector2(vm, args[0]);
    ::Vector2 a1 = unpack_vector2(vm, args[1]);
    ::Vector2 a2 = unpack_vector2(vm, args[2]);
    ::Color a3 = unpack_color(vm, args[3]);
    ::Color a4 = unpack_color(vm, args[4]);
    ::Color a5 = unpack_color(vm, args[5]);
    ::DrawTriangleGradient(a0, a1, a2, a3, a4, a5);
    return 0;
}

static uint64_t wrap_draw_triangle_lines(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector2 a0 = unpack_vector2(vm, args[0]);
    ::Vector2 a1 = unpack_vector2(vm, args[1]);
    ::Vector2 a2 = unpack_vector2(vm, args[2]);
    ::Color a3 = unpack_color(vm, args[3]);
    ::DrawTriangleLines(a0, a1, a2, a3);
    return 0;
}

static uint64_t wrap_draw_triangle_lines_ex(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector2 a0 = unpack_vector2(vm, args[0]);
    ::Vector2 a1 = unpack_vector2(vm, args[1]);
    ::Vector2 a2 = unpack_vector2(vm, args[2]);
    float a3 = unpack_f32_arg(args[3]);
    ::Color a4 = unpack_color(vm, args[4]);
    ::DrawTriangleLinesEx(a0, a1, a2, a3, a4);
    return 0;
}

static uint64_t wrap_draw_triangle_fan(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    auto v_a0 = unpack_vector2_array(vm, args[0], 0);
    int a1 = static_cast<int>(args[1]);
    ::Color a2 = unpack_color(vm, args[2]);
    ::DrawTriangleFan(v_a0.data(), a1, a2);
    return 0;
}

static uint64_t wrap_draw_triangle_strip(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    auto v_a0 = unpack_vector2_array(vm, args[0], 0);
    int a1 = static_cast<int>(args[1]);
    ::Color a2 = unpack_color(vm, args[2]);
    ::DrawTriangleStrip(v_a0.data(), a1, a2);
    return 0;
}

static uint64_t wrap_draw_rectangle(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    int a1 = static_cast<int>(args[1]);
    int a2 = static_cast<int>(args[2]);
    int a3 = static_cast<int>(args[3]);
    ::Color a4 = unpack_color(vm, args[4]);
    ::DrawRectangle(a0, a1, a2, a3, a4);
    return 0;
}

static uint64_t wrap_draw_rectangle_v(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector2 a0 = unpack_vector2(vm, args[0]);
    ::Vector2 a1 = unpack_vector2(vm, args[1]);
    ::Color a2 = unpack_color(vm, args[2]);
    ::DrawRectangleV(a0, a1, a2);
    return 0;
}

static uint64_t wrap_draw_rectangle_rec(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Rectangle a0 = unpack_rectangle(vm, args[0]);
    ::Color a1 = unpack_color(vm, args[1]);
    ::DrawRectangleRec(a0, a1);
    return 0;
}

static uint64_t wrap_draw_rectangle_pro(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Rectangle a0 = unpack_rectangle(vm, args[0]);
    ::Vector2 a1 = unpack_vector2(vm, args[1]);
    float a2 = unpack_f32_arg(args[2]);
    ::Color a3 = unpack_color(vm, args[3]);
    ::DrawRectanglePro(a0, a1, a2, a3);
    return 0;
}

static uint64_t wrap_draw_rectangle_gradient_v(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    int a1 = static_cast<int>(args[1]);
    int a2 = static_cast<int>(args[2]);
    int a3 = static_cast<int>(args[3]);
    ::Color a4 = unpack_color(vm, args[4]);
    ::Color a5 = unpack_color(vm, args[5]);
    ::DrawRectangleGradientV(a0, a1, a2, a3, a4, a5);
    return 0;
}

static uint64_t wrap_draw_rectangle_gradient_h(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    int a1 = static_cast<int>(args[1]);
    int a2 = static_cast<int>(args[2]);
    int a3 = static_cast<int>(args[3]);
    ::Color a4 = unpack_color(vm, args[4]);
    ::Color a5 = unpack_color(vm, args[5]);
    ::DrawRectangleGradientH(a0, a1, a2, a3, a4, a5);
    return 0;
}

static uint64_t wrap_draw_rectangle_gradient_ex(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Rectangle a0 = unpack_rectangle(vm, args[0]);
    ::Color a1 = unpack_color(vm, args[1]);
    ::Color a2 = unpack_color(vm, args[2]);
    ::Color a3 = unpack_color(vm, args[3]);
    ::Color a4 = unpack_color(vm, args[4]);
    ::DrawRectangleGradientEx(a0, a1, a2, a3, a4);
    return 0;
}

static uint64_t wrap_draw_rectangle_lines(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    int a1 = static_cast<int>(args[1]);
    int a2 = static_cast<int>(args[2]);
    int a3 = static_cast<int>(args[3]);
    ::Color a4 = unpack_color(vm, args[4]);
    ::DrawRectangleLines(a0, a1, a2, a3, a4);
    return 0;
}

static uint64_t wrap_draw_rectangle_lines_ex(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Rectangle a0 = unpack_rectangle(vm, args[0]);
    float a1 = unpack_f32_arg(args[1]);
    ::Color a2 = unpack_color(vm, args[2]);
    ::DrawRectangleLinesEx(a0, a1, a2);
    return 0;
}

static uint64_t wrap_draw_rectangle_rounded(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Rectangle a0 = unpack_rectangle(vm, args[0]);
    float a1 = unpack_f32_arg(args[1]);
    int a2 = static_cast<int>(args[2]);
    ::Color a3 = unpack_color(vm, args[3]);
    ::DrawRectangleRounded(a0, a1, a2, a3);
    return 0;
}

static uint64_t wrap_draw_rectangle_rounded_lines(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Rectangle a0 = unpack_rectangle(vm, args[0]);
    float a1 = unpack_f32_arg(args[1]);
    int a2 = static_cast<int>(args[2]);
    ::Color a3 = unpack_color(vm, args[3]);
    ::DrawRectangleRoundedLines(a0, a1, a2, a3);
    return 0;
}

static uint64_t wrap_draw_rectangle_rounded_lines_ex(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Rectangle a0 = unpack_rectangle(vm, args[0]);
    float a1 = unpack_f32_arg(args[1]);
    int a2 = static_cast<int>(args[2]);
    float a3 = unpack_f32_arg(args[3]);
    ::Color a4 = unpack_color(vm, args[4]);
    ::DrawRectangleRoundedLinesEx(a0, a1, a2, a3, a4);
    return 0;
}

static uint64_t wrap_draw_poly(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector2 a0 = unpack_vector2(vm, args[0]);
    int a1 = static_cast<int>(args[1]);
    float a2 = unpack_f32_arg(args[2]);
    float a3 = unpack_f32_arg(args[3]);
    ::Color a4 = unpack_color(vm, args[4]);
    ::DrawPoly(a0, a1, a2, a3, a4);
    return 0;
}

static uint64_t wrap_draw_poly_lines(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector2 a0 = unpack_vector2(vm, args[0]);
    int a1 = static_cast<int>(args[1]);
    float a2 = unpack_f32_arg(args[2]);
    float a3 = unpack_f32_arg(args[3]);
    ::Color a4 = unpack_color(vm, args[4]);
    ::DrawPolyLines(a0, a1, a2, a3, a4);
    return 0;
}

static uint64_t wrap_draw_poly_lines_ex(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector2 a0 = unpack_vector2(vm, args[0]);
    int a1 = static_cast<int>(args[1]);
    float a2 = unpack_f32_arg(args[2]);
    float a3 = unpack_f32_arg(args[3]);
    float a4 = unpack_f32_arg(args[4]);
    ::Color a5 = unpack_color(vm, args[5]);
    ::DrawPolyLinesEx(a0, a1, a2, a3, a4, a5);
    return 0;
}

static uint64_t wrap_draw_circle(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    int a1 = static_cast<int>(args[1]);
    float a2 = unpack_f32_arg(args[2]);
    ::Color a3 = unpack_color(vm, args[3]);
    ::DrawCircle(a0, a1, a2, a3);
    return 0;
}

static uint64_t wrap_draw_circle_v(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector2 a0 = unpack_vector2(vm, args[0]);
    float a1 = unpack_f32_arg(args[1]);
    ::Color a2 = unpack_color(vm, args[2]);
    ::DrawCircleV(a0, a1, a2);
    return 0;
}

static uint64_t wrap_draw_circle_gradient(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector2 a0 = unpack_vector2(vm, args[0]);
    float a1 = unpack_f32_arg(args[1]);
    ::Color a2 = unpack_color(vm, args[2]);
    ::Color a3 = unpack_color(vm, args[3]);
    ::DrawCircleGradient(a0, a1, a2, a3);
    return 0;
}

static uint64_t wrap_draw_circle_sector(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector2 a0 = unpack_vector2(vm, args[0]);
    float a1 = unpack_f32_arg(args[1]);
    float a2 = unpack_f32_arg(args[2]);
    float a3 = unpack_f32_arg(args[3]);
    int a4 = static_cast<int>(args[4]);
    ::Color a5 = unpack_color(vm, args[5]);
    ::DrawCircleSector(a0, a1, a2, a3, a4, a5);
    return 0;
}

static uint64_t wrap_draw_circle_sector_lines(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector2 a0 = unpack_vector2(vm, args[0]);
    float a1 = unpack_f32_arg(args[1]);
    float a2 = unpack_f32_arg(args[2]);
    float a3 = unpack_f32_arg(args[3]);
    int a4 = static_cast<int>(args[4]);
    ::Color a5 = unpack_color(vm, args[5]);
    ::DrawCircleSectorLines(a0, a1, a2, a3, a4, a5);
    return 0;
}

static uint64_t wrap_draw_circle_sector_lines_ex(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector2 a0 = unpack_vector2(vm, args[0]);
    float a1 = unpack_f32_arg(args[1]);
    float a2 = unpack_f32_arg(args[2]);
    float a3 = unpack_f32_arg(args[3]);
    int a4 = static_cast<int>(args[4]);
    float a5 = unpack_f32_arg(args[5]);
    ::Color a6 = unpack_color(vm, args[6]);
    ::DrawCircleSectorLinesEx(a0, a1, a2, a3, a4, a5, a6);
    return 0;
}

static uint64_t wrap_draw_circle_lines(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    int a1 = static_cast<int>(args[1]);
    float a2 = unpack_f32_arg(args[2]);
    ::Color a3 = unpack_color(vm, args[3]);
    ::DrawCircleLines(a0, a1, a2, a3);
    return 0;
}

static uint64_t wrap_draw_circle_lines_v(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector2 a0 = unpack_vector2(vm, args[0]);
    float a1 = unpack_f32_arg(args[1]);
    ::Color a2 = unpack_color(vm, args[2]);
    ::DrawCircleLinesV(a0, a1, a2);
    return 0;
}

static uint64_t wrap_draw_circle_lines_ex(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector2 a0 = unpack_vector2(vm, args[0]);
    float a1 = unpack_f32_arg(args[1]);
    float a2 = unpack_f32_arg(args[2]);
    ::Color a3 = unpack_color(vm, args[3]);
    ::DrawCircleLinesEx(a0, a1, a2, a3);
    return 0;
}

static uint64_t wrap_draw_ellipse(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    int a1 = static_cast<int>(args[1]);
    float a2 = unpack_f32_arg(args[2]);
    float a3 = unpack_f32_arg(args[3]);
    ::Color a4 = unpack_color(vm, args[4]);
    ::DrawEllipse(a0, a1, a2, a3, a4);
    return 0;
}

static uint64_t wrap_draw_ellipse_v(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector2 a0 = unpack_vector2(vm, args[0]);
    float a1 = unpack_f32_arg(args[1]);
    float a2 = unpack_f32_arg(args[2]);
    ::Color a3 = unpack_color(vm, args[3]);
    ::DrawEllipseV(a0, a1, a2, a3);
    return 0;
}

static uint64_t wrap_draw_ellipse_lines(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    int a1 = static_cast<int>(args[1]);
    float a2 = unpack_f32_arg(args[2]);
    float a3 = unpack_f32_arg(args[3]);
    ::Color a4 = unpack_color(vm, args[4]);
    ::DrawEllipseLines(a0, a1, a2, a3, a4);
    return 0;
}

static uint64_t wrap_draw_ellipse_lines_v(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector2 a0 = unpack_vector2(vm, args[0]);
    float a1 = unpack_f32_arg(args[1]);
    float a2 = unpack_f32_arg(args[2]);
    ::Color a3 = unpack_color(vm, args[3]);
    ::DrawEllipseLinesV(a0, a1, a2, a3);
    return 0;
}

static uint64_t wrap_draw_ellipse_lines_ex(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector2 a0 = unpack_vector2(vm, args[0]);
    float a1 = unpack_f32_arg(args[1]);
    float a2 = unpack_f32_arg(args[2]);
    float a3 = unpack_f32_arg(args[3]);
    ::Color a4 = unpack_color(vm, args[4]);
    ::DrawEllipseLinesEx(a0, a1, a2, a3, a4);
    return 0;
}

static uint64_t wrap_draw_ring(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector2 a0 = unpack_vector2(vm, args[0]);
    float a1 = unpack_f32_arg(args[1]);
    float a2 = unpack_f32_arg(args[2]);
    float a3 = unpack_f32_arg(args[3]);
    float a4 = unpack_f32_arg(args[4]);
    int a5 = static_cast<int>(args[5]);
    ::Color a6 = unpack_color(vm, args[6]);
    ::DrawRing(a0, a1, a2, a3, a4, a5, a6);
    return 0;
}

static uint64_t wrap_draw_ring_lines(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector2 a0 = unpack_vector2(vm, args[0]);
    float a1 = unpack_f32_arg(args[1]);
    float a2 = unpack_f32_arg(args[2]);
    float a3 = unpack_f32_arg(args[3]);
    float a4 = unpack_f32_arg(args[4]);
    int a5 = static_cast<int>(args[5]);
    ::Color a6 = unpack_color(vm, args[6]);
    ::DrawRingLines(a0, a1, a2, a3, a4, a5, a6);
    return 0;
}

static uint64_t wrap_draw_ring_lines_ex(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector2 a0 = unpack_vector2(vm, args[0]);
    float a1 = unpack_f32_arg(args[1]);
    float a2 = unpack_f32_arg(args[2]);
    float a3 = unpack_f32_arg(args[3]);
    float a4 = unpack_f32_arg(args[4]);
    int a5 = static_cast<int>(args[5]);
    float a6 = unpack_f32_arg(args[6]);
    ::Color a7 = unpack_color(vm, args[7]);
    ::DrawRingLinesEx(a0, a1, a2, a3, a4, a5, a6, a7);
    return 0;
}

static uint64_t wrap_draw_spline_linear(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    auto v_a0 = unpack_vector2_array(vm, args[0], 0);
    int a1 = static_cast<int>(args[1]);
    float a2 = unpack_f32_arg(args[2]);
    ::Color a3 = unpack_color(vm, args[3]);
    ::DrawSplineLinear(v_a0.data(), a1, a2, a3);
    return 0;
}

static uint64_t wrap_draw_spline_basis(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    auto v_a0 = unpack_vector2_array(vm, args[0], 0);
    int a1 = static_cast<int>(args[1]);
    float a2 = unpack_f32_arg(args[2]);
    ::Color a3 = unpack_color(vm, args[3]);
    ::DrawSplineBasis(v_a0.data(), a1, a2, a3);
    return 0;
}

static uint64_t wrap_draw_spline_catmull_rom(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    auto v_a0 = unpack_vector2_array(vm, args[0], 0);
    int a1 = static_cast<int>(args[1]);
    float a2 = unpack_f32_arg(args[2]);
    ::Color a3 = unpack_color(vm, args[3]);
    ::DrawSplineCatmullRom(v_a0.data(), a1, a2, a3);
    return 0;
}

static uint64_t wrap_draw_spline_bezier_quadratic(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    auto v_a0 = unpack_vector2_array(vm, args[0], 0);
    int a1 = static_cast<int>(args[1]);
    float a2 = unpack_f32_arg(args[2]);
    ::Color a3 = unpack_color(vm, args[3]);
    ::DrawSplineBezierQuadratic(v_a0.data(), a1, a2, a3);
    return 0;
}

static uint64_t wrap_draw_spline_bezier_cubic(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    auto v_a0 = unpack_vector2_array(vm, args[0], 0);
    int a1 = static_cast<int>(args[1]);
    float a2 = unpack_f32_arg(args[2]);
    ::Color a3 = unpack_color(vm, args[3]);
    ::DrawSplineBezierCubic(v_a0.data(), a1, a2, a3);
    return 0;
}

static uint64_t wrap_draw_spline_segment_linear(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector2 a0 = unpack_vector2(vm, args[0]);
    ::Vector2 a1 = unpack_vector2(vm, args[1]);
    float a2 = unpack_f32_arg(args[2]);
    ::Color a3 = unpack_color(vm, args[3]);
    ::DrawSplineSegmentLinear(a0, a1, a2, a3);
    return 0;
}

static uint64_t wrap_draw_spline_segment_basis(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector2 a0 = unpack_vector2(vm, args[0]);
    ::Vector2 a1 = unpack_vector2(vm, args[1]);
    ::Vector2 a2 = unpack_vector2(vm, args[2]);
    ::Vector2 a3 = unpack_vector2(vm, args[3]);
    float a4 = unpack_f32_arg(args[4]);
    ::Color a5 = unpack_color(vm, args[5]);
    ::DrawSplineSegmentBasis(a0, a1, a2, a3, a4, a5);
    return 0;
}

static uint64_t wrap_draw_spline_segment_catmull_rom(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector2 a0 = unpack_vector2(vm, args[0]);
    ::Vector2 a1 = unpack_vector2(vm, args[1]);
    ::Vector2 a2 = unpack_vector2(vm, args[2]);
    ::Vector2 a3 = unpack_vector2(vm, args[3]);
    float a4 = unpack_f32_arg(args[4]);
    ::Color a5 = unpack_color(vm, args[5]);
    ::DrawSplineSegmentCatmullRom(a0, a1, a2, a3, a4, a5);
    return 0;
}

static uint64_t wrap_draw_spline_segment_bezier_quadratic(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector2 a0 = unpack_vector2(vm, args[0]);
    ::Vector2 a1 = unpack_vector2(vm, args[1]);
    ::Vector2 a2 = unpack_vector2(vm, args[2]);
    float a3 = unpack_f32_arg(args[3]);
    ::Color a4 = unpack_color(vm, args[4]);
    ::DrawSplineSegmentBezierQuadratic(a0, a1, a2, a3, a4);
    return 0;
}

static uint64_t wrap_draw_spline_segment_bezier_cubic(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector2 a0 = unpack_vector2(vm, args[0]);
    ::Vector2 a1 = unpack_vector2(vm, args[1]);
    ::Vector2 a2 = unpack_vector2(vm, args[2]);
    ::Vector2 a3 = unpack_vector2(vm, args[3]);
    float a4 = unpack_f32_arg(args[4]);
    ::Color a5 = unpack_color(vm, args[5]);
    ::DrawSplineSegmentBezierCubic(a0, a1, a2, a3, a4, a5);
    return 0;
}

static uint64_t wrap_get_spline_point_linear(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector2 a0 = unpack_vector2(vm, args[0]);
    ::Vector2 a1 = unpack_vector2(vm, args[1]);
    float a2 = unpack_f32_arg(args[2]);
    auto res = ::GetSplinePointLinear(a0, a1, a2);
    pack_vector2(vm, args[3], res);
    return 0;
}

static uint64_t wrap_get_spline_point_basis(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector2 a0 = unpack_vector2(vm, args[0]);
    ::Vector2 a1 = unpack_vector2(vm, args[1]);
    ::Vector2 a2 = unpack_vector2(vm, args[2]);
    ::Vector2 a3 = unpack_vector2(vm, args[3]);
    float a4 = unpack_f32_arg(args[4]);
    auto res = ::GetSplinePointBasis(a0, a1, a2, a3, a4);
    pack_vector2(vm, args[5], res);
    return 0;
}

static uint64_t wrap_get_spline_point_catmull_rom(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector2 a0 = unpack_vector2(vm, args[0]);
    ::Vector2 a1 = unpack_vector2(vm, args[1]);
    ::Vector2 a2 = unpack_vector2(vm, args[2]);
    ::Vector2 a3 = unpack_vector2(vm, args[3]);
    float a4 = unpack_f32_arg(args[4]);
    auto res = ::GetSplinePointCatmullRom(a0, a1, a2, a3, a4);
    pack_vector2(vm, args[5], res);
    return 0;
}

static uint64_t wrap_get_spline_point_bezier_quadratic(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector2 a0 = unpack_vector2(vm, args[0]);
    ::Vector2 a1 = unpack_vector2(vm, args[1]);
    ::Vector2 a2 = unpack_vector2(vm, args[2]);
    float a3 = unpack_f32_arg(args[3]);
    auto res = ::GetSplinePointBezierQuadratic(a0, a1, a2, a3);
    pack_vector2(vm, args[4], res);
    return 0;
}

static uint64_t wrap_get_spline_point_bezier_cubic(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector2 a0 = unpack_vector2(vm, args[0]);
    ::Vector2 a1 = unpack_vector2(vm, args[1]);
    ::Vector2 a2 = unpack_vector2(vm, args[2]);
    ::Vector2 a3 = unpack_vector2(vm, args[3]);
    float a4 = unpack_f32_arg(args[4]);
    auto res = ::GetSplinePointBezierCubic(a0, a1, a2, a3, a4);
    pack_vector2(vm, args[5], res);
    return 0;
}

static uint64_t wrap_check_collision_recs(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Rectangle a0 = unpack_rectangle(vm, args[0]);
    ::Rectangle a1 = unpack_rectangle(vm, args[1]);
    bool res = ::CheckCollisionRecs(a0, a1);
    return res ? 1 : 0;
}

static uint64_t wrap_check_collision_circles(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector2 a0 = unpack_vector2(vm, args[0]);
    float a1 = unpack_f32_arg(args[1]);
    ::Vector2 a2 = unpack_vector2(vm, args[2]);
    float a3 = unpack_f32_arg(args[3]);
    bool res = ::CheckCollisionCircles(a0, a1, a2, a3);
    return res ? 1 : 0;
}

static uint64_t wrap_check_collision_circle_rec(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector2 a0 = unpack_vector2(vm, args[0]);
    float a1 = unpack_f32_arg(args[1]);
    ::Rectangle a2 = unpack_rectangle(vm, args[2]);
    bool res = ::CheckCollisionCircleRec(a0, a1, a2);
    return res ? 1 : 0;
}

static uint64_t wrap_check_collision_circle_line(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector2 a0 = unpack_vector2(vm, args[0]);
    float a1 = unpack_f32_arg(args[1]);
    ::Vector2 a2 = unpack_vector2(vm, args[2]);
    ::Vector2 a3 = unpack_vector2(vm, args[3]);
    bool res = ::CheckCollisionCircleLine(a0, a1, a2, a3);
    return res ? 1 : 0;
}

static uint64_t wrap_check_collision_point_rec(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector2 a0 = unpack_vector2(vm, args[0]);
    ::Rectangle a1 = unpack_rectangle(vm, args[1]);
    bool res = ::CheckCollisionPointRec(a0, a1);
    return res ? 1 : 0;
}

static uint64_t wrap_check_collision_point_circle(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector2 a0 = unpack_vector2(vm, args[0]);
    ::Vector2 a1 = unpack_vector2(vm, args[1]);
    float a2 = unpack_f32_arg(args[2]);
    bool res = ::CheckCollisionPointCircle(a0, a1, a2);
    return res ? 1 : 0;
}

static uint64_t wrap_check_collision_point_triangle(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector2 a0 = unpack_vector2(vm, args[0]);
    ::Vector2 a1 = unpack_vector2(vm, args[1]);
    ::Vector2 a2 = unpack_vector2(vm, args[2]);
    ::Vector2 a3 = unpack_vector2(vm, args[3]);
    bool res = ::CheckCollisionPointTriangle(a0, a1, a2, a3);
    return res ? 1 : 0;
}

static uint64_t wrap_check_collision_point_line(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector2 a0 = unpack_vector2(vm, args[0]);
    ::Vector2 a1 = unpack_vector2(vm, args[1]);
    ::Vector2 a2 = unpack_vector2(vm, args[2]);
    int a3 = static_cast<int>(args[3]);
    bool res = ::CheckCollisionPointLine(a0, a1, a2, a3);
    return res ? 1 : 0;
}

static uint64_t wrap_check_collision_point_poly(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector2 a0 = unpack_vector2(vm, args[0]);
    auto v_a1 = unpack_vector2_array(vm, args[1], 0);
    int a2 = static_cast<int>(args[2]);
    bool res = ::CheckCollisionPointPoly(a0, v_a1.data(), a2);
    return res ? 1 : 0;
}

static uint64_t wrap_check_collision_lines(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector2 a0 = unpack_vector2(vm, args[0]);
    ::Vector2 a1 = unpack_vector2(vm, args[1]);
    ::Vector2 a2 = unpack_vector2(vm, args[2]);
    ::Vector2 a3 = unpack_vector2(vm, args[3]);
    auto v_a4 = unpack_vector2_array(vm, args[4], 0);
    bool res = ::CheckCollisionLines(a0, a1, a2, a3, v_a4.data());
    return res ? 1 : 0;
}

static uint64_t wrap_get_collision_rec(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Rectangle a0 = unpack_rectangle(vm, args[0]);
    ::Rectangle a1 = unpack_rectangle(vm, args[1]);
    auto res = ::GetCollisionRec(a0, a1);
    pack_rectangle(vm, args[2], res);
    return 0;
}

static uint64_t wrap_load_image(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    auto res = ::LoadImage(a0);
    pack_image(vm, args[1], res);
    return 0;
}

static uint64_t wrap_load_image_raw(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    int a1 = static_cast<int>(args[1]);
    int a2 = static_cast<int>(args[2]);
    int a3 = static_cast<int>(args[3]);
    int a4 = static_cast<int>(args[4]);
    auto res = ::LoadImageRaw(a0, a1, a2, a3, a4);
    pack_image(vm, args[5], res);
    return 0;
}

static uint64_t wrap_load_image_anim(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    auto v_a1 = unpack_int32_array(vm, args[1], 0);
    auto res = ::LoadImageAnim(a0, v_a1.data());
    pack_image(vm, args[2], res);
    return 0;
}

static uint64_t wrap_load_image_anim_from_memory(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    auto v_a1 = unpack_uint8_array(vm, args[1], 0);
    int a2 = static_cast<int>(args[2]);
    auto v_a3 = unpack_int32_array(vm, args[3], 0);
    auto res = ::LoadImageAnimFromMemory(a0, v_a1.data(), a2, v_a3.data());
    pack_image(vm, args[4], res);
    return 0;
}

static uint64_t wrap_load_image_from_memory(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    auto v_a1 = unpack_uint8_array(vm, args[1], 0);
    int a2 = static_cast<int>(args[2]);
    auto res = ::LoadImageFromMemory(a0, v_a1.data(), a2);
    pack_image(vm, args[3], res);
    return 0;
}

static uint64_t wrap_load_image_from_texture(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Texture2D a0 = unpack_texture2d(vm, args[0]);
    auto res = ::LoadImageFromTexture(a0);
    pack_image(vm, args[1], res);
    return 0;
}

static uint64_t wrap_load_image_from_screen(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    auto res = ::LoadImageFromScreen();
    pack_image(vm, args[0], res);
    return 0;
}

static uint64_t wrap_is_image_valid(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    bool res = ::IsImageValid(a0);
    return res ? 1 : 0;
}

static uint64_t wrap_unload_image(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    ::UnloadImage(a0);
    return 0;
}

static uint64_t wrap_export_image(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    std::string s_a1 = read_solix_chars(vm, args[1]);
    const char *a1 = s_a1.c_str();
    bool res = ::ExportImage(a0, a1);
    return res ? 1 : 0;
}

static uint64_t wrap_export_image_to_memory(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    std::string s_a1 = read_solix_chars(vm, args[1]);
    const char *a1 = s_a1.c_str();
    auto v_a2 = unpack_int32_array(vm, args[2], 0);
    auto res = ::ExportImageToMemory(a0, a1, v_a2.data());
    return reinterpret_cast<uint64_t>(res);
}

static uint64_t wrap_export_image_as_code(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    std::string s_a1 = read_solix_chars(vm, args[1]);
    const char *a1 = s_a1.c_str();
    bool res = ::ExportImageAsCode(a0, a1);
    return res ? 1 : 0;
}

static uint64_t wrap_gen_image_color(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    int a1 = static_cast<int>(args[1]);
    ::Color a2 = unpack_color(vm, args[2]);
    auto res = ::GenImageColor(a0, a1, a2);
    pack_image(vm, args[3], res);
    return 0;
}

static uint64_t wrap_gen_image_gradient_linear(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    int a1 = static_cast<int>(args[1]);
    int a2 = static_cast<int>(args[2]);
    ::Color a3 = unpack_color(vm, args[3]);
    ::Color a4 = unpack_color(vm, args[4]);
    auto res = ::GenImageGradientLinear(a0, a1, a2, a3, a4);
    pack_image(vm, args[5], res);
    return 0;
}

static uint64_t wrap_gen_image_gradient_radial(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    int a1 = static_cast<int>(args[1]);
    float a2 = unpack_f32_arg(args[2]);
    ::Color a3 = unpack_color(vm, args[3]);
    ::Color a4 = unpack_color(vm, args[4]);
    auto res = ::GenImageGradientRadial(a0, a1, a2, a3, a4);
    pack_image(vm, args[5], res);
    return 0;
}

static uint64_t wrap_gen_image_gradient_square(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    int a1 = static_cast<int>(args[1]);
    float a2 = unpack_f32_arg(args[2]);
    ::Color a3 = unpack_color(vm, args[3]);
    ::Color a4 = unpack_color(vm, args[4]);
    auto res = ::GenImageGradientSquare(a0, a1, a2, a3, a4);
    pack_image(vm, args[5], res);
    return 0;
}

static uint64_t wrap_gen_image_checked(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    int a1 = static_cast<int>(args[1]);
    int a2 = static_cast<int>(args[2]);
    int a3 = static_cast<int>(args[3]);
    ::Color a4 = unpack_color(vm, args[4]);
    ::Color a5 = unpack_color(vm, args[5]);
    auto res = ::GenImageChecked(a0, a1, a2, a3, a4, a5);
    pack_image(vm, args[6], res);
    return 0;
}

static uint64_t wrap_gen_image_white_noise(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    int a1 = static_cast<int>(args[1]);
    float a2 = unpack_f32_arg(args[2]);
    auto res = ::GenImageWhiteNoise(a0, a1, a2);
    pack_image(vm, args[3], res);
    return 0;
}

static uint64_t wrap_gen_image_perlin_noise(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    int a1 = static_cast<int>(args[1]);
    int a2 = static_cast<int>(args[2]);
    int a3 = static_cast<int>(args[3]);
    float a4 = unpack_f32_arg(args[4]);
    auto res = ::GenImagePerlinNoise(a0, a1, a2, a3, a4);
    pack_image(vm, args[5], res);
    return 0;
}

static uint64_t wrap_gen_image_cellular(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    int a1 = static_cast<int>(args[1]);
    int a2 = static_cast<int>(args[2]);
    auto res = ::GenImageCellular(a0, a1, a2);
    pack_image(vm, args[3], res);
    return 0;
}

static uint64_t wrap_gen_image_text(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    int a1 = static_cast<int>(args[1]);
    std::string s_a2 = read_solix_chars(vm, args[2]);
    const char *a2 = s_a2.c_str();
    auto res = ::GenImageText(a0, a1, a2);
    pack_image(vm, args[3], res);
    return 0;
}

static uint64_t wrap_image_copy(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    auto res = ::ImageCopy(a0);
    pack_image(vm, args[1], res);
    return 0;
}

static uint64_t wrap_image_from_image(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    ::Rectangle a1 = unpack_rectangle(vm, args[1]);
    auto res = ::ImageFromImage(a0, a1);
    pack_image(vm, args[2], res);
    return 0;
}

static uint64_t wrap_image_from_channel(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    int a1 = static_cast<int>(args[1]);
    auto res = ::ImageFromChannel(a0, a1);
    pack_image(vm, args[2], res);
    return 0;
}

static uint64_t wrap_image_text(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    int a1 = static_cast<int>(args[1]);
    ::Color a2 = unpack_color(vm, args[2]);
    auto res = ::ImageText(a0, a1, a2);
    pack_image(vm, args[3], res);
    return 0;
}

static uint64_t wrap_image_text_ex(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Font a0 = unpack_font(vm, args[0]);
    std::string s_a1 = read_solix_chars(vm, args[1]);
    const char *a1 = s_a1.c_str();
    float a2 = unpack_f32_arg(args[2]);
    float a3 = unpack_f32_arg(args[3]);
    ::Color a4 = unpack_color(vm, args[4]);
    auto res = ::ImageTextEx(a0, a1, a2, a3, a4);
    pack_image(vm, args[5], res);
    return 0;
}

static uint64_t wrap_image_format(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    int a1 = static_cast<int>(args[1]);
    ::ImageFormat(&a0, a1);
    pack_image(vm, args[0], a0);
    return 0;
}

static uint64_t wrap_image_to_pot(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    ::Color a1 = unpack_color(vm, args[1]);
    ::ImageToPOT(&a0, a1);
    pack_image(vm, args[0], a0);
    return 0;
}

static uint64_t wrap_image_crop(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    ::Rectangle a1 = unpack_rectangle(vm, args[1]);
    ::ImageCrop(&a0, a1);
    pack_image(vm, args[0], a0);
    return 0;
}

static uint64_t wrap_image_alpha_crop(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    float a1 = unpack_f32_arg(args[1]);
    ::ImageAlphaCrop(&a0, a1);
    pack_image(vm, args[0], a0);
    return 0;
}

static uint64_t wrap_image_alpha_clear(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    ::Color a1 = unpack_color(vm, args[1]);
    float a2 = unpack_f32_arg(args[2]);
    ::ImageAlphaClear(&a0, a1, a2);
    pack_image(vm, args[0], a0);
    return 0;
}

static uint64_t wrap_image_alpha_mask(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    ::Image a1 = unpack_image(vm, args[1]);
    ::ImageAlphaMask(&a0, a1);
    pack_image(vm, args[0], a0);
    return 0;
}

static uint64_t wrap_image_alpha_premultiply(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    ::ImageAlphaPremultiply(&a0);
    pack_image(vm, args[0], a0);
    return 0;
}

static uint64_t wrap_image_blur_gaussian(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    int a1 = static_cast<int>(args[1]);
    ::ImageBlurGaussian(&a0, a1);
    pack_image(vm, args[0], a0);
    return 0;
}

static uint64_t wrap_image_kernel_convolution(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    auto v_a1 = unpack_float32_array(vm, args[1], 0);
    int a2 = static_cast<int>(args[2]);
    ::ImageKernelConvolution(&a0, v_a1.data(), a2);
    pack_image(vm, args[0], a0);
    return 0;
}

static uint64_t wrap_image_resize(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    int a1 = static_cast<int>(args[1]);
    int a2 = static_cast<int>(args[2]);
    ::ImageResize(&a0, a1, a2);
    pack_image(vm, args[0], a0);
    return 0;
}

static uint64_t wrap_image_resize_nn(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    int a1 = static_cast<int>(args[1]);
    int a2 = static_cast<int>(args[2]);
    ::ImageResizeNN(&a0, a1, a2);
    pack_image(vm, args[0], a0);
    return 0;
}

static uint64_t wrap_image_resize_canvas(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    int a1 = static_cast<int>(args[1]);
    int a2 = static_cast<int>(args[2]);
    int a3 = static_cast<int>(args[3]);
    int a4 = static_cast<int>(args[4]);
    ::Color a5 = unpack_color(vm, args[5]);
    ::ImageResizeCanvas(&a0, a1, a2, a3, a4, a5);
    pack_image(vm, args[0], a0);
    return 0;
}

static uint64_t wrap_image_mipmaps(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    ::ImageMipmaps(&a0);
    pack_image(vm, args[0], a0);
    return 0;
}

static uint64_t wrap_image_dither(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    int a1 = static_cast<int>(args[1]);
    int a2 = static_cast<int>(args[2]);
    int a3 = static_cast<int>(args[3]);
    int a4 = static_cast<int>(args[4]);
    ::ImageDither(&a0, a1, a2, a3, a4);
    pack_image(vm, args[0], a0);
    return 0;
}

static uint64_t wrap_image_flip_vertical(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    ::ImageFlipVertical(&a0);
    pack_image(vm, args[0], a0);
    return 0;
}

static uint64_t wrap_image_flip_horizontal(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    ::ImageFlipHorizontal(&a0);
    pack_image(vm, args[0], a0);
    return 0;
}

static uint64_t wrap_image_rotate(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    int a1 = static_cast<int>(args[1]);
    ::ImageRotate(&a0, a1);
    pack_image(vm, args[0], a0);
    return 0;
}

static uint64_t wrap_image_rotate_cw(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    ::ImageRotateCW(&a0);
    pack_image(vm, args[0], a0);
    return 0;
}

static uint64_t wrap_image_rotate_ccw(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    ::ImageRotateCCW(&a0);
    pack_image(vm, args[0], a0);
    return 0;
}

static uint64_t wrap_image_color_tint(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    ::Color a1 = unpack_color(vm, args[1]);
    ::ImageColorTint(&a0, a1);
    pack_image(vm, args[0], a0);
    return 0;
}

static uint64_t wrap_image_color_invert(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    ::ImageColorInvert(&a0);
    pack_image(vm, args[0], a0);
    return 0;
}

static uint64_t wrap_image_color_grayscale(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    ::ImageColorGrayscale(&a0);
    pack_image(vm, args[0], a0);
    return 0;
}

static uint64_t wrap_image_color_contrast(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    int a1 = static_cast<int>(args[1]);
    ::ImageColorContrast(&a0, a1);
    pack_image(vm, args[0], a0);
    return 0;
}

static uint64_t wrap_image_color_brightness(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    int a1 = static_cast<int>(args[1]);
    ::ImageColorBrightness(&a0, a1);
    pack_image(vm, args[0], a0);
    return 0;
}

static uint64_t wrap_image_color_replace(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    ::Color a1 = unpack_color(vm, args[1]);
    ::Color a2 = unpack_color(vm, args[2]);
    ::ImageColorReplace(&a0, a1, a2);
    pack_image(vm, args[0], a0);
    return 0;
}

static uint64_t wrap_load_image_colors(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    auto res = ::LoadImageColors(a0);
    return reinterpret_cast<uint64_t>(res);
}

static uint64_t wrap_load_image_palette(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    int a1 = static_cast<int>(args[1]);
    auto v_a2 = unpack_int32_array(vm, args[2], 0);
    auto res = ::LoadImagePalette(a0, a1, v_a2.data());
    return reinterpret_cast<uint64_t>(res);
}

static uint64_t wrap_unload_image_colors(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    auto v_a0 = unpack_color_array(vm, args[0], 0);
    ::UnloadImageColors(v_a0.data());
    return 0;
}

static uint64_t wrap_unload_image_palette(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    auto v_a0 = unpack_color_array(vm, args[0], 0);
    ::UnloadImagePalette(v_a0.data());
    return 0;
}

static uint64_t wrap_get_image_alpha_border(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    float a1 = unpack_f32_arg(args[1]);
    auto res = ::GetImageAlphaBorder(a0, a1);
    pack_rectangle(vm, args[2], res);
    return 0;
}

static uint64_t wrap_get_image_color(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    int a1 = static_cast<int>(args[1]);
    int a2 = static_cast<int>(args[2]);
    auto res = ::GetImageColor(a0, a1, a2);
    pack_color(vm, args[3], res);
    return 0;
}

static uint64_t wrap_image_clear_background(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    ::Color a1 = unpack_color(vm, args[1]);
    ::ImageClearBackground(&a0, a1);
    pack_image(vm, args[0], a0);
    return 0;
}

static uint64_t wrap_image_draw_pixel(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    int a1 = static_cast<int>(args[1]);
    int a2 = static_cast<int>(args[2]);
    ::Color a3 = unpack_color(vm, args[3]);
    ::ImageDrawPixel(&a0, a1, a2, a3);
    pack_image(vm, args[0], a0);
    return 0;
}

static uint64_t wrap_image_draw_pixel_v(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    ::Vector2 a1 = unpack_vector2(vm, args[1]);
    ::Color a2 = unpack_color(vm, args[2]);
    ::ImageDrawPixelV(&a0, a1, a2);
    pack_image(vm, args[0], a0);
    return 0;
}

static uint64_t wrap_image_draw_line(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    int a1 = static_cast<int>(args[1]);
    int a2 = static_cast<int>(args[2]);
    int a3 = static_cast<int>(args[3]);
    int a4 = static_cast<int>(args[4]);
    ::Color a5 = unpack_color(vm, args[5]);
    ::ImageDrawLine(&a0, a1, a2, a3, a4, a5);
    pack_image(vm, args[0], a0);
    return 0;
}

static uint64_t wrap_image_draw_line_v(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    ::Vector2 a1 = unpack_vector2(vm, args[1]);
    ::Vector2 a2 = unpack_vector2(vm, args[2]);
    ::Color a3 = unpack_color(vm, args[3]);
    ::ImageDrawLineV(&a0, a1, a2, a3);
    pack_image(vm, args[0], a0);
    return 0;
}

static uint64_t wrap_image_draw_line_ex(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    ::Vector2 a1 = unpack_vector2(vm, args[1]);
    ::Vector2 a2 = unpack_vector2(vm, args[2]);
    int a3 = static_cast<int>(args[3]);
    ::Color a4 = unpack_color(vm, args[4]);
    ::ImageDrawLineEx(&a0, a1, a2, a3, a4);
    pack_image(vm, args[0], a0);
    return 0;
}

static uint64_t wrap_image_draw_line_strip(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    auto v_a1 = unpack_vector2_array(vm, args[1], 0);
    int a2 = static_cast<int>(args[2]);
    ::Color a3 = unpack_color(vm, args[3]);
    ::ImageDrawLineStrip(&a0, v_a1.data(), a2, a3);
    pack_image(vm, args[0], a0);
    return 0;
}

static uint64_t wrap_image_draw_triangle(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    ::Vector2 a1 = unpack_vector2(vm, args[1]);
    ::Vector2 a2 = unpack_vector2(vm, args[2]);
    ::Vector2 a3 = unpack_vector2(vm, args[3]);
    ::Color a4 = unpack_color(vm, args[4]);
    ::ImageDrawTriangle(&a0, a1, a2, a3, a4);
    pack_image(vm, args[0], a0);
    return 0;
}

static uint64_t wrap_image_draw_triangle_gradient(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    ::Vector2 a1 = unpack_vector2(vm, args[1]);
    ::Vector2 a2 = unpack_vector2(vm, args[2]);
    ::Vector2 a3 = unpack_vector2(vm, args[3]);
    ::Color a4 = unpack_color(vm, args[4]);
    ::Color a5 = unpack_color(vm, args[5]);
    ::Color a6 = unpack_color(vm, args[6]);
    ::ImageDrawTriangleGradient(&a0, a1, a2, a3, a4, a5, a6);
    pack_image(vm, args[0], a0);
    return 0;
}

static uint64_t wrap_image_draw_triangle_lines(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    ::Vector2 a1 = unpack_vector2(vm, args[1]);
    ::Vector2 a2 = unpack_vector2(vm, args[2]);
    ::Vector2 a3 = unpack_vector2(vm, args[3]);
    ::Color a4 = unpack_color(vm, args[4]);
    ::ImageDrawTriangleLines(&a0, a1, a2, a3, a4);
    pack_image(vm, args[0], a0);
    return 0;
}

static uint64_t wrap_image_draw_triangle_fan(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    auto v_a1 = unpack_vector2_array(vm, args[1], 0);
    int a2 = static_cast<int>(args[2]);
    ::Color a3 = unpack_color(vm, args[3]);
    ::ImageDrawTriangleFan(&a0, v_a1.data(), a2, a3);
    pack_image(vm, args[0], a0);
    return 0;
}

static uint64_t wrap_image_draw_triangle_strip(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    auto v_a1 = unpack_vector2_array(vm, args[1], 0);
    int a2 = static_cast<int>(args[2]);
    ::Color a3 = unpack_color(vm, args[3]);
    ::ImageDrawTriangleStrip(&a0, v_a1.data(), a2, a3);
    pack_image(vm, args[0], a0);
    return 0;
}

static uint64_t wrap_image_draw_rectangle(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    int a1 = static_cast<int>(args[1]);
    int a2 = static_cast<int>(args[2]);
    int a3 = static_cast<int>(args[3]);
    int a4 = static_cast<int>(args[4]);
    ::Color a5 = unpack_color(vm, args[5]);
    ::ImageDrawRectangle(&a0, a1, a2, a3, a4, a5);
    pack_image(vm, args[0], a0);
    return 0;
}

static uint64_t wrap_image_draw_rectangle_v(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    ::Vector2 a1 = unpack_vector2(vm, args[1]);
    ::Vector2 a2 = unpack_vector2(vm, args[2]);
    ::Color a3 = unpack_color(vm, args[3]);
    ::ImageDrawRectangleV(&a0, a1, a2, a3);
    pack_image(vm, args[0], a0);
    return 0;
}

static uint64_t wrap_image_draw_rectangle_rec(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    ::Rectangle a1 = unpack_rectangle(vm, args[1]);
    ::Color a2 = unpack_color(vm, args[2]);
    ::ImageDrawRectangleRec(&a0, a1, a2);
    pack_image(vm, args[0], a0);
    return 0;
}

static uint64_t wrap_image_draw_rectangle_pro(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    ::Rectangle a1 = unpack_rectangle(vm, args[1]);
    ::Vector2 a2 = unpack_vector2(vm, args[2]);
    float a3 = unpack_f32_arg(args[3]);
    ::Color a4 = unpack_color(vm, args[4]);
    ::ImageDrawRectanglePro(&a0, a1, a2, a3, a4);
    pack_image(vm, args[0], a0);
    return 0;
}

static uint64_t wrap_image_draw_rectangle_lines(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    int a1 = static_cast<int>(args[1]);
    int a2 = static_cast<int>(args[2]);
    int a3 = static_cast<int>(args[3]);
    int a4 = static_cast<int>(args[4]);
    ::Color a5 = unpack_color(vm, args[5]);
    ::ImageDrawRectangleLines(&a0, a1, a2, a3, a4, a5);
    pack_image(vm, args[0], a0);
    return 0;
}

static uint64_t wrap_image_draw_rectangle_lines_ex(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    ::Rectangle a1 = unpack_rectangle(vm, args[1]);
    int a2 = static_cast<int>(args[2]);
    ::Color a3 = unpack_color(vm, args[3]);
    ::ImageDrawRectangleLinesEx(&a0, a1, a2, a3);
    pack_image(vm, args[0], a0);
    return 0;
}

static uint64_t wrap_image_draw_rectangle_gradient_ex(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    ::Rectangle a1 = unpack_rectangle(vm, args[1]);
    ::Color a2 = unpack_color(vm, args[2]);
    ::Color a3 = unpack_color(vm, args[3]);
    ::Color a4 = unpack_color(vm, args[4]);
    ::Color a5 = unpack_color(vm, args[5]);
    ::ImageDrawRectangleGradientEx(&a0, a1, a2, a3, a4, a5);
    pack_image(vm, args[0], a0);
    return 0;
}

static uint64_t wrap_image_draw_circle(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    int a1 = static_cast<int>(args[1]);
    int a2 = static_cast<int>(args[2]);
    int a3 = static_cast<int>(args[3]);
    ::Color a4 = unpack_color(vm, args[4]);
    ::ImageDrawCircle(&a0, a1, a2, a3, a4);
    pack_image(vm, args[0], a0);
    return 0;
}

static uint64_t wrap_image_draw_circle_v(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    ::Vector2 a1 = unpack_vector2(vm, args[1]);
    int a2 = static_cast<int>(args[2]);
    ::Color a3 = unpack_color(vm, args[3]);
    ::ImageDrawCircleV(&a0, a1, a2, a3);
    pack_image(vm, args[0], a0);
    return 0;
}

static uint64_t wrap_image_draw_circle_lines(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    int a1 = static_cast<int>(args[1]);
    int a2 = static_cast<int>(args[2]);
    int a3 = static_cast<int>(args[3]);
    ::Color a4 = unpack_color(vm, args[4]);
    ::ImageDrawCircleLines(&a0, a1, a2, a3, a4);
    pack_image(vm, args[0], a0);
    return 0;
}

static uint64_t wrap_image_draw_circle_lines_v(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    ::Vector2 a1 = unpack_vector2(vm, args[1]);
    int a2 = static_cast<int>(args[2]);
    ::Color a3 = unpack_color(vm, args[3]);
    ::ImageDrawCircleLinesV(&a0, a1, a2, a3);
    pack_image(vm, args[0], a0);
    return 0;
}

static uint64_t wrap_image_draw_circle_gradient(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    ::Vector2 a1 = unpack_vector2(vm, args[1]);
    float a2 = unpack_f32_arg(args[2]);
    ::Color a3 = unpack_color(vm, args[3]);
    ::Color a4 = unpack_color(vm, args[4]);
    ::ImageDrawCircleGradient(&a0, a1, a2, a3, a4);
    pack_image(vm, args[0], a0);
    return 0;
}

static uint64_t wrap_image_draw_image(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    ::Image a1 = unpack_image(vm, args[1]);
    int a2 = static_cast<int>(args[2]);
    int a3 = static_cast<int>(args[3]);
    ::Color a4 = unpack_color(vm, args[4]);
    ::ImageDrawImage(&a0, a1, a2, a3, a4);
    pack_image(vm, args[0], a0);
    return 0;
}

static uint64_t wrap_image_draw_image_ex(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    ::Image a1 = unpack_image(vm, args[1]);
    ::Vector2 a2 = unpack_vector2(vm, args[2]);
    float a3 = unpack_f32_arg(args[3]);
    float a4 = unpack_f32_arg(args[4]);
    ::Color a5 = unpack_color(vm, args[5]);
    ::ImageDrawImageEx(&a0, a1, a2, a3, a4, a5);
    pack_image(vm, args[0], a0);
    return 0;
}

static uint64_t wrap_image_draw_image_rec(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    ::Image a1 = unpack_image(vm, args[1]);
    ::Rectangle a2 = unpack_rectangle(vm, args[2]);
    ::Vector2 a3 = unpack_vector2(vm, args[3]);
    ::Color a4 = unpack_color(vm, args[4]);
    ::ImageDrawImageRec(&a0, a1, a2, a3, a4);
    pack_image(vm, args[0], a0);
    return 0;
}

static uint64_t wrap_image_draw_image_pro(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    ::Image a1 = unpack_image(vm, args[1]);
    ::Rectangle a2 = unpack_rectangle(vm, args[2]);
    ::Rectangle a3 = unpack_rectangle(vm, args[3]);
    ::Vector2 a4 = unpack_vector2(vm, args[4]);
    float a5 = unpack_f32_arg(args[5]);
    ::Color a6 = unpack_color(vm, args[6]);
    ::ImageDrawImagePro(&a0, a1, a2, a3, a4, a5, a6);
    pack_image(vm, args[0], a0);
    return 0;
}

static uint64_t wrap_image_draw_text(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    std::string s_a1 = read_solix_chars(vm, args[1]);
    const char *a1 = s_a1.c_str();
    int a2 = static_cast<int>(args[2]);
    int a3 = static_cast<int>(args[3]);
    int a4 = static_cast<int>(args[4]);
    ::Color a5 = unpack_color(vm, args[5]);
    ::ImageDrawText(&a0, a1, a2, a3, a4, a5);
    pack_image(vm, args[0], a0);
    return 0;
}

static uint64_t wrap_image_draw_text_ex(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    ::Font a1 = unpack_font(vm, args[1]);
    std::string s_a2 = read_solix_chars(vm, args[2]);
    const char *a2 = s_a2.c_str();
    ::Vector2 a3 = unpack_vector2(vm, args[3]);
    float a4 = unpack_f32_arg(args[4]);
    float a5 = unpack_f32_arg(args[5]);
    ::Color a6 = unpack_color(vm, args[6]);
    ::ImageDrawTextEx(&a0, a1, a2, a3, a4, a5, a6);
    pack_image(vm, args[0], a0);
    return 0;
}

static uint64_t wrap_image_draw_text_pro(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    ::Font a1 = unpack_font(vm, args[1]);
    std::string s_a2 = read_solix_chars(vm, args[2]);
    const char *a2 = s_a2.c_str();
    ::Vector2 a3 = unpack_vector2(vm, args[3]);
    ::Vector2 a4 = unpack_vector2(vm, args[4]);
    float a5 = unpack_f32_arg(args[5]);
    float a6 = unpack_f32_arg(args[6]);
    float a7 = unpack_f32_arg(args[7]);
    ::Color a8 = unpack_color(vm, args[8]);
    ::ImageDrawTextPro(&a0, a1, a2, a3, a4, a5, a6, a7, a8);
    pack_image(vm, args[0], a0);
    return 0;
}

static uint64_t wrap_load_texture(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    auto res = ::LoadTexture(a0);
    pack_texture2d(vm, args[1], res);
    return 0;
}

static uint64_t wrap_load_texture_from_image(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    auto res = ::LoadTextureFromImage(a0);
    pack_texture2d(vm, args[1], res);
    return 0;
}

static uint64_t wrap_load_texture_cubemap(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    int a1 = static_cast<int>(args[1]);
    auto res = ::LoadTextureCubemap(a0, a1);
    pack_texture2d(vm, args[2], res);
    return 0;
}

static uint64_t wrap_load_render_texture(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    int a1 = static_cast<int>(args[1]);
    auto res = ::LoadRenderTexture(a0, a1);
    pack_rendertexture2d(vm, args[2], res);
    return 0;
}

static uint64_t wrap_load_render_texture_ex(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    int a1 = static_cast<int>(args[1]);
    int a2 = static_cast<int>(args[2]);
    auto res = ::LoadRenderTextureEx(a0, a1, a2);
    pack_rendertexture2d(vm, args[3], res);
    return 0;
}

static uint64_t wrap_is_texture_valid(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Texture2D a0 = unpack_texture2d(vm, args[0]);
    bool res = ::IsTextureValid(a0);
    return res ? 1 : 0;
}

static uint64_t wrap_unload_texture(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Texture2D a0 = unpack_texture2d(vm, args[0]);
    ::UnloadTexture(a0);
    return 0;
}

static uint64_t wrap_is_render_texture_valid(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::RenderTexture2D a0 = unpack_rendertexture2d(vm, args[0]);
    bool res = ::IsRenderTextureValid(a0);
    return res ? 1 : 0;
}

static uint64_t wrap_unload_render_texture(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::RenderTexture2D a0 = unpack_rendertexture2d(vm, args[0]);
    ::UnloadRenderTexture(a0);
    return 0;
}

static uint64_t wrap_update_texture(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Texture2D a0 = unpack_texture2d(vm, args[0]);
    void *a1 = reinterpret_cast<void *>(args[1]);
    ::UpdateTexture(a0, a1);
    return 0;
}

static uint64_t wrap_update_texture_rec(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Texture2D a0 = unpack_texture2d(vm, args[0]);
    ::Rectangle a1 = unpack_rectangle(vm, args[1]);
    void *a2 = reinterpret_cast<void *>(args[2]);
    ::UpdateTextureRec(a0, a1, a2);
    return 0;
}

static uint64_t wrap_gen_texture_mipmaps(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Texture2D a0 = unpack_texture2d(vm, args[0]);
    ::GenTextureMipmaps(&a0);
    pack_texture2d(vm, args[0], a0);
    return 0;
}

static uint64_t wrap_set_texture_filter(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Texture2D a0 = unpack_texture2d(vm, args[0]);
    int a1 = static_cast<int>(args[1]);
    ::SetTextureFilter(a0, a1);
    return 0;
}

static uint64_t wrap_set_texture_wrap(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Texture2D a0 = unpack_texture2d(vm, args[0]);
    int a1 = static_cast<int>(args[1]);
    ::SetTextureWrap(a0, a1);
    return 0;
}

static uint64_t wrap_draw_texture(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Texture2D a0 = unpack_texture2d(vm, args[0]);
    int a1 = static_cast<int>(args[1]);
    int a2 = static_cast<int>(args[2]);
    ::Color a3 = unpack_color(vm, args[3]);
    ::DrawTexture(a0, a1, a2, a3);
    return 0;
}

static uint64_t wrap_draw_texture_v(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Texture2D a0 = unpack_texture2d(vm, args[0]);
    ::Vector2 a1 = unpack_vector2(vm, args[1]);
    ::Color a2 = unpack_color(vm, args[2]);
    ::DrawTextureV(a0, a1, a2);
    return 0;
}

static uint64_t wrap_draw_texture_ex(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Texture2D a0 = unpack_texture2d(vm, args[0]);
    ::Vector2 a1 = unpack_vector2(vm, args[1]);
    float a2 = unpack_f32_arg(args[2]);
    float a3 = unpack_f32_arg(args[3]);
    ::Color a4 = unpack_color(vm, args[4]);
    ::DrawTextureEx(a0, a1, a2, a3, a4);
    return 0;
}

static uint64_t wrap_draw_texture_rec(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Texture2D a0 = unpack_texture2d(vm, args[0]);
    ::Rectangle a1 = unpack_rectangle(vm, args[1]);
    ::Vector2 a2 = unpack_vector2(vm, args[2]);
    ::Color a3 = unpack_color(vm, args[3]);
    ::DrawTextureRec(a0, a1, a2, a3);
    return 0;
}

static uint64_t wrap_draw_texture_pro(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Texture2D a0 = unpack_texture2d(vm, args[0]);
    ::Rectangle a1 = unpack_rectangle(vm, args[1]);
    ::Rectangle a2 = unpack_rectangle(vm, args[2]);
    ::Vector2 a3 = unpack_vector2(vm, args[3]);
    float a4 = unpack_f32_arg(args[4]);
    ::Color a5 = unpack_color(vm, args[5]);
    ::DrawTexturePro(a0, a1, a2, a3, a4, a5);
    return 0;
}

static uint64_t wrap_draw_texture_n_patch(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Texture2D a0 = unpack_texture2d(vm, args[0]);
    ::NPatchInfo a1 = unpack_npatchinfo(vm, args[1]);
    ::Rectangle a2 = unpack_rectangle(vm, args[2]);
    ::Vector2 a3 = unpack_vector2(vm, args[3]);
    float a4 = unpack_f32_arg(args[4]);
    ::Color a5 = unpack_color(vm, args[5]);
    ::DrawTextureNPatch(a0, a1, a2, a3, a4, a5);
    return 0;
}

static uint64_t wrap_color_is_equal(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Color a0 = unpack_color(vm, args[0]);
    ::Color a1 = unpack_color(vm, args[1]);
    bool res = ::ColorIsEqual(a0, a1);
    return res ? 1 : 0;
}

static uint64_t wrap_fade(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Color a0 = unpack_color(vm, args[0]);
    float a1 = unpack_f32_arg(args[1]);
    auto res = ::Fade(a0, a1);
    pack_color(vm, args[2], res);
    return 0;
}

static uint64_t wrap_color_to_int(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Color a0 = unpack_color(vm, args[0]);
    auto res = ::ColorToInt(a0);
    return static_cast<uint64_t>(res);
}

static uint64_t wrap_color_normalize(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Color a0 = unpack_color(vm, args[0]);
    auto res = ::ColorNormalize(a0);
    pack_vector4(vm, args[1], res);
    return 0;
}

static uint64_t wrap_color_from_normalized(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector4 a0 = unpack_vector4(vm, args[0]);
    auto res = ::ColorFromNormalized(a0);
    pack_color(vm, args[1], res);
    return 0;
}

static uint64_t wrap_color_to_hsv(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Color a0 = unpack_color(vm, args[0]);
    auto res = ::ColorToHSV(a0);
    pack_vector3(vm, args[1], res);
    return 0;
}

static uint64_t wrap_color_from_hsv(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    float a0 = unpack_f32_arg(args[0]);
    float a1 = unpack_f32_arg(args[1]);
    float a2 = unpack_f32_arg(args[2]);
    auto res = ::ColorFromHSV(a0, a1, a2);
    pack_color(vm, args[3], res);
    return 0;
}

static uint64_t wrap_color_tint(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Color a0 = unpack_color(vm, args[0]);
    ::Color a1 = unpack_color(vm, args[1]);
    auto res = ::ColorTint(a0, a1);
    pack_color(vm, args[2], res);
    return 0;
}

static uint64_t wrap_color_brightness(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Color a0 = unpack_color(vm, args[0]);
    float a1 = unpack_f32_arg(args[1]);
    auto res = ::ColorBrightness(a0, a1);
    pack_color(vm, args[2], res);
    return 0;
}

static uint64_t wrap_color_contrast(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Color a0 = unpack_color(vm, args[0]);
    float a1 = unpack_f32_arg(args[1]);
    auto res = ::ColorContrast(a0, a1);
    pack_color(vm, args[2], res);
    return 0;
}

static uint64_t wrap_color_alpha(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Color a0 = unpack_color(vm, args[0]);
    float a1 = unpack_f32_arg(args[1]);
    auto res = ::ColorAlpha(a0, a1);
    pack_color(vm, args[2], res);
    return 0;
}

static uint64_t wrap_color_alpha_blend(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Color a0 = unpack_color(vm, args[0]);
    ::Color a1 = unpack_color(vm, args[1]);
    ::Color a2 = unpack_color(vm, args[2]);
    auto res = ::ColorAlphaBlend(a0, a1, a2);
    pack_color(vm, args[3], res);
    return 0;
}

static uint64_t wrap_color_lerp(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Color a0 = unpack_color(vm, args[0]);
    ::Color a1 = unpack_color(vm, args[1]);
    float a2 = unpack_f32_arg(args[2]);
    auto res = ::ColorLerp(a0, a1, a2);
    pack_color(vm, args[3], res);
    return 0;
}

static uint64_t wrap_get_color(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    unsigned int a0 = static_cast<unsigned int>(args[0]);
    auto res = ::GetColor(a0);
    pack_color(vm, args[1], res);
    return 0;
}

static uint64_t wrap_get_pixel_color(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    void *a0 = reinterpret_cast<void *>(args[0]);
    int a1 = static_cast<int>(args[1]);
    auto res = ::GetPixelColor(a0, a1);
    pack_color(vm, args[2], res);
    return 0;
}

static uint64_t wrap_set_pixel_color(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    void *a0 = reinterpret_cast<void *>(args[0]);
    ::Color a1 = unpack_color(vm, args[1]);
    int a2 = static_cast<int>(args[2]);
    ::SetPixelColor(a0, a1, a2);
    return 0;
}

static uint64_t wrap_get_pixel_data_size(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    int a1 = static_cast<int>(args[1]);
    int a2 = static_cast<int>(args[2]);
    auto res = ::GetPixelDataSize(a0, a1, a2);
    return static_cast<uint64_t>(res);
}

static uint64_t wrap_get_font_default(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    auto res = ::GetFontDefault();
    pack_font(vm, args[0], res);
    return 0;
}

static uint64_t wrap_load_font(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    auto res = ::LoadFont(a0);
    pack_font(vm, args[1], res);
    return 0;
}

static uint64_t wrap_load_font_ex(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    int a1 = static_cast<int>(args[1]);
    auto v_a2 = unpack_int32_array(vm, args[2], 0);
    int a3 = static_cast<int>(args[3]);
    auto res = ::LoadFontEx(a0, a1, v_a2.data(), a3);
    pack_font(vm, args[4], res);
    return 0;
}

static uint64_t wrap_load_font_from_image(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    ::Color a1 = unpack_color(vm, args[1]);
    int a2 = static_cast<int>(args[2]);
    auto res = ::LoadFontFromImage(a0, a1, a2);
    pack_font(vm, args[3], res);
    return 0;
}

static uint64_t wrap_load_font_from_memory(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    auto v_a1 = unpack_uint8_array(vm, args[1], 0);
    int a2 = static_cast<int>(args[2]);
    int a3 = static_cast<int>(args[3]);
    auto v_a4 = unpack_int32_array(vm, args[4], 0);
    int a5 = static_cast<int>(args[5]);
    auto res = ::LoadFontFromMemory(a0, v_a1.data(), a2, a3, v_a4.data(), a5);
    pack_font(vm, args[6], res);
    return 0;
}

static uint64_t wrap_is_font_valid(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Font a0 = unpack_font(vm, args[0]);
    bool res = ::IsFontValid(a0);
    return res ? 1 : 0;
}

static uint64_t wrap_load_font_data(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    auto v_a0 = unpack_uint8_array(vm, args[0], 0);
    int a1 = static_cast<int>(args[1]);
    int a2 = static_cast<int>(args[2]);
    auto v_a3 = unpack_int32_array(vm, args[3], 0);
    int a4 = static_cast<int>(args[4]);
    int a5 = static_cast<int>(args[5]);
    auto v_a6 = unpack_int32_array(vm, args[6], 0);
    auto res = ::LoadFontData(v_a0.data(), a1, a2, v_a3.data(), a4, a5, v_a6.data());
    return reinterpret_cast<uint64_t>(res);
}

static uint64_t wrap_unload_font_data(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::GlyphInfo *a0 = reinterpret_cast<::GlyphInfo *>(args[0]);
    int a1 = static_cast<int>(args[1]);
    ::UnloadFontData(a0, a1);
    return 0;
}

static uint64_t wrap_unload_font(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Font a0 = unpack_font(vm, args[0]);
    ::UnloadFont(a0);
    return 0;
}

static uint64_t wrap_export_font_as_code(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Font a0 = unpack_font(vm, args[0]);
    std::string s_a1 = read_solix_chars(vm, args[1]);
    const char *a1 = s_a1.c_str();
    bool res = ::ExportFontAsCode(a0, a1);
    return res ? 1 : 0;
}

static uint64_t wrap_draw_fps(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    int a1 = static_cast<int>(args[1]);
    ::DrawFPS(a0, a1);
    return 0;
}

static uint64_t wrap_draw_text(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    int a1 = static_cast<int>(args[1]);
    int a2 = static_cast<int>(args[2]);
    int a3 = static_cast<int>(args[3]);
    ::Color a4 = unpack_color(vm, args[4]);
    ::DrawText(a0, a1, a2, a3, a4);
    return 0;
}

static uint64_t wrap_draw_text_ex(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Font a0 = unpack_font(vm, args[0]);
    std::string s_a1 = read_solix_chars(vm, args[1]);
    const char *a1 = s_a1.c_str();
    ::Vector2 a2 = unpack_vector2(vm, args[2]);
    float a3 = unpack_f32_arg(args[3]);
    float a4 = unpack_f32_arg(args[4]);
    ::Color a5 = unpack_color(vm, args[5]);
    ::DrawTextEx(a0, a1, a2, a3, a4, a5);
    return 0;
}

static uint64_t wrap_draw_text_pro(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Font a0 = unpack_font(vm, args[0]);
    std::string s_a1 = read_solix_chars(vm, args[1]);
    const char *a1 = s_a1.c_str();
    ::Vector2 a2 = unpack_vector2(vm, args[2]);
    ::Vector2 a3 = unpack_vector2(vm, args[3]);
    float a4 = unpack_f32_arg(args[4]);
    float a5 = unpack_f32_arg(args[5]);
    float a6 = unpack_f32_arg(args[6]);
    ::Color a7 = unpack_color(vm, args[7]);
    ::DrawTextPro(a0, a1, a2, a3, a4, a5, a6, a7);
    return 0;
}

static uint64_t wrap_draw_text_codepoint(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Font a0 = unpack_font(vm, args[0]);
    int a1 = static_cast<int>(args[1]);
    ::Vector2 a2 = unpack_vector2(vm, args[2]);
    float a3 = unpack_f32_arg(args[3]);
    ::Color a4 = unpack_color(vm, args[4]);
    ::DrawTextCodepoint(a0, a1, a2, a3, a4);
    return 0;
}

static uint64_t wrap_draw_text_codepoints(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Font a0 = unpack_font(vm, args[0]);
    auto v_a1 = unpack_int32_array(vm, args[1], 0);
    int a2 = static_cast<int>(args[2]);
    ::Vector2 a3 = unpack_vector2(vm, args[3]);
    float a4 = unpack_f32_arg(args[4]);
    float a5 = unpack_f32_arg(args[5]);
    ::Color a6 = unpack_color(vm, args[6]);
    ::DrawTextCodepoints(a0, v_a1.data(), a2, a3, a4, a5, a6);
    return 0;
}

static uint64_t wrap_set_text_line_spacing(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    ::SetTextLineSpacing(a0);
    return 0;
}

static uint64_t wrap_measure_text(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    int a1 = static_cast<int>(args[1]);
    auto res = ::MeasureText(a0, a1);
    return static_cast<uint64_t>(res);
}

static uint64_t wrap_measure_text_ex(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Font a0 = unpack_font(vm, args[0]);
    std::string s_a1 = read_solix_chars(vm, args[1]);
    const char *a1 = s_a1.c_str();
    float a2 = unpack_f32_arg(args[2]);
    float a3 = unpack_f32_arg(args[3]);
    auto res = ::MeasureTextEx(a0, a1, a2, a3);
    pack_vector2(vm, args[4], res);
    return 0;
}

static uint64_t wrap_measure_text_codepoints(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Font a0 = unpack_font(vm, args[0]);
    auto v_a1 = unpack_int32_array(vm, args[1], 0);
    int a2 = static_cast<int>(args[2]);
    float a3 = unpack_f32_arg(args[3]);
    float a4 = unpack_f32_arg(args[4]);
    auto res = ::MeasureTextCodepoints(a0, v_a1.data(), a2, a3, a4);
    pack_vector2(vm, args[5], res);
    return 0;
}

static uint64_t wrap_get_glyph_index(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Font a0 = unpack_font(vm, args[0]);
    int a1 = static_cast<int>(args[1]);
    auto res = ::GetGlyphIndex(a0, a1);
    return static_cast<uint64_t>(res);
}

static uint64_t wrap_get_glyph_info(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Font a0 = unpack_font(vm, args[0]);
    int a1 = static_cast<int>(args[1]);
    auto res = ::GetGlyphInfo(a0, a1);
    pack_glyphinfo(vm, args[2], res);
    return 0;
}

static uint64_t wrap_get_glyph_atlas_rec(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Font a0 = unpack_font(vm, args[0]);
    int a1 = static_cast<int>(args[1]);
    auto res = ::GetGlyphAtlasRec(a0, a1);
    pack_rectangle(vm, args[2], res);
    return 0;
}

static uint64_t wrap_load_utf8(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    auto v_a0 = unpack_int32_array(vm, args[0], 0);
    int a1 = static_cast<int>(args[1]);
    const char *res = ::LoadUTF8(v_a0.data(), a1);
    return allocate_solix_chars(vm, res);
}

static uint64_t wrap_unload_utf8(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    char *a0 = const_cast<char *>(s_a0.c_str());
    ::UnloadUTF8(a0);
    return 0;
}

static uint64_t wrap_load_codepoints(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    auto v_a1 = unpack_int32_array(vm, args[1], 0);
    auto res = ::LoadCodepoints(a0, v_a1.data());
    return reinterpret_cast<uint64_t>(res);
}

static uint64_t wrap_unload_codepoints(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    auto v_a0 = unpack_int32_array(vm, args[0], 0);
    ::UnloadCodepoints(v_a0.data());
    return 0;
}

static uint64_t wrap_get_codepoint_count(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    auto res = ::GetCodepointCount(a0);
    return static_cast<uint64_t>(res);
}

static uint64_t wrap_get_codepoint(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    auto v_a1 = unpack_int32_array(vm, args[1], 0);
    auto res = ::GetCodepoint(a0, v_a1.data());
    return static_cast<uint64_t>(res);
}

static uint64_t wrap_get_codepoint_next(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    auto v_a1 = unpack_int32_array(vm, args[1], 0);
    auto res = ::GetCodepointNext(a0, v_a1.data());
    return static_cast<uint64_t>(res);
}

static uint64_t wrap_get_codepoint_previous(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    auto v_a1 = unpack_int32_array(vm, args[1], 0);
    auto res = ::GetCodepointPrevious(a0, v_a1.data());
    return static_cast<uint64_t>(res);
}

static uint64_t wrap_codepoint_to_utf8(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    auto v_a1 = unpack_int32_array(vm, args[1], 0);
    const char *res = ::CodepointToUTF8(a0, v_a1.data());
    return allocate_solix_chars(vm, res);
}

static uint64_t wrap_load_text_lines(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    auto v_a1 = unpack_int32_array(vm, args[1], 0);
    auto res = ::LoadTextLines(a0, v_a1.data());
    return reinterpret_cast<uint64_t>(res);
}

static uint64_t wrap_unload_text_lines(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    char **a0 = nullptr; // string array slice
    int a1 = static_cast<int>(args[1]);
    ::UnloadTextLines(a0, a1);
    return 0;
}

static uint64_t wrap_text_copy(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    char *a0 = const_cast<char *>(s_a0.c_str());
    std::string s_a1 = read_solix_chars(vm, args[1]);
    const char *a1 = s_a1.c_str();
    auto res = ::TextCopy(a0, a1);
    return static_cast<uint64_t>(res);
}

static uint64_t wrap_text_is_equal(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    std::string s_a1 = read_solix_chars(vm, args[1]);
    const char *a1 = s_a1.c_str();
    bool res = ::TextIsEqual(a0, a1);
    return res ? 1 : 0;
}

static uint64_t wrap_text_length(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    auto res = ::TextLength(a0);
    return static_cast<uint64_t>(res);
}

static uint64_t wrap_text_subtext(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    int a1 = static_cast<int>(args[1]);
    int a2 = static_cast<int>(args[2]);
    const char *res = ::TextSubtext(a0, a1, a2);
    return allocate_solix_chars(vm, res);
}

static uint64_t wrap_text_remove_spaces(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    const char *res = ::TextRemoveSpaces(a0);
    return allocate_solix_chars(vm, res);
}

static uint64_t wrap_get_text_between(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    std::string s_a1 = read_solix_chars(vm, args[1]);
    const char *a1 = s_a1.c_str();
    std::string s_a2 = read_solix_chars(vm, args[2]);
    const char *a2 = s_a2.c_str();
    const char *res = ::GetTextBetween(a0, a1, a2);
    return allocate_solix_chars(vm, res);
}

static uint64_t wrap_text_replace(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    std::string s_a1 = read_solix_chars(vm, args[1]);
    const char *a1 = s_a1.c_str();
    std::string s_a2 = read_solix_chars(vm, args[2]);
    const char *a2 = s_a2.c_str();
    const char *res = ::TextReplace(a0, a1, a2);
    return allocate_solix_chars(vm, res);
}

static uint64_t wrap_text_replace_alloc(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    std::string s_a1 = read_solix_chars(vm, args[1]);
    const char *a1 = s_a1.c_str();
    std::string s_a2 = read_solix_chars(vm, args[2]);
    const char *a2 = s_a2.c_str();
    const char *res = ::TextReplaceAlloc(a0, a1, a2);
    return allocate_solix_chars(vm, res);
}

static uint64_t wrap_text_replace_between(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    std::string s_a1 = read_solix_chars(vm, args[1]);
    const char *a1 = s_a1.c_str();
    std::string s_a2 = read_solix_chars(vm, args[2]);
    const char *a2 = s_a2.c_str();
    std::string s_a3 = read_solix_chars(vm, args[3]);
    const char *a3 = s_a3.c_str();
    const char *res = ::TextReplaceBetween(a0, a1, a2, a3);
    return allocate_solix_chars(vm, res);
}

static uint64_t wrap_text_replace_between_alloc(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    std::string s_a1 = read_solix_chars(vm, args[1]);
    const char *a1 = s_a1.c_str();
    std::string s_a2 = read_solix_chars(vm, args[2]);
    const char *a2 = s_a2.c_str();
    std::string s_a3 = read_solix_chars(vm, args[3]);
    const char *a3 = s_a3.c_str();
    const char *res = ::TextReplaceBetweenAlloc(a0, a1, a2, a3);
    return allocate_solix_chars(vm, res);
}

static uint64_t wrap_text_insert(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    std::string s_a1 = read_solix_chars(vm, args[1]);
    const char *a1 = s_a1.c_str();
    int a2 = static_cast<int>(args[2]);
    const char *res = ::TextInsert(a0, a1, a2);
    return allocate_solix_chars(vm, res);
}

static uint64_t wrap_text_insert_alloc(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    std::string s_a1 = read_solix_chars(vm, args[1]);
    const char *a1 = s_a1.c_str();
    int a2 = static_cast<int>(args[2]);
    const char *res = ::TextInsertAlloc(a0, a1, a2);
    return allocate_solix_chars(vm, res);
}

static uint64_t wrap_text_join(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    char **a0 = nullptr; // string array slice
    int a1 = static_cast<int>(args[1]);
    std::string s_a2 = read_solix_chars(vm, args[2]);
    const char *a2 = s_a2.c_str();
    const char *res = ::TextJoin(a0, a1, a2);
    return allocate_solix_chars(vm, res);
}

static uint64_t wrap_text_split(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    uint64_t a1 = args[1];
    auto v_a2 = unpack_int32_array(vm, args[2], 0);
    auto res = ::TextSplit(a0, a1, v_a2.data());
    return reinterpret_cast<uint64_t>(res);
}

static uint64_t wrap_text_append(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    char *a0 = const_cast<char *>(s_a0.c_str());
    std::string s_a1 = read_solix_chars(vm, args[1]);
    const char *a1 = s_a1.c_str();
    auto v_a2 = unpack_int32_array(vm, args[2], 0);
    ::TextAppend(a0, a1, v_a2.data());
    return 0;
}

static uint64_t wrap_text_find_index(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    std::string s_a1 = read_solix_chars(vm, args[1]);
    const char *a1 = s_a1.c_str();
    auto res = ::TextFindIndex(a0, a1);
    return static_cast<uint64_t>(res);
}

static uint64_t wrap_text_to_upper(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    const char *res = ::TextToUpper(a0);
    return allocate_solix_chars(vm, res);
}

static uint64_t wrap_text_to_lower(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    const char *res = ::TextToLower(a0);
    return allocate_solix_chars(vm, res);
}

static uint64_t wrap_text_to_pascal(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    const char *res = ::TextToPascal(a0);
    return allocate_solix_chars(vm, res);
}

static uint64_t wrap_text_to_snake(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    const char *res = ::TextToSnake(a0);
    return allocate_solix_chars(vm, res);
}

static uint64_t wrap_text_to_camel(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    const char *res = ::TextToCamel(a0);
    return allocate_solix_chars(vm, res);
}

static uint64_t wrap_text_to_integer(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    auto res = ::TextToInteger(a0);
    return static_cast<uint64_t>(res);
}

static uint64_t wrap_text_to_float(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    float res = ::TextToFloat(a0);
    return pack_f32_ret(res);
}

static uint64_t wrap_draw_line3d(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector3 a0 = unpack_vector3(vm, args[0]);
    ::Vector3 a1 = unpack_vector3(vm, args[1]);
    ::Color a2 = unpack_color(vm, args[2]);
    ::DrawLine3D(a0, a1, a2);
    return 0;
}

static uint64_t wrap_draw_point3d(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector3 a0 = unpack_vector3(vm, args[0]);
    ::Color a1 = unpack_color(vm, args[1]);
    ::DrawPoint3D(a0, a1);
    return 0;
}

static uint64_t wrap_draw_circle3d(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector3 a0 = unpack_vector3(vm, args[0]);
    float a1 = unpack_f32_arg(args[1]);
    ::Vector3 a2 = unpack_vector3(vm, args[2]);
    float a3 = unpack_f32_arg(args[3]);
    ::Color a4 = unpack_color(vm, args[4]);
    ::DrawCircle3D(a0, a1, a2, a3, a4);
    return 0;
}

static uint64_t wrap_draw_triangle3d(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector3 a0 = unpack_vector3(vm, args[0]);
    ::Vector3 a1 = unpack_vector3(vm, args[1]);
    ::Vector3 a2 = unpack_vector3(vm, args[2]);
    ::Color a3 = unpack_color(vm, args[3]);
    ::DrawTriangle3D(a0, a1, a2, a3);
    return 0;
}

static uint64_t wrap_draw_triangle_strip3d(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    auto v_a0 = unpack_vector3_array(vm, args[0], 0);
    int a1 = static_cast<int>(args[1]);
    ::Color a2 = unpack_color(vm, args[2]);
    ::DrawTriangleStrip3D(v_a0.data(), a1, a2);
    return 0;
}

static uint64_t wrap_draw_cube(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector3 a0 = unpack_vector3(vm, args[0]);
    float a1 = unpack_f32_arg(args[1]);
    float a2 = unpack_f32_arg(args[2]);
    float a3 = unpack_f32_arg(args[3]);
    ::Color a4 = unpack_color(vm, args[4]);
    ::DrawCube(a0, a1, a2, a3, a4);
    return 0;
}

static uint64_t wrap_draw_cube_v(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector3 a0 = unpack_vector3(vm, args[0]);
    ::Vector3 a1 = unpack_vector3(vm, args[1]);
    ::Color a2 = unpack_color(vm, args[2]);
    ::DrawCubeV(a0, a1, a2);
    return 0;
}

static uint64_t wrap_draw_cube_wires(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector3 a0 = unpack_vector3(vm, args[0]);
    float a1 = unpack_f32_arg(args[1]);
    float a2 = unpack_f32_arg(args[2]);
    float a3 = unpack_f32_arg(args[3]);
    ::Color a4 = unpack_color(vm, args[4]);
    ::DrawCubeWires(a0, a1, a2, a3, a4);
    return 0;
}

static uint64_t wrap_draw_cube_wires_v(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector3 a0 = unpack_vector3(vm, args[0]);
    ::Vector3 a1 = unpack_vector3(vm, args[1]);
    ::Color a2 = unpack_color(vm, args[2]);
    ::DrawCubeWiresV(a0, a1, a2);
    return 0;
}

static uint64_t wrap_draw_sphere(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector3 a0 = unpack_vector3(vm, args[0]);
    float a1 = unpack_f32_arg(args[1]);
    ::Color a2 = unpack_color(vm, args[2]);
    ::DrawSphere(a0, a1, a2);
    return 0;
}

static uint64_t wrap_draw_sphere_ex(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector3 a0 = unpack_vector3(vm, args[0]);
    float a1 = unpack_f32_arg(args[1]);
    int a2 = static_cast<int>(args[2]);
    int a3 = static_cast<int>(args[3]);
    ::Color a4 = unpack_color(vm, args[4]);
    ::DrawSphereEx(a0, a1, a2, a3, a4);
    return 0;
}

static uint64_t wrap_draw_sphere_wires(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector3 a0 = unpack_vector3(vm, args[0]);
    float a1 = unpack_f32_arg(args[1]);
    int a2 = static_cast<int>(args[2]);
    int a3 = static_cast<int>(args[3]);
    ::Color a4 = unpack_color(vm, args[4]);
    ::DrawSphereWires(a0, a1, a2, a3, a4);
    return 0;
}

static uint64_t wrap_draw_cylinder(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector3 a0 = unpack_vector3(vm, args[0]);
    float a1 = unpack_f32_arg(args[1]);
    float a2 = unpack_f32_arg(args[2]);
    float a3 = unpack_f32_arg(args[3]);
    int a4 = static_cast<int>(args[4]);
    ::Color a5 = unpack_color(vm, args[5]);
    ::DrawCylinder(a0, a1, a2, a3, a4, a5);
    return 0;
}

static uint64_t wrap_draw_cylinder_ex(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector3 a0 = unpack_vector3(vm, args[0]);
    ::Vector3 a1 = unpack_vector3(vm, args[1]);
    float a2 = unpack_f32_arg(args[2]);
    float a3 = unpack_f32_arg(args[3]);
    int a4 = static_cast<int>(args[4]);
    ::Color a5 = unpack_color(vm, args[5]);
    ::DrawCylinderEx(a0, a1, a2, a3, a4, a5);
    return 0;
}

static uint64_t wrap_draw_cylinder_wires(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector3 a0 = unpack_vector3(vm, args[0]);
    float a1 = unpack_f32_arg(args[1]);
    float a2 = unpack_f32_arg(args[2]);
    float a3 = unpack_f32_arg(args[3]);
    int a4 = static_cast<int>(args[4]);
    ::Color a5 = unpack_color(vm, args[5]);
    ::DrawCylinderWires(a0, a1, a2, a3, a4, a5);
    return 0;
}

static uint64_t wrap_draw_cylinder_wires_ex(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector3 a0 = unpack_vector3(vm, args[0]);
    ::Vector3 a1 = unpack_vector3(vm, args[1]);
    float a2 = unpack_f32_arg(args[2]);
    float a3 = unpack_f32_arg(args[3]);
    int a4 = static_cast<int>(args[4]);
    ::Color a5 = unpack_color(vm, args[5]);
    ::DrawCylinderWiresEx(a0, a1, a2, a3, a4, a5);
    return 0;
}

static uint64_t wrap_draw_capsule(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector3 a0 = unpack_vector3(vm, args[0]);
    ::Vector3 a1 = unpack_vector3(vm, args[1]);
    float a2 = unpack_f32_arg(args[2]);
    int a3 = static_cast<int>(args[3]);
    int a4 = static_cast<int>(args[4]);
    ::Color a5 = unpack_color(vm, args[5]);
    ::DrawCapsule(a0, a1, a2, a3, a4, a5);
    return 0;
}

static uint64_t wrap_draw_capsule_wires(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector3 a0 = unpack_vector3(vm, args[0]);
    ::Vector3 a1 = unpack_vector3(vm, args[1]);
    float a2 = unpack_f32_arg(args[2]);
    int a3 = static_cast<int>(args[3]);
    int a4 = static_cast<int>(args[4]);
    ::Color a5 = unpack_color(vm, args[5]);
    ::DrawCapsuleWires(a0, a1, a2, a3, a4, a5);
    return 0;
}

static uint64_t wrap_draw_plane(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector3 a0 = unpack_vector3(vm, args[0]);
    ::Vector2 a1 = unpack_vector2(vm, args[1]);
    ::Color a2 = unpack_color(vm, args[2]);
    ::DrawPlane(a0, a1, a2);
    return 0;
}

static uint64_t wrap_draw_ray(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Ray a0 = unpack_ray(vm, args[0]);
    ::Color a1 = unpack_color(vm, args[1]);
    ::DrawRay(a0, a1);
    return 0;
}

static uint64_t wrap_draw_grid(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    float a1 = unpack_f32_arg(args[1]);
    ::DrawGrid(a0, a1);
    return 0;
}

static uint64_t wrap_load_model(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    auto res = ::LoadModel(a0);
    pack_model(vm, args[1], res);
    return 0;
}

static uint64_t wrap_load_model_from_mesh(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Mesh a0 = unpack_mesh(vm, args[0]);
    auto res = ::LoadModelFromMesh(a0);
    pack_model(vm, args[1], res);
    return 0;
}

static uint64_t wrap_is_model_valid(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Model a0 = unpack_model(vm, args[0]);
    bool res = ::IsModelValid(a0);
    return res ? 1 : 0;
}

static uint64_t wrap_unload_model(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Model a0 = unpack_model(vm, args[0]);
    ::UnloadModel(a0);
    return 0;
}

static uint64_t wrap_get_model_bounding_box(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Model a0 = unpack_model(vm, args[0]);
    auto res = ::GetModelBoundingBox(a0);
    pack_boundingbox(vm, args[1], res);
    return 0;
}

static uint64_t wrap_draw_model(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Model a0 = unpack_model(vm, args[0]);
    ::Vector3 a1 = unpack_vector3(vm, args[1]);
    float a2 = unpack_f32_arg(args[2]);
    ::Color a3 = unpack_color(vm, args[3]);
    ::DrawModel(a0, a1, a2, a3);
    return 0;
}

static uint64_t wrap_draw_model_ex(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Model a0 = unpack_model(vm, args[0]);
    ::Vector3 a1 = unpack_vector3(vm, args[1]);
    ::Vector3 a2 = unpack_vector3(vm, args[2]);
    float a3 = unpack_f32_arg(args[3]);
    ::Vector3 a4 = unpack_vector3(vm, args[4]);
    ::Color a5 = unpack_color(vm, args[5]);
    ::DrawModelEx(a0, a1, a2, a3, a4, a5);
    return 0;
}

static uint64_t wrap_draw_model_wires(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Model a0 = unpack_model(vm, args[0]);
    ::Vector3 a1 = unpack_vector3(vm, args[1]);
    float a2 = unpack_f32_arg(args[2]);
    ::Color a3 = unpack_color(vm, args[3]);
    ::DrawModelWires(a0, a1, a2, a3);
    return 0;
}

static uint64_t wrap_draw_model_wires_ex(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Model a0 = unpack_model(vm, args[0]);
    ::Vector3 a1 = unpack_vector3(vm, args[1]);
    ::Vector3 a2 = unpack_vector3(vm, args[2]);
    float a3 = unpack_f32_arg(args[3]);
    ::Vector3 a4 = unpack_vector3(vm, args[4]);
    ::Color a5 = unpack_color(vm, args[5]);
    ::DrawModelWiresEx(a0, a1, a2, a3, a4, a5);
    return 0;
}

static uint64_t wrap_draw_bounding_box(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::BoundingBox a0 = unpack_boundingbox(vm, args[0]);
    ::Color a1 = unpack_color(vm, args[1]);
    ::DrawBoundingBox(a0, a1);
    return 0;
}

static uint64_t wrap_draw_billboard(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Camera a0 = unpack_camera(vm, args[0]);
    ::Texture2D a1 = unpack_texture2d(vm, args[1]);
    ::Vector3 a2 = unpack_vector3(vm, args[2]);
    float a3 = unpack_f32_arg(args[3]);
    ::Color a4 = unpack_color(vm, args[4]);
    ::DrawBillboard(a0, a1, a2, a3, a4);
    return 0;
}

static uint64_t wrap_draw_billboard_rec(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Camera a0 = unpack_camera(vm, args[0]);
    ::Texture2D a1 = unpack_texture2d(vm, args[1]);
    ::Rectangle a2 = unpack_rectangle(vm, args[2]);
    ::Vector3 a3 = unpack_vector3(vm, args[3]);
    ::Vector2 a4 = unpack_vector2(vm, args[4]);
    ::Color a5 = unpack_color(vm, args[5]);
    ::DrawBillboardRec(a0, a1, a2, a3, a4, a5);
    return 0;
}

static uint64_t wrap_draw_billboard_pro(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Camera a0 = unpack_camera(vm, args[0]);
    ::Texture2D a1 = unpack_texture2d(vm, args[1]);
    ::Rectangle a2 = unpack_rectangle(vm, args[2]);
    ::Vector3 a3 = unpack_vector3(vm, args[3]);
    ::Vector3 a4 = unpack_vector3(vm, args[4]);
    ::Vector2 a5 = unpack_vector2(vm, args[5]);
    ::Vector2 a6 = unpack_vector2(vm, args[6]);
    float a7 = unpack_f32_arg(args[7]);
    ::Color a8 = unpack_color(vm, args[8]);
    ::DrawBillboardPro(a0, a1, a2, a3, a4, a5, a6, a7, a8);
    return 0;
}

static uint64_t wrap_upload_mesh(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Mesh a0 = unpack_mesh(vm, args[0]);
    bool a1 = (args[1] != 0);
    ::UploadMesh(&a0, a1);
    pack_mesh(vm, args[0], a0);
    return 0;
}

static uint64_t wrap_update_mesh_buffer(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Mesh a0 = unpack_mesh(vm, args[0]);
    int a1 = static_cast<int>(args[1]);
    void *a2 = reinterpret_cast<void *>(args[2]);
    int a3 = static_cast<int>(args[3]);
    int a4 = static_cast<int>(args[4]);
    ::UpdateMeshBuffer(a0, a1, a2, a3, a4);
    return 0;
}

static uint64_t wrap_unload_mesh(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Mesh a0 = unpack_mesh(vm, args[0]);
    ::UnloadMesh(a0);
    return 0;
}

static uint64_t wrap_draw_mesh(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Mesh a0 = unpack_mesh(vm, args[0]);
    ::Material a1 = unpack_material(vm, args[1]);
    ::Matrix a2 = unpack_matrix(vm, args[2]);
    ::DrawMesh(a0, a1, a2);
    return 0;
}

static uint64_t wrap_draw_mesh_instanced(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Mesh a0 = unpack_mesh(vm, args[0]);
    ::Material a1 = unpack_material(vm, args[1]);
    auto v_a2 = unpack_matrix_array(vm, args[2], 0);
    int a3 = static_cast<int>(args[3]);
    ::DrawMeshInstanced(a0, a1, v_a2.data(), a3);
    return 0;
}

static uint64_t wrap_get_mesh_bounding_box(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Mesh a0 = unpack_mesh(vm, args[0]);
    auto res = ::GetMeshBoundingBox(a0);
    pack_boundingbox(vm, args[1], res);
    return 0;
}

static uint64_t wrap_gen_mesh_tangents(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Mesh a0 = unpack_mesh(vm, args[0]);
    ::GenMeshTangents(&a0);
    pack_mesh(vm, args[0], a0);
    return 0;
}

static uint64_t wrap_export_mesh(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Mesh a0 = unpack_mesh(vm, args[0]);
    std::string s_a1 = read_solix_chars(vm, args[1]);
    const char *a1 = s_a1.c_str();
    bool res = ::ExportMesh(a0, a1);
    return res ? 1 : 0;
}

static uint64_t wrap_export_mesh_as_code(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Mesh a0 = unpack_mesh(vm, args[0]);
    std::string s_a1 = read_solix_chars(vm, args[1]);
    const char *a1 = s_a1.c_str();
    bool res = ::ExportMeshAsCode(a0, a1);
    return res ? 1 : 0;
}

static uint64_t wrap_gen_mesh_poly(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    float a1 = unpack_f32_arg(args[1]);
    auto res = ::GenMeshPoly(a0, a1);
    pack_mesh(vm, args[2], res);
    return 0;
}

static uint64_t wrap_gen_mesh_plane(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    float a0 = unpack_f32_arg(args[0]);
    float a1 = unpack_f32_arg(args[1]);
    int a2 = static_cast<int>(args[2]);
    int a3 = static_cast<int>(args[3]);
    auto res = ::GenMeshPlane(a0, a1, a2, a3);
    pack_mesh(vm, args[4], res);
    return 0;
}

static uint64_t wrap_gen_mesh_cube(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    float a0 = unpack_f32_arg(args[0]);
    float a1 = unpack_f32_arg(args[1]);
    float a2 = unpack_f32_arg(args[2]);
    auto res = ::GenMeshCube(a0, a1, a2);
    pack_mesh(vm, args[3], res);
    return 0;
}

static uint64_t wrap_gen_mesh_sphere(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    float a0 = unpack_f32_arg(args[0]);
    int a1 = static_cast<int>(args[1]);
    int a2 = static_cast<int>(args[2]);
    auto res = ::GenMeshSphere(a0, a1, a2);
    pack_mesh(vm, args[3], res);
    return 0;
}

static uint64_t wrap_gen_mesh_hemi_sphere(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    float a0 = unpack_f32_arg(args[0]);
    int a1 = static_cast<int>(args[1]);
    int a2 = static_cast<int>(args[2]);
    auto res = ::GenMeshHemiSphere(a0, a1, a2);
    pack_mesh(vm, args[3], res);
    return 0;
}

static uint64_t wrap_gen_mesh_cylinder(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    float a0 = unpack_f32_arg(args[0]);
    float a1 = unpack_f32_arg(args[1]);
    int a2 = static_cast<int>(args[2]);
    auto res = ::GenMeshCylinder(a0, a1, a2);
    pack_mesh(vm, args[3], res);
    return 0;
}

static uint64_t wrap_gen_mesh_cone(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    float a0 = unpack_f32_arg(args[0]);
    float a1 = unpack_f32_arg(args[1]);
    int a2 = static_cast<int>(args[2]);
    auto res = ::GenMeshCone(a0, a1, a2);
    pack_mesh(vm, args[3], res);
    return 0;
}

static uint64_t wrap_gen_mesh_torus(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    float a0 = unpack_f32_arg(args[0]);
    float a1 = unpack_f32_arg(args[1]);
    int a2 = static_cast<int>(args[2]);
    int a3 = static_cast<int>(args[3]);
    auto res = ::GenMeshTorus(a0, a1, a2, a3);
    pack_mesh(vm, args[4], res);
    return 0;
}

static uint64_t wrap_gen_mesh_knot(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    float a0 = unpack_f32_arg(args[0]);
    float a1 = unpack_f32_arg(args[1]);
    int a2 = static_cast<int>(args[2]);
    int a3 = static_cast<int>(args[3]);
    auto res = ::GenMeshKnot(a0, a1, a2, a3);
    pack_mesh(vm, args[4], res);
    return 0;
}

static uint64_t wrap_gen_mesh_heightmap(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    ::Vector3 a1 = unpack_vector3(vm, args[1]);
    auto res = ::GenMeshHeightmap(a0, a1);
    pack_mesh(vm, args[2], res);
    return 0;
}

static uint64_t wrap_gen_mesh_cubicmap(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Image a0 = unpack_image(vm, args[0]);
    ::Vector3 a1 = unpack_vector3(vm, args[1]);
    auto res = ::GenMeshCubicmap(a0, a1);
    pack_mesh(vm, args[2], res);
    return 0;
}

static uint64_t wrap_load_materials(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    auto v_a1 = unpack_int32_array(vm, args[1], 0);
    auto res = ::LoadMaterials(a0, v_a1.data());
    return reinterpret_cast<uint64_t>(res);
}

static uint64_t wrap_load_material_default(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    auto res = ::LoadMaterialDefault();
    pack_material(vm, args[0], res);
    return 0;
}

static uint64_t wrap_is_material_valid(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Material a0 = unpack_material(vm, args[0]);
    bool res = ::IsMaterialValid(a0);
    return res ? 1 : 0;
}

static uint64_t wrap_unload_material(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Material a0 = unpack_material(vm, args[0]);
    ::UnloadMaterial(a0);
    return 0;
}

static uint64_t wrap_set_material_texture(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Material *a0 = reinterpret_cast<::Material *>(args[0]);
    int a1 = static_cast<int>(args[1]);
    ::Texture2D a2 = unpack_texture2d(vm, args[2]);
    ::SetMaterialTexture(a0, a1, a2);
    return 0;
}

static uint64_t wrap_set_model_mesh_material(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Model a0 = unpack_model(vm, args[0]);
    int a1 = static_cast<int>(args[1]);
    int a2 = static_cast<int>(args[2]);
    ::SetModelMeshMaterial(&a0, a1, a2);
    pack_model(vm, args[0], a0);
    return 0;
}

static uint64_t wrap_load_model_animations(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    auto v_a1 = unpack_int32_array(vm, args[1], 0);
    auto res = ::LoadModelAnimations(a0, v_a1.data());
    return reinterpret_cast<uint64_t>(res);
}

static uint64_t wrap_update_model_animation(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Model a0 = unpack_model(vm, args[0]);
    ::ModelAnimation a1 = unpack_modelanimation(vm, args[1]);
    float a2 = unpack_f32_arg(args[2]);
    ::UpdateModelAnimation(a0, a1, a2);
    return 0;
}

static uint64_t wrap_update_model_animation_ex(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Model a0 = unpack_model(vm, args[0]);
    ::ModelAnimation a1 = unpack_modelanimation(vm, args[1]);
    float a2 = unpack_f32_arg(args[2]);
    ::ModelAnimation a3 = unpack_modelanimation(vm, args[3]);
    float a4 = unpack_f32_arg(args[4]);
    float a5 = unpack_f32_arg(args[5]);
    ::UpdateModelAnimationEx(a0, a1, a2, a3, a4, a5);
    return 0;
}

static uint64_t wrap_unload_model_animations(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::ModelAnimation *a0 = reinterpret_cast<::ModelAnimation *>(args[0]);
    int a1 = static_cast<int>(args[1]);
    ::UnloadModelAnimations(a0, a1);
    return 0;
}

static uint64_t wrap_is_model_animation_valid(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Model a0 = unpack_model(vm, args[0]);
    ::ModelAnimation a1 = unpack_modelanimation(vm, args[1]);
    bool res = ::IsModelAnimationValid(a0, a1);
    return res ? 1 : 0;
}

static uint64_t wrap_check_collision_spheres(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector3 a0 = unpack_vector3(vm, args[0]);
    float a1 = unpack_f32_arg(args[1]);
    ::Vector3 a2 = unpack_vector3(vm, args[2]);
    float a3 = unpack_f32_arg(args[3]);
    bool res = ::CheckCollisionSpheres(a0, a1, a2, a3);
    return res ? 1 : 0;
}

static uint64_t wrap_check_collision_boxes(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::BoundingBox a0 = unpack_boundingbox(vm, args[0]);
    ::BoundingBox a1 = unpack_boundingbox(vm, args[1]);
    bool res = ::CheckCollisionBoxes(a0, a1);
    return res ? 1 : 0;
}

static uint64_t wrap_check_collision_box_sphere(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::BoundingBox a0 = unpack_boundingbox(vm, args[0]);
    ::Vector3 a1 = unpack_vector3(vm, args[1]);
    float a2 = unpack_f32_arg(args[2]);
    bool res = ::CheckCollisionBoxSphere(a0, a1, a2);
    return res ? 1 : 0;
}

static uint64_t wrap_get_ray_collision_sphere(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Ray a0 = unpack_ray(vm, args[0]);
    ::Vector3 a1 = unpack_vector3(vm, args[1]);
    float a2 = unpack_f32_arg(args[2]);
    auto res = ::GetRayCollisionSphere(a0, a1, a2);
    pack_raycollision(vm, args[3], res);
    return 0;
}

static uint64_t wrap_get_ray_collision_box(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Ray a0 = unpack_ray(vm, args[0]);
    ::BoundingBox a1 = unpack_boundingbox(vm, args[1]);
    auto res = ::GetRayCollisionBox(a0, a1);
    pack_raycollision(vm, args[2], res);
    return 0;
}

static uint64_t wrap_get_ray_collision_mesh(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Ray a0 = unpack_ray(vm, args[0]);
    ::Mesh a1 = unpack_mesh(vm, args[1]);
    ::Matrix a2 = unpack_matrix(vm, args[2]);
    auto res = ::GetRayCollisionMesh(a0, a1, a2);
    pack_raycollision(vm, args[3], res);
    return 0;
}

static uint64_t wrap_get_ray_collision_triangle(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Ray a0 = unpack_ray(vm, args[0]);
    ::Vector3 a1 = unpack_vector3(vm, args[1]);
    ::Vector3 a2 = unpack_vector3(vm, args[2]);
    ::Vector3 a3 = unpack_vector3(vm, args[3]);
    auto res = ::GetRayCollisionTriangle(a0, a1, a2, a3);
    pack_raycollision(vm, args[4], res);
    return 0;
}

static uint64_t wrap_get_ray_collision_quad(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Ray a0 = unpack_ray(vm, args[0]);
    ::Vector3 a1 = unpack_vector3(vm, args[1]);
    ::Vector3 a2 = unpack_vector3(vm, args[2]);
    ::Vector3 a3 = unpack_vector3(vm, args[3]);
    ::Vector3 a4 = unpack_vector3(vm, args[4]);
    auto res = ::GetRayCollisionQuad(a0, a1, a2, a3, a4);
    pack_raycollision(vm, args[5], res);
    return 0;
}

static uint64_t wrap_init_audio_device(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::InitAudioDevice();
    return 0;
}

static uint64_t wrap_close_audio_device(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::CloseAudioDevice();
    return 0;
}

static uint64_t wrap_is_audio_device_ready(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    bool res = ::IsAudioDeviceReady();
    return res ? 1 : 0;
}

static uint64_t wrap_set_master_volume(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    float a0 = unpack_f32_arg(args[0]);
    ::SetMasterVolume(a0);
    return 0;
}

static uint64_t wrap_get_master_volume(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    float res = ::GetMasterVolume();
    return pack_f32_ret(res);
}

static uint64_t wrap_load_wave(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    auto res = ::LoadWave(a0);
    pack_wave(vm, args[1], res);
    return 0;
}

static uint64_t wrap_load_wave_from_memory(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    auto v_a1 = unpack_uint8_array(vm, args[1], 0);
    int a2 = static_cast<int>(args[2]);
    auto res = ::LoadWaveFromMemory(a0, v_a1.data(), a2);
    pack_wave(vm, args[3], res);
    return 0;
}

static uint64_t wrap_is_wave_valid(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Wave a0 = unpack_wave(vm, args[0]);
    bool res = ::IsWaveValid(a0);
    return res ? 1 : 0;
}

static uint64_t wrap_load_sound(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    auto res = ::LoadSound(a0);
    pack_sound(vm, args[1], res);
    return 0;
}

static uint64_t wrap_load_sound_from_wave(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Wave a0 = unpack_wave(vm, args[0]);
    auto res = ::LoadSoundFromWave(a0);
    pack_sound(vm, args[1], res);
    return 0;
}

static uint64_t wrap_load_sound_alias(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Sound a0 = unpack_sound(vm, args[0]);
    auto res = ::LoadSoundAlias(a0);
    pack_sound(vm, args[1], res);
    return 0;
}

static uint64_t wrap_is_sound_valid(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Sound a0 = unpack_sound(vm, args[0]);
    bool res = ::IsSoundValid(a0);
    return res ? 1 : 0;
}

static uint64_t wrap_update_sound(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Sound a0 = unpack_sound(vm, args[0]);
    void *a1 = reinterpret_cast<void *>(args[1]);
    int a2 = static_cast<int>(args[2]);
    ::UpdateSound(a0, a1, a2);
    return 0;
}

static uint64_t wrap_unload_wave(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Wave a0 = unpack_wave(vm, args[0]);
    ::UnloadWave(a0);
    return 0;
}

static uint64_t wrap_unload_sound(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Sound a0 = unpack_sound(vm, args[0]);
    ::UnloadSound(a0);
    return 0;
}

static uint64_t wrap_unload_sound_alias(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Sound a0 = unpack_sound(vm, args[0]);
    ::UnloadSoundAlias(a0);
    return 0;
}

static uint64_t wrap_export_wave(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Wave a0 = unpack_wave(vm, args[0]);
    std::string s_a1 = read_solix_chars(vm, args[1]);
    const char *a1 = s_a1.c_str();
    bool res = ::ExportWave(a0, a1);
    return res ? 1 : 0;
}

static uint64_t wrap_export_wave_as_code(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Wave a0 = unpack_wave(vm, args[0]);
    std::string s_a1 = read_solix_chars(vm, args[1]);
    const char *a1 = s_a1.c_str();
    bool res = ::ExportWaveAsCode(a0, a1);
    return res ? 1 : 0;
}

static uint64_t wrap_play_sound(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Sound a0 = unpack_sound(vm, args[0]);
    ::PlaySound(a0);
    return 0;
}

static uint64_t wrap_stop_sound(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Sound a0 = unpack_sound(vm, args[0]);
    ::StopSound(a0);
    return 0;
}

static uint64_t wrap_pause_sound(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Sound a0 = unpack_sound(vm, args[0]);
    ::PauseSound(a0);
    return 0;
}

static uint64_t wrap_resume_sound(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Sound a0 = unpack_sound(vm, args[0]);
    ::ResumeSound(a0);
    return 0;
}

static uint64_t wrap_is_sound_playing(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Sound a0 = unpack_sound(vm, args[0]);
    bool res = ::IsSoundPlaying(a0);
    return res ? 1 : 0;
}

static uint64_t wrap_set_sound_volume(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Sound a0 = unpack_sound(vm, args[0]);
    float a1 = unpack_f32_arg(args[1]);
    ::SetSoundVolume(a0, a1);
    return 0;
}

static uint64_t wrap_set_sound_pitch(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Sound a0 = unpack_sound(vm, args[0]);
    float a1 = unpack_f32_arg(args[1]);
    ::SetSoundPitch(a0, a1);
    return 0;
}

static uint64_t wrap_set_sound_pan(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Sound a0 = unpack_sound(vm, args[0]);
    float a1 = unpack_f32_arg(args[1]);
    ::SetSoundPan(a0, a1);
    return 0;
}

static uint64_t wrap_wave_copy(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Wave a0 = unpack_wave(vm, args[0]);
    auto res = ::WaveCopy(a0);
    pack_wave(vm, args[1], res);
    return 0;
}

static uint64_t wrap_wave_crop(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Wave a0 = unpack_wave(vm, args[0]);
    int a1 = static_cast<int>(args[1]);
    int a2 = static_cast<int>(args[2]);
    ::WaveCrop(&a0, a1, a2);
    pack_wave(vm, args[0], a0);
    return 0;
}

static uint64_t wrap_wave_format(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Wave a0 = unpack_wave(vm, args[0]);
    int a1 = static_cast<int>(args[1]);
    int a2 = static_cast<int>(args[2]);
    int a3 = static_cast<int>(args[3]);
    ::WaveFormat(&a0, a1, a2, a3);
    pack_wave(vm, args[0], a0);
    return 0;
}

static uint64_t wrap_load_wave_samples(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Wave a0 = unpack_wave(vm, args[0]);
    auto res = ::LoadWaveSamples(a0);
    return reinterpret_cast<uint64_t>(res);
}

static uint64_t wrap_unload_wave_samples(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    auto v_a0 = unpack_float32_array(vm, args[0], 0);
    ::UnloadWaveSamples(v_a0.data());
    return 0;
}

static uint64_t wrap_load_music_stream(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    auto res = ::LoadMusicStream(a0);
    pack_music(vm, args[1], res);
    return 0;
}

static uint64_t wrap_load_music_stream_from_memory(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    auto v_a1 = unpack_uint8_array(vm, args[1], 0);
    int a2 = static_cast<int>(args[2]);
    auto res = ::LoadMusicStreamFromMemory(a0, v_a1.data(), a2);
    pack_music(vm, args[3], res);
    return 0;
}

static uint64_t wrap_is_music_valid(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Music a0 = unpack_music(vm, args[0]);
    bool res = ::IsMusicValid(a0);
    return res ? 1 : 0;
}

static uint64_t wrap_unload_music_stream(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Music a0 = unpack_music(vm, args[0]);
    ::UnloadMusicStream(a0);
    return 0;
}

static uint64_t wrap_play_music_stream(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Music a0 = unpack_music(vm, args[0]);
    ::PlayMusicStream(a0);
    return 0;
}

static uint64_t wrap_is_music_stream_playing(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Music a0 = unpack_music(vm, args[0]);
    bool res = ::IsMusicStreamPlaying(a0);
    return res ? 1 : 0;
}

static uint64_t wrap_update_music_stream(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Music a0 = unpack_music(vm, args[0]);
    ::UpdateMusicStream(a0);
    return 0;
}

static uint64_t wrap_stop_music_stream(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Music a0 = unpack_music(vm, args[0]);
    ::StopMusicStream(a0);
    return 0;
}

static uint64_t wrap_pause_music_stream(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Music a0 = unpack_music(vm, args[0]);
    ::PauseMusicStream(a0);
    return 0;
}

static uint64_t wrap_resume_music_stream(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Music a0 = unpack_music(vm, args[0]);
    ::ResumeMusicStream(a0);
    return 0;
}

static uint64_t wrap_seek_music_stream(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Music a0 = unpack_music(vm, args[0]);
    float a1 = unpack_f32_arg(args[1]);
    ::SeekMusicStream(a0, a1);
    return 0;
}

static uint64_t wrap_set_music_volume(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Music a0 = unpack_music(vm, args[0]);
    float a1 = unpack_f32_arg(args[1]);
    ::SetMusicVolume(a0, a1);
    return 0;
}

static uint64_t wrap_set_music_pitch(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Music a0 = unpack_music(vm, args[0]);
    float a1 = unpack_f32_arg(args[1]);
    ::SetMusicPitch(a0, a1);
    return 0;
}

static uint64_t wrap_set_music_pan(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Music a0 = unpack_music(vm, args[0]);
    float a1 = unpack_f32_arg(args[1]);
    ::SetMusicPan(a0, a1);
    return 0;
}

static uint64_t wrap_get_music_time_length(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Music a0 = unpack_music(vm, args[0]);
    float res = ::GetMusicTimeLength(a0);
    return pack_f32_ret(res);
}

static uint64_t wrap_get_music_time_played(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Music a0 = unpack_music(vm, args[0]);
    float res = ::GetMusicTimePlayed(a0);
    return pack_f32_ret(res);
}

static uint64_t wrap_load_audio_stream(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    unsigned int a0 = static_cast<unsigned int>(args[0]);
    unsigned int a1 = static_cast<unsigned int>(args[1]);
    unsigned int a2 = static_cast<unsigned int>(args[2]);
    auto res = ::LoadAudioStream(a0, a1, a2);
    pack_audiostream(vm, args[3], res);
    return 0;
}

static uint64_t wrap_is_audio_stream_valid(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::AudioStream a0 = unpack_audiostream(vm, args[0]);
    bool res = ::IsAudioStreamValid(a0);
    return res ? 1 : 0;
}

static uint64_t wrap_unload_audio_stream(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::AudioStream a0 = unpack_audiostream(vm, args[0]);
    ::UnloadAudioStream(a0);
    return 0;
}

static uint64_t wrap_update_audio_stream(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::AudioStream a0 = unpack_audiostream(vm, args[0]);
    void *a1 = reinterpret_cast<void *>(args[1]);
    int a2 = static_cast<int>(args[2]);
    ::UpdateAudioStream(a0, a1, a2);
    return 0;
}

static uint64_t wrap_is_audio_stream_processed(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::AudioStream a0 = unpack_audiostream(vm, args[0]);
    bool res = ::IsAudioStreamProcessed(a0);
    return res ? 1 : 0;
}

static uint64_t wrap_play_audio_stream(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::AudioStream a0 = unpack_audiostream(vm, args[0]);
    ::PlayAudioStream(a0);
    return 0;
}

static uint64_t wrap_pause_audio_stream(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::AudioStream a0 = unpack_audiostream(vm, args[0]);
    ::PauseAudioStream(a0);
    return 0;
}

static uint64_t wrap_resume_audio_stream(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::AudioStream a0 = unpack_audiostream(vm, args[0]);
    ::ResumeAudioStream(a0);
    return 0;
}

static uint64_t wrap_is_audio_stream_playing(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::AudioStream a0 = unpack_audiostream(vm, args[0]);
    bool res = ::IsAudioStreamPlaying(a0);
    return res ? 1 : 0;
}

static uint64_t wrap_stop_audio_stream(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::AudioStream a0 = unpack_audiostream(vm, args[0]);
    ::StopAudioStream(a0);
    return 0;
}

static uint64_t wrap_set_audio_stream_volume(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::AudioStream a0 = unpack_audiostream(vm, args[0]);
    float a1 = unpack_f32_arg(args[1]);
    ::SetAudioStreamVolume(a0, a1);
    return 0;
}

static uint64_t wrap_set_audio_stream_pitch(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::AudioStream a0 = unpack_audiostream(vm, args[0]);
    float a1 = unpack_f32_arg(args[1]);
    ::SetAudioStreamPitch(a0, a1);
    return 0;
}

static uint64_t wrap_set_audio_stream_pan(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::AudioStream a0 = unpack_audiostream(vm, args[0]);
    float a1 = unpack_f32_arg(args[1]);
    ::SetAudioStreamPan(a0, a1);
    return 0;
}

static uint64_t wrap_set_audio_stream_buffer_size_default(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    ::SetAudioStreamBufferSizeDefault(a0);
    return 0;
}

} // namespace

void register_raylib_natives(solix::NativeRegistry &registry) {
    registry.register_function("Raylib_native_init_window", wrap_init_window);
    registry.register_function("raylib_Raylib_native_init_window", wrap_init_window);
    registry.register_function("native_init_window", wrap_init_window);
    registry.register_function("Raylib_native_close_window", wrap_close_window);
    registry.register_function("raylib_Raylib_native_close_window", wrap_close_window);
    registry.register_function("native_close_window", wrap_close_window);
    registry.register_function("Raylib_native_window_should_close", wrap_window_should_close);
    registry.register_function("raylib_Raylib_native_window_should_close", wrap_window_should_close);
    registry.register_function("native_window_should_close", wrap_window_should_close);
    registry.register_function("Raylib_native_is_window_ready", wrap_is_window_ready);
    registry.register_function("raylib_Raylib_native_is_window_ready", wrap_is_window_ready);
    registry.register_function("native_is_window_ready", wrap_is_window_ready);
    registry.register_function("Raylib_native_is_window_fullscreen", wrap_is_window_fullscreen);
    registry.register_function("raylib_Raylib_native_is_window_fullscreen", wrap_is_window_fullscreen);
    registry.register_function("native_is_window_fullscreen", wrap_is_window_fullscreen);
    registry.register_function("Raylib_native_is_window_hidden", wrap_is_window_hidden);
    registry.register_function("raylib_Raylib_native_is_window_hidden", wrap_is_window_hidden);
    registry.register_function("native_is_window_hidden", wrap_is_window_hidden);
    registry.register_function("Raylib_native_is_window_minimized", wrap_is_window_minimized);
    registry.register_function("raylib_Raylib_native_is_window_minimized", wrap_is_window_minimized);
    registry.register_function("native_is_window_minimized", wrap_is_window_minimized);
    registry.register_function("Raylib_native_is_window_maximized", wrap_is_window_maximized);
    registry.register_function("raylib_Raylib_native_is_window_maximized", wrap_is_window_maximized);
    registry.register_function("native_is_window_maximized", wrap_is_window_maximized);
    registry.register_function("Raylib_native_is_window_focused", wrap_is_window_focused);
    registry.register_function("raylib_Raylib_native_is_window_focused", wrap_is_window_focused);
    registry.register_function("native_is_window_focused", wrap_is_window_focused);
    registry.register_function("Raylib_native_is_window_resized", wrap_is_window_resized);
    registry.register_function("raylib_Raylib_native_is_window_resized", wrap_is_window_resized);
    registry.register_function("native_is_window_resized", wrap_is_window_resized);
    registry.register_function("Raylib_native_is_window_state", wrap_is_window_state);
    registry.register_function("raylib_Raylib_native_is_window_state", wrap_is_window_state);
    registry.register_function("native_is_window_state", wrap_is_window_state);
    registry.register_function("Raylib_native_set_window_state", wrap_set_window_state);
    registry.register_function("raylib_Raylib_native_set_window_state", wrap_set_window_state);
    registry.register_function("native_set_window_state", wrap_set_window_state);
    registry.register_function("Raylib_native_clear_window_state", wrap_clear_window_state);
    registry.register_function("raylib_Raylib_native_clear_window_state", wrap_clear_window_state);
    registry.register_function("native_clear_window_state", wrap_clear_window_state);
    registry.register_function("Raylib_native_toggle_fullscreen", wrap_toggle_fullscreen);
    registry.register_function("raylib_Raylib_native_toggle_fullscreen", wrap_toggle_fullscreen);
    registry.register_function("native_toggle_fullscreen", wrap_toggle_fullscreen);
    registry.register_function("Raylib_native_toggle_borderless_windowed", wrap_toggle_borderless_windowed);
    registry.register_function("raylib_Raylib_native_toggle_borderless_windowed", wrap_toggle_borderless_windowed);
    registry.register_function("native_toggle_borderless_windowed", wrap_toggle_borderless_windowed);
    registry.register_function("Raylib_native_maximize_window", wrap_maximize_window);
    registry.register_function("raylib_Raylib_native_maximize_window", wrap_maximize_window);
    registry.register_function("native_maximize_window", wrap_maximize_window);
    registry.register_function("Raylib_native_minimize_window", wrap_minimize_window);
    registry.register_function("raylib_Raylib_native_minimize_window", wrap_minimize_window);
    registry.register_function("native_minimize_window", wrap_minimize_window);
    registry.register_function("Raylib_native_restore_window", wrap_restore_window);
    registry.register_function("raylib_Raylib_native_restore_window", wrap_restore_window);
    registry.register_function("native_restore_window", wrap_restore_window);
    registry.register_function("Raylib_native_set_window_icon", wrap_set_window_icon);
    registry.register_function("raylib_Raylib_native_set_window_icon", wrap_set_window_icon);
    registry.register_function("native_set_window_icon", wrap_set_window_icon);
    registry.register_function("Raylib_native_set_window_icons", wrap_set_window_icons);
    registry.register_function("raylib_Raylib_native_set_window_icons", wrap_set_window_icons);
    registry.register_function("native_set_window_icons", wrap_set_window_icons);
    registry.register_function("Raylib_native_set_window_title", wrap_set_window_title);
    registry.register_function("raylib_Raylib_native_set_window_title", wrap_set_window_title);
    registry.register_function("native_set_window_title", wrap_set_window_title);
    registry.register_function("Raylib_native_set_window_position", wrap_set_window_position);
    registry.register_function("raylib_Raylib_native_set_window_position", wrap_set_window_position);
    registry.register_function("native_set_window_position", wrap_set_window_position);
    registry.register_function("Raylib_native_set_window_monitor", wrap_set_window_monitor);
    registry.register_function("raylib_Raylib_native_set_window_monitor", wrap_set_window_monitor);
    registry.register_function("native_set_window_monitor", wrap_set_window_monitor);
    registry.register_function("Raylib_native_set_window_min_size", wrap_set_window_min_size);
    registry.register_function("raylib_Raylib_native_set_window_min_size", wrap_set_window_min_size);
    registry.register_function("native_set_window_min_size", wrap_set_window_min_size);
    registry.register_function("Raylib_native_set_window_max_size", wrap_set_window_max_size);
    registry.register_function("raylib_Raylib_native_set_window_max_size", wrap_set_window_max_size);
    registry.register_function("native_set_window_max_size", wrap_set_window_max_size);
    registry.register_function("Raylib_native_set_window_size", wrap_set_window_size);
    registry.register_function("raylib_Raylib_native_set_window_size", wrap_set_window_size);
    registry.register_function("native_set_window_size", wrap_set_window_size);
    registry.register_function("Raylib_native_set_window_opacity", wrap_set_window_opacity);
    registry.register_function("raylib_Raylib_native_set_window_opacity", wrap_set_window_opacity);
    registry.register_function("native_set_window_opacity", wrap_set_window_opacity);
    registry.register_function("Raylib_native_set_window_focused", wrap_set_window_focused);
    registry.register_function("raylib_Raylib_native_set_window_focused", wrap_set_window_focused);
    registry.register_function("native_set_window_focused", wrap_set_window_focused);
    registry.register_function("Raylib_native_get_window_handle", wrap_get_window_handle);
    registry.register_function("raylib_Raylib_native_get_window_handle", wrap_get_window_handle);
    registry.register_function("native_get_window_handle", wrap_get_window_handle);
    registry.register_function("Raylib_native_get_screen_width", wrap_get_screen_width);
    registry.register_function("raylib_Raylib_native_get_screen_width", wrap_get_screen_width);
    registry.register_function("native_get_screen_width", wrap_get_screen_width);
    registry.register_function("Raylib_native_get_screen_height", wrap_get_screen_height);
    registry.register_function("raylib_Raylib_native_get_screen_height", wrap_get_screen_height);
    registry.register_function("native_get_screen_height", wrap_get_screen_height);
    registry.register_function("Raylib_native_get_render_width", wrap_get_render_width);
    registry.register_function("raylib_Raylib_native_get_render_width", wrap_get_render_width);
    registry.register_function("native_get_render_width", wrap_get_render_width);
    registry.register_function("Raylib_native_get_render_height", wrap_get_render_height);
    registry.register_function("raylib_Raylib_native_get_render_height", wrap_get_render_height);
    registry.register_function("native_get_render_height", wrap_get_render_height);
    registry.register_function("Raylib_native_get_monitor_count", wrap_get_monitor_count);
    registry.register_function("raylib_Raylib_native_get_monitor_count", wrap_get_monitor_count);
    registry.register_function("native_get_monitor_count", wrap_get_monitor_count);
    registry.register_function("Raylib_native_get_current_monitor", wrap_get_current_monitor);
    registry.register_function("raylib_Raylib_native_get_current_monitor", wrap_get_current_monitor);
    registry.register_function("native_get_current_monitor", wrap_get_current_monitor);
    registry.register_function("Raylib_native_get_monitor_position", wrap_get_monitor_position);
    registry.register_function("raylib_Raylib_native_get_monitor_position", wrap_get_monitor_position);
    registry.register_function("native_get_monitor_position", wrap_get_monitor_position);
    registry.register_function("Raylib_native_get_monitor_width", wrap_get_monitor_width);
    registry.register_function("raylib_Raylib_native_get_monitor_width", wrap_get_monitor_width);
    registry.register_function("native_get_monitor_width", wrap_get_monitor_width);
    registry.register_function("Raylib_native_get_monitor_height", wrap_get_monitor_height);
    registry.register_function("raylib_Raylib_native_get_monitor_height", wrap_get_monitor_height);
    registry.register_function("native_get_monitor_height", wrap_get_monitor_height);
    registry.register_function("Raylib_native_get_monitor_physical_width", wrap_get_monitor_physical_width);
    registry.register_function("raylib_Raylib_native_get_monitor_physical_width", wrap_get_monitor_physical_width);
    registry.register_function("native_get_monitor_physical_width", wrap_get_monitor_physical_width);
    registry.register_function("Raylib_native_get_monitor_physical_height", wrap_get_monitor_physical_height);
    registry.register_function("raylib_Raylib_native_get_monitor_physical_height", wrap_get_monitor_physical_height);
    registry.register_function("native_get_monitor_physical_height", wrap_get_monitor_physical_height);
    registry.register_function("Raylib_native_get_monitor_refresh_rate", wrap_get_monitor_refresh_rate);
    registry.register_function("raylib_Raylib_native_get_monitor_refresh_rate", wrap_get_monitor_refresh_rate);
    registry.register_function("native_get_monitor_refresh_rate", wrap_get_monitor_refresh_rate);
    registry.register_function("Raylib_native_get_window_position", wrap_get_window_position);
    registry.register_function("raylib_Raylib_native_get_window_position", wrap_get_window_position);
    registry.register_function("native_get_window_position", wrap_get_window_position);
    registry.register_function("Raylib_native_get_window_scale_dpi", wrap_get_window_scale_dpi);
    registry.register_function("raylib_Raylib_native_get_window_scale_dpi", wrap_get_window_scale_dpi);
    registry.register_function("native_get_window_scale_dpi", wrap_get_window_scale_dpi);
    registry.register_function("Raylib_native_get_monitor_name", wrap_get_monitor_name);
    registry.register_function("raylib_Raylib_native_get_monitor_name", wrap_get_monitor_name);
    registry.register_function("native_get_monitor_name", wrap_get_monitor_name);
    registry.register_function("Raylib_native_set_clipboard_text", wrap_set_clipboard_text);
    registry.register_function("raylib_Raylib_native_set_clipboard_text", wrap_set_clipboard_text);
    registry.register_function("native_set_clipboard_text", wrap_set_clipboard_text);
    registry.register_function("Raylib_native_get_clipboard_text", wrap_get_clipboard_text);
    registry.register_function("raylib_Raylib_native_get_clipboard_text", wrap_get_clipboard_text);
    registry.register_function("native_get_clipboard_text", wrap_get_clipboard_text);
    registry.register_function("Raylib_native_get_clipboard_image", wrap_get_clipboard_image);
    registry.register_function("raylib_Raylib_native_get_clipboard_image", wrap_get_clipboard_image);
    registry.register_function("native_get_clipboard_image", wrap_get_clipboard_image);
    registry.register_function("Raylib_native_enable_event_waiting", wrap_enable_event_waiting);
    registry.register_function("raylib_Raylib_native_enable_event_waiting", wrap_enable_event_waiting);
    registry.register_function("native_enable_event_waiting", wrap_enable_event_waiting);
    registry.register_function("Raylib_native_disable_event_waiting", wrap_disable_event_waiting);
    registry.register_function("raylib_Raylib_native_disable_event_waiting", wrap_disable_event_waiting);
    registry.register_function("native_disable_event_waiting", wrap_disable_event_waiting);
    registry.register_function("Raylib_native_show_cursor", wrap_show_cursor);
    registry.register_function("raylib_Raylib_native_show_cursor", wrap_show_cursor);
    registry.register_function("native_show_cursor", wrap_show_cursor);
    registry.register_function("Raylib_native_hide_cursor", wrap_hide_cursor);
    registry.register_function("raylib_Raylib_native_hide_cursor", wrap_hide_cursor);
    registry.register_function("native_hide_cursor", wrap_hide_cursor);
    registry.register_function("Raylib_native_is_cursor_hidden", wrap_is_cursor_hidden);
    registry.register_function("raylib_Raylib_native_is_cursor_hidden", wrap_is_cursor_hidden);
    registry.register_function("native_is_cursor_hidden", wrap_is_cursor_hidden);
    registry.register_function("Raylib_native_enable_cursor", wrap_enable_cursor);
    registry.register_function("raylib_Raylib_native_enable_cursor", wrap_enable_cursor);
    registry.register_function("native_enable_cursor", wrap_enable_cursor);
    registry.register_function("Raylib_native_disable_cursor", wrap_disable_cursor);
    registry.register_function("raylib_Raylib_native_disable_cursor", wrap_disable_cursor);
    registry.register_function("native_disable_cursor", wrap_disable_cursor);
    registry.register_function("Raylib_native_is_cursor_on_screen", wrap_is_cursor_on_screen);
    registry.register_function("raylib_Raylib_native_is_cursor_on_screen", wrap_is_cursor_on_screen);
    registry.register_function("native_is_cursor_on_screen", wrap_is_cursor_on_screen);
    registry.register_function("Raylib_native_clear_background", wrap_clear_background);
    registry.register_function("raylib_Raylib_native_clear_background", wrap_clear_background);
    registry.register_function("native_clear_background", wrap_clear_background);
    registry.register_function("Raylib_native_begin_drawing", wrap_begin_drawing);
    registry.register_function("raylib_Raylib_native_begin_drawing", wrap_begin_drawing);
    registry.register_function("native_begin_drawing", wrap_begin_drawing);
    registry.register_function("Raylib_native_end_drawing", wrap_end_drawing);
    registry.register_function("raylib_Raylib_native_end_drawing", wrap_end_drawing);
    registry.register_function("native_end_drawing", wrap_end_drawing);
    registry.register_function("Raylib_native_begin_mode2d", wrap_begin_mode2d);
    registry.register_function("raylib_Raylib_native_begin_mode2d", wrap_begin_mode2d);
    registry.register_function("native_begin_mode2d", wrap_begin_mode2d);
    registry.register_function("Raylib_native_end_mode2d", wrap_end_mode2d);
    registry.register_function("raylib_Raylib_native_end_mode2d", wrap_end_mode2d);
    registry.register_function("native_end_mode2d", wrap_end_mode2d);
    registry.register_function("Raylib_native_begin_mode3d", wrap_begin_mode3d);
    registry.register_function("raylib_Raylib_native_begin_mode3d", wrap_begin_mode3d);
    registry.register_function("native_begin_mode3d", wrap_begin_mode3d);
    registry.register_function("Raylib_native_end_mode3d", wrap_end_mode3d);
    registry.register_function("raylib_Raylib_native_end_mode3d", wrap_end_mode3d);
    registry.register_function("native_end_mode3d", wrap_end_mode3d);
    registry.register_function("Raylib_native_begin_texture_mode", wrap_begin_texture_mode);
    registry.register_function("raylib_Raylib_native_begin_texture_mode", wrap_begin_texture_mode);
    registry.register_function("native_begin_texture_mode", wrap_begin_texture_mode);
    registry.register_function("Raylib_native_end_texture_mode", wrap_end_texture_mode);
    registry.register_function("raylib_Raylib_native_end_texture_mode", wrap_end_texture_mode);
    registry.register_function("native_end_texture_mode", wrap_end_texture_mode);
    registry.register_function("Raylib_native_begin_shader_mode", wrap_begin_shader_mode);
    registry.register_function("raylib_Raylib_native_begin_shader_mode", wrap_begin_shader_mode);
    registry.register_function("native_begin_shader_mode", wrap_begin_shader_mode);
    registry.register_function("Raylib_native_end_shader_mode", wrap_end_shader_mode);
    registry.register_function("raylib_Raylib_native_end_shader_mode", wrap_end_shader_mode);
    registry.register_function("native_end_shader_mode", wrap_end_shader_mode);
    registry.register_function("Raylib_native_begin_blend_mode", wrap_begin_blend_mode);
    registry.register_function("raylib_Raylib_native_begin_blend_mode", wrap_begin_blend_mode);
    registry.register_function("native_begin_blend_mode", wrap_begin_blend_mode);
    registry.register_function("Raylib_native_end_blend_mode", wrap_end_blend_mode);
    registry.register_function("raylib_Raylib_native_end_blend_mode", wrap_end_blend_mode);
    registry.register_function("native_end_blend_mode", wrap_end_blend_mode);
    registry.register_function("Raylib_native_begin_scissor_mode", wrap_begin_scissor_mode);
    registry.register_function("raylib_Raylib_native_begin_scissor_mode", wrap_begin_scissor_mode);
    registry.register_function("native_begin_scissor_mode", wrap_begin_scissor_mode);
    registry.register_function("Raylib_native_end_scissor_mode", wrap_end_scissor_mode);
    registry.register_function("raylib_Raylib_native_end_scissor_mode", wrap_end_scissor_mode);
    registry.register_function("native_end_scissor_mode", wrap_end_scissor_mode);
    registry.register_function("Raylib_native_begin_vr_stereo_mode", wrap_begin_vr_stereo_mode);
    registry.register_function("raylib_Raylib_native_begin_vr_stereo_mode", wrap_begin_vr_stereo_mode);
    registry.register_function("native_begin_vr_stereo_mode", wrap_begin_vr_stereo_mode);
    registry.register_function("Raylib_native_end_vr_stereo_mode", wrap_end_vr_stereo_mode);
    registry.register_function("raylib_Raylib_native_end_vr_stereo_mode", wrap_end_vr_stereo_mode);
    registry.register_function("native_end_vr_stereo_mode", wrap_end_vr_stereo_mode);
    registry.register_function("Raylib_native_load_vr_stereo_config", wrap_load_vr_stereo_config);
    registry.register_function("raylib_Raylib_native_load_vr_stereo_config", wrap_load_vr_stereo_config);
    registry.register_function("native_load_vr_stereo_config", wrap_load_vr_stereo_config);
    registry.register_function("Raylib_native_unload_vr_stereo_config", wrap_unload_vr_stereo_config);
    registry.register_function("raylib_Raylib_native_unload_vr_stereo_config", wrap_unload_vr_stereo_config);
    registry.register_function("native_unload_vr_stereo_config", wrap_unload_vr_stereo_config);
    registry.register_function("Raylib_native_load_shader", wrap_load_shader);
    registry.register_function("raylib_Raylib_native_load_shader", wrap_load_shader);
    registry.register_function("native_load_shader", wrap_load_shader);
    registry.register_function("Raylib_native_load_shader_from_memory", wrap_load_shader_from_memory);
    registry.register_function("raylib_Raylib_native_load_shader_from_memory", wrap_load_shader_from_memory);
    registry.register_function("native_load_shader_from_memory", wrap_load_shader_from_memory);
    registry.register_function("Raylib_native_is_shader_valid", wrap_is_shader_valid);
    registry.register_function("raylib_Raylib_native_is_shader_valid", wrap_is_shader_valid);
    registry.register_function("native_is_shader_valid", wrap_is_shader_valid);
    registry.register_function("Raylib_native_get_shader_location", wrap_get_shader_location);
    registry.register_function("raylib_Raylib_native_get_shader_location", wrap_get_shader_location);
    registry.register_function("native_get_shader_location", wrap_get_shader_location);
    registry.register_function("Raylib_native_get_shader_location_attrib", wrap_get_shader_location_attrib);
    registry.register_function("raylib_Raylib_native_get_shader_location_attrib", wrap_get_shader_location_attrib);
    registry.register_function("native_get_shader_location_attrib", wrap_get_shader_location_attrib);
    registry.register_function("Raylib_native_set_shader_value", wrap_set_shader_value);
    registry.register_function("raylib_Raylib_native_set_shader_value", wrap_set_shader_value);
    registry.register_function("native_set_shader_value", wrap_set_shader_value);
    registry.register_function("Raylib_native_set_shader_value_v", wrap_set_shader_value_v);
    registry.register_function("raylib_Raylib_native_set_shader_value_v", wrap_set_shader_value_v);
    registry.register_function("native_set_shader_value_v", wrap_set_shader_value_v);
    registry.register_function("Raylib_native_set_shader_value_matrix", wrap_set_shader_value_matrix);
    registry.register_function("raylib_Raylib_native_set_shader_value_matrix", wrap_set_shader_value_matrix);
    registry.register_function("native_set_shader_value_matrix", wrap_set_shader_value_matrix);
    registry.register_function("Raylib_native_set_shader_value_texture", wrap_set_shader_value_texture);
    registry.register_function("raylib_Raylib_native_set_shader_value_texture", wrap_set_shader_value_texture);
    registry.register_function("native_set_shader_value_texture", wrap_set_shader_value_texture);
    registry.register_function("Raylib_native_unload_shader", wrap_unload_shader);
    registry.register_function("raylib_Raylib_native_unload_shader", wrap_unload_shader);
    registry.register_function("native_unload_shader", wrap_unload_shader);
    registry.register_function("Raylib_native_get_screen_to_world_ray", wrap_get_screen_to_world_ray);
    registry.register_function("raylib_Raylib_native_get_screen_to_world_ray", wrap_get_screen_to_world_ray);
    registry.register_function("native_get_screen_to_world_ray", wrap_get_screen_to_world_ray);
    registry.register_function("Raylib_native_get_screen_to_world_ray_ex", wrap_get_screen_to_world_ray_ex);
    registry.register_function("raylib_Raylib_native_get_screen_to_world_ray_ex", wrap_get_screen_to_world_ray_ex);
    registry.register_function("native_get_screen_to_world_ray_ex", wrap_get_screen_to_world_ray_ex);
    registry.register_function("Raylib_native_get_world_to_screen", wrap_get_world_to_screen);
    registry.register_function("raylib_Raylib_native_get_world_to_screen", wrap_get_world_to_screen);
    registry.register_function("native_get_world_to_screen", wrap_get_world_to_screen);
    registry.register_function("Raylib_native_get_world_to_screen_ex", wrap_get_world_to_screen_ex);
    registry.register_function("raylib_Raylib_native_get_world_to_screen_ex", wrap_get_world_to_screen_ex);
    registry.register_function("native_get_world_to_screen_ex", wrap_get_world_to_screen_ex);
    registry.register_function("Raylib_native_get_world_to_screen2d", wrap_get_world_to_screen2d);
    registry.register_function("raylib_Raylib_native_get_world_to_screen2d", wrap_get_world_to_screen2d);
    registry.register_function("native_get_world_to_screen2d", wrap_get_world_to_screen2d);
    registry.register_function("Raylib_native_get_screen_to_world2d", wrap_get_screen_to_world2d);
    registry.register_function("raylib_Raylib_native_get_screen_to_world2d", wrap_get_screen_to_world2d);
    registry.register_function("native_get_screen_to_world2d", wrap_get_screen_to_world2d);
    registry.register_function("Raylib_native_get_camera_matrix", wrap_get_camera_matrix);
    registry.register_function("raylib_Raylib_native_get_camera_matrix", wrap_get_camera_matrix);
    registry.register_function("native_get_camera_matrix", wrap_get_camera_matrix);
    registry.register_function("Raylib_native_get_camera_matrix2d", wrap_get_camera_matrix2d);
    registry.register_function("raylib_Raylib_native_get_camera_matrix2d", wrap_get_camera_matrix2d);
    registry.register_function("native_get_camera_matrix2d", wrap_get_camera_matrix2d);
    registry.register_function("Raylib_native_set_target_fps", wrap_set_target_fps);
    registry.register_function("raylib_Raylib_native_set_target_fps", wrap_set_target_fps);
    registry.register_function("native_set_target_fps", wrap_set_target_fps);
    registry.register_function("Raylib_native_get_frame_time", wrap_get_frame_time);
    registry.register_function("raylib_Raylib_native_get_frame_time", wrap_get_frame_time);
    registry.register_function("native_get_frame_time", wrap_get_frame_time);
    registry.register_function("Raylib_native_get_time", wrap_get_time);
    registry.register_function("raylib_Raylib_native_get_time", wrap_get_time);
    registry.register_function("native_get_time", wrap_get_time);
    registry.register_function("Raylib_native_get_fps", wrap_get_fps);
    registry.register_function("raylib_Raylib_native_get_fps", wrap_get_fps);
    registry.register_function("native_get_fps", wrap_get_fps);
    registry.register_function("Raylib_native_swap_screen_buffer", wrap_swap_screen_buffer);
    registry.register_function("raylib_Raylib_native_swap_screen_buffer", wrap_swap_screen_buffer);
    registry.register_function("native_swap_screen_buffer", wrap_swap_screen_buffer);
    registry.register_function("Raylib_native_poll_input_events", wrap_poll_input_events);
    registry.register_function("raylib_Raylib_native_poll_input_events", wrap_poll_input_events);
    registry.register_function("native_poll_input_events", wrap_poll_input_events);
    registry.register_function("Raylib_native_wait_time", wrap_wait_time);
    registry.register_function("raylib_Raylib_native_wait_time", wrap_wait_time);
    registry.register_function("native_wait_time", wrap_wait_time);
    registry.register_function("Raylib_native_set_random_seed", wrap_set_random_seed);
    registry.register_function("raylib_Raylib_native_set_random_seed", wrap_set_random_seed);
    registry.register_function("native_set_random_seed", wrap_set_random_seed);
    registry.register_function("Raylib_native_get_random_value", wrap_get_random_value);
    registry.register_function("raylib_Raylib_native_get_random_value", wrap_get_random_value);
    registry.register_function("native_get_random_value", wrap_get_random_value);
    registry.register_function("Raylib_native_load_random_sequence", wrap_load_random_sequence);
    registry.register_function("raylib_Raylib_native_load_random_sequence", wrap_load_random_sequence);
    registry.register_function("native_load_random_sequence", wrap_load_random_sequence);
    registry.register_function("Raylib_native_unload_random_sequence", wrap_unload_random_sequence);
    registry.register_function("raylib_Raylib_native_unload_random_sequence", wrap_unload_random_sequence);
    registry.register_function("native_unload_random_sequence", wrap_unload_random_sequence);
    registry.register_function("Raylib_native_take_screenshot", wrap_take_screenshot);
    registry.register_function("raylib_Raylib_native_take_screenshot", wrap_take_screenshot);
    registry.register_function("native_take_screenshot", wrap_take_screenshot);
    registry.register_function("Raylib_native_set_config_flags", wrap_set_config_flags);
    registry.register_function("raylib_Raylib_native_set_config_flags", wrap_set_config_flags);
    registry.register_function("native_set_config_flags", wrap_set_config_flags);
    registry.register_function("Raylib_native_open_url", wrap_open_url);
    registry.register_function("raylib_Raylib_native_open_url", wrap_open_url);
    registry.register_function("native_open_url", wrap_open_url);
    registry.register_function("Raylib_native_set_trace_log_level", wrap_set_trace_log_level);
    registry.register_function("raylib_Raylib_native_set_trace_log_level", wrap_set_trace_log_level);
    registry.register_function("native_set_trace_log_level", wrap_set_trace_log_level);
    registry.register_function("Raylib_native_mem_alloc", wrap_mem_alloc);
    registry.register_function("raylib_Raylib_native_mem_alloc", wrap_mem_alloc);
    registry.register_function("native_mem_alloc", wrap_mem_alloc);
    registry.register_function("Raylib_native_mem_realloc", wrap_mem_realloc);
    registry.register_function("raylib_Raylib_native_mem_realloc", wrap_mem_realloc);
    registry.register_function("native_mem_realloc", wrap_mem_realloc);
    registry.register_function("Raylib_native_mem_free", wrap_mem_free);
    registry.register_function("raylib_Raylib_native_mem_free", wrap_mem_free);
    registry.register_function("native_mem_free", wrap_mem_free);
    registry.register_function("Raylib_native_load_file_data", wrap_load_file_data);
    registry.register_function("raylib_Raylib_native_load_file_data", wrap_load_file_data);
    registry.register_function("native_load_file_data", wrap_load_file_data);
    registry.register_function("Raylib_native_unload_file_data", wrap_unload_file_data);
    registry.register_function("raylib_Raylib_native_unload_file_data", wrap_unload_file_data);
    registry.register_function("native_unload_file_data", wrap_unload_file_data);
    registry.register_function("Raylib_native_save_file_data", wrap_save_file_data);
    registry.register_function("raylib_Raylib_native_save_file_data", wrap_save_file_data);
    registry.register_function("native_save_file_data", wrap_save_file_data);
    registry.register_function("Raylib_native_export_data_as_code", wrap_export_data_as_code);
    registry.register_function("raylib_Raylib_native_export_data_as_code", wrap_export_data_as_code);
    registry.register_function("native_export_data_as_code", wrap_export_data_as_code);
    registry.register_function("Raylib_native_load_file_text", wrap_load_file_text);
    registry.register_function("raylib_Raylib_native_load_file_text", wrap_load_file_text);
    registry.register_function("native_load_file_text", wrap_load_file_text);
    registry.register_function("Raylib_native_unload_file_text", wrap_unload_file_text);
    registry.register_function("raylib_Raylib_native_unload_file_text", wrap_unload_file_text);
    registry.register_function("native_unload_file_text", wrap_unload_file_text);
    registry.register_function("Raylib_native_save_file_text", wrap_save_file_text);
    registry.register_function("raylib_Raylib_native_save_file_text", wrap_save_file_text);
    registry.register_function("native_save_file_text", wrap_save_file_text);
    registry.register_function("Raylib_native_file_rename", wrap_file_rename);
    registry.register_function("raylib_Raylib_native_file_rename", wrap_file_rename);
    registry.register_function("native_file_rename", wrap_file_rename);
    registry.register_function("Raylib_native_file_remove", wrap_file_remove);
    registry.register_function("raylib_Raylib_native_file_remove", wrap_file_remove);
    registry.register_function("native_file_remove", wrap_file_remove);
    registry.register_function("Raylib_native_file_copy", wrap_file_copy);
    registry.register_function("raylib_Raylib_native_file_copy", wrap_file_copy);
    registry.register_function("native_file_copy", wrap_file_copy);
    registry.register_function("Raylib_native_file_move", wrap_file_move);
    registry.register_function("raylib_Raylib_native_file_move", wrap_file_move);
    registry.register_function("native_file_move", wrap_file_move);
    registry.register_function("Raylib_native_file_text_replace", wrap_file_text_replace);
    registry.register_function("raylib_Raylib_native_file_text_replace", wrap_file_text_replace);
    registry.register_function("native_file_text_replace", wrap_file_text_replace);
    registry.register_function("Raylib_native_file_text_find_index", wrap_file_text_find_index);
    registry.register_function("raylib_Raylib_native_file_text_find_index", wrap_file_text_find_index);
    registry.register_function("native_file_text_find_index", wrap_file_text_find_index);
    registry.register_function("Raylib_native_file_exists", wrap_file_exists);
    registry.register_function("raylib_Raylib_native_file_exists", wrap_file_exists);
    registry.register_function("native_file_exists", wrap_file_exists);
    registry.register_function("Raylib_native_directory_exists", wrap_directory_exists);
    registry.register_function("raylib_Raylib_native_directory_exists", wrap_directory_exists);
    registry.register_function("native_directory_exists", wrap_directory_exists);
    registry.register_function("Raylib_native_is_file_extension", wrap_is_file_extension);
    registry.register_function("raylib_Raylib_native_is_file_extension", wrap_is_file_extension);
    registry.register_function("native_is_file_extension", wrap_is_file_extension);
    registry.register_function("Raylib_native_is_file_hidden", wrap_is_file_hidden);
    registry.register_function("raylib_Raylib_native_is_file_hidden", wrap_is_file_hidden);
    registry.register_function("native_is_file_hidden", wrap_is_file_hidden);
    registry.register_function("Raylib_native_get_file_length", wrap_get_file_length);
    registry.register_function("raylib_Raylib_native_get_file_length", wrap_get_file_length);
    registry.register_function("native_get_file_length", wrap_get_file_length);
    registry.register_function("Raylib_native_get_file_mod_time", wrap_get_file_mod_time);
    registry.register_function("raylib_Raylib_native_get_file_mod_time", wrap_get_file_mod_time);
    registry.register_function("native_get_file_mod_time", wrap_get_file_mod_time);
    registry.register_function("Raylib_native_get_file_extension", wrap_get_file_extension);
    registry.register_function("raylib_Raylib_native_get_file_extension", wrap_get_file_extension);
    registry.register_function("native_get_file_extension", wrap_get_file_extension);
    registry.register_function("Raylib_native_get_file_name", wrap_get_file_name);
    registry.register_function("raylib_Raylib_native_get_file_name", wrap_get_file_name);
    registry.register_function("native_get_file_name", wrap_get_file_name);
    registry.register_function("Raylib_native_get_file_name_without_ext", wrap_get_file_name_without_ext);
    registry.register_function("raylib_Raylib_native_get_file_name_without_ext", wrap_get_file_name_without_ext);
    registry.register_function("native_get_file_name_without_ext", wrap_get_file_name_without_ext);
    registry.register_function("Raylib_native_get_directory_path", wrap_get_directory_path);
    registry.register_function("raylib_Raylib_native_get_directory_path", wrap_get_directory_path);
    registry.register_function("native_get_directory_path", wrap_get_directory_path);
    registry.register_function("Raylib_native_get_prev_directory_path", wrap_get_prev_directory_path);
    registry.register_function("raylib_Raylib_native_get_prev_directory_path", wrap_get_prev_directory_path);
    registry.register_function("native_get_prev_directory_path", wrap_get_prev_directory_path);
    registry.register_function("Raylib_native_get_working_directory", wrap_get_working_directory);
    registry.register_function("raylib_Raylib_native_get_working_directory", wrap_get_working_directory);
    registry.register_function("native_get_working_directory", wrap_get_working_directory);
    registry.register_function("Raylib_native_get_application_directory", wrap_get_application_directory);
    registry.register_function("raylib_Raylib_native_get_application_directory", wrap_get_application_directory);
    registry.register_function("native_get_application_directory", wrap_get_application_directory);
    registry.register_function("Raylib_native_make_directory", wrap_make_directory);
    registry.register_function("raylib_Raylib_native_make_directory", wrap_make_directory);
    registry.register_function("native_make_directory", wrap_make_directory);
    registry.register_function("Raylib_native_change_directory", wrap_change_directory);
    registry.register_function("raylib_Raylib_native_change_directory", wrap_change_directory);
    registry.register_function("native_change_directory", wrap_change_directory);
    registry.register_function("Raylib_native_is_path_file", wrap_is_path_file);
    registry.register_function("raylib_Raylib_native_is_path_file", wrap_is_path_file);
    registry.register_function("native_is_path_file", wrap_is_path_file);
    registry.register_function("Raylib_native_is_path_directory", wrap_is_path_directory);
    registry.register_function("raylib_Raylib_native_is_path_directory", wrap_is_path_directory);
    registry.register_function("native_is_path_directory", wrap_is_path_directory);
    registry.register_function("Raylib_native_is_path_absolute", wrap_is_path_absolute);
    registry.register_function("raylib_Raylib_native_is_path_absolute", wrap_is_path_absolute);
    registry.register_function("native_is_path_absolute", wrap_is_path_absolute);
    registry.register_function("Raylib_native_is_file_name_valid", wrap_is_file_name_valid);
    registry.register_function("raylib_Raylib_native_is_file_name_valid", wrap_is_file_name_valid);
    registry.register_function("native_is_file_name_valid", wrap_is_file_name_valid);
    registry.register_function("Raylib_native_load_directory_files", wrap_load_directory_files);
    registry.register_function("raylib_Raylib_native_load_directory_files", wrap_load_directory_files);
    registry.register_function("native_load_directory_files", wrap_load_directory_files);
    registry.register_function("Raylib_native_load_directory_files_ex", wrap_load_directory_files_ex);
    registry.register_function("raylib_Raylib_native_load_directory_files_ex", wrap_load_directory_files_ex);
    registry.register_function("native_load_directory_files_ex", wrap_load_directory_files_ex);
    registry.register_function("Raylib_native_unload_directory_files", wrap_unload_directory_files);
    registry.register_function("raylib_Raylib_native_unload_directory_files", wrap_unload_directory_files);
    registry.register_function("native_unload_directory_files", wrap_unload_directory_files);
    registry.register_function("Raylib_native_is_file_dropped", wrap_is_file_dropped);
    registry.register_function("raylib_Raylib_native_is_file_dropped", wrap_is_file_dropped);
    registry.register_function("native_is_file_dropped", wrap_is_file_dropped);
    registry.register_function("Raylib_native_load_dropped_files", wrap_load_dropped_files);
    registry.register_function("raylib_Raylib_native_load_dropped_files", wrap_load_dropped_files);
    registry.register_function("native_load_dropped_files", wrap_load_dropped_files);
    registry.register_function("Raylib_native_unload_dropped_files", wrap_unload_dropped_files);
    registry.register_function("raylib_Raylib_native_unload_dropped_files", wrap_unload_dropped_files);
    registry.register_function("native_unload_dropped_files", wrap_unload_dropped_files);
    registry.register_function("Raylib_native_get_directory_file_count", wrap_get_directory_file_count);
    registry.register_function("raylib_Raylib_native_get_directory_file_count", wrap_get_directory_file_count);
    registry.register_function("native_get_directory_file_count", wrap_get_directory_file_count);
    registry.register_function("Raylib_native_get_directory_file_count_ex", wrap_get_directory_file_count_ex);
    registry.register_function("raylib_Raylib_native_get_directory_file_count_ex", wrap_get_directory_file_count_ex);
    registry.register_function("native_get_directory_file_count_ex", wrap_get_directory_file_count_ex);
    registry.register_function("Raylib_native_compress_data", wrap_compress_data);
    registry.register_function("raylib_Raylib_native_compress_data", wrap_compress_data);
    registry.register_function("native_compress_data", wrap_compress_data);
    registry.register_function("Raylib_native_decompress_data", wrap_decompress_data);
    registry.register_function("raylib_Raylib_native_decompress_data", wrap_decompress_data);
    registry.register_function("native_decompress_data", wrap_decompress_data);
    registry.register_function("Raylib_native_encode_data_base64", wrap_encode_data_base64);
    registry.register_function("raylib_Raylib_native_encode_data_base64", wrap_encode_data_base64);
    registry.register_function("native_encode_data_base64", wrap_encode_data_base64);
    registry.register_function("Raylib_native_decode_data_base64", wrap_decode_data_base64);
    registry.register_function("raylib_Raylib_native_decode_data_base64", wrap_decode_data_base64);
    registry.register_function("native_decode_data_base64", wrap_decode_data_base64);
    registry.register_function("Raylib_native_compute_crc32", wrap_compute_crc32);
    registry.register_function("raylib_Raylib_native_compute_crc32", wrap_compute_crc32);
    registry.register_function("native_compute_crc32", wrap_compute_crc32);
    registry.register_function("Raylib_native_compute_md5", wrap_compute_md5);
    registry.register_function("raylib_Raylib_native_compute_md5", wrap_compute_md5);
    registry.register_function("native_compute_md5", wrap_compute_md5);
    registry.register_function("Raylib_native_compute_sha1", wrap_compute_sha1);
    registry.register_function("raylib_Raylib_native_compute_sha1", wrap_compute_sha1);
    registry.register_function("native_compute_sha1", wrap_compute_sha1);
    registry.register_function("Raylib_native_compute_sha256", wrap_compute_sha256);
    registry.register_function("raylib_Raylib_native_compute_sha256", wrap_compute_sha256);
    registry.register_function("native_compute_sha256", wrap_compute_sha256);
    registry.register_function("Raylib_native_load_automation_event_list", wrap_load_automation_event_list);
    registry.register_function("raylib_Raylib_native_load_automation_event_list", wrap_load_automation_event_list);
    registry.register_function("native_load_automation_event_list", wrap_load_automation_event_list);
    registry.register_function("Raylib_native_unload_automation_event_list", wrap_unload_automation_event_list);
    registry.register_function("raylib_Raylib_native_unload_automation_event_list", wrap_unload_automation_event_list);
    registry.register_function("native_unload_automation_event_list", wrap_unload_automation_event_list);
    registry.register_function("Raylib_native_export_automation_event_list", wrap_export_automation_event_list);
    registry.register_function("raylib_Raylib_native_export_automation_event_list", wrap_export_automation_event_list);
    registry.register_function("native_export_automation_event_list", wrap_export_automation_event_list);
    registry.register_function("Raylib_native_set_automation_event_list", wrap_set_automation_event_list);
    registry.register_function("raylib_Raylib_native_set_automation_event_list", wrap_set_automation_event_list);
    registry.register_function("native_set_automation_event_list", wrap_set_automation_event_list);
    registry.register_function("Raylib_native_set_automation_event_base_frame", wrap_set_automation_event_base_frame);
    registry.register_function("raylib_Raylib_native_set_automation_event_base_frame", wrap_set_automation_event_base_frame);
    registry.register_function("native_set_automation_event_base_frame", wrap_set_automation_event_base_frame);
    registry.register_function("Raylib_native_start_automation_event_recording", wrap_start_automation_event_recording);
    registry.register_function("raylib_Raylib_native_start_automation_event_recording", wrap_start_automation_event_recording);
    registry.register_function("native_start_automation_event_recording", wrap_start_automation_event_recording);
    registry.register_function("Raylib_native_stop_automation_event_recording", wrap_stop_automation_event_recording);
    registry.register_function("raylib_Raylib_native_stop_automation_event_recording", wrap_stop_automation_event_recording);
    registry.register_function("native_stop_automation_event_recording", wrap_stop_automation_event_recording);
    registry.register_function("Raylib_native_play_automation_event", wrap_play_automation_event);
    registry.register_function("raylib_Raylib_native_play_automation_event", wrap_play_automation_event);
    registry.register_function("native_play_automation_event", wrap_play_automation_event);
    registry.register_function("Raylib_native_is_key_pressed", wrap_is_key_pressed);
    registry.register_function("raylib_Raylib_native_is_key_pressed", wrap_is_key_pressed);
    registry.register_function("native_is_key_pressed", wrap_is_key_pressed);
    registry.register_function("Raylib_native_is_key_pressed_repeat", wrap_is_key_pressed_repeat);
    registry.register_function("raylib_Raylib_native_is_key_pressed_repeat", wrap_is_key_pressed_repeat);
    registry.register_function("native_is_key_pressed_repeat", wrap_is_key_pressed_repeat);
    registry.register_function("Raylib_native_is_key_down", wrap_is_key_down);
    registry.register_function("raylib_Raylib_native_is_key_down", wrap_is_key_down);
    registry.register_function("native_is_key_down", wrap_is_key_down);
    registry.register_function("Raylib_native_is_key_released", wrap_is_key_released);
    registry.register_function("raylib_Raylib_native_is_key_released", wrap_is_key_released);
    registry.register_function("native_is_key_released", wrap_is_key_released);
    registry.register_function("Raylib_native_is_key_up", wrap_is_key_up);
    registry.register_function("raylib_Raylib_native_is_key_up", wrap_is_key_up);
    registry.register_function("native_is_key_up", wrap_is_key_up);
    registry.register_function("Raylib_native_get_key_pressed", wrap_get_key_pressed);
    registry.register_function("raylib_Raylib_native_get_key_pressed", wrap_get_key_pressed);
    registry.register_function("native_get_key_pressed", wrap_get_key_pressed);
    registry.register_function("Raylib_native_get_char_pressed", wrap_get_char_pressed);
    registry.register_function("raylib_Raylib_native_get_char_pressed", wrap_get_char_pressed);
    registry.register_function("native_get_char_pressed", wrap_get_char_pressed);
    registry.register_function("Raylib_native_get_key_name", wrap_get_key_name);
    registry.register_function("raylib_Raylib_native_get_key_name", wrap_get_key_name);
    registry.register_function("native_get_key_name", wrap_get_key_name);
    registry.register_function("Raylib_native_set_exit_key", wrap_set_exit_key);
    registry.register_function("raylib_Raylib_native_set_exit_key", wrap_set_exit_key);
    registry.register_function("native_set_exit_key", wrap_set_exit_key);
    registry.register_function("Raylib_native_is_gamepad_available", wrap_is_gamepad_available);
    registry.register_function("raylib_Raylib_native_is_gamepad_available", wrap_is_gamepad_available);
    registry.register_function("native_is_gamepad_available", wrap_is_gamepad_available);
    registry.register_function("Raylib_native_get_gamepad_name", wrap_get_gamepad_name);
    registry.register_function("raylib_Raylib_native_get_gamepad_name", wrap_get_gamepad_name);
    registry.register_function("native_get_gamepad_name", wrap_get_gamepad_name);
    registry.register_function("Raylib_native_is_gamepad_button_pressed", wrap_is_gamepad_button_pressed);
    registry.register_function("raylib_Raylib_native_is_gamepad_button_pressed", wrap_is_gamepad_button_pressed);
    registry.register_function("native_is_gamepad_button_pressed", wrap_is_gamepad_button_pressed);
    registry.register_function("Raylib_native_is_gamepad_button_down", wrap_is_gamepad_button_down);
    registry.register_function("raylib_Raylib_native_is_gamepad_button_down", wrap_is_gamepad_button_down);
    registry.register_function("native_is_gamepad_button_down", wrap_is_gamepad_button_down);
    registry.register_function("Raylib_native_is_gamepad_button_released", wrap_is_gamepad_button_released);
    registry.register_function("raylib_Raylib_native_is_gamepad_button_released", wrap_is_gamepad_button_released);
    registry.register_function("native_is_gamepad_button_released", wrap_is_gamepad_button_released);
    registry.register_function("Raylib_native_is_gamepad_button_up", wrap_is_gamepad_button_up);
    registry.register_function("raylib_Raylib_native_is_gamepad_button_up", wrap_is_gamepad_button_up);
    registry.register_function("native_is_gamepad_button_up", wrap_is_gamepad_button_up);
    registry.register_function("Raylib_native_get_gamepad_button_pressed", wrap_get_gamepad_button_pressed);
    registry.register_function("raylib_Raylib_native_get_gamepad_button_pressed", wrap_get_gamepad_button_pressed);
    registry.register_function("native_get_gamepad_button_pressed", wrap_get_gamepad_button_pressed);
    registry.register_function("Raylib_native_get_gamepad_axis_count", wrap_get_gamepad_axis_count);
    registry.register_function("raylib_Raylib_native_get_gamepad_axis_count", wrap_get_gamepad_axis_count);
    registry.register_function("native_get_gamepad_axis_count", wrap_get_gamepad_axis_count);
    registry.register_function("Raylib_native_get_gamepad_axis_movement", wrap_get_gamepad_axis_movement);
    registry.register_function("raylib_Raylib_native_get_gamepad_axis_movement", wrap_get_gamepad_axis_movement);
    registry.register_function("native_get_gamepad_axis_movement", wrap_get_gamepad_axis_movement);
    registry.register_function("Raylib_native_set_gamepad_mappings", wrap_set_gamepad_mappings);
    registry.register_function("raylib_Raylib_native_set_gamepad_mappings", wrap_set_gamepad_mappings);
    registry.register_function("native_set_gamepad_mappings", wrap_set_gamepad_mappings);
    registry.register_function("Raylib_native_set_gamepad_vibration", wrap_set_gamepad_vibration);
    registry.register_function("raylib_Raylib_native_set_gamepad_vibration", wrap_set_gamepad_vibration);
    registry.register_function("native_set_gamepad_vibration", wrap_set_gamepad_vibration);
    registry.register_function("Raylib_native_is_mouse_button_pressed", wrap_is_mouse_button_pressed);
    registry.register_function("raylib_Raylib_native_is_mouse_button_pressed", wrap_is_mouse_button_pressed);
    registry.register_function("native_is_mouse_button_pressed", wrap_is_mouse_button_pressed);
    registry.register_function("Raylib_native_is_mouse_button_down", wrap_is_mouse_button_down);
    registry.register_function("raylib_Raylib_native_is_mouse_button_down", wrap_is_mouse_button_down);
    registry.register_function("native_is_mouse_button_down", wrap_is_mouse_button_down);
    registry.register_function("Raylib_native_is_mouse_button_released", wrap_is_mouse_button_released);
    registry.register_function("raylib_Raylib_native_is_mouse_button_released", wrap_is_mouse_button_released);
    registry.register_function("native_is_mouse_button_released", wrap_is_mouse_button_released);
    registry.register_function("Raylib_native_is_mouse_button_up", wrap_is_mouse_button_up);
    registry.register_function("raylib_Raylib_native_is_mouse_button_up", wrap_is_mouse_button_up);
    registry.register_function("native_is_mouse_button_up", wrap_is_mouse_button_up);
    registry.register_function("Raylib_native_get_mouse_x", wrap_get_mouse_x);
    registry.register_function("raylib_Raylib_native_get_mouse_x", wrap_get_mouse_x);
    registry.register_function("native_get_mouse_x", wrap_get_mouse_x);
    registry.register_function("Raylib_native_get_mouse_y", wrap_get_mouse_y);
    registry.register_function("raylib_Raylib_native_get_mouse_y", wrap_get_mouse_y);
    registry.register_function("native_get_mouse_y", wrap_get_mouse_y);
    registry.register_function("Raylib_native_get_mouse_position", wrap_get_mouse_position);
    registry.register_function("raylib_Raylib_native_get_mouse_position", wrap_get_mouse_position);
    registry.register_function("native_get_mouse_position", wrap_get_mouse_position);
    registry.register_function("Raylib_native_get_mouse_delta", wrap_get_mouse_delta);
    registry.register_function("raylib_Raylib_native_get_mouse_delta", wrap_get_mouse_delta);
    registry.register_function("native_get_mouse_delta", wrap_get_mouse_delta);
    registry.register_function("Raylib_native_set_mouse_position", wrap_set_mouse_position);
    registry.register_function("raylib_Raylib_native_set_mouse_position", wrap_set_mouse_position);
    registry.register_function("native_set_mouse_position", wrap_set_mouse_position);
    registry.register_function("Raylib_native_set_mouse_offset", wrap_set_mouse_offset);
    registry.register_function("raylib_Raylib_native_set_mouse_offset", wrap_set_mouse_offset);
    registry.register_function("native_set_mouse_offset", wrap_set_mouse_offset);
    registry.register_function("Raylib_native_set_mouse_scale", wrap_set_mouse_scale);
    registry.register_function("raylib_Raylib_native_set_mouse_scale", wrap_set_mouse_scale);
    registry.register_function("native_set_mouse_scale", wrap_set_mouse_scale);
    registry.register_function("Raylib_native_get_mouse_wheel_move", wrap_get_mouse_wheel_move);
    registry.register_function("raylib_Raylib_native_get_mouse_wheel_move", wrap_get_mouse_wheel_move);
    registry.register_function("native_get_mouse_wheel_move", wrap_get_mouse_wheel_move);
    registry.register_function("Raylib_native_get_mouse_wheel_move_v", wrap_get_mouse_wheel_move_v);
    registry.register_function("raylib_Raylib_native_get_mouse_wheel_move_v", wrap_get_mouse_wheel_move_v);
    registry.register_function("native_get_mouse_wheel_move_v", wrap_get_mouse_wheel_move_v);
    registry.register_function("Raylib_native_set_mouse_cursor", wrap_set_mouse_cursor);
    registry.register_function("raylib_Raylib_native_set_mouse_cursor", wrap_set_mouse_cursor);
    registry.register_function("native_set_mouse_cursor", wrap_set_mouse_cursor);
    registry.register_function("Raylib_native_get_touch_x", wrap_get_touch_x);
    registry.register_function("raylib_Raylib_native_get_touch_x", wrap_get_touch_x);
    registry.register_function("native_get_touch_x", wrap_get_touch_x);
    registry.register_function("Raylib_native_get_touch_y", wrap_get_touch_y);
    registry.register_function("raylib_Raylib_native_get_touch_y", wrap_get_touch_y);
    registry.register_function("native_get_touch_y", wrap_get_touch_y);
    registry.register_function("Raylib_native_get_touch_position", wrap_get_touch_position);
    registry.register_function("raylib_Raylib_native_get_touch_position", wrap_get_touch_position);
    registry.register_function("native_get_touch_position", wrap_get_touch_position);
    registry.register_function("Raylib_native_get_touch_point_id", wrap_get_touch_point_id);
    registry.register_function("raylib_Raylib_native_get_touch_point_id", wrap_get_touch_point_id);
    registry.register_function("native_get_touch_point_id", wrap_get_touch_point_id);
    registry.register_function("Raylib_native_get_touch_point_count", wrap_get_touch_point_count);
    registry.register_function("raylib_Raylib_native_get_touch_point_count", wrap_get_touch_point_count);
    registry.register_function("native_get_touch_point_count", wrap_get_touch_point_count);
    registry.register_function("Raylib_native_set_gestures_enabled", wrap_set_gestures_enabled);
    registry.register_function("raylib_Raylib_native_set_gestures_enabled", wrap_set_gestures_enabled);
    registry.register_function("native_set_gestures_enabled", wrap_set_gestures_enabled);
    registry.register_function("Raylib_native_is_gesture_detected", wrap_is_gesture_detected);
    registry.register_function("raylib_Raylib_native_is_gesture_detected", wrap_is_gesture_detected);
    registry.register_function("native_is_gesture_detected", wrap_is_gesture_detected);
    registry.register_function("Raylib_native_get_gesture_detected", wrap_get_gesture_detected);
    registry.register_function("raylib_Raylib_native_get_gesture_detected", wrap_get_gesture_detected);
    registry.register_function("native_get_gesture_detected", wrap_get_gesture_detected);
    registry.register_function("Raylib_native_get_gesture_hold_duration", wrap_get_gesture_hold_duration);
    registry.register_function("raylib_Raylib_native_get_gesture_hold_duration", wrap_get_gesture_hold_duration);
    registry.register_function("native_get_gesture_hold_duration", wrap_get_gesture_hold_duration);
    registry.register_function("Raylib_native_get_gesture_drag_vector", wrap_get_gesture_drag_vector);
    registry.register_function("raylib_Raylib_native_get_gesture_drag_vector", wrap_get_gesture_drag_vector);
    registry.register_function("native_get_gesture_drag_vector", wrap_get_gesture_drag_vector);
    registry.register_function("Raylib_native_get_gesture_drag_angle", wrap_get_gesture_drag_angle);
    registry.register_function("raylib_Raylib_native_get_gesture_drag_angle", wrap_get_gesture_drag_angle);
    registry.register_function("native_get_gesture_drag_angle", wrap_get_gesture_drag_angle);
    registry.register_function("Raylib_native_get_gesture_pinch_vector", wrap_get_gesture_pinch_vector);
    registry.register_function("raylib_Raylib_native_get_gesture_pinch_vector", wrap_get_gesture_pinch_vector);
    registry.register_function("native_get_gesture_pinch_vector", wrap_get_gesture_pinch_vector);
    registry.register_function("Raylib_native_get_gesture_pinch_angle", wrap_get_gesture_pinch_angle);
    registry.register_function("raylib_Raylib_native_get_gesture_pinch_angle", wrap_get_gesture_pinch_angle);
    registry.register_function("native_get_gesture_pinch_angle", wrap_get_gesture_pinch_angle);
    registry.register_function("Raylib_native_update_camera", wrap_update_camera);
    registry.register_function("raylib_Raylib_native_update_camera", wrap_update_camera);
    registry.register_function("native_update_camera", wrap_update_camera);
    registry.register_function("Raylib_native_update_camera_pro", wrap_update_camera_pro);
    registry.register_function("raylib_Raylib_native_update_camera_pro", wrap_update_camera_pro);
    registry.register_function("native_update_camera_pro", wrap_update_camera_pro);
    registry.register_function("Raylib_native_set_shapes_texture", wrap_set_shapes_texture);
    registry.register_function("raylib_Raylib_native_set_shapes_texture", wrap_set_shapes_texture);
    registry.register_function("native_set_shapes_texture", wrap_set_shapes_texture);
    registry.register_function("Raylib_native_get_shapes_texture", wrap_get_shapes_texture);
    registry.register_function("raylib_Raylib_native_get_shapes_texture", wrap_get_shapes_texture);
    registry.register_function("native_get_shapes_texture", wrap_get_shapes_texture);
    registry.register_function("Raylib_native_get_shapes_texture_rectangle", wrap_get_shapes_texture_rectangle);
    registry.register_function("raylib_Raylib_native_get_shapes_texture_rectangle", wrap_get_shapes_texture_rectangle);
    registry.register_function("native_get_shapes_texture_rectangle", wrap_get_shapes_texture_rectangle);
    registry.register_function("Raylib_native_draw_pixel", wrap_draw_pixel);
    registry.register_function("raylib_Raylib_native_draw_pixel", wrap_draw_pixel);
    registry.register_function("native_draw_pixel", wrap_draw_pixel);
    registry.register_function("Raylib_native_draw_pixel_v", wrap_draw_pixel_v);
    registry.register_function("raylib_Raylib_native_draw_pixel_v", wrap_draw_pixel_v);
    registry.register_function("native_draw_pixel_v", wrap_draw_pixel_v);
    registry.register_function("Raylib_native_draw_line", wrap_draw_line);
    registry.register_function("raylib_Raylib_native_draw_line", wrap_draw_line);
    registry.register_function("native_draw_line", wrap_draw_line);
    registry.register_function("Raylib_native_draw_line_v", wrap_draw_line_v);
    registry.register_function("raylib_Raylib_native_draw_line_v", wrap_draw_line_v);
    registry.register_function("native_draw_line_v", wrap_draw_line_v);
    registry.register_function("Raylib_native_draw_line_ex", wrap_draw_line_ex);
    registry.register_function("raylib_Raylib_native_draw_line_ex", wrap_draw_line_ex);
    registry.register_function("native_draw_line_ex", wrap_draw_line_ex);
    registry.register_function("Raylib_native_draw_line_strip", wrap_draw_line_strip);
    registry.register_function("raylib_Raylib_native_draw_line_strip", wrap_draw_line_strip);
    registry.register_function("native_draw_line_strip", wrap_draw_line_strip);
    registry.register_function("Raylib_native_draw_line_bezier", wrap_draw_line_bezier);
    registry.register_function("raylib_Raylib_native_draw_line_bezier", wrap_draw_line_bezier);
    registry.register_function("native_draw_line_bezier", wrap_draw_line_bezier);
    registry.register_function("Raylib_native_draw_line_dashed", wrap_draw_line_dashed);
    registry.register_function("raylib_Raylib_native_draw_line_dashed", wrap_draw_line_dashed);
    registry.register_function("native_draw_line_dashed", wrap_draw_line_dashed);
    registry.register_function("Raylib_native_draw_triangle", wrap_draw_triangle);
    registry.register_function("raylib_Raylib_native_draw_triangle", wrap_draw_triangle);
    registry.register_function("native_draw_triangle", wrap_draw_triangle);
    registry.register_function("Raylib_native_draw_triangle_gradient", wrap_draw_triangle_gradient);
    registry.register_function("raylib_Raylib_native_draw_triangle_gradient", wrap_draw_triangle_gradient);
    registry.register_function("native_draw_triangle_gradient", wrap_draw_triangle_gradient);
    registry.register_function("Raylib_native_draw_triangle_lines", wrap_draw_triangle_lines);
    registry.register_function("raylib_Raylib_native_draw_triangle_lines", wrap_draw_triangle_lines);
    registry.register_function("native_draw_triangle_lines", wrap_draw_triangle_lines);
    registry.register_function("Raylib_native_draw_triangle_lines_ex", wrap_draw_triangle_lines_ex);
    registry.register_function("raylib_Raylib_native_draw_triangle_lines_ex", wrap_draw_triangle_lines_ex);
    registry.register_function("native_draw_triangle_lines_ex", wrap_draw_triangle_lines_ex);
    registry.register_function("Raylib_native_draw_triangle_fan", wrap_draw_triangle_fan);
    registry.register_function("raylib_Raylib_native_draw_triangle_fan", wrap_draw_triangle_fan);
    registry.register_function("native_draw_triangle_fan", wrap_draw_triangle_fan);
    registry.register_function("Raylib_native_draw_triangle_strip", wrap_draw_triangle_strip);
    registry.register_function("raylib_Raylib_native_draw_triangle_strip", wrap_draw_triangle_strip);
    registry.register_function("native_draw_triangle_strip", wrap_draw_triangle_strip);
    registry.register_function("Raylib_native_draw_rectangle", wrap_draw_rectangle);
    registry.register_function("raylib_Raylib_native_draw_rectangle", wrap_draw_rectangle);
    registry.register_function("native_draw_rectangle", wrap_draw_rectangle);
    registry.register_function("Raylib_native_draw_rectangle_v", wrap_draw_rectangle_v);
    registry.register_function("raylib_Raylib_native_draw_rectangle_v", wrap_draw_rectangle_v);
    registry.register_function("native_draw_rectangle_v", wrap_draw_rectangle_v);
    registry.register_function("Raylib_native_draw_rectangle_rec", wrap_draw_rectangle_rec);
    registry.register_function("raylib_Raylib_native_draw_rectangle_rec", wrap_draw_rectangle_rec);
    registry.register_function("native_draw_rectangle_rec", wrap_draw_rectangle_rec);
    registry.register_function("Raylib_native_draw_rectangle_pro", wrap_draw_rectangle_pro);
    registry.register_function("raylib_Raylib_native_draw_rectangle_pro", wrap_draw_rectangle_pro);
    registry.register_function("native_draw_rectangle_pro", wrap_draw_rectangle_pro);
    registry.register_function("Raylib_native_draw_rectangle_gradient_v", wrap_draw_rectangle_gradient_v);
    registry.register_function("raylib_Raylib_native_draw_rectangle_gradient_v", wrap_draw_rectangle_gradient_v);
    registry.register_function("native_draw_rectangle_gradient_v", wrap_draw_rectangle_gradient_v);
    registry.register_function("Raylib_native_draw_rectangle_gradient_h", wrap_draw_rectangle_gradient_h);
    registry.register_function("raylib_Raylib_native_draw_rectangle_gradient_h", wrap_draw_rectangle_gradient_h);
    registry.register_function("native_draw_rectangle_gradient_h", wrap_draw_rectangle_gradient_h);
    registry.register_function("Raylib_native_draw_rectangle_gradient_ex", wrap_draw_rectangle_gradient_ex);
    registry.register_function("raylib_Raylib_native_draw_rectangle_gradient_ex", wrap_draw_rectangle_gradient_ex);
    registry.register_function("native_draw_rectangle_gradient_ex", wrap_draw_rectangle_gradient_ex);
    registry.register_function("Raylib_native_draw_rectangle_lines", wrap_draw_rectangle_lines);
    registry.register_function("raylib_Raylib_native_draw_rectangle_lines", wrap_draw_rectangle_lines);
    registry.register_function("native_draw_rectangle_lines", wrap_draw_rectangle_lines);
    registry.register_function("Raylib_native_draw_rectangle_lines_ex", wrap_draw_rectangle_lines_ex);
    registry.register_function("raylib_Raylib_native_draw_rectangle_lines_ex", wrap_draw_rectangle_lines_ex);
    registry.register_function("native_draw_rectangle_lines_ex", wrap_draw_rectangle_lines_ex);
    registry.register_function("Raylib_native_draw_rectangle_rounded", wrap_draw_rectangle_rounded);
    registry.register_function("raylib_Raylib_native_draw_rectangle_rounded", wrap_draw_rectangle_rounded);
    registry.register_function("native_draw_rectangle_rounded", wrap_draw_rectangle_rounded);
    registry.register_function("Raylib_native_draw_rectangle_rounded_lines", wrap_draw_rectangle_rounded_lines);
    registry.register_function("raylib_Raylib_native_draw_rectangle_rounded_lines", wrap_draw_rectangle_rounded_lines);
    registry.register_function("native_draw_rectangle_rounded_lines", wrap_draw_rectangle_rounded_lines);
    registry.register_function("Raylib_native_draw_rectangle_rounded_lines_ex", wrap_draw_rectangle_rounded_lines_ex);
    registry.register_function("raylib_Raylib_native_draw_rectangle_rounded_lines_ex", wrap_draw_rectangle_rounded_lines_ex);
    registry.register_function("native_draw_rectangle_rounded_lines_ex", wrap_draw_rectangle_rounded_lines_ex);
    registry.register_function("Raylib_native_draw_poly", wrap_draw_poly);
    registry.register_function("raylib_Raylib_native_draw_poly", wrap_draw_poly);
    registry.register_function("native_draw_poly", wrap_draw_poly);
    registry.register_function("Raylib_native_draw_poly_lines", wrap_draw_poly_lines);
    registry.register_function("raylib_Raylib_native_draw_poly_lines", wrap_draw_poly_lines);
    registry.register_function("native_draw_poly_lines", wrap_draw_poly_lines);
    registry.register_function("Raylib_native_draw_poly_lines_ex", wrap_draw_poly_lines_ex);
    registry.register_function("raylib_Raylib_native_draw_poly_lines_ex", wrap_draw_poly_lines_ex);
    registry.register_function("native_draw_poly_lines_ex", wrap_draw_poly_lines_ex);
    registry.register_function("Raylib_native_draw_circle", wrap_draw_circle);
    registry.register_function("raylib_Raylib_native_draw_circle", wrap_draw_circle);
    registry.register_function("native_draw_circle", wrap_draw_circle);
    registry.register_function("Raylib_native_draw_circle_v", wrap_draw_circle_v);
    registry.register_function("raylib_Raylib_native_draw_circle_v", wrap_draw_circle_v);
    registry.register_function("native_draw_circle_v", wrap_draw_circle_v);
    registry.register_function("Raylib_native_draw_circle_gradient", wrap_draw_circle_gradient);
    registry.register_function("raylib_Raylib_native_draw_circle_gradient", wrap_draw_circle_gradient);
    registry.register_function("native_draw_circle_gradient", wrap_draw_circle_gradient);
    registry.register_function("Raylib_native_draw_circle_sector", wrap_draw_circle_sector);
    registry.register_function("raylib_Raylib_native_draw_circle_sector", wrap_draw_circle_sector);
    registry.register_function("native_draw_circle_sector", wrap_draw_circle_sector);
    registry.register_function("Raylib_native_draw_circle_sector_lines", wrap_draw_circle_sector_lines);
    registry.register_function("raylib_Raylib_native_draw_circle_sector_lines", wrap_draw_circle_sector_lines);
    registry.register_function("native_draw_circle_sector_lines", wrap_draw_circle_sector_lines);
    registry.register_function("Raylib_native_draw_circle_sector_lines_ex", wrap_draw_circle_sector_lines_ex);
    registry.register_function("raylib_Raylib_native_draw_circle_sector_lines_ex", wrap_draw_circle_sector_lines_ex);
    registry.register_function("native_draw_circle_sector_lines_ex", wrap_draw_circle_sector_lines_ex);
    registry.register_function("Raylib_native_draw_circle_lines", wrap_draw_circle_lines);
    registry.register_function("raylib_Raylib_native_draw_circle_lines", wrap_draw_circle_lines);
    registry.register_function("native_draw_circle_lines", wrap_draw_circle_lines);
    registry.register_function("Raylib_native_draw_circle_lines_v", wrap_draw_circle_lines_v);
    registry.register_function("raylib_Raylib_native_draw_circle_lines_v", wrap_draw_circle_lines_v);
    registry.register_function("native_draw_circle_lines_v", wrap_draw_circle_lines_v);
    registry.register_function("Raylib_native_draw_circle_lines_ex", wrap_draw_circle_lines_ex);
    registry.register_function("raylib_Raylib_native_draw_circle_lines_ex", wrap_draw_circle_lines_ex);
    registry.register_function("native_draw_circle_lines_ex", wrap_draw_circle_lines_ex);
    registry.register_function("Raylib_native_draw_ellipse", wrap_draw_ellipse);
    registry.register_function("raylib_Raylib_native_draw_ellipse", wrap_draw_ellipse);
    registry.register_function("native_draw_ellipse", wrap_draw_ellipse);
    registry.register_function("Raylib_native_draw_ellipse_v", wrap_draw_ellipse_v);
    registry.register_function("raylib_Raylib_native_draw_ellipse_v", wrap_draw_ellipse_v);
    registry.register_function("native_draw_ellipse_v", wrap_draw_ellipse_v);
    registry.register_function("Raylib_native_draw_ellipse_lines", wrap_draw_ellipse_lines);
    registry.register_function("raylib_Raylib_native_draw_ellipse_lines", wrap_draw_ellipse_lines);
    registry.register_function("native_draw_ellipse_lines", wrap_draw_ellipse_lines);
    registry.register_function("Raylib_native_draw_ellipse_lines_v", wrap_draw_ellipse_lines_v);
    registry.register_function("raylib_Raylib_native_draw_ellipse_lines_v", wrap_draw_ellipse_lines_v);
    registry.register_function("native_draw_ellipse_lines_v", wrap_draw_ellipse_lines_v);
    registry.register_function("Raylib_native_draw_ellipse_lines_ex", wrap_draw_ellipse_lines_ex);
    registry.register_function("raylib_Raylib_native_draw_ellipse_lines_ex", wrap_draw_ellipse_lines_ex);
    registry.register_function("native_draw_ellipse_lines_ex", wrap_draw_ellipse_lines_ex);
    registry.register_function("Raylib_native_draw_ring", wrap_draw_ring);
    registry.register_function("raylib_Raylib_native_draw_ring", wrap_draw_ring);
    registry.register_function("native_draw_ring", wrap_draw_ring);
    registry.register_function("Raylib_native_draw_ring_lines", wrap_draw_ring_lines);
    registry.register_function("raylib_Raylib_native_draw_ring_lines", wrap_draw_ring_lines);
    registry.register_function("native_draw_ring_lines", wrap_draw_ring_lines);
    registry.register_function("Raylib_native_draw_ring_lines_ex", wrap_draw_ring_lines_ex);
    registry.register_function("raylib_Raylib_native_draw_ring_lines_ex", wrap_draw_ring_lines_ex);
    registry.register_function("native_draw_ring_lines_ex", wrap_draw_ring_lines_ex);
    registry.register_function("Raylib_native_draw_spline_linear", wrap_draw_spline_linear);
    registry.register_function("raylib_Raylib_native_draw_spline_linear", wrap_draw_spline_linear);
    registry.register_function("native_draw_spline_linear", wrap_draw_spline_linear);
    registry.register_function("Raylib_native_draw_spline_basis", wrap_draw_spline_basis);
    registry.register_function("raylib_Raylib_native_draw_spline_basis", wrap_draw_spline_basis);
    registry.register_function("native_draw_spline_basis", wrap_draw_spline_basis);
    registry.register_function("Raylib_native_draw_spline_catmull_rom", wrap_draw_spline_catmull_rom);
    registry.register_function("raylib_Raylib_native_draw_spline_catmull_rom", wrap_draw_spline_catmull_rom);
    registry.register_function("native_draw_spline_catmull_rom", wrap_draw_spline_catmull_rom);
    registry.register_function("Raylib_native_draw_spline_bezier_quadratic", wrap_draw_spline_bezier_quadratic);
    registry.register_function("raylib_Raylib_native_draw_spline_bezier_quadratic", wrap_draw_spline_bezier_quadratic);
    registry.register_function("native_draw_spline_bezier_quadratic", wrap_draw_spline_bezier_quadratic);
    registry.register_function("Raylib_native_draw_spline_bezier_cubic", wrap_draw_spline_bezier_cubic);
    registry.register_function("raylib_Raylib_native_draw_spline_bezier_cubic", wrap_draw_spline_bezier_cubic);
    registry.register_function("native_draw_spline_bezier_cubic", wrap_draw_spline_bezier_cubic);
    registry.register_function("Raylib_native_draw_spline_segment_linear", wrap_draw_spline_segment_linear);
    registry.register_function("raylib_Raylib_native_draw_spline_segment_linear", wrap_draw_spline_segment_linear);
    registry.register_function("native_draw_spline_segment_linear", wrap_draw_spline_segment_linear);
    registry.register_function("Raylib_native_draw_spline_segment_basis", wrap_draw_spline_segment_basis);
    registry.register_function("raylib_Raylib_native_draw_spline_segment_basis", wrap_draw_spline_segment_basis);
    registry.register_function("native_draw_spline_segment_basis", wrap_draw_spline_segment_basis);
    registry.register_function("Raylib_native_draw_spline_segment_catmull_rom", wrap_draw_spline_segment_catmull_rom);
    registry.register_function("raylib_Raylib_native_draw_spline_segment_catmull_rom", wrap_draw_spline_segment_catmull_rom);
    registry.register_function("native_draw_spline_segment_catmull_rom", wrap_draw_spline_segment_catmull_rom);
    registry.register_function("Raylib_native_draw_spline_segment_bezier_quadratic", wrap_draw_spline_segment_bezier_quadratic);
    registry.register_function("raylib_Raylib_native_draw_spline_segment_bezier_quadratic", wrap_draw_spline_segment_bezier_quadratic);
    registry.register_function("native_draw_spline_segment_bezier_quadratic", wrap_draw_spline_segment_bezier_quadratic);
    registry.register_function("Raylib_native_draw_spline_segment_bezier_cubic", wrap_draw_spline_segment_bezier_cubic);
    registry.register_function("raylib_Raylib_native_draw_spline_segment_bezier_cubic", wrap_draw_spline_segment_bezier_cubic);
    registry.register_function("native_draw_spline_segment_bezier_cubic", wrap_draw_spline_segment_bezier_cubic);
    registry.register_function("Raylib_native_get_spline_point_linear", wrap_get_spline_point_linear);
    registry.register_function("raylib_Raylib_native_get_spline_point_linear", wrap_get_spline_point_linear);
    registry.register_function("native_get_spline_point_linear", wrap_get_spline_point_linear);
    registry.register_function("Raylib_native_get_spline_point_basis", wrap_get_spline_point_basis);
    registry.register_function("raylib_Raylib_native_get_spline_point_basis", wrap_get_spline_point_basis);
    registry.register_function("native_get_spline_point_basis", wrap_get_spline_point_basis);
    registry.register_function("Raylib_native_get_spline_point_catmull_rom", wrap_get_spline_point_catmull_rom);
    registry.register_function("raylib_Raylib_native_get_spline_point_catmull_rom", wrap_get_spline_point_catmull_rom);
    registry.register_function("native_get_spline_point_catmull_rom", wrap_get_spline_point_catmull_rom);
    registry.register_function("Raylib_native_get_spline_point_bezier_quadratic", wrap_get_spline_point_bezier_quadratic);
    registry.register_function("raylib_Raylib_native_get_spline_point_bezier_quadratic", wrap_get_spline_point_bezier_quadratic);
    registry.register_function("native_get_spline_point_bezier_quadratic", wrap_get_spline_point_bezier_quadratic);
    registry.register_function("Raylib_native_get_spline_point_bezier_cubic", wrap_get_spline_point_bezier_cubic);
    registry.register_function("raylib_Raylib_native_get_spline_point_bezier_cubic", wrap_get_spline_point_bezier_cubic);
    registry.register_function("native_get_spline_point_bezier_cubic", wrap_get_spline_point_bezier_cubic);
    registry.register_function("Raylib_native_check_collision_recs", wrap_check_collision_recs);
    registry.register_function("raylib_Raylib_native_check_collision_recs", wrap_check_collision_recs);
    registry.register_function("native_check_collision_recs", wrap_check_collision_recs);
    registry.register_function("Raylib_native_check_collision_circles", wrap_check_collision_circles);
    registry.register_function("raylib_Raylib_native_check_collision_circles", wrap_check_collision_circles);
    registry.register_function("native_check_collision_circles", wrap_check_collision_circles);
    registry.register_function("Raylib_native_check_collision_circle_rec", wrap_check_collision_circle_rec);
    registry.register_function("raylib_Raylib_native_check_collision_circle_rec", wrap_check_collision_circle_rec);
    registry.register_function("native_check_collision_circle_rec", wrap_check_collision_circle_rec);
    registry.register_function("Raylib_native_check_collision_circle_line", wrap_check_collision_circle_line);
    registry.register_function("raylib_Raylib_native_check_collision_circle_line", wrap_check_collision_circle_line);
    registry.register_function("native_check_collision_circle_line", wrap_check_collision_circle_line);
    registry.register_function("Raylib_native_check_collision_point_rec", wrap_check_collision_point_rec);
    registry.register_function("raylib_Raylib_native_check_collision_point_rec", wrap_check_collision_point_rec);
    registry.register_function("native_check_collision_point_rec", wrap_check_collision_point_rec);
    registry.register_function("Raylib_native_check_collision_point_circle", wrap_check_collision_point_circle);
    registry.register_function("raylib_Raylib_native_check_collision_point_circle", wrap_check_collision_point_circle);
    registry.register_function("native_check_collision_point_circle", wrap_check_collision_point_circle);
    registry.register_function("Raylib_native_check_collision_point_triangle", wrap_check_collision_point_triangle);
    registry.register_function("raylib_Raylib_native_check_collision_point_triangle", wrap_check_collision_point_triangle);
    registry.register_function("native_check_collision_point_triangle", wrap_check_collision_point_triangle);
    registry.register_function("Raylib_native_check_collision_point_line", wrap_check_collision_point_line);
    registry.register_function("raylib_Raylib_native_check_collision_point_line", wrap_check_collision_point_line);
    registry.register_function("native_check_collision_point_line", wrap_check_collision_point_line);
    registry.register_function("Raylib_native_check_collision_point_poly", wrap_check_collision_point_poly);
    registry.register_function("raylib_Raylib_native_check_collision_point_poly", wrap_check_collision_point_poly);
    registry.register_function("native_check_collision_point_poly", wrap_check_collision_point_poly);
    registry.register_function("Raylib_native_check_collision_lines", wrap_check_collision_lines);
    registry.register_function("raylib_Raylib_native_check_collision_lines", wrap_check_collision_lines);
    registry.register_function("native_check_collision_lines", wrap_check_collision_lines);
    registry.register_function("Raylib_native_get_collision_rec", wrap_get_collision_rec);
    registry.register_function("raylib_Raylib_native_get_collision_rec", wrap_get_collision_rec);
    registry.register_function("native_get_collision_rec", wrap_get_collision_rec);
    registry.register_function("Raylib_native_load_image", wrap_load_image);
    registry.register_function("raylib_Raylib_native_load_image", wrap_load_image);
    registry.register_function("native_load_image", wrap_load_image);
    registry.register_function("Raylib_native_load_image_raw", wrap_load_image_raw);
    registry.register_function("raylib_Raylib_native_load_image_raw", wrap_load_image_raw);
    registry.register_function("native_load_image_raw", wrap_load_image_raw);
    registry.register_function("Raylib_native_load_image_anim", wrap_load_image_anim);
    registry.register_function("raylib_Raylib_native_load_image_anim", wrap_load_image_anim);
    registry.register_function("native_load_image_anim", wrap_load_image_anim);
    registry.register_function("Raylib_native_load_image_anim_from_memory", wrap_load_image_anim_from_memory);
    registry.register_function("raylib_Raylib_native_load_image_anim_from_memory", wrap_load_image_anim_from_memory);
    registry.register_function("native_load_image_anim_from_memory", wrap_load_image_anim_from_memory);
    registry.register_function("Raylib_native_load_image_from_memory", wrap_load_image_from_memory);
    registry.register_function("raylib_Raylib_native_load_image_from_memory", wrap_load_image_from_memory);
    registry.register_function("native_load_image_from_memory", wrap_load_image_from_memory);
    registry.register_function("Raylib_native_load_image_from_texture", wrap_load_image_from_texture);
    registry.register_function("raylib_Raylib_native_load_image_from_texture", wrap_load_image_from_texture);
    registry.register_function("native_load_image_from_texture", wrap_load_image_from_texture);
    registry.register_function("Raylib_native_load_image_from_screen", wrap_load_image_from_screen);
    registry.register_function("raylib_Raylib_native_load_image_from_screen", wrap_load_image_from_screen);
    registry.register_function("native_load_image_from_screen", wrap_load_image_from_screen);
    registry.register_function("Raylib_native_is_image_valid", wrap_is_image_valid);
    registry.register_function("raylib_Raylib_native_is_image_valid", wrap_is_image_valid);
    registry.register_function("native_is_image_valid", wrap_is_image_valid);
    registry.register_function("Raylib_native_unload_image", wrap_unload_image);
    registry.register_function("raylib_Raylib_native_unload_image", wrap_unload_image);
    registry.register_function("native_unload_image", wrap_unload_image);
    registry.register_function("Raylib_native_export_image", wrap_export_image);
    registry.register_function("raylib_Raylib_native_export_image", wrap_export_image);
    registry.register_function("native_export_image", wrap_export_image);
    registry.register_function("Raylib_native_export_image_to_memory", wrap_export_image_to_memory);
    registry.register_function("raylib_Raylib_native_export_image_to_memory", wrap_export_image_to_memory);
    registry.register_function("native_export_image_to_memory", wrap_export_image_to_memory);
    registry.register_function("Raylib_native_export_image_as_code", wrap_export_image_as_code);
    registry.register_function("raylib_Raylib_native_export_image_as_code", wrap_export_image_as_code);
    registry.register_function("native_export_image_as_code", wrap_export_image_as_code);
    registry.register_function("Raylib_native_gen_image_color", wrap_gen_image_color);
    registry.register_function("raylib_Raylib_native_gen_image_color", wrap_gen_image_color);
    registry.register_function("native_gen_image_color", wrap_gen_image_color);
    registry.register_function("Raylib_native_gen_image_gradient_linear", wrap_gen_image_gradient_linear);
    registry.register_function("raylib_Raylib_native_gen_image_gradient_linear", wrap_gen_image_gradient_linear);
    registry.register_function("native_gen_image_gradient_linear", wrap_gen_image_gradient_linear);
    registry.register_function("Raylib_native_gen_image_gradient_radial", wrap_gen_image_gradient_radial);
    registry.register_function("raylib_Raylib_native_gen_image_gradient_radial", wrap_gen_image_gradient_radial);
    registry.register_function("native_gen_image_gradient_radial", wrap_gen_image_gradient_radial);
    registry.register_function("Raylib_native_gen_image_gradient_square", wrap_gen_image_gradient_square);
    registry.register_function("raylib_Raylib_native_gen_image_gradient_square", wrap_gen_image_gradient_square);
    registry.register_function("native_gen_image_gradient_square", wrap_gen_image_gradient_square);
    registry.register_function("Raylib_native_gen_image_checked", wrap_gen_image_checked);
    registry.register_function("raylib_Raylib_native_gen_image_checked", wrap_gen_image_checked);
    registry.register_function("native_gen_image_checked", wrap_gen_image_checked);
    registry.register_function("Raylib_native_gen_image_white_noise", wrap_gen_image_white_noise);
    registry.register_function("raylib_Raylib_native_gen_image_white_noise", wrap_gen_image_white_noise);
    registry.register_function("native_gen_image_white_noise", wrap_gen_image_white_noise);
    registry.register_function("Raylib_native_gen_image_perlin_noise", wrap_gen_image_perlin_noise);
    registry.register_function("raylib_Raylib_native_gen_image_perlin_noise", wrap_gen_image_perlin_noise);
    registry.register_function("native_gen_image_perlin_noise", wrap_gen_image_perlin_noise);
    registry.register_function("Raylib_native_gen_image_cellular", wrap_gen_image_cellular);
    registry.register_function("raylib_Raylib_native_gen_image_cellular", wrap_gen_image_cellular);
    registry.register_function("native_gen_image_cellular", wrap_gen_image_cellular);
    registry.register_function("Raylib_native_gen_image_text", wrap_gen_image_text);
    registry.register_function("raylib_Raylib_native_gen_image_text", wrap_gen_image_text);
    registry.register_function("native_gen_image_text", wrap_gen_image_text);
    registry.register_function("Raylib_native_image_copy", wrap_image_copy);
    registry.register_function("raylib_Raylib_native_image_copy", wrap_image_copy);
    registry.register_function("native_image_copy", wrap_image_copy);
    registry.register_function("Raylib_native_image_from_image", wrap_image_from_image);
    registry.register_function("raylib_Raylib_native_image_from_image", wrap_image_from_image);
    registry.register_function("native_image_from_image", wrap_image_from_image);
    registry.register_function("Raylib_native_image_from_channel", wrap_image_from_channel);
    registry.register_function("raylib_Raylib_native_image_from_channel", wrap_image_from_channel);
    registry.register_function("native_image_from_channel", wrap_image_from_channel);
    registry.register_function("Raylib_native_image_text", wrap_image_text);
    registry.register_function("raylib_Raylib_native_image_text", wrap_image_text);
    registry.register_function("native_image_text", wrap_image_text);
    registry.register_function("Raylib_native_image_text_ex", wrap_image_text_ex);
    registry.register_function("raylib_Raylib_native_image_text_ex", wrap_image_text_ex);
    registry.register_function("native_image_text_ex", wrap_image_text_ex);
    registry.register_function("Raylib_native_image_format", wrap_image_format);
    registry.register_function("raylib_Raylib_native_image_format", wrap_image_format);
    registry.register_function("native_image_format", wrap_image_format);
    registry.register_function("Raylib_native_image_to_pot", wrap_image_to_pot);
    registry.register_function("raylib_Raylib_native_image_to_pot", wrap_image_to_pot);
    registry.register_function("native_image_to_pot", wrap_image_to_pot);
    registry.register_function("Raylib_native_image_crop", wrap_image_crop);
    registry.register_function("raylib_Raylib_native_image_crop", wrap_image_crop);
    registry.register_function("native_image_crop", wrap_image_crop);
    registry.register_function("Raylib_native_image_alpha_crop", wrap_image_alpha_crop);
    registry.register_function("raylib_Raylib_native_image_alpha_crop", wrap_image_alpha_crop);
    registry.register_function("native_image_alpha_crop", wrap_image_alpha_crop);
    registry.register_function("Raylib_native_image_alpha_clear", wrap_image_alpha_clear);
    registry.register_function("raylib_Raylib_native_image_alpha_clear", wrap_image_alpha_clear);
    registry.register_function("native_image_alpha_clear", wrap_image_alpha_clear);
    registry.register_function("Raylib_native_image_alpha_mask", wrap_image_alpha_mask);
    registry.register_function("raylib_Raylib_native_image_alpha_mask", wrap_image_alpha_mask);
    registry.register_function("native_image_alpha_mask", wrap_image_alpha_mask);
    registry.register_function("Raylib_native_image_alpha_premultiply", wrap_image_alpha_premultiply);
    registry.register_function("raylib_Raylib_native_image_alpha_premultiply", wrap_image_alpha_premultiply);
    registry.register_function("native_image_alpha_premultiply", wrap_image_alpha_premultiply);
    registry.register_function("Raylib_native_image_blur_gaussian", wrap_image_blur_gaussian);
    registry.register_function("raylib_Raylib_native_image_blur_gaussian", wrap_image_blur_gaussian);
    registry.register_function("native_image_blur_gaussian", wrap_image_blur_gaussian);
    registry.register_function("Raylib_native_image_kernel_convolution", wrap_image_kernel_convolution);
    registry.register_function("raylib_Raylib_native_image_kernel_convolution", wrap_image_kernel_convolution);
    registry.register_function("native_image_kernel_convolution", wrap_image_kernel_convolution);
    registry.register_function("Raylib_native_image_resize", wrap_image_resize);
    registry.register_function("raylib_Raylib_native_image_resize", wrap_image_resize);
    registry.register_function("native_image_resize", wrap_image_resize);
    registry.register_function("Raylib_native_image_resize_nn", wrap_image_resize_nn);
    registry.register_function("raylib_Raylib_native_image_resize_nn", wrap_image_resize_nn);
    registry.register_function("native_image_resize_nn", wrap_image_resize_nn);
    registry.register_function("Raylib_native_image_resize_canvas", wrap_image_resize_canvas);
    registry.register_function("raylib_Raylib_native_image_resize_canvas", wrap_image_resize_canvas);
    registry.register_function("native_image_resize_canvas", wrap_image_resize_canvas);
    registry.register_function("Raylib_native_image_mipmaps", wrap_image_mipmaps);
    registry.register_function("raylib_Raylib_native_image_mipmaps", wrap_image_mipmaps);
    registry.register_function("native_image_mipmaps", wrap_image_mipmaps);
    registry.register_function("Raylib_native_image_dither", wrap_image_dither);
    registry.register_function("raylib_Raylib_native_image_dither", wrap_image_dither);
    registry.register_function("native_image_dither", wrap_image_dither);
    registry.register_function("Raylib_native_image_flip_vertical", wrap_image_flip_vertical);
    registry.register_function("raylib_Raylib_native_image_flip_vertical", wrap_image_flip_vertical);
    registry.register_function("native_image_flip_vertical", wrap_image_flip_vertical);
    registry.register_function("Raylib_native_image_flip_horizontal", wrap_image_flip_horizontal);
    registry.register_function("raylib_Raylib_native_image_flip_horizontal", wrap_image_flip_horizontal);
    registry.register_function("native_image_flip_horizontal", wrap_image_flip_horizontal);
    registry.register_function("Raylib_native_image_rotate", wrap_image_rotate);
    registry.register_function("raylib_Raylib_native_image_rotate", wrap_image_rotate);
    registry.register_function("native_image_rotate", wrap_image_rotate);
    registry.register_function("Raylib_native_image_rotate_cw", wrap_image_rotate_cw);
    registry.register_function("raylib_Raylib_native_image_rotate_cw", wrap_image_rotate_cw);
    registry.register_function("native_image_rotate_cw", wrap_image_rotate_cw);
    registry.register_function("Raylib_native_image_rotate_ccw", wrap_image_rotate_ccw);
    registry.register_function("raylib_Raylib_native_image_rotate_ccw", wrap_image_rotate_ccw);
    registry.register_function("native_image_rotate_ccw", wrap_image_rotate_ccw);
    registry.register_function("Raylib_native_image_color_tint", wrap_image_color_tint);
    registry.register_function("raylib_Raylib_native_image_color_tint", wrap_image_color_tint);
    registry.register_function("native_image_color_tint", wrap_image_color_tint);
    registry.register_function("Raylib_native_image_color_invert", wrap_image_color_invert);
    registry.register_function("raylib_Raylib_native_image_color_invert", wrap_image_color_invert);
    registry.register_function("native_image_color_invert", wrap_image_color_invert);
    registry.register_function("Raylib_native_image_color_grayscale", wrap_image_color_grayscale);
    registry.register_function("raylib_Raylib_native_image_color_grayscale", wrap_image_color_grayscale);
    registry.register_function("native_image_color_grayscale", wrap_image_color_grayscale);
    registry.register_function("Raylib_native_image_color_contrast", wrap_image_color_contrast);
    registry.register_function("raylib_Raylib_native_image_color_contrast", wrap_image_color_contrast);
    registry.register_function("native_image_color_contrast", wrap_image_color_contrast);
    registry.register_function("Raylib_native_image_color_brightness", wrap_image_color_brightness);
    registry.register_function("raylib_Raylib_native_image_color_brightness", wrap_image_color_brightness);
    registry.register_function("native_image_color_brightness", wrap_image_color_brightness);
    registry.register_function("Raylib_native_image_color_replace", wrap_image_color_replace);
    registry.register_function("raylib_Raylib_native_image_color_replace", wrap_image_color_replace);
    registry.register_function("native_image_color_replace", wrap_image_color_replace);
    registry.register_function("Raylib_native_load_image_colors", wrap_load_image_colors);
    registry.register_function("raylib_Raylib_native_load_image_colors", wrap_load_image_colors);
    registry.register_function("native_load_image_colors", wrap_load_image_colors);
    registry.register_function("Raylib_native_load_image_palette", wrap_load_image_palette);
    registry.register_function("raylib_Raylib_native_load_image_palette", wrap_load_image_palette);
    registry.register_function("native_load_image_palette", wrap_load_image_palette);
    registry.register_function("Raylib_native_unload_image_colors", wrap_unload_image_colors);
    registry.register_function("raylib_Raylib_native_unload_image_colors", wrap_unload_image_colors);
    registry.register_function("native_unload_image_colors", wrap_unload_image_colors);
    registry.register_function("Raylib_native_unload_image_palette", wrap_unload_image_palette);
    registry.register_function("raylib_Raylib_native_unload_image_palette", wrap_unload_image_palette);
    registry.register_function("native_unload_image_palette", wrap_unload_image_palette);
    registry.register_function("Raylib_native_get_image_alpha_border", wrap_get_image_alpha_border);
    registry.register_function("raylib_Raylib_native_get_image_alpha_border", wrap_get_image_alpha_border);
    registry.register_function("native_get_image_alpha_border", wrap_get_image_alpha_border);
    registry.register_function("Raylib_native_get_image_color", wrap_get_image_color);
    registry.register_function("raylib_Raylib_native_get_image_color", wrap_get_image_color);
    registry.register_function("native_get_image_color", wrap_get_image_color);
    registry.register_function("Raylib_native_image_clear_background", wrap_image_clear_background);
    registry.register_function("raylib_Raylib_native_image_clear_background", wrap_image_clear_background);
    registry.register_function("native_image_clear_background", wrap_image_clear_background);
    registry.register_function("Raylib_native_image_draw_pixel", wrap_image_draw_pixel);
    registry.register_function("raylib_Raylib_native_image_draw_pixel", wrap_image_draw_pixel);
    registry.register_function("native_image_draw_pixel", wrap_image_draw_pixel);
    registry.register_function("Raylib_native_image_draw_pixel_v", wrap_image_draw_pixel_v);
    registry.register_function("raylib_Raylib_native_image_draw_pixel_v", wrap_image_draw_pixel_v);
    registry.register_function("native_image_draw_pixel_v", wrap_image_draw_pixel_v);
    registry.register_function("Raylib_native_image_draw_line", wrap_image_draw_line);
    registry.register_function("raylib_Raylib_native_image_draw_line", wrap_image_draw_line);
    registry.register_function("native_image_draw_line", wrap_image_draw_line);
    registry.register_function("Raylib_native_image_draw_line_v", wrap_image_draw_line_v);
    registry.register_function("raylib_Raylib_native_image_draw_line_v", wrap_image_draw_line_v);
    registry.register_function("native_image_draw_line_v", wrap_image_draw_line_v);
    registry.register_function("Raylib_native_image_draw_line_ex", wrap_image_draw_line_ex);
    registry.register_function("raylib_Raylib_native_image_draw_line_ex", wrap_image_draw_line_ex);
    registry.register_function("native_image_draw_line_ex", wrap_image_draw_line_ex);
    registry.register_function("Raylib_native_image_draw_line_strip", wrap_image_draw_line_strip);
    registry.register_function("raylib_Raylib_native_image_draw_line_strip", wrap_image_draw_line_strip);
    registry.register_function("native_image_draw_line_strip", wrap_image_draw_line_strip);
    registry.register_function("Raylib_native_image_draw_triangle", wrap_image_draw_triangle);
    registry.register_function("raylib_Raylib_native_image_draw_triangle", wrap_image_draw_triangle);
    registry.register_function("native_image_draw_triangle", wrap_image_draw_triangle);
    registry.register_function("Raylib_native_image_draw_triangle_gradient", wrap_image_draw_triangle_gradient);
    registry.register_function("raylib_Raylib_native_image_draw_triangle_gradient", wrap_image_draw_triangle_gradient);
    registry.register_function("native_image_draw_triangle_gradient", wrap_image_draw_triangle_gradient);
    registry.register_function("Raylib_native_image_draw_triangle_lines", wrap_image_draw_triangle_lines);
    registry.register_function("raylib_Raylib_native_image_draw_triangle_lines", wrap_image_draw_triangle_lines);
    registry.register_function("native_image_draw_triangle_lines", wrap_image_draw_triangle_lines);
    registry.register_function("Raylib_native_image_draw_triangle_fan", wrap_image_draw_triangle_fan);
    registry.register_function("raylib_Raylib_native_image_draw_triangle_fan", wrap_image_draw_triangle_fan);
    registry.register_function("native_image_draw_triangle_fan", wrap_image_draw_triangle_fan);
    registry.register_function("Raylib_native_image_draw_triangle_strip", wrap_image_draw_triangle_strip);
    registry.register_function("raylib_Raylib_native_image_draw_triangle_strip", wrap_image_draw_triangle_strip);
    registry.register_function("native_image_draw_triangle_strip", wrap_image_draw_triangle_strip);
    registry.register_function("Raylib_native_image_draw_rectangle", wrap_image_draw_rectangle);
    registry.register_function("raylib_Raylib_native_image_draw_rectangle", wrap_image_draw_rectangle);
    registry.register_function("native_image_draw_rectangle", wrap_image_draw_rectangle);
    registry.register_function("Raylib_native_image_draw_rectangle_v", wrap_image_draw_rectangle_v);
    registry.register_function("raylib_Raylib_native_image_draw_rectangle_v", wrap_image_draw_rectangle_v);
    registry.register_function("native_image_draw_rectangle_v", wrap_image_draw_rectangle_v);
    registry.register_function("Raylib_native_image_draw_rectangle_rec", wrap_image_draw_rectangle_rec);
    registry.register_function("raylib_Raylib_native_image_draw_rectangle_rec", wrap_image_draw_rectangle_rec);
    registry.register_function("native_image_draw_rectangle_rec", wrap_image_draw_rectangle_rec);
    registry.register_function("Raylib_native_image_draw_rectangle_pro", wrap_image_draw_rectangle_pro);
    registry.register_function("raylib_Raylib_native_image_draw_rectangle_pro", wrap_image_draw_rectangle_pro);
    registry.register_function("native_image_draw_rectangle_pro", wrap_image_draw_rectangle_pro);
    registry.register_function("Raylib_native_image_draw_rectangle_lines", wrap_image_draw_rectangle_lines);
    registry.register_function("raylib_Raylib_native_image_draw_rectangle_lines", wrap_image_draw_rectangle_lines);
    registry.register_function("native_image_draw_rectangle_lines", wrap_image_draw_rectangle_lines);
    registry.register_function("Raylib_native_image_draw_rectangle_lines_ex", wrap_image_draw_rectangle_lines_ex);
    registry.register_function("raylib_Raylib_native_image_draw_rectangle_lines_ex", wrap_image_draw_rectangle_lines_ex);
    registry.register_function("native_image_draw_rectangle_lines_ex", wrap_image_draw_rectangle_lines_ex);
    registry.register_function("Raylib_native_image_draw_rectangle_gradient_ex", wrap_image_draw_rectangle_gradient_ex);
    registry.register_function("raylib_Raylib_native_image_draw_rectangle_gradient_ex", wrap_image_draw_rectangle_gradient_ex);
    registry.register_function("native_image_draw_rectangle_gradient_ex", wrap_image_draw_rectangle_gradient_ex);
    registry.register_function("Raylib_native_image_draw_circle", wrap_image_draw_circle);
    registry.register_function("raylib_Raylib_native_image_draw_circle", wrap_image_draw_circle);
    registry.register_function("native_image_draw_circle", wrap_image_draw_circle);
    registry.register_function("Raylib_native_image_draw_circle_v", wrap_image_draw_circle_v);
    registry.register_function("raylib_Raylib_native_image_draw_circle_v", wrap_image_draw_circle_v);
    registry.register_function("native_image_draw_circle_v", wrap_image_draw_circle_v);
    registry.register_function("Raylib_native_image_draw_circle_lines", wrap_image_draw_circle_lines);
    registry.register_function("raylib_Raylib_native_image_draw_circle_lines", wrap_image_draw_circle_lines);
    registry.register_function("native_image_draw_circle_lines", wrap_image_draw_circle_lines);
    registry.register_function("Raylib_native_image_draw_circle_lines_v", wrap_image_draw_circle_lines_v);
    registry.register_function("raylib_Raylib_native_image_draw_circle_lines_v", wrap_image_draw_circle_lines_v);
    registry.register_function("native_image_draw_circle_lines_v", wrap_image_draw_circle_lines_v);
    registry.register_function("Raylib_native_image_draw_circle_gradient", wrap_image_draw_circle_gradient);
    registry.register_function("raylib_Raylib_native_image_draw_circle_gradient", wrap_image_draw_circle_gradient);
    registry.register_function("native_image_draw_circle_gradient", wrap_image_draw_circle_gradient);
    registry.register_function("Raylib_native_image_draw_image", wrap_image_draw_image);
    registry.register_function("raylib_Raylib_native_image_draw_image", wrap_image_draw_image);
    registry.register_function("native_image_draw_image", wrap_image_draw_image);
    registry.register_function("Raylib_native_image_draw_image_ex", wrap_image_draw_image_ex);
    registry.register_function("raylib_Raylib_native_image_draw_image_ex", wrap_image_draw_image_ex);
    registry.register_function("native_image_draw_image_ex", wrap_image_draw_image_ex);
    registry.register_function("Raylib_native_image_draw_image_rec", wrap_image_draw_image_rec);
    registry.register_function("raylib_Raylib_native_image_draw_image_rec", wrap_image_draw_image_rec);
    registry.register_function("native_image_draw_image_rec", wrap_image_draw_image_rec);
    registry.register_function("Raylib_native_image_draw_image_pro", wrap_image_draw_image_pro);
    registry.register_function("raylib_Raylib_native_image_draw_image_pro", wrap_image_draw_image_pro);
    registry.register_function("native_image_draw_image_pro", wrap_image_draw_image_pro);
    registry.register_function("Raylib_native_image_draw_text", wrap_image_draw_text);
    registry.register_function("raylib_Raylib_native_image_draw_text", wrap_image_draw_text);
    registry.register_function("native_image_draw_text", wrap_image_draw_text);
    registry.register_function("Raylib_native_image_draw_text_ex", wrap_image_draw_text_ex);
    registry.register_function("raylib_Raylib_native_image_draw_text_ex", wrap_image_draw_text_ex);
    registry.register_function("native_image_draw_text_ex", wrap_image_draw_text_ex);
    registry.register_function("Raylib_native_image_draw_text_pro", wrap_image_draw_text_pro);
    registry.register_function("raylib_Raylib_native_image_draw_text_pro", wrap_image_draw_text_pro);
    registry.register_function("native_image_draw_text_pro", wrap_image_draw_text_pro);
    registry.register_function("Raylib_native_load_texture", wrap_load_texture);
    registry.register_function("raylib_Raylib_native_load_texture", wrap_load_texture);
    registry.register_function("native_load_texture", wrap_load_texture);
    registry.register_function("Raylib_native_load_texture_from_image", wrap_load_texture_from_image);
    registry.register_function("raylib_Raylib_native_load_texture_from_image", wrap_load_texture_from_image);
    registry.register_function("native_load_texture_from_image", wrap_load_texture_from_image);
    registry.register_function("Raylib_native_load_texture_cubemap", wrap_load_texture_cubemap);
    registry.register_function("raylib_Raylib_native_load_texture_cubemap", wrap_load_texture_cubemap);
    registry.register_function("native_load_texture_cubemap", wrap_load_texture_cubemap);
    registry.register_function("Raylib_native_load_render_texture", wrap_load_render_texture);
    registry.register_function("raylib_Raylib_native_load_render_texture", wrap_load_render_texture);
    registry.register_function("native_load_render_texture", wrap_load_render_texture);
    registry.register_function("Raylib_native_load_render_texture_ex", wrap_load_render_texture_ex);
    registry.register_function("raylib_Raylib_native_load_render_texture_ex", wrap_load_render_texture_ex);
    registry.register_function("native_load_render_texture_ex", wrap_load_render_texture_ex);
    registry.register_function("Raylib_native_is_texture_valid", wrap_is_texture_valid);
    registry.register_function("raylib_Raylib_native_is_texture_valid", wrap_is_texture_valid);
    registry.register_function("native_is_texture_valid", wrap_is_texture_valid);
    registry.register_function("Raylib_native_unload_texture", wrap_unload_texture);
    registry.register_function("raylib_Raylib_native_unload_texture", wrap_unload_texture);
    registry.register_function("native_unload_texture", wrap_unload_texture);
    registry.register_function("Raylib_native_is_render_texture_valid", wrap_is_render_texture_valid);
    registry.register_function("raylib_Raylib_native_is_render_texture_valid", wrap_is_render_texture_valid);
    registry.register_function("native_is_render_texture_valid", wrap_is_render_texture_valid);
    registry.register_function("Raylib_native_unload_render_texture", wrap_unload_render_texture);
    registry.register_function("raylib_Raylib_native_unload_render_texture", wrap_unload_render_texture);
    registry.register_function("native_unload_render_texture", wrap_unload_render_texture);
    registry.register_function("Raylib_native_update_texture", wrap_update_texture);
    registry.register_function("raylib_Raylib_native_update_texture", wrap_update_texture);
    registry.register_function("native_update_texture", wrap_update_texture);
    registry.register_function("Raylib_native_update_texture_rec", wrap_update_texture_rec);
    registry.register_function("raylib_Raylib_native_update_texture_rec", wrap_update_texture_rec);
    registry.register_function("native_update_texture_rec", wrap_update_texture_rec);
    registry.register_function("Raylib_native_gen_texture_mipmaps", wrap_gen_texture_mipmaps);
    registry.register_function("raylib_Raylib_native_gen_texture_mipmaps", wrap_gen_texture_mipmaps);
    registry.register_function("native_gen_texture_mipmaps", wrap_gen_texture_mipmaps);
    registry.register_function("Raylib_native_set_texture_filter", wrap_set_texture_filter);
    registry.register_function("raylib_Raylib_native_set_texture_filter", wrap_set_texture_filter);
    registry.register_function("native_set_texture_filter", wrap_set_texture_filter);
    registry.register_function("Raylib_native_set_texture_wrap", wrap_set_texture_wrap);
    registry.register_function("raylib_Raylib_native_set_texture_wrap", wrap_set_texture_wrap);
    registry.register_function("native_set_texture_wrap", wrap_set_texture_wrap);
    registry.register_function("Raylib_native_draw_texture", wrap_draw_texture);
    registry.register_function("raylib_Raylib_native_draw_texture", wrap_draw_texture);
    registry.register_function("native_draw_texture", wrap_draw_texture);
    registry.register_function("Raylib_native_draw_texture_v", wrap_draw_texture_v);
    registry.register_function("raylib_Raylib_native_draw_texture_v", wrap_draw_texture_v);
    registry.register_function("native_draw_texture_v", wrap_draw_texture_v);
    registry.register_function("Raylib_native_draw_texture_ex", wrap_draw_texture_ex);
    registry.register_function("raylib_Raylib_native_draw_texture_ex", wrap_draw_texture_ex);
    registry.register_function("native_draw_texture_ex", wrap_draw_texture_ex);
    registry.register_function("Raylib_native_draw_texture_rec", wrap_draw_texture_rec);
    registry.register_function("raylib_Raylib_native_draw_texture_rec", wrap_draw_texture_rec);
    registry.register_function("native_draw_texture_rec", wrap_draw_texture_rec);
    registry.register_function("Raylib_native_draw_texture_pro", wrap_draw_texture_pro);
    registry.register_function("raylib_Raylib_native_draw_texture_pro", wrap_draw_texture_pro);
    registry.register_function("native_draw_texture_pro", wrap_draw_texture_pro);
    registry.register_function("Raylib_native_draw_texture_n_patch", wrap_draw_texture_n_patch);
    registry.register_function("raylib_Raylib_native_draw_texture_n_patch", wrap_draw_texture_n_patch);
    registry.register_function("native_draw_texture_n_patch", wrap_draw_texture_n_patch);
    registry.register_function("Raylib_native_color_is_equal", wrap_color_is_equal);
    registry.register_function("raylib_Raylib_native_color_is_equal", wrap_color_is_equal);
    registry.register_function("native_color_is_equal", wrap_color_is_equal);
    registry.register_function("Raylib_native_fade", wrap_fade);
    registry.register_function("raylib_Raylib_native_fade", wrap_fade);
    registry.register_function("native_fade", wrap_fade);
    registry.register_function("Raylib_native_color_to_int", wrap_color_to_int);
    registry.register_function("raylib_Raylib_native_color_to_int", wrap_color_to_int);
    registry.register_function("native_color_to_int", wrap_color_to_int);
    registry.register_function("Raylib_native_color_normalize", wrap_color_normalize);
    registry.register_function("raylib_Raylib_native_color_normalize", wrap_color_normalize);
    registry.register_function("native_color_normalize", wrap_color_normalize);
    registry.register_function("Raylib_native_color_from_normalized", wrap_color_from_normalized);
    registry.register_function("raylib_Raylib_native_color_from_normalized", wrap_color_from_normalized);
    registry.register_function("native_color_from_normalized", wrap_color_from_normalized);
    registry.register_function("Raylib_native_color_to_hsv", wrap_color_to_hsv);
    registry.register_function("raylib_Raylib_native_color_to_hsv", wrap_color_to_hsv);
    registry.register_function("native_color_to_hsv", wrap_color_to_hsv);
    registry.register_function("Raylib_native_color_from_hsv", wrap_color_from_hsv);
    registry.register_function("raylib_Raylib_native_color_from_hsv", wrap_color_from_hsv);
    registry.register_function("native_color_from_hsv", wrap_color_from_hsv);
    registry.register_function("Raylib_native_color_tint", wrap_color_tint);
    registry.register_function("raylib_Raylib_native_color_tint", wrap_color_tint);
    registry.register_function("native_color_tint", wrap_color_tint);
    registry.register_function("Raylib_native_color_brightness", wrap_color_brightness);
    registry.register_function("raylib_Raylib_native_color_brightness", wrap_color_brightness);
    registry.register_function("native_color_brightness", wrap_color_brightness);
    registry.register_function("Raylib_native_color_contrast", wrap_color_contrast);
    registry.register_function("raylib_Raylib_native_color_contrast", wrap_color_contrast);
    registry.register_function("native_color_contrast", wrap_color_contrast);
    registry.register_function("Raylib_native_color_alpha", wrap_color_alpha);
    registry.register_function("raylib_Raylib_native_color_alpha", wrap_color_alpha);
    registry.register_function("native_color_alpha", wrap_color_alpha);
    registry.register_function("Raylib_native_color_alpha_blend", wrap_color_alpha_blend);
    registry.register_function("raylib_Raylib_native_color_alpha_blend", wrap_color_alpha_blend);
    registry.register_function("native_color_alpha_blend", wrap_color_alpha_blend);
    registry.register_function("Raylib_native_color_lerp", wrap_color_lerp);
    registry.register_function("raylib_Raylib_native_color_lerp", wrap_color_lerp);
    registry.register_function("native_color_lerp", wrap_color_lerp);
    registry.register_function("Raylib_native_get_color", wrap_get_color);
    registry.register_function("raylib_Raylib_native_get_color", wrap_get_color);
    registry.register_function("native_get_color", wrap_get_color);
    registry.register_function("Raylib_native_get_pixel_color", wrap_get_pixel_color);
    registry.register_function("raylib_Raylib_native_get_pixel_color", wrap_get_pixel_color);
    registry.register_function("native_get_pixel_color", wrap_get_pixel_color);
    registry.register_function("Raylib_native_set_pixel_color", wrap_set_pixel_color);
    registry.register_function("raylib_Raylib_native_set_pixel_color", wrap_set_pixel_color);
    registry.register_function("native_set_pixel_color", wrap_set_pixel_color);
    registry.register_function("Raylib_native_get_pixel_data_size", wrap_get_pixel_data_size);
    registry.register_function("raylib_Raylib_native_get_pixel_data_size", wrap_get_pixel_data_size);
    registry.register_function("native_get_pixel_data_size", wrap_get_pixel_data_size);
    registry.register_function("Raylib_native_get_font_default", wrap_get_font_default);
    registry.register_function("raylib_Raylib_native_get_font_default", wrap_get_font_default);
    registry.register_function("native_get_font_default", wrap_get_font_default);
    registry.register_function("Raylib_native_load_font", wrap_load_font);
    registry.register_function("raylib_Raylib_native_load_font", wrap_load_font);
    registry.register_function("native_load_font", wrap_load_font);
    registry.register_function("Raylib_native_load_font_ex", wrap_load_font_ex);
    registry.register_function("raylib_Raylib_native_load_font_ex", wrap_load_font_ex);
    registry.register_function("native_load_font_ex", wrap_load_font_ex);
    registry.register_function("Raylib_native_load_font_from_image", wrap_load_font_from_image);
    registry.register_function("raylib_Raylib_native_load_font_from_image", wrap_load_font_from_image);
    registry.register_function("native_load_font_from_image", wrap_load_font_from_image);
    registry.register_function("Raylib_native_load_font_from_memory", wrap_load_font_from_memory);
    registry.register_function("raylib_Raylib_native_load_font_from_memory", wrap_load_font_from_memory);
    registry.register_function("native_load_font_from_memory", wrap_load_font_from_memory);
    registry.register_function("Raylib_native_is_font_valid", wrap_is_font_valid);
    registry.register_function("raylib_Raylib_native_is_font_valid", wrap_is_font_valid);
    registry.register_function("native_is_font_valid", wrap_is_font_valid);
    registry.register_function("Raylib_native_load_font_data", wrap_load_font_data);
    registry.register_function("raylib_Raylib_native_load_font_data", wrap_load_font_data);
    registry.register_function("native_load_font_data", wrap_load_font_data);
    registry.register_function("Raylib_native_unload_font_data", wrap_unload_font_data);
    registry.register_function("raylib_Raylib_native_unload_font_data", wrap_unload_font_data);
    registry.register_function("native_unload_font_data", wrap_unload_font_data);
    registry.register_function("Raylib_native_unload_font", wrap_unload_font);
    registry.register_function("raylib_Raylib_native_unload_font", wrap_unload_font);
    registry.register_function("native_unload_font", wrap_unload_font);
    registry.register_function("Raylib_native_export_font_as_code", wrap_export_font_as_code);
    registry.register_function("raylib_Raylib_native_export_font_as_code", wrap_export_font_as_code);
    registry.register_function("native_export_font_as_code", wrap_export_font_as_code);
    registry.register_function("Raylib_native_draw_fps", wrap_draw_fps);
    registry.register_function("raylib_Raylib_native_draw_fps", wrap_draw_fps);
    registry.register_function("native_draw_fps", wrap_draw_fps);
    registry.register_function("Raylib_native_draw_text", wrap_draw_text);
    registry.register_function("raylib_Raylib_native_draw_text", wrap_draw_text);
    registry.register_function("native_draw_text", wrap_draw_text);
    registry.register_function("Raylib_native_draw_text_ex", wrap_draw_text_ex);
    registry.register_function("raylib_Raylib_native_draw_text_ex", wrap_draw_text_ex);
    registry.register_function("native_draw_text_ex", wrap_draw_text_ex);
    registry.register_function("Raylib_native_draw_text_pro", wrap_draw_text_pro);
    registry.register_function("raylib_Raylib_native_draw_text_pro", wrap_draw_text_pro);
    registry.register_function("native_draw_text_pro", wrap_draw_text_pro);
    registry.register_function("Raylib_native_draw_text_codepoint", wrap_draw_text_codepoint);
    registry.register_function("raylib_Raylib_native_draw_text_codepoint", wrap_draw_text_codepoint);
    registry.register_function("native_draw_text_codepoint", wrap_draw_text_codepoint);
    registry.register_function("Raylib_native_draw_text_codepoints", wrap_draw_text_codepoints);
    registry.register_function("raylib_Raylib_native_draw_text_codepoints", wrap_draw_text_codepoints);
    registry.register_function("native_draw_text_codepoints", wrap_draw_text_codepoints);
    registry.register_function("Raylib_native_set_text_line_spacing", wrap_set_text_line_spacing);
    registry.register_function("raylib_Raylib_native_set_text_line_spacing", wrap_set_text_line_spacing);
    registry.register_function("native_set_text_line_spacing", wrap_set_text_line_spacing);
    registry.register_function("Raylib_native_measure_text", wrap_measure_text);
    registry.register_function("raylib_Raylib_native_measure_text", wrap_measure_text);
    registry.register_function("native_measure_text", wrap_measure_text);
    registry.register_function("Raylib_native_measure_text_ex", wrap_measure_text_ex);
    registry.register_function("raylib_Raylib_native_measure_text_ex", wrap_measure_text_ex);
    registry.register_function("native_measure_text_ex", wrap_measure_text_ex);
    registry.register_function("Raylib_native_measure_text_codepoints", wrap_measure_text_codepoints);
    registry.register_function("raylib_Raylib_native_measure_text_codepoints", wrap_measure_text_codepoints);
    registry.register_function("native_measure_text_codepoints", wrap_measure_text_codepoints);
    registry.register_function("Raylib_native_get_glyph_index", wrap_get_glyph_index);
    registry.register_function("raylib_Raylib_native_get_glyph_index", wrap_get_glyph_index);
    registry.register_function("native_get_glyph_index", wrap_get_glyph_index);
    registry.register_function("Raylib_native_get_glyph_info", wrap_get_glyph_info);
    registry.register_function("raylib_Raylib_native_get_glyph_info", wrap_get_glyph_info);
    registry.register_function("native_get_glyph_info", wrap_get_glyph_info);
    registry.register_function("Raylib_native_get_glyph_atlas_rec", wrap_get_glyph_atlas_rec);
    registry.register_function("raylib_Raylib_native_get_glyph_atlas_rec", wrap_get_glyph_atlas_rec);
    registry.register_function("native_get_glyph_atlas_rec", wrap_get_glyph_atlas_rec);
    registry.register_function("Raylib_native_load_utf8", wrap_load_utf8);
    registry.register_function("raylib_Raylib_native_load_utf8", wrap_load_utf8);
    registry.register_function("native_load_utf8", wrap_load_utf8);
    registry.register_function("Raylib_native_unload_utf8", wrap_unload_utf8);
    registry.register_function("raylib_Raylib_native_unload_utf8", wrap_unload_utf8);
    registry.register_function("native_unload_utf8", wrap_unload_utf8);
    registry.register_function("Raylib_native_load_codepoints", wrap_load_codepoints);
    registry.register_function("raylib_Raylib_native_load_codepoints", wrap_load_codepoints);
    registry.register_function("native_load_codepoints", wrap_load_codepoints);
    registry.register_function("Raylib_native_unload_codepoints", wrap_unload_codepoints);
    registry.register_function("raylib_Raylib_native_unload_codepoints", wrap_unload_codepoints);
    registry.register_function("native_unload_codepoints", wrap_unload_codepoints);
    registry.register_function("Raylib_native_get_codepoint_count", wrap_get_codepoint_count);
    registry.register_function("raylib_Raylib_native_get_codepoint_count", wrap_get_codepoint_count);
    registry.register_function("native_get_codepoint_count", wrap_get_codepoint_count);
    registry.register_function("Raylib_native_get_codepoint", wrap_get_codepoint);
    registry.register_function("raylib_Raylib_native_get_codepoint", wrap_get_codepoint);
    registry.register_function("native_get_codepoint", wrap_get_codepoint);
    registry.register_function("Raylib_native_get_codepoint_next", wrap_get_codepoint_next);
    registry.register_function("raylib_Raylib_native_get_codepoint_next", wrap_get_codepoint_next);
    registry.register_function("native_get_codepoint_next", wrap_get_codepoint_next);
    registry.register_function("Raylib_native_get_codepoint_previous", wrap_get_codepoint_previous);
    registry.register_function("raylib_Raylib_native_get_codepoint_previous", wrap_get_codepoint_previous);
    registry.register_function("native_get_codepoint_previous", wrap_get_codepoint_previous);
    registry.register_function("Raylib_native_codepoint_to_utf8", wrap_codepoint_to_utf8);
    registry.register_function("raylib_Raylib_native_codepoint_to_utf8", wrap_codepoint_to_utf8);
    registry.register_function("native_codepoint_to_utf8", wrap_codepoint_to_utf8);
    registry.register_function("Raylib_native_load_text_lines", wrap_load_text_lines);
    registry.register_function("raylib_Raylib_native_load_text_lines", wrap_load_text_lines);
    registry.register_function("native_load_text_lines", wrap_load_text_lines);
    registry.register_function("Raylib_native_unload_text_lines", wrap_unload_text_lines);
    registry.register_function("raylib_Raylib_native_unload_text_lines", wrap_unload_text_lines);
    registry.register_function("native_unload_text_lines", wrap_unload_text_lines);
    registry.register_function("Raylib_native_text_copy", wrap_text_copy);
    registry.register_function("raylib_Raylib_native_text_copy", wrap_text_copy);
    registry.register_function("native_text_copy", wrap_text_copy);
    registry.register_function("Raylib_native_text_is_equal", wrap_text_is_equal);
    registry.register_function("raylib_Raylib_native_text_is_equal", wrap_text_is_equal);
    registry.register_function("native_text_is_equal", wrap_text_is_equal);
    registry.register_function("Raylib_native_text_length", wrap_text_length);
    registry.register_function("raylib_Raylib_native_text_length", wrap_text_length);
    registry.register_function("native_text_length", wrap_text_length);
    registry.register_function("Raylib_native_text_subtext", wrap_text_subtext);
    registry.register_function("raylib_Raylib_native_text_subtext", wrap_text_subtext);
    registry.register_function("native_text_subtext", wrap_text_subtext);
    registry.register_function("Raylib_native_text_remove_spaces", wrap_text_remove_spaces);
    registry.register_function("raylib_Raylib_native_text_remove_spaces", wrap_text_remove_spaces);
    registry.register_function("native_text_remove_spaces", wrap_text_remove_spaces);
    registry.register_function("Raylib_native_get_text_between", wrap_get_text_between);
    registry.register_function("raylib_Raylib_native_get_text_between", wrap_get_text_between);
    registry.register_function("native_get_text_between", wrap_get_text_between);
    registry.register_function("Raylib_native_text_replace", wrap_text_replace);
    registry.register_function("raylib_Raylib_native_text_replace", wrap_text_replace);
    registry.register_function("native_text_replace", wrap_text_replace);
    registry.register_function("Raylib_native_text_replace_alloc", wrap_text_replace_alloc);
    registry.register_function("raylib_Raylib_native_text_replace_alloc", wrap_text_replace_alloc);
    registry.register_function("native_text_replace_alloc", wrap_text_replace_alloc);
    registry.register_function("Raylib_native_text_replace_between", wrap_text_replace_between);
    registry.register_function("raylib_Raylib_native_text_replace_between", wrap_text_replace_between);
    registry.register_function("native_text_replace_between", wrap_text_replace_between);
    registry.register_function("Raylib_native_text_replace_between_alloc", wrap_text_replace_between_alloc);
    registry.register_function("raylib_Raylib_native_text_replace_between_alloc", wrap_text_replace_between_alloc);
    registry.register_function("native_text_replace_between_alloc", wrap_text_replace_between_alloc);
    registry.register_function("Raylib_native_text_insert", wrap_text_insert);
    registry.register_function("raylib_Raylib_native_text_insert", wrap_text_insert);
    registry.register_function("native_text_insert", wrap_text_insert);
    registry.register_function("Raylib_native_text_insert_alloc", wrap_text_insert_alloc);
    registry.register_function("raylib_Raylib_native_text_insert_alloc", wrap_text_insert_alloc);
    registry.register_function("native_text_insert_alloc", wrap_text_insert_alloc);
    registry.register_function("Raylib_native_text_join", wrap_text_join);
    registry.register_function("raylib_Raylib_native_text_join", wrap_text_join);
    registry.register_function("native_text_join", wrap_text_join);
    registry.register_function("Raylib_native_text_split", wrap_text_split);
    registry.register_function("raylib_Raylib_native_text_split", wrap_text_split);
    registry.register_function("native_text_split", wrap_text_split);
    registry.register_function("Raylib_native_text_append", wrap_text_append);
    registry.register_function("raylib_Raylib_native_text_append", wrap_text_append);
    registry.register_function("native_text_append", wrap_text_append);
    registry.register_function("Raylib_native_text_find_index", wrap_text_find_index);
    registry.register_function("raylib_Raylib_native_text_find_index", wrap_text_find_index);
    registry.register_function("native_text_find_index", wrap_text_find_index);
    registry.register_function("Raylib_native_text_to_upper", wrap_text_to_upper);
    registry.register_function("raylib_Raylib_native_text_to_upper", wrap_text_to_upper);
    registry.register_function("native_text_to_upper", wrap_text_to_upper);
    registry.register_function("Raylib_native_text_to_lower", wrap_text_to_lower);
    registry.register_function("raylib_Raylib_native_text_to_lower", wrap_text_to_lower);
    registry.register_function("native_text_to_lower", wrap_text_to_lower);
    registry.register_function("Raylib_native_text_to_pascal", wrap_text_to_pascal);
    registry.register_function("raylib_Raylib_native_text_to_pascal", wrap_text_to_pascal);
    registry.register_function("native_text_to_pascal", wrap_text_to_pascal);
    registry.register_function("Raylib_native_text_to_snake", wrap_text_to_snake);
    registry.register_function("raylib_Raylib_native_text_to_snake", wrap_text_to_snake);
    registry.register_function("native_text_to_snake", wrap_text_to_snake);
    registry.register_function("Raylib_native_text_to_camel", wrap_text_to_camel);
    registry.register_function("raylib_Raylib_native_text_to_camel", wrap_text_to_camel);
    registry.register_function("native_text_to_camel", wrap_text_to_camel);
    registry.register_function("Raylib_native_text_to_integer", wrap_text_to_integer);
    registry.register_function("raylib_Raylib_native_text_to_integer", wrap_text_to_integer);
    registry.register_function("native_text_to_integer", wrap_text_to_integer);
    registry.register_function("Raylib_native_text_to_float", wrap_text_to_float);
    registry.register_function("raylib_Raylib_native_text_to_float", wrap_text_to_float);
    registry.register_function("native_text_to_float", wrap_text_to_float);
    registry.register_function("Raylib_native_draw_line3d", wrap_draw_line3d);
    registry.register_function("raylib_Raylib_native_draw_line3d", wrap_draw_line3d);
    registry.register_function("native_draw_line3d", wrap_draw_line3d);
    registry.register_function("Raylib_native_draw_point3d", wrap_draw_point3d);
    registry.register_function("raylib_Raylib_native_draw_point3d", wrap_draw_point3d);
    registry.register_function("native_draw_point3d", wrap_draw_point3d);
    registry.register_function("Raylib_native_draw_circle3d", wrap_draw_circle3d);
    registry.register_function("raylib_Raylib_native_draw_circle3d", wrap_draw_circle3d);
    registry.register_function("native_draw_circle3d", wrap_draw_circle3d);
    registry.register_function("Raylib_native_draw_triangle3d", wrap_draw_triangle3d);
    registry.register_function("raylib_Raylib_native_draw_triangle3d", wrap_draw_triangle3d);
    registry.register_function("native_draw_triangle3d", wrap_draw_triangle3d);
    registry.register_function("Raylib_native_draw_triangle_strip3d", wrap_draw_triangle_strip3d);
    registry.register_function("raylib_Raylib_native_draw_triangle_strip3d", wrap_draw_triangle_strip3d);
    registry.register_function("native_draw_triangle_strip3d", wrap_draw_triangle_strip3d);
    registry.register_function("Raylib_native_draw_cube", wrap_draw_cube);
    registry.register_function("raylib_Raylib_native_draw_cube", wrap_draw_cube);
    registry.register_function("native_draw_cube", wrap_draw_cube);
    registry.register_function("Raylib_native_draw_cube_v", wrap_draw_cube_v);
    registry.register_function("raylib_Raylib_native_draw_cube_v", wrap_draw_cube_v);
    registry.register_function("native_draw_cube_v", wrap_draw_cube_v);
    registry.register_function("Raylib_native_draw_cube_wires", wrap_draw_cube_wires);
    registry.register_function("raylib_Raylib_native_draw_cube_wires", wrap_draw_cube_wires);
    registry.register_function("native_draw_cube_wires", wrap_draw_cube_wires);
    registry.register_function("Raylib_native_draw_cube_wires_v", wrap_draw_cube_wires_v);
    registry.register_function("raylib_Raylib_native_draw_cube_wires_v", wrap_draw_cube_wires_v);
    registry.register_function("native_draw_cube_wires_v", wrap_draw_cube_wires_v);
    registry.register_function("Raylib_native_draw_sphere", wrap_draw_sphere);
    registry.register_function("raylib_Raylib_native_draw_sphere", wrap_draw_sphere);
    registry.register_function("native_draw_sphere", wrap_draw_sphere);
    registry.register_function("Raylib_native_draw_sphere_ex", wrap_draw_sphere_ex);
    registry.register_function("raylib_Raylib_native_draw_sphere_ex", wrap_draw_sphere_ex);
    registry.register_function("native_draw_sphere_ex", wrap_draw_sphere_ex);
    registry.register_function("Raylib_native_draw_sphere_wires", wrap_draw_sphere_wires);
    registry.register_function("raylib_Raylib_native_draw_sphere_wires", wrap_draw_sphere_wires);
    registry.register_function("native_draw_sphere_wires", wrap_draw_sphere_wires);
    registry.register_function("Raylib_native_draw_cylinder", wrap_draw_cylinder);
    registry.register_function("raylib_Raylib_native_draw_cylinder", wrap_draw_cylinder);
    registry.register_function("native_draw_cylinder", wrap_draw_cylinder);
    registry.register_function("Raylib_native_draw_cylinder_ex", wrap_draw_cylinder_ex);
    registry.register_function("raylib_Raylib_native_draw_cylinder_ex", wrap_draw_cylinder_ex);
    registry.register_function("native_draw_cylinder_ex", wrap_draw_cylinder_ex);
    registry.register_function("Raylib_native_draw_cylinder_wires", wrap_draw_cylinder_wires);
    registry.register_function("raylib_Raylib_native_draw_cylinder_wires", wrap_draw_cylinder_wires);
    registry.register_function("native_draw_cylinder_wires", wrap_draw_cylinder_wires);
    registry.register_function("Raylib_native_draw_cylinder_wires_ex", wrap_draw_cylinder_wires_ex);
    registry.register_function("raylib_Raylib_native_draw_cylinder_wires_ex", wrap_draw_cylinder_wires_ex);
    registry.register_function("native_draw_cylinder_wires_ex", wrap_draw_cylinder_wires_ex);
    registry.register_function("Raylib_native_draw_capsule", wrap_draw_capsule);
    registry.register_function("raylib_Raylib_native_draw_capsule", wrap_draw_capsule);
    registry.register_function("native_draw_capsule", wrap_draw_capsule);
    registry.register_function("Raylib_native_draw_capsule_wires", wrap_draw_capsule_wires);
    registry.register_function("raylib_Raylib_native_draw_capsule_wires", wrap_draw_capsule_wires);
    registry.register_function("native_draw_capsule_wires", wrap_draw_capsule_wires);
    registry.register_function("Raylib_native_draw_plane", wrap_draw_plane);
    registry.register_function("raylib_Raylib_native_draw_plane", wrap_draw_plane);
    registry.register_function("native_draw_plane", wrap_draw_plane);
    registry.register_function("Raylib_native_draw_ray", wrap_draw_ray);
    registry.register_function("raylib_Raylib_native_draw_ray", wrap_draw_ray);
    registry.register_function("native_draw_ray", wrap_draw_ray);
    registry.register_function("Raylib_native_draw_grid", wrap_draw_grid);
    registry.register_function("raylib_Raylib_native_draw_grid", wrap_draw_grid);
    registry.register_function("native_draw_grid", wrap_draw_grid);
    registry.register_function("Raylib_native_load_model", wrap_load_model);
    registry.register_function("raylib_Raylib_native_load_model", wrap_load_model);
    registry.register_function("native_load_model", wrap_load_model);
    registry.register_function("Raylib_native_load_model_from_mesh", wrap_load_model_from_mesh);
    registry.register_function("raylib_Raylib_native_load_model_from_mesh", wrap_load_model_from_mesh);
    registry.register_function("native_load_model_from_mesh", wrap_load_model_from_mesh);
    registry.register_function("Raylib_native_is_model_valid", wrap_is_model_valid);
    registry.register_function("raylib_Raylib_native_is_model_valid", wrap_is_model_valid);
    registry.register_function("native_is_model_valid", wrap_is_model_valid);
    registry.register_function("Raylib_native_unload_model", wrap_unload_model);
    registry.register_function("raylib_Raylib_native_unload_model", wrap_unload_model);
    registry.register_function("native_unload_model", wrap_unload_model);
    registry.register_function("Raylib_native_get_model_bounding_box", wrap_get_model_bounding_box);
    registry.register_function("raylib_Raylib_native_get_model_bounding_box", wrap_get_model_bounding_box);
    registry.register_function("native_get_model_bounding_box", wrap_get_model_bounding_box);
    registry.register_function("Raylib_native_draw_model", wrap_draw_model);
    registry.register_function("raylib_Raylib_native_draw_model", wrap_draw_model);
    registry.register_function("native_draw_model", wrap_draw_model);
    registry.register_function("Raylib_native_draw_model_ex", wrap_draw_model_ex);
    registry.register_function("raylib_Raylib_native_draw_model_ex", wrap_draw_model_ex);
    registry.register_function("native_draw_model_ex", wrap_draw_model_ex);
    registry.register_function("Raylib_native_draw_model_wires", wrap_draw_model_wires);
    registry.register_function("raylib_Raylib_native_draw_model_wires", wrap_draw_model_wires);
    registry.register_function("native_draw_model_wires", wrap_draw_model_wires);
    registry.register_function("Raylib_native_draw_model_wires_ex", wrap_draw_model_wires_ex);
    registry.register_function("raylib_Raylib_native_draw_model_wires_ex", wrap_draw_model_wires_ex);
    registry.register_function("native_draw_model_wires_ex", wrap_draw_model_wires_ex);
    registry.register_function("Raylib_native_draw_bounding_box", wrap_draw_bounding_box);
    registry.register_function("raylib_Raylib_native_draw_bounding_box", wrap_draw_bounding_box);
    registry.register_function("native_draw_bounding_box", wrap_draw_bounding_box);
    registry.register_function("Raylib_native_draw_billboard", wrap_draw_billboard);
    registry.register_function("raylib_Raylib_native_draw_billboard", wrap_draw_billboard);
    registry.register_function("native_draw_billboard", wrap_draw_billboard);
    registry.register_function("Raylib_native_draw_billboard_rec", wrap_draw_billboard_rec);
    registry.register_function("raylib_Raylib_native_draw_billboard_rec", wrap_draw_billboard_rec);
    registry.register_function("native_draw_billboard_rec", wrap_draw_billboard_rec);
    registry.register_function("Raylib_native_draw_billboard_pro", wrap_draw_billboard_pro);
    registry.register_function("raylib_Raylib_native_draw_billboard_pro", wrap_draw_billboard_pro);
    registry.register_function("native_draw_billboard_pro", wrap_draw_billboard_pro);
    registry.register_function("Raylib_native_upload_mesh", wrap_upload_mesh);
    registry.register_function("raylib_Raylib_native_upload_mesh", wrap_upload_mesh);
    registry.register_function("native_upload_mesh", wrap_upload_mesh);
    registry.register_function("Raylib_native_update_mesh_buffer", wrap_update_mesh_buffer);
    registry.register_function("raylib_Raylib_native_update_mesh_buffer", wrap_update_mesh_buffer);
    registry.register_function("native_update_mesh_buffer", wrap_update_mesh_buffer);
    registry.register_function("Raylib_native_unload_mesh", wrap_unload_mesh);
    registry.register_function("raylib_Raylib_native_unload_mesh", wrap_unload_mesh);
    registry.register_function("native_unload_mesh", wrap_unload_mesh);
    registry.register_function("Raylib_native_draw_mesh", wrap_draw_mesh);
    registry.register_function("raylib_Raylib_native_draw_mesh", wrap_draw_mesh);
    registry.register_function("native_draw_mesh", wrap_draw_mesh);
    registry.register_function("Raylib_native_draw_mesh_instanced", wrap_draw_mesh_instanced);
    registry.register_function("raylib_Raylib_native_draw_mesh_instanced", wrap_draw_mesh_instanced);
    registry.register_function("native_draw_mesh_instanced", wrap_draw_mesh_instanced);
    registry.register_function("Raylib_native_get_mesh_bounding_box", wrap_get_mesh_bounding_box);
    registry.register_function("raylib_Raylib_native_get_mesh_bounding_box", wrap_get_mesh_bounding_box);
    registry.register_function("native_get_mesh_bounding_box", wrap_get_mesh_bounding_box);
    registry.register_function("Raylib_native_gen_mesh_tangents", wrap_gen_mesh_tangents);
    registry.register_function("raylib_Raylib_native_gen_mesh_tangents", wrap_gen_mesh_tangents);
    registry.register_function("native_gen_mesh_tangents", wrap_gen_mesh_tangents);
    registry.register_function("Raylib_native_export_mesh", wrap_export_mesh);
    registry.register_function("raylib_Raylib_native_export_mesh", wrap_export_mesh);
    registry.register_function("native_export_mesh", wrap_export_mesh);
    registry.register_function("Raylib_native_export_mesh_as_code", wrap_export_mesh_as_code);
    registry.register_function("raylib_Raylib_native_export_mesh_as_code", wrap_export_mesh_as_code);
    registry.register_function("native_export_mesh_as_code", wrap_export_mesh_as_code);
    registry.register_function("Raylib_native_gen_mesh_poly", wrap_gen_mesh_poly);
    registry.register_function("raylib_Raylib_native_gen_mesh_poly", wrap_gen_mesh_poly);
    registry.register_function("native_gen_mesh_poly", wrap_gen_mesh_poly);
    registry.register_function("Raylib_native_gen_mesh_plane", wrap_gen_mesh_plane);
    registry.register_function("raylib_Raylib_native_gen_mesh_plane", wrap_gen_mesh_plane);
    registry.register_function("native_gen_mesh_plane", wrap_gen_mesh_plane);
    registry.register_function("Raylib_native_gen_mesh_cube", wrap_gen_mesh_cube);
    registry.register_function("raylib_Raylib_native_gen_mesh_cube", wrap_gen_mesh_cube);
    registry.register_function("native_gen_mesh_cube", wrap_gen_mesh_cube);
    registry.register_function("Raylib_native_gen_mesh_sphere", wrap_gen_mesh_sphere);
    registry.register_function("raylib_Raylib_native_gen_mesh_sphere", wrap_gen_mesh_sphere);
    registry.register_function("native_gen_mesh_sphere", wrap_gen_mesh_sphere);
    registry.register_function("Raylib_native_gen_mesh_hemi_sphere", wrap_gen_mesh_hemi_sphere);
    registry.register_function("raylib_Raylib_native_gen_mesh_hemi_sphere", wrap_gen_mesh_hemi_sphere);
    registry.register_function("native_gen_mesh_hemi_sphere", wrap_gen_mesh_hemi_sphere);
    registry.register_function("Raylib_native_gen_mesh_cylinder", wrap_gen_mesh_cylinder);
    registry.register_function("raylib_Raylib_native_gen_mesh_cylinder", wrap_gen_mesh_cylinder);
    registry.register_function("native_gen_mesh_cylinder", wrap_gen_mesh_cylinder);
    registry.register_function("Raylib_native_gen_mesh_cone", wrap_gen_mesh_cone);
    registry.register_function("raylib_Raylib_native_gen_mesh_cone", wrap_gen_mesh_cone);
    registry.register_function("native_gen_mesh_cone", wrap_gen_mesh_cone);
    registry.register_function("Raylib_native_gen_mesh_torus", wrap_gen_mesh_torus);
    registry.register_function("raylib_Raylib_native_gen_mesh_torus", wrap_gen_mesh_torus);
    registry.register_function("native_gen_mesh_torus", wrap_gen_mesh_torus);
    registry.register_function("Raylib_native_gen_mesh_knot", wrap_gen_mesh_knot);
    registry.register_function("raylib_Raylib_native_gen_mesh_knot", wrap_gen_mesh_knot);
    registry.register_function("native_gen_mesh_knot", wrap_gen_mesh_knot);
    registry.register_function("Raylib_native_gen_mesh_heightmap", wrap_gen_mesh_heightmap);
    registry.register_function("raylib_Raylib_native_gen_mesh_heightmap", wrap_gen_mesh_heightmap);
    registry.register_function("native_gen_mesh_heightmap", wrap_gen_mesh_heightmap);
    registry.register_function("Raylib_native_gen_mesh_cubicmap", wrap_gen_mesh_cubicmap);
    registry.register_function("raylib_Raylib_native_gen_mesh_cubicmap", wrap_gen_mesh_cubicmap);
    registry.register_function("native_gen_mesh_cubicmap", wrap_gen_mesh_cubicmap);
    registry.register_function("Raylib_native_load_materials", wrap_load_materials);
    registry.register_function("raylib_Raylib_native_load_materials", wrap_load_materials);
    registry.register_function("native_load_materials", wrap_load_materials);
    registry.register_function("Raylib_native_load_material_default", wrap_load_material_default);
    registry.register_function("raylib_Raylib_native_load_material_default", wrap_load_material_default);
    registry.register_function("native_load_material_default", wrap_load_material_default);
    registry.register_function("Raylib_native_is_material_valid", wrap_is_material_valid);
    registry.register_function("raylib_Raylib_native_is_material_valid", wrap_is_material_valid);
    registry.register_function("native_is_material_valid", wrap_is_material_valid);
    registry.register_function("Raylib_native_unload_material", wrap_unload_material);
    registry.register_function("raylib_Raylib_native_unload_material", wrap_unload_material);
    registry.register_function("native_unload_material", wrap_unload_material);
    registry.register_function("Raylib_native_set_material_texture", wrap_set_material_texture);
    registry.register_function("raylib_Raylib_native_set_material_texture", wrap_set_material_texture);
    registry.register_function("native_set_material_texture", wrap_set_material_texture);
    registry.register_function("Raylib_native_set_model_mesh_material", wrap_set_model_mesh_material);
    registry.register_function("raylib_Raylib_native_set_model_mesh_material", wrap_set_model_mesh_material);
    registry.register_function("native_set_model_mesh_material", wrap_set_model_mesh_material);
    registry.register_function("Raylib_native_load_model_animations", wrap_load_model_animations);
    registry.register_function("raylib_Raylib_native_load_model_animations", wrap_load_model_animations);
    registry.register_function("native_load_model_animations", wrap_load_model_animations);
    registry.register_function("Raylib_native_update_model_animation", wrap_update_model_animation);
    registry.register_function("raylib_Raylib_native_update_model_animation", wrap_update_model_animation);
    registry.register_function("native_update_model_animation", wrap_update_model_animation);
    registry.register_function("Raylib_native_update_model_animation_ex", wrap_update_model_animation_ex);
    registry.register_function("raylib_Raylib_native_update_model_animation_ex", wrap_update_model_animation_ex);
    registry.register_function("native_update_model_animation_ex", wrap_update_model_animation_ex);
    registry.register_function("Raylib_native_unload_model_animations", wrap_unload_model_animations);
    registry.register_function("raylib_Raylib_native_unload_model_animations", wrap_unload_model_animations);
    registry.register_function("native_unload_model_animations", wrap_unload_model_animations);
    registry.register_function("Raylib_native_is_model_animation_valid", wrap_is_model_animation_valid);
    registry.register_function("raylib_Raylib_native_is_model_animation_valid", wrap_is_model_animation_valid);
    registry.register_function("native_is_model_animation_valid", wrap_is_model_animation_valid);
    registry.register_function("Raylib_native_check_collision_spheres", wrap_check_collision_spheres);
    registry.register_function("raylib_Raylib_native_check_collision_spheres", wrap_check_collision_spheres);
    registry.register_function("native_check_collision_spheres", wrap_check_collision_spheres);
    registry.register_function("Raylib_native_check_collision_boxes", wrap_check_collision_boxes);
    registry.register_function("raylib_Raylib_native_check_collision_boxes", wrap_check_collision_boxes);
    registry.register_function("native_check_collision_boxes", wrap_check_collision_boxes);
    registry.register_function("Raylib_native_check_collision_box_sphere", wrap_check_collision_box_sphere);
    registry.register_function("raylib_Raylib_native_check_collision_box_sphere", wrap_check_collision_box_sphere);
    registry.register_function("native_check_collision_box_sphere", wrap_check_collision_box_sphere);
    registry.register_function("Raylib_native_get_ray_collision_sphere", wrap_get_ray_collision_sphere);
    registry.register_function("raylib_Raylib_native_get_ray_collision_sphere", wrap_get_ray_collision_sphere);
    registry.register_function("native_get_ray_collision_sphere", wrap_get_ray_collision_sphere);
    registry.register_function("Raylib_native_get_ray_collision_box", wrap_get_ray_collision_box);
    registry.register_function("raylib_Raylib_native_get_ray_collision_box", wrap_get_ray_collision_box);
    registry.register_function("native_get_ray_collision_box", wrap_get_ray_collision_box);
    registry.register_function("Raylib_native_get_ray_collision_mesh", wrap_get_ray_collision_mesh);
    registry.register_function("raylib_Raylib_native_get_ray_collision_mesh", wrap_get_ray_collision_mesh);
    registry.register_function("native_get_ray_collision_mesh", wrap_get_ray_collision_mesh);
    registry.register_function("Raylib_native_get_ray_collision_triangle", wrap_get_ray_collision_triangle);
    registry.register_function("raylib_Raylib_native_get_ray_collision_triangle", wrap_get_ray_collision_triangle);
    registry.register_function("native_get_ray_collision_triangle", wrap_get_ray_collision_triangle);
    registry.register_function("Raylib_native_get_ray_collision_quad", wrap_get_ray_collision_quad);
    registry.register_function("raylib_Raylib_native_get_ray_collision_quad", wrap_get_ray_collision_quad);
    registry.register_function("native_get_ray_collision_quad", wrap_get_ray_collision_quad);
    registry.register_function("Raylib_native_init_audio_device", wrap_init_audio_device);
    registry.register_function("raylib_Raylib_native_init_audio_device", wrap_init_audio_device);
    registry.register_function("native_init_audio_device", wrap_init_audio_device);
    registry.register_function("Raylib_native_close_audio_device", wrap_close_audio_device);
    registry.register_function("raylib_Raylib_native_close_audio_device", wrap_close_audio_device);
    registry.register_function("native_close_audio_device", wrap_close_audio_device);
    registry.register_function("Raylib_native_is_audio_device_ready", wrap_is_audio_device_ready);
    registry.register_function("raylib_Raylib_native_is_audio_device_ready", wrap_is_audio_device_ready);
    registry.register_function("native_is_audio_device_ready", wrap_is_audio_device_ready);
    registry.register_function("Raylib_native_set_master_volume", wrap_set_master_volume);
    registry.register_function("raylib_Raylib_native_set_master_volume", wrap_set_master_volume);
    registry.register_function("native_set_master_volume", wrap_set_master_volume);
    registry.register_function("Raylib_native_get_master_volume", wrap_get_master_volume);
    registry.register_function("raylib_Raylib_native_get_master_volume", wrap_get_master_volume);
    registry.register_function("native_get_master_volume", wrap_get_master_volume);
    registry.register_function("Raylib_native_load_wave", wrap_load_wave);
    registry.register_function("raylib_Raylib_native_load_wave", wrap_load_wave);
    registry.register_function("native_load_wave", wrap_load_wave);
    registry.register_function("Raylib_native_load_wave_from_memory", wrap_load_wave_from_memory);
    registry.register_function("raylib_Raylib_native_load_wave_from_memory", wrap_load_wave_from_memory);
    registry.register_function("native_load_wave_from_memory", wrap_load_wave_from_memory);
    registry.register_function("Raylib_native_is_wave_valid", wrap_is_wave_valid);
    registry.register_function("raylib_Raylib_native_is_wave_valid", wrap_is_wave_valid);
    registry.register_function("native_is_wave_valid", wrap_is_wave_valid);
    registry.register_function("Raylib_native_load_sound", wrap_load_sound);
    registry.register_function("raylib_Raylib_native_load_sound", wrap_load_sound);
    registry.register_function("native_load_sound", wrap_load_sound);
    registry.register_function("Raylib_native_load_sound_from_wave", wrap_load_sound_from_wave);
    registry.register_function("raylib_Raylib_native_load_sound_from_wave", wrap_load_sound_from_wave);
    registry.register_function("native_load_sound_from_wave", wrap_load_sound_from_wave);
    registry.register_function("Raylib_native_load_sound_alias", wrap_load_sound_alias);
    registry.register_function("raylib_Raylib_native_load_sound_alias", wrap_load_sound_alias);
    registry.register_function("native_load_sound_alias", wrap_load_sound_alias);
    registry.register_function("Raylib_native_is_sound_valid", wrap_is_sound_valid);
    registry.register_function("raylib_Raylib_native_is_sound_valid", wrap_is_sound_valid);
    registry.register_function("native_is_sound_valid", wrap_is_sound_valid);
    registry.register_function("Raylib_native_update_sound", wrap_update_sound);
    registry.register_function("raylib_Raylib_native_update_sound", wrap_update_sound);
    registry.register_function("native_update_sound", wrap_update_sound);
    registry.register_function("Raylib_native_unload_wave", wrap_unload_wave);
    registry.register_function("raylib_Raylib_native_unload_wave", wrap_unload_wave);
    registry.register_function("native_unload_wave", wrap_unload_wave);
    registry.register_function("Raylib_native_unload_sound", wrap_unload_sound);
    registry.register_function("raylib_Raylib_native_unload_sound", wrap_unload_sound);
    registry.register_function("native_unload_sound", wrap_unload_sound);
    registry.register_function("Raylib_native_unload_sound_alias", wrap_unload_sound_alias);
    registry.register_function("raylib_Raylib_native_unload_sound_alias", wrap_unload_sound_alias);
    registry.register_function("native_unload_sound_alias", wrap_unload_sound_alias);
    registry.register_function("Raylib_native_export_wave", wrap_export_wave);
    registry.register_function("raylib_Raylib_native_export_wave", wrap_export_wave);
    registry.register_function("native_export_wave", wrap_export_wave);
    registry.register_function("Raylib_native_export_wave_as_code", wrap_export_wave_as_code);
    registry.register_function("raylib_Raylib_native_export_wave_as_code", wrap_export_wave_as_code);
    registry.register_function("native_export_wave_as_code", wrap_export_wave_as_code);
    registry.register_function("Raylib_native_play_sound", wrap_play_sound);
    registry.register_function("raylib_Raylib_native_play_sound", wrap_play_sound);
    registry.register_function("native_play_sound", wrap_play_sound);
    registry.register_function("Raylib_native_stop_sound", wrap_stop_sound);
    registry.register_function("raylib_Raylib_native_stop_sound", wrap_stop_sound);
    registry.register_function("native_stop_sound", wrap_stop_sound);
    registry.register_function("Raylib_native_pause_sound", wrap_pause_sound);
    registry.register_function("raylib_Raylib_native_pause_sound", wrap_pause_sound);
    registry.register_function("native_pause_sound", wrap_pause_sound);
    registry.register_function("Raylib_native_resume_sound", wrap_resume_sound);
    registry.register_function("raylib_Raylib_native_resume_sound", wrap_resume_sound);
    registry.register_function("native_resume_sound", wrap_resume_sound);
    registry.register_function("Raylib_native_is_sound_playing", wrap_is_sound_playing);
    registry.register_function("raylib_Raylib_native_is_sound_playing", wrap_is_sound_playing);
    registry.register_function("native_is_sound_playing", wrap_is_sound_playing);
    registry.register_function("Raylib_native_set_sound_volume", wrap_set_sound_volume);
    registry.register_function("raylib_Raylib_native_set_sound_volume", wrap_set_sound_volume);
    registry.register_function("native_set_sound_volume", wrap_set_sound_volume);
    registry.register_function("Raylib_native_set_sound_pitch", wrap_set_sound_pitch);
    registry.register_function("raylib_Raylib_native_set_sound_pitch", wrap_set_sound_pitch);
    registry.register_function("native_set_sound_pitch", wrap_set_sound_pitch);
    registry.register_function("Raylib_native_set_sound_pan", wrap_set_sound_pan);
    registry.register_function("raylib_Raylib_native_set_sound_pan", wrap_set_sound_pan);
    registry.register_function("native_set_sound_pan", wrap_set_sound_pan);
    registry.register_function("Raylib_native_wave_copy", wrap_wave_copy);
    registry.register_function("raylib_Raylib_native_wave_copy", wrap_wave_copy);
    registry.register_function("native_wave_copy", wrap_wave_copy);
    registry.register_function("Raylib_native_wave_crop", wrap_wave_crop);
    registry.register_function("raylib_Raylib_native_wave_crop", wrap_wave_crop);
    registry.register_function("native_wave_crop", wrap_wave_crop);
    registry.register_function("Raylib_native_wave_format", wrap_wave_format);
    registry.register_function("raylib_Raylib_native_wave_format", wrap_wave_format);
    registry.register_function("native_wave_format", wrap_wave_format);
    registry.register_function("Raylib_native_load_wave_samples", wrap_load_wave_samples);
    registry.register_function("raylib_Raylib_native_load_wave_samples", wrap_load_wave_samples);
    registry.register_function("native_load_wave_samples", wrap_load_wave_samples);
    registry.register_function("Raylib_native_unload_wave_samples", wrap_unload_wave_samples);
    registry.register_function("raylib_Raylib_native_unload_wave_samples", wrap_unload_wave_samples);
    registry.register_function("native_unload_wave_samples", wrap_unload_wave_samples);
    registry.register_function("Raylib_native_load_music_stream", wrap_load_music_stream);
    registry.register_function("raylib_Raylib_native_load_music_stream", wrap_load_music_stream);
    registry.register_function("native_load_music_stream", wrap_load_music_stream);
    registry.register_function("Raylib_native_load_music_stream_from_memory", wrap_load_music_stream_from_memory);
    registry.register_function("raylib_Raylib_native_load_music_stream_from_memory", wrap_load_music_stream_from_memory);
    registry.register_function("native_load_music_stream_from_memory", wrap_load_music_stream_from_memory);
    registry.register_function("Raylib_native_is_music_valid", wrap_is_music_valid);
    registry.register_function("raylib_Raylib_native_is_music_valid", wrap_is_music_valid);
    registry.register_function("native_is_music_valid", wrap_is_music_valid);
    registry.register_function("Raylib_native_unload_music_stream", wrap_unload_music_stream);
    registry.register_function("raylib_Raylib_native_unload_music_stream", wrap_unload_music_stream);
    registry.register_function("native_unload_music_stream", wrap_unload_music_stream);
    registry.register_function("Raylib_native_play_music_stream", wrap_play_music_stream);
    registry.register_function("raylib_Raylib_native_play_music_stream", wrap_play_music_stream);
    registry.register_function("native_play_music_stream", wrap_play_music_stream);
    registry.register_function("Raylib_native_is_music_stream_playing", wrap_is_music_stream_playing);
    registry.register_function("raylib_Raylib_native_is_music_stream_playing", wrap_is_music_stream_playing);
    registry.register_function("native_is_music_stream_playing", wrap_is_music_stream_playing);
    registry.register_function("Raylib_native_update_music_stream", wrap_update_music_stream);
    registry.register_function("raylib_Raylib_native_update_music_stream", wrap_update_music_stream);
    registry.register_function("native_update_music_stream", wrap_update_music_stream);
    registry.register_function("Raylib_native_stop_music_stream", wrap_stop_music_stream);
    registry.register_function("raylib_Raylib_native_stop_music_stream", wrap_stop_music_stream);
    registry.register_function("native_stop_music_stream", wrap_stop_music_stream);
    registry.register_function("Raylib_native_pause_music_stream", wrap_pause_music_stream);
    registry.register_function("raylib_Raylib_native_pause_music_stream", wrap_pause_music_stream);
    registry.register_function("native_pause_music_stream", wrap_pause_music_stream);
    registry.register_function("Raylib_native_resume_music_stream", wrap_resume_music_stream);
    registry.register_function("raylib_Raylib_native_resume_music_stream", wrap_resume_music_stream);
    registry.register_function("native_resume_music_stream", wrap_resume_music_stream);
    registry.register_function("Raylib_native_seek_music_stream", wrap_seek_music_stream);
    registry.register_function("raylib_Raylib_native_seek_music_stream", wrap_seek_music_stream);
    registry.register_function("native_seek_music_stream", wrap_seek_music_stream);
    registry.register_function("Raylib_native_set_music_volume", wrap_set_music_volume);
    registry.register_function("raylib_Raylib_native_set_music_volume", wrap_set_music_volume);
    registry.register_function("native_set_music_volume", wrap_set_music_volume);
    registry.register_function("Raylib_native_set_music_pitch", wrap_set_music_pitch);
    registry.register_function("raylib_Raylib_native_set_music_pitch", wrap_set_music_pitch);
    registry.register_function("native_set_music_pitch", wrap_set_music_pitch);
    registry.register_function("Raylib_native_set_music_pan", wrap_set_music_pan);
    registry.register_function("raylib_Raylib_native_set_music_pan", wrap_set_music_pan);
    registry.register_function("native_set_music_pan", wrap_set_music_pan);
    registry.register_function("Raylib_native_get_music_time_length", wrap_get_music_time_length);
    registry.register_function("raylib_Raylib_native_get_music_time_length", wrap_get_music_time_length);
    registry.register_function("native_get_music_time_length", wrap_get_music_time_length);
    registry.register_function("Raylib_native_get_music_time_played", wrap_get_music_time_played);
    registry.register_function("raylib_Raylib_native_get_music_time_played", wrap_get_music_time_played);
    registry.register_function("native_get_music_time_played", wrap_get_music_time_played);
    registry.register_function("Raylib_native_load_audio_stream", wrap_load_audio_stream);
    registry.register_function("raylib_Raylib_native_load_audio_stream", wrap_load_audio_stream);
    registry.register_function("native_load_audio_stream", wrap_load_audio_stream);
    registry.register_function("Raylib_native_is_audio_stream_valid", wrap_is_audio_stream_valid);
    registry.register_function("raylib_Raylib_native_is_audio_stream_valid", wrap_is_audio_stream_valid);
    registry.register_function("native_is_audio_stream_valid", wrap_is_audio_stream_valid);
    registry.register_function("Raylib_native_unload_audio_stream", wrap_unload_audio_stream);
    registry.register_function("raylib_Raylib_native_unload_audio_stream", wrap_unload_audio_stream);
    registry.register_function("native_unload_audio_stream", wrap_unload_audio_stream);
    registry.register_function("Raylib_native_update_audio_stream", wrap_update_audio_stream);
    registry.register_function("raylib_Raylib_native_update_audio_stream", wrap_update_audio_stream);
    registry.register_function("native_update_audio_stream", wrap_update_audio_stream);
    registry.register_function("Raylib_native_is_audio_stream_processed", wrap_is_audio_stream_processed);
    registry.register_function("raylib_Raylib_native_is_audio_stream_processed", wrap_is_audio_stream_processed);
    registry.register_function("native_is_audio_stream_processed", wrap_is_audio_stream_processed);
    registry.register_function("Raylib_native_play_audio_stream", wrap_play_audio_stream);
    registry.register_function("raylib_Raylib_native_play_audio_stream", wrap_play_audio_stream);
    registry.register_function("native_play_audio_stream", wrap_play_audio_stream);
    registry.register_function("Raylib_native_pause_audio_stream", wrap_pause_audio_stream);
    registry.register_function("raylib_Raylib_native_pause_audio_stream", wrap_pause_audio_stream);
    registry.register_function("native_pause_audio_stream", wrap_pause_audio_stream);
    registry.register_function("Raylib_native_resume_audio_stream", wrap_resume_audio_stream);
    registry.register_function("raylib_Raylib_native_resume_audio_stream", wrap_resume_audio_stream);
    registry.register_function("native_resume_audio_stream", wrap_resume_audio_stream);
    registry.register_function("Raylib_native_is_audio_stream_playing", wrap_is_audio_stream_playing);
    registry.register_function("raylib_Raylib_native_is_audio_stream_playing", wrap_is_audio_stream_playing);
    registry.register_function("native_is_audio_stream_playing", wrap_is_audio_stream_playing);
    registry.register_function("Raylib_native_stop_audio_stream", wrap_stop_audio_stream);
    registry.register_function("raylib_Raylib_native_stop_audio_stream", wrap_stop_audio_stream);
    registry.register_function("native_stop_audio_stream", wrap_stop_audio_stream);
    registry.register_function("Raylib_native_set_audio_stream_volume", wrap_set_audio_stream_volume);
    registry.register_function("raylib_Raylib_native_set_audio_stream_volume", wrap_set_audio_stream_volume);
    registry.register_function("native_set_audio_stream_volume", wrap_set_audio_stream_volume);
    registry.register_function("Raylib_native_set_audio_stream_pitch", wrap_set_audio_stream_pitch);
    registry.register_function("raylib_Raylib_native_set_audio_stream_pitch", wrap_set_audio_stream_pitch);
    registry.register_function("native_set_audio_stream_pitch", wrap_set_audio_stream_pitch);
    registry.register_function("Raylib_native_set_audio_stream_pan", wrap_set_audio_stream_pan);
    registry.register_function("raylib_Raylib_native_set_audio_stream_pan", wrap_set_audio_stream_pan);
    registry.register_function("native_set_audio_stream_pan", wrap_set_audio_stream_pan);
    registry.register_function("Raylib_native_set_audio_stream_buffer_size_default", wrap_set_audio_stream_buffer_size_default);
    registry.register_function("raylib_Raylib_native_set_audio_stream_buffer_size_default", wrap_set_audio_stream_buffer_size_default);
    registry.register_function("native_set_audio_stream_buffer_size_default", wrap_set_audio_stream_buffer_size_default);
}
