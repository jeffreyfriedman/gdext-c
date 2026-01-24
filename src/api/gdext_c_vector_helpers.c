/**
 * TDD #162: Vector conversion helpers for Go bindings
 * 
 * Root cause: Go calls gdext_variant_to_vector3 but it doesn't exist!
 * This causes "dyld: missing symbol" crashes when returning Vector3 from methods.
 */

#include "../../include/gdext_c.h"
#include "../core/gdext_c_core.h"
#include <stdio.h>
#include <string.h>

/**
 * TDD #162: Convert Variant to Vector3
 * @param variant The Variant containing a Vector3
 * @param out_x Pointer to store X coordinate
 * @param out_y Pointer to store Y coordinate
 * @param out_z Pointer to store Z coordinate
 */
void gdext_variant_to_vector3(void* variant, float* out_x, float* out_y, float* out_z) {
    fprintf(stderr, "[gdext-c] 🔧 TDD #162: gdext_variant_to_vector3 called\n");
    
    if (!variant || !out_x || !out_y || !out_z) {
        fprintf(stderr, "[gdext-c] ❌ TDD #162: NULL parameter!\n");
        if (out_x) *out_x = 0.0f;
        if (out_y) *out_y = 0.0f;
        if (out_z) *out_z = 0.0f;
        return;
    }
    
    if (!gdext_c_is_initialized()) {
        fprintf(stderr, "[gdext-c] ❌ TDD #162: gdext-c not initialized!\n");
        *out_x = *out_y = *out_z = 0.0f;
        return;
    }
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    
    // Get the variant-to-type constructor for Vector3
    // TDD #162: VECTOR3 = 9 (0-indexed, the 10th enum value)
    // 0=NIL, 1=BOOL, 2=INT, 3=FLOAT, 4=STRING, 5=VECTOR2, 6=VECTOR2I, 7=RECT2, 8=RECT2I, 9=VECTOR3
    GDExtensionTypeFromVariantConstructorFunc constructor = 
        iface->get_variant_to_type_constructor(9); // 9 = VECTOR3
    
    if (!constructor) {
        fprintf(stderr, "[gdext-c] ❌ TDD #162: No Vector3 constructor found!\n");
        *out_x = *out_y = *out_z = 0.0f;
        return;
    }
    
    // Allocate space for the Vector3 (3 floats = 12 bytes)
    float vec3_data[3];
    
    // Convert Variant to Vector3
    constructor(vec3_data, variant);
    
    // Extract coordinates
    *out_x = vec3_data[0];
    *out_y = vec3_data[1];
    *out_z = vec3_data[2];
    
    fprintf(stderr, "[gdext-c] ✅ TDD #162: Converted Vector3(%.2f, %.2f, %.2f)\n", 
            *out_x, *out_y, *out_z);
}

/**
 * TDD #162: Convert Variant to Vector2
 * @param variant The Variant containing a Vector2
 * @param out_x Pointer to store X coordinate
 * @param out_y Pointer to store Y coordinate
 */
void gdext_variant_to_vector2(void* variant, float* out_x, float* out_y) {
    if (!variant || !out_x || !out_y) {
        if (out_x) *out_x = 0.0f;
        if (out_y) *out_y = 0.0f;
        return;
    }
    
    if (!gdext_c_is_initialized()) {
        *out_x = *out_y = 0.0f;
        return;
    }
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    
    // TDD #162: VECTOR2 = 5 (0-indexed, the 6th enum value)
    // 0=NIL, 1=BOOL, 2=INT, 3=FLOAT, 4=STRING, 5=VECTOR2
    GDExtensionTypeFromVariantConstructorFunc constructor = 
        iface->get_variant_to_type_constructor(5); // 5 = VECTOR2 (correct!)
    
    if (!constructor) {
        *out_x = *out_y = 0.0f;
        return;
    }
    
    // Allocate space for the Vector2 (2 floats = 8 bytes)
    float vec2_data[2];
    
    // Convert Variant to Vector2
    constructor(vec2_data, variant);
    
    // Extract coordinates
    *out_x = vec2_data[0];
    *out_y = vec2_data[1];
}

