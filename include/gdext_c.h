/**
 * @file gdext_c.h
 * @brief Universal C API for Godot GDExtension
 * 
 * This library provides a clean, simple C API for creating Godot GDExtensions
 * in ANY programming language. It abstracts away the complexity of the Godot
 * GDExtension C++ API and provides a flat, easy-to-use C interface.
 * 
 * @version 0.1.0
 * @date 2026-01-13
 * @license MIT
 * 
 * @example
 * // Initialize the library
 * gdext_c_initialize(proc_address);
 * 
 * // Create an object
 * void* node = gdext_c_create_object("Node3D");
 * 
 * // Set a property
 * gdext_c_vec3 pos = gdext_c_vec3_new(10.0f, 0.0f, 5.0f);
 * gdext_c_set_property_vec3(node, "position", pos);
 */

#ifndef GDEXT_C_H
#define GDEXT_C_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

// TDD #122: Include Godot GDExtension interface
#include "gdextension_interface.h"

/* ============================================================================
 * Version Information
 * ============================================================================ */

#define GDEXT_C_VERSION_MAJOR 0
#define GDEXT_C_VERSION_MINOR 1
#define GDEXT_C_VERSION_PATCH 0

/**
 * @brief Get the library version as a string
 * @return Version string (e.g., "0.1.0")
 */
const char* gdext_c_version(void);

/* ============================================================================
 * Core Types
 * ============================================================================ */

/**
 * @brief Opaque handle to a Godot object
 * 
 * All Godot objects (Node, Resource, etc.) are represented as opaque pointers.
 * Never dereference this pointer directly - use the API functions instead.
 */
typedef void* gdext_c_object_t;

/**
 * @brief Opaque handle to a StringName
 * 
 * StringNames are Godot's optimized string type for identifiers.
 */
typedef void* gdext_c_string_name_t;

/**
 * @brief Function pointer for GDExtension proc address lookup
 * 
 * This is provided by Godot during initialization.
 */
typedef void* (*gdext_c_proc_address_func)(const char*);

/* ============================================================================
 * Math Types
 * ============================================================================ */

/**
 * @brief 3D Vector
 */
typedef struct {
    float x, y, z;
} gdext_c_vec3;

/**
 * @brief 2D Vector
 */
typedef struct {
    float x, y;
} gdext_c_vec2;

/**
 * @brief Quaternion for rotations
 */
typedef struct {
    float x, y, z, w;
} gdext_c_quat;

/**
 * @brief 3D Transform (position, rotation, scale)
 */
typedef struct {
    gdext_c_vec3 origin;
    gdext_c_vec3 basis_x;
    gdext_c_vec3 basis_y;
    gdext_c_vec3 basis_z;
} gdext_c_transform3d;

/**
 * @brief Color (RGBA)
 */
typedef struct {
    float r, g, b, a;
} gdext_c_color;

/* ============================================================================
 * Initialization & Lifecycle
 * ============================================================================ */

/**
 * @brief Initialize the gdext-c library
 * 
 * This MUST be called before any other gdext-c functions.
 * Typically called from your GDExtension initialization callback.
 * 
 * @param proc_address Function pointer provided by Godot for API lookups
 * @return true on success, false on failure
 */
bool gdext_c_initialize(gdext_c_proc_address_func proc_address);

/**
 * @brief Check if the library is initialized
 * @return true if initialized, false otherwise
 */
bool gdext_c_is_initialized(void);

/**
 * @brief Cleanup the library (optional)
 * 
 * Called during GDExtension unload. Usually not necessary.
 */
void gdext_c_cleanup(void);

/* ============================================================================
 * Error Handling
 * ============================================================================ */

/**
 * @brief Error codes
 */
typedef enum {
    GDEXT_C_OK = 0,
    GDEXT_C_ERROR_NOT_INITIALIZED,
    GDEXT_C_ERROR_INVALID_ARGUMENT,
    GDEXT_C_ERROR_NULL_POINTER,
    GDEXT_C_ERROR_GODOT_API_FAILED,
    GDEXT_C_ERROR_OUT_OF_MEMORY,
    GDEXT_C_ERROR_OBJECT_NOT_FOUND,
    GDEXT_C_ERROR_METHOD_NOT_FOUND,
    GDEXT_C_ERROR_PROPERTY_NOT_FOUND
} gdext_c_error;

/**
 * @brief Get the last error code
 * @return Error code from the last operation
 */
gdext_c_error gdext_c_get_last_error(void);

/**
 * @brief Get a human-readable error message
 * @return Error message string (valid until next error)
 */
const char* gdext_c_get_error_message(void);

/**
 * @brief Clear the last error
 */
void gdext_c_clear_error(void);

/* ============================================================================
 * Object Creation & Destruction
 * ============================================================================ */

/**
 * @brief Create a new Godot object
 * 
 * @param class_name Name of the Godot class (e.g., "Node3D", "Sprite2D")
 * @return Object handle, or NULL on error
 * 
 * @example
 * gdext_c_object_t node = gdext_c_create_object("Node3D");
 */
gdext_c_object_t gdext_c_create_object(const char* class_name);

/**
 * @brief Free a Godot object
 * 
 * Note: Usually not needed - Godot manages object lifetime.
 * Only use if you explicitly need to free an object.
 * 
 * @param object Object to free
 */
void gdext_c_free_object(gdext_c_object_t object);

/* ============================================================================
 * Scene Tree Operations
 * ============================================================================ */

/**
 * @brief Get a singleton object by name
 * 
 * @param singleton_name Name of the singleton (e.g., "Engine", "Input")
 * @return Singleton object, or NULL if not found
 * 
 * @example
 * gdext_c_object_t engine = gdext_c_get_singleton("Engine");
 */
gdext_c_object_t gdext_c_get_singleton(const char* singleton_name);

/**
 * @brief Get the root node of the scene tree
 * @return Root node, or NULL on error
 */
gdext_c_object_t gdext_c_get_root_node(void);

/**
 * @brief Get a node by path
 * 
 * @param path Node path (e.g., "/root/Main/Player")
 * @return Node object, or NULL if not found
 * 
 * @example
 * gdext_c_object_t player = gdext_c_get_node("/root/Main/Player");
 */
gdext_c_object_t gdext_c_get_node(const char* path);

/**
 * @brief Add a child node
 * 
 * @param parent Parent node
 * @param child Child node to add
 * @return true on success, false on error
 */
bool gdext_c_add_child(gdext_c_object_t parent, gdext_c_object_t child);

/**
 * @brief Remove a child node
 * 
 * @param parent Parent node
 * @param child Child node to remove
 * @return true on success, false on error
 */
bool gdext_c_remove_child(gdext_c_object_t parent, gdext_c_object_t child);

/* ============================================================================
 * Property Access
 * ============================================================================ */

/**
 * @brief Get a property value (generic)
 * 
 * @param object Object to query
 * @param property Property name
 * @param out_value Pointer to store the value
 * @param value_size Size of the value buffer
 * @return true on success, false on error
 */
bool gdext_c_get_property(gdext_c_object_t object, const char* property,
                           void* out_value, size_t value_size);

/**
 * @brief Set a property value (generic)
 * 
 * @param object Object to modify
 * @param property Property name
 * @param value Pointer to the value
 * @param value_size Size of the value
 * @return true on success, false on error
 */
bool gdext_c_set_property(gdext_c_object_t object, const char* property,
                           const void* value, size_t value_size);

/* Typed property accessors for common types */

/**
 * @brief Get a boolean property
 */
bool gdext_c_get_property_bool(gdext_c_object_t object, const char* property, bool* out_value);

/**
 * @brief Set a boolean property
 */
bool gdext_c_set_property_bool(gdext_c_object_t object, const char* property, bool value);

/**
 * @brief Get a float property
 */
bool gdext_c_get_property_float(gdext_c_object_t object, const char* property, float* out_value);

/**
 * @brief Set a float property
 */
bool gdext_c_set_property_float(gdext_c_object_t object, const char* property, float value);

/**
 * @brief Get a Vector3 property
 */
bool gdext_c_get_property_vec3(gdext_c_object_t object, const char* property, gdext_c_vec3* out_value);

/**
 * @brief Set a Vector3 property
 */
bool gdext_c_set_property_vec3(gdext_c_object_t object, const char* property, gdext_c_vec3 value);

/* ============================================================================
 * Method Calling
 * ============================================================================ */

/**
 * @brief Call a method with no arguments
 * 
 * @param object Object to call method on
 * @param method Method name
 * @return Return value (if any), or NULL
 * 
 * @example
 * gdext_c_call_method_void(node, "queue_free");
 */
gdext_c_object_t gdext_c_call_method_void(gdext_c_object_t object, const char* method);

/**
 * @brief Call a method with arguments
 * 
 * @param object Object to call method on
 * @param method Method name
 * @param args Array of argument pointers
 * @param arg_count Number of arguments
 * @return Return value (if any), or NULL
 * 
 * @example
 * void* args[] = { &vec3_pos };
 * gdext_c_call_method(node, "set_position", args, 1);
 */
gdext_c_object_t gdext_c_call_method(gdext_c_object_t object, const char* method,
                                      void** args, int arg_count);

/* ============================================================================
 * Class Registration (Advanced)
 * ============================================================================ */

/**
 * @brief Callback for creating custom class instances
 */
typedef void* (*gdext_c_create_func)(void* userdata);

/**
 * @brief Callback for destroying custom class instances
 */
typedef void (*gdext_c_free_func)(void* userdata, void* instance);

/**
 * @brief Callback for notification events
 */
typedef void (*gdext_c_notification_func)(void* instance, int32_t what, bool reversed);

/**
 * @brief Register a custom class
 * 
 * @param class_name Name of your custom class
 * @param parent_class Name of the parent Godot class
 * @param create_func Callback to create instances
 * @param free_func Callback to destroy instances
 * @param notification_func Callback for notifications
 * @return true on success, false on error
 * 
 * @example
 * gdext_c_register_class("MyCustomNode", "Node",
 *                         my_create, my_free, my_notification);
 */
bool gdext_c_register_class(const char* class_name, const char* parent_class,
                             gdext_c_create_func create_func,
                             gdext_c_free_func free_func,
                             gdext_c_notification_func notification_func);

/* ============================================================================
 * StringName Helpers
 * ============================================================================ */

/**
 * @brief Create a StringName from a C string
 * 
 * @param str C string
 * @return StringName handle, or NULL on error
 */
gdext_c_string_name_t gdext_c_create_string_name(const char* str);

/**
 * @brief Free a StringName
 * 
 * @param string_name StringName to free
 */
void gdext_c_free_string_name(gdext_c_string_name_t string_name);

/* ============================================================================
 * Math Helpers
 * ============================================================================ */

/**
 * @brief Create a Vector3
 */
static inline gdext_c_vec3 gdext_c_vec3_new(float x, float y, float z) {
    gdext_c_vec3 v = {x, y, z};
    return v;
}

/**
 * @brief Create a Vector2
 */
static inline gdext_c_vec2 gdext_c_vec2_new(float x, float y) {
    gdext_c_vec2 v = {x, y};
    return v;
}

/**
 * @brief Create a Color
 */
static inline gdext_c_color gdext_c_color_new(float r, float g, float b, float a) {
    gdext_c_color c = {r, g, b, a};
    return c;
}

/* ============================================================================
 * Variant Helpers (TDD #127) - Pure C Variant Conversions
 * ============================================================================ */

/**
 * @brief Create a Variant from an int64
 * @param value Integer value
 * @return Variant pointer (caller must free with gdext_variant_free)
 */
void* gdext_variant_from_int(int64_t value);

/**
 * @brief Create a Variant from a double
 * @param value Float value
 * @return Variant pointer (caller must free with gdext_variant_free)
 */
void* gdext_variant_from_float(double value);

/**
 * @brief Create a Variant from a bool
 * @param value Boolean value (0 or 1)
 * @return Variant pointer (caller must free with gdext_variant_free)
 */
void* gdext_variant_from_bool(int value);

/**
 * @brief Create a Variant from a C string
 * @param value C string
 * @return Variant pointer (caller must free with gdext_variant_free)
 */
void* gdext_variant_from_string(const char* value);

/**
 * @brief Create a Variant from Vector2
 */
void* gdext_variant_from_vector2(float x, float y);

/**
 * @brief Create a Variant from Vector3
 */
void* gdext_variant_from_vector3(float x, float y, float z);

/**
 * @brief Create a Variant from Color
 */
void* gdext_variant_from_color(float r, float g, float b, float a);

/**
 * @brief Create a Variant from an object pointer
 */
void* gdext_variant_from_object(void* object);

/**
 * @brief Create a Variant containing a PackedByteArray
 * @param variant_ptr Pointer to variant to initialize
 * @param data Byte data to copy into PackedByteArray
 * @param len Length of data in bytes
 * 
 * TDD SVO: Required for RDShaderSPIRV.SetBytecodeCompute()
 * This function creates a Variant containing a PackedByteArray with the given data.
 */
void gdext_variant_from_packed_byte_array(void* variant_ptr, const unsigned char* data, size_t len);

/**
 * @brief Extract int64 from a Variant
 */
int64_t gdext_variant_to_int(void* variant);

/**
 * @brief Extract double from a Variant
 */
double gdext_variant_to_float(void* variant);

/**
 * @brief Extract bool from a Variant
 */
int gdext_variant_to_bool(void* variant);

/**
 * @brief Extract Object pointer from a Variant (TDD SVO Test 2)
 * @param variant Pointer to a GDExtensionVariantPtr containing an Object
 * @return Object pointer (GDExtensionObjectPtr) or NULL if variant doesn't contain an object
 * 
 * This function safely extracts an Object pointer from a Variant.
 * It handles NULL variants and non-Object variants correctly.
 */
void* gdext_variant_to_object(void* variant);

/**
 * @brief Extract PackedByteArray from a Variant
 * @param variant Pointer to a GDExtensionVariantPtr containing a PackedByteArray
 * @param out_data Pointer to receive the data pointer (caller must NOT free)
 * @param out_size Pointer to receive the size in bytes
 * 
 * TDD SVO: Required for RDShaderSPIRV.GetBytecodeCompute()
 * The returned data pointer is valid as long as the Variant exists.
 */
void gdext_variant_to_packed_byte_array(void* variant, unsigned char** out_data, size_t* out_size);


/**
 * @brief Extract RID from a Variant
 * @param variant Pointer to a GDExtensionVariantPtr containing an RID
 * @param out_id Pointer to receive the RID value (uint64)
 * 
 * TDD SVO: Required for shader_create_from_spirv return value
 * RID is Godot's Resource IDentifier - an opaque 64-bit handle.
 */
void gdext_variant_to_rid(void* variant, uint64_t* out_id);

/**
 * @brief Create a Variant from an RID
 * @param variant_ptr Pointer to a GDExtensionVariantPtr to initialize
 * @param rid_id The RID value (uint64)
 * 
 * TDD SVO: Required for passing RIDs to Godot methods
 */
void gdext_variant_from_rid(void* variant_ptr, uint64_t rid_id);
/**
/**
 * @brief Free a Variant
 */
void gdext_variant_free(void* variant);

/**
 * @brief Create a new empty Variant
 */
void* gdext_variant_new(void);

/* ============================================================================
 * Method Calling Helpers (TDD #127) - Pure C Method Invocation
 * ============================================================================ */

/**
 * @brief Call a method on a Godot object with variant arguments
 * @param object The Godot object
 * @param method_name The method name
 * @param args Array of Variant pointers (from gdext_variant_from_*)
 * @param arg_count Number of arguments
 * @return Variant pointer with return value (caller must free)
 */
void* gdext_call_method(void* object, const char* method_name, void** args, int arg_count);

/**
 * @brief Call a method with no arguments
 */
void* gdext_call_method0(void* object, const char* method_name);

/**
 * @brief Call a method with 1 argument
 */
void* gdext_call_method1(void* object, const char* method_name, void* arg1);

/**
 * @brief Call a method with 2 arguments
 */
void* gdext_call_method2(void* object, const char* method_name, void* arg1, void* arg2);

/**
 * @brief Call a method with 3 arguments
 */
void* gdext_call_method3(void* object, const char* method_name, void* arg1, void* arg2, void* arg3);

/**
 * @brief TDD #133: Call a method via call_deferred
 * 
 * CRITICAL for scene tree modifications (add_child, remove_child, queue_free, etc.)
 * GDExtension REQUIRES deferred calls for scene tree changes to avoid crashes.
 * 
 * @param object The Godot object
 * @param method_name The method to call deferred
 * @param args Array of variant pointers
 * @param arg_count Number of arguments
 * @return Variant pointer (usually NIL for deferred calls)
 */
void* gdext_call_method_deferred(void* object, const char* method_name, void** args, int arg_count);

/**
 * @brief Helper: Call a method deferred with 1 argument
 * 
 * Perfect for add_child(child) - the most common deferred call!
 */
void* gdext_call_method1_deferred(void* object, const char* method_name, void* arg1);

/**
 * @brief TDD #133 Option B: Add child deferred with object pointers
 * 
 * Specialized function for add_child that accepts object pointers directly
 * instead of variants. Handles variant conversion internally to avoid
 * crashes in Go ToVariant() calls.
 * 
 * @param parent_object The parent object (raw pointer, NOT a variant)
 * @param child_object The child object to add (raw pointer, NOT a variant)
 * @return Variant pointer (usually NIL, should be freed by caller)
 */
void* gdext_add_child_deferred(void* parent_object, void* child_object);

/* ============================================================================
 * Property Access (TDD #128) - Get/Set Object Properties
 * ============================================================================ */

/**
 * @brief Set a property on a Godot object
 * 
 * @param object The object to set the property on
 * @param property_name The name of the property
 * @param value The value to set (as a Variant)
 * @return true on success, false on failure
 */
bool gdext_c_object_set_property(gdext_c_object_t object, const char* property_name, GDExtensionConstVariantPtr value);

/**
 * @brief Get a property from a Godot object
 * 
 * @param object The object to get the property from
 * @param property_name The name of the property
 * @return The property value as a Variant, or NULL on failure
 */
void* gdext_c_object_get_property(gdext_c_object_t object, const char* property_name);

/* Legacy compatibility (for existing code) */
#define gdext_set_property(obj, name, val) (gdext_c_object_set_property((gdext_c_object_t)(obj), (name), (val)) ? 1 : 0)
#define gdext_get_property(obj, name) gdext_c_object_get_property((gdext_c_object_t)(obj), (name))

/* ============================================================================
 * Array Helpers (TDD #127) - Pure C Array Operations
 * ============================================================================ */

/**
 * @brief Get element from Array
 */
void* gdext_array_get(void* array, int index);

/**
 * @brief Set element in Array
 */
void gdext_array_set(void* array, int index, void* value);

/**
 * @brief Resize Array
 */
void gdext_array_resize(void* array, int new_size);

/**
 * @brief Get Array size
 */
int gdext_array_size(void* array);

/**
 * @brief Create new empty Array Variant
 */
void* gdext_variant_new_array(void);

/**
 * @brief Create PackedInt32Array Variant
 */
void* gdext_variant_from_packed_int32_array(int32_t* values, int count);

/**
 * @brief Create PackedVector3Array Variant
 */
void* gdext_variant_from_packed_vector3_array(float* values, int count);

/**
 * @brief Create new empty PackedInt32Array Variant
 */
void* gdext_variant_new_packed_int32_array(void);

/**
 * @brief Create new empty PackedVector3Array Variant
 */
void* gdext_variant_new_packed_vector3_array(void);

/* ============================================================================
 * PackedByteArray (TDD #152) - For GPU buffer updates
 * ============================================================================ */

/**
 * @brief Include PackedByteArray functions for SVO renderer buffer_update
 * 
 * PackedByteArray is critical for uploading binary data to GPU buffers.
 * Used by RenderingDevice.buffer_update() in the SVO voxel renderer.
 */
#include "gdext_c_packed_byte_array.h"

/**
 * TDD #154b: PURE C callback registration (NO RUST!)
 * Allows Go game logic to register callbacks for ready/process/physics_process
 */
#include "gdext_c_callbacks.h"

/**
 * TDD #155: PURE C GDExtension entry point (REPLACES Rust bridge!)
 * Main GDExtension initialization - this is what Godot loads
 */
#include "gdext_c_gdextension.h"

/**
 * TDD #159: Go helper functions
 * Convenience functions for Go game logic (GetNodeFromScene, etc.)
 */
#include "gdext_c_go_helpers.h"

#ifdef __cplusplus
}
#endif

#endif /* GDEXT_C_H */

