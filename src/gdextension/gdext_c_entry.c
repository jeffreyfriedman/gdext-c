/**
 * @file gdext_c_entry.c
 * @brief Pure C GDExtension entry point implementation
 * 
 * TDD #155: REPLACES Rust bridge entirely!
 * This provides the main entry point for Godot to load our extension.
 */

#include "gdext_c_gdextension.h"
#include "gdext_c_core.h"
#include "gdext_c_callbacks.h"
#include "gdext_c_signal_handler.h"       // TDD Option 3: Crash signal handlers
#include "gdext_c_object_registry.h"      // TDD #202: Object lifetime tracking
#include "../core/gdext_c_refcounted_cleanup.h"  // PLATFORM FIX: Thread-safe RefCounted cleanup
#include "threading/gdext_c_thread.h"     // TDD: Thread detection for GPU safety
#include "threading/gdext_c_gpu_queue.h"  // TDD: GPU operation queue
#include "threading/gdext_c_gpu_safe.h"   // TDD Phase 3: Safe GPU wrappers
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>  // TDD #158: For loading game_logic.dylib

// Global storage for library handle
static GDExtensionClassLibraryPtr g_library = NULL;
static void* g_game_logic_handle = NULL;  // TDD #158: Handle to game_logic.dylib

/**
 * @brief Initialize at specific level
 */
void gdext_c_initialize_level(void *p_userdata, GDExtensionInitializationLevel p_level) {
    (void)p_userdata;
    
    const char* level_name = "UNKNOWN";
    switch (p_level) {
        case GDEXTENSION_INITIALIZATION_CORE:
            level_name = "CORE";
            // TDD Option 3: Initialize crash signal handlers FIRST
            gdext_c_signal_handler_init();
            // TDD #202: Initialize object lifetime tracking
            gdext_registry_init();
            // PLATFORM FIX: Initialize RefCounted cleanup queue (thread-safe!)
            gdext_refcounted_cleanup_init();
            // TDD: Initialize thread detection (MUST be on main thread!)
            gdext_thread_init();
            // TDD: Initialize GPU operation queue
            gdext_gpu_queue_init();
            break;
        case GDEXTENSION_INITIALIZATION_SERVERS:
            level_name = "SERVERS";
            // TDD Phase 3: Initialize GPU safe wrappers (RenderingServer available now)
            gdext_gpu_safe_init();
            break;
        case GDEXTENSION_INITIALIZATION_SCENE:
            level_name = "SCENE";
            break;
        case GDEXTENSION_INITIALIZATION_EDITOR:
            level_name = "EDITOR";
            break;
    }
    
    printf("[gdext-c] Initializing level: %s\n", level_name);
    fflush(stdout);
    
    // Load game logic at CORE level
    if (p_level == GDEXTENSION_INITIALIZATION_CORE) {
        fprintf(stderr, "[gdext-c] 📚 Attempting to load game_logic.dylib...\n");
        fflush(stderr);
        
        const char* paths[] = {
            "@loader_path/game_logic.dylib",
            "@loader_path/../bin/macos/game_logic.dylib",
            "./bin/macos/game_logic.dylib",
            "bin/macos/game_logic.dylib",
            NULL
        };
        
        for (int i = 0; paths[i] != NULL; i++) {
            fprintf(stderr, "[gdext-c] 🔍 Trying: %s\n", paths[i]);
            fflush(stderr);
            
            g_game_logic_handle = dlopen(paths[i], RTLD_LAZY | RTLD_GLOBAL);
            if (g_game_logic_handle) {
                fprintf(stderr, "[gdext-c] ✅ Loaded game_logic.dylib from: %s\n", paths[i]);
                fflush(stderr);
                break;
            } else {
                fprintf(stderr, "[gdext-c] ❌ dlopen('%s') failed: %s\n", paths[i], dlerror());
                fflush(stderr);
            }
        }
        
        if (!g_game_logic_handle) {
            fprintf(stderr, "[gdext-c] ❌ CRITICAL: Failed to load game_logic.dylib from all paths!\n");
            fprintf(stderr, "[gdext-c] 💡 Last dlerror: %s\n", dlerror());
            fflush(stderr);
        } else {
            fprintf(stderr, "[gdext-c] 🎮 game_logic.dylib loaded, Go init() should run now...\n");
            fflush(stderr);
        }
    }
    
    // Register GameNode class at SCENE level
    if (p_level == GDEXTENSION_INITIALIZATION_SCENE) {
        gdext_c_register_game_node_class(NULL, NULL);
    }
}

/**
 * @brief Deinitialize at specific level
 */
void gdext_c_deinitialize_level(void *p_userdata, GDExtensionInitializationLevel p_level) {
    (void)p_userdata;
    
    const char* level_name = "UNKNOWN";
    switch (p_level) {
        case GDEXTENSION_INITIALIZATION_CORE:
            level_name = "CORE";
            // Clean up in reverse order of initialization
            {
                extern void gdext_gpu_queue_shutdown(void);
                gdext_gpu_queue_shutdown();
            }
            {
                extern void gdext_gpu_safe_shutdown(void);
                gdext_gpu_safe_shutdown();
            }
            gdext_refcounted_cleanup_shutdown();
            {
                extern void gdext_registry_shutdown(void);
                gdext_registry_shutdown();
            }
            {
                extern void gdext_c_objects_shutdown(void);
                gdext_c_objects_shutdown();
            }
            break;
        case GDEXTENSION_INITIALIZATION_SERVERS:
            level_name = "SERVERS";
            break;
        case GDEXTENSION_INITIALIZATION_SCENE:
            level_name = "SCENE";
            break;
        case GDEXTENSION_INITIALIZATION_EDITOR:
            level_name = "EDITOR";
            break;
    }
    
    printf("[gdext-c] Deinitializing level: %s\n", level_name);
    fflush(stdout);
}

/**
 * @brief Main GDExtension library initialization function
 * 
 * This is THE entry point that Godot calls.
 * REPLACES Rust bridge's gdext_initialize function entirely!
 * 
 * TDD #206: MUST use GDE_EXPORT to make this symbol visible!
 * Without this, Godot cannot find the entry point and the extension won't load.
 */
GDExtensionBool GDE_EXPORT gdext_c_library_init(
    GDExtensionInterfaceGetProcAddress p_get_proc_address,
    const GDExtensionClassLibraryPtr p_library,
    GDExtensionInitialization *r_initialization
) {
    fprintf(stderr, "[gdext-c] 🚀 Library init CALLED!\n");
    fflush(stderr);
    
    // Store library handle
    g_library = p_library;
    
    // Initialize gdext-c core (stores proc_address, gets interface functions)
    // Note: gdext_c_initialize only takes proc_address, not library handle
    // Cast to the expected type (gdext_c_proc_address_func)
    if (!gdext_c_initialize((gdext_c_proc_address_func)p_get_proc_address)) {
        fprintf(stderr, "[gdext-c] ❌ Failed to initialize gdext-c core!\n");
        fflush(stderr);
        return 0;
    }
    
    fprintf(stderr, "[gdext-c] ✅ Core initialized\n");
    fflush(stderr);
    
    // Store library handle for class registration
    gdext_c_set_library_handle(p_library);
    
    // Set up initialization structure
    r_initialization->minimum_initialization_level = GDEXTENSION_INITIALIZATION_SCENE;
    r_initialization->userdata = NULL;
    r_initialization->initialize = gdext_c_initialize_level;
    r_initialization->deinitialize = gdext_c_deinitialize_level;
    
    fprintf(stderr, "[gdext-c] ✅ Library initialized, will load game_logic.dylib at CORE level\n");
    fflush(stderr);
    
    return 1; // Success!
}

/**
 * @brief Godot standard entry point
 * TDD: Wrapper that calls gdext_c_library_init
 * Godot expects this specific function name for GDExtension loading
 */
GDExtensionBool GDE_EXPORT gdextension_init(
    GDExtensionInterfaceGetProcAddress p_get_proc_address,
    const GDExtensionClassLibraryPtr p_library,
    GDExtensionInitialization *r_initialization
) {
    // Simply forward to the main initialization function
    return gdext_c_library_init(p_get_proc_address, p_library, r_initialization);
}

