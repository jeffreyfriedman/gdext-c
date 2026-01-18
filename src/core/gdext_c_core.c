/**
 * @file gdext_c_core.c
 * @brief Core initialization for gdext-c library
 * 
 * TDD #122: Pure library (no GDExtension entry point - that's in the game!)
 * This file provides initialization functions that the game calls.
 */

#include "../../include/gdext_c.h"
#include "gdext_c_core.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

// Global state (exported for use by other gdext-c files)
static GDExtensionInterfaceGetProcAddress g_proc_address = NULL;
static GDExtensionInterface g_interface;
static bool g_initialized = false;
static GDExtensionClassLibraryPtr g_library_handle = NULL;

// TDD #140: Export interface for generated code
// Generated code expects this exact symbol name
GDExtensionInterface iface_impl;

/**
 * @brief Initialize the gdext-c library
 * 
 * MUST be called before using any other gdext-c functions.
 * Typically called from your game's GDExtension initialization callback.
 * 
 * @param proc_address The GDExtensionInterfaceGetProcAddress function pointer from Godot
 * @return true on success, false on failure
 */
bool gdext_c_initialize(gdext_c_proc_address_func proc_address) {
    if (g_initialized) {
        printf("[gdext-c] 💡 TDD #122: Already initialized\n");
        return true;
    }
    
    if (!proc_address) {
        fprintf(stderr, "[gdext-c] ❌ TDD #122: NULL proc_address!\n");
        return false;
    }
    
    printf("[gdext-c] 🚀 TDD #122: Initializing gdext-c library...\n");
    fflush(stdout);
    
    // Store proc_address for later use
    g_proc_address = (GDExtensionInterfaceGetProcAddress)proc_address;
    
    // TDD #127: Populate GDExtension interface struct with all needed functions
    memset(&g_interface, 0, sizeof(GDExtensionInterface));
    
    // Memory management
    g_interface.mem_alloc = (GDExtensionInterfaceMemAlloc)g_proc_address("mem_alloc");
    g_interface.mem_free = (GDExtensionInterfaceMemFree)g_proc_address("mem_free");
    
    // Variant operations
    g_interface.variant_new_copy = (GDExtensionInterfaceVariantNewCopy)g_proc_address("variant_new_copy");
    g_interface.variant_new_nil = (GDExtensionInterfaceVariantNewNil)g_proc_address("variant_new_nil");
    g_interface.variant_destroy = (GDExtensionInterfaceVariantDestroy)g_proc_address("variant_destroy");
    g_interface.variant_call = (GDExtensionInterfaceVariantCall)g_proc_address("variant_call");
    g_interface.get_variant_from_type_constructor = (GDExtensionInterfaceGetVariantFromTypeConstructor)g_proc_address("get_variant_from_type_constructor");
    g_interface.get_variant_to_type_constructor = (GDExtensionInterfaceGetVariantToTypeConstructor)g_proc_address("get_variant_to_type_constructor");
    g_interface.variant_get_type = (GDExtensionInterfaceVariantGetType)g_proc_address("variant_get_type");
    
    // String operations
    g_interface.string_new_with_latin1_chars = (GDExtensionInterfaceStringNewWithLatin1Chars)g_proc_address("string_new_with_latin1_chars");
    g_interface.string_to_latin1_chars = (GDExtensionInterfaceStringToLatin1Chars)g_proc_address("string_to_latin1_chars");
    
    // StringName operations
    g_interface.string_name_new_with_latin1_chars = (GDExtensionInterfaceStringNameNewWithLatin1Chars)g_proc_address("string_name_new_with_latin1_chars");
    
    // Object operations
    g_interface.classdb_construct_object = (GDExtensionInterfaceClassdbConstructObject)g_proc_address("classdb_construct_object");
    g_interface.object_destroy = (GDExtensionInterfaceObjectDestroy)g_proc_address("object_destroy");
    g_interface.object_get_instance_from_id = (GDExtensionInterfaceObjectGetInstanceFromId)g_proc_address("object_get_instance_from_id");
    g_interface.object_get_instance_id = (GDExtensionInterfaceObjectGetInstanceId)g_proc_address("object_get_instance_id");
    g_interface.object_set_instance = (GDExtensionInterfaceObjectSetInstance)g_proc_address("object_set_instance");
    g_interface.object_get_class_name = (GDExtensionInterfaceObjectGetClassName)g_proc_address("object_get_class_name");
    g_interface.object_cast_to = (GDExtensionInterfaceObjectCastTo)g_proc_address("object_cast_to");
    
    // Method calling
    g_interface.classdb_get_method_bind = (GDExtensionInterfaceClassdbGetMethodBind)g_proc_address("classdb_get_method_bind");
    g_interface.object_method_bind_ptrcall = (GDExtensionInterfaceObjectMethodBindPtrcall)g_proc_address("object_method_bind_ptrcall");
    
    // Singleton access
    g_interface.global_get_singleton = (GDExtensionInterfaceGlobalGetSingleton)g_proc_address("global_get_singleton");
    
    // Class registration (TDD #156: Version 4/5 is what Godot 4.5 actually uses!)
    g_interface.classdb_register_extension_class2 = (GDExtensionInterfaceClassdbRegisterExtensionClass2)g_proc_address("classdb_register_extension_class2");
    g_interface.classdb_register_extension_class3 = (GDExtensionInterfaceClassdbRegisterExtensionClass3)g_proc_address("classdb_register_extension_class3");
    g_interface.classdb_register_extension_class4 = (GDExtensionInterfaceClassdbRegisterExtensionClass4)g_proc_address("classdb_register_extension_class4");
    
    // Verify critical functions were loaded
    if (!g_interface.string_name_new_with_latin1_chars || !g_interface.get_variant_from_type_constructor || !g_interface.variant_call) {
        fprintf(stderr, "[gdext-c] ❌ TDD #122: Failed to load critical GDExtension functions!\n");
        fflush(stderr);
        return false;
    }
    
    g_initialized = true;
    
    // TDD #140: Copy interface to exported symbol for generated code
    memcpy(&iface_impl, &g_interface, sizeof(GDExtensionInterface));
    
    printf("[gdext-c] ✅ TDD #122: gdext-c initialized successfully!\n");
    printf("[gdext-c] 🎉 TDD #127: Variant/method calling functions ready!\n");
    printf("[gdext-c] 🎉 TDD #140: Interface exported for generated code!\n");
    fflush(stdout);
    
    return true;
}

/**
 * @brief Check if gdext-c is initialized
 * @return true if initialized, false otherwise
 */
bool gdext_c_is_initialized(void) {
    return g_initialized;
}

/**
 * @brief Get the stored proc_address (for internal use)
 * @return The proc_address function pointer, or NULL if not initialized
 */
gdext_c_proc_address_func gdext_c_get_proc_address_internal(void) {
    return (gdext_c_proc_address_func)g_proc_address;
}

/**
 * @brief Set the library handle (TDD #155 - for GDExtension entry point)
 * @param library The library handle from Godot
 */
void gdext_c_set_library_handle(GDExtensionClassLibraryPtr library) {
    g_library_handle = library;
}

/**
 * @brief Get the library handle (TDD #155 - for GDExtension entry point)
 * @return The stored library handle, or NULL if not set
 */
GDExtensionClassLibraryPtr gdext_c_get_library_handle(void) {
    return g_library_handle;
}

/**
 * @brief Get the GDExtension interface functions (TDD #127)
 * @return Pointer to the interface struct, or NULL if not initialized
 */
const GDExtensionInterface* gdext_c_get_interface_functions(void) {
    if (!g_initialized) {
        return NULL;
    }
    return &g_interface;
}

/**
 * @brief Cleanup the library (optional)
 * 
 * Usually not needed - just let the library stay initialized for the lifetime of the process.
 */
void gdext_c_cleanup(void) {
    if (!g_initialized) {
        return;
    }
    
    printf("[gdext-c] 🧹 TDD #122: Cleaning up gdext-c...\n");
    fflush(stdout);
    
    g_proc_address = NULL;
    memset(&g_interface, 0, sizeof(GDExtensionInterface));
    g_initialized = false;
    
    printf("[gdext-c] ✅ TDD #122: gdext-c cleanup complete\n");
    fflush(stdout);
}

