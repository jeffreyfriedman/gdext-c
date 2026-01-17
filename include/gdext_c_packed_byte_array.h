#ifndef GDEXT_C_PACKED_BYTE_ARRAY_H
#define GDEXT_C_PACKED_BYTE_ARRAY_H

#include <stddef.h>
#include <stdint.h>
#include "gdextension_interface.h"
#include "../generated/gdext_c_builtin.h"  // For gdext_c_packed_byte_array_t

#ifdef __cplusplus
extern "C" {
#endif

/**
 * PackedByteArray - Godot's efficient byte array builtin type
 * 
 * This is a proper GDExtension builtin type (16 bytes opaque struct).
 * Used for efficient storage and transfer of binary data to GPU buffers.
 * 
 * Key use case: RenderingDevice.buffer_update(buffer, offset, size, data)
 * 
 * TDD #152 - Full implementation for v0.1.0
 */

/**
 * Create an empty PackedByteArray
 * @param out Output PackedByteArray to initialize
 */
void gdext_c_packed_byte_array_create(gdext_c_packed_byte_array_t* out);

/**
 * Create a PackedByteArray from a byte slice
 * @param out Output PackedByteArray
 * @param data Source byte data
 * @param size Number of bytes
 */
void gdext_c_packed_byte_array_from_bytes(
    gdext_c_packed_byte_array_t* out,
    const uint8_t* data,
    size_t size
);

/**
 * Get the size of a PackedByteArray
 * @param arr The PackedByteArray
 * @return Number of bytes in the array
 */
size_t gdext_c_packed_byte_array_size(const gdext_c_packed_byte_array_t* arr);

/**
 * Get a byte at a specific index
 * @param arr The PackedByteArray
 * @param index Index to read
 * @return Byte value at index
 */
uint8_t gdext_c_packed_byte_array_get(const gdext_c_packed_byte_array_t* arr, size_t index);

/**
 * Set a byte at a specific index
 * @param arr The PackedByteArray
 * @param index Index to write
 * @param value Byte value to set
 */
void gdext_c_packed_byte_array_set(gdext_c_packed_byte_array_t* arr, size_t index, uint8_t value);

/**
 * Destroy a PackedByteArray (free resources)
 * @param arr The PackedByteArray to destroy
 */
void gdext_c_packed_byte_array_destroy(gdext_c_packed_byte_array_t* arr);

/**
 * Get raw pointer to PackedByteArray data
 * @param arr The PackedByteArray
 * @return Pointer to internal data (NULL for v0.1.0 - use indexed access instead)
 */
const uint8_t* gdext_c_packed_byte_array_ptr(const gdext_c_packed_byte_array_t* arr);

#ifdef __cplusplus
}
#endif

#endif /* GDEXT_C_PACKED_BYTE_ARRAY_H */
