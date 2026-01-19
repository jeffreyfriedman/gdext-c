/**
 * @file gdext_c_signals.c
 * @brief Signal connection helpers and input stubs for Go
 * 
 * TDD #160: Stub implementations for signal and input functions
 */

#include "gdext_c_core.h"
#include "gdext_c_generated.h"  // For generated singleton access
#include <stdio.h>
#include <stdint.h>

/**
 * @brief Connect a signal (stub for now)
 * TDD #160: Returns 0 (not yet implemented)
 */
uintptr_t gdext_connect_signal(uintptr_t object_ptr, const char* signal_name) {
    fprintf(stderr, "[gdext-c] ⚠️  TDD #160: gdext_connect_signal('%s') - stub\n", signal_name);
    (void)object_ptr;
    return 0; // Return 0 = no callback ID
}

/**
 * @brief Disconnect a signal (stub for now)
 */
int gdext_disconnect_signal(uintptr_t object_ptr, const char* signal_name) {
    fprintf(stderr, "[gdext-c] ⚠️  TDD #160: gdext_disconnect_signal('%s') - stub\n", signal_name);
    (void)object_ptr;
    return 0;
}

/**
 * @brief Emit a signal (stub for now)
 */
int gdext_emit_signal(uintptr_t object_ptr, const char* signal_name, void** args, uintptr_t arg_count) {
    fprintf(stderr, "[gdext-c] ⚠️  TDD #160: gdext_emit_signal('%s') - stub\n", signal_name);
    (void)object_ptr;
    (void)args;
    (void)arg_count;
    return 0;
}

/**
 * @brief Initialize signals (stub for now)
 */
void gdext_init_signals(void* callback) {
    fprintf(stderr, "[gdext-c] ⚠️  TDD #160: gdext_init_signals() - stub\n");
    (void)callback;
}

// ============================================================================
// TDD #160: Input function stubs
// ============================================================================

int gdext_is_key_pressed(int keycode) {
    (void)keycode;
    return 0;
}

int gdext_is_mouse_button_pressed(int button) {
    (void)button;
    return 0;
}

int gdext_is_action_pressed(const char* action) {
    (void)action;
    return 0;
}

int gdext_is_action_just_pressed(const char* action) {
    (void)action;
    return 0;
}

int gdext_is_action_just_released(const char* action) {
    (void)action;
    return 0;
}

float gdext_get_action_strength(const char* action) {
    (void)action;
    return 0.0f;
}

void* gdext_get_mouse_position() {
    return NULL;
}

int gdext_is_joy_button_pressed(int device, int button) {
    (void)device;
    (void)button;
    return 0;
}

float gdext_get_joy_axis(int device, int axis) {
    (void)device;
    (void)axis;
    return 0.0f;
}

// ============================================================================
// TDD #160: Singleton access functions
// ============================================================================

void* gdext_get_engine_singleton() {
    // Engine singleton is accessed via global_get_singleton
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    if (!iface) {
        return NULL;
    }
    
    // Create StringName for "Engine"
    char engine_sn[64] = {0};
    iface->string_name_new_with_latin1_chars(engine_sn, "Engine", 0);
    
    // Get singleton
    return iface->global_get_singleton(engine_sn);
}

void* gdext_get_singleton_by_name(const char* name) {
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    if (!iface) {
        return NULL;
    }
    
    // Create StringName for singleton name
    char singleton_sn[64] = {0};
    iface->string_name_new_with_latin1_chars(singleton_sn, name, 0);
    
    // Get singleton
    return iface->global_get_singleton(singleton_sn);
}

// ============================================================================
// TDD #160: Scene/resource function stubs
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
// TDD #160: Logging function stubs
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
// TDD #160: Input action registration stubs
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
    return 1; // Return success
}

// ============================================================================
// TDD #160: Scene tree traversal stubs
// ============================================================================

int gdext_is_scene_tree_traversing() {
    return 0;
}

void gdext_set_scene_tree_traversing(int traversing) {
    (void)traversing;
}

// ============================================================================
// TDD #160: Signal helper stubs
// ============================================================================

void gdext_free_signal_args(void** args, uintptr_t arg_count) {
    (void)args;
    (void)arg_count;
}

