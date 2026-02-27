/**
 * gdext_c_arrays.c - TDD #127: Pure C Array Helpers
 * 
 * Replaces Rust bridge functions:
 * - gdext_array_get, gdext_array_set
 * - gdext_array_resize, gdext_array_size
 * - gdext_variant_from/to_packed_*_array
 * 
 * Uses GDExtension C API directly - NO RUST!
 * 
 * CRITICAL: These functions receive GDExtensionVariantPtr (already Variants),
 * NOT GDExtensionObjectPtr. We must call variant_call DIRECTLY on them,
 * NOT through gdext_call_method which wraps its arg in gdext_variant_from_object().
 */

#include "../../include/gdext_c.h"
#include "../core/gdext_c_core.h"
#include "../core/gdext_c_signal_handler.h"
#include "../../include/gdextension_interface.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declarations for static helpers
static void* create_empty_builtin_variant(int type_id, const char* type_name);

// ============================================================================
// HELPER: Call a method directly on a Variant (Array, Dictionary, PackedXxxArray)
// Unlike gdext_call_method which wraps in gdext_variant_from_object(),
// this calls variant_call directly on the Variant itself.
// ============================================================================

static void* variant_call_method(void* variant, const char* method_name, 
                                  GDExtensionConstVariantPtr* args, int arg_count) {
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    
    // Create StringName for method
    unsigned char method_sn_buffer[256];
    GDExtensionStringNamePtr method_sn = (GDExtensionStringNamePtr)method_sn_buffer;
    iface->string_name_new_with_latin1_chars(method_sn, method_name, 0);
    
    // Allocate return variant
    GDExtensionVariantPtr ret = malloc(GDEXT_VARIANT_SIZE);
    if (!ret) {
        GDExtensionPtrDestructor sn_dtor = iface->variant_get_ptr_destructor(GDEXTENSION_VARIANT_TYPE_STRING_NAME);
        if (sn_dtor) sn_dtor(method_sn);
        return NULL;
    }
    iface->variant_new_nil(ret);
    
    GDExtensionCallError error;
    memset(&error, 0, sizeof(error));
    
    // Call method DIRECTLY on the variant (NOT wrapped in another variant!)
    iface->variant_call(
        (GDExtensionVariantPtr)variant,
        method_sn,
        args,
        arg_count,
        ret,
        &error
    );
    
    // Destroy StringName
    GDExtensionPtrDestructor sn_dtor = iface->variant_get_ptr_destructor(GDEXTENSION_VARIANT_TYPE_STRING_NAME);
    if (sn_dtor) sn_dtor(method_sn);
    
    if (error.error != 0) {
        GDExtensionVariantType vtype = iface->variant_get_type((GDExtensionConstVariantPtr)variant);
        fprintf(stderr, "[gdext-c] ⚠️  variant_call_method error: method=%s, error=%d, arg=%d, expected=%d, variant_type=%d\n", 
                method_name, error.error, error.argument, error.expected, (int)vtype);
        iface->variant_destroy(ret);
        free(ret);
        return NULL;
    }
    
    return ret;
}

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
    
    void* index_variant = gdext_variant_from_int(index);
    if (!index_variant) {
        return NULL;
    }
    
    GDExtensionConstVariantPtr args[1] = { index_variant };
    void* result = variant_call_method(array, "get", args, 1);
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
    
    void* index_variant = gdext_variant_from_int(index);
    if (!index_variant) {
        return;
    }
    
    GDExtensionConstVariantPtr args[2] = { index_variant, value };
    void* result = variant_call_method(array, "set", args, 2);
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
    
    void* size_variant = gdext_variant_from_int(new_size);
    if (!size_variant) {
        return;
    }
    
    GDExtensionConstVariantPtr args[1] = { size_variant };
    void* result = variant_call_method(array, "resize", args, 1);
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
    
    void* result = variant_call_method(array, "size", NULL, 0);
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
 * 
 * CRITICAL FIX: Uses variant_get_ptr_constructor to create a valid native Array,
 * then wraps it in a Variant. The old approach of passing a zeroed buffer to
 * get_variant_from_type_constructor was invalid because it needs a valid Array.
 */
void* gdext_variant_new_array() {
    if (!gdext_c_is_initialized()) {
        fprintf(stderr, "[gdext-c] ❌ gdext_variant_new_array: not initialized!\n");
        return NULL;
    }
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    
    // Step 1: Create a valid native Array using the default (empty) constructor
    GDExtensionPtrConstructor array_ctor = iface->variant_get_ptr_constructor(GDEXTENSION_VARIANT_TYPE_ARRAY, 0);
    if (!array_ctor) {
        fprintf(stderr, "[gdext-c] ❌ gdext_variant_new_array: failed to get Array constructor!\n");
        return NULL;
    }
    
    unsigned char native_array[256] = {0};
    array_ctor((GDExtensionUninitializedTypePtr)native_array, NULL); // No args = empty array
    
    // Step 2: Wrap the valid native Array in a Variant
    GDExtensionVariantPtr variant = malloc(GDEXT_VARIANT_SIZE);
    if (!variant) {
        fprintf(stderr, "[gdext-c] ❌ gdext_variant_new_array: malloc failed!\n");
        // Destroy the native array
        GDExtensionPtrDestructor array_dtor = iface->variant_get_ptr_destructor(GDEXTENSION_VARIANT_TYPE_ARRAY);
        if (array_dtor) array_dtor(native_array);
        return NULL;
    }
    
    GDExtensionVariantFromTypeConstructorFunc from_type = iface->get_variant_from_type_constructor(GDEXTENSION_VARIANT_TYPE_ARRAY);
    if (!from_type) {
        free(variant);
        GDExtensionPtrDestructor array_dtor = iface->variant_get_ptr_destructor(GDEXTENSION_VARIANT_TYPE_ARRAY);
        if (array_dtor) array_dtor(native_array);
        return NULL;
    }
    
    from_type(variant, native_array);
    
    // Destroy the native array (variant now owns a copy)
    GDExtensionPtrDestructor array_dtor = iface->variant_get_ptr_destructor(GDEXTENSION_VARIANT_TYPE_ARRAY);
    if (array_dtor) array_dtor(native_array);
    
    return variant;
}

// ============================================================================
// PACKED ARRAYS
// ============================================================================

// ============================================================================
// BULK PACKED ARRAY CREATION (TDD: resize + operator_index + memcpy)
//
// These functions create packed arrays from raw C data using the fast path:
//   1. Create empty native array via default constructor
//   2. Resize to N elements
//   3. Get writable pointer via operator_index(0)
//   4. memcpy the data
//   5. Wrap in Variant
//
// This replaces per-element append (3.8M CGO calls → 1 memcpy).
// ============================================================================

// Lazy-initialized function pointers shared by all bulk array creators
static int bulk_array_init = 0;
static GDExtensionInterfaceGetProcAddress bulk_proc_address = NULL;

typedef GDExtensionPtrConstructor (*BulkGetPtrConstructor)(GDExtensionVariantType, int32_t);
typedef GDExtensionPtrDestructor (*BulkGetPtrDestructor)(GDExtensionVariantType);
typedef GDExtensionPtrBuiltInMethod (*BulkGetPtrBuiltinMethod)(GDExtensionVariantType, GDExtensionConstStringNamePtr, GDExtensionInt);

static BulkGetPtrConstructor bulk_get_constructor = NULL;
static BulkGetPtrDestructor bulk_get_destructor = NULL;
static BulkGetPtrBuiltinMethod bulk_get_builtin_method = NULL;

static void ensure_bulk_init() {
    if (bulk_array_init) return;
    
    bulk_proc_address = (GDExtensionInterfaceGetProcAddress)gdext_c_get_proc_address_internal();
    if (!bulk_proc_address) {
        fprintf(stderr, "[gdext-c] ❌ bulk_array: proc_address is NULL\n");
        return;
    }
    bulk_get_constructor = (BulkGetPtrConstructor)bulk_proc_address("variant_get_ptr_constructor");
    bulk_get_destructor = (BulkGetPtrDestructor)bulk_proc_address("variant_get_ptr_destructor");
    bulk_get_builtin_method = (BulkGetPtrBuiltinMethod)bulk_proc_address("variant_get_ptr_builtin_method");
    
    if (!bulk_get_constructor || !bulk_get_destructor || !bulk_get_builtin_method) {
        fprintf(stderr, "[gdext-c] ❌ bulk_array: failed to get API functions\n");
        return;
    }
    bulk_array_init = 1;
}

// Helper: create a bulk packed array variant using resize + operator_index + memcpy.
// type_id: GDEXTENSION_VARIANT_TYPE_PACKED_*
// operator_index_name: "packed_vector3_array_operator_index" etc.
// data: raw data pointer
// element_count: number of elements (NOT bytes)
// element_size: sizeof each element in bytes (e.g., 12 for Vector3, 4 for int32, 16 for Color)
// resize_hash: method hash for "resize" (848867239 for all packed arrays)
static void* bulk_create_packed_array(
    GDExtensionVariantType type_id,
    const char* operator_index_name,
    const void* data,
    int element_count,
    size_t element_size,
    GDExtensionInt resize_hash
) {
    ensure_bulk_init();
    if (!bulk_array_init) return NULL;
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    if (!iface) return NULL;
    
    // Step 1: Create empty native packed array
    GDExtensionPtrConstructor ctor = bulk_get_constructor(type_id, 0);
    if (!ctor) {
        fprintf(stderr, "[gdext-c] ❌ bulk_create: no constructor for type %d\n", type_id);
        return NULL;
    }
    
    unsigned char native_array[256] = {0};  // Opaque native type (big enough for any packed array)
    ctor((GDExtensionUninitializedTypePtr)native_array, NULL);
    
    if (element_count > 0 && data) {
        // Step 2: Resize to element_count
        uint8_t resize_name_buf[8] = {0};
        iface->string_name_new_with_latin1_chars(resize_name_buf, "resize", 0);
        GDExtensionPtrBuiltInMethod resize_fn = bulk_get_builtin_method(type_id, resize_name_buf, resize_hash);
        
        if (!resize_fn) {
            fprintf(stderr, "[gdext-c] ❌ bulk_create: no resize method for type %d\n", type_id);
            GDExtensionPtrDestructor dtor = bulk_get_destructor(type_id);
            if (dtor) dtor(native_array);
            return NULL;
        }
        
        int64_t new_size = (int64_t)element_count;
        const GDExtensionConstTypePtr resize_args[1] = { (GDExtensionConstTypePtr)&new_size };
        int64_t resize_result = 0;
        resize_fn((GDExtensionTypePtr)native_array, resize_args, (GDExtensionTypePtr)&resize_result, 1);
        
        // Step 3: Get writable pointer via operator_index(0)
        typedef void* (*OperatorIndexFunc)(GDExtensionTypePtr, GDExtensionInt);
        OperatorIndexFunc op_index = (OperatorIndexFunc)bulk_proc_address(operator_index_name);
        
        if (!op_index) {
            fprintf(stderr, "[gdext-c] ❌ bulk_create: no %s\n", operator_index_name);
            GDExtensionPtrDestructor dtor = bulk_get_destructor(type_id);
            if (dtor) dtor(native_array);
            return NULL;
        }
        
        void* dest = op_index((GDExtensionTypePtr)native_array, 0);
        if (!dest) {
            fprintf(stderr, "[gdext-c] ❌ bulk_create: operator_index returned NULL\n");
            GDExtensionPtrDestructor dtor = bulk_get_destructor(type_id);
            if (dtor) dtor(native_array);
            return NULL;
        }
        
        // Step 4: memcpy all data at once
        memcpy(dest, data, (size_t)element_count * element_size);
    }
    
    // Step 5: Wrap native array in Variant
    GDExtensionVariantPtr variant = malloc(GDEXT_VARIANT_SIZE);
    if (!variant) {
        GDExtensionPtrDestructor dtor = bulk_get_destructor(type_id);
        if (dtor) dtor(native_array);
        return NULL;
    }
    
    GDExtensionVariantFromTypeConstructorFunc from_type = iface->get_variant_from_type_constructor(type_id);
    if (!from_type) {
        free(variant);
        GDExtensionPtrDestructor dtor = bulk_get_destructor(type_id);
        if (dtor) dtor(native_array);
        return NULL;
    }
    
    from_type(variant, native_array);
    
    // NOTE: Do NOT destroy native_array when it contains data!
    // Godot PackedArrays use copy-on-write (COW) semantics.
    // from_type() copies the 16-byte header which includes a shared pointer
    // to the internal data buffer. Destroying native_array would decrement
    // the refcount and potentially free the buffer that the variant still
    // references, causing a use-after-free / segfault.
    //
    // For empty arrays (element_count == 0), there's no shared buffer,
    // so destruction is safe. For filled arrays, we intentionally "leak"
    // the native type on the stack — its 16-byte header is stack-allocated
    // and will be reclaimed when the function returns, while the variant
    // now owns the reference to the internal buffer.
    if (element_count == 0) {
        GDExtensionPtrDestructor dtor = bulk_get_destructor(type_id);
        if (dtor) dtor(native_array);
    }
    
    return variant;
}

/**
 * Create a PackedInt32Array Variant from a C int32_t array.
 * NOTE: Bulk memcpy approach causes COW segfaults with Godot's PackedArray
 * reference counting. Using create-empty for now; Go side uses per-element append.
 */
void* gdext_variant_from_packed_int32_array(int32_t* values, int count) {
    if (!gdext_c_is_initialized()) {
        return NULL;
    }
    return create_empty_builtin_variant(GDEXTENSION_VARIANT_TYPE_PACKED_INT32_ARRAY, "PackedInt32Array");
}

/**
 * Create a PackedInt64Array Variant (TDD: Go compatibility stub)
 */
void* gdext_variant_from_packed_int64_array(int64_t* values, int count) {
    if (!gdext_c_is_initialized()) {
        return NULL;
    }
    (void)values; (void)count;
    return create_empty_builtin_variant(GDEXTENSION_VARIANT_TYPE_PACKED_INT64_ARRAY, "PackedInt64Array");
}

/**
 * Create a PackedFloat32Array Variant (TDD: Go compatibility stub)
 */
void* gdext_variant_from_packed_float32_array(float* values, int count) {
    if (!gdext_c_is_initialized()) {
        return NULL;
    }
    (void)values; (void)count;
    return create_empty_builtin_variant(GDEXTENSION_VARIANT_TYPE_PACKED_FLOAT32_ARRAY, "PackedFloat32Array");
}

/**
 * Create a PackedFloat64Array Variant (TDD: Go compatibility stub)
 */
void* gdext_variant_from_packed_float64_array(double* values, int count) {
    if (!gdext_c_is_initialized()) {
        return NULL;
    }
    (void)values; (void)count;
    return create_empty_builtin_variant(GDEXTENSION_VARIANT_TYPE_PACKED_FLOAT64_ARRAY, "PackedFloat64Array");
}

/**
 * Create a PackedVector3Array Variant from a flat float array.
 * NOTE: Returns empty array; Go side uses per-element append.
 */
void* gdext_variant_from_packed_vector3_array(float* values, int count) {
    if (!gdext_c_is_initialized()) {
        return NULL;
    }
    return create_empty_builtin_variant(GDEXTENSION_VARIANT_TYPE_PACKED_VECTOR3_ARRAY, "PackedVector3Array");
}

/**
 * Create a PackedColorArray Variant from a flat float array.
 * NOTE: Returns empty array; Go side uses per-element append.
 */
void* gdext_variant_from_packed_color_array(float* values, int count) {
    if (!gdext_c_is_initialized()) {
        return NULL;
    }
    return create_empty_builtin_variant(GDEXTENSION_VARIANT_TYPE_PACKED_COLOR_ARRAY, "PackedColorArray");
}

/**
 * Helper: Create an empty Variant of a given built-in type using proper constructors
 */
static void* create_empty_builtin_variant(int type_id, const char* type_name) {
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    
    // Step 1: Create valid native type using default constructor
    GDExtensionPtrConstructor ctor = iface->variant_get_ptr_constructor(type_id, 0);
    if (!ctor) {
        fprintf(stderr, "[gdext-c] ❌ create_empty_builtin_variant: no constructor for %s!\n", type_name);
        return NULL;
    }
    
    unsigned char native_type[256] = {0};
    ctor((GDExtensionUninitializedTypePtr)native_type, NULL);
    
    // Step 2: Wrap in Variant
    GDExtensionVariantPtr variant = malloc(GDEXT_VARIANT_SIZE);
    if (!variant) {
        GDExtensionPtrDestructor dtor = iface->variant_get_ptr_destructor(type_id);
        if (dtor) dtor(native_type);
        return NULL;
    }
    
    GDExtensionVariantFromTypeConstructorFunc from_type = iface->get_variant_from_type_constructor(type_id);
    if (!from_type) {
        free(variant);
        GDExtensionPtrDestructor dtor = iface->variant_get_ptr_destructor(type_id);
        if (dtor) dtor(native_type);
        return NULL;
    }
    
    from_type(variant, native_type);
    
    // Destroy native (variant owns a copy)
    GDExtensionPtrDestructor dtor = iface->variant_get_ptr_destructor(type_id);
    if (dtor) dtor(native_type);
    
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
    return create_empty_builtin_variant(GDEXTENSION_VARIANT_TYPE_PACKED_INT32_ARRAY, "PackedInt32Array");
}

/**
 * Create a new empty PackedVector3Array Variant
 */
void* gdext_variant_new_packed_vector3_array() {
    if (!gdext_c_is_initialized()) {
        fprintf(stderr, "[gdext-c] ❌ gdext_variant_new_packed_vector3_array: not initialized!\n");
        return NULL;
    }
    return create_empty_builtin_variant(GDEXTENSION_VARIANT_TYPE_PACKED_VECTOR3_ARRAY, "PackedVector3Array");
}

// ============================================================================
// PACKED ARRAY ELEMENT OPERATIONS (used by mesh_arrays.go)
// ============================================================================

/**
 * Append a Vector3 to a PackedVector3Array
 * @param array Variant containing a PackedVector3Array
 * @param x, y, z Vector3 components
 */
void gdext_packed_vector3_array_append(void* array, float x, float y, float z) {
    if (!gdext_c_is_initialized() || !array) {
        return;
    }
    
    // Log first call only for debugging
    static int pv3_append_count = 0;
    if (pv3_append_count == 0) {
        const GDExtensionInterface* iface = gdext_c_get_interface_functions();
        GDExtensionVariantType vtype = iface->variant_get_type((GDExtensionConstVariantPtr)array);
        fprintf(stderr, "[gdext-c] PackedVector3Array.append first call: array type=%d (expect %d)\n", (int)vtype, GDEXTENSION_VARIANT_TYPE_PACKED_VECTOR3_ARRAY);
        fflush(stderr);
    }
    pv3_append_count++;
    
    void* vec3_variant = gdext_variant_from_vector3(x, y, z);
    if (!vec3_variant) {
        return;
    }
    
    GDExtensionConstVariantPtr args[1] = { vec3_variant };
    void* result = variant_call_method(array, "append", args, 1);
    if (result) {
        gdext_variant_free(result);
    }
    gdext_variant_free(vec3_variant);
}

/**
 * Append an int32 to a PackedInt32Array
 * @param array Variant containing a PackedInt32Array
 * @param value The int32 value to append
 */
void gdext_packed_int32_array_append(void* array, int32_t value) {
    if (!gdext_c_is_initialized() || !array) {
        return;
    }
    
    // Log first call only for debugging
    static int pi32_append_count = 0;
    if (pi32_append_count == 0) {
        const GDExtensionInterface* iface = gdext_c_get_interface_functions();
        GDExtensionVariantType vtype = iface->variant_get_type((GDExtensionConstVariantPtr)array);
        fprintf(stderr, "[gdext-c] PackedInt32Array.append first call: array type=%d (expect %d)\n", (int)vtype, GDEXTENSION_VARIANT_TYPE_PACKED_INT32_ARRAY);
        fflush(stderr);
    }
    pi32_append_count++;
    
    void* int_variant = gdext_variant_from_int(value);
    if (!int_variant) {
        return;
    }
    
    GDExtensionConstVariantPtr args[1] = { int_variant };
    void* result = variant_call_method(array, "append", args, 1);
    if (result) {
        gdext_variant_free(result);
    }
    gdext_variant_free(int_variant);
}

/**
 * Get the size of a PackedVector3Array
 * @param array Variant containing a PackedVector3Array
 * @return Number of elements
 */
int64_t gdext_packed_vector3_array_size(void* array) {
    if (!gdext_c_is_initialized() || !array) {
        return 0;
    }
    
    void* result = variant_call_method(array, "size", NULL, 0);
    if (!result) {
        return 0;
    }
    
    int64_t size = gdext_variant_to_int(result);
    gdext_variant_free(result);
    return size;
}

/**
 * Get the size of a PackedInt32Array
 * @param array Variant containing a PackedInt32Array
 * @return Number of elements
 */
int64_t gdext_packed_int32_array_size(void* array) {
    if (!gdext_c_is_initialized() || !array) {
        return 0;
    }
    
    void* result = variant_call_method(array, "size", NULL, 0);
    if (!result) {
        return 0;
    }
    
    int64_t size = gdext_variant_to_int(result);
    gdext_variant_free(result);
    return size;
}

// ============================================================================
// BULK APPEND OPERATIONS - Avoid per-element malloc/StringName overhead
// ============================================================================

/**
 * Bulk append Vector3 elements to a PackedVector3Array.
 * data: flat float array [x0,y0,z0, x1,y1,z1, ...]
 * count: number of Vector3 elements (NOT floats)
 *
 * Optimization over per-element append:
 * - StringName "append" created ONCE (not per element)
 * - Vector3 variant constructed in pre-allocated buffer (no per-element malloc)
 * - Return variant on stack (no per-element malloc)
 * Saves ~2x overhead vs per-element gdext_packed_vector3_array_append.
 */
void gdext_packed_vector3_array_append_bulk(void* array, const float* data, int count) {
    if (!gdext_c_is_initialized() || !array || !data || count <= 0) {
        return;
    }

    const GDExtensionInterface* iface = gdext_c_get_interface_functions();

    // Cache StringName "append" for entire loop (created ONCE, not per element)
    unsigned char method_sn_buffer[256];
    GDExtensionStringNamePtr method_sn = (GDExtensionStringNamePtr)method_sn_buffer;
    iface->string_name_new_with_latin1_chars(method_sn, "append", 0);

    // Pre-allocate reusable variant buffer for Vector3 (1 malloc, not N)
    GDExtensionVariantPtr vec3_var = malloc(GDEXT_VARIANT_SIZE);
    if (!vec3_var) {
        GDExtensionPtrDestructor sn_dtor = iface->variant_get_ptr_destructor(GDEXTENSION_VARIANT_TYPE_STRING_NAME);
        if (sn_dtor) sn_dtor(method_sn);
        return;
    }

    // Cache the Vector3 from_type constructor (1 lookup, not N)
    GDExtensionVariantFromTypeConstructorFunc vec3_ctor = iface->get_variant_from_type_constructor(GDEXTENSION_VARIANT_TYPE_VECTOR3);
    if (!vec3_ctor) {
        free(vec3_var);
        GDExtensionPtrDestructor sn_dtor = iface->variant_get_ptr_destructor(GDEXTENSION_VARIANT_TYPE_STRING_NAME);
        if (sn_dtor) sn_dtor(method_sn);
        return;
    }

    // Stack-allocated return variant (0 mallocs for return)
    unsigned char ret_buffer[GDEXT_VARIANT_SIZE];
    GDExtensionVariantPtr ret = (GDExtensionVariantPtr)ret_buffer;
    GDExtensionCallError error;

    // Initialize vec3_var before first destroy call
    iface->variant_new_nil(vec3_var);

    for (int i = 0; i < count; i++) {
        // Construct Vector3 variant in-place (reuse buffer, no malloc)
        float vec3[3] = { data[i * 3], data[i * 3 + 1], data[i * 3 + 2] };
        iface->variant_destroy(vec3_var);
        vec3_ctor(vec3_var, vec3);

        // Call append with cached StringName (no StringName creation per element)
        GDExtensionConstVariantPtr args[1] = { vec3_var };
        iface->variant_new_nil(ret);
        memset(&error, 0, sizeof(error));
        iface->variant_call((GDExtensionVariantPtr)array, method_sn, args, 1, ret, &error);
        iface->variant_destroy(ret);
    }

    // Cleanup (1 free + 1 StringName destroy, not N of each)
    iface->variant_destroy(vec3_var);
    free(vec3_var);
    GDExtensionPtrDestructor sn_dtor = iface->variant_get_ptr_destructor(GDEXTENSION_VARIANT_TYPE_STRING_NAME);
    if (sn_dtor) sn_dtor(method_sn);
}

/**
 * Bulk append int32 elements to a PackedInt32Array.
 * data: int32_t array
 * count: number of elements
 */
void gdext_packed_int32_array_append_bulk(void* array, const int32_t* data, int count) {
    if (!gdext_c_is_initialized() || !array || !data || count <= 0) {
        return;
    }

    const GDExtensionInterface* iface = gdext_c_get_interface_functions();

    unsigned char method_sn_buffer[256];
    GDExtensionStringNamePtr method_sn = (GDExtensionStringNamePtr)method_sn_buffer;
    iface->string_name_new_with_latin1_chars(method_sn, "append", 0);

    GDExtensionVariantPtr int_var = malloc(GDEXT_VARIANT_SIZE);
    if (!int_var) {
        GDExtensionPtrDestructor sn_dtor = iface->variant_get_ptr_destructor(GDEXTENSION_VARIANT_TYPE_STRING_NAME);
        if (sn_dtor) sn_dtor(method_sn);
        return;
    }

    GDExtensionVariantFromTypeConstructorFunc int_ctor = iface->get_variant_from_type_constructor(GDEXTENSION_VARIANT_TYPE_INT);
    if (!int_ctor) {
        free(int_var);
        GDExtensionPtrDestructor sn_dtor = iface->variant_get_ptr_destructor(GDEXTENSION_VARIANT_TYPE_STRING_NAME);
        if (sn_dtor) sn_dtor(method_sn);
        return;
    }

    unsigned char ret_buffer[GDEXT_VARIANT_SIZE];
    GDExtensionVariantPtr ret = (GDExtensionVariantPtr)ret_buffer;
    GDExtensionCallError error;

    iface->variant_new_nil(int_var);

    for (int i = 0; i < count; i++) {
        int64_t val = (int64_t)data[i];
        iface->variant_destroy(int_var);
        int_ctor(int_var, &val);

        GDExtensionConstVariantPtr args[1] = { int_var };
        iface->variant_new_nil(ret);
        memset(&error, 0, sizeof(error));
        iface->variant_call((GDExtensionVariantPtr)array, method_sn, args, 1, ret, &error);
        iface->variant_destroy(ret);
    }

    iface->variant_destroy(int_var);
    free(int_var);
    GDExtensionPtrDestructor sn_dtor = iface->variant_get_ptr_destructor(GDEXTENSION_VARIANT_TYPE_STRING_NAME);
    if (sn_dtor) sn_dtor(method_sn);
}

/**
 * Bulk append Color elements to a PackedColorArray.
 * data: flat float array [r0,g0,b0,a0, r1,g1,b1,a1, ...]
 * count: number of Color elements (NOT floats)
 */
void gdext_packed_color_array_append_bulk(void* array, const float* data, int count) {
    if (!gdext_c_is_initialized() || !array || !data || count <= 0) {
        return;
    }

    const GDExtensionInterface* iface = gdext_c_get_interface_functions();

    unsigned char method_sn_buffer[256];
    GDExtensionStringNamePtr method_sn = (GDExtensionStringNamePtr)method_sn_buffer;
    iface->string_name_new_with_latin1_chars(method_sn, "append", 0);

    GDExtensionVariantPtr color_var = malloc(GDEXT_VARIANT_SIZE);
    if (!color_var) {
        GDExtensionPtrDestructor sn_dtor = iface->variant_get_ptr_destructor(GDEXTENSION_VARIANT_TYPE_STRING_NAME);
        if (sn_dtor) sn_dtor(method_sn);
        return;
    }

    GDExtensionVariantFromTypeConstructorFunc color_ctor = iface->get_variant_from_type_constructor(GDEXTENSION_VARIANT_TYPE_COLOR);
    if (!color_ctor) {
        free(color_var);
        GDExtensionPtrDestructor sn_dtor = iface->variant_get_ptr_destructor(GDEXTENSION_VARIANT_TYPE_STRING_NAME);
        if (sn_dtor) sn_dtor(method_sn);
        return;
    }

    unsigned char ret_buffer[GDEXT_VARIANT_SIZE];
    GDExtensionVariantPtr ret = (GDExtensionVariantPtr)ret_buffer;
    GDExtensionCallError error;

    iface->variant_new_nil(color_var);

    for (int i = 0; i < count; i++) {
        float rgba[4] = { data[i * 4], data[i * 4 + 1], data[i * 4 + 2], data[i * 4 + 3] };
        iface->variant_destroy(color_var);
        color_ctor(color_var, rgba);

        GDExtensionConstVariantPtr args[1] = { color_var };
        iface->variant_new_nil(ret);
        memset(&error, 0, sizeof(error));
        iface->variant_call((GDExtensionVariantPtr)array, method_sn, args, 1, ret, &error);
        iface->variant_destroy(ret);
    }

    iface->variant_destroy(color_var);
    free(color_var);
    GDExtensionPtrDestructor sn_dtor = iface->variant_get_ptr_destructor(GDEXTENSION_VARIANT_TYPE_STRING_NAME);
    if (sn_dtor) sn_dtor(method_sn);
}

// ============================================================================
// PACKED COLOR ARRAY OPERATIONS (TDD Cycle 3: Biome vertex colors)
// ============================================================================

/**
 * Create a new empty PackedColorArray Variant
 */
void* gdext_variant_new_packed_color_array(void) {
    if (!gdext_c_is_initialized()) {
        fprintf(stderr, "[gdext-c] gdext_variant_new_packed_color_array: not initialized!\n");
        return NULL;
    }
    return create_empty_builtin_variant(GDEXTENSION_VARIANT_TYPE_PACKED_COLOR_ARRAY, "PackedColorArray");
}

/**
 * Append a Color (RGBA) to a PackedColorArray
 * @param array Variant containing a PackedColorArray
 * @param r, g, b, a Color components (0.0 - 1.0)
 */
void gdext_packed_color_array_append(void* array, float r, float g, float b, float a) {
    if (!gdext_c_is_initialized() || !array) {
        return;
    }

    void* color_variant = gdext_variant_from_color(r, g, b, a);
    if (!color_variant) {
        return;
    }

    GDExtensionConstVariantPtr args[1] = { color_variant };
    void* result = variant_call_method(array, "append", args, 1);
    if (result) {
        gdext_variant_free(result);
    }
    gdext_variant_free(color_variant);
}

