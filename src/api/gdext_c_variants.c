/**
 * gdext_c_variants.c - TDD #127: Pure C Variant Helpers
 * 
 * Replaces Rust bridge functions:
 * - gdext_variant_from_* (create variants from C types)
 * - gdext_variant_to_* (extract C types from variants)
 * - gdext_variant_free (cleanup)
 * 
 * Uses GDExtension C API directly - NO RUST!
 */

#include "../../include/gdext_c.h"
#include "../core/gdext_c_core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// VARIANT CREATION (from C types → Variant)
// ============================================================================

/**
 * Create a Variant from a C int64_t
 * Returns a pointer to a heap-allocated GDExtensionVariantPtr
 * Caller must free with gdext_variant_free()
 */
void* gdext_variant_from_int(int64_t value) {
    if (!gdext_c_is_initialized()) {
        fprintf(stderr, "[gdext-c] ❌ gdext_variant_from_int: not initialized!\n");
        return NULL;
    }
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    
    // Allocate variant on heap
    GDExtensionVariantPtr variant = malloc(sizeof(GDExtensionUninitializedVariantPtr));
    if (!variant) {
        fprintf(stderr, "[gdext-c] ❌ gdext_variant_from_int: malloc failed!\n");
        return NULL;
    }
    
    // Type 2 = INT (from VariantType enum in Godot)
    // Get the constructor function for INT type
    GDExtensionVariantFromTypeConstructorFunc constructor = iface->get_variant_from_type_constructor(2);
    if (!constructor) {
        fprintf(stderr, "[gdext-c] ❌ gdext_variant_from_int: failed to get constructor!\n");
        free(variant);
        return NULL;
    }
    
    // Call constructor to create variant from int
    constructor(variant, &value);
    
    return variant;
}

/**
 * Create a Variant from a C double
 */
void* gdext_variant_from_float(double value) {
    if (!gdext_c_is_initialized()) {
        fprintf(stderr, "[gdext-c] ❌ gdext_variant_from_float: not initialized!\n");
        return NULL;
    }
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    
    GDExtensionVariantPtr variant = malloc(sizeof(GDExtensionUninitializedVariantPtr));
    if (!variant) {
        fprintf(stderr, "[gdext-c] ❌ gdext_variant_from_float: malloc failed!\n");
        return NULL;
    }
    
    // Type 3 = FLOAT
    GDExtensionVariantFromTypeConstructorFunc constructor = iface->get_variant_from_type_constructor(3);
    if (!constructor) {
        free(variant);
        return NULL;
    }
    
    constructor(variant, &value);
    return variant;
}

/**
 * Create a Variant from a C bool
 */
void* gdext_variant_from_bool(int value) {
    if (!gdext_c_is_initialized()) {
        fprintf(stderr, "[gdext-c] ❌ gdext_variant_from_bool: not initialized!\n");
        return NULL;
    }
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    
    GDExtensionVariantPtr variant = malloc(sizeof(GDExtensionUninitializedVariantPtr));
    if (!variant) {
        fprintf(stderr, "[gdext-c] ❌ gdext_variant_from_bool: malloc failed!\n");
        return NULL;
    }
    
    // Type 1 = BOOL
    GDExtensionBool bool_val = value ? 1 : 0;
    GDExtensionVariantFromTypeConstructorFunc constructor = iface->get_variant_from_type_constructor(1);
    if (!constructor) {
        free(variant);
        return NULL;
    }
    
    constructor(variant, &bool_val);
    return variant;
}

/**
 * Create a Variant from a C string
 */
void* gdext_variant_from_string(const char* value) {
    if (!gdext_c_is_initialized()) {
        fprintf(stderr, "[gdext-c] ❌ gdext_variant_from_string: not initialized!\n");
        return NULL;
    }
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    
    GDExtensionVariantPtr variant = malloc(sizeof(GDExtensionUninitializedVariantPtr));
    if (!variant) {
        fprintf(stderr, "[gdext-c] ❌ gdext_variant_from_string: malloc failed!\n");
        return NULL;
    }
    
    // Create a GDExtension String first
    unsigned char str_buffer[256]; // Stack-allocated string (simpler)
    GDExtensionStringPtr str = (GDExtensionStringPtr)str_buffer;
    iface->string_new_with_latin1_chars(str, value); // 2 args: dest, source
    
    // Type 4 = STRING
    GDExtensionVariantFromTypeConstructorFunc constructor = iface->get_variant_from_type_constructor(4);
    if (!constructor) {
        free(variant);
        return NULL;
    }
    
    constructor(variant, str);
    return variant;
}

/**
 * Create a Variant from a Vector2 (x, y)
 */
void* gdext_variant_from_vector2(float x, float y) {
    if (!gdext_c_is_initialized()) {
        fprintf(stderr, "[gdext-c] ❌ gdext_variant_from_vector2: not initialized!\n");
        return NULL;
    }
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    
    GDExtensionVariantPtr variant = malloc(sizeof(GDExtensionUninitializedVariantPtr));
    if (!variant) {
        return NULL;
    }
    
    // Type 5 = VECTOR2
    float vec2[2] = {x, y};
    GDExtensionVariantFromTypeConstructorFunc constructor = iface->get_variant_from_type_constructor(5);
    if (!constructor) {
        free(variant);
        return NULL;
    }
    
    constructor(variant, vec2);
    return variant;
}

/**
 * Create a Variant from a Vector3 (x, y, z)
 */
void* gdext_variant_from_vector3(float x, float y, float z) {
    if (!gdext_c_is_initialized()) {
        fprintf(stderr, "[gdext-c] ❌ gdext_variant_from_vector3: not initialized!\n");
        return NULL;
    }
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    
    GDExtensionVariantPtr variant = malloc(sizeof(GDExtensionUninitializedVariantPtr));
    if (!variant) {
        return NULL;
    }
    
    // Type 7 = VECTOR3
    float vec3[3] = {x, y, z};
    GDExtensionVariantFromTypeConstructorFunc constructor = iface->get_variant_from_type_constructor(7);
    if (!constructor) {
        free(variant);
        return NULL;
    }
    
    constructor(variant, vec3);
    return variant;
}

/**
 * Create a Variant from a Color (r, g, b, a)
 */
void* gdext_variant_from_color(float r, float g, float b, float a) {
    if (!gdext_c_is_initialized()) {
        fprintf(stderr, "[gdext-c] ❌ gdext_variant_from_color: not initialized!\n");
        return NULL;
    }
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    
    GDExtensionVariantPtr variant = malloc(sizeof(GDExtensionUninitializedVariantPtr));
    if (!variant) {
        return NULL;
    }
    
    // Type 20 = COLOR
    float color[4] = {r, g, b, a};
    GDExtensionVariantFromTypeConstructorFunc constructor = iface->get_variant_from_type_constructor(20);
    if (!constructor) {
        free(variant);
        return NULL;
    }
    
    constructor(variant, color);
    return variant;
}

/**
 * Create a Variant from a Godot object pointer
 */
void* gdext_variant_from_object(void* object) {
    if (!gdext_c_is_initialized()) {
        fprintf(stderr, "[gdext-c] ❌ gdext_variant_from_object: not initialized!\n");
        return NULL;
    }
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    
    GDExtensionVariantPtr variant = malloc(sizeof(GDExtensionUninitializedVariantPtr));
    if (!variant) {
        return NULL;
    }
    
    // Type 24 = OBJECT
    GDExtensionVariantFromTypeConstructorFunc constructor = iface->get_variant_from_type_constructor(24);
    if (!constructor) {
        free(variant);
        return NULL;
    }
    
    constructor(variant, &object);
    return variant;
}

// ============================================================================
// VARIANT EXTRACTION (Variant → C types)
// ============================================================================

/**
 * Extract int64_t from a Variant
 */
int64_t gdext_variant_to_int(void* variant) {
    if (!gdext_c_is_initialized() || !variant) {
        return 0;
    }
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    
    int64_t result = 0;
    GDExtensionTypeFromVariantConstructorFunc constructor = iface->get_variant_to_type_constructor(2);
    if (constructor) {
        constructor(&result, variant);
    }
    
    return result;
}

/**
 * Extract double from a Variant
 */
double gdext_variant_to_float(void* variant) {
    if (!gdext_c_is_initialized() || !variant) {
        return 0.0;
    }
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    
    double result = 0.0;
    GDExtensionTypeFromVariantConstructorFunc constructor = iface->get_variant_to_type_constructor(3);
    if (constructor) {
        constructor(&result, variant);
    }
    
    return result;
}

/**
 * Extract bool from a Variant
 */
int gdext_variant_to_bool(void* variant) {
    if (!gdext_c_is_initialized() || !variant) {
        return 0;
    }
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    
    GDExtensionBool result = 0;
    GDExtensionTypeFromVariantConstructorFunc constructor = iface->get_variant_to_type_constructor(1);
    if (constructor) {
        constructor(&result, variant);
    }
    
    return result ? 1 : 0;
}

// ============================================================================
// VARIANT LIFECYCLE
// ============================================================================

/**
 * Free a Variant created by gdext_variant_from_*
 */
void gdext_variant_free(void* variant) {
    if (!variant) {
        return;
    }
    
    if (gdext_c_is_initialized()) {
        const GDExtensionInterface* iface = gdext_c_get_interface_functions();
        iface->variant_destroy(variant);
    }
    
    free(variant);
}

/**
 * Create a new empty Variant (NIL)
 */
void* gdext_variant_new() {
    if (!gdext_c_is_initialized()) {
        fprintf(stderr, "[gdext-c] ❌ gdext_variant_new: not initialized!\n");
        return NULL;
    }
    
    GDExtensionVariantPtr variant = malloc(sizeof(GDExtensionUninitializedVariantPtr));
    if (!variant) {
        fprintf(stderr, "[gdext-c] ❌ gdext_variant_new: malloc failed!\n");
        return NULL;
    }
    
    // Initialize as NIL variant
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    iface->variant_new_nil(variant);
    
    return variant;
}

