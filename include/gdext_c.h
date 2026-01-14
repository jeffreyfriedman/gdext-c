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

#ifdef __cplusplus
}
#endif

#endif /* GDEXT_C_H */

