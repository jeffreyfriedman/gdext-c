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
#include <stdlib.h>

// TDD #172: Path lookup cache to avoid repeated Node.get_node() calls
// Saw 34 lookups of '/root/Main/CanvasLayer/HUD' in 15 frames!
#define PATH_CACHE_SIZE 256

typedef struct {
    char* path;
    void* node;
} PathCacheEntry;

static PathCacheEntry path_cache[PATH_CACHE_SIZE];
static int path_cache_count = 0;

// Simple hash function for strings
static unsigned int hash_string(const char* str) {
    unsigned int hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    return hash % PATH_CACHE_SIZE;
}

// Look up path in cache
static void* path_cache_get(const char* path) {
    unsigned int hash = hash_string(path);
    
    // Linear probing for collision resolution
    for (int i = 0; i < PATH_CACHE_SIZE; i++) {
        unsigned int index = (hash + i) % PATH_CACHE_SIZE;
        if (path_cache[index].path == NULL) {
            // Empty slot, not in cache
            return NULL;
        }
        if (strcmp(path_cache[index].path, path) == 0) {
            // Found in cache!
            return path_cache[index].node;
        }
    }
    
    return NULL; // Cache full or not found
}

// Add path to cache
static void path_cache_put(const char* path, void* node) {
    unsigned int hash = hash_string(path);
    
    // Linear probing for collision resolution
    for (int i = 0; i < PATH_CACHE_SIZE; i++) {
        unsigned int index = (hash + i) % PATH_CACHE_SIZE;
        if (path_cache[index].path == NULL) {
            // Empty slot, add here
            path_cache[index].path = strdup(path);
            path_cache[index].node = node;
            path_cache_count++;
            return;
        }
        if (strcmp(path_cache[index].path, path) == 0) {
            // Already in cache, update
            path_cache[index].node = node;
            return;
        }
    }
    
    // Cache full, evict oldest (index 0) and shift
    free(path_cache[0].path);
    for (int i = 0; i < PATH_CACHE_SIZE - 1; i++) {
        path_cache[i] = path_cache[i + 1];
    }
    path_cache[PATH_CACHE_SIZE - 1].path = strdup(path);
    path_cache[PATH_CACHE_SIZE - 1].node = node;
}

/**
 * @brief Check if a key is currently pressed
 */
int gdext_go_is_key_pressed(int keycode) {
    return 0; // Stub
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
    
    // Check cache first (no logging on cache hit - hot path!)
    void* cached = path_cache_get(path);
    if (cached != NULL) {
        return cached;
    }
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    if (!iface) {
        fprintf(stderr, "[gdext-c] ❌ No interface!\n");
        return NULL;
    }
    
    // Create StringName for "Engine"
    char engine_sn[64] = {0};
    iface->string_name_new_with_latin1_chars(engine_sn, "Engine", 0);
    
    // Get Engine singleton
    GDExtensionObjectPtr engine = iface->global_get_singleton(engine_sn);
    if (!engine) {
        fprintf(stderr, "[gdext-c] ❌ Failed to get Engine singleton\n");
        return NULL;
    }
    
    // Call Engine.get_main_loop()
    gdext_c_object_t main_loop = gdext_engine_get_main_loop((gdext_c_object_t)engine);
    if (main_loop == 0) {
        fprintf(stderr, "[gdext-c] ❌ Failed to get main loop\n");
        return NULL;
    }
    
    // Call SceneTree.get_root()
    gdext_c_object_t root = gdext_scene_tree_get_root(main_loop);
    if (root == 0) {
        fprintf(stderr, "[gdext-c] ❌ Failed to get root node\n");
        return NULL;
    }
    
    // If path is just "/root", return root node
    if (strcmp(path, "/root") == 0) {
        void* root_ptr = (void*)(uintptr_t)root;
        path_cache_put(path, root_ptr);
        return root_ptr;
    }
    
    // For other paths, use Node.get_node(NodePath) to traverse the scene tree
    const char* relative_path = path;
    if (strncmp(path, "/root/", 6) == 0) {
        relative_path = path + 6;
    }
    
    // Create a String variant from the path
    void* path_string_variant = gdext_variant_from_string(relative_path);
    if (!path_string_variant) {
        fprintf(stderr, "[gdext-c] ❌ Failed to create String variant for path '%s'\n", path);
        return NULL;
    }
    
    // Call Node.get_node(relative_path) on root
    void* result_variant = gdext_call_method(root, "get_node", (void*[]){path_string_variant}, 1);
    
    if (path_string_variant) {
        gdext_variant_free(path_string_variant);
    }
    
    if (result_variant == NULL) {
        fprintf(stderr, "[gdext-c] ❌ Node not found at path '%s'\n", path);
        return NULL;
    }
    
    void* node_ptr = gdext_variant_to_object(result_variant);
    gdext_variant_free(result_variant);
    
    if (node_ptr == NULL) {
        fprintf(stderr, "[gdext-c] ❌ Node not found at path '%s' (object extraction returned NULL)\n", path);
        return NULL;
    }
    
    // Cache the looked-up node
    path_cache_put(path, node_ptr);
    
    return node_ptr;
}

/**
 * @brief Update a node's 2D position
 */
void gdext_go_update_node_position(void* node, float x, float y) {
    (void)node; (void)x; (void)y; // Stub
}

/**
 * @brief Set text on a Label node
 */
void gdext_go_set_node_text(void* node, const char* text) {
    (void)node; (void)text; // Stub
}

/**
 * @brief Set size on a Control node
 */
void gdext_go_set_node_size(void* node, float width, float height) {
    (void)node; (void)width; (void)height; // Stub
}
