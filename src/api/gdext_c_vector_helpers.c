/**
 * Vector conversion helpers for Go bindings
 */

#include "../../include/gdext_c.h"
#include "../core/gdext_c_core.h"
#include <string.h>

void gdext_variant_to_vector3(void* variant, float* out_x, float* out_y, float* out_z) {
    if (!variant || !out_x || !out_y || !out_z) {
        if (out_x) *out_x = 0.0f;
        if (out_y) *out_y = 0.0f;
        if (out_z) *out_z = 0.0f;
        return;
    }
    
    if (!gdext_c_is_initialized()) {
        *out_x = *out_y = *out_z = 0.0f;
        return;
    }
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    
    GDExtensionTypeFromVariantConstructorFunc constructor = 
        iface->get_variant_to_type_constructor(9); // 9 = VECTOR3
    
    if (!constructor) {
        *out_x = *out_y = *out_z = 0.0f;
        return;
    }
    
    float vec3_data[3];
    constructor(vec3_data, variant);
    
    *out_x = vec3_data[0];
    *out_y = vec3_data[1];
    *out_z = vec3_data[2];
}

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
    
    GDExtensionTypeFromVariantConstructorFunc constructor = 
        iface->get_variant_to_type_constructor(5); // 5 = VECTOR2
    
    if (!constructor) {
        *out_x = *out_y = 0.0f;
        return;
    }
    
    float vec2_data[2];
    constructor(vec2_data, variant);
    
    *out_x = vec2_data[0];
    *out_y = vec2_data[1];
}

// TDD: Additional geometry conversion for Go compatibility
void gdext_variant_to_rect2(void* variant, float* out_x, float* out_y, float* out_w, float* out_h) {
    if (!variant || !out_x || !out_y || !out_w || !out_h) {
        if (out_x) *out_x = 0.0f;
        if (out_y) *out_y = 0.0f;
        if (out_w) *out_w = 0.0f;
        if (out_h) *out_h = 0.0f;
        return;
    }
    
    if (!gdext_c_is_initialized()) {
        *out_x = *out_y = *out_w = *out_h = 0.0f;
        return;
    }
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    
    GDExtensionTypeFromVariantConstructorFunc constructor = 
        iface->get_variant_to_type_constructor(6); // 6 = RECT2
    
    if (!constructor) {
        *out_x = *out_y = *out_w = *out_h = 0.0f;
        return;
    }
    
    float rect_data[4];
    constructor(rect_data, variant);
    
    *out_x = rect_data[0];
    *out_y = rect_data[1];
    *out_w = rect_data[2];
    *out_h = rect_data[3];
}

void gdext_variant_to_color(void* variant, float* out_r, float* out_g, float* out_b, float* out_a) {
    if (!variant || !out_r || !out_g || !out_b || !out_a) {
        if (out_r) *out_r = 0.0f;
        if (out_g) *out_g = 0.0f;
        if (out_b) *out_b = 0.0f;
        if (out_a) *out_a = 1.0f;
        return;
    }
    
    if (!gdext_c_is_initialized()) {
        *out_r = *out_g = *out_b = 0.0f;
        *out_a = 1.0f;
        return;
    }
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    
    GDExtensionTypeFromVariantConstructorFunc constructor = 
        iface->get_variant_to_type_constructor(20); // 20 = COLOR
    
    if (!constructor) {
        *out_r = *out_g = *out_b = 0.0f;
        *out_a = 1.0f;
        return;
    }
    
    float color_data[4];
    constructor(color_data, variant);
    
    *out_r = color_data[0];
    *out_g = color_data[1];
    *out_b = color_data[2];
    *out_a = color_data[3];
}
