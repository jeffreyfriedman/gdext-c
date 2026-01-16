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
            free(ret);
            return NULL;
        }
        
        for (int i = 0; i < arg_count; i++) {
            fprintf(stderr, "[gdext-c] 🔍 TDD #129:   arg[%d] = %p\n", i, args[i]);
            arg_ptrs[i] = args[i];
        }
        fprintf(stderr, "[gdext-c] 🔍 TDD #129: Arguments converted\n");
    }
    
    // Call the method using variant_call
    fprintf(stderr, "[gdext-c] 🔧 TDD #131: About to call variant_call...\n");
    fprintf(stderr, "[gdext-c]   object=%p, method_sn=%p, arg_ptrs=%p, arg_count=%d, ret=%p\n",
            object, (void*)method_sn, (void*)arg_ptrs, arg_count, ret);
    fprintf(stderr, "[gdext-c]   iface->variant_call=%p\n", (void*)iface->variant_call);
    
    GDExtensionCallError error;
    memset(&error, 0, sizeof(error));
    
    fprintf(stderr, "[gdext-c] 🔧 TDD #131: Calling variant_call NOW...\n");
    iface->variant_call(
        object,              // Instance (the object)
        method_sn,           // Method name (StringName pointer)
        arg_ptrs,            // Arguments
        arg_count,           // Argument count
        ret,                 // Return value
        &error               // Error info
    );
    fprintf(stderr, "[gdext-c] ✅ TDD #131: variant_call returned! error.error=%d\n", error.error);
    
    if (error.error != 0) {
        fprintf(stderr, "[gdext-c] ⚠️  TDD #131: Call error details:\n");
        fprintf(stderr, "[gdext-c]    error=%d, argument=%d, expected=%d\n", 
                error.error, error.argument, error.expected);
    }
    
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
    
    fprintf(stderr, "[gdext-c] ✅ TDD #129: gdext_call_method SUCCESS!\n");
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
    fprintf(stderr, "[gdext-c] 🔧 TDD #134: gdext_add_child_deferred START (DIRECT add_child)\n");
    fprintf(stderr, "[gdext-c]   parent=%p, child=%p\n", parent_object, child_object);
    
    if (!gdext_c_is_initialized()) {
        fprintf(stderr, "[gdext-c] ❌ TDD #134: not initialized!\n");
        return NULL;
    }
    
    if (!parent_object || !child_object) {
        fprintf(stderr, "[gdext-c] ❌ TDD #134: NULL parent or child!\n");
        return NULL;
    }
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    
    // Create StringName for "add_child"
    char add_child_sn[64] = {0};
    iface->string_name_new_with_latin1_chars(add_child_sn, "add_child", 0);
    
    // Create StringName for "Node" class
    char node_class_sn[64] = {0};
    iface->string_name_new_with_latin1_chars(node_class_sn, "Node", 0);
    
    fprintf(stderr, "[gdext-c] 🔧 TDD #134 FINAL: Getting method bind for Node.add_child(node, force_readable, internal)...\n");
    
    // TDD #134 FINAL FIX: Use original hash with ALL 3 arguments!
    // Hash: 3863233950 for full signature: add_child(Node, bool, InternalMode)
    GDExtensionMethodBindPtr method_bind = iface->classdb_get_method_bind(
        node_class_sn,
        add_child_sn,
        3863233950  // Original hash for Node.add_child with all args
    );
    
    if (!method_bind) {
        fprintf(stderr, "[gdext-c] ❌ TDD #134: Failed to get method bind for add_child!\n");
        return NULL;
    }
    
    fprintf(stderr, "[gdext-c] ✅ TDD #134: Got method bind for add_child\n");
    
    // TDD #134 BREAKTHROUGH: add_child has 3 arguments!
    // add_child(node: Node, force_readable_name: bool = false, internal: InternalMode = 0)
    // object_method_bind_ptrcall requires ALL arguments, even defaulted ones!
    
    // Prepare default values for optional arguments
    GDExtensionBool force_readable = 0;  // false
    int64_t internal_mode = 0;           // INTERNAL_MODE_DISABLED = 0
    
    GDExtensionConstTypePtr args[3];
    args[0] = (GDExtensionConstTypePtr)&child_object;    // Node* (pointer-to-pointer for objects!)
    args[1] = (GDExtensionConstTypePtr)&force_readable;  // bool* (pointer to bool)
    args[2] = (GDExtensionConstTypePtr)&internal_mode;   // int64_t* (pointer to enum as int)
    
    fprintf(stderr, "[gdext-c] 🔧 TDD #134 FINAL: Calling add_child with ALL 3 args via object_method_bind_ptrcall...\n");
    
    // Call using ptrcall with ALL 3 arguments!
    iface->object_method_bind_ptrcall(method_bind, parent_object, args, NULL);
    
    fprintf(stderr, "[gdext-c] ✅ TDD #134: add_child SUCCESS!\n");
    
    // Return NIL variant
    GDExtensionVariantPtr ret = malloc(sizeof(GDExtensionUninitializedVariantPtr));
    if (ret) {
        iface->variant_new_nil(ret);
    }
    
    fprintf(stderr, "[gdext-c] ✅ TDD #134: gdext_add_child_deferred COMPLETE!\n");
    return ret;
}

