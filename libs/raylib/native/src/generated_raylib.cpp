#include "raylib.h"
#include "solix/native.h"
#include "solix/runtime.hpp"
#include "solix/native_registry.hpp"
#include <cstring>
#include <string>
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

// Struct Unpackers (reading from Solix object offset)
// Solix class layout: [0..3: vtable_id]
inline ::Color unpack_color(solix::RuntimeContext &vm, uint64_t addr) {
    if (addr == 0) return {0, 0, 0, 255};
    uint32_t a = static_cast<uint32_t>(addr);
    uint8_t r = vm.memory.read_u8(a, 4);
    uint8_t g = vm.memory.read_u8(a, 5);
    uint8_t b = vm.memory.read_u8(a, 6);
    uint8_t av = vm.memory.read_u8(a, 7);
    return ::Color{r, g, b, av};
}

inline ::Vector2 unpack_vector2(solix::RuntimeContext &vm, uint64_t addr) {
    if (addr == 0) return {0.0f, 0.0f};
    uint32_t a = static_cast<uint32_t>(addr);
    uint32_t rx = vm.memory.read_u32(a, 4);
    uint32_t ry = vm.memory.read_u32(a, 8);
    float x, y;
    std::memcpy(&x, &rx, 4);
    std::memcpy(&y, &ry, 4);
    return ::Vector2{x, y};
}

inline ::Vector3 unpack_vector3(solix::RuntimeContext &vm, uint64_t addr) {
    if (addr == 0) return {0.0f, 0.0f, 0.0f};
    uint32_t a = static_cast<uint32_t>(addr);
    uint32_t rx = vm.memory.read_u32(a, 4);
    uint32_t ry = vm.memory.read_u32(a, 8);
    uint32_t rz = vm.memory.read_u32(a, 12);
    float x, y, z;
    std::memcpy(&x, &rx, 4);
    std::memcpy(&y, &ry, 4);
    std::memcpy(&z, &rz, 4);
    return ::Vector3{x, y, z};
}

inline ::Vector4 unpack_vector4(solix::RuntimeContext &vm, uint64_t addr) {
    if (addr == 0) return {0.0f, 0.0f, 0.0f, 0.0f};
    uint32_t a = static_cast<uint32_t>(addr);
    uint32_t rx = vm.memory.read_u32(a, 4);
    uint32_t ry = vm.memory.read_u32(a, 8);
    uint32_t rz = vm.memory.read_u32(a, 12);
    uint32_t rw = vm.memory.read_u32(a, 16);
    float x, y, z, w;
    std::memcpy(&x, &rx, 4);
    std::memcpy(&y, &ry, 4);
    std::memcpy(&z, &rz, 4);
    std::memcpy(&w, &rw, 4);
    return ::Vector4{x, y, z, w};
}

inline ::Rectangle unpack_rectangle(solix::RuntimeContext &vm, uint64_t addr) {
    if (addr == 0) return {0.0f, 0.0f, 0.0f, 0.0f};
    uint32_t a = static_cast<uint32_t>(addr);
    uint32_t rx = vm.memory.read_u32(a, 4);
    uint32_t ry = vm.memory.read_u32(a, 8);
    uint32_t rw = vm.memory.read_u32(a, 12);
    uint32_t rh = vm.memory.read_u32(a, 16);
    float x, y, w, h;
    std::memcpy(&x, &rx, 4);
    std::memcpy(&y, &ry, 4);
    std::memcpy(&w, &rw, 4);
    std::memcpy(&h, &rh, 4);
    return ::Rectangle{x, y, w, h};
}

// Struct Packers (writing result into Solix object)
inline void pack_vector2(solix::RuntimeContext &vm, uint64_t addr, const ::Vector2 &v) {
    uint32_t a = static_cast<uint32_t>(addr);
    uint32_t rx, ry;
    std::memcpy(&rx, &v.x, 4);
    std::memcpy(&ry, &v.y, 4);
    vm.memory.write_u32(a, 4, rx);
    vm.memory.write_u32(a, 8, ry);
}

inline void pack_vector3(solix::RuntimeContext &vm, uint64_t addr, const ::Vector3 &v) {
    uint32_t a = static_cast<uint32_t>(addr);
    uint32_t rx, ry, rz;
    std::memcpy(&rx, &v.x, 4);
    std::memcpy(&ry, &v.y, 4);
    std::memcpy(&rz, &v.z, 4);
    vm.memory.write_u32(a, 4, rx);
    vm.memory.write_u32(a, 8, ry);
    vm.memory.write_u32(a, 12, rz);
}

inline void pack_vector4(solix::RuntimeContext &vm, uint64_t addr, const ::Vector4 &v) {
    uint32_t a = static_cast<uint32_t>(addr);
    uint32_t rx, ry, rz, rw;
    std::memcpy(&rx, &v.x, 4);
    std::memcpy(&ry, &v.y, 4);
    std::memcpy(&rz, &v.z, 4);
    std::memcpy(&rw, &v.w, 4);
    vm.memory.write_u32(a, 4, rx);
    vm.memory.write_u32(a, 8, ry);
    vm.memory.write_u32(a, 12, rz);
    vm.memory.write_u32(a, 16, rw);
}

inline void pack_color(solix::RuntimeContext &vm, uint64_t addr, const ::Color &c) {
    uint32_t a = static_cast<uint32_t>(addr);
    vm.memory.write_u8(a, 4, c.r);
    vm.memory.write_u8(a, 5, c.g);
    vm.memory.write_u8(a, 6, c.b);
    vm.memory.write_u8(a, 7, c.a);
}

inline void pack_rectangle(solix::RuntimeContext &vm, uint64_t addr, const ::Rectangle &r) {
    uint32_t a = static_cast<uint32_t>(addr);
    uint32_t rx, ry, rw, rh;
    std::memcpy(&rx, &r.x, 4);
    std::memcpy(&ry, &r.y, 4);
    std::memcpy(&rw, &r.width, 4);
    std::memcpy(&rh, &r.height, 4);
    vm.memory.write_u32(a, 4, rx);
    vm.memory.write_u32(a, 8, ry);
    vm.memory.write_u32(a, 12, rw);
    vm.memory.write_u32(a, 16, rh);
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
    return ::WindowShouldClose() ? 1 : 0;
}

static uint64_t wrap_is_window_ready(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    return ::IsWindowReady() ? 1 : 0;
}

static uint64_t wrap_is_window_fullscreen(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    return ::IsWindowFullscreen() ? 1 : 0;
}

static uint64_t wrap_is_window_hidden(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    return ::IsWindowHidden() ? 1 : 0;
}

static uint64_t wrap_is_window_minimized(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    return ::IsWindowMinimized() ? 1 : 0;
}

static uint64_t wrap_is_window_maximized(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    return ::IsWindowMaximized() ? 1 : 0;
}

static uint64_t wrap_is_window_focused(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    return ::IsWindowFocused() ? 1 : 0;
}

static uint64_t wrap_is_window_resized(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    return ::IsWindowResized() ? 1 : 0;
}

static uint64_t wrap_is_window_state(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    unsigned int a0 = static_cast<unsigned int>(args[0]);
    return ::IsWindowState(a0) ? 1 : 0;
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

static uint64_t wrap_get_screen_width(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    return static_cast<uint64_t>(::GetScreenWidth());
}

static uint64_t wrap_get_screen_height(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    return static_cast<uint64_t>(::GetScreenHeight());
}

static uint64_t wrap_get_render_width(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    return static_cast<uint64_t>(::GetRenderWidth());
}

static uint64_t wrap_get_render_height(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    return static_cast<uint64_t>(::GetRenderHeight());
}

static uint64_t wrap_get_monitor_count(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    return static_cast<uint64_t>(::GetMonitorCount());
}

static uint64_t wrap_get_current_monitor(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    return static_cast<uint64_t>(::GetCurrentMonitor());
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
    return static_cast<uint64_t>(::GetMonitorWidth(a0));
}

static uint64_t wrap_get_monitor_height(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    return static_cast<uint64_t>(::GetMonitorHeight(a0));
}

static uint64_t wrap_get_monitor_physical_width(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    return static_cast<uint64_t>(::GetMonitorPhysicalWidth(a0));
}

static uint64_t wrap_get_monitor_physical_height(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    return static_cast<uint64_t>(::GetMonitorPhysicalHeight(a0));
}

static uint64_t wrap_get_monitor_refresh_rate(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    return static_cast<uint64_t>(::GetMonitorRefreshRate(a0));
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
    ::GetMonitorName(a0);
    return 0;
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
    ::GetClipboardText();
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
    return ::IsCursorHidden() ? 1 : 0;
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
    return ::IsCursorOnScreen() ? 1 : 0;
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

static uint64_t wrap_end_mode2d(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::EndMode2D();
    return 0;
}

static uint64_t wrap_end_mode3d(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::EndMode3D();
    return 0;
}

static uint64_t wrap_end_texture_mode(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::EndTextureMode();
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

static uint64_t wrap_end_vr_stereo_mode(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::EndVrStereoMode();
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
    return static_cast<uint64_t>(::GetFPS());
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
    return static_cast<uint64_t>(::GetRandomValue(a0, a1));
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

static uint64_t wrap_save_file_text(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    std::string s_a1 = read_solix_chars(vm, args[1]);
    const char *a1 = s_a1.c_str();
    return ::SaveFileText(a0, a1) ? 1 : 0;
}

static uint64_t wrap_file_rename(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    std::string s_a1 = read_solix_chars(vm, args[1]);
    const char *a1 = s_a1.c_str();
    return static_cast<uint64_t>(::FileRename(a0, a1));
}

static uint64_t wrap_file_remove(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    return static_cast<uint64_t>(::FileRemove(a0));
}

static uint64_t wrap_file_copy(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    std::string s_a1 = read_solix_chars(vm, args[1]);
    const char *a1 = s_a1.c_str();
    return static_cast<uint64_t>(::FileCopy(a0, a1));
}

static uint64_t wrap_file_move(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    std::string s_a1 = read_solix_chars(vm, args[1]);
    const char *a1 = s_a1.c_str();
    return static_cast<uint64_t>(::FileMove(a0, a1));
}

static uint64_t wrap_file_text_replace(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    std::string s_a1 = read_solix_chars(vm, args[1]);
    const char *a1 = s_a1.c_str();
    std::string s_a2 = read_solix_chars(vm, args[2]);
    const char *a2 = s_a2.c_str();
    return static_cast<uint64_t>(::FileTextReplace(a0, a1, a2));
}

static uint64_t wrap_file_text_find_index(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    std::string s_a1 = read_solix_chars(vm, args[1]);
    const char *a1 = s_a1.c_str();
    return static_cast<uint64_t>(::FileTextFindIndex(a0, a1));
}

static uint64_t wrap_file_exists(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    return ::FileExists(a0) ? 1 : 0;
}

static uint64_t wrap_directory_exists(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    return ::DirectoryExists(a0) ? 1 : 0;
}

static uint64_t wrap_is_file_extension(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    std::string s_a1 = read_solix_chars(vm, args[1]);
    const char *a1 = s_a1.c_str();
    return ::IsFileExtension(a0, a1) ? 1 : 0;
}

static uint64_t wrap_is_file_hidden(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    return ::IsFileHidden(a0) ? 1 : 0;
}

static uint64_t wrap_get_file_length(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    return static_cast<uint64_t>(::GetFileLength(a0));
}

static uint64_t wrap_get_file_extension(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    ::GetFileExtension(a0);
    return 0;
}

static uint64_t wrap_get_file_name(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    ::GetFileName(a0);
    return 0;
}

static uint64_t wrap_get_file_name_without_ext(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    ::GetFileNameWithoutExt(a0);
    return 0;
}

static uint64_t wrap_get_directory_path(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    ::GetDirectoryPath(a0);
    return 0;
}

static uint64_t wrap_get_prev_directory_path(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    ::GetPrevDirectoryPath(a0);
    return 0;
}

static uint64_t wrap_get_working_directory(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::GetWorkingDirectory();
    return 0;
}

static uint64_t wrap_get_application_directory(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::GetApplicationDirectory();
    return 0;
}

static uint64_t wrap_make_directory(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    return static_cast<uint64_t>(::MakeDirectory(a0));
}

static uint64_t wrap_change_directory(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    return static_cast<uint64_t>(::ChangeDirectory(a0));
}

static uint64_t wrap_is_path_file(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    return ::IsPathFile(a0) ? 1 : 0;
}

static uint64_t wrap_is_path_directory(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    return ::IsPathDirectory(a0) ? 1 : 0;
}

static uint64_t wrap_is_path_absolute(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    return ::IsPathAbsolute(a0) ? 1 : 0;
}

static uint64_t wrap_is_file_name_valid(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    return ::IsFileNameValid(a0) ? 1 : 0;
}

static uint64_t wrap_is_file_dropped(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    return ::IsFileDropped() ? 1 : 0;
}

static uint64_t wrap_get_directory_file_count(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    return static_cast<uint64_t>(::GetDirectoryFileCount(a0));
}

static uint64_t wrap_get_directory_file_count_ex(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    std::string s_a1 = read_solix_chars(vm, args[1]);
    const char *a1 = s_a1.c_str();
    bool a2 = (args[2] != 0);
    return static_cast<uint64_t>(::GetDirectoryFileCountEx(a0, a1, a2));
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

static uint64_t wrap_is_key_pressed(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    return ::IsKeyPressed(a0) ? 1 : 0;
}

static uint64_t wrap_is_key_pressed_repeat(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    return ::IsKeyPressedRepeat(a0) ? 1 : 0;
}

static uint64_t wrap_is_key_down(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    return ::IsKeyDown(a0) ? 1 : 0;
}

static uint64_t wrap_is_key_released(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    return ::IsKeyReleased(a0) ? 1 : 0;
}

static uint64_t wrap_is_key_up(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    return ::IsKeyUp(a0) ? 1 : 0;
}

static uint64_t wrap_get_key_pressed(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    return static_cast<uint64_t>(::GetKeyPressed());
}

static uint64_t wrap_get_char_pressed(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    return static_cast<uint64_t>(::GetCharPressed());
}

static uint64_t wrap_get_key_name(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    ::GetKeyName(a0);
    return 0;
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
    return ::IsGamepadAvailable(a0) ? 1 : 0;
}

static uint64_t wrap_get_gamepad_name(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    ::GetGamepadName(a0);
    return 0;
}

static uint64_t wrap_is_gamepad_button_pressed(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    int a1 = static_cast<int>(args[1]);
    return ::IsGamepadButtonPressed(a0, a1) ? 1 : 0;
}

static uint64_t wrap_is_gamepad_button_down(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    int a1 = static_cast<int>(args[1]);
    return ::IsGamepadButtonDown(a0, a1) ? 1 : 0;
}

static uint64_t wrap_is_gamepad_button_released(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    int a1 = static_cast<int>(args[1]);
    return ::IsGamepadButtonReleased(a0, a1) ? 1 : 0;
}

static uint64_t wrap_is_gamepad_button_up(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    int a1 = static_cast<int>(args[1]);
    return ::IsGamepadButtonUp(a0, a1) ? 1 : 0;
}

static uint64_t wrap_get_gamepad_button_pressed(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    return static_cast<uint64_t>(::GetGamepadButtonPressed());
}

static uint64_t wrap_get_gamepad_axis_count(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    return static_cast<uint64_t>(::GetGamepadAxisCount(a0));
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
    return static_cast<uint64_t>(::SetGamepadMappings(a0));
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
    return ::IsMouseButtonPressed(a0) ? 1 : 0;
}

static uint64_t wrap_is_mouse_button_down(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    return ::IsMouseButtonDown(a0) ? 1 : 0;
}

static uint64_t wrap_is_mouse_button_released(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    return ::IsMouseButtonReleased(a0) ? 1 : 0;
}

static uint64_t wrap_is_mouse_button_up(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    return ::IsMouseButtonUp(a0) ? 1 : 0;
}

static uint64_t wrap_get_mouse_x(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    return static_cast<uint64_t>(::GetMouseX());
}

static uint64_t wrap_get_mouse_y(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    return static_cast<uint64_t>(::GetMouseY());
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
    return static_cast<uint64_t>(::GetTouchX());
}

static uint64_t wrap_get_touch_y(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    return static_cast<uint64_t>(::GetTouchY());
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
    return static_cast<uint64_t>(::GetTouchPointId(a0));
}

static uint64_t wrap_get_touch_point_count(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    return static_cast<uint64_t>(::GetTouchPointCount());
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
    return ::IsGestureDetected(a0) ? 1 : 0;
}

static uint64_t wrap_get_gesture_detected(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    return static_cast<uint64_t>(::GetGestureDetected());
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
    return ::CheckCollisionRecs(a0, a1) ? 1 : 0;
}

static uint64_t wrap_check_collision_circles(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector2 a0 = unpack_vector2(vm, args[0]);
    float a1 = unpack_f32_arg(args[1]);
    ::Vector2 a2 = unpack_vector2(vm, args[2]);
    float a3 = unpack_f32_arg(args[3]);
    return ::CheckCollisionCircles(a0, a1, a2, a3) ? 1 : 0;
}

static uint64_t wrap_check_collision_circle_rec(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector2 a0 = unpack_vector2(vm, args[0]);
    float a1 = unpack_f32_arg(args[1]);
    ::Rectangle a2 = unpack_rectangle(vm, args[2]);
    return ::CheckCollisionCircleRec(a0, a1, a2) ? 1 : 0;
}

static uint64_t wrap_check_collision_circle_line(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector2 a0 = unpack_vector2(vm, args[0]);
    float a1 = unpack_f32_arg(args[1]);
    ::Vector2 a2 = unpack_vector2(vm, args[2]);
    ::Vector2 a3 = unpack_vector2(vm, args[3]);
    return ::CheckCollisionCircleLine(a0, a1, a2, a3) ? 1 : 0;
}

static uint64_t wrap_check_collision_point_rec(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector2 a0 = unpack_vector2(vm, args[0]);
    ::Rectangle a1 = unpack_rectangle(vm, args[1]);
    return ::CheckCollisionPointRec(a0, a1) ? 1 : 0;
}

static uint64_t wrap_check_collision_point_circle(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector2 a0 = unpack_vector2(vm, args[0]);
    ::Vector2 a1 = unpack_vector2(vm, args[1]);
    float a2 = unpack_f32_arg(args[2]);
    return ::CheckCollisionPointCircle(a0, a1, a2) ? 1 : 0;
}

static uint64_t wrap_check_collision_point_triangle(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector2 a0 = unpack_vector2(vm, args[0]);
    ::Vector2 a1 = unpack_vector2(vm, args[1]);
    ::Vector2 a2 = unpack_vector2(vm, args[2]);
    ::Vector2 a3 = unpack_vector2(vm, args[3]);
    return ::CheckCollisionPointTriangle(a0, a1, a2, a3) ? 1 : 0;
}

static uint64_t wrap_check_collision_point_line(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector2 a0 = unpack_vector2(vm, args[0]);
    ::Vector2 a1 = unpack_vector2(vm, args[1]);
    ::Vector2 a2 = unpack_vector2(vm, args[2]);
    int a3 = static_cast<int>(args[3]);
    return ::CheckCollisionPointLine(a0, a1, a2, a3) ? 1 : 0;
}

static uint64_t wrap_get_collision_rec(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Rectangle a0 = unpack_rectangle(vm, args[0]);
    ::Rectangle a1 = unpack_rectangle(vm, args[1]);
    auto res = ::GetCollisionRec(a0, a1);
    pack_rectangle(vm, args[2], res);
    return 0;
}

static uint64_t wrap_color_is_equal(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Color a0 = unpack_color(vm, args[0]);
    ::Color a1 = unpack_color(vm, args[1]);
    return ::ColorIsEqual(a0, a1) ? 1 : 0;
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
    return static_cast<uint64_t>(::ColorToInt(a0));
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

static uint64_t wrap_get_pixel_data_size(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    int a1 = static_cast<int>(args[1]);
    int a2 = static_cast<int>(args[2]);
    return static_cast<uint64_t>(::GetPixelDataSize(a0, a1, a2));
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
    return static_cast<uint64_t>(::MeasureText(a0, a1));
}

static uint64_t wrap_get_codepoint_count(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    return static_cast<uint64_t>(::GetCodepointCount(a0));
}

static uint64_t wrap_text_is_equal(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    std::string s_a1 = read_solix_chars(vm, args[1]);
    const char *a1 = s_a1.c_str();
    return ::TextIsEqual(a0, a1) ? 1 : 0;
}

static uint64_t wrap_text_length(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    return static_cast<uint64_t>(::TextLength(a0));
}

static uint64_t wrap_text_subtext(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    int a1 = static_cast<int>(args[1]);
    int a2 = static_cast<int>(args[2]);
    ::TextSubtext(a0, a1, a2);
    return 0;
}

static uint64_t wrap_text_remove_spaces(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    ::TextRemoveSpaces(a0);
    return 0;
}

static uint64_t wrap_text_find_index(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    std::string s_a1 = read_solix_chars(vm, args[1]);
    const char *a1 = s_a1.c_str();
    return static_cast<uint64_t>(::TextFindIndex(a0, a1));
}

static uint64_t wrap_text_to_integer(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s_a0 = read_solix_chars(vm, args[0]);
    const char *a0 = s_a0.c_str();
    return static_cast<uint64_t>(::TextToInteger(a0));
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

static uint64_t wrap_draw_grid(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int a0 = static_cast<int>(args[0]);
    float a1 = unpack_f32_arg(args[1]);
    ::DrawGrid(a0, a1);
    return 0;
}

static uint64_t wrap_check_collision_spheres(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ::Vector3 a0 = unpack_vector3(vm, args[0]);
    float a1 = unpack_f32_arg(args[1]);
    ::Vector3 a2 = unpack_vector3(vm, args[2]);
    float a3 = unpack_f32_arg(args[3]);
    return ::CheckCollisionSpheres(a0, a1, a2, a3) ? 1 : 0;
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
    return ::IsAudioDeviceReady() ? 1 : 0;
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
    registry.register_function("Raylib_native_end_mode2d", wrap_end_mode2d);
    registry.register_function("raylib_Raylib_native_end_mode2d", wrap_end_mode2d);
    registry.register_function("native_end_mode2d", wrap_end_mode2d);
    registry.register_function("Raylib_native_end_mode3d", wrap_end_mode3d);
    registry.register_function("raylib_Raylib_native_end_mode3d", wrap_end_mode3d);
    registry.register_function("native_end_mode3d", wrap_end_mode3d);
    registry.register_function("Raylib_native_end_texture_mode", wrap_end_texture_mode);
    registry.register_function("raylib_Raylib_native_end_texture_mode", wrap_end_texture_mode);
    registry.register_function("native_end_texture_mode", wrap_end_texture_mode);
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
    registry.register_function("Raylib_native_end_vr_stereo_mode", wrap_end_vr_stereo_mode);
    registry.register_function("raylib_Raylib_native_end_vr_stereo_mode", wrap_end_vr_stereo_mode);
    registry.register_function("native_end_vr_stereo_mode", wrap_end_vr_stereo_mode);
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
    registry.register_function("Raylib_native_is_file_dropped", wrap_is_file_dropped);
    registry.register_function("raylib_Raylib_native_is_file_dropped", wrap_is_file_dropped);
    registry.register_function("native_is_file_dropped", wrap_is_file_dropped);
    registry.register_function("Raylib_native_get_directory_file_count", wrap_get_directory_file_count);
    registry.register_function("raylib_Raylib_native_get_directory_file_count", wrap_get_directory_file_count);
    registry.register_function("native_get_directory_file_count", wrap_get_directory_file_count);
    registry.register_function("Raylib_native_get_directory_file_count_ex", wrap_get_directory_file_count_ex);
    registry.register_function("raylib_Raylib_native_get_directory_file_count_ex", wrap_get_directory_file_count_ex);
    registry.register_function("native_get_directory_file_count_ex", wrap_get_directory_file_count_ex);
    registry.register_function("Raylib_native_set_automation_event_base_frame", wrap_set_automation_event_base_frame);
    registry.register_function("raylib_Raylib_native_set_automation_event_base_frame", wrap_set_automation_event_base_frame);
    registry.register_function("native_set_automation_event_base_frame", wrap_set_automation_event_base_frame);
    registry.register_function("Raylib_native_start_automation_event_recording", wrap_start_automation_event_recording);
    registry.register_function("raylib_Raylib_native_start_automation_event_recording", wrap_start_automation_event_recording);
    registry.register_function("native_start_automation_event_recording", wrap_start_automation_event_recording);
    registry.register_function("Raylib_native_stop_automation_event_recording", wrap_stop_automation_event_recording);
    registry.register_function("raylib_Raylib_native_stop_automation_event_recording", wrap_stop_automation_event_recording);
    registry.register_function("native_stop_automation_event_recording", wrap_stop_automation_event_recording);
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
    registry.register_function("Raylib_native_get_collision_rec", wrap_get_collision_rec);
    registry.register_function("raylib_Raylib_native_get_collision_rec", wrap_get_collision_rec);
    registry.register_function("native_get_collision_rec", wrap_get_collision_rec);
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
    registry.register_function("Raylib_native_get_pixel_data_size", wrap_get_pixel_data_size);
    registry.register_function("raylib_Raylib_native_get_pixel_data_size", wrap_get_pixel_data_size);
    registry.register_function("native_get_pixel_data_size", wrap_get_pixel_data_size);
    registry.register_function("Raylib_native_draw_fps", wrap_draw_fps);
    registry.register_function("raylib_Raylib_native_draw_fps", wrap_draw_fps);
    registry.register_function("native_draw_fps", wrap_draw_fps);
    registry.register_function("Raylib_native_draw_text", wrap_draw_text);
    registry.register_function("raylib_Raylib_native_draw_text", wrap_draw_text);
    registry.register_function("native_draw_text", wrap_draw_text);
    registry.register_function("Raylib_native_set_text_line_spacing", wrap_set_text_line_spacing);
    registry.register_function("raylib_Raylib_native_set_text_line_spacing", wrap_set_text_line_spacing);
    registry.register_function("native_set_text_line_spacing", wrap_set_text_line_spacing);
    registry.register_function("Raylib_native_measure_text", wrap_measure_text);
    registry.register_function("raylib_Raylib_native_measure_text", wrap_measure_text);
    registry.register_function("native_measure_text", wrap_measure_text);
    registry.register_function("Raylib_native_get_codepoint_count", wrap_get_codepoint_count);
    registry.register_function("raylib_Raylib_native_get_codepoint_count", wrap_get_codepoint_count);
    registry.register_function("native_get_codepoint_count", wrap_get_codepoint_count);
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
    registry.register_function("Raylib_native_text_find_index", wrap_text_find_index);
    registry.register_function("raylib_Raylib_native_text_find_index", wrap_text_find_index);
    registry.register_function("native_text_find_index", wrap_text_find_index);
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
    registry.register_function("Raylib_native_draw_grid", wrap_draw_grid);
    registry.register_function("raylib_Raylib_native_draw_grid", wrap_draw_grid);
    registry.register_function("native_draw_grid", wrap_draw_grid);
    registry.register_function("Raylib_native_check_collision_spheres", wrap_check_collision_spheres);
    registry.register_function("raylib_Raylib_native_check_collision_spheres", wrap_check_collision_spheres);
    registry.register_function("native_check_collision_spheres", wrap_check_collision_spheres);
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
    registry.register_function("Raylib_native_set_audio_stream_buffer_size_default", wrap_set_audio_stream_buffer_size_default);
    registry.register_function("raylib_Raylib_native_set_audio_stream_buffer_size_default", wrap_set_audio_stream_buffer_size_default);
    registry.register_function("native_set_audio_stream_buffer_size_default", wrap_set_audio_stream_buffer_size_default);
}
