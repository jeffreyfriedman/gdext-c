/**
 * @file gdext_c_signals.c
 * @brief Signal connection and Input singleton integration for Go
 * 
 * TDD #160: Real implementations for signal connect/disconnect/emit
 * and Input singleton methods. Uses cached method binds for optimal
 * performance (ptrcall).
 *
 * Signal system architecture:
 *   Go callback → goSignalCallbackBridge(callbackID, argCount, args)
 *                    ↑
 *   C bridge    → on_signal_callback() invokes stored GoSignalCallbackFn
 *                    ↑
 *   Godot       → Callable fires when signal is emitted
 *
 * Each connection creates a Callable with userdata=callbackID.
 * When the signal fires, Godot calls our Callable, which calls back to Go.
 */

#include "../core/gdext_c_core.h"
#include "gdext_c_generated.h"  // For generated singleton access
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

// ============================================================================
// Signal Connection Registry
// ============================================================================

// Function pointer type for the Go callback bridge
typedef void (*GoSignalCallbackFn)(uintptr_t callback_id, uintptr_t arg_count, void** args);

// Stored Go callback bridge (set by gdext_init_signals)
static GoSignalCallbackFn g_go_signal_callback = NULL;

// Maximum number of concurrent signal connections
#define MAX_SIGNAL_CONNECTIONS 256

// Callable storage size (must hold a full Godot Callable)
#define CALLABLE_STORAGE_SIZE 256

// Signal connection entry in the registry
typedef struct {
    int active;                              // 1 if this slot is in use
    uintptr_t callback_id;                   // Unique ID for this connection
    uintptr_t object_ptr;                    // The Godot object this is connected to
    char signal_name[128];                   // Signal name (e.g., "pressed")
    char callable_storage[CALLABLE_STORAGE_SIZE]; // Callable data (must persist!)
} SignalConnection;

// Global connection registry
static SignalConnection g_connections[MAX_SIGNAL_CONNECTIONS];
static uintptr_t g_next_callback_id = 1;  // Start at 1 (0 = error)
static int g_signal_system_initialized = 0;

// Cached Object.connect() and Object.disconnect() method binds
static GDExtensionMethodBindPtr g_object_connect_mb = NULL;
static GDExtensionMethodBindPtr g_object_disconnect_mb = NULL;

// Cached callable_custom_create2 function pointer
static GDExtensionInterfaceCallableCustomCreate2 g_callable_create = NULL;

// Cached StringName destructor
static GDExtensionPtrDestructor g_sn_destructor = NULL;

// ============================================================================
// Signal Handler (called by Godot when a signal fires)
// ============================================================================

/**
 * @brief Callable is_valid function - must return TRUE or Godot won't call it
 */
static GDExtensionBool on_signal_is_valid(void* p_userdata) {
    (void)p_userdata;
    return 1;  // Always valid
}

/**
 * @brief Signal handler called by Godot when a connected signal fires.
 *
 * This is the call_func for our custom Callable. Godot calls this with
 * the signal arguments, and we forward them to Go via the stored callback.
 *
 * @param p_userdata  Our callback_id (cast from uintptr_t)
 * @param p_args      Array of Variant pointers (signal arguments)
 * @param p_argument_count Number of arguments
 * @param r_return    Return value (unused for signals)
 * @param r_error     Error output
 */
static void on_signal_callback(
    void* p_userdata,
    const GDExtensionConstVariantPtr* p_args,
    GDExtensionInt p_argument_count,
    GDExtensionVariantPtr r_return,
    GDExtensionCallError* r_error
) {
    (void)r_return;

    // Set no error
    if (r_error) {
        r_error->error = GDEXTENSION_CALL_OK;
    }

    uintptr_t callback_id = (uintptr_t)p_userdata;

    // Forward to Go callback bridge
    if (g_go_signal_callback) {
        g_go_signal_callback(callback_id, (uintptr_t)p_argument_count, (void**)p_args);
    }
}

// ============================================================================
// Signal System Initialization
// ============================================================================

/**
 * @brief Lazily initialize signal system internals.
 *
 * Caches Object.connect() and Object.disconnect() method binds
 * and the callable_custom_create2 function pointer.
 */
static void ensure_signal_system_initialized(void) {
    if (g_signal_system_initialized) return;

    if (!gdext_c_is_initialized()) return;

    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    if (!iface) return;

    // Cache StringName destructor (type 21 = STRING_NAME)
    if (iface->variant_get_ptr_destructor) {
        g_sn_destructor = iface->variant_get_ptr_destructor(21);
    }

    // Cache Object.connect() method bind
    // Signature: connect(signal: StringName, callable: Callable, flags: int = 0) -> Error
    // Hash: 1518946055
    {
        char class_sn[64] = {0};
        char method_sn[64] = {0};
        iface->string_name_new_with_latin1_chars(class_sn, "Object", 1);
        iface->string_name_new_with_latin1_chars(method_sn, "connect", 1);
        g_object_connect_mb = iface->classdb_get_method_bind(class_sn, method_sn, 1518946055);
    }

    // Cache Object.disconnect() method bind
    // Signature: disconnect(signal: StringName, callable: Callable)
    // Hash: 1874754934
    {
        char class_sn[64] = {0};
        char method_sn[64] = {0};
        iface->string_name_new_with_latin1_chars(class_sn, "Object", 1);
        iface->string_name_new_with_latin1_chars(method_sn, "disconnect", 1);
        g_object_disconnect_mb = iface->classdb_get_method_bind(class_sn, method_sn, 1874754934);
    }

    // Cache callable_custom_create2
    {
        gdext_c_proc_address_func proc = gdext_c_get_proc_address_internal();
        if (proc) {
            g_callable_create = (GDExtensionInterfaceCallableCustomCreate2)proc("callable_custom_create2");
        }
    }

    if (g_object_connect_mb && g_object_disconnect_mb && g_callable_create) {
        g_signal_system_initialized = 1;
        fprintf(stderr, "[gdext-c] ✅ Signal system initialized: connect=%p, disconnect=%p, callable_create=%p\n",
                (void*)g_object_connect_mb, (void*)g_object_disconnect_mb, (void*)g_callable_create);
    } else {
        fprintf(stderr, "[gdext-c] ⚠️ Signal system partial init: connect=%p, disconnect=%p, callable_create=%p\n",
                (void*)g_object_connect_mb, (void*)g_object_disconnect_mb, (void*)g_callable_create);
    }
}

// ============================================================================
// Signal API Implementation
// ============================================================================

/**
 * @brief Initialize the signal system with the Go callback bridge.
 *
 * @param callback  The Go function to call when any signal fires.
 *                  Signature: func(callbackID, argCount, args)
 */
void gdext_init_signals(void* callback) {
    g_go_signal_callback = (GoSignalCallbackFn)callback;

    // Zero out the connection registry
    memset(g_connections, 0, sizeof(g_connections));
    g_next_callback_id = 1;

    fprintf(stderr, "[gdext-c] ✅ Signal callback registered: %p\n", callback);
}

/**
 * @brief Connect a Go callback to a Godot signal on an object.
 *
 * Creates a custom Callable wrapping the callback_id as userdata,
 * then calls Object.connect(signal_name, callable, 0) via ptrcall.
 *
 * @param object_ptr  The Godot object to connect the signal on
 * @param signal_name The signal name (e.g., "pressed", "body_entered")
 * @return callback_id on success, 0 on failure
 */
uintptr_t gdext_connect_signal(uintptr_t object_ptr, const char* signal_name) {
    if (!object_ptr || !signal_name) return 0;

    ensure_signal_system_initialized();

    if (!g_signal_system_initialized) {
        fprintf(stderr, "[gdext-c] ❌ gdext_connect_signal: signal system not initialized\n");
        return 0;
    }

    if (!g_go_signal_callback) {
        fprintf(stderr, "[gdext-c] ❌ gdext_connect_signal: no Go callback registered (call gdext_init_signals first)\n");
        return 0;
    }

    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    if (!iface) return 0;

    // Find a free slot in the registry
    int slot = -1;
    for (int i = 0; i < MAX_SIGNAL_CONNECTIONS; i++) {
        if (!g_connections[i].active) {
            slot = i;
            break;
        }
    }

    if (slot < 0) {
        fprintf(stderr, "[gdext-c] ❌ gdext_connect_signal: registry full (%d max)\n", MAX_SIGNAL_CONNECTIONS);
        return 0;
    }

    // Allocate callback ID
    uintptr_t callback_id = g_next_callback_id++;

    // Store connection info
    SignalConnection* conn = &g_connections[slot];
    conn->active = 1;
    conn->callback_id = callback_id;
    conn->object_ptr = object_ptr;
    strncpy(conn->signal_name, signal_name, sizeof(conn->signal_name) - 1);
    conn->signal_name[sizeof(conn->signal_name) - 1] = '\0';

    // Create custom Callable with callback_id as userdata
    GDExtensionCallableCustomInfo2 callable_info;
    memset(&callable_info, 0, sizeof(callable_info));
    callable_info.callable_userdata = (void*)callback_id;
    callable_info.token = gdext_c_get_library_handle();
    callable_info.object_id = 0;
    callable_info.call_func = on_signal_callback;
    callable_info.is_valid_func = on_signal_is_valid;
    callable_info.free_func = NULL;
    callable_info.hash_func = NULL;
    callable_info.equal_func = NULL;
    callable_info.less_than_func = NULL;
    callable_info.to_string_func = NULL;
    callable_info.get_argument_count_func = NULL;

    g_callable_create(
        (GDExtensionUninitializedTypePtr)conn->callable_storage,
        &callable_info
    );

    // Create StringName for signal (p_is_static=0: signal_name from Go, temporary)
    char signal_sn[64] = {0};
    iface->string_name_new_with_latin1_chars(signal_sn, signal_name, 0);

    // Call Object.connect(signal_name, callable, flags=0) via ptrcall
    int64_t flags = 0;
    const void* connect_args[3] = {
        signal_sn,
        conn->callable_storage,
        &flags
    };

    int64_t error_result = 0;
    iface->object_method_bind_ptrcall(
        g_object_connect_mb,
        (GDExtensionObjectPtr)object_ptr,
        connect_args,
        &error_result
    );

    // Cleanup StringName
    if (g_sn_destructor) g_sn_destructor(signal_sn);

    if (error_result != 0) {
        fprintf(stderr, "[gdext-c] ❌ Object.connect('%s') failed with error %lld\n",
                signal_name, (long long)error_result);
        conn->active = 0;
        return 0;
    }

    fprintf(stderr, "[gdext-c] ✅ Signal connected: '%s' → callback_id=%lu (slot %d)\n",
            signal_name, (unsigned long)callback_id, slot);

    return callback_id;
}

/**
 * @brief Disconnect a signal from a Godot object.
 *
 * Looks up the connection in the registry by object + signal name,
 * then calls Object.disconnect(signal_name, callable) via ptrcall.
 *
 * @param object_ptr  The Godot object
 * @param signal_name The signal to disconnect
 * @return 1 on success, 0 on failure
 */
int gdext_disconnect_signal(uintptr_t object_ptr, const char* signal_name) {
    if (!object_ptr || !signal_name) return 0;

    ensure_signal_system_initialized();

    if (!g_signal_system_initialized || !g_object_disconnect_mb) return 0;

    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    if (!iface) return 0;

    // Find the connection in the registry
    int slot = -1;
    for (int i = 0; i < MAX_SIGNAL_CONNECTIONS; i++) {
        if (g_connections[i].active &&
            g_connections[i].object_ptr == object_ptr &&
            strcmp(g_connections[i].signal_name, signal_name) == 0) {
            slot = i;
            break;
        }
    }

    if (slot < 0) {
        fprintf(stderr, "[gdext-c] ⚠️ gdext_disconnect_signal: no connection found for '%s'\n", signal_name);
        return 0;
    }

    SignalConnection* conn = &g_connections[slot];

    // Create StringName for signal
    char signal_sn[64] = {0};
    iface->string_name_new_with_latin1_chars(signal_sn, signal_name, 0);

    // Call Object.disconnect(signal_name, callable) via ptrcall
    const void* disconnect_args[2] = {
        signal_sn,
        conn->callable_storage
    };

    iface->object_method_bind_ptrcall(
        g_object_disconnect_mb,
        (GDExtensionObjectPtr)object_ptr,
        disconnect_args,
        NULL  // disconnect returns void
    );

    // Cleanup StringName
    if (g_sn_destructor) g_sn_destructor(signal_sn);

    // Mark slot as free
    conn->active = 0;

    fprintf(stderr, "[gdext-c] ✅ Signal disconnected: '%s' (slot %d, callback_id=%lu)\n",
            signal_name, slot, (unsigned long)conn->callback_id);

    return 1;
}

/**
 * @brief Emit a signal on a Godot object.
 *
 * Uses Godot's variant_call to call emit_signal() on the object.
 *
 * @param object_ptr  The Godot object to emit the signal on
 * @param signal_name The signal name
 * @param args        Array of Variant pointers (signal arguments)
 * @param arg_count   Number of arguments
 * @return 1 on success, 0 on failure
 */
int gdext_emit_signal(uintptr_t object_ptr, const char* signal_name, void** args, uintptr_t arg_count) {
    if (!object_ptr || !signal_name) return 0;

    if (!gdext_c_is_initialized()) return 0;

    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    if (!iface) return 0;

    // Create StringName variant for the signal name
    // emit_signal takes (signal: StringName, ...) as variant_call args
    void* signal_variant = gdext_variant_from_string(signal_name);
    if (!signal_variant) return 0;

    // Build combined args array: [signal_name_variant, ...user_args]
    uintptr_t total_args = 1 + arg_count;
    void** all_args = (void**)malloc(total_args * sizeof(void*));
    if (!all_args) {
        gdext_variant_free(signal_variant);
        return 0;
    }

    all_args[0] = signal_variant;
    for (uintptr_t i = 0; i < arg_count; i++) {
        all_args[i + 1] = args[i];
    }

    // Call emit_signal via gdext_call_method (variant_call)
    void* result = gdext_call_method((void*)object_ptr, "emit_signal", all_args, (int)total_args);

    // Cleanup
    if (result) gdext_variant_free(result);
    gdext_variant_free(signal_variant);
    free(all_args);

    return 1;
}

/**
 * @brief Free signal argument variants.
 *
 * @param args      Array of Variant pointers
 * @param arg_count Number of arguments
 */
void gdext_free_signal_args(void** args, uintptr_t arg_count) {
    if (!args) return;

    for (uintptr_t i = 0; i < arg_count; i++) {
        if (args[i]) {
            gdext_variant_free(args[i]);
        }
    }
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
static GDExtensionMethodBindPtr g_get_last_mouse_velocity_mb = NULL;

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
#define HASH_GET_LAST_MOUSE_VELOCITY 1497962370

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
    
    memset(method_sn, 0, sizeof(method_sn));
    iface->string_name_new_with_latin1_chars(method_sn, "get_last_mouse_velocity", 1);
    g_get_last_mouse_velocity_mb = iface->classdb_get_method_bind(class_sn, method_sn, HASH_GET_LAST_MOUSE_VELOCITY);
    
    fprintf(stderr, "[gdext-c] ✅ Input system initialized: action_pressed=%p, just_pressed=%p, key_pressed=%p, mouse_vel=%p\n",
            (void*)g_is_action_pressed_mb, (void*)g_is_action_just_pressed_mb, (void*)g_is_key_pressed_mb, (void*)g_get_last_mouse_velocity_mb);
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

// TDD: Alias for Go compatibility (Go code expects this symbol name from old Rust bridge)
// Both functions do the same thing - check if a key is currently pressed
int gdext_is_key_pressed_from_events(int keycode) {
    return gdext_is_key_pressed(keycode);
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

/**
 * @brief Get the last mouse velocity from the Input singleton.
 * Returns mouse velocity in pixels/second via output pointers.
 * Callers multiply by frame delta to get per-frame pixel delta.
 */
void gdext_get_last_mouse_velocity(float* out_x, float* out_y) {
    if (out_x) *out_x = 0.0f;
    if (out_y) *out_y = 0.0f;

    ensure_input_initialized();
    if (!g_input_singleton || !g_get_last_mouse_velocity_mb) return;

    const GDExtensionInterface* iface = gdext_c_get_interface_functions();

    // get_last_mouse_velocity() -> Vector2 (no arguments)
    // Vector2 ptrcall returns 2x float (8 bytes)
    float result[2] = {0.0f, 0.0f};
    iface->object_method_bind_ptrcall(g_get_last_mouse_velocity_mb, g_input_singleton, NULL, result);

    if (out_x) *out_x = result[0];
    if (out_y) *out_y = result[1];
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

// TDD: Log to Godot console using print_rich() utility function
// CRITICAL FIX: printf/fprintf go to stdout (invisible in Godot extension)
// Must use Godot's UtilityFunctions.print_rich() instead
void gdext_log_message(const char* message) {
    // Use stderr for C-side logging (simpler and more reliable)
    // Note: Go-side logging via godotlog is preferred for game code
    fprintf(stderr, "[Godot] %s\n", message);
    fflush(stderr);
}

void gdext_log_warning(const char* message) {
    // Use stderr for C-side logging (simpler and more reliable)
    fprintf(stderr, "[Godot Warning] %s\n", message);
    fflush(stderr);
}

void gdext_log_error(const char* message) {
    // Use stderr for C-side logging (simpler and more reliable)
    fprintf(stderr, "[Godot Error] %s\n", message);
    fflush(stderr);
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
