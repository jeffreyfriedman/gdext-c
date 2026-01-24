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
    fprintf(stderr, "[gdext-c] 🔍 TDD #129: gdext_call_method called: method=%s, object=%p, args=%d\n", 
            method_name, object, arg_count);
    
    if (!gdext_c_is_initialized()) {
        fprintf(stderr, "[gdext-c] ❌ gdext_call_method: not initialized!\n");
        return NULL;
    }
    fprintf(stderr, "[gdext-c] 🔍 TDD #129: gdext-c is initialized\n");
    
    if (!object || !method_name) {
        fprintf(stderr, "[gdext-c] ❌ gdext_call_method: NULL object or method_name!\n");
        return NULL;
    }
    fprintf(stderr, "[gdext-c] 🔍 TDD #129: object and method_name valid\n");
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    fprintf(stderr, "[gdext-c] 🔍 TDD #129: Got interface functions: %p\n", (void*)iface);
    
    // TDD #160: CRITICAL FIX - variant_call expects GDExtensionVariantPtr, not Object!
    // We need to wrap the object in a Variant first
    fprintf(stderr, "[gdext-c] 🔍 TDD #160: Wrapping object in Variant...\n");
    GDExtensionVariantPtr object_variant = gdext_variant_from_object(object);
    if (!object_variant) {
        fprintf(stderr, "[gdext-c] ❌ gdext_call_method: gdext_variant_from_object failed!\n");
        return NULL;
    }
    fprintf(stderr, "[gdext-c] ✅ TDD #160: Object wrapped in Variant: %p\n", object_variant);
    
    // Create StringName for method (stack-allocated buffer)
    fprintf(stderr, "[gdext-c] 🔍 TDD #129: Creating StringName for method '%s'...\n", method_name);
    unsigned char method_sn_buffer[256];
    GDExtensionStringNamePtr method_sn = (GDExtensionStringNamePtr)method_sn_buffer;
    iface->string_name_new_with_latin1_chars(method_sn, method_name, 0); // 3 args: dest, source, is_static
    fprintf(stderr, "[gdext-c] 🔍 TDD #129: StringName created\n");
    
    // Allocate return variant
    fprintf(stderr, "[gdext-c] 🔍 TDD #129: Allocating return variant...\n");
    GDExtensionVariantPtr ret = malloc(sizeof(GDExtensionUninitializedVariantPtr));
    if (!ret) {
        fprintf(stderr, "[gdext-c] ❌ gdext_call_method: ret malloc failed!\n");
        iface->variant_destroy(object_variant);
        free(object_variant);
        return NULL;
    }
    fprintf(stderr, "[gdext-c] 🔍 TDD #129: Return variant allocated: %p\n", ret);
    
    // Initialize return variant as NIL
    fprintf(stderr, "[gdext-c] 🔍 TDD #129: Initializing return variant as NIL...\n");
    iface->variant_new_nil(ret);
    fprintf(stderr, "[gdext-c] 🔍 TDD #129: Return variant initialized\n");
    
    // Convert args to GDExtensionConstVariantPtr array
    GDExtensionConstVariantPtr* arg_ptrs = NULL;
    if (arg_count > 0 && args) {
        fprintf(stderr, "[gdext-c] 🔍 TDD #129: Converting %d arguments...\n", arg_count);
        arg_ptrs = malloc(sizeof(GDExtensionConstVariantPtr) * arg_count);
        if (!arg_ptrs) {
            fprintf(stderr, "[gdext-c] ❌ gdext_call_method: arg_ptrs malloc failed!\n");
            iface->variant_destroy(object_variant);
            free(object_variant);
            free(ret);
            return NULL;
        }
        
        for (int i = 0; i < arg_count; i++) {
            fprintf(stderr, "[gdext-c] 🔍 TDD #129:   arg[%d] = %p\n", i, args[i]);
            arg_ptrs[i] = args[i];
        }
        fprintf(stderr, "[gdext-c] 🔍 TDD #129: Arguments converted\n");
    }
    
    // Call the method using variant_call (TDD #160: Pass object_variant, not raw object!)
    fprintf(stderr, "[gdext-c] 🔧 TDD #160: About to call variant_call with wrapped object...\n");
    fprintf(stderr, "[gdext-c]   object_variant=%p, method_sn=%p, arg_ptrs=%p, arg_count=%d, ret=%p\n",
            object_variant, (void*)method_sn, (void*)arg_ptrs, arg_count, ret);
    fprintf(stderr, "[gdext-c]   iface->variant_call=%p\n", (void*)iface->variant_call);
    
    GDExtensionCallError error;
    memset(&error, 0, sizeof(error));
    
    fprintf(stderr, "[gdext-c] 🔧 TDD #160: Calling variant_call NOW (with Variant-wrapped object)...\n");
    iface->variant_call(
        object_variant,      // TDD #160: Pass Variant containing object, not raw object!
        method_sn,           // Method name (StringName pointer)
        arg_ptrs,            // Arguments
        arg_count,           // Argument count
        ret,                 // Return value
        &error               // Error info
    );
    fprintf(stderr, "[gdext-c] ✅ TDD #160: variant_call returned! error.error=%d\n", error.error);
    
    if (error.error != 0) {
        fprintf(stderr, "[gdext-c] ⚠️  TDD #131: Call error details:\n");
        fprintf(stderr, "[gdext-c]    error=%d, argument=%d, expected=%d\n", 
                error.error, error.argument, error.expected);
    }
    
    // Clean up temporary allocations
    if (arg_ptrs) {
        free(arg_ptrs);
    }
    
    // TDD #161: CRITICAL FIX - StringName needs explicit cleanup!
    // Even stack-allocated builtins like StringName have internal heap allocations
    // We must call the destructor to free them
    fprintf(stderr, "[gdext-c] 🔧 TDD #161: Destroying StringName (type 21)...\n");
    
    // Get the StringName destructor from Godot
    // GDEXTENSION_VARIANT_TYPE_STRING_NAME = 21
    GDExtensionPtrDestructor string_name_destructor = 
        iface->variant_get_ptr_destructor(21); // 21 = STRING_NAME
    
    if (string_name_destructor) {
        string_name_destructor(method_sn);
        fprintf(stderr, "[gdext-c] ✅ TDD #161: StringName destroyed!\n");
    } else {
        fprintf(stderr, "[gdext-c] ⚠️  TDD #161: No StringName destructor found (may leak)\n");
    }
    
    // TDD #160: Clean up the object_variant wrapper
    iface->variant_destroy(object_variant);
    free(object_variant);
    
    // Check for errors
    if (error.error != 0) { // GDEXTENSION_CALL_OK = 0
        fprintf(stderr, "[gdext-c] ❌ gdext_call_method: Call failed with error %d\n", error.error);
        fprintf(stderr, "[gdext-c]    Method: %s, Object: %p, Args: %d\n", 
                method_name, object, arg_count);
        iface->variant_destroy(ret);
        free(ret);
        return NULL;
    }
    
    fprintf(stderr, "[gdext-c] ✅ TDD #160: gdext_call_method SUCCESS!\n");
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
 * This is CRITICAL for add_child, remove_child, queue_free, etc.
 * GDExtension REQUIRES scene tree modifications to be deferred to avoid crashes.
 * 
 * @param object The Godot object to call the method on
 * @param method_name The name of the method to defer
 * @param args Array of Variant pointers for the deferred method call
 * @param arg_count Number of arguments
 * @return Variant pointer (usually NIL for deferred calls)
 */
void* gdext_call_method_deferred(void* object, const char* method_name, void** args, int arg_count) {
    fprintf(stderr, "[gdext-c] 🔧 TDD #133: gdext_call_method_deferred START\n");
    fprintf(stderr, "[gdext-c]   object=%p, method='%s', arg_count=%d\n", object, method_name, arg_count);
    
    if (!gdext_c_is_initialized()) {
        fprintf(stderr, "[gdext-c] ❌ TDD #133: not initialized!\n");
        return NULL;
    }
    
    if (!object || !method_name) {
        fprintf(stderr, "[gdext-c] ❌ TDD #133: NULL object or method_name!\n");
        return NULL;
    }
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    
    // Create StringName for the method to defer
    unsigned char method_sn_buffer[256];
    GDExtensionStringNamePtr method_sn = (GDExtensionStringNamePtr)method_sn_buffer;
    iface->string_name_new_with_latin1_chars(method_sn, method_name, 0);
    fprintf(stderr, "[gdext-c] ✅ TDD #133: Created StringName for '%s'\n", method_name);
    
    // Convert StringName to Variant (first argument to call_deferred)
    GDExtensionVariantPtr method_name_variant = malloc(sizeof(GDExtensionUninitializedVariantPtr));
    if (!method_name_variant) {
        fprintf(stderr, "[gdext-c] ❌ TDD #133: malloc failed for method_name_variant!\n");
        return NULL;
    }
    
    // Use variant_new_copy to convert StringName to variant
    iface->variant_new_copy(method_name_variant, (GDExtensionConstVariantPtr)method_sn);
    fprintf(stderr, "[gdext-c] ✅ TDD #133: Converted method name to variant\n");
    
    // Build arguments array: [method_name_variant, ...original args]
    int total_arg_count = arg_count + 1;
    GDExtensionConstVariantPtr* deferred_args = malloc(sizeof(GDExtensionConstVariantPtr) * total_arg_count);
    if (!deferred_args) {
        fprintf(stderr, "[gdext-c] ❌ TDD #133: malloc failed for deferred_args!\n");
        iface->variant_destroy(method_name_variant);
        free(method_name_variant);
        return NULL;
    }
    
    // First arg is the method name
    deferred_args[0] = method_name_variant;
    
    // Copy remaining args
    for (int i = 0; i < arg_count; i++) {
        deferred_args[i + 1] = args[i];
        fprintf(stderr, "[gdext-c] 🔧 TDD #133:   deferred_args[%d] = %p (original arg[%d])\n", 
                i + 1, args[i], i);
    }
    
    // Create StringName for "call_deferred"
    unsigned char call_deferred_sn_buffer[256];
    GDExtensionStringNamePtr call_deferred_sn = (GDExtensionStringNamePtr)call_deferred_sn_buffer;
    iface->string_name_new_with_latin1_chars(call_deferred_sn, "call_deferred", 0);
    fprintf(stderr, "[gdext-c] ✅ TDD #133: Created StringName for 'call_deferred'\n");
    
    // Allocate return variant
    GDExtensionVariantPtr ret = malloc(sizeof(GDExtensionUninitializedVariantPtr));
    if (!ret) {
        fprintf(stderr, "[gdext-c] ❌ TDD #133: malloc failed for ret!\n");
        iface->variant_destroy(method_name_variant);
        free(method_name_variant);
        free(deferred_args);
        return NULL;
    }
    iface->variant_new_nil(ret);
    
    // Call call_deferred(method_name, ...args)
    fprintf(stderr, "[gdext-c] 🔧 TDD #133: Calling variant_call(object, 'call_deferred', %d args)...\n", total_arg_count);
    GDExtensionCallError error;
    memset(&error, 0, sizeof(error));
    
    iface->variant_call(
        object,              // Instance
        call_deferred_sn,    // Method: "call_deferred"
        deferred_args,       // Arguments: [method_name, ...original args]
        total_arg_count,     // Argument count
        ret,                 // Return value
        &error               // Error info
    );
    
    fprintf(stderr, "[gdext-c] ✅ TDD #133: variant_call returned! error.error=%d\n", error.error);
    
    // Clean up
    iface->variant_destroy(method_name_variant);
    free(method_name_variant);
    free(deferred_args);
    
    if (error.error != 0) {
        fprintf(stderr, "[gdext-c] ❌ TDD #133: Call failed with error %d\n", error.error);
        iface->variant_destroy(ret);
        free(ret);
        return NULL;
    }
    
    fprintf(stderr, "[gdext-c] ✅ TDD #133: gdext_call_method_deferred SUCCESS!\n");
    return ret;
}

/**
 * Helper: Call a method deferred with 1 argument (perfect for add_child!)
 */
void* gdext_call_method1_deferred(void* object, const char* method_name, void* arg1) {
    void* args[1] = {arg1};
    return gdext_call_method_deferred(object, method_name, args, 1);
}

/**
 * TDD #134 SIMPLIFIED: Just call add_child directly using object_method_bind_ptrcall
 * 
 * GDExtension handles the scene tree safety internally. We don't need call_deferred!
 * 
 * @param parent_object The parent object (NOT a variant)
 * @param child_object The child object to add (NOT a variant)
 * @return Variant pointer (usually NIL)
 */
void* gdext_add_child_deferred(void* parent_object, void* child_object) {
    fprintf(stderr, "[gdext-c] 🔧 TDD #176: gdext_add_child_deferred START (PROPER call_deferred with varargs!)\n");
    fprintf(stderr, "[gdext-c]   parent=%p, child=%p\n", parent_object, child_object);
    
    if (!gdext_c_is_initialized()) {
        fprintf(stderr, "[gdext-c] ❌ TDD #176: not initialized!\n");
        return NULL;
    }
    
    if (!parent_object || !child_object) {
        fprintf(stderr, "[gdext-c] ❌ TDD #176: NULL parent or child!\n");
        return NULL;
    }
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    
    // TDD #176: Use Object.call_deferred (vararg method) with object_method_bind_call
    // This is the CORRECT way to defer method calls in GDExtension!
    
    // Create StringName for "Object" class
    unsigned char object_class_sn[256];
    iface->string_name_new_with_latin1_chars(object_class_sn, "Object", 0);
    
    // Create StringName for "call_deferred" method
    unsigned char call_deferred_sn[256];
    iface->string_name_new_with_latin1_chars(call_deferred_sn, "call_deferred", 0);
    
    fprintf(stderr, "[gdext-c] 🔧 TDD #176: Getting method bind for Object.call_deferred (vararg)...\n");
    
    // Get method bind for call_deferred (hash: 3400424181 - vararg method!)
    GDExtensionMethodBindPtr method_bind = iface->classdb_get_method_bind(
        object_class_sn,
        call_deferred_sn,
        3400424181  // Hash for Object.call_deferred (vararg)
    );
    
    if (!method_bind) {
        fprintf(stderr, "[gdext-c] ❌ TDD #176: Failed to get method bind for call_deferred!\n");
        return NULL;
    }
    
    fprintf(stderr, "[gdext-c] ✅ TDD #176: Got method bind for call_deferred: %p\n", method_bind);
    
    // Prepare arguments for call_deferred("add_child", child_node)
    // Argument 1: StringName "add_child"
    unsigned char add_child_sn[256];
    iface->string_name_new_with_latin1_chars(add_child_sn, "add_child", 0);
    
    // Create Variant from StringName (type 21 = GDEXTENSION_VARIANT_TYPE_STRING_NAME)
    GDExtensionVariantPtr method_name_variant = malloc(sizeof(GDExtensionUninitializedVariantPtr));
    GDExtensionVariantFromTypeConstructorFunc string_name_to_variant = iface->get_variant_from_type_constructor(GDEXTENSION_VARIANT_TYPE_STRING_NAME);
    string_name_to_variant(method_name_variant, add_child_sn);
    
    // Argument 2: Object (child_node) - properly create variant from object
    GDExtensionVariantPtr child_variant = gdext_variant_from_object(child_object);
    if (!child_variant) {
        fprintf(stderr, "[gdext-c] ❌ TDD #176: Failed to create variant from child object!\n");
        if (method_name_variant) {
            iface->variant_destroy(method_name_variant);
            free(method_name_variant);
        }
        return NULL;
    }
    
    // Build argument array for vararg call
    GDExtensionVariantPtr args[2];
    args[0] = method_name_variant;
    args[1] = child_variant;
    
    // Allocate return variant
    GDExtensionVariantPtr ret = malloc(sizeof(GDExtensionUninitializedVariantPtr));
    iface->variant_new_nil(ret);
    
    // Error info
    GDExtensionCallError error;
    
    fprintf(stderr, "[gdext-c] 🔧 TDD #176: Calling object_method_bind_call (vararg support)...\n");
    
    // TDD #176 KEY: Use object_method_bind_call for vararg methods!
    // This accepts Variant** (array of variant pointers), unlike ptrcall
    iface->object_method_bind_call(
        method_bind,
        parent_object,
        (const GDExtensionConstVariantPtr*)args,
        2,  // 2 arguments: method_name, child_node
        ret,
        &error
    );
    
    // Free temporary variants
    if (method_name_variant) {
        iface->variant_destroy(method_name_variant);
        free(method_name_variant);
    }
    if (child_variant) {
        gdext_variant_free(child_variant);  // Use gdext_variant_free for consistency
    }
    
    if (error.error != GDEXTENSION_CALL_OK) {
        fprintf(stderr, "[gdext-c] ❌ TDD #176: call_deferred failed with error: %d\n", error.error);
        if (ret) {
            iface->variant_destroy(ret);
            free(ret);
        }
        return NULL;
    }
    
    fprintf(stderr, "[gdext-c] ✅ TDD #176: add_child DEFERRED via call_deferred (thread-safe)!\n");
    fprintf(stderr, "[gdext-c] ✅ TDD #176: gdext_add_child_deferred COMPLETE!\n");
    
    // Return the result variant (call_deferred typically returns NIL)
    return ret;
}

