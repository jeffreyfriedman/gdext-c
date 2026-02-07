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
#include "../core/gdext_c_signal_handler.h"  // TDD Option 3: Crash signal handlers
#include "../core/gdext_c_object_registry.h"  // TDD #202: Object lifetime tracking
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Set to 1 to enable verbose debug logging (VERY slow - only for crash diagnosis)
#ifndef GDEXT_C_VERBOSE_METHOD_CALLING
#define GDEXT_C_VERBOSE_METHOD_CALLING 0
#endif

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
    
    // TDD #202: Validate object before calling method
    char error_buf[256];
    if (gdext_registry_check_valid(object, method_name, error_buf, sizeof(error_buf)) != 0) {
        fprintf(stderr, "[gdext-c] 🚨 OBJECT LIFETIME ERROR: %s\n", error_buf);
        fflush(stderr);
        fprintf(stderr, "[gdext-c] 🚨 This is a use-after-free bug! Aborting to prevent corruption.\n");
        fflush(stderr);
        abort();
    }
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    
    // Wrap the object in a Variant (variant_call expects GDExtensionVariantPtr)
    GDExtensionVariantPtr object_variant = gdext_variant_from_object(object);
    if (!object_variant) {
        fprintf(stderr, "[gdext-c] ❌ gdext_call_method: gdext_variant_from_object failed for '%s'!\n", method_name);
        return NULL;
    }
    
    // Create StringName for method (stack-allocated buffer)
    unsigned char method_sn_buffer[256];
    GDExtensionStringNamePtr method_sn = (GDExtensionStringNamePtr)method_sn_buffer;
    iface->string_name_new_with_latin1_chars(method_sn, method_name, 0);
    
    // Allocate return variant
    GDExtensionVariantPtr ret = malloc(GDEXT_VARIANT_SIZE);
    if (!ret) {
        fprintf(stderr, "[gdext-c] ❌ gdext_call_method: ret malloc failed!\n");
        iface->variant_destroy(object_variant);
        free(object_variant);
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
            iface->variant_destroy(object_variant);
            free(object_variant);
            free(ret);
            return NULL;
        }
        
        for (int i = 0; i < arg_count; i++) {
            arg_ptrs[i] = args[i];
        }
    }
    
    GDExtensionCallError error;
    memset(&error, 0, sizeof(error));
    
    // Set signal context for crash diagnosis
    gdext_c_signal_set_context("variant_call", method_name, object);
    
    iface->variant_call(
        object_variant,      // Variant containing object
        method_sn,           // Method name (StringName pointer)
        arg_ptrs,            // Arguments
        arg_count,           // Argument count
        ret,                 // Return value
        &error               // Error info
    );
    
    // Clear signal context after successful call
    gdext_c_signal_set_context("idle", "none", NULL);
    
    // Handle get_node returning NULL
    if (strcmp(method_name, "get_node") == 0) {
        GDExtensionVariantType ret_type = iface->variant_get_type(ret);
        if (ret_type == 0) { // NIL = node not found
            iface->variant_destroy(object_variant);
            free(object_variant);
            if (arg_ptrs) free(arg_ptrs);
            // Destroy the StringName
            GDExtensionPtrDestructor string_name_destructor = 
                iface->variant_get_ptr_destructor(21);
            if (string_name_destructor) {
                string_name_destructor(method_sn);
            }
            return NULL;
        }
    }
    
    if (error.error != 0) {
        fprintf(stderr, "[gdext-c] ⚠️  Call error: method=%s, error=%d, argument=%d, expected=%d\n", 
                method_name, error.error, error.argument, error.expected);
    }
    
    // Clean up temporary allocations
    if (arg_ptrs) {
        free(arg_ptrs);
    }
    
    // StringName cleanup
    GDExtensionPtrDestructor string_name_destructor = 
        iface->variant_get_ptr_destructor(21);
    if (string_name_destructor) {
        string_name_destructor(method_sn);
    }
    
    // Clean up the object_variant wrapper
    iface->variant_destroy(object_variant);
    free(object_variant);
    
    // Check for errors
    if (error.error != 0) {
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

/**
 * TDD #133: Call a method via call_deferred (required for scene tree modifications!)
 * 
 * @param object The Godot object to call the method on
 * @param method_name The name of the method to defer
 * @param args Array of Variant pointers for the deferred method call
 * @param arg_count Number of arguments
 * @return Variant pointer (usually NIL for deferred calls)
 */
void* gdext_call_method_deferred(void* object, const char* method_name, void** args, int arg_count) {
    if (!gdext_c_is_initialized()) {
        fprintf(stderr, "[gdext-c] ❌ gdext_call_method_deferred: not initialized!\n");
        return NULL;
    }
    
    if (!object || !method_name) {
        fprintf(stderr, "[gdext-c] ❌ gdext_call_method_deferred: NULL object or method_name!\n");
        return NULL;
    }
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    
    // Create StringName for the method to defer
    unsigned char method_sn_buffer[256];
    GDExtensionStringNamePtr method_sn = (GDExtensionStringNamePtr)method_sn_buffer;
    iface->string_name_new_with_latin1_chars(method_sn, method_name, 0);
    
    // Convert StringName to Variant (first argument to call_deferred)
    GDExtensionVariantPtr method_name_variant = malloc(GDEXT_VARIANT_SIZE);
    if (!method_name_variant) {
        fprintf(stderr, "[gdext-c] ❌ gdext_call_method_deferred: malloc failed!\n");
        return NULL;
    }
    
    iface->variant_new_copy(method_name_variant, (GDExtensionConstVariantPtr)method_sn);
    
    // Build arguments array: [method_name_variant, ...original args]
    int total_arg_count = arg_count + 1;
    GDExtensionConstVariantPtr* deferred_args = malloc(sizeof(GDExtensionConstVariantPtr) * total_arg_count);
    if (!deferred_args) {
        iface->variant_destroy(method_name_variant);
        free(method_name_variant);
        return NULL;
    }
    
    deferred_args[0] = method_name_variant;
    for (int i = 0; i < arg_count; i++) {
        deferred_args[i + 1] = args[i];
    }
    
    // Create StringName for "call_deferred"
    unsigned char call_deferred_sn_buffer[256];
    GDExtensionStringNamePtr call_deferred_sn = (GDExtensionStringNamePtr)call_deferred_sn_buffer;
    iface->string_name_new_with_latin1_chars(call_deferred_sn, "call_deferred", 0);
    
    // Allocate return variant
    GDExtensionVariantPtr ret = malloc(GDEXT_VARIANT_SIZE);
    if (!ret) {
        iface->variant_destroy(method_name_variant);
        free(method_name_variant);
        free(deferred_args);
        return NULL;
    }
    iface->variant_new_nil(ret);
    
    // Call call_deferred(method_name, ...args)
    GDExtensionCallError error;
    memset(&error, 0, sizeof(error));
    
    iface->variant_call(
        object,
        call_deferred_sn,
        deferred_args,
        total_arg_count,
        ret,
        &error
    );
    
    // Clean up
    iface->variant_destroy(method_name_variant);
    free(method_name_variant);
    free(deferred_args);
    
    // Clean up StringNames
    GDExtensionPtrDestructor string_name_destructor = 
        iface->variant_get_ptr_destructor(21);
    if (string_name_destructor) {
        string_name_destructor(method_sn);
        string_name_destructor(call_deferred_sn);
    }
    
    if (error.error != 0) {
        fprintf(stderr, "[gdext-c] ❌ gdext_call_method_deferred: Call failed with error %d\n", error.error);
        iface->variant_destroy(ret);
        free(ret);
        return NULL;
    }
    
    return ret;
}

/**
 * Helper: Call a method deferred with 1 argument
 */
void* gdext_call_method1_deferred(void* object, const char* method_name, void* arg1) {
    void* args[1] = {arg1};
    return gdext_call_method_deferred(object, method_name, args, 1);
}

/**
 * TDD #134: Add child using call_deferred via object_method_bind_call
 * 
 * @param parent_object The parent object (NOT a variant)
 * @param child_object The child object to add (NOT a variant)
 * @return Variant pointer (usually NIL)
 */
void* gdext_add_child_deferred(void* parent_object, void* child_object) {
    if (!gdext_c_is_initialized()) {
        fprintf(stderr, "[gdext-c] ❌ gdext_add_child_deferred: not initialized!\n");
        return NULL;
    }
    
    if (!parent_object || !child_object) {
        fprintf(stderr, "[gdext-c] ❌ gdext_add_child_deferred: NULL parent or child!\n");
        return NULL;
    }
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    
    // Get method bind for Object.call_deferred (vararg method)
    char object_class_sn[256] = {0};
    iface->string_name_new_with_latin1_chars(object_class_sn, "Object", 0);
    
    char call_deferred_sn[256] = {0};
    iface->string_name_new_with_latin1_chars(call_deferred_sn, "call_deferred", 0);
    
    GDExtensionMethodBindPtr method_bind = iface->classdb_get_method_bind(
        object_class_sn,
        call_deferred_sn,
        3400424181  // Hash for Object.call_deferred (vararg)
    );
    
    if (!method_bind) {
        fprintf(stderr, "[gdext-c] ❌ gdext_add_child_deferred: Failed to get method bind!\n");
        return NULL;
    }
    
    // Create arguments for call_deferred("add_child", child_node)
    
    // Argument 1: StringName "add_child"
    char add_child_sn[256] = {0};
    iface->string_name_new_with_latin1_chars(add_child_sn, "add_child", 0);
    
    // Create Variant from StringName
    GDExtensionVariantPtr method_name_variant = malloc(GDEXT_VARIANT_SIZE);
    if (!method_name_variant) {
        return NULL;
    }
    
    GDExtensionVariantFromTypeConstructorFunc string_name_to_variant = 
        iface->get_variant_from_type_constructor(GDEXTENSION_VARIANT_TYPE_STRING_NAME);
    
    if (!string_name_to_variant) {
        free(method_name_variant);
        return NULL;
    }
    
    string_name_to_variant(method_name_variant, add_child_sn);
    
    // Argument 2: Object (child_node) - create variant from object
    GDExtensionVariantPtr child_variant = malloc(GDEXT_VARIANT_SIZE);
    if (!child_variant) {
        iface->variant_destroy(method_name_variant);
        free(method_name_variant);
        return NULL;
    }
    
    GDExtensionVariantFromTypeConstructorFunc object_to_variant = 
        iface->get_variant_from_type_constructor(GDEXTENSION_VARIANT_TYPE_OBJECT);
    
    if (!object_to_variant) {
        iface->variant_destroy(method_name_variant);
        free(method_name_variant);
        free(child_variant);
        return NULL;
    }
    
    object_to_variant(child_variant, &child_object);
    
    // Call Object.call_deferred using object_method_bind_call (for vararg methods)
    GDExtensionConstVariantPtr call_args[2];
    call_args[0] = (GDExtensionConstVariantPtr)method_name_variant;
    call_args[1] = (GDExtensionConstVariantPtr)child_variant;
    
    GDExtensionVariantPtr ret = malloc(GDEXT_VARIANT_SIZE);
    if (!ret) {
        iface->variant_destroy(method_name_variant);
        iface->variant_destroy(child_variant);
        free(method_name_variant);
        free(child_variant);
        return NULL;
    }
    iface->variant_new_nil(ret);
    
    GDExtensionCallError error;
    
    iface->object_method_bind_call(
        method_bind,
        parent_object,
        call_args,
        2,
        ret,
        &error
    );
    
    // Clean up temporary variants
    iface->variant_destroy(method_name_variant);
    iface->variant_destroy(child_variant);
    free(method_name_variant);
    free(child_variant);
    
    if (error.error != GDEXTENSION_CALL_OK) {
        fprintf(stderr, "[gdext-c] ❌ gdext_add_child_deferred: call_deferred failed with error: %d\n", error.error);
        if (ret) {
            iface->variant_destroy(ret);
            free(ret);
        }
        return NULL;
    }
    
    return ret;
}
