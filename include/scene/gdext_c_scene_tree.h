/**
 * @file gdext_c_scene_tree.h
 * @brief Pure C scene tree manipulation helpers
 * 
 * UNIVERSAL INFRASTRUCTURE: Benefits ALL language bindings!
 * 
 * Provides high-level C wrappers for common scene tree operations:
 * - Getting the SceneTree singleton
 * - Getting the root node
 * - Adding/removing nodes from tree
 * - Method calling on nodes
 * 
 * This eliminates the need for each language binding to reimplement
 * these common operations. Go, Rust, Python, etc. can all use these.
 */

#ifndef GDEXT_C_SCENE_TREE_H
#define GDEXT_C_SCENE_TREE_H

#include "../gdextension_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get the SceneTree singleton
 * 
 * @return Pointer to SceneTree singleton, or NULL on failure
 */
GDExtensionObjectPtr gdext_c_get_scene_tree(void);

/**
 * @brief Get the root node (/root)
 * 
 * Calls get_root() on the SceneTree singleton.
 * 
 * @return Pointer to root node, or NULL on failure
 */
GDExtensionObjectPtr gdext_c_get_root_node(void);

/**
 * @brief Get a Godot singleton by name
 * 
 * Universal helper for any singleton (Engine, Input, OS, etc.)
 * 
 * @param singleton_name Name of singleton (e.g., "Engine", "Input")
 * @return Pointer to singleton, or NULL if not found
 */
GDExtensionObjectPtr gdext_c_get_singleton(const char* singleton_name);

/**
 * @brief Add a child node to a parent (DEFERRED)
 * 
 * Uses call_deferred to safely add a child to the scene tree.
 * This is the safe way to modify the scene tree from GDExtension.
 * 
 * @param parent Parent node
 * @param child Child node to add
 * @return 1 on success, 0 on failure
 */
int gdext_c_add_child_deferred(GDExtensionObjectPtr parent, GDExtensionObjectPtr child);

/**
 * @brief Remove a child node from parent (DEFERRED)
 * 
 * @param parent Parent node
 * @param child Child node to remove
 * @return 1 on success, 0 on failure
 */
int gdext_c_remove_child_deferred(GDExtensionObjectPtr parent, GDExtensionObjectPtr child);

/**
 * @brief Call a method on an object with no arguments
 * 
 * Simplified method calling for common operations.
 * 
 * @param object Object to call method on
 * @param method_name Name of method
 * @param out_result Optional pointer to store return value (can be NULL)
 * @return 1 on success, 0 on failure
 */
int gdext_c_call_method_simple(
    GDExtensionObjectPtr object,
    const char* method_name,
    GDExtensionVariantPtr out_result
);

/**
 * @brief Call a method on an object with deferred execution
 * 
 * Calls call_deferred on the object. Safe for scene tree modifications.
 * 
 * @param object Object to call method on
 * @param method_name Name of method to call
 * @param args Array of variant arguments (can be NULL)
 * @param arg_count Number of arguments
 * @return 1 on success, 0 on failure
 */
int gdext_c_call_method_deferred_with_args(
    GDExtensionObjectPtr object,
    const char* method_name,
    GDExtensionVariantPtr* args,
    int arg_count
);

#ifdef __cplusplus
}
#endif

#endif // GDEXT_C_SCENE_TREE_H

