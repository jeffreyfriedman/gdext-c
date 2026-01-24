/**
 * @file gdext_c_go_helpers.c
 * @brief Helper functions for Go game logic
 * 
 * TDD #159: Port helper functions from old bridge to pure C
 * These are simple wrappers that provide convenience functions for common operations
 */

#include "gdext_c_go_helpers.h"
#include "gdext_c_core.h"
#include "gdext_c_generated.h"
#include <stdio.h>
#include <string.h>

/**
 * @brief Check if a key is currently pressed
 */
int gdext_go_is_key_pressed(int keycode) {
    // TDD #159: Stub - not critical for game init
    fprintf(stderr, "[gdext-c] ⚠️ gdext_go_is_key_pressed: Stub\n");
    return 0;
}

/**
 * @brief Get a node by path from the scene tree
 * This is the CRITICAL function needed for game initialization
 */
void* gdext_go_get_node(const char* path) {
    if (!path) {
        fprintf(stderr, "[gdext-c] ❌ gdext_go_get_node: NULL path\n");
        return NULL;
    }
    
    fprintf(stderr, "[gdext-c] 🔍 gdext_go_get_node('%s')\n", path);
    fflush(stderr);
    
    // Get interface
    fprintf(stderr, "[gdext-c] 🔍 Getting interface...\n");
    fflush(stderr);
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    
    fprintf(stderr, "[gdext-c] 🔍 Interface: %p\n", (void*)iface);
    fflush(stderr);
    
    if (!iface) {
        fprintf(stderr, "[gdext-c] ❌ No interface!\n");
        return NULL;
    }
    
    fprintf(stderr, "[gdext-c] 🔍 Interface OK, creating StringName...\n");
    fflush(stderr);
    
    // Create StringName for "Engine" - use a buffer, not a pointer
    char engine_sn[64] = {0};
    iface->string_name_new_with_latin1_chars(engine_sn, "Engine", 0);
    
    fprintf(stderr, "[gdext-c] 🔍 StringName created, getting Engine singleton...\n");
    fflush(stderr);
    
    // Get Engine singleton
    GDExtensionObjectPtr engine = iface->global_get_singleton(engine_sn);
    
    if (!engine) {
        fprintf(stderr, "[gdext-c] ❌ Failed to get Engine singleton\n");
        return NULL;
    }
    
    fprintf(stderr, "[gdext-c] ✅ Got Engine singleton: %p\n", engine);
    fprintf(stderr, "[gdext-c] 🔍 About to call gdext_engine_get_main_loop...\n");
    fflush(stderr);
    
    // Call Engine.get_main_loop()
    gdext_c_object_t main_loop = gdext_engine_get_main_loop((gdext_c_object_t)engine);
    
    fprintf(stderr, "[gdext-c] 🔍 gdext_engine_get_main_loop returned: %p\n", (void*)(uintptr_t)main_loop);
    fflush(stderr);
    
    if (main_loop == 0) {
        fprintf(stderr, "[gdext-c] ❌ Failed to get main loop\n");
        return NULL;
    }
    
    fprintf(stderr, "[gdext-c] ✅ Got main loop\n");
    
    // Call SceneTree.get_root()
    gdext_c_object_t root = gdext_scene_tree_get_root(main_loop);
    if (root == 0) {
        fprintf(stderr, "[gdext-c] ❌ Failed to get root node\n");
        return NULL;
    }
    
    fprintf(stderr, "[gdext-c] ✅ Got root node: %p\n", (void*)root);
    
    // TDD #171: Implement proper path lookup using Node.get_node(path)
    // If path is just "/root", return root node
    if (strcmp(path, "/root") == 0) {
        fprintf(stderr, "[gdext-c] ✅ Returning root node for path '/root'\n");
        return (void*)(uintptr_t)root;
    }
    
    // For other paths, use Node.get_node(NodePath) to traverse the scene tree
    // First, remove "/root/" prefix if present
    const char* relative_path = path;
    if (strncmp(path, "/root/", 6) == 0) {
        relative_path = path + 6; // Skip "/root/", removing the leading "/" too
    } else if (strncmp(path, "/root", 5) == 0 && path[5] == '\0') {
        // Just "/root" - already handled above
        relative_path = path;
    }
    
    fprintf(stderr, "[gdext-c] 🔍 Looking up path: '%s' (relative: '%s')\n", path, relative_path);
    
    // TDD #171: Create a NodePath variant from the string path
    // First create a String variant, then use it to create NodePath
    void* path_string_variant = gdext_variant_from_string(relative_path);
    if (!path_string_variant) {
        fprintf(stderr, "[gdext-c] ❌ Failed to create String variant for path\n");
        return NULL;
    }
    
    // Call Node.get_node(relative_path) on root
    void* result_variant = gdext_call_method(root, "get_node", (void*[]){path_string_variant}, 1);
    
    // Free the path variant
    if (path_string_variant) {
        gdext_variant_free(path_string_variant);
    }
    
    if (result_variant == NULL) {
        fprintf(stderr, "[gdext-c] ❌ Node not found at path '%s' (variant is NULL)\n", path);
        return NULL;
    }
    
    // TDD #171: Extract the object pointer from the returned variant
    void* node_ptr = gdext_variant_to_object(result_variant);
    
    // Free the result variant
    gdext_variant_free(result_variant);
    
    if (node_ptr == NULL) {
        fprintf(stderr, "[gdext-c] ❌ Node not found at path '%s' (object extraction returned NULL)\n", path);
        return NULL;
    }
    
    fprintf(stderr, "[gdext-c] ✅ Found node at path '%s': %p\n", path, node_ptr);
    return node_ptr;
}

/**
 * @brief Update a node's 2D position
 */
void gdext_go_update_node_position(void* node, float x, float y) {
    // TDD #159: Stub - not critical for game init
    fprintf(stderr, "[gdext-c] ⚠️ gdext_go_update_node_position: Stub\n");
}

/**
 * @brief Set text on a Label node
 */
void gdext_go_set_node_text(void* node, const char* text) {
    // TDD #159: Stub - not critical for game init
    fprintf(stderr, "[gdext-c] ⚠️ gdext_go_set_node_text: Stub\n");
}

/**
 * @brief Set size on a Control node
 */
void gdext_go_set_node_size(void* node, float width, float height) {
    // TDD #159: Stub - not critical for game init
    fprintf(stderr, "[gdext-c] ⚠️ gdext_go_set_node_size: Stub\n");
}
