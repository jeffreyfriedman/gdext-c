#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gdextension_bridge.h"

// TDD #104: Pure C implementation of node access
// Replaces Rust gdext_go_get_node() with pure C + GDExtension API

// TDD #106: Implement scene node access using GDExtension C API
void *gdext_c_get_scene_node(const char *node_path) {
    printf("[C Bridge] 🔍 TDD #106: gdext_c_get_scene_node('%s')\n", node_path);
    fflush(stdout);
    
    if (node_path == NULL) {
        printf("[C Bridge] ❌ TDD #106: NULL node_path\n");
        fflush(stdout);
        return NULL;
    }
    
    // Step 1: Get proc_address
    typedef void* (*GetProcAddressFunc)(const char*);
    void* proc_addr = gdext_c_bridge_get_proc_address();
    if (!proc_addr) {
        fprintf(stderr, "[C Bridge] ❌ TDD #106: proc_address is NULL!\n");
        fflush(stderr);
        return NULL;
    }
    GetProcAddressFunc get_proc_addr = (GetProcAddressFunc)proc_addr;
    printf("[C Bridge] ✅ TDD #106: Got proc_address: %p\n", proc_addr);
    fflush(stdout);
    
    // Step 2: Get global_get_singleton function
    typedef void* (*GlobalGetSingletonFunc)(const void*);
    GlobalGetSingletonFunc global_get_singleton = 
        (GlobalGetSingletonFunc)get_proc_addr("global_get_singleton");
    
    if (!global_get_singleton) {
        fprintf(stderr, "[C Bridge] ❌ TDD #106: global_get_singleton not available!\n");
        fflush(stderr);
        return NULL;
    }
    printf("[C Bridge] ✅ TDD #106: Got global_get_singleton function\n");
    fflush(stdout);
    
    // Step 3: Get Engine singleton and call get_main_loop()
    printf("[C Bridge] 🔍 TDD #106: Getting Engine singleton...\n");
    fflush(stdout);
    
    char engine_sn[64] = {0};
    g_string_name_new_with_latin1_chars(engine_sn, "Engine", 0);
    
    void* engine = global_get_singleton(engine_sn);
    if (!engine) {
        fprintf(stderr, "[C Bridge] ❌ TDD #106: Failed to get Engine singleton!\n");
        fflush(stderr);
        return NULL;
    }
    printf("[C Bridge] ✅ TDD #106: Got Engine singleton: %p\n", engine);
    fflush(stdout);
    
    // Step 4: Get classdb_get_method_bind function
    typedef void* (*ClassDBGetMethodBindFunc)(const void*, const void*, int64_t);
    ClassDBGetMethodBindFunc classdb_get_method_bind = 
        (ClassDBGetMethodBindFunc)get_proc_addr("classdb_get_method_bind");
    
    if (!classdb_get_method_bind) {
        fprintf(stderr, "[C Bridge] ❌ TDD #106: classdb_get_method_bind not available!\n");
        fflush(stderr);
        return NULL;
    }
    printf("[C Bridge] ✅ TDD #106: Got classdb_get_method_bind function\n");
    fflush(stdout);
    
    // Step 5: Get SceneTree via Engine.get_main_loop() with correct Godot 4.5 hash
    // TDD #107: Hash found from extension_api.json: 1016888095
    printf("[C Bridge] 🔍 TDD #107: Getting Engine.get_main_loop() with correct hash...\n");
    fflush(stdout);
    
    char engine_class_sn[64] = {0};
    char get_main_loop_sn[64] = {0};
    g_string_name_new_with_latin1_chars(engine_class_sn, "Engine", 0);
    g_string_name_new_with_latin1_chars(get_main_loop_sn, "get_main_loop", 0);
    
    // Hash from Godot 4.5 extension_api.json
    void* get_main_loop_bind = classdb_get_method_bind(engine_class_sn, get_main_loop_sn, 1016888095);
    
    if (!get_main_loop_bind) {
        fprintf(stderr, "[C Bridge] ❌ TDD #107: Failed to get Engine.get_main_loop method bind!\n");
        fprintf(stderr, "[C Bridge] 💡 TDD #107: Hash 1016888095 from extension_api.json\n");
        fflush(stderr);
        return NULL;
    }
    
    printf("[C Bridge] ✅ TDD #107: Got Engine.get_main_loop method bind: %p\n", get_main_loop_bind);
    fflush(stdout);
    
    // Step 6: Call Engine.get_main_loop() to get SceneTree  
    typedef void (*ObjectMethodBindPtrcallFunc)(void*, void*, const void**, void*);
    ObjectMethodBindPtrcallFunc object_method_bind_ptrcall = 
        (ObjectMethodBindPtrcallFunc)get_proc_addr("object_method_bind_ptrcall");
    
    if (!object_method_bind_ptrcall) {
        fprintf(stderr, "[C Bridge] ❌ TDD #107: object_method_bind_ptrcall not available!\n");
        fflush(stderr);
        return NULL;
    }
    
    printf("[C Bridge] 🔍 TDD #107: Calling Engine.get_main_loop()...\n");
    fflush(stdout);
    
    void* scene_tree = NULL;
    object_method_bind_ptrcall(get_main_loop_bind, engine, NULL, &scene_tree);
    
    if (!scene_tree) {
        fprintf(stderr, "[C Bridge] ❌ TDD #107: Engine.get_main_loop() returned NULL!\n");
        fflush(stderr);
        return NULL;
    }
    
    printf("[C Bridge] ✅ TDD #107: Got SceneTree from Engine.get_main_loop(): %p\n", scene_tree);
    fflush(stdout);
    
    // Step 7: Get object_method_bind_ptrcall for calling SceneTree.get_root()
    typedef void (*ObjectMethodBindPtrcallFunc2)(void*, void*, const void**, void*);
    ObjectMethodBindPtrcallFunc2 object_method_bind_ptrcall2 = 
        (ObjectMethodBindPtrcallFunc2)get_proc_addr("object_method_bind_ptrcall");
    
    if (!object_method_bind_ptrcall2) {
        fprintf(stderr, "[C Bridge] ❌ TDD #107: object_method_bind_ptrcall not available!\n");
        fflush(stderr);
        return NULL;
    }
    printf("[C Bridge] ✅ TDD #107: Got object_method_bind_ptrcall function\n");
    fflush(stdout);
    
    // Step 8: If requesting "/root", call SceneTree.get_root()
    if (strcmp(node_path, "/root") == 0) {
        printf("[C Bridge] 🔍 TDD #106: Requesting /root, calling SceneTree.get_root()...\n");
        fflush(stdout);
        
        char scene_tree_class_sn[64] = {0};
        char get_root_sn[64] = {0};
        g_string_name_new_with_latin1_chars(scene_tree_class_sn, "SceneTree", 0);
        g_string_name_new_with_latin1_chars(get_root_sn, "get_root", 0);
        
        printf("[C Bridge] 🔍 TDD #107: Getting method bind for SceneTree.get_root()...\n");
        fflush(stdout);
        
        // Hash from Godot 4.5 extension_api.json: 1757182445
        void* get_root_bind = classdb_get_method_bind(scene_tree_class_sn, get_root_sn, 1757182445);
        if (!get_root_bind) {
            fprintf(stderr, "[C Bridge] ❌ TDD #106: Failed to get SceneTree.get_root method bind!\n");
            fflush(stderr);
            return NULL;
        }
        
        printf("[C Bridge] ✅ TDD #106: Got SceneTree.get_root method bind: %p\n", get_root_bind);
        fflush(stdout);
        
        printf("[C Bridge] 🔍 TDD #106: Calling SceneTree.get_root()...\n");
        fflush(stdout);
        
        void* root_node = NULL;
        object_method_bind_ptrcall2(get_root_bind, scene_tree, NULL, &root_node);
        
        printf("[C Bridge] 🔍 TDD #106: get_root() returned: %p\n", root_node);
        fflush(stdout);
        
        if (!root_node) {
            fprintf(stderr, "[C Bridge] ❌ TDD #106: SceneTree.get_root() returned NULL!\n");
            fflush(stderr);
            return NULL;
        }
        
        printf("[C Bridge] ✅ TDD #106: Got root node: %p\n", root_node);
        fflush(stdout);
        return root_node;
    }
    
    // Step 9: For other paths, we'd call SceneTree.get_node(path)
    // TODO: Implement get_node(path) for non-root paths
    printf("[C Bridge] ⏳ TDD #106: Non-root paths not yet implemented\n");
    printf("[C Bridge] 💡 TDD #106: Use SceneTree.get_root() for now\n");
    fflush(stdout);
    
    return NULL;
}

// TDD #105: Wrapper for backward compatibility (calls renamed function)
void *gdext_go_get_node(const char *node_path) {
    return gdext_c_get_scene_node(node_path);
}

