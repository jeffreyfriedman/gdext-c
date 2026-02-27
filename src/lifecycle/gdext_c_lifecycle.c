/**
 * @file gdext_c_lifecycle.c
 * @brief Universal lifecycle system implementation
 * 
 * TDD: Inspired by graphics.gd architecture
 * Provides signal-based callbacks for all language bindings
 */

#include "gdext_c_lifecycle.h"
#include "gdext_c_core.h"
#include "gdext_c_generated.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>   // For int64_t
#include <unistd.h>   // For usleep
#include <dlfcn.h>    // For dlsym (TDD: find Go trampolines)

// TDD 1.2: Global storage for registered callbacks
static gdext_c_ready_callback g_ready_callback = NULL;
static gdext_c_process_callback g_process_callback = NULL;
static gdext_c_physics_callback g_physics_callback = NULL;
static gdext_c_shutdown_callback g_shutdown_callback = NULL;

// TDD 1.3: Global storage for SceneTree connection
static void* g_scene_tree = NULL;
static int g_signals_connected = 0;

// TDD B1: Global storage for Callable variants (must persist!)
static char g_process_callable_storage[256] = {0};
static char g_physics_callable_storage[256] = {0};

/**
 * @brief TDD 1.1: Check if Godot engine is fully initialized
 */
int gdext_c_is_engine_ready(void) {
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    if (!iface) {
        return 0; // Interface not available yet
    }
    
    // Check 1: Can we get the Engine singleton?
    char engine_sn[64] = {0};
    iface->string_name_new_with_latin1_chars(engine_sn, "Engine", 0);
    void* engine = iface->global_get_singleton(engine_sn);
    if (!engine) {
        return 0; // Engine singleton not available
    }
    
    // Check 2: Can we get the SceneTree?
    char engine_class_sn[64] = {0};
    char get_main_loop_sn[64] = {0};
    iface->string_name_new_with_latin1_chars(engine_class_sn, "Engine", 0);
    iface->string_name_new_with_latin1_chars(get_main_loop_sn, "get_main_loop", 0);
    
    void* get_main_loop_bind = iface->classdb_get_method_bind(engine_class_sn, get_main_loop_sn, 1016888095);
    if (!get_main_loop_bind) {
        return 0; // Method bind not available
    }
    
    void* main_loop = NULL;
    iface->object_method_bind_ptrcall(get_main_loop_bind, engine, NULL, &main_loop);
    
    if (!main_loop) {
        return 0; // SceneTree not available yet
    }
    
    // All checks passed!
    // Note: Removed RenderingDevice check as it's not needed for basic readiness
    return 1;
}

/**
 * @brief TDD 1.1: Wait until engine is ready (blocking)
 */
void gdext_c_wait_for_engine_ready(void) {
    printf("[gdext-c] ⏳ Waiting for Godot engine to be ready...\n");
    fflush(stdout);
    
    int attempts = 0;
    while (!gdext_c_is_engine_ready()) {
        attempts++;
        
        if (attempts % 100 == 0) {
            printf("[gdext-c] ⏳ Still waiting... (attempt %d)\n", attempts);
            fflush(stdout);
        }
        
        // Sleep for 10ms between checks
        usleep(10000);
        
        // Safety timeout: 10 seconds
        if (attempts > 1000) {
            fprintf(stderr, "[gdext-c] ❌ Timeout waiting for engine ready!\n");
            fflush(stderr);
            return;
        }
    }
    
    printf("[gdext-c] ✅ Godot engine is ready! (after %d attempts)\n", attempts);
    fflush(stdout);
}

/**
 * @brief TDD 1.2: Register language callbacks
 */
void gdext_c_register_lifecycle_callbacks(
    gdext_c_ready_callback ready,
    gdext_c_process_callback process,
    gdext_c_physics_callback physics,
    gdext_c_shutdown_callback shutdown
) {
    g_ready_callback = ready;
    g_process_callback = process;
    g_physics_callback = physics;
    g_shutdown_callback = shutdown;
    
    printf("[gdext-c] ✅ Lifecycle callbacks registered\n");
    fflush(stdout);
}

/**
 * @brief TDD B1-A v5: is_valid function for the Callable
 * This MUST return TRUE or Godot won't call the Callable!
 */
static GDExtensionBool on_process_frame_is_valid(void* p_userdata) {
    (void)p_userdata;
    return 1;  // Always valid
}

/**
 * @brief TDD B1: Signal handler for process_frame
 * 
 * This is called by SceneTree's "process_frame" signal every frame
 * Signature matches GDExtension Callable requirements
 */
static void on_process_frame_signal(
    void* p_userdata,
    const GDExtensionConstVariantPtr* p_args,
    GDExtensionInt p_argument_count,
    GDExtensionVariantPtr r_return,
    GDExtensionCallError* r_error
) {
    (void)p_userdata;
    (void)p_args;
    (void)p_argument_count;
    (void)r_return;
    
    // Set no error
    if (r_error) {
        r_error->error = GDEXTENSION_CALL_OK;
    }
    
    // HOT PATH - no logging
    double delta = 0.016666667;
    
    if (g_process_callback) {
        g_process_callback(delta);
    }
}

/**
 * @brief TDD B1: Signal handler for physics_frame
 * 
 * This is called by SceneTree's "physics_frame" signal at 60 Hz
 * Signature matches GDExtension Callable requirements
 */
static void on_physics_frame_signal(
    void* p_userdata,
    const GDExtensionConstVariantPtr* p_args,
    GDExtensionInt p_argument_count,
    GDExtensionVariantPtr r_return,
    GDExtensionCallError* r_error
) {
    (void)p_userdata;
    (void)p_args;
    (void)p_argument_count;
    (void)r_return;
    
    // Set no error
    if (r_error) {
        r_error->error = GDEXTENSION_CALL_OK;
    }
    
    // TODO: Get actual delta from signal args if needed
    // For now, assume 60 Hz
    double delta = 0.016666667;
    
    if (g_physics_callback) {
        g_physics_callback(delta);
    }
}

/**
 * @brief TDD 1.3: Connect to SceneTree signals
 */
int gdext_c_connect_to_scene_tree(void) {
    printf("[gdext-c] 🔌 Connecting to SceneTree signals...\n");
    fflush(stdout);
    
    if (g_signals_connected) {
        printf("[gdext-c] ✅ Already connected to SceneTree\n");
        return 1;
    }
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    if (!iface) {
        fprintf(stderr, "[gdext-c] ❌ Interface not available\n");
        fflush(stderr);
        return 0;
    }
    
    // Get Engine singleton
    char engine_sn[64] = {0};
    iface->string_name_new_with_latin1_chars(engine_sn, "Engine", 0);
    void* engine = iface->global_get_singleton(engine_sn);
    if (!engine) {
        fprintf(stderr, "[gdext-c] ❌ Failed to get Engine singleton\n");
        fflush(stderr);
        return 0;
    }
    
    // Get SceneTree (MainLoop)
    char engine_class_sn[64] = {0};
    char get_main_loop_sn[64] = {0};
    iface->string_name_new_with_latin1_chars(engine_class_sn, "Engine", 0);
    iface->string_name_new_with_latin1_chars(get_main_loop_sn, "get_main_loop", 0);
    
    void* get_main_loop_bind = iface->classdb_get_method_bind(engine_class_sn, get_main_loop_sn, 1016888095);
    if (!get_main_loop_bind) {
        fprintf(stderr, "[gdext-c] ❌ Failed to get get_main_loop method bind\n");
        fflush(stderr);
        return 0;
    }
    
    iface->object_method_bind_ptrcall(get_main_loop_bind, engine, NULL, &g_scene_tree);
    
    if (!g_scene_tree) {
        fprintf(stderr, "[gdext-c] ❌ Failed to get SceneTree\n");
        fflush(stderr);
        return 0;
    }
    
    // Create Callable for physics_frame
    
    GDExtensionCallableCustomInfo2 process_info;
    memset(&process_info, 0, sizeof(process_info));
    
    process_info.callable_userdata = NULL;
    process_info.token = gdext_c_get_library_handle();
    process_info.object_id = 0;
    process_info.call_func = on_process_frame_signal;
    process_info.is_valid_func = on_process_frame_is_valid;
    process_info.free_func = NULL;
    process_info.hash_func = NULL;
    process_info.equal_func = NULL;
    process_info.less_than_func = NULL;
    process_info.to_string_func = NULL;
    process_info.get_argument_count_func = NULL;
    
    GDExtensionInterfaceCallableCustomCreate2 callable_create = 
        (GDExtensionInterfaceCallableCustomCreate2)gdext_c_get_proc_address_internal()("callable_custom_create2");
    
    if (!callable_create) {
        fprintf(stderr, "[gdext-c] ❌ Failed to get callable_custom_create2\n");
        return 0;
    }
    
    callable_create((GDExtensionUninitializedTypePtr)g_process_callable_storage, &process_info);
    char signal_name_storage[64] = {0};
    iface->string_name_new_with_latin1_chars((GDExtensionUninitializedStringNamePtr)signal_name_storage, "physics_frame", 0);
    
    // Get SceneTree class name
    char scene_tree_class_sn[64] = {0};
    iface->string_name_new_with_latin1_chars((GDExtensionUninitializedStringNamePtr)scene_tree_class_sn, "SceneTree", 0);
    
    // Get connect() method
    char connect_method_sn[64] = {0};
    iface->string_name_new_with_latin1_chars((GDExtensionUninitializedStringNamePtr)connect_method_sn, "connect", 0);
    
    void* connect_bind = iface->classdb_get_method_bind(
        (GDExtensionConstStringNamePtr)scene_tree_class_sn,
        (GDExtensionConstStringNamePtr)connect_method_sn,
        1518946055  // Method hash for Object.connect()
    );
    
    if (!connect_bind) {
        fprintf(stderr, "[gdext-c] ❌ TDD B1-A v4: Failed to get connect() method bind\n");
        fflush(stderr);
        return 0;
    }
    
    // Call: scene_tree.connect("physics_frame", our_callable, 0)
    const void* connect_args[3];
    connect_args[0] = signal_name_storage;
    connect_args[1] = g_process_callable_storage;
    
    int64_t flags = 0;
    connect_args[2] = &flags;
    
    int64_t error_result = 0;
    void* error_result_ptr = &error_result;
    
    iface->object_method_bind_ptrcall(
        (GDExtensionMethodBindPtr)connect_bind,
        g_scene_tree,
        connect_args,
        error_result_ptr
    );
    
    if (error_result != 0) {
        fprintf(stderr, "[gdext-c] ❌ connect() returned error: %lld\n", (long long)error_result);
        return 0;
    }
    
    g_signals_connected = 1;
    
    printf("[gdext-c] ✅ SceneTree signals connected\n");
    fflush(stdout);
    
    return 1;
}

/**
 * @brief TDD 1.5: Run the game loop (blocks until shutdown)
 */
void gdext_c_run_game_loop(void) {
    if (!gdext_c_connect_to_scene_tree()) {
        fprintf(stderr, "[gdext-c] ❌ Failed to connect to SceneTree\n");
        return;
    }
    
    if (g_ready_callback) {
        g_ready_callback();
    }
    
    printf("[gdext-c] ✅ Game loop started (signal-driven)\n");
    fflush(stdout);
}

/**
 * @brief TDD 1.4: Dispatch ready callback (called by GameNode bridge)
 */
void gdext_c_lifecycle_dispatch_ready(void) {
    if (g_ready_callback) {
        g_ready_callback();
    }
}

/**
 * @brief TDD 1.4: Dispatch process callback (called by GameNode bridge)
 */
void gdext_c_lifecycle_dispatch_process(double delta) {
    if (g_process_callback) {
        g_process_callback(delta);
    }
}

/**
 * @brief TDD 1.4: Dispatch physics callback (called by GameNode bridge)
 */
void gdext_c_lifecycle_dispatch_physics(double delta) {
    if (g_physics_callback) {
        g_physics_callback(delta);
    }
}

/**
 * @brief Helper for Go: Register lifecycle callbacks (like c_register_go_callbacks)
 * 
 * This is a convenience wrapper that Go can call directly.
 * It's exported from libgdext_c.dylib and can be found via dlsym or extern declaration.
 * 
 * TDD FIX: Instead of requiring function pointers from Go (CGO limitation),
 * we use dlsym to find the //export trampolines by name.
 */
void c_register_go_lifecycle_callbacks(
    gdext_c_ready_callback ready,
    gdext_c_process_callback process,
    gdext_c_physics_callback physics,
    gdext_c_shutdown_callback shutdown
) {
    // TDD: If pointers are provided, use them (backward compat)
    if (ready || process || physics || shutdown) {
        gdext_c_register_lifecycle_callbacks(ready, process, physics, shutdown);
        return;
    }
    
    // TDD FIX: Use dlsym to find Go //export trampolines
    // Use RTLD_SELF on macOS to search the current binary (statically linked Go code)
    #ifdef __APPLE__
        void* handle = RTLD_SELF;
    #else
        void* handle = RTLD_DEFAULT;
    #endif
    
    void* ready_fn = dlsym(handle, "go_ready_trampoline");
    void* process_fn = dlsym(handle, "go_process_trampoline");
    void* physics_fn = dlsym(handle, "go_physics_trampoline");
    void* shutdown_fn = dlsym(handle, "go_shutdown_trampoline");
    
    if (physics_fn) {
        fprintf(stderr, "[gdext-c] ✅ Found Go trampolines via dlsym (physics=%p)\n", physics_fn);
        fflush(stderr);
        gdext_c_register_lifecycle_callbacks(
            (gdext_c_ready_callback)ready_fn,
            (gdext_c_process_callback)process_fn,
            (gdext_c_physics_callback)physics_fn,
            (gdext_c_shutdown_callback)shutdown_fn
        );
    } else {
        fprintf(stderr, "[gdext-c] ⚠️  dlsym could not find Go trampolines\n");
        fflush(stderr);
    }
}

/**
 * @brief Called during GDExtension shutdown (can trigger shutdown callback)
 */
void gdext_c_lifecycle_shutdown(void) {
    if (g_shutdown_callback) {
        g_shutdown_callback();
    }
    
    // Process remaining GPU operations before clearing state
    extern int gdext_gpu_queue_process(int max_operations);
    gdext_gpu_queue_process(0);
    
    g_ready_callback = NULL;
    g_process_callback = NULL;
    g_physics_callback = NULL;
    g_shutdown_callback = NULL;
    g_signals_connected = 0;
    
    printf("[gdext-c] Lifecycle shutdown complete\n");
    fflush(stdout);
}

