/**
 * @file gdext_c_signals.c
 * @brief Signal connection helpers and Input singleton integration for Go
 * 
 * TDD #160: Real implementations for Input singleton methods
 * Uses cached method binds for optimal performance (ptrcall).
 */

#include "gdext_c_core.h"
#include "gdext_c_generated.h"  // For generated singleton access
#include <stdio.h>
#include <stdint.h>
#include <string.h>

// ============================================================================
// Signal connection stubs (not yet implemented)
// ============================================================================

uintptr_t gdext_connect_signal(uintptr_t object_ptr, const char* signal_name) {
    (void)object_ptr; (void)signal_name;
    return 0;
}

int gdext_disconnect_signal(uintptr_t object_ptr, const char* signal_name) {
    (void)object_ptr; (void)signal_name;
    return 0;
}

int gdext_emit_signal(uintptr_t object_ptr, const char* signal_name, void** args, uintptr_t arg_count) {
    (void)object_ptr; (void)signal_name; (void)args; (void)arg_count;
    return 0;
}

void gdext_init_signals(void* callback) {
    (void)callback;
}

// ============================================================================
// Input singleton - cached state for high-performance ptrcall
// ============================================================================

// Cached Input singleton
static GDExtensionObjectPtr g_input_singleton = NULL;
static int g_input_init_attempted = 0;

// Cached method binds (looked up once, used every frame)
static GDExtensionMethodBindPtr g_is_key_pressed_mb = NULL;
static GDExtensionMethodBindPtr g_is_mouse_button_pressed_mb = NULL;
static GDExtensionMethodBindPtr g_is_action_pressed_mb = NULL;
static GDExtensionMethodBindPtr g_is_action_just_pressed_mb = NULL;
static GDExtensionMethodBindPtr g_is_action_just_released_mb = NULL;
static GDExtensionMethodBindPtr g_get_action_strength_mb = NULL;
static GDExtensionMethodBindPtr g_is_joy_button_pressed_mb = NULL;
static GDExtensionMethodBindPtr g_get_joy_axis_mb = NULL;

// Cached StringName destructor (for proper cleanup of per-frame StringNames)
static GDExtensionPtrDestructor g_string_name_destructor = NULL;

// Method hashes from extension_api.json (Godot 4.2)
#define HASH_IS_KEY_PRESSED          1938909964
#define HASH_IS_MOUSE_BUTTON_PRESSED 1821097125
#define HASH_IS_ACTION_PRESSED       1558498928
#define HASH_IS_ACTION_JUST_PRESSED  1558498928
#define HASH_IS_ACTION_JUST_RELEASED 1558498928
#define HASH_GET_ACTION_STRENGTH     801543509
#define HASH_IS_JOY_BUTTON_PRESSED   787208542
#define HASH_GET_JOY_AXIS            4063175957

/**
 * @brief Lazily initialize the Input singleton and cache method binds.
 * Called on first use. Uses ptrcall for maximum performance.
 *
 * IMPORTANT: String literals (e.g. "Input", "is_key_pressed") use p_is_static=1
 * because they live in the binary's read-only section for the process lifetime.
 * Per-frame action names from Go use p_is_static=0 because Go frees the CString
 * after the call. With p_is_static=1, Godot would store a dangling pointer.
 */
static void ensure_input_initialized(void) {
    if (g_input_init_attempted) return;
    g_input_init_attempted = 1;
    
    if (!gdext_c_is_initialized()) return;
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    if (!iface || !iface->global_get_singleton || !iface->classdb_get_method_bind) return;
    
    // Cache StringName destructor for per-frame cleanup
    // GDEXTENSION_VARIANT_TYPE_STRING_NAME = 21
    if (iface->variant_get_ptr_destructor) {
        g_string_name_destructor = iface->variant_get_ptr_destructor(21);
    }
    
    // Get Input singleton (p_is_static=1: "Input" is a string literal)
    char input_sn[64] = {0};
    iface->string_name_new_with_latin1_chars(input_sn, "Input", 1);
    g_input_singleton = iface->global_get_singleton(input_sn);
    
    if (!g_input_singleton) {
        fprintf(stderr, "[gdext-c] ⚠️ Input singleton not available yet\n");
        g_input_init_attempted = 0; // Allow retry
        return;
    }
    
    // Class StringName for method bind lookup (p_is_static=1: string literals)
    char class_sn[64] = {0};
    iface->string_name_new_with_latin1_chars(class_sn, "Input", 1);
    
    // Cache all method binds (p_is_static=1: method name string literals)
    char method_sn[64] = {0};
    
    iface->string_name_new_with_latin1_chars(method_sn, "is_key_pressed", 1);
    g_is_key_pressed_mb = iface->classdb_get_method_bind(class_sn, method_sn, HASH_IS_KEY_PRESSED);
    
    memset(method_sn, 0, sizeof(method_sn));
    iface->string_name_new_with_latin1_chars(method_sn, "is_mouse_button_pressed", 1);
    g_is_mouse_button_pressed_mb = iface->classdb_get_method_bind(class_sn, method_sn, HASH_IS_MOUSE_BUTTON_PRESSED);
    
    memset(method_sn, 0, sizeof(method_sn));
    iface->string_name_new_with_latin1_chars(method_sn, "is_action_pressed", 1);
    g_is_action_pressed_mb = iface->classdb_get_method_bind(class_sn, method_sn, HASH_IS_ACTION_PRESSED);
    
    memset(method_sn, 0, sizeof(method_sn));
    iface->string_name_new_with_latin1_chars(method_sn, "is_action_just_pressed", 1);
    g_is_action_just_pressed_mb = iface->classdb_get_method_bind(class_sn, method_sn, HASH_IS_ACTION_JUST_PRESSED);
    
    memset(method_sn, 0, sizeof(method_sn));
    iface->string_name_new_with_latin1_chars(method_sn, "is_action_just_released", 1);
    g_is_action_just_released_mb = iface->classdb_get_method_bind(class_sn, method_sn, HASH_IS_ACTION_JUST_RELEASED);
    
    memset(method_sn, 0, sizeof(method_sn));
    iface->string_name_new_with_latin1_chars(method_sn, "get_action_strength", 1);
    g_get_action_strength_mb = iface->classdb_get_method_bind(class_sn, method_sn, HASH_GET_ACTION_STRENGTH);
    
    memset(method_sn, 0, sizeof(method_sn));
    iface->string_name_new_with_latin1_chars(method_sn, "is_joy_button_pressed", 1);
    g_is_joy_button_pressed_mb = iface->classdb_get_method_bind(class_sn, method_sn, HASH_IS_JOY_BUTTON_PRESSED);
    
    memset(method_sn, 0, sizeof(method_sn));
    iface->string_name_new_with_latin1_chars(method_sn, "get_joy_axis", 1);
    g_get_joy_axis_mb = iface->classdb_get_method_bind(class_sn, method_sn, HASH_GET_JOY_AXIS);
    
    fprintf(stderr, "[gdext-c] ✅ Input system initialized: action_pressed=%p, just_pressed=%p, key_pressed=%p\n",
            (void*)g_is_action_pressed_mb, (void*)g_is_action_just_pressed_mb, (void*)g_is_key_pressed_mb);
}

// ============================================================================
// Input function implementations using cached ptrcall
// ============================================================================

int gdext_is_key_pressed(int keycode) {
    ensure_input_initialized();
    if (!g_input_singleton || !g_is_key_pressed_mb) return 0;
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    
    // is_key_pressed(keycode: Key) -> bool
    // Key is an enum, passed as int64_t for ptrcall
    int64_t key = (int64_t)keycode;
    const GDExtensionConstTypePtr args[1] = { &key };
    
    GDExtensionBool result = 0;
    iface->object_method_bind_ptrcall(g_is_key_pressed_mb, g_input_singleton, args, &result);
    
    return (int)result;
}

int gdext_is_mouse_button_pressed(int button) {
    ensure_input_initialized();
    if (!g_input_singleton || !g_is_mouse_button_pressed_mb) return 0;
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    
    // is_mouse_button_pressed(button: MouseButton) -> bool
    int64_t btn = (int64_t)button;
    const GDExtensionConstTypePtr args[1] = { &btn };
    
    GDExtensionBool result = 0;
    iface->object_method_bind_ptrcall(g_is_mouse_button_pressed_mb, g_input_singleton, args, &result);
    
    return (int)result;
}

int gdext_is_action_pressed(const char* action) {
    ensure_input_initialized();
    if (!g_input_singleton || !g_is_action_pressed_mb) return 0;
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    
    // is_action_pressed(action: StringName, exact_match: bool = false) -> bool
    // CRITICAL: p_is_static=0 because `action` is a temporary CString from Go.
    // With p_is_static=1, Godot stores the pointer without copying → dangling after Go frees it.
    char action_sn[64] = {0};
    iface->string_name_new_with_latin1_chars(action_sn, action, 0);
    
    GDExtensionBool exact_match = 0;
    const GDExtensionConstTypePtr args[2] = { action_sn, &exact_match };
    
    GDExtensionBool result = 0;
    iface->object_method_bind_ptrcall(g_is_action_pressed_mb, g_input_singleton, args, &result);
    
    // Destroy the StringName to prevent leaks (p_is_static=0 → ref-counted)
    if (g_string_name_destructor) g_string_name_destructor(action_sn);
    
    return (int)result;
}

int gdext_is_action_just_pressed(const char* action) {
    ensure_input_initialized();
    if (!g_input_singleton || !g_is_action_just_pressed_mb) return 0;
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    
    // is_action_just_pressed(action: StringName, exact_match: bool = false) -> bool
    char action_sn[64] = {0};
    iface->string_name_new_with_latin1_chars(action_sn, action, 0);
    
    GDExtensionBool exact_match = 0;
    const GDExtensionConstTypePtr args[2] = { action_sn, &exact_match };
    
    GDExtensionBool result = 0;
    iface->object_method_bind_ptrcall(g_is_action_just_pressed_mb, g_input_singleton, args, &result);
    
    if (g_string_name_destructor) g_string_name_destructor(action_sn);
    
    return (int)result;
}

int gdext_is_action_just_released(const char* action) {
    ensure_input_initialized();
    if (!g_input_singleton || !g_is_action_just_released_mb) return 0;
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    
    // is_action_just_released(action: StringName, exact_match: bool = false) -> bool
    char action_sn[64] = {0};
    iface->string_name_new_with_latin1_chars(action_sn, action, 0);
    
    GDExtensionBool exact_match = 0;
    const GDExtensionConstTypePtr args[2] = { action_sn, &exact_match };
    
    GDExtensionBool result = 0;
    iface->object_method_bind_ptrcall(g_is_action_just_released_mb, g_input_singleton, args, &result);
    
    if (g_string_name_destructor) g_string_name_destructor(action_sn);
    
    return (int)result;
}

float gdext_get_action_strength(const char* action) {
    ensure_input_initialized();
    if (!g_input_singleton || !g_get_action_strength_mb) return 0.0f;
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    
    // get_action_strength(action: StringName, exact_match: bool = false) -> float
    char action_sn[64] = {0};
    iface->string_name_new_with_latin1_chars(action_sn, action, 0);
    
    GDExtensionBool exact_match = 0;
    const GDExtensionConstTypePtr args[2] = { action_sn, &exact_match };
    
    // Godot float is double in ptrcall convention
    double result = 0.0;
    iface->object_method_bind_ptrcall(g_get_action_strength_mb, g_input_singleton, args, &result);
    
    if (g_string_name_destructor) g_string_name_destructor(action_sn);
    
    return (float)result;
}

void* gdext_get_mouse_position() {
    // TODO: Implement via Viewport.get_mouse_position() 
    // Input singleton doesn't have get_mouse_position directly
    return NULL;
}

int gdext_is_joy_button_pressed(int device, int button) {
    ensure_input_initialized();
    if (!g_input_singleton || !g_is_joy_button_pressed_mb) return 0;
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    
    // is_joy_button_pressed(device: int, button: JoyButton) -> bool
    int64_t dev = (int64_t)device;
    int64_t btn = (int64_t)button;
    const GDExtensionConstTypePtr args[2] = { &dev, &btn };
    
    GDExtensionBool result = 0;
    iface->object_method_bind_ptrcall(g_is_joy_button_pressed_mb, g_input_singleton, args, &result);
    
    return (int)result;
}

float gdext_get_joy_axis(int device, int axis) {
    ensure_input_initialized();
    if (!g_input_singleton || !g_get_joy_axis_mb) return 0.0f;
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    
    // get_joy_axis(device: int, axis: JoyAxis) -> float
    int64_t dev = (int64_t)device;
    int64_t ax = (int64_t)axis;
    const GDExtensionConstTypePtr args[2] = { &dev, &ax };
    
    double result = 0.0;
    iface->object_method_bind_ptrcall(g_get_joy_axis_mb, g_input_singleton, args, &result);
    
    return (float)result;
}

// ============================================================================
// Singleton access functions
// ============================================================================

void* gdext_get_engine_singleton() {
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    if (!iface) {
        return NULL;
    }
    
    // p_is_static=1: "Engine" is a string literal
    char engine_sn[64] = {0};
    iface->string_name_new_with_latin1_chars(engine_sn, "Engine", 1);
    
    return iface->global_get_singleton(engine_sn);
}

void* gdext_get_singleton_by_name(const char* name) {
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    if (!iface) {
        return NULL;
    }
    
    // p_is_static=0: name comes from caller (may be temporary)
    char singleton_sn[64] = {0};
    iface->string_name_new_with_latin1_chars(singleton_sn, name, 0);
    
    void* result = iface->global_get_singleton(singleton_sn);
    
    // Destroy the StringName to prevent leak
    if (g_string_name_destructor) g_string_name_destructor(singleton_sn);
    
    return result;
}

// ============================================================================
// Scene/resource function stubs
// ============================================================================

void* gdext_get_scene_tree_from_existing_objects() {
    return NULL;
}

void* gdext_load_resource(const char* path, const char* type_hint) {
    (void)path;
    (void)type_hint;
    return NULL;
}

void* gdext_preload_resource(const char* path) {
    (void)path;
    return NULL;
}

// ============================================================================
// Logging function stubs
// ============================================================================

void gdext_log_message(const char* message) {
    printf("[Godot] %s\n", message);
}

void gdext_log_warning(const char* message) {
    fprintf(stderr, "[Godot Warning] %s\n", message);
}

void gdext_log_error(const char* message) {
    fprintf(stderr, "[Godot Error] %s\n", message);
}

// ============================================================================
// Input action registration stubs
// ============================================================================

int gdext_register_input_action_key(const char* action, int key_code) {
    (void)action;
    (void)key_code;
    return 0;
}

int gdext_register_input_action_mouse(const char* action, int button_code) {
    (void)action;
    (void)button_code;
    return 0;
}

int gdext_validate_input_action(const char* action, int expected_key_code) {
    (void)action;
    (void)expected_key_code;
    return 1;
}

// ============================================================================
// Scene tree traversal stubs
// ============================================================================

int gdext_is_scene_tree_traversing() {
    return 0;
}

void gdext_set_scene_tree_traversing(int traversing) {
    (void)traversing;
}

// ============================================================================
// Signal helper stubs
// ============================================================================

void gdext_free_signal_args(void** args, uintptr_t arg_count) {
    (void)args;
    (void)arg_count;
}
