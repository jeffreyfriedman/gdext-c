/**
 * gdext_c_method_calling.c - TDD #127: Pure C Method Calling
 * 
 * Replaces Rust bridge function:
 * - gdext_call_method (call methods on Godot objects with Variant args)
 * 
 * This is the MOST CRITICAL function - used by SetProperty, CallMethod1, CallMethod2, etc.
 * 
 * Uses GDExtension C API directly - NO RUST!
 */

#include "../../include/gdext_c.h"
#include "../core/gdext_c_core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Call a method on a Godot object with variant arguments
 * 
 * @param object The Godot object to call the method on
 * @param method_name The name of the method to call
 * @param args Array of Variant pointers (created with gdext_variant_from_*)
 * @param arg_count Number of arguments in the args array
 * @return Variant pointer containing the return value (or NULL on error)
 *         Caller must free with gdext_variant_free()
 */
void* gdext_call_method(void* object, const char* method_name, void** args, int arg_count) {
    if (!gdext_c_is_initialized()) {
        fprintf(stderr, "[gdext-c] ❌ gdext_call_method: not initialized!\n");
        return NULL;
    }
    
    if (!object || !method_name) {
        fprintf(stderr, "[gdext-c] ❌ gdext_call_method: NULL object or method_name!\n");
        return NULL;
    }
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    
    // Create StringName for method (stack-allocated buffer)
    unsigned char method_sn_buffer[256];
    GDExtensionStringNamePtr method_sn = (GDExtensionStringNamePtr)method_sn_buffer;
    iface->string_name_new_with_latin1_chars(method_sn, method_name, 0); // 3 args: dest, source, is_static
    
    // Allocate return variant
    GDExtensionVariantPtr ret = malloc(sizeof(GDExtensionUninitializedVariantPtr));
    if (!ret) {
        fprintf(stderr, "[gdext-c] ❌ gdext_call_method: ret malloc failed!\n");
        return NULL;
    }
    
    // Initialize return variant as NIL
    iface->variant_new_nil(ret);
    
    // Convert args to GDExtensionConstVariantPtr array
    GDExtensionConstVariantPtr* arg_ptrs = NULL;
    if (arg_count > 0 && args) {
        arg_ptrs = malloc(sizeof(GDExtensionConstVariantPtr) * arg_count);
        if (!arg_ptrs) {
            fprintf(stderr, "[gdext-c] ❌ gdext_call_method: arg_ptrs malloc failed!\n");
            free(ret);
            return NULL;
        }
        
        for (int i = 0; i < arg_count; i++) {
            arg_ptrs[i] = args[i];
        }
    }
    
    // Call the method using variant_call
    GDExtensionCallError error;
    iface->variant_call(
        object,              // Instance (the object)
        method_sn,           // Method name (StringName pointer)
        arg_ptrs,            // Arguments
        arg_count,           // Argument count
        ret,                 // Return value
        &error               // Error info
    );
    
    // Clean up temporary allocations
    if (arg_ptrs) {
        free(arg_ptrs);
    }
    // StringName is stack-allocated, no cleanup needed
    
    // Check for errors
    if (error.error != 0) { // GDEXTENSION_CALL_OK = 0
        fprintf(stderr, "[gdext-c] ❌ gdext_call_method: Call failed with error %d\n", error.error);
        fprintf(stderr, "[gdext-c]    Method: %s, Object: %p, Args: %d\n", 
                method_name, object, arg_count);
        iface->variant_destroy(ret);
        free(ret);
        return NULL;
    }
    
    return ret;
}

/**
 * Helper: Call a method with no arguments
 */
void* gdext_call_method0(void* object, const char* method_name) {
    return gdext_call_method(object, method_name, NULL, 0);
}

/**
 * Helper: Call a method with 1 argument
 */
void* gdext_call_method1(void* object, const char* method_name, void* arg1) {
    void* args[1] = {arg1};
    return gdext_call_method(object, method_name, args, 1);
}

/**
 * Helper: Call a method with 2 arguments
 */
void* gdext_call_method2(void* object, const char* method_name, void* arg1, void* arg2) {
    void* args[2] = {arg1, arg2};
    return gdext_call_method(object, method_name, args, 2);
}

/**
 * Helper: Call a method with 3 arguments
 */
void* gdext_call_method3(void* object, const char* method_name, void* arg1, void* arg2, void* arg3) {
    void* args[3] = {arg1, arg2, arg3};
    return gdext_call_method(object, method_name, args, 3);
}

