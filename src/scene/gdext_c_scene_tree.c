/**
 * @file gdext_c_scene_tree.c
 * @brief Pure C scene tree manipulation helpers - IMPLEMENTATION
 * 
 * UNIVERSAL INFRASTRUCTURE for ALL language bindings!
 */

#include "scene/gdext_c_scene_tree.h"
#include "gdext_c_core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Get a Godot singleton by name
 */
GDExtensionObjectPtr gdext_c_get_singleton(const char* singleton_name) {
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    if (!iface || !iface->global_get_singleton) {
        fprintf(stderr, "[gdext-c] ❌ Interface not initialized or global_get_singleton missing\n");
        return NULL;
    }
    
    // Create StringName for singleton
    gdext_c_string_name_t sn = gdext_c_create_string_name(singleton_name);
    
    // Get singleton
    GDExtensionObjectPtr singleton = iface->global_get_singleton((GDExtensionConstStringNamePtr)&sn);
    
    // Free StringName
    gdext_c_free_string_name(sn);
    
    if (!singleton) {
        fprintf(stderr, "[gdext-c] ⚠️  Singleton '%s' not found\n", singleton_name);
    }
    
    return singleton;
}

/**
 * @brief Get the SceneTree (via Engine.get_main_loop())
 * 
 * SceneTree is NOT a direct singleton - you get it via Engine.get_main_loop()
 */
GDExtensionObjectPtr gdext_c_get_scene_tree(void) {
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    if (!iface) {
        fprintf(stderr, "[gdext-c] ❌ Interface not initialized\n");
        return NULL;
    }
    
    // Get Engine singleton
    GDExtensionObjectPtr engine = gdext_c_get_singleton("Engine");
    if (!engine) {
        fprintf(stderr, "[gdext-c] ❌ Failed to get Engine singleton\n");
        return NULL;
    }
    
    // Get Engine class name and method name
    gdext_c_string_name_t engine_class = gdext_c_create_string_name("Engine");
    gdext_c_string_name_t get_main_loop_method = gdext_c_create_string_name("get_main_loop");
    
    // Get method bind for Engine.get_main_loop()
    GDExtensionMethodBindPtr method = iface->classdb_get_method_bind(
        (GDExtensionConstStringNamePtr)&engine_class,
        (GDExtensionConstStringNamePtr)&get_main_loop_method,
        1016888095  // Hash for get_main_loop()
    );
    
    gdext_c_free_string_name(engine_class);
    gdext_c_free_string_name(get_main_loop_method);
    
    if (!method) {
        fprintf(stderr, "[gdext-c] ❌ Failed to get Engine.get_main_loop method bind\n");
        return NULL;
    }
    
    // Call Engine.get_main_loop() to get SceneTree
    GDExtensionObjectPtr scene_tree = NULL;
    iface->object_method_bind_ptrcall(method, engine, NULL, &scene_tree);
    
    if (!scene_tree) {
        fprintf(stderr, "[gdext-c] ❌ Engine.get_main_loop() returned NULL\n");
    }
    
    return scene_tree;
}

// Note: gdext_c_get_root_node() already exists in scene_access.c
// We just use that instead of duplicating it here.

/**
 * @brief Call a method on an object with no arguments
 */
int gdext_c_call_method_simple(
    GDExtensionObjectPtr object,
    const char* method_name,
    GDExtensionVariantPtr out_result
) {
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    if (!iface || !object) {
        return 0;
    }
    
    // Create method name StringName
    gdext_c_string_name_t method_sn = gdext_c_create_string_name(method_name);
    
    // Prepare for call
    GDExtensionCallError error = {0};
    uint8_t result_storage[24] = {0};  // Variant size
    GDExtensionVariantPtr result = out_result ? out_result : (GDExtensionVariantPtr)result_storage;
    
    // Initialize result variant to NIL
    iface->variant_new_nil(result);
    
    // Call method
    iface->object_method_bind_call(
        NULL,  // method bind (NULL = use reflection)
        object,
        NULL,  // args
        0,     // arg count
        result,
        &error
    );
    
    gdext_c_free_string_name(method_sn);
    
    // Check for errors
    if (error.error != GDEXTENSION_CALL_OK) {
        fprintf(stderr, "[gdext-c] ⚠️  Method call '%s' failed with error %d\n", method_name, error.error);
        if (!out_result) {
            iface->variant_destroy(result);
        }
        return 0;
    }
    
    if (!out_result) {
        iface->variant_destroy(result);
    }
    
    return 1;
}

/**
 * @brief Call a method with deferred execution
 */
int gdext_c_call_method_deferred_with_args(
    GDExtensionObjectPtr object,
    const char* method_name,
    GDExtensionVariantPtr* args,
    int arg_count
) {
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    if (!iface || !object) {
        return 0;
    }
    
    // Get Object class
    gdext_c_string_name_t object_class = gdext_c_create_string_name("Object");
    gdext_c_string_name_t call_deferred_method = gdext_c_create_string_name("call_deferred");
    
    // Get call_deferred method bind
    GDExtensionMethodBindPtr method = iface->classdb_get_method_bind(
        (GDExtensionConstStringNamePtr)&object_class,
        (GDExtensionConstStringNamePtr)&call_deferred_method,
        1517810467  // Hash for call_deferred
    );
    
    gdext_c_free_string_name(object_class);
    gdext_c_free_string_name(call_deferred_method);
    
    if (!method) {
        fprintf(stderr, "[gdext-c] ❌ Failed to get call_deferred method bind\n");
        return 0;
    }
    
    // Create variant for method name (use StringName type constructor)
    uint8_t method_name_variant_storage[24] = {0};
    GDExtensionVariantPtr method_name_variant = (GDExtensionVariantPtr)method_name_variant_storage;
    
    gdext_c_string_name_t method_name_sn = gdext_c_create_string_name(method_name);
    
    // Get StringName to Variant constructor
    GDExtensionVariantFromTypeConstructorFunc string_name_to_variant = 
        iface->get_variant_from_type_constructor(GDEXTENSION_VARIANT_TYPE_STRING_NAME);
    
    if (!string_name_to_variant) {
        fprintf(stderr, "[gdext-c] ❌ Failed to get StringName to Variant constructor\n");
        gdext_c_free_string_name(method_name_sn);
        return 0;
    }
    
    string_name_to_variant(method_name_variant, &method_name_sn);
    gdext_c_free_string_name(method_name_sn);
    
    // Build arguments array: [method_name, ...actual_args]
    GDExtensionConstVariantPtr* all_args = malloc(sizeof(GDExtensionConstVariantPtr) * (arg_count + 1));
    all_args[0] = method_name_variant;
    for (int i = 0; i < arg_count; i++) {
        all_args[i + 1] = args[i];
    }
    
    // Call call_deferred(method_name, ...args)
    uint8_t result_storage[24] = {0};
    GDExtensionVariantPtr result = (GDExtensionVariantPtr)result_storage;
    GDExtensionCallError error = {0};
    
    iface->variant_new_nil(result);
    iface->object_method_bind_call(
        method,
        object,
        all_args,
        arg_count + 1,
        result,
        &error
    );
    
    // Cleanup
    iface->variant_destroy(method_name_variant);
    iface->variant_destroy(result);
    free(all_args);
    
    if (error.error != GDEXTENSION_CALL_OK) {
        fprintf(stderr, "[gdext-c] ⚠️  call_deferred('%s') failed with error %d\n", method_name, error.error);
        return 0;
    }
    
    return 1;
}

/**
 * @brief Add a child node to a parent (DEFERRED)
 */
int gdext_c_add_child_deferred(GDExtensionObjectPtr parent, GDExtensionObjectPtr child) {
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    if (!iface || !parent || !child) {
        fprintf(stderr, "[gdext-c] ❌ Invalid arguments to add_child_deferred\n");
        return 0;
    }
    
    printf("[gdext-c] 🔧 add_child_deferred: parent=%p, child=%p\n", (void*)parent, (void*)child);
    fflush(stdout);
    
    // Create variant for child object using Object to Variant constructor
    uint8_t child_variant_storage[24] = {0};
    GDExtensionVariantPtr child_variant = (GDExtensionVariantPtr)child_variant_storage;
    
    GDExtensionVariantFromTypeConstructorFunc object_to_variant = 
        iface->get_variant_from_type_constructor(GDEXTENSION_VARIANT_TYPE_OBJECT);
    
    if (!object_to_variant) {
        fprintf(stderr, "[gdext-c] ❌ Failed to get Object to Variant constructor\n");
        return 0;
    }
    
    object_to_variant(child_variant, &child);
    
    // Call: parent.call_deferred("add_child", child)
    GDExtensionVariantPtr args[1] = {child_variant};
    int result = gdext_c_call_method_deferred_with_args(parent, "add_child", args, 1);
    
    iface->variant_destroy(child_variant);
    
    if (result) {
        printf("[gdext-c] ✅ add_child_deferred succeeded\n");
        fflush(stdout);
    } else {
        fprintf(stderr, "[gdext-c] ❌ add_child_deferred failed\n");
        fflush(stderr);
    }
    
    return result;
}

/**
 * @brief Remove a child node from parent (DEFERRED)
 */
int gdext_c_remove_child_deferred(GDExtensionObjectPtr parent, GDExtensionObjectPtr child) {
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    if (!iface || !parent || !child) {
        return 0;
    }
    
    // Create variant for child object
    uint8_t child_variant_storage[24] = {0};
    GDExtensionVariantPtr child_variant = (GDExtensionVariantPtr)child_variant_storage;
    
    GDExtensionVariantFromTypeConstructorFunc object_to_variant = 
        iface->get_variant_from_type_constructor(GDEXTENSION_VARIANT_TYPE_OBJECT);
    
    if (!object_to_variant) {
        fprintf(stderr, "[gdext-c] ❌ Failed to get Object to Variant constructor\n");
        return 0;
    }
    
    object_to_variant(child_variant, &child);
    
    // Call: parent.call_deferred("remove_child", child)
    GDExtensionVariantPtr args[1] = {child_variant};
    int result = gdext_c_call_method_deferred_with_args(parent, "remove_child", args, 1);
    
    iface->variant_destroy(child_variant);
    
    return result;
}

