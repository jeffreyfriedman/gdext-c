/**
 * @file gdext_c_properties.c
 * @brief Property getter/setter implementation for gdext-c
 * 
 * TDD #128: Implement property access using GDExtension API
 */

#include "../../include/gdext_c.h"
#include "../core/gdext_c_core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Set a property on a Godot object
 * 
 * Uses object_set_indexed to set properties by name.
 * 
 * @param object The object to set the property on
 * @param property_name The name of the property
 * @param value The value to set (as a Variant)
 * @return true on success, false on failure
 */
bool gdext_c_object_set_property(
    gdext_c_object_t object,
    const char* property_name,
    GDExtensionConstVariantPtr value
) {
    if (!gdext_c_is_initialized()) {
        fprintf(stderr, "[gdext-c] ❌ Library not initialized!\n");
        return false;
    }
    
    if (!object || !property_name || !value) {
        fprintf(stderr, "[gdext-c] ❌ NULL parameter in gdext_c_object_set_property\n");
        return false;
    }
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    if (!iface) {
        fprintf(stderr, "[gdext-c] ❌ Failed to get interface functions!\n");
        return false;
    }
    
    printf("[gdext-c] 🔧 TDD #128: Setting property '%s' on object %p\n", property_name, object);
    fflush(stdout);
    
    /* Create StringNames as char buffers (like in scene_access.c) */
    char property_sn[64] = {0};
    char set_method_sn[64] = {0};
    char object_class_sn[64] = {0};
    
    iface->string_name_new_with_latin1_chars(property_sn, property_name, 0);
    iface->string_name_new_with_latin1_chars(set_method_sn, "set", 0);
    iface->string_name_new_with_latin1_chars(object_class_sn, "Object", 0);
    
    /* Get the 'set' method bind */
    GDExtensionMethodBindPtr set_method = iface->classdb_get_method_bind(
        object_class_sn,
        set_method_sn,
        3776071444 /* Hash for Object.set(property: StringName, value: Variant) */
    );
    
    if (!set_method) {
        fprintf(stderr, "[gdext-c] ❌ Failed to get 'set' method bind!\n");
        return false;
    }
    
    /* Prepare arguments for the 'set' method */
    /* Args: [property_name (StringName), value (Variant)] */
    GDExtensionConstTypePtr args[2];
    args[0] = (GDExtensionConstTypePtr)property_sn;
    args[1] = (GDExtensionConstTypePtr)value;
    
    /* Call the method (no return value expected) */
    iface->object_method_bind_ptrcall(set_method, object, args, NULL);
    
    printf("[gdext-c] ✅ TDD #128: Property '%s' set successfully!\n", property_name);
    fflush(stdout);
    
    return true;
}

/**
 * @brief Get a property from a Godot object
 * 
 * Uses object_get_indexed to get properties by name.
 * 
 * @param object The object to get the property from
 * @param property_name The name of the property
 * @return The property value as a Variant, or NULL on failure
 */
GDExtensionVariantPtr gdext_c_object_get_property(
    gdext_c_object_t object,
    const char* property_name
) {
    if (!gdext_c_is_initialized()) {
        fprintf(stderr, "[gdext-c] ❌ Library not initialized!\n");
        return NULL;
    }
    
    if (!object || !property_name) {
        fprintf(stderr, "[gdext-c] ❌ NULL parameter in gdext_c_object_get_property\n");
        return NULL;
    }
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    if (!iface) {
        fprintf(stderr, "[gdext-c] ❌ Failed to get interface functions!\n");
        return NULL;
    }
    
    /* Create StringNames as char buffers */
    char property_sn[64] = {0};
    char get_method_sn[64] = {0};
    char object_class_sn[64] = {0};
    
    iface->string_name_new_with_latin1_chars(property_sn, property_name, 0);
    iface->string_name_new_with_latin1_chars(get_method_sn, "get", 0);
    iface->string_name_new_with_latin1_chars(object_class_sn, "Object", 0);
    
    GDExtensionMethodBindPtr get_method = iface->classdb_get_method_bind(
        object_class_sn,
        get_method_sn,
        2760726917 /* Hash for Object.get(property: StringName) -> Variant */
    );
    
    if (!get_method) {
        fprintf(stderr, "[gdext-c] ❌ Failed to get 'get' method bind!\n");
        return NULL;
    }
    
    /* Prepare argument */
    GDExtensionConstTypePtr args[1];
    args[0] = (GDExtensionConstTypePtr)property_sn;
    
    /* Prepare return value */
    GDExtensionVariantPtr result = (GDExtensionVariantPtr)malloc(sizeof(GDExtensionUninitializedVariantPtr));
    if (!result) {
        fprintf(stderr, "[gdext-c] ❌ Malloc failed for result variant!\n");
        return NULL;
    }
    
    iface->variant_new_nil(result);
    
    /* Call the method */
    iface->object_method_bind_ptrcall(get_method, object, args, result);
    
    /* Clean up */
    
    return result;
}

