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
    
    printf("[gdext-c] 🎯 TDD #155: Initializing level: %s (PURE C - NO RUST!)\n", level_name);
    fflush(stdout);
    
    // TDD #158: Load game logic at CORE level (earliest possible)
    if (p_level == GDEXTENSION_INITIALIZATION_CORE) {
        printf("[gdext-c] 🎮 TDD #158: Loading game_logic.dylib...\n");
        fflush(stdout);
        
        // Try multiple paths to find game_logic.dylib
        const char* paths[] = {
            "@loader_path/game_logic.dylib",           // Relative to this library
            "@loader_path/../bin/macos/game_logic.dylib",  // From Godot binary
            "./bin/macos/game_logic.dylib",            // From project root
            "bin/macos/game_logic.dylib",              // Alternate
            NULL
        };
        
        for (int i = 0; paths[i] != NULL; i++) {
            printf("[gdext-c] 📂 Trying: %s\n", paths[i]);
            fflush(stdout);
            
            // TDD #160: Use RTLD_LAZY - symbols will be resolved when actually called
            // This allows game_logic to load even if some symbols are missing
            g_game_logic_handle = dlopen(paths[i], RTLD_LAZY | RTLD_GLOBAL);
            if (g_game_logic_handle) {
                printf("[gdext-c] ✅ TDD #158: Loaded game_logic.dylib from: %s\n", paths[i]);
                printf("[gdext-c] 📍 Handle: %p\n", g_game_logic_handle);
                fflush(stdout);
                break;
            } else {
                printf("[gdext-c] ⚠️  Failed: %s\n", dlerror());
                fflush(stdout);
            }
        }
        
        if (!g_game_logic_handle) {
            printf("[gdext-c] ❌ TDD #158: Failed to load game_logic.dylib from any path!\n");
            fflush(stdout);
        }
    }
    
    // TDD #156: Register GameNode class at SCENE level
    // TDD 3.1: GameNode acts as a bridge for lifecycle callbacks
    if (p_level == GDEXTENSION_INITIALIZATION_SCENE) {
        printf("[gdext-c] 🎮 TDD 3.1: Registering GameNode class...\n");
        fflush(stdout);
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
    
    printf("[gdext-c] 🔽 TDD #155: Deinitializing level: %s\n", level_name);
    fflush(stdout);
}

/**
 * @brief Main GDExtension library initialization function
 * 
 * This is THE entry point that Godot calls.
 * REPLACES Rust bridge's gdext_initialize function entirely!
 */
GDExtensionBool gdext_c_library_init(
    GDExtensionInterfaceGetProcAddress p_get_proc_address,
    const GDExtensionClassLibraryPtr p_library,
    GDExtensionInitialization *r_initialization
) {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║ TDD #155: PURE C GDEXTENSION ENTRY POINT (NO RUST!)       ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    printf("[gdext-c] 🚀 gdext_c_library_init called by Godot\n");
    printf("[gdext-c] 📍 p_get_proc_address: %p\n", (void*)p_get_proc_address);
    printf("[gdext-c] 📍 p_library: %p\n", (void*)p_library);
    fflush(stdout);
    
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
    
    // Store library handle for class registration
    gdext_c_set_library_handle(p_library);
    
    printf("[gdext-c] ✅ gdext-c core initialized successfully\n");
    fflush(stdout);
    
    // Set up initialization structure
    r_initialization->minimum_initialization_level = GDEXTENSION_INITIALIZATION_SCENE;
    r_initialization->userdata = NULL;
    r_initialization->initialize = gdext_c_initialize_level;
    r_initialization->deinitialize = gdext_c_deinitialize_level;
    
    printf("[gdext-c] ✅ Initialization structure configured\n");
    printf("[gdext-c] 📝 Minimum level: SCENE\n");
    printf("[gdext-c] 📝 Callbacks registered\n");
    printf("[gdext-c] 🎉 PURE C GDEXTENSION READY - NO RUST!\n");
    printf("\n");
    fflush(stdout);
    
    return 1; // Success!
}

