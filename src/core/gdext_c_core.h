/**
 * @file gdext_c_core.h
 * @brief Internal header for gdext-c core functionality
 * 
 * This is NOT part of the public API - only used internally by gdext-c.
 * Provides access to GDExtension interface functions.
 */

#ifndef GDEXT_C_CORE_H
#define GDEXT_C_CORE_H

#include "../../include/gdext_c.h"
#include "../../include/gdextension_interface.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Structure holding all GDExtension interface function pointers
 * 
 * This is populated during gdext_c_initialize() and provides access
 * to all Godot GDExtension C API functions.
 */
typedef struct {
    // Memory management
    GDExtensionInterfaceMemAlloc mem_alloc;
    GDExtensionInterfaceMemFree mem_free;
    
    // Variant operations
    GDExtensionInterfaceVariantNewCopy variant_new_copy;
    GDExtensionInterfaceVariantNewNil variant_new_nil;
    GDExtensionInterfaceVariantDestroy variant_destroy;
    GDExtensionInterfaceVariantCall variant_call;
    GDExtensionInterfaceGetVariantFromTypeConstructor get_variant_from_type_constructor;
    GDExtensionInterfaceGetVariantToTypeConstructor get_variant_to_type_constructor;
    GDExtensionInterfaceVariantGetType variant_get_type;
    
    // String operations
    GDExtensionInterfaceStringNewWithLatin1Chars string_new_with_latin1_chars;
    GDExtensionInterfaceStringToLatin1Chars string_to_latin1_chars;
    
    // StringName operations
    GDExtensionInterfaceStringNameNewWithLatin1Chars string_name_new_with_latin1_chars;
    
    // Object operations
    GDExtensionInterfaceClassdbConstructObject classdb_construct_object;
    GDExtensionInterfaceObjectDestroy object_destroy;
    GDExtensionInterfaceObjectGetInstanceFromId object_get_instance_from_id;
    GDExtensionInterfaceObjectGetInstanceId object_get_instance_id;
    GDExtensionInterfaceObjectSetInstance object_set_instance;
    GDExtensionInterfaceObjectGetClassName object_get_class_name;
    GDExtensionInterfaceObjectCastTo object_cast_to;
    
    // Method calling
    GDExtensionInterfaceClassdbGetMethodBind classdb_get_method_bind;
    GDExtensionInterfaceObjectMethodBindPtrcall object_method_bind_ptrcall;
    
    // Singleton access
    GDExtensionInterfaceGlobalGetSingleton global_get_singleton;
    
    // Class registration (TDD #156: Add version 2 for official example compatibility!)
    GDExtensionInterfaceClassdbRegisterExtensionClass2 classdb_register_extension_class2;
    GDExtensionInterfaceClassdbRegisterExtensionClass3 classdb_register_extension_class3;
    
    // Property/method info
    // (Add more as needed)
    
} GDExtensionInterface;

/**
 * @brief Get the populated GDExtension interface functions
 * 
 * @return Pointer to the interface struct, or NULL if not initialized
 */
const GDExtensionInterface* gdext_c_get_interface_functions(void);

/**
 * @brief Get the raw proc_address function pointer
 * 
 * @return The proc_address, or NULL if not initialized
 */
gdext_c_proc_address_func gdext_c_get_proc_address_internal(void);

/**
 * @brief Set the library handle (TDD #155 - for GDExtension entry point)
 * @param library The library handle from Godot
 */
void gdext_c_set_library_handle(GDExtensionClassLibraryPtr library);

/**
 * @brief Get the library handle (TDD #155 - for GDExtension entry point)
 * @return The stored library handle, or NULL if not set
 */
GDExtensionClassLibraryPtr gdext_c_get_library_handle(void);

#ifdef __cplusplus
}
#endif

#endif /* GDEXT_C_CORE_H */

