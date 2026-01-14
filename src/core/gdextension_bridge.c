/**
 * GDExtension C Bridge - Implementation
 * 
 * Minimal C bridge for Go → Godot communication
 * TDD Phase 1: Initialization only
 */

#include "gdextension_bridge.h"
#include "gdextension_interface.h"
#include <stdio.h>
#include <string.h>

// Global state
static GDExtensionInterfaceGetProcAddress g_get_proc_address = NULL;
static bool g_initialized = false;

// TDD #106: Global StringName function (used by multiple C files)
GDExtensionInterfaceStringNameNewWithLatin1Chars g_string_name_new_with_latin1_chars = NULL;

// Forward declarations for initialization callbacks
static void gdext_c_bridge_initialize_level(void* userdata, GDExtensionInitializationLevel p_level);
static void gdext_c_bridge_deinitialize_level(void* userdata, GDExtensionInitializationLevel p_level);

/**
 * TDD Test 1: Initialize GDExtension
 * 
 * Called by Godot when loading the extension.
 * Stores the interface pointer for later use.
 */
GDExtensionBool gdext_c_bridge_entry_point(
    GDExtensionInterfaceGetProcAddress p_get_proc_address,
    GDExtensionClassLibraryPtr p_library,
    GDExtensionInitialization* r_initialization
) {
    printf("[C Bridge] 🚀 TDD Phase 1: GDExtension initialization starting...\n");
    
    if (!p_get_proc_address || !r_initialization) {
        fprintf(stderr, "[C Bridge] ❌ ERROR: Null pointers during initialization\n");
        return 0; // false
    }
    
    // Store the get_proc_address function
    g_get_proc_address = p_get_proc_address;
    
    // Build the interface structure
    // In Godot 4.3+, the interface is built by calling get_proc_address for each function
    printf("[C Bridge] 📍 Storing GDExtension interface pointer...\n");
    
    // For now, we'll access functions via get_proc_address
    // Later we can build a full interface struct if needed
    
    // Set up initialization callbacks
    r_initialization->minimum_initialization_level = GDEXTENSION_INITIALIZATION_CORE;
    r_initialization->userdata = NULL;
    r_initialization->initialize = gdext_c_bridge_initialize_level;
    r_initialization->deinitialize = gdext_c_bridge_deinitialize_level;
    
    g_initialized = true;
    printf("[C Bridge] ✅ GDExtension C bridge initialized successfully\n");
    
    return 1; // true
}

/**
 * Called when Godot initializes each level (CORE, SERVERS, SCENE, EDITOR)
 */
static void gdext_c_bridge_initialize_level(void* userdata, GDExtensionInitializationLevel p_level) {
    const char* level_names[] = {"CORE", "SERVERS", "SCENE", "EDITOR"};
    if (p_level < 4) {
        printf("[C Bridge] 🎯 Initializing level: %s\n", level_names[p_level]);
    }
    
    // TDD #81: Disabled test_array_insert() - using GDScript workaround instead
    // (test code crashes, but workaround documented in GODOT_C_API_LIMITATION.md)
    
    // TDD #73: Test disabled - using GDScript workaround
    // if (p_level == 2) {  // SCENE level
    //     test_array_insert();
    // }
}

/**
 * Called when Godot deinitializes each level
 */
static void gdext_c_bridge_deinitialize_level(void* userdata, GDExtensionInitializationLevel p_level) {
    const char* level_names[] = {"CORE", "SERVERS", "SCENE", "EDITOR"};
    if (p_level < 4) {
        printf("[C Bridge] 🔽 Deinitializing level: %s\n", level_names[p_level]);
    }
}

/**
 * TDD Test 2: Retrieve the get_proc_address function
 * 
 * Returns the stored function pointer, or NULL if not initialized.
 */
void* gdext_c_bridge_get_proc_address(void) {
    return (void*)g_get_proc_address;
}

/**
 * TDD #106: Initialize C bridge with proc_address
 * 
 * Called from C GDExtension entry point after loading C bridge library.
 * This allows C bridge to access Godot API without being a GDExtension itself.
 */
void gdext_c_bridge_init_with_proc_address(void* p_proc_address) {
    printf("[C Bridge] 🔧 TDD #106: Initializing with proc_address: %p\n", p_proc_address);
    fflush(stdout);
    
    g_get_proc_address = (GDExtensionInterfaceGetProcAddress)p_proc_address;
    
    // TDD #106: Initialize StringName function (needed by all C files)
    printf("[C Bridge] 🔧 TDD #106: Initializing g_string_name_new_with_latin1_chars...\n");
    fflush(stdout);
    
    g_string_name_new_with_latin1_chars = 
        (GDExtensionInterfaceStringNameNewWithLatin1Chars)
        g_get_proc_address("string_name_new_with_latin1_chars");
    
    if (!g_string_name_new_with_latin1_chars) {
        fprintf(stderr, "[C Bridge] ❌ TDD #106: Failed to get string_name_new_with_latin1_chars!\n");
        fflush(stderr);
    } else {
        printf("[C Bridge] ✅ TDD #106: g_string_name_new_with_latin1_chars initialized: %p\n", 
               (void*)g_string_name_new_with_latin1_chars);
        fflush(stdout);
    }
    
    g_initialized = true;
    
    printf("[C Bridge] ✅ TDD #106: C bridge initialized!\n");
    fflush(stdout);
}

/**
 * TDD Test 3: Check if bridge is initialized
 */
bool gdext_c_bridge_is_initialized(void) {
    return g_initialized;
}

/**
 * TDD Test 4: Get a specific interface function by name
 * 
 * This is how we'll call Godot functions - by looking them up dynamically.
 */
void* gdext_c_bridge_get_function(const char* function_name) {
    if (!g_initialized || !g_get_proc_address) {
        fprintf(stderr, "[C Bridge] ❌ ERROR: Bridge not initialized\n");
        return NULL;
    }
    
    GDExtensionInterfaceFunctionPtr func = g_get_proc_address(function_name);
    if (!func) {
        fprintf(stderr, "[C Bridge] ⚠️  WARNING: Function '%s' not found\n", function_name);
        return NULL;
    }
    
    printf("[C Bridge] ✅ Found function: %s\n", function_name);
    return (void*)func;
}

