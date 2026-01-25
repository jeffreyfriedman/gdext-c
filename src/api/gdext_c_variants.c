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
#include "../../include/gdext_c_packed_byte_array.h"
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
    
    // TDD #162: VECTOR3 = 9 (not 7!)
    // 0=NIL, 1=BOOL, 2=INT, 3=FLOAT, 4=STRING, 5=VECTOR2, 6=VECTOR2I, 7=RECT2, 8=RECT2I, 9=VECTOR3
    float vec3[3] = {x, y, z};
    GDExtensionVariantFromTypeConstructorFunc constructor = iface->get_variant_from_type_constructor(9);
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
    
    // TDD STEP 1: Check object instance ID BEFORE wrapping
    uint64_t object_id_before = 0;
    if (iface->object_get_instance_id) {
        object_id_before = iface->object_get_instance_id(object);
        fprintf(stderr, "[gdext-c] 🔍 STEP 1: Object ptr=%p, instance_id=%llu (BEFORE Variant wrap)\n", 
                object, (unsigned long long)object_id_before);
    } else {
        fprintf(stderr, "[gdext-c] ⚠️  STEP 1: object_get_instance_id not available!\n");
    }
    
    // The constructor expects a pointer to the type data
    // For Object type, the data is the object pointer itself, so we pass &object
    constructor(variant, &object);
    
    // TDD STEP 1: Verify the variant contains the correct object
    // Extract the object back and check instance ID
    GDExtensionTypeFromVariantConstructorFunc extractor = iface->get_variant_to_type_constructor(24);
    if (extractor) {
        void* extracted_obj = NULL;
        extractor(&extracted_obj, variant);
        
        uint64_t object_id_after = 0;
        if (iface->object_get_instance_id && extracted_obj != NULL) {
            object_id_after = iface->object_get_instance_id(extracted_obj);
            fprintf(stderr, "[gdext-c] 🔍 STEP 1: Extracted ptr=%p, instance_id=%llu (AFTER Variant wrap)\n", 
                    extracted_obj, (unsigned long long)object_id_after);
            
            if (object_id_before == object_id_after) {
                fprintf(stderr, "[gdext-c] ✅ STEP 1: Instance IDs MATCH! Object preserved correctly.\n");
            } else {
                fprintf(stderr, "[gdext-c] ❌ STEP 1: Instance IDs MISMATCH! %llu → %llu\n", 
                        (unsigned long long)object_id_before, (unsigned long long)object_id_after);
            }
        } else if (extracted_obj == NULL) {
            fprintf(stderr, "[gdext-c] ❌ STEP 1: Extracted object is NULL after Variant wrap!\n");
        }
        
        // Also check pointer equality
        if (extracted_obj == object) {
            fprintf(stderr, "[gdext-c] ✅ STEP 1: Pointer MATCH (%p)\n", object);
        } else {
            fprintf(stderr, "[gdext-c] ❌ STEP 1: Pointer MISMATCH (original=%p, extracted=%p)\n", 
                    object, extracted_obj);
        }
    }
    
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

/**
 * TDD SVO Test 2: Extract Object pointer from a Variant
 * 
 * This function converts a Variant containing an Object to the actual Object pointer.
 * It handles NULL variants correctly and returns NULL if the variant doesn't contain an object.
 * 
 * Process:
 * 1. Get the object instance ID from the variant
 * 2. Get the Object pointer using the instance ID
 * 
 * Returns: Object pointer (void*) or NULL if:
 *   - gdext-c not initialized
 *   - variant is NULL
 *   - variant doesn't contain an object
 *   - object instance ID is 0 (invalid/null)
 */
void* gdext_variant_to_object(void* variant) {
    if (!gdext_c_is_initialized() || !variant) {
        fprintf(stderr, "[gdext-c] ⚠️ gdext_variant_to_object: not initialized or variant is NULL!\n");
        return NULL;
    }
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    
    // Check if the variant is of type OBJECT
    GDExtensionVariantType variant_type = iface->variant_get_type(variant);
    if (variant_type != GDEXTENSION_VARIANT_TYPE_OBJECT) {
        fprintf(stderr, "[gdext-c] ⚠️ gdext_variant_to_object: Variant is not of type OBJECT (type: %d)!\n", variant_type);
        return NULL;
    }

    // TDD: Get variant_get_object_instance_id function directly from proc_address
    extern gdext_c_proc_address_func gdext_c_get_proc_address_internal(void);
    GDExtensionInterfaceVariantGetObjectInstanceId get_instance_id_fn = 
        (GDExtensionInterfaceVariantGetObjectInstanceId)gdext_c_get_proc_address_internal()("variant_get_object_instance_id");
    
    if (!get_instance_id_fn) {
        fprintf(stderr, "[gdext-c] ❌ gdext_variant_to_object: variant_get_object_instance_id function not available!\n");
        return NULL;
    }

    // Get the object instance ID from the variant
    GDObjectInstanceID instance_id = get_instance_id_fn(variant);
    if (instance_id == 0) {
        fprintf(stderr, "[gdext-c] ⚠️ gdext_variant_to_object: Failed to get object instance ID from variant!\n");
        return NULL;
    }

    // Get the Object pointer from the instance ID (this function is in our interface)
    GDExtensionObjectPtr object_ptr = iface->object_get_instance_from_id(instance_id);
    if (object_ptr == NULL) {
        fprintf(stderr, "[gdext-c] ⚠️ gdext_variant_to_object: Failed to get object pointer from instance ID %llu!\n", instance_id);
        return NULL;
    }

    return object_ptr;
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

/**
 * Create a Variant from a byte array (PackedByteArray)
 * TDD SVO: Required for RDShaderSPIRV.SetBytecodeCompute()
 * 
 * This reuses the existing gdext_c_packed_byte_array helpers from
 * src/math/gdext_c_packed_byte_array.c to avoid duplication.
 * 
 * IMPORTANT: The Variant takes ownership of the PackedByteArray data.
 * We do NOT destroy it here - the Variant will handle cleanup when freed.
 */
void gdext_variant_from_packed_byte_array(void* variant_ptr, const unsigned char* data, size_t len) {
    fprintf(stderr, "[gdext-c] 🔍 TDD PackedByteArray: variant_ptr=%p, data=%p, len=%zu\n", variant_ptr, data, len);
    
    if (!gdext_c_is_initialized() || !variant_ptr) {
        fprintf(stderr, "[gdext-c] ❌ gdext_variant_from_packed_byte_array: not initialized or variant is NULL!\n");
        return;
    }
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    
    // Create a PackedByteArray (16 bytes opaque struct per builtin types)
    // Allocate on heap so it persists after this function returns
    gdext_c_packed_byte_array_t* pba = malloc(sizeof(gdext_c_packed_byte_array_t));
    if (!pba) {
        fprintf(stderr, "[gdext-c] ❌ Malloc failed for PackedByteArray!\n");
        return;
    }
    fprintf(stderr, "[gdext-c] 🔍 TDD: Creating PackedByteArray...\n");
    
    // Initialize PackedByteArray with data using existing helper
    gdext_c_packed_byte_array_from_bytes(pba, data, len);
    fprintf(stderr, "[gdext-c] 🔍 TDD: PackedByteArray created with %zu bytes\n", len);
    
    // Get constructor to convert PackedByteArray → Variant
    GDExtensionVariantFromTypeConstructorFunc constructor = 
        iface->get_variant_from_type_constructor(GDEXTENSION_VARIANT_TYPE_PACKED_BYTE_ARRAY);
    
    if (!constructor) {
        fprintf(stderr, "[gdext-c] ❌ Failed to get PackedByteArray→Variant constructor!\n");
        // Cleanup PackedByteArray
        gdext_c_packed_byte_array_destroy(pba);
        free(pba);
        return;
    }
    fprintf(stderr, "[gdext-c] 🔍 TDD: Got constructor, converting to Variant...\n");
    
    // Convert PackedByteArray to Variant
    // NOTE: PackedByteArray uses internal reference counting
    // The Variant will increment the ref count, so we must destroy our temp copy
    constructor(variant_ptr, pba);
    fprintf(stderr, "[gdext-c] ✅ TDD: PackedByteArray converted to Variant\n");
    
    // Destroy the temporary PackedByteArray
    // The Variant has incremented the internal ref count, so the data stays alive
    gdext_c_packed_byte_array_destroy(pba);
    free(pba);
    fprintf(stderr, "[gdext-c] ✅ TDD: Temporary PackedByteArray freed (Variant holds ref)\n");
}

/**
 * Extract PackedByteArray from a Variant
 * TDD SVO: Required for RDShaderSPIRV.GetBytecodeCompute()
 */
void gdext_variant_to_packed_byte_array(void* variant, unsigned char** out_data, size_t* out_size) {
    fprintf(stderr, "[gdext-c] 🔍 TDD: gdext_variant_to_packed_byte_array called\n");
    
    if (!gdext_c_is_initialized() || !variant || !out_data || !out_size) {
        fprintf(stderr, "[gdext-c] ❌ gdext_variant_to_packed_byte_array: invalid params!\n");
        if (out_data) *out_data = NULL;
        if (out_size) *out_size = 0;
        return;
    }
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    
    // Get the to-type constructor to extract PackedByteArray from Variant
    GDExtensionTypeFromVariantConstructorFunc to_constructor = 
        iface->get_variant_to_type_constructor(GDEXTENSION_VARIANT_TYPE_PACKED_BYTE_ARRAY);
    
    if (!to_constructor) {
        fprintf(stderr, "[gdext-c] ❌ Failed to get Variant→PackedByteArray constructor!\n");
        *out_data = NULL;
        *out_size = 0;
        return;
    }
    
    // Extract PackedByteArray from Variant
    gdext_c_packed_byte_array_t pba;
    to_constructor(&pba, variant);
    fprintf(stderr, "[gdext-c] ✅ TDD: Extracted PackedByteArray from Variant\n");
    
    // Get size
    extern size_t gdext_c_packed_byte_array_size(const gdext_c_packed_byte_array_t* arr);
    *out_size = gdext_c_packed_byte_array_size(&pba);
    fprintf(stderr, "[gdext-c] 🔍 TDD: PackedByteArray size: %zu bytes\n", *out_size);
    
    // Get pointer to data
    extern const uint8_t* gdext_c_packed_byte_array_ptr(const gdext_c_packed_byte_array_t* arr);
    *out_data = (unsigned char*)gdext_c_packed_byte_array_ptr(&pba);
    fprintf(stderr, "[gdext-c] 🔍 TDD: PackedByteArray data pointer: %p\n", *out_data);
    
    // DO NOT destroy pba - the Variant still owns it!
    // The caller just gets a pointer to the data
    fprintf(stderr, "[gdext-c] ✅ TDD: gdext_variant_to_packed_byte_array complete\n");
}


/**
 * Extract RID from a Variant
 * TDD SVO: Required for shader_create_from_spirv return value
 * 
 * RID is Godot's Resource IDentifier - an opaque 64-bit handle used for
 * GPU resources, shaders, textures, etc.
 */
void gdext_variant_to_rid(void* variant, uint64_t* out_id) {
    fprintf(stderr, "[gdext-c] 🔍 TDD: gdext_variant_to_rid called\n");
    
    if (!gdext_c_is_initialized() || !variant || !out_id) {
        fprintf(stderr, "[gdext-c] ❌ gdext_variant_to_rid: invalid params!\n");
        if (out_id) *out_id = 0;
        return;
    }
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    
    // Get the to-type constructor to extract RID from Variant
    GDExtensionTypeFromVariantConstructorFunc to_constructor = 
        iface->get_variant_to_type_constructor(GDEXTENSION_VARIANT_TYPE_RID);
    
    if (!to_constructor) {
        fprintf(stderr, "[gdext-c] ❌ Failed to get Variant→RID constructor!\n");
        *out_id = 0;
        return;
    }
    
    // Extract RID from Variant
    // RID is 8 bytes (uint64)
    uint64_t rid_value = 0;
    to_constructor(&rid_value, variant);
    *out_id = rid_value;
    
    fprintf(stderr, "[gdext-c] ✅ TDD: Extracted RID from Variant: %llu\n", (unsigned long long)*out_id);
}

/**
 * Create a Variant from an RID
 * TDD SVO: Required for passing RIDs to Godot methods
 */
void gdext_variant_from_rid(void* variant_ptr, uint64_t rid_id) {
    fprintf(stderr, "[gdext-c] 🔍 TDD: gdext_variant_from_rid called with id=%llu\n", (unsigned long long)rid_id);
    
    if (!gdext_c_is_initialized() || !variant_ptr) {
        fprintf(stderr, "[gdext-c] ❌ gdext_variant_from_rid: not initialized or variant is NULL!\n");
        return;
    }
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    
    // Get constructor to convert RID → Variant
    GDExtensionVariantFromTypeConstructorFunc constructor = 
        iface->get_variant_from_type_constructor(GDEXTENSION_VARIANT_TYPE_RID);
    
    if (!constructor) {
        fprintf(stderr, "[gdext-c] ❌ Failed to get RID→Variant constructor!\n");
        return;
    }
    
    // Convert RID to Variant
    uint64_t rid_value = rid_id;
    constructor(variant_ptr, &rid_value);
    
    fprintf(stderr, "[gdext-c] ✅ TDD: RID converted to Variant\n");
}

/**
 * @brief Create a Variant from an array of Object pointers (Godot Array type)
 * 
 * TDD SVO: Required for UniformSetCreate() which takes an Array of RDUniform objects.
 * Creates a Godot Array variant and populates it with object references.
 */
void gdext_variant_from_object_array(void* variant_ptr, const size_t* object_ids, size_t count) {
    fprintf(stderr, "[gdext-c] 🔍 TDD: gdext_variant_from_object_array called with %zu objects\n", count);
    fflush(stderr);
    
    if (!gdext_c_is_initialized() || !variant_ptr) {
        fprintf(stderr, "[gdext-c] ❌ gdext_variant_from_object_array: not initialized or variant is NULL!\n");
        return;
    }
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    gdext_c_proc_address_func proc_address = gdext_c_get_proc_address_internal();
    
    // 1. Create an empty Array (type 28)
    typedef void (*ArrayConstructFunc)(GDExtensionTypePtr);
    GDExtensionTypePtr array_ptr = iface->mem_alloc(256); // Allocate space for Array struct
    if (!array_ptr) {
        fprintf(stderr, "[gdext-c] ❌ Failed to allocate Array struct!\n");
        return;
    }
    
    // Get Array constructor (default constructor, no args)
    typedef GDExtensionPtrConstructor (*GetPtrConstructorFunc)(GDExtensionVariantType, int32_t);
    GetPtrConstructorFunc get_constructor = (GetPtrConstructorFunc)proc_address("variant_get_ptr_constructor");
    if (!get_constructor) {
        fprintf(stderr, "[gdext-c] ❌ Failed to get variant_get_ptr_constructor!\n");
        iface->mem_free(array_ptr);
        return;
    }
    
    // Constructor 0 is the default constructor for Array
    GDExtensionPtrConstructor array_constructor = get_constructor(GDEXTENSION_VARIANT_TYPE_ARRAY, 0);
    if (!array_constructor) {
        fprintf(stderr, "[gdext-c] ❌ Failed to get Array constructor!\n");
        iface->mem_free(array_ptr);
        return;
    }
    
    array_constructor(array_ptr, NULL); // Create empty array
    fprintf(stderr, "[gdext-c] ✅ TDD: Created empty Array\n");
    fflush(stderr);
    
    // 2. Get Array.append() method
    typedef GDExtensionPtrBuiltInMethod (*GetPtrBuiltinMethod)(GDExtensionVariantType, GDExtensionConstStringNamePtr, GDExtensionInt);
    GetPtrBuiltinMethod get_builtin_method = (GetPtrBuiltinMethod)proc_address("variant_get_ptr_builtin_method");
    if (!get_builtin_method) {
        fprintf(stderr, "[gdext-c] ❌ Failed to get variant_get_ptr_builtin_method!\n");
        iface->mem_free(array_ptr);
        return;
    }
    
    uint8_t append_name[8] = {0};
    iface->string_name_new_with_latin1_chars(append_name, "append", 0);
    
    // Array.append() hash for Godot 4.5 (checked via extension_api.json)
    GDExtensionPtrBuiltInMethod append_method = get_builtin_method(
        GDEXTENSION_VARIANT_TYPE_ARRAY,
        append_name,
        3316032543 // CORRECT hash for append(value: Variant) in Godot 4.5
    );
    
    if (!append_method) {
        fprintf(stderr, "[gdext-c] ❌ Failed to get Array.append() method!\n");
        iface->mem_free(array_ptr);
        return;
    }
    
    // 3. For each object, create a Variant and append it
    // TDD: Collect variants to free them AFTER array is converted to Variant
    GDExtensionVariantPtr* obj_variants = iface->mem_alloc(count * sizeof(GDExtensionVariantPtr));
    size_t variant_count = 0;
    
    for (size_t i = 0; i < count; i++) {
        void* object_ptr = (void*)object_ids[i];
        if (!object_ptr) {
            fprintf(stderr, "[gdext-c] ⚠️  Object %zu is NULL, skipping...\n", i);
            continue;
        }
        
        // TDD STEP 1 FIX: Check if RefCounted (instance_id==0)
        uint64_t instance_id = 0;
        if (iface->object_get_instance_id) {
            instance_id = iface->object_get_instance_id(object_ptr);
            if (instance_id == 0) {
                // This is a RefCounted object
                fprintf(stderr, "[gdext-c] ⚠️  STEP 1 FIX: Object %zu is RefCounted (id=0) - needs special handling!\n", i);
                // TODO: Implement proper reference counting
                // For now, just detect and log
            }
        }
        
        // Create Variant from object using gdext_variant_from_object
        // Note: gdext_variant_from_object returns a new Variant pointer
        GDExtensionVariantPtr obj_variant = gdext_variant_from_object(object_ptr);
        if (!obj_variant) {
            fprintf(stderr, "[gdext-c] ❌ Failed to create Variant for object %zu!\n", i);
            continue;
        }
        
        // Store for later cleanup (don't free yet - Array holds references!)
        obj_variants[variant_count++] = obj_variant;
        
        // Append to array
        const GDExtensionConstTypePtr append_args[1] = { (GDExtensionConstTypePtr)obj_variant };
        uint8_t ret_val = 0; // append returns void, but we still need storage
        append_method(array_ptr, append_args, &ret_val, 1);
        
        // TDD STEP 1: Verify object is still valid after append
        fprintf(stderr, "[gdext-c] 🔍 STEP 1: Appended object %zu (original ptr=%p, instance_id=%llu)\n", 
                i, object_ptr, (unsigned long long)instance_id);
    }
    
    fprintf(stderr, "[gdext-c] ✅ TDD: Appended %zu objects to Array\n", count);
    fflush(stderr);
    
    // 4. Convert Array to Variant
    GDExtensionVariantFromTypeConstructorFunc from_constructor = 
        iface->get_variant_from_type_constructor(GDEXTENSION_VARIANT_TYPE_ARRAY);
    
    if (!from_constructor) {
        fprintf(stderr, "[gdext-c] ❌ Failed to get Array→Variant constructor!\n");
        // Clean up on error
        for (size_t i = 0; i < variant_count; i++) {
            iface->variant_destroy(obj_variants[i]);
            iface->mem_free(obj_variants[i]);
        }
        iface->mem_free(obj_variants);
        iface->mem_free(array_ptr);
        return;
    }
    
    fprintf(stderr, "[gdext-c] 🔍 TDD FIX: Converting Array to Variant...\n");
    from_constructor(variant_ptr, array_ptr);
    fprintf(stderr, "[gdext-c] ✅ TDD FIX: Array converted to Variant: variant_ptr=%p\n", variant_ptr);
    
    // TDD STEP 1: Extract Array back from Variant and inspect first element
    fprintf(stderr, "[gdext-c] 🔍 STEP 1: Extracting Array back from Variant to verify objects...\n");
    GDExtensionTypeFromVariantConstructorFunc array_extractor = iface->get_variant_to_type_constructor(GDEXTENSION_VARIANT_TYPE_ARRAY);
    if (array_extractor) {
        // Extract the Array back
        GDExtensionTypePtr extracted_array_ptr = iface->mem_alloc(256);
        array_extractor(extracted_array_ptr, variant_ptr);
        
        // Get Array.size() to verify count
        typedef GDExtensionPtrBuiltInMethod (*GetPtrBuiltinMethod)(GDExtensionVariantType, GDExtensionConstStringNamePtr, GDExtensionInt);
        GetPtrBuiltinMethod get_size_method = (GetPtrBuiltinMethod)proc_address("variant_get_ptr_builtin_method");
        if (get_size_method) {
            uint8_t size_name[8] = {0};
            iface->string_name_new_with_latin1_chars(size_name, "size", 0);
            GDExtensionPtrBuiltInMethod size_method = get_size_method(GDEXTENSION_VARIANT_TYPE_ARRAY, size_name, 3173160232);
            if (size_method) {
                int64_t array_size = 0;
                size_method(extracted_array_ptr, NULL, &array_size, 0);
                fprintf(stderr, "[gdext-c] 🔍 STEP 1: Extracted array size = %lld (expected %zu)\n", 
                        (long long)array_size, count);
                
                // Get Array[0] to check first object
                if (array_size > 0) {
                    uint8_t operator_name[8] = {0};
                    iface->string_name_new_with_latin1_chars(operator_name, "operator[]", 0);
                    // operator[] hash for Array (int index) -> Variant
                    GDExtensionPtrOperatorEvaluator array_get = (GDExtensionPtrOperatorEvaluator)proc_address("variant_get_ptr_operator_evaluator");
                    // TODO: This is getting complex - may need simpler approach
                    fprintf(stderr, "[gdext-c] ⚠️  STEP 1: Array element inspection requires operator[] which is complex\n");
                    fprintf(stderr, "[gdext-c] 💡 STEP 1: Array has %lld elements, will verify in next iteration\n", (long long)array_size);
                }
            }
        }
        
        iface->mem_free(extracted_array_ptr);
    }
    
    // TDD: Free the individual object variants
    // The Array Variant should have made its own copies
    for (size_t i = 0; i < variant_count; i++) {
        iface->variant_destroy(obj_variants[i]);
        iface->mem_free(obj_variants[i]);
    }
    iface->mem_free(obj_variants);
    
    // Clean up array struct (the Variant now owns the data)
    typedef void (*ArrayDestructorFunc)(GDExtensionTypePtr);
    typedef GDExtensionPtrDestructor (*GetPtrDestructorFunc)(GDExtensionVariantType);
    GetPtrDestructorFunc get_destructor = (GetPtrDestructorFunc)proc_address("variant_get_ptr_destructor");
    if (get_destructor) {
        GDExtensionPtrDestructor array_destructor = get_destructor(GDEXTENSION_VARIANT_TYPE_ARRAY);
        if (array_destructor) {
            array_destructor(array_ptr);
        }
    }
    iface->mem_free(array_ptr);
    
    fprintf(stderr, "[gdext-c] ✅ TDD: gdext_variant_from_object_array complete! Variant contains Array with %zu objects\n", count);
    fflush(stderr);
}
