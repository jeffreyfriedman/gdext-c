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
#include <stdio.h>
#include <string.h>

// Global storage for library handle
static GDExtensionClassLibraryPtr g_library = NULL;

/**
 * @brief Initialize at specific level
 */
void gdext_c_initialize_level(void *p_userdata, GDExtensionInitializationLevel p_level) {
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
    
    printf("[gdext-c] 🎯 TDD #155: Initializing level: %s (PURE C - NO RUST!)\n", level_name);
    fflush(stdout);
    
    // TODO TDD #155: Register GameNode class at SCENE level
    // For now, skip GameNode registration - let Go game logic create nodes directly
    // This will be implemented in a future version with proper instance creation
    if (p_level == GDEXTENSION_INITIALIZATION_SCENE) {
        printf("[gdext-c] 📝 TDD #155: Scene level initialized (GameNode registration skipped for now)\n");
        printf("[gdext-c] 💡 Go game logic will create nodes directly using Godot built-in classes\n");
        fflush(stdout);
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

