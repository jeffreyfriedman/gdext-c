/**
 * gdextension_entry.c - TDD #89: Pure C GDExtension Entry Point
 * 
 * This replaces the Rust bridge (libgdext_rust_bridge.dylib) with a pure C implementation.
 * 
 * Architecture:
 *   Godot → C GDExtension → Go Game Logic
 *   
 * This file:
 * - Implements gdextension_initialize() (required by Godot)
 * - Loads game_logic.dylib
 * - Passes proc_address to Go
 * - Routes Godot callbacks to Go
 */

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include "../c-bridge/gdextension_interface.h"

// Export macro for GDExtension entry point
#if defined(_WIN32)
    #define GDX_EXPORT __declspec(dllexport)
#else
    #define GDX_EXPORT __attribute__((visibility("default")))
#endif

// Global state
static void* g_proc_address = NULL;
static void* g_go_library = NULL;
static void* g_rust_bridge = NULL;
static void* g_library = NULL; // TDD #97: Store library pointer for GameNode registration
static GDExtensionInitialization g_rust_init_data = {0};
static int g_initialized = 0;
static int g_rust_levels_forwarded[4] = {0, 0, 0, 0}; // Track which levels we've forwarded to Rust

// Forward declarations
static void gdext_c_entry_initialize_level(void* userdata, GDExtensionInitializationLevel p_level);
static void gdext_c_entry_deinitialize_level(void* userdata, GDExtensionInitializationLevel p_level);

// TDD #99: Minimal C GameNode registration
extern void register_gamenode_minimal_c(void* p_library);

/**
 * TDD #89: Main GDExtension entry point
 * 
 * Called by Godot when loading the extension.
 * This is where we initialize everything.
 */
GDExtensionBool GDX_EXPORT gdextension_initialize(
    GDExtensionInterfaceGetProcAddress p_get_proc_address,
    GDExtensionClassLibraryPtr p_library,
    GDExtensionInitialization* r_initialization
) {
    printf("[C GDExtension] 🚀 TDD #89: Pure C GDExtension initializing...\n");
    printf("[C GDExtension] 💡 This replaces the Rust bridge entirely!\n");
    fflush(stdout);
    
    if (!p_get_proc_address || !r_initialization) {
        fprintf(stderr, "[C GDExtension] ❌ ERROR: Null pointers during initialization\n");
        return 0;
    }
    
    // Step 1: Store proc_address and library pointer globally
    g_proc_address = p_get_proc_address;
    g_library = p_library;
    printf("[C GDExtension] ✅ Stored proc_address: %p\n", g_proc_address);
    printf("[C GDExtension] ✅ Stored library: %p\n", g_library);
    fflush(stdout);
    
    // Step 2: Set up initialization callbacks
    r_initialization->minimum_initialization_level = GDEXTENSION_INITIALIZATION_CORE;
    r_initialization->userdata = NULL;
    r_initialization->initialize = gdext_c_entry_initialize_level;
    r_initialization->deinitialize = gdext_c_entry_deinitialize_level;
    
    g_initialized = 1;
    printf("[C GDExtension] ✅ Pure C GDExtension initialized successfully!\n");
    printf("[C GDExtension] 🎯 NO RUST - just C + Go!\n");
    fflush(stdout);
    
    return 1; // true
}

/**
 * Called when Godot initializes each level (CORE, SERVERS, SCENE, EDITOR)
 */
static void gdext_c_entry_initialize_level(void* userdata, GDExtensionInitializationLevel p_level) {
    const char* level_names[] = {"CORE", "SERVERS", "SCENE", "EDITOR"};
    if (p_level < 4) {
        printf("[C GDExtension] 🎯 Initializing level: %s\n", level_names[p_level]);
        fflush(stdout);
    }
    
    // TDD #99: Register MINIMAL GameNode in PURE C at SCENE level (NO RUST, NO GDSCRIPT!)
    if (p_level == GDEXTENSION_INITIALIZATION_SCENE && !g_rust_levels_forwarded[p_level]) {
        printf("[C GDExtension] 📋 TDD #99: Registering MINIMAL C GameNode (NO RUST, NO GDSCRIPT)...\n");
        fflush(stdout);
        g_rust_levels_forwarded[p_level] = 1;  // Mark as done
        register_gamenode_minimal_c(g_library);
        printf("[C GDExtension] ✅ TDD #99: Pure C GameNode registered (NO GDSCRIPT!)!\n");
        fflush(stdout);
    }
    
    // TDD #90: Load Rust bridge first (for symbols) then Go library
    if (p_level == GDEXTENSION_INITIALIZATION_CORE && !g_go_library) {
        // TDD #104: Step 0: Load C bridge FIRST with RTLD_GLOBAL so its symbols take precedence!
        printf("[C GDExtension] 📦 TDD #104: Loading C bridge FIRST (for gdext_go_get_node)...\n");
        fflush(stdout);
        
        const char* c_bridge_path = "bin/macos/libgdext_c_bridge.dylib";
        void* c_bridge = dlopen(c_bridge_path, RTLD_NOW | RTLD_GLOBAL);
        
        if (!c_bridge) {
            fprintf(stderr, "[C GDExtension] ❌ Failed to load C bridge: %s\n", dlerror());
            fprintf(stderr, "[C GDExtension] 💡 Tried path: %s\n", c_bridge_path);
            fflush(stderr);
            return;
        }
        
        printf("[C GDExtension] ✅ C bridge loaded FIRST: %p\n", c_bridge);
        printf("[C GDExtension] 💡 C bridge gdext_go_get_node will override Rust version!\n");
        fflush(stdout);
        
        // TDD #106: Initialize C bridge with our proc_address
        printf("[C GDExtension] 🔧 TDD #106: Initializing C bridge with proc_address...\n");
        fflush(stdout);
        
        typedef void (*InitCBridgeFunc)(void*);
        InitCBridgeFunc init_c_bridge = (InitCBridgeFunc)dlsym(c_bridge, "gdext_c_bridge_init_with_proc_address");
        
        if (!init_c_bridge) {
            fprintf(stderr, "[C GDExtension] ❌ TDD #106: Failed to find gdext_c_bridge_init_with_proc_address!\n");
            fflush(stderr);
        } else {
            init_c_bridge(g_proc_address);
            printf("[C GDExtension] ✅ TDD #106: C bridge initialized with proc_address!\n");
            fflush(stdout);
        }
        
        // TDD #111: Rust bridge NO LONGER NEEDED!
        // All critical functionality is now pure C:
        // - StringName creation: string_name_helper.c (uses Godot C API)
        // - Scene access: scene_access.c (uses Godot C API)
        // - GameNode: game_node_minimal.c (uses Godot C API)
        // - Process notifications: game_node_minimal.c (uses Godot C API)
        printf("[C GDExtension] 🎉 TDD #111: 100%% PURE C+Go - NO RUST NEEDED!\n");
        printf("[C GDExtension] ✅ All gdext_* functions replaced with pure C implementations\n");
        fflush(stdout);
        
        /*
        // TDD #111: REMOVED - Rust bridge no longer needed!
        const char* rust_bridge_path = "bin/macos/libgdext_rust_bridge.dylib";
        g_rust_bridge = dlopen(rust_bridge_path, RTLD_NOW | RTLD_GLOBAL);
        
        if (!g_rust_bridge) {
            fprintf(stderr, "[C GDExtension] ❌ Failed to load Rust bridge: %s\n", dlerror());
            fprintf(stderr, "[C GDExtension] 💡 Tried path: %s\n", rust_bridge_path);
            fflush(stderr);
            return;
        }
        
        printf("[C GDExtension] ✅ Rust bridge loaded (for symbols only): %p\n", g_rust_bridge);
        printf("[C GDExtension] 💡 Rust bridge provides gdext_* utility functions ONLY\n");
        printf("[C GDExtension] 🎯 TDD #97: NOT using Rust for GameNode - implementing in PURE C!\n");
        printf("[C GDExtension] 💪 100%% C GDExtension - NO RUST IN CRITICAL PATH!\n");
        fflush(stdout);
        */
        
        // Load Go game logic library (the only dynamic library we need!)  
        // TDD #90: Use RTLD_LAZY since dependencies are already loaded
        printf("[C GDExtension] 📦 TDD #90: Loading Go game logic library...\n");
        fflush(stdout);
        
        const char* go_lib_path = "bin/macos/game_logic.dylib";
        g_go_library = dlopen(go_lib_path, RTLD_LAZY | RTLD_GLOBAL);
        
        if (!g_go_library) {
            fprintf(stderr, "[C GDExtension] ❌ Failed to load Go library: %s\n", dlerror());
            fprintf(stderr, "[C GDExtension] 💡 Tried path: %s\n", go_lib_path);
            fflush(stderr);
            return;
        }
        
        printf("[C GDExtension] ✅ Go library loaded: %p\n", g_go_library);
        fflush(stdout);
        
        // TDD #89: Call Go's SetProcAddress to pass proc_addr
        printf("[C GDExtension] 🔗 TDD #89: Passing proc_addr to Go...\n");
        fflush(stdout);
        
        typedef void (*SetProcAddressFunc)(void*);
        SetProcAddressFunc set_proc = (SetProcAddressFunc)dlsym(g_go_library, "SetProcAddress");
        
        if (!set_proc) {
            fprintf(stderr, "[C GDExtension] ❌ Failed to find SetProcAddress: %s\n", dlerror());
            fprintf(stderr, "[C GDExtension] 💡 Ensure main.go exports: //export SetProcAddress\n");
            fflush(stderr);
            return;
        }
        
        set_proc(g_proc_address);
        printf("[C GDExtension] ✅ proc_addr passed to Go!\n");
        fflush(stdout);
        
        printf("[C GDExtension] 🎉 TDD #89: C→Go initialization complete!\n");
        fflush(stdout);
    }
}

/**
 * Called when Godot deinitializes each level
 */
static void gdext_c_entry_deinitialize_level(void* userdata, GDExtensionInitializationLevel p_level) {
    const char* level_names[] = {"CORE", "SERVERS", "SCENE", "EDITOR"};
    if (p_level < 4) {
        printf("[C GDExtension] 🔽 Deinitializing level: %s\n", level_names[p_level]);
        fflush(stdout);
    }
    
    // TDD #92: Do NOT forward level deinit to Rust bridge
    // We're not managing its lifecycle, only using it for GameNode registration
    
    // Unload libraries on CORE level
    if (p_level == GDEXTENSION_INITIALIZATION_CORE) {
        if (g_go_library) {
            printf("[C GDExtension] 🔌 Unloading Go library...\n");
            fflush(stdout);
            dlclose(g_go_library);
            g_go_library = NULL;
        }
        if (g_rust_bridge) {
            printf("[C GDExtension] 🔌 Unloading Rust bridge...\n");
            fflush(stdout);
            dlclose(g_rust_bridge);
            g_rust_bridge = NULL;
        }
    }
}

/**
 * TDD #89: Get the stored proc_address
 * This allows other C files to access it without needing the Rust bridge
 */
void* gdext_c_entry_get_proc_address(void) {
    return g_proc_address;
}

