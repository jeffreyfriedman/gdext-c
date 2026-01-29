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
    printf("[gdext-c] 📋 Registering lifecycle callbacks...\n");
    
    g_ready_callback = ready;
    g_process_callback = process;
    g_physics_callback = physics;
    g_shutdown_callback = shutdown;
    
    printf("[gdext-c] ✅ Lifecycle callbacks registered:\n");
    printf("[gdext-c]    Ready:    %p\n", (void*)ready);
    printf("[gdext-c]    Process:  %p\n", (void*)process);
    printf("[gdext-c]    Physics:  %p\n", (void*)physics);
    printf("[gdext-c]    Shutdown: %p\n", (void*)shutdown);
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
    // TDD B1-A: FIRST LINE - Log immediately to see if callback is EVER called
    static int entry_count = 0;
    entry_count++;
    
    // Force immediate flush
    fprintf(stderr, "[gdext-c] 🚨 TDD B1-A v5: CALLBACK ENTERED! (call #%d)\n", entry_count);
    fflush(stderr);
    
    (void)p_userdata;
    (void)p_args;
    (void)p_argument_count;
    (void)r_return;
    
    // TDD B1-A: Log to verify callback is called
    static int call_count = 0;
    call_count++;
    if (call_count == 1 || call_count % 60 == 0) {
        printf("[gdext-c] 🎯 TDD B1-A v5: physics_frame callback #%d (IT WORKS!)\n", call_count);
        fflush(stdout);
    }
    
    // Set no error
    if (r_error) {
        r_error->error = GDEXTENSION_CALL_OK;
    }
    
    // TODO: Get actual delta from signal args if needed
    // For now, assume 60 FPS
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
    
    printf("[gdext-c] ✅ Got SceneTree: %p\n", g_scene_tree);
    fflush(stdout);
    
    // TDD B1-A: Try physics_frame instead of process_frame
    printf("[gdext-c] 🔨 TDD B1-A: Creating Callable for physics_frame...\n");
    fflush(stdout);
    
    // TDD B1-A v6: Try library handle as token (what godot-cpp uses)
    GDExtensionCallableCustomInfo2 process_info;
    memset(&process_info, 0, sizeof(process_info));
    
    process_info.callable_userdata = NULL;  // No user data needed
    process_info.token = gdext_c_get_library_handle();  // TDD B1-A v6: Use library handle!
    process_info.object_id = 0;              // Static function, no object
    process_info.call_func = on_process_frame_signal;
    process_info.is_valid_func = on_process_frame_is_valid;  // TDD B1-A v5: CRITICAL! Must return TRUE
    process_info.free_func = NULL;            // Static, no cleanup needed
    process_info.hash_func = NULL;            // Use default
    process_info.equal_func = NULL;           // Use default
    process_info.less_than_func = NULL;       // Use default
    process_info.to_string_func = NULL;       // Use default
    process_info.get_argument_count_func = NULL;  // No args
    
    printf("[gdext-c] 🔧 TDD B1-A v6: Creating Callable with library handle token + is_valid...\n");
    fflush(stdout);
    
    // Get callable_custom_create2 function directly
    GDExtensionInterfaceCallableCustomCreate2 callable_create = 
        (GDExtensionInterfaceCallableCustomCreate2)gdext_c_get_proc_address_internal()("callable_custom_create2");
    
    if (!callable_create) {
        fprintf(stderr, "[gdext-c] ❌ TDD B1: Failed to get callable_custom_create2 function\n");
        fflush(stderr);
        return 0;
    }
    
    // Create the Callable in persistent storage
    callable_create((GDExtensionUninitializedTypePtr)g_process_callable_storage, &process_info);
    
    printf("[gdext-c] ✅ TDD B1-A: Callable created successfully\n");
    fflush(stdout);
    
    // TDD B1-A v4: Back to simpler Object.connect() approach with NULL token
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
    
    printf("[gdext-c] 🔌 TDD B1-A v4: Calling scene_tree.connect() with NULL token Callable...\n");
    fflush(stdout);
    
    iface->object_method_bind_ptrcall(
        (GDExtensionMethodBindPtr)connect_bind,
        g_scene_tree,
        connect_args,
        error_result_ptr
    );
    
    if (error_result != 0) {
        fprintf(stderr, "[gdext-c] ❌ TDD B1-A v4: connect() returned error: %lld\n", (long long)error_result);
        fflush(stderr);
        return 0;
    }
    
    printf("[gdext-c] ✅ TDD B1-A v4: Connected with NULL token!\n");
    fflush(stdout);
    
    g_signals_connected = 1;
    
    printf("[gdext-c] 🎉 TDD B1-A: SceneTree signals connected (pure C, no GameNode)!\n");
    fflush(stdout);
    
    return 1;
}

/**
 * @brief TDD 1.5: Run the game loop (blocks until shutdown)
 */
void gdext_c_run_game_loop(void) {
    printf("[gdext-c] 🎮 Starting game loop...\n");
    fflush(stdout);
    
    // Connect to SceneTree signals
    if (!gdext_c_connect_to_scene_tree()) {
        fprintf(stderr, "[gdext-c] ❌ Failed to connect to SceneTree\n");
        fflush(stderr);
        return;
    }
    
    // Call ready callback once
    if (g_ready_callback) {
        printf("[gdext-c] 🎯 Calling ready callback...\n");
        fflush(stdout);
        g_ready_callback();
        printf("[gdext-c] ✅ Ready callback completed\n");
        fflush(stdout);
    }
    
    printf("[gdext-c] ✅ Game loop started (callbacks will fire via SceneTree signals)\n");
    printf("[gdext-c] 💡 This function does NOT block - callbacks are signal-driven\n");
    fflush(stdout);
    
    // NOTE: Unlike a traditional game loop, we don't block here!
    // The callbacks are triggered by Godot's SceneTree signals.
    // The game will continue running via Godot's main loop.
    
    // Shutdown callback will be called by gdext_c_deinitialize_level() or similar
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
 */
void c_register_go_lifecycle_callbacks(
    gdext_c_ready_callback ready,
    gdext_c_process_callback process,
    gdext_c_physics_callback physics,
    gdext_c_shutdown_callback shutdown
) {
    printf("[gdext-c] 🚀 c_register_go_lifecycle_callbacks called (TDD 3.1)\n");
    fflush(stdout);
    
    gdext_c_register_lifecycle_callbacks(ready, process, physics, shutdown);
    
    printf("[gdext-c] ✅ Go lifecycle callbacks registered\n");
    fflush(stdout);
}

/**
 * @brief Called during GDExtension shutdown (can trigger shutdown callback)
 */
void gdext_c_lifecycle_shutdown(void) {
    printf("[gdext-c] 🛑 Lifecycle shutdown requested...\n");
    fflush(stdout);
    
    if (g_shutdown_callback) {
        printf("[gdext-c] 🧹 Calling shutdown callback...\n");
        fflush(stdout);
        g_shutdown_callback();
        printf("[gdext-c] ✅ Shutdown callback completed\n");
        fflush(stdout);
    }
    
    // Clear callbacks
    g_ready_callback = NULL;
    g_process_callback = NULL;
    g_physics_callback = NULL;
    g_shutdown_callback = NULL;
    g_signals_connected = 0;
    
    printf("[gdext-c] ✅ Lifecycle shutdown complete\n");
    fflush(stdout);
}

