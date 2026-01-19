#include "gdext_c_packed_byte_array.h"
#include "../core/gdext_c_core.h"
#include "../generated/gdext_c_builtin.h"
#include <stdio.h>
#include <string.h>

// TDD #152: PackedByteArray implementation using proper GDExtension API

// Access to global interface
// Declared in gdext_c_core.h
extern const GDExtensionInterface* gdext_c_get_interface_functions(void);

// Static function pointers (initialized once)
static GDExtensionPtrConstructor packed_byte_array_constructor_empty = NULL;
static GDExtensionPtrDestructor packed_byte_array_destructor = NULL;
static GDExtensionPtrBuiltInMethod resize_method = NULL;
static GDExtensionPtrBuiltInMethod size_method = NULL;
static GDExtensionPtrIndexedSetter indexed_setter = NULL;
static GDExtensionPtrIndexedGetter indexed_getter = NULL;

// Function pointer types from GDExtension API
typedef GDExtensionPtrConstructor (*GetPtrConstructor)(GDExtensionVariantType, int32_t);
typedef GDExtensionPtrDestructor (*GetPtrDestructor)(GDExtensionVariantType);
typedef GDExtensionPtrBuiltInMethod (*GetPtrBuiltinMethod)(GDExtensionVariantType, GDExtensionConstStringNamePtr, GDExtensionInt);
typedef GDExtensionPtrIndexedSetter (*GetPtrIndexedSetter)(GDExtensionVariantType);
typedef GDExtensionPtrIndexedGetter (*GetPtrIndexedGetter)(GDExtensionVariantType);

static int initialized = 0;

// Use getter function to access proc_address
extern gdext_c_proc_address_func gdext_c_get_proc_address_internal(void);

// Initialize function pointers (call once)
static void ensure_initialized() {
    if (initialized) return;
    
    GDExtensionVariantType type = GDEXTENSION_VARIANT_TYPE_PACKED_BYTE_ARRAY;
    
    // Get proc_address function
    GDExtensionInterfaceGetProcAddress proc_address = (GDExtensionInterfaceGetProcAddress)gdext_c_get_proc_address_internal();
    if (!proc_address) {
        fprintf(stderr, "[gdext-c] ERROR: proc_address is NULL - gdext_c not initialized?\n");
        return;
    }
    
    // Get function pointers from proc_address
    GetPtrConstructor get_constructor = (GetPtrConstructor)proc_address("variant_get_ptr_constructor");
    GetPtrDestructor get_destructor = (GetPtrDestructor)proc_address("variant_get_ptr_destructor");
    GetPtrBuiltinMethod get_builtin_method = (GetPtrBuiltinMethod)proc_address("variant_get_ptr_builtin_method");
    GetPtrIndexedSetter get_indexed_setter = (GetPtrIndexedSetter)proc_address("variant_get_ptr_indexed_setter");
    GetPtrIndexedGetter get_indexed_getter = (GetPtrIndexedGetter)proc_address("variant_get_ptr_indexed_getter");
    
    if (!get_constructor || !get_destructor || !get_builtin_method || !get_indexed_setter || !get_indexed_getter) {
        fprintf(stderr, "[gdext-c] ERROR: Failed to get GDExtension API functions\n");
        return;
    }
    
    // Get PackedByteArray-specific function pointers
    packed_byte_array_constructor_empty = get_constructor(type, 0); // Empty constructor
    packed_byte_array_destructor = get_destructor(type);
    indexed_setter = get_indexed_setter(type);
    indexed_getter = get_indexed_getter(type);
    
    // Get builtin methods - need interface for StringName creation
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    if (!iface || !iface->string_name_new_with_latin1_chars) {
        fprintf(stderr, "[gdext-c] ERROR: Interface functions not available\n");
        return;
    }
    
    // Allocate proper StringName structures (opaque 8-byte type on 64-bit systems)
    uint8_t resize_name[8] = {0};  // StringName is typically 8 bytes
    uint8_t size_name[8] = {0};
    iface->string_name_new_with_latin1_chars(resize_name, "resize", 0);
    iface->string_name_new_with_latin1_chars(size_name, "size", 0);
    
    resize_method = get_builtin_method(type, resize_name, 848867239);
    size_method = get_builtin_method(type, size_name, 3173160232);
    
    if (!packed_byte_array_constructor_empty || !packed_byte_array_destructor || !indexed_setter || !indexed_getter || !resize_method || !size_method) {
        fprintf(stderr, "[gdext-c] ERROR: Failed to get PackedByteArray methods\n");
        return;
    }
    
    initialized = 1;
}

void gdext_c_packed_byte_array_create(gdext_c_packed_byte_array_t* out) {
    ensure_initialized();
    
    // Call empty constructor to initialize the 16-byte struct
    packed_byte_array_constructor_empty((GDExtensionTypePtr)out->opaque, NULL);
}

void gdext_c_packed_byte_array_from_bytes(
    gdext_c_packed_byte_array_t* out,
    const uint8_t* data,
    size_t size
) {
    ensure_initialized();
    
    fprintf(stderr, "[gdext-c] 🔍 TDD: packed_byte_array_from_bytes called with %zu bytes\n", size);
    fflush(stderr);
    
    // Create empty array
    gdext_c_packed_byte_array_create(out);
    fprintf(stderr, "[gdext-c] 🔍 TDD: Empty array created\n");
    fflush(stderr);
    
    // Resize to fit data
    int64_t new_size = (int64_t)size;
    const GDExtensionConstTypePtr resize_args[1] = { (GDExtensionConstTypePtr)&new_size };
    resize_method((GDExtensionTypePtr)out->opaque, resize_args, NULL, 1);
    fprintf(stderr, "[gdext-c] 🔍 TDD: Resized to %zu bytes\n", size);
    fflush(stderr);
    
    // Fill with data using indexed setter
    // NOTE: This is slow for large arrays! TODO: Use bulk copy if available
    fprintf(stderr, "[gdext-c] 🔍 TDD: Filling with data (loop of %zu iterations)...\n", size);
    fflush(stderr);
    
    for (size_t i = 0; i < size; i++) {
        if (i % 1000 == 0) {  // Progress logging every 1000 bytes
            fprintf(stderr, "[gdext-c] 🔍 TDD: Progress: %zu/%zu bytes\n", i, size);
            fflush(stderr);
        }
        GDExtensionInt index = (GDExtensionInt)i;
        int64_t value = (int64_t)data[i];
        indexed_setter((GDExtensionTypePtr)out->opaque, index, (GDExtensionConstTypePtr)&value);
    }
    
    fprintf(stderr, "[gdext-c] ✅ TDD: All %zu bytes filled\n", size);
    fflush(stderr);
}

size_t gdext_c_packed_byte_array_size(const gdext_c_packed_byte_array_t* arr) {
    ensure_initialized();
    
    int64_t result = 0;
    size_method((GDExtensionTypePtr)arr->opaque, NULL, &result, 0);
    return (size_t)result;
}

uint8_t gdext_c_packed_byte_array_get(const gdext_c_packed_byte_array_t* arr, size_t index) {
    ensure_initialized();
    
    GDExtensionInt idx = (GDExtensionInt)index;
    int64_t result = 0;
    indexed_getter((GDExtensionConstTypePtr)arr->opaque, idx, (GDExtensionTypePtr)&result);
    return (uint8_t)result;
}

void gdext_c_packed_byte_array_set(gdext_c_packed_byte_array_t* arr, size_t index, uint8_t value) {
    ensure_initialized();
    
    GDExtensionInt idx = (GDExtensionInt)index;
    int64_t val = (int64_t)value;
    indexed_setter((GDExtensionTypePtr)arr->opaque, idx, (GDExtensionConstTypePtr)&val);
}

void gdext_c_packed_byte_array_destroy(gdext_c_packed_byte_array_t* arr) {
    if (!initialized) return;
    
    // Call Godot's destructor to free internal resources
    if (packed_byte_array_destructor) {
        packed_byte_array_destructor((GDExtensionTypePtr)arr->opaque);
    }
    
    // Clear memory
    memset(arr->opaque, 0, sizeof(arr->opaque));
}

const uint8_t* gdext_c_packed_byte_array_ptr(const gdext_c_packed_byte_array_t* arr) {
    // For v0.1.0, return NULL - direct pointer access requires more API work
    // The indexed getter/setter is sufficient for buffer_update use case
    (void)arr; // Suppress unused warning
    return NULL;
}
