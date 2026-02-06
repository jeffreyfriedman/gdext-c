/**
 * @file gdext_c_builtin.h
 * @brief Godot builtin types (Vector2, Vector3, Color, etc.)
 * 
 * Generated for Godot Godot Engine v4.6.stable.official - TDD #142
 */

#ifndef GDEXT_C_BUILTIN_H
#define GDEXT_C_BUILTIN_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Vector2 */
typedef struct {
    float x;
    float y;
} gdext_c_vector2_t;

/* Vector3 */
typedef struct {
    float x;
    float y;
    float z;
} gdext_c_vector3_t;

/* Vector4 */
typedef struct {
    float x, y, z, w;
} gdext_c_vector4_t;

/* Color */
typedef struct {
    float r, g, b, a;
} gdext_c_color_t;

/* Rect2 */
typedef struct {
    gdext_c_vector2_t position;
    gdext_c_vector2_t size;
} gdext_c_rect2_t;

/* PackedByteArray - TDD #152 for SVO renderer buffer_update */
/* This is an opaque 16-byte builtin type (on 64-bit systems) */
typedef struct {
    uint8_t opaque[16];
} gdext_c_packed_byte_array_t;

/* TODO: More builtin types (Transform2D, Transform3D, Quaternion, etc.) */

#ifdef __cplusplus
}
#endif

#endif /* GDEXT_C_BUILTIN_H */
