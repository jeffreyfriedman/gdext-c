/**
 * @file scene_access.c
 * @brief Scene tree access for gdext-c
 * 
 * TDD #122: Pure C scene tree operations
 */

#include "../../include/gdext_c.h"
#include <stdio.h>
#include <string.h>

// TDD #127: Use new interface-based approach
#include "../core/gdext_c_core.h"

/**
 * @brief Get the root node of the scene tree
 * 
 * TDD #122: Access scene tree via Engine.get_main_loop().get_root()
 * 
 * @return Root node pointer, or NULL on failure
 */
gdext_c_object_t gdext_c_get_root_node(void) {
    if (!gdext_c_is_initialized()) {
        fprintf(stderr, "[gdext-c] ❌ Library not initialized! Call gdext_c_initialize() first\n");
        return NULL;
    }
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    if (!iface) {
        fprintf(stderr, "[gdext-c] ❌ Failed to get interface!\n");
        return NULL;
    }
    
    printf("[gdext-c] 🌳 TDD #122: Getting scene tree root node...\n");
    fflush(stdout);
    
    // Step 1: Get Engine singleton
    char engine_sn[64] = {0};
    iface->string_name_new_with_latin1_chars(engine_sn, "Engine", 0);
    void* engine = iface->global_get_singleton(engine_sn);
    
    if (!engine) {
        fprintf(stderr, "[gdext-c] ❌ Failed to get Engine singleton!\n");
        return NULL;
    }
    
    // Step 2: Call Engine.get_main_loop() to get SceneTree
    char engine_class_sn[64] = {0};
    char get_main_loop_sn[64] = {0};
    iface->string_name_new_with_latin1_chars(engine_class_sn, "Engine", 0);
    iface->string_name_new_with_latin1_chars(get_main_loop_sn, "get_main_loop", 0);
    
    void* get_main_loop_bind = iface->classdb_get_method_bind(engine_class_sn, get_main_loop_sn, 1016888095);
    if (!get_main_loop_bind) {
        fprintf(stderr, "[gdext-c] ❌ Failed to get Engine.get_main_loop method bind!\n");
        return NULL;
    }
    
    void* scene_tree = NULL;
    iface->object_method_bind_ptrcall(get_main_loop_bind, engine, NULL, &scene_tree);
    
    if (!scene_tree) {
        fprintf(stderr, "[gdext-c] ❌ Failed to get SceneTree!\n");
        return NULL;
    }
    
    // Step 3: Call SceneTree.get_root() to get root node
    char scene_tree_class_sn[64] = {0};
    char get_root_sn[64] = {0};
    iface->string_name_new_with_latin1_chars(scene_tree_class_sn, "SceneTree", 0);
    iface->string_name_new_with_latin1_chars(get_root_sn, "get_root", 0);
    
    void* get_root_bind = iface->classdb_get_method_bind(scene_tree_class_sn, get_root_sn, 1757182445);
    if (!get_root_bind) {
        fprintf(stderr, "[gdext-c] ❌ Failed to get SceneTree.get_root method bind!\n");
        return NULL;
    }
    
    void* root_node = NULL;
    iface->object_method_bind_ptrcall(get_root_bind, scene_tree, NULL, &root_node);
    
    if (!root_node) {
        fprintf(stderr, "[gdext-c] ❌ Failed to get root node!\n");
        return NULL;
    }
    
    printf("[gdext-c] ✅ TDD #122: Got scene tree root node: %p\n", root_node);
    fflush(stdout);
    
    return (gdext_c_object_t)root_node;
}

/**
 * @brief Get a node by path (e.g., "/root/Main/Player")
 * 
 * TDD #122: Node path resolution
 * 
 * @param path Node path (Godot NodePath format)
 * @return Node pointer, or NULL if not found
 */
gdext_c_object_t gdext_c_get_node(const char* path) {
    if (!path) {
        fprintf(stderr, "[gdext-c] ❌ NULL path!\n");
        return NULL;
    }
    
    // For now, only support "/root"
    if (strcmp(path, "/root") == 0) {
        return gdext_c_get_root_node();
    }
    
    // TODO: Implement full path resolution (would need SceneTree.get_node(path))
    printf("[gdext-c] ⚠️  Only '/root' supported currently, got: '%s'\n", path);
    return NULL;
}
