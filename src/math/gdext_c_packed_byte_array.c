#include "gdext_c_packed_byte_array.h"
#include "../core/gdext_c_core.h"  // Use relative path for src/ directory
#include <stdio.h>
#include <string.h>

// Access to global interface
extern const GDExtensionInterface* iface;

// TDD #152: PackedByteArray implementation for SVO renderer buffer_update

// Static pointers to Godot constructors/destructors (initialized once)
static GDExtensionVariantFromTypeConstructorFunc packed_byte_array_constructor = NULL;
static GDExtensionTypeFromVariantConstructorFunc variant_to_packed_byte_array = NULL;
static GDExtensionPtrBuiltInMethod size_method = NULL;
static GDExtensionPtrBuiltInMethod get_method = NULL;
static GDExtensionPtrBuiltInMethod set_method = NULL;
static GDExtensionPtrBuiltInMethod resize_method = NULL;
static GDExtensionPtrBuiltInMethod append_method = NULL;
static GDExtensionPtrBuiltInMethod ptr_method = NULL;  // get_ptr or to_byte_array
static GDExtensionPtrDestructor packed_byte_array_destructor = NULL;

static int initialized = 0;

// Initialize function pointers (call once)
static void ensure_initialized() {
    if (initialized) return;
    
    // Get PackedByteArray type index (29 in Godot 4.x)
    GDExtensionVariantType type = GDEXTENSION_VARIANT_TYPE_PACKED_BYTE_ARRAY;
    
    // Get constructor: Variant ← PackedByteArray
    packed_byte_array_constructor = iface->get_variant_from_type_constructor(type);
    if (!packed_byte_array_constructor) {
        fprintf(stderr, "[gdext-c] ERROR: Failed to get PackedByteArray→Variant constructor\n");
        return;
    }
    
    // Get conversion: PackedByteArray ← Variant
    variant_to_packed_byte_array = iface->get_variant_to_type_constructor(type);
    if (!variant_to_packed_byte_array) {
        fprintf(stderr, "[gdext-c] ERROR: Failed to get Variant→PackedByteArray constructor\n");
        return;
    }
    
    // Get destructor
    packed_byte_array_destructor = iface->get_variant_ptr_destructor(type);
    if (!packed_byte_array_destructor) {
        fprintf(stderr, "[gdext-c] ERROR: Failed to get PackedByteArray destructor\n");
        return;
    }
    
    // Get builtin methods
    char size_name[64], get_name[64], set_name[64], resize_name[64], append_name[64];
    iface->string_name_new_with_latin1_chars(size_name, "size", 0);
    iface->string_name_new_with_latin1_chars(get_name, "get", 0);
    iface->string_name_new_with_latin1_chars(set_name, "set", 0);
    iface->string_name_new_with_latin1_chars(resize_name, "resize", 0);
    iface->string_name_new_with_latin1_chars(append_name, "append", 0);
    
    size_method = iface->variant_get_ptr_builtin_method(type, size_name, 3173160232);
    get_method = iface->variant_get_ptr_builtin_method(type, get_name, 4103005248);
    set_method = iface->variant_get_ptr_builtin_method(type, set_name, 3638975848);
    resize_method = iface->variant_get_ptr_builtin_method(type, resize_name, 848867239);
    append_method = iface->variant_get_ptr_builtin_method(type, append_name, 1246311656);
    
    if (!size_method || !get_method || !set_method || !resize_method || !append_method) {
        fprintf(stderr, "[gdext-c] ERROR: Failed to get PackedByteArray methods\n");
        return;
    }
    
    initialized = 1;
}

void gdext_c_packed_byte_array_create(gdext_c_packed_byte_array_t* out) {
    ensure_initialized();
    
    // Create empty PackedByteArray using default constructor
    // This initializes the 16-byte struct to an empty array
    memset(out->opaque, 0, sizeof(out->opaque));
    
    // Call the variant constructor with no arguments to initialize properly
    // (This is equivalent to PackedByteArray() in GDScript)
    GDExtensionVariantPtr temp_variant = NULL;
    variant_to_packed_byte_array((GDExtensionTypePtr)out->opaque, temp_variant);
}

void gdext_c_packed_byte_array_from_bytes(
    gdext_c_packed_byte_array_t* out,
    const uint8_t* data,
    size_t size
) {
    ensure_initialized();
    
    // Create empty array
    gdext_c_packed_byte_array_create(out);
    
    // Resize to fit data
    int64_t new_size = (int64_t)size;
    const void* resize_args[1] = { &new_size };
    resize_method((GDExtensionTypePtr)out->opaque, resize_args, NULL, 1);
    
    // Fill with data using set() method
    for (size_t i = 0; i < size; i++) {
        int64_t index = (int64_t)i;
        int64_t value = (int64_t)data[i];
        const void* set_args[2] = { &index, &value };
        set_method((GDExtensionTypePtr)out->opaque, set_args, NULL, 2);
    }
}

size_t gdext_c_packed_byte_array_size(const gdext_c_packed_byte_array_t* arr) {
    ensure_initialized();
    
    int64_t result = 0;
    size_method((GDExtensionTypePtr)arr->opaque, NULL, &result, 0);
    return (size_t)result;
}

uint8_t gdext_c_packed_byte_array_get(const gdext_c_packed_byte_array_t* arr, size_t index) {
    ensure_initialized();
    
    int64_t idx = (int64_t)index;
    const void* args[1] = { &idx };
    int64_t result = 0;
    get_method((GDExtensionTypePtr)arr->opaque, args, &result, 1);
    return (uint8_t)result;
}

void gdext_c_packed_byte_array_set(gdext_c_packed_byte_array_t* arr, size_t index, uint8_t value) {
    ensure_initialized();
    
    int64_t idx = (int64_t)index;
    int64_t val = (int64_t)value;
    const void* args[2] = { &idx, &val };
    set_method((GDExtensionTypePtr)arr->opaque, args, NULL, 2);
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
    // For now, return NULL - we'll implement direct pointer access later if needed
    // Most use cases (like buffer_update) can pass the PackedByteArray directly
    return NULL;
}

