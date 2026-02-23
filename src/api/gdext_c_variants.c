/**
 * gdext_c_variants.c - Pure C Variant Helpers
 * 
 * Replaces Rust bridge functions:
 * - gdext_variant_from_* (create variants from C types)
 * - gdext_variant_to_* (extract C types from variants)
 * - gdext_variant_free (cleanup)
 * 
 * Uses GDExtension C API directly - NO RUST!
 */

#include "../../include/gdext_c.h"
#include "../../include/gdext_c_packed_byte_array.h"
#include "../core/gdext_c_core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// VARIANT CREATION (from C types → Variant)
// ============================================================================

void* gdext_variant_from_int(int64_t value) {
    if (!gdext_c_is_initialized()) return NULL;
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    GDExtensionVariantPtr variant = malloc(GDEXT_VARIANT_SIZE);
    if (!variant) return NULL;
    
    GDExtensionVariantFromTypeConstructorFunc constructor = iface->get_variant_from_type_constructor(2); // INT
    if (!constructor) { free(variant); return NULL; }
    
    constructor(variant, &value);
    return variant;
}

void* gdext_variant_from_float(double value) {
    if (!gdext_c_is_initialized()) return NULL;
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    GDExtensionVariantPtr variant = malloc(GDEXT_VARIANT_SIZE);
    if (!variant) return NULL;
    
    GDExtensionVariantFromTypeConstructorFunc constructor = iface->get_variant_from_type_constructor(3); // FLOAT
    if (!constructor) { free(variant); return NULL; }
    
    constructor(variant, &value);
    return variant;
}

void* gdext_variant_from_bool(int value) {
    if (!gdext_c_is_initialized()) return NULL;
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    GDExtensionVariantPtr variant = malloc(GDEXT_VARIANT_SIZE);
    if (!variant) return NULL;
    
    GDExtensionBool bool_val = value ? 1 : 0;
    GDExtensionVariantFromTypeConstructorFunc constructor = iface->get_variant_from_type_constructor(1); // BOOL
    if (!constructor) { free(variant); return NULL; }
    
    constructor(variant, &bool_val);
    return variant;
}

void* gdext_variant_from_string(const char* value) {
    if (!gdext_c_is_initialized()) return NULL;
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    GDExtensionVariantPtr variant = malloc(GDEXT_VARIANT_SIZE);
    if (!variant) return NULL;
    
    unsigned char str_buffer[256];
    GDExtensionStringPtr str = (GDExtensionStringPtr)str_buffer;
    iface->string_new_with_latin1_chars(str, value);
    
    GDExtensionVariantFromTypeConstructorFunc constructor = iface->get_variant_from_type_constructor(4); // STRING
    if (!constructor) { free(variant); return NULL; }
    
    constructor(variant, str);
    return variant;
}

void* gdext_variant_from_vector2(float x, float y) {
    if (!gdext_c_is_initialized()) return NULL;
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    GDExtensionVariantPtr variant = malloc(GDEXT_VARIANT_SIZE);
    if (!variant) return NULL;
    
    float vec2[2] = {x, y};
    GDExtensionVariantFromTypeConstructorFunc constructor = iface->get_variant_from_type_constructor(5); // VECTOR2
    if (!constructor) { free(variant); return NULL; }
    
    constructor(variant, vec2);
    return variant;
}

void* gdext_variant_from_vector3(float x, float y, float z) {
    if (!gdext_c_is_initialized()) return NULL;
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    GDExtensionVariantPtr variant = malloc(GDEXT_VARIANT_SIZE);
    if (!variant) return NULL;
    
    float vec3[3] = {x, y, z};
    GDExtensionVariantFromTypeConstructorFunc constructor = iface->get_variant_from_type_constructor(9); // VECTOR3
    if (!constructor) { free(variant); return NULL; }
    
    constructor(variant, vec3);
    return variant;
}

void* gdext_variant_from_color(float r, float g, float b, float a) {
    if (!gdext_c_is_initialized()) return NULL;
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    GDExtensionVariantPtr variant = malloc(GDEXT_VARIANT_SIZE);
    if (!variant) return NULL;
    
    float color[4] = {r, g, b, a};
    GDExtensionVariantFromTypeConstructorFunc constructor = iface->get_variant_from_type_constructor(20); // COLOR
    if (!constructor) { free(variant); return NULL; }
    
    constructor(variant, color);
    return variant;
}

void* gdext_variant_from_object(void* object) {
    if (!gdext_c_is_initialized()) return NULL;
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    GDExtensionVariantPtr variant = malloc(GDEXT_VARIANT_SIZE);
    if (!variant) return NULL;
    
    GDExtensionVariantFromTypeConstructorFunc constructor = iface->get_variant_from_type_constructor(24); // OBJECT
    if (!constructor) { free(variant); return NULL; }
    
    constructor(variant, &object);
    return variant;
}

// ============================================================================
// VARIANT EXTRACTION (Variant → C types)
// ============================================================================

int64_t gdext_variant_to_int(void* variant) {
    if (!gdext_c_is_initialized() || !variant) return 0;
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    int64_t result = 0;
    GDExtensionTypeFromVariantConstructorFunc constructor = iface->get_variant_to_type_constructor(2);
    if (constructor) constructor(&result, variant);
    return result;
}

double gdext_variant_to_float(void* variant) {
    if (!gdext_c_is_initialized() || !variant) return 0.0;
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    double result = 0.0;
    GDExtensionTypeFromVariantConstructorFunc constructor = iface->get_variant_to_type_constructor(3);
    if (constructor) constructor(&result, variant);
    return result;
}

int gdext_variant_to_bool(void* variant) {
    if (!gdext_c_is_initialized() || !variant) return 0;
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    GDExtensionBool result = 0;
    GDExtensionTypeFromVariantConstructorFunc constructor = iface->get_variant_to_type_constructor(1);
    if (constructor) constructor(&result, variant);
    return result ? 1 : 0;
}

void* gdext_variant_to_object(void* variant) {
    if (!gdext_c_is_initialized() || !variant) return NULL;
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    
    GDExtensionVariantType variant_type = iface->variant_get_type(variant);
    if (variant_type != GDEXTENSION_VARIANT_TYPE_OBJECT) return NULL;

    extern gdext_c_proc_address_func gdext_c_get_proc_address_internal(void);
    GDExtensionInterfaceVariantGetObjectInstanceId get_instance_id_fn = 
        (GDExtensionInterfaceVariantGetObjectInstanceId)gdext_c_get_proc_address_internal()("variant_get_object_instance_id");
    
    if (!get_instance_id_fn) return NULL;

    GDObjectInstanceID instance_id = get_instance_id_fn(variant);
    if (instance_id == 0) return NULL;

    GDExtensionObjectPtr object_ptr = iface->object_get_instance_from_id(instance_id);
    return object_ptr;
}

// TDD: Alias for gdext-go compatibility (Go code expects this symbol name)
void* gdext_get_object_from_variant(void* variant) {
    return gdext_variant_to_object(variant);
}

// ============================================================================
// VARIANT LIFECYCLE
// ============================================================================

void gdext_variant_free(void* variant) {
    if (!variant) return;
    
    if (gdext_c_is_initialized()) {
        const GDExtensionInterface* iface = gdext_c_get_interface_functions();
        iface->variant_destroy(variant);
    }
    
    free(variant);
}

void* gdext_variant_new() {
    if (!gdext_c_is_initialized()) return NULL;
    
    GDExtensionVariantPtr variant = malloc(GDEXT_VARIANT_SIZE);
    if (!variant) return NULL;
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    iface->variant_new_nil(variant);
    return variant;
}

// ============================================================================
// PACKED BYTE ARRAY
// ============================================================================

void gdext_variant_from_packed_byte_array(void* variant_ptr, const unsigned char* data, size_t len) {
    if (!gdext_c_is_initialized() || !variant_ptr) return;
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    
    gdext_c_packed_byte_array_t* pba = malloc(sizeof(gdext_c_packed_byte_array_t));
    if (!pba) return;
    
    gdext_c_packed_byte_array_from_bytes(pba, data, len);
    
    GDExtensionVariantFromTypeConstructorFunc constructor = 
        iface->get_variant_from_type_constructor(GDEXTENSION_VARIANT_TYPE_PACKED_BYTE_ARRAY);
    
    if (!constructor) {
        gdext_c_packed_byte_array_destroy(pba);
        free(pba);
        return;
    }
    
    constructor(variant_ptr, pba);
    
    gdext_c_packed_byte_array_destroy(pba);
    free(pba);
}

void gdext_variant_to_packed_byte_array(void* variant, unsigned char** out_data, size_t* out_size) {
    if (!gdext_c_is_initialized() || !variant || !out_data || !out_size) {
        if (out_data) *out_data = NULL;
        if (out_size) *out_size = 0;
        return;
    }
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    
    GDExtensionTypeFromVariantConstructorFunc to_constructor = 
        iface->get_variant_to_type_constructor(GDEXTENSION_VARIANT_TYPE_PACKED_BYTE_ARRAY);
    
    if (!to_constructor) {
        *out_data = NULL;
        *out_size = 0;
        return;
    }
    
    gdext_c_packed_byte_array_t pba;
    to_constructor(&pba, variant);
    
    extern size_t gdext_c_packed_byte_array_size(const gdext_c_packed_byte_array_t* arr);
    *out_size = gdext_c_packed_byte_array_size(&pba);
    
    extern const uint8_t* gdext_c_packed_byte_array_ptr(const gdext_c_packed_byte_array_t* arr);
    *out_data = (unsigned char*)gdext_c_packed_byte_array_ptr(&pba);
}

// ============================================================================
// RID HELPERS
// ============================================================================

void gdext_variant_to_rid(void* variant, uint64_t* out_id) {
    if (!gdext_c_is_initialized() || !variant || !out_id) {
        if (out_id) *out_id = 0;
        return;
    }
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    
    GDExtensionTypeFromVariantConstructorFunc to_constructor = 
        iface->get_variant_to_type_constructor(GDEXTENSION_VARIANT_TYPE_RID);
    
    if (!to_constructor) {
        *out_id = 0;
        return;
    }
    
    uint64_t rid_value = 0;
    to_constructor(&rid_value, variant);
    *out_id = rid_value;
}

void gdext_variant_from_rid(void* variant_ptr, uint64_t rid_id) {
    if (!gdext_c_is_initialized() || !variant_ptr) return;
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    
    GDExtensionVariantFromTypeConstructorFunc constructor = 
        iface->get_variant_from_type_constructor(GDEXTENSION_VARIANT_TYPE_RID);
    
    if (!constructor) return;
    
    uint64_t rid_value = rid_id;
    constructor(variant_ptr, &rid_value);
}

// ============================================================================
// OBJECT ARRAY
// ============================================================================

void gdext_variant_from_object_array(void* variant_ptr, const size_t* object_ids, size_t count) {
    if (!gdext_c_is_initialized() || !variant_ptr) return;
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    gdext_c_proc_address_func proc_address = gdext_c_get_proc_address_internal();
    
    // Create an empty Array (type 28)
    GDExtensionTypePtr array_ptr = iface->mem_alloc(256);
    if (!array_ptr) return;
    
    typedef GDExtensionPtrConstructor (*GetPtrConstructorFunc)(GDExtensionVariantType, int32_t);
    GetPtrConstructorFunc get_constructor = (GetPtrConstructorFunc)proc_address("variant_get_ptr_constructor");
    if (!get_constructor) { iface->mem_free(array_ptr); return; }
    
    GDExtensionPtrConstructor array_constructor = get_constructor(GDEXTENSION_VARIANT_TYPE_ARRAY, 0);
    if (!array_constructor) { iface->mem_free(array_ptr); return; }
    
    array_constructor(array_ptr, NULL);
    
    // Get Array.append() method
    typedef GDExtensionPtrBuiltInMethod (*GetPtrBuiltinMethod)(GDExtensionVariantType, GDExtensionConstStringNamePtr, GDExtensionInt);
    GetPtrBuiltinMethod get_builtin_method = (GetPtrBuiltinMethod)proc_address("variant_get_ptr_builtin_method");
    if (!get_builtin_method) { iface->mem_free(array_ptr); return; }
    
    uint8_t append_name[8] = {0};
    iface->string_name_new_with_latin1_chars(append_name, "append", 0);
    
    GDExtensionPtrBuiltInMethod append_method = get_builtin_method(
        GDEXTENSION_VARIANT_TYPE_ARRAY, append_name, 3316032543);
    
    if (!append_method) { iface->mem_free(array_ptr); return; }
    
    // Collect variants for cleanup
    GDExtensionVariantPtr* obj_variants = iface->mem_alloc(count * sizeof(GDExtensionVariantPtr));
    size_t variant_count = 0;
    
    for (size_t i = 0; i < count; i++) {
        void* object_ptr = (void*)object_ids[i];
        if (!object_ptr) continue;
        
        // Handle RefCounted objects
        if (iface->object_get_instance_id) {
            uint64_t instance_id = iface->object_get_instance_id(object_ptr);
            if (instance_id == 0) {
                // RefCounted object - increment refcount
                uint8_t refcounted_class_sn[16] = {0};
                uint8_t reference_method_sn[16] = {0};
                iface->string_name_new_with_latin1_chars(refcounted_class_sn, "RefCounted", 0);
                iface->string_name_new_with_latin1_chars(reference_method_sn, "reference", 0);
                
                GDExtensionMethodBindPtr method_bind = iface->classdb_get_method_bind(
                    refcounted_class_sn, reference_method_sn, 2240911060);
                
                if (method_bind) {
                    uint8_t ret_val = 0;
                    iface->object_method_bind_ptrcall(method_bind, object_ptr, NULL, &ret_val);
                }
                
                GDExtensionPtrDestructor sn_destructor = iface->variant_get_ptr_destructor(21);
                if (sn_destructor) {
                    sn_destructor(refcounted_class_sn);
                    sn_destructor(reference_method_sn);
                }
            }
        }
        
        GDExtensionVariantPtr obj_variant = gdext_variant_from_object(object_ptr);
        if (!obj_variant) continue;
        
        obj_variants[variant_count++] = obj_variant;
        
        const GDExtensionConstTypePtr append_args[1] = { (GDExtensionConstTypePtr)obj_variant };
        uint8_t ret_val = 0;
        append_method(array_ptr, append_args, &ret_val, 1);
    }
    
    // Convert Array to Variant
    GDExtensionVariantFromTypeConstructorFunc from_constructor = 
        iface->get_variant_from_type_constructor(GDEXTENSION_VARIANT_TYPE_ARRAY);
    
    if (!from_constructor) {
        for (size_t i = 0; i < variant_count; i++) {
            iface->variant_destroy(obj_variants[i]);
            iface->mem_free(obj_variants[i]);
        }
        iface->mem_free(obj_variants);
        iface->mem_free(array_ptr);
        return;
    }
    
    from_constructor(variant_ptr, array_ptr);
    
    // Cleanup
    for (size_t i = 0; i < variant_count; i++) {
        iface->variant_destroy(obj_variants[i]);
        iface->mem_free(obj_variants[i]);
    }
    iface->mem_free(obj_variants);
    
    typedef GDExtensionPtrDestructor (*GetPtrDestructorFunc)(GDExtensionVariantType);
    GetPtrDestructorFunc get_destructor = (GetPtrDestructorFunc)proc_address("variant_get_ptr_destructor");
    if (get_destructor) {
        GDExtensionPtrDestructor array_destructor = get_destructor(GDEXTENSION_VARIANT_TYPE_ARRAY);
        if (array_destructor) array_destructor(array_ptr);
    }
    iface->mem_free(array_ptr);
}
