/**
 * gdext_c_arrays.c - TDD #127: Pure C Array Helpers
 * 
 * Replaces Rust bridge functions:
 * - gdext_array_get, gdext_array_set
 * - gdext_array_resize, gdext_array_size
 * - gdext_variant_from/to_packed_*_array
 * 
 * Uses GDExtension C API directly - NO RUST!
 */

#include "../../include/gdext_c.h"
#include "../core/gdext_c_core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// ARRAY OPERATIONS
// ============================================================================

/**
 * Get an element from a Godot Array
 * @param array Variant containing a Godot Array
 * @param index Index of the element to get
 * @return Variant pointer containing the element (caller must free)
 */
void* gdext_array_get(void* array, int index) {
    if (!gdext_c_is_initialized() || !array) {
        fprintf(stderr, "[gdext-c] ❌ gdext_array_get: not initialized or NULL array!\n");
        return NULL;
    }
    
    // Use variant_call to call Array.get(index)
    void* index_variant = gdext_variant_from_int(index);
    if (!index_variant) {
        return NULL;
    }
    
    void* result = gdext_call_method1(array, "get", index_variant);
    gdext_variant_free(index_variant);
    
    return result;
}

/**
 * Set an element in a Godot Array
 * @param array Variant containing a Godot Array
 * @param index Index of the element to set
 * @param value Variant to set at the index
 */
void gdext_array_set(void* array, int index, void* value) {
    if (!gdext_c_is_initialized() || !array || !value) {
        fprintf(stderr, "[gdext-c] ❌ gdext_array_set: not initialized or NULL params!\n");
        return;
    }
    
    // Use variant_call to call Array.set(index, value)
    void* index_variant = gdext_variant_from_int(index);
    if (!index_variant) {
        return;
    }
    
    void* result = gdext_call_method2(array, "set", index_variant, value);
    if (result) {
        gdext_variant_free(result);
    }
    gdext_variant_free(index_variant);
}

/**
 * Resize a Godot Array
 * @param array Variant containing a Godot Array
 * @param new_size New size for the array
 */
void gdext_array_resize(void* array, int new_size) {
    if (!gdext_c_is_initialized() || !array) {
        fprintf(stderr, "[gdext-c] ❌ gdext_array_resize: not initialized or NULL array!\n");
        return;
    }
    
    // Use variant_call to call Array.resize(new_size)
    void* size_variant = gdext_variant_from_int(new_size);
    if (!size_variant) {
        return;
    }
    
    void* result = gdext_call_method1(array, "resize", size_variant);
    if (result) {
        gdext_variant_free(result);
    }
    gdext_variant_free(size_variant);
}

/**
 * Get the size of a Godot Array
 * @param array Variant containing a Godot Array
 * @return Size of the array
 */
int gdext_array_size(void* array) {
    if (!gdext_c_is_initialized() || !array) {
        fprintf(stderr, "[gdext-c] ❌ gdext_array_size: not initialized or NULL array!\n");
        return 0;
    }
    
    // Use variant_call to call Array.size()
    void* result = gdext_call_method0(array, "size");
    if (!result) {
        return 0;
    }
    
    int size = (int)gdext_variant_to_int(result);
    gdext_variant_free(result);
    
    return size;
}

// ============================================================================
// ARRAY CREATION
// ============================================================================

/**
 * Create a new empty Array Variant
 */
void* gdext_variant_new_array() {
    if (!gdext_c_is_initialized()) {
        fprintf(stderr, "[gdext-c] ❌ gdext_variant_new_array: not initialized!\n");
        return NULL;
    }
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    
    GDExtensionVariantPtr variant = malloc(GDEXT_VARIANT_SIZE);
    if (!variant) {
        fprintf(stderr, "[gdext-c] ❌ gdext_variant_new_array: malloc failed!\n");
        return NULL;
    }
    
    // Type 28 = ARRAY - get constructor and create empty array
    GDExtensionVariantFromTypeConstructorFunc constructor = iface->get_variant_from_type_constructor(28);
    if (!constructor) {
        free(variant);
        return NULL;
    }
    
    // Create empty array (pass NULL for empty)
    unsigned char empty_array[256] = {0};
    constructor(variant, empty_array);
    
    return variant;
}

// ============================================================================
// PACKED ARRAYS
// ============================================================================

/**
 * Create a PackedInt32Array Variant from a C array
 */
void* gdext_variant_from_packed_int32_array(int32_t* values, int count) {
    if (!gdext_c_is_initialized()) {
        fprintf(stderr, "[gdext-c] ❌ gdext_variant_from_packed_int32_array: not initialized!\n");
        return NULL;
    }
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    
    GDExtensionVariantPtr variant = malloc(GDEXT_VARIANT_SIZE);
    if (!variant) {
        return NULL;
    }
    
    // Type 31 = PACKED_INT32_ARRAY
    GDExtensionVariantFromTypeConstructorFunc constructor = iface->get_variant_from_type_constructor(31);
    if (!constructor) {
        free(variant);
        return NULL;
    }
    
    constructor(variant, values);
    return variant;
}

/**
 * Create a PackedVector3Array Variant from a C array
 */
void* gdext_variant_from_packed_vector3_array(float* values, int count) {
    if (!gdext_c_is_initialized()) {
        fprintf(stderr, "[gdext-c] ❌ gdext_variant_from_packed_vector3_array: not initialized!\n");
        return NULL;
    }
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    
    GDExtensionVariantPtr variant = malloc(GDEXT_VARIANT_SIZE);
    if (!variant) {
        return NULL;
    }
    
    // Type 35 = PACKED_VECTOR3_ARRAY
    GDExtensionVariantFromTypeConstructorFunc constructor = iface->get_variant_from_type_constructor(35);
    if (!constructor) {
        free(variant);
        return NULL;
    }
    
    constructor(variant, values);
    return variant;
}

/**
 * Create a new empty PackedInt32Array Variant
 */
void* gdext_variant_new_packed_int32_array() {
    if (!gdext_c_is_initialized()) {
        fprintf(stderr, "[gdext-c] ❌ gdext_variant_new_packed_int32_array: not initialized!\n");
        return NULL;
    }
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    
    GDExtensionVariantPtr variant = malloc(GDEXT_VARIANT_SIZE);
    if (!variant) {
        return NULL;
    }
    
    // Type 31 = PACKED_INT32_ARRAY
    GDExtensionVariantFromTypeConstructorFunc constructor = iface->get_variant_from_type_constructor(31);
    if (!constructor) {
        free(variant);
        return NULL;
    }
    
    unsigned char empty[256] = {0};
    constructor(variant, empty);
    return variant;
}

/**
 * Create a new empty PackedVector3Array Variant
 */
void* gdext_variant_new_packed_vector3_array() {
    if (!gdext_c_is_initialized()) {
        fprintf(stderr, "[gdext-c] ❌ gdext_variant_new_packed_vector3_array: not initialized!\n");
        return NULL;
    }
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    
    GDExtensionVariantPtr variant = malloc(GDEXT_VARIANT_SIZE);
    if (!variant) {
        return NULL;
    }
    
    // Type 35 = PACKED_VECTOR3_ARRAY
    GDExtensionVariantFromTypeConstructorFunc constructor = iface->get_variant_from_type_constructor(35);
    if (!constructor) {
        free(variant);
        return NULL;
    }
    
    unsigned char empty[256] = {0};
    constructor(variant, empty);
    return variant;
}

