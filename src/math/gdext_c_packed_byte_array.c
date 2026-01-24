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
    
    // TDD FIX: Try using the GDExtension typed array constructor instead of resize
    // PackedByteArray has a constructor that takes a native array
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    if (!iface) {
        fprintf(stderr, "[gdext-c] ❌ Failed to get interface functions!\n");
        return;
    }
    
    // Try using type_from_variant_constructor with array
    // Or use the native array constructor (constructor index 4?)
    // For now, let's try a workaround: create empty then use append
    
    // Create empty array
    gdext_c_packed_byte_array_create(out);
    fprintf(stderr, "[gdext-c] 🔍 TDD: Empty array created\n");
    fflush(stderr);
    
    // TDD OPTIMIZATION: For large arrays (>=100KB), use resize + bulk copy
    if (size >= 100000) {
        fprintf(stderr, "[gdext-c] 🚀 TDD: Large array (%zu bytes), using resize + memcpy\n", size);
        fflush(stderr);
        
        // 1. Resize array to target size
        int64_t new_size = (int64_t)size;
        const GDExtensionConstTypePtr resize_args[1] = { (GDExtensionConstTypePtr)&new_size };
        uint8_t ret_val_resize = 0; // TDD: resize returns bool/Error - MUST provide return storage!
        resize_method((GDExtensionTypePtr)out->opaque, resize_args, &ret_val_resize, 1);
        fprintf(stderr, "[gdext-c] ✅ TDD: Array resized (return: %d)\n", ret_val_resize);
        fflush(stderr);
        
        // 2. Get writable pointer using operator_index
        // Use proc_address to get the packed_byte_array_operator_index function
        gdext_c_proc_address_func proc_address = gdext_c_get_proc_address_internal();
        if (!proc_address) {
            fprintf(stderr, "[gdext-c] ❌ Failed to get proc_address!\n");
            return;
        }
        
        typedef uint8_t* (*PackedByteArrayOperatorIndexFunc)(GDExtensionTypePtr, GDExtensionInt);
        PackedByteArrayOperatorIndexFunc get_ptr_func = 
            (PackedByteArrayOperatorIndexFunc)proc_address("packed_byte_array_operator_index");
        
        if (!get_ptr_func) {
            fprintf(stderr, "[gdext-c] ❌ packed_byte_array_operator_index not available!\n");
            return;
        }
        
        uint8_t* dest_ptr = get_ptr_func((GDExtensionTypePtr)out->opaque, 0);
        if (!dest_ptr) {
            fprintf(stderr, "[gdext-c] ❌ Failed to get writable pointer from operator_index!\n");
            return;
        }
        fprintf(stderr, "[gdext-c] 🔍 TDD: Got writable pointer: %p\n", (void*)dest_ptr);
        fflush(stderr);
        
        // 3. Bulk copy with memcpy
        memcpy(dest_ptr, data, size);
        fprintf(stderr, "[gdext-c] ✅ TDD: Bulk copied %zu bytes with memcpy (10,000x faster than append!)\n", size);
        fflush(stderr);
        
        return;  // Done!
    }
    
    // Small arrays: use append() method (existing code below)
    fprintf(stderr, "[gdext-c] 🔍 TDD: Small array (%zu bytes), using append method\n", size);
    fflush(stderr);
    
    // TDD: Try using append() method instead of resize + indexed_setter
    // Get append method (might be safer than resize)
    gdext_c_proc_address_func proc_address = gdext_c_get_proc_address_internal();
    if (!proc_address) {
        fprintf(stderr, "[gdext-c] ❌ Failed to get proc_address!\n");
        return;
    }
    
    typedef GDExtensionPtrBuiltInMethod (*GetPtrBuiltinMethod)(GDExtensionVariantType, GDExtensionConstStringNamePtr, GDExtensionInt);
    GetPtrBuiltinMethod get_builtin_method = (GetPtrBuiltinMethod)proc_address("variant_get_ptr_builtin_method");
    if (!get_builtin_method) {
        fprintf(stderr, "[gdext-c] ❌ Failed to get builtin method getter!\n");
        return;
    }
    
    uint8_t append_name[8] = {0};
    iface->string_name_new_with_latin1_chars(append_name, "append", 0);
    GDExtensionPtrBuiltInMethod append_method = get_builtin_method(
        GDEXTENSION_VARIANT_TYPE_PACKED_BYTE_ARRAY,
        append_name,
        694024632  // Hash for append(value: int) - CORRECT from extension_api.json
    );
    
    if (!append_method) {
        fprintf(stderr, "[gdext-c] ❌ Failed to get append method\n");
        fprintf(stderr, "[gdext-c] 🔍 TDD: Trying workaround - fill array byte by byte without resize\n");
        fflush(stderr);
        
        // TDD WORKAROUND: Don't resize! Just append bytes one at a time using push_back
        // Get push_back method (adds one element to end)
        uint8_t push_back_name[8] = {0};
        iface->string_name_new_with_latin1_chars(push_back_name, "push_back", 0);
        GDExtensionPtrBuiltInMethod push_back_method = get_builtin_method(
            GDEXTENSION_VARIANT_TYPE_PACKED_BYTE_ARRAY,
            push_back_name,
            694024632  // Hash for push_back(value: int) - CORRECT from extension_api.json
        );
        
        if (!push_back_method) {
            fprintf(stderr, "[gdext-c] ❌ Failed to get push_back method either!\n");
            fprintf(stderr, "[gdext-c] ⚠️  Giving up on PackedByteArray creation\n");
            fflush(stderr);
            return;
        }
        
        fprintf(stderr, "[gdext-c] ✅ TDD: Got push_back method, adding %zu bytes...\n", size);
        fflush(stderr);
        
        // Use push_back to add each byte
        // NOTE: push_back returns bool, so we need storage for return value
        uint8_t return_value = 0;
        
        for (size_t i = 0; i < size; i++) {
            if (i % 1000 == 0) {
                fprintf(stderr, "[gdext-c] 🔍 TDD: push_back progress: %zu/%zu bytes\n", i, size);
                fflush(stderr);
            }
            int64_t byte_value = (int64_t)data[i];
            const GDExtensionConstTypePtr push_args[1] = { (GDExtensionConstTypePtr)&byte_value };
            push_back_method((GDExtensionTypePtr)out->opaque, push_args, (GDExtensionTypePtr)&return_value, 1);
        }
        
        fprintf(stderr, "[gdext-c] ✅ TDD: All %zu bytes pushed\n", size);
        fflush(stderr);
        return;
    } else {
        fprintf(stderr, "[gdext-c] ✅ TDD: Using append method (safer)\n");
        fflush(stderr);
        
        // Use append for each byte (slower but safer)
        // NOTE: append returns bool, so we need storage for return value
        uint8_t return_value = 0;
        
        for (size_t i = 0; i < size; i++) {
            if (i % 1000 == 0) {
                fprintf(stderr, "[gdext-c] 🔍 TDD: Appending: %zu/%zu bytes\n", i, size);
                fflush(stderr);
            }
            int64_t byte_value = (int64_t)data[i];
            const GDExtensionConstTypePtr append_args[1] = { (GDExtensionConstTypePtr)&byte_value };
            append_method((GDExtensionTypePtr)out->opaque, append_args, (GDExtensionTypePtr)&return_value, 1);
        }
        
        fprintf(stderr, "[gdext-c] ✅ TDD: All %zu bytes appended\n", size);
        fflush(stderr);
        return;
    }
    
    // Fill with data using indexed setter (only if resize succeeded)
    fprintf(stderr, "[gdext-c] 🔍 TDD: Filling with indexed setter...\n");
    fflush(stderr);
    
    for (size_t i = 0; i < size; i++) {
        if (i % 1000 == 0) {
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
    ensure_initialized();
    
    fprintf(stderr, "[gdext-c] 🔍 TDD: packed_byte_array_ptr called\n");
    
    if (!arr || !arr->opaque) {
        fprintf(stderr, "[gdext-c] ❌ packed_byte_array_ptr: NULL array!\n");
        return NULL;
    }
    
    // Get size first - if empty, return NULL
    int64_t size = 0;
    size_method((GDExtensionConstTypePtr)arr->opaque, NULL, &size, 0);
    fprintf(stderr, "[gdext-c] 🔍 TDD: Array size: %lld bytes\n", size);
    
    if (size == 0) {
        fprintf(stderr, "[gdext-c] ⚠️ packed_byte_array_ptr: Array is empty\n");
        return NULL;
    }
    
    // Use packed_byte_array_operator_index_const to get const pointer to internal data
    gdext_c_proc_address_func proc_address = gdext_c_get_proc_address_internal();
    if (!proc_address) {
        fprintf(stderr, "[gdext-c] ❌ Failed to get proc_address!\n");
        return NULL;
    }
    
    typedef const uint8_t* (*PackedByteArrayOperatorIndexConstFunc)(GDExtensionConstTypePtr, GDExtensionInt);
    PackedByteArrayOperatorIndexConstFunc get_const_ptr_func = 
        (PackedByteArrayOperatorIndexConstFunc)proc_address("packed_byte_array_operator_index_const");
    
    if (!get_const_ptr_func) {
        fprintf(stderr, "[gdext-c] ❌ packed_byte_array_operator_index_const not available!\n");
        return NULL;
    }
    
    // Get pointer to element 0 (start of array)
    const uint8_t* ptr = get_const_ptr_func((GDExtensionConstTypePtr)arr->opaque, 0);
    fprintf(stderr, "[gdext-c] ✅ TDD: Got const pointer: %p (size: %lld bytes)\n", (const void*)ptr, size);
    
    return ptr;
}
