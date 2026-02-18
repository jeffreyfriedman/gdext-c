/**
 * @file gdext_c_game_node.c
 * @brief Pure C implementation of GameNode class
 * 
 * TDD #156: Complete GameNode with instance creation and notifications
 * This is a Node that triggers Go callbacks for ready/process/physics_process.
 */

#include "gdext_c_gdextension.h"
#include "gdext_c_core.h"
#include "gdext_c_callbacks.h"
#include "gdext_c_generated.h"  // TDD #160: For set_process functions
#include "gdext_c_lifecycle.h"  // TDD 1.4: For lifecycle callback dispatch
#include "threading/gdext_c_gpu_queue.h"  // TDD: GPU operation queue processing
#include "scene/gdext_c_scene_tree.h"  // OPTION B: Scene tree manipulation helpers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// GameNode instance data
typedef struct {
    GDExtensionObjectPtr godot_object; // The Godot Node object this instance is attached to
} GameNodeInstance;

// StringName as opaque struct (8 bytes on 64-bit as per extension_api.json!)
typedef struct {
    uint8_t opaque[8];
} StringName;

// Forward declaration
static void game_node_notification(void *p_instance, int32_t p_what, GDExtensionBool p_reversed);

/**
 * @brief Create a GameNode instance
 * TDD #157: Must return Object*, not just instance data!
 */
void* gdext_c_game_node_create_instance(void *p_userdata, GDExtensionBool p_notify_postinitialize) {
    (void)p_userdata;
    
    // Get the interface functions
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    if (!iface) {
        fprintf(stderr, "[gdext-c] ❌ Interface not initialized!\n");
        return NULL;
    }
    
    // Create base Node object
    StringName node_class_name;
    memset(&node_class_name, 0, sizeof(StringName));
    iface->string_name_new_with_latin1_chars((GDExtensionStringNamePtr)&node_class_name, "Node", 0);
    
    GDExtensionObjectPtr object = iface->classdb_construct_object((GDExtensionConstStringNamePtr)&node_class_name);
    
    if (!object) {
        fprintf(stderr, "[gdext-c] ❌ Failed to construct Node object!\n");
        return NULL;
    }
    
    // Allocate our custom instance data
    GameNodeInstance* instance = (GameNodeInstance*)malloc(sizeof(GameNodeInstance));
    if (!instance) {
        fprintf(stderr, "[gdext-c] ❌ Failed to allocate GameNode instance data!\n");
        return NULL;
    }
    
    instance->godot_object = object;
    
    // Attach our instance data to the object
    StringName gamenode_class_name;
    memset(&gamenode_class_name, 0, sizeof(StringName));
    iface->string_name_new_with_latin1_chars((GDExtensionStringNamePtr)&gamenode_class_name, "GameNode", 0);
    
    iface->object_set_instance(object, (GDExtensionConstStringNamePtr)&gamenode_class_name, instance);
    
    // Send NOTIFICATION_POSTINITIALIZE if requested
    if (p_notify_postinitialize) {
        game_node_notification(instance, 70, 0);
    }
    
    // TDD #157: Return the Object pointer, not just instance data!
    return object;
}

/**
 * @brief Free a GameNode instance
 * TDD #156: Instance destruction callback
 */
void gdext_c_game_node_free_instance(void *p_userdata, void *p_instance) {
    (void)p_userdata;
    
    if (!p_instance) {
        return;
    }
    
    GameNodeInstance* instance = (GameNodeInstance*)p_instance;
    instance->godot_object = NULL;
    free(instance);
}

/**
 * @brief GameNode notification handler
 * TDD #156: Handles _ready, _process, _physics_process via notifications
 */
static void game_node_notification(void *p_instance, int32_t p_what, GDExtensionBool p_reversed) {
    GameNodeInstance* instance = (GameNodeInstance*)p_instance;
    
    // Handle notifications
    switch (p_what) {
        case 10: // NOTIFICATION_ENTER_TREE - Enable process/physics EARLY
            if (instance && instance->godot_object) {
                extern void gdext_node_set_process(gdext_c_object_t instance, GDExtensionBool enable);
                extern void gdext_node_set_physics_process(gdext_c_object_t instance, GDExtensionBool enable);
                extern void gdext_node_set_process_mode(gdext_c_object_t instance, int32_t mode);
                
                gdext_node_set_process_mode(instance->godot_object, 0);  // PROCESS_MODE_ALWAYS
                gdext_node_set_process(instance->godot_object, 1);
                gdext_node_set_physics_process(instance->godot_object, 1);
            }
            break;
        
        case 13: // NOTIFICATION_READY
            // Double-check physics is enabled
            if (instance && instance->godot_object) {
                extern GDExtensionBool gdext_node_is_physics_processing(gdext_c_object_t instance);
                extern void gdext_node_set_physics_process(gdext_c_object_t instance, GDExtensionBool enable);
                
                GDExtensionBool is_physics = gdext_node_is_physics_processing(instance->godot_object);
                if (!is_physics) {
                    gdext_node_set_physics_process(instance->godot_object, 1);
                }
            }
            
            gdext_c_lifecycle_dispatch_ready();
            break;
            
        case 16: // NOTIFICATION_PHYSICS_PROCESS
            // DO NOT dispatch here! The _physics_process virtual method (registered via
            // get_virtual_func) already dispatches with the correct delta from Godot.
            // Dispatching in both places causes double-dispatch: Game.Update() runs
            // twice per frame, frame counters increment 2x, and state corruption occurs.
            break;
            
        case 17: { // NOTIFICATION_PROCESS - HOT PATH, no logging!
            // Process queued GPU operations (CRITICAL for Metal threading!)
            gdext_gpu_queue_process(0);  // 0 = process all pending
            // Dispatch to lifecycle system
            gdext_c_lifecycle_dispatch_process(0.016); // TODO: Get actual delta from Godot
            break;
        }
        
        case 27: // NOTIFICATION_POST_ENTER_TREE
            break;
            
        case 2012: // NOTIFICATION_PREDELETE
            fprintf(stderr, "[gdext-c] ⚠️  GameNode PREDELETE received! Callbacks will be cleared.\n");
            fflush(stderr);
            gdext_c_lifecycle_shutdown();
            break;
            
        default:
            // Only log unusual notifications (skip common Godot internal ones)
            if (p_what < 10 || (p_what > 17 && p_what < 27) || (p_what > 50 && p_what < 2000)) {
                // Skip silently - these are common Godot internal notifications
            }
            break;
    }
}

/**
 * @brief Virtual method wrapper for _process
 * TDD: Test if _process virtual works differently than _physics_process
 */
static void game_node_process_virtual(GDExtensionClassInstancePtr p_instance, const GDExtensionConstTypePtr *p_args, GDExtensionTypePtr r_ret) {
    (void)p_instance;
    (void)r_ret;
    
    // HOT PATH - no logging
    double delta = 0.016666667; // Default 60 FPS
    if (p_args && p_args[0]) {
        delta = *(const double*)p_args[0];
    }
    
    gdext_c_lifecycle_dispatch_process(delta);
}

/**
 * @brief Virtual method wrapper for _physics_process
 * TDD: Wire virtual method to lifecycle callbacks!
 */
static int g_physics_virtual_frame_count = 0;

static void game_node_physics_process_virtual(GDExtensionClassInstancePtr p_instance, const GDExtensionConstTypePtr *p_args, GDExtensionTypePtr r_ret) {
    (void)p_instance;
    (void)r_ret;
    
    g_physics_virtual_frame_count++;
    
    // Diagnostic: log first few frames and then every 1000 to confirm virtual fires
    if (g_physics_virtual_frame_count <= 5 || g_physics_virtual_frame_count % 1000 == 0) {
        fprintf(stderr, "[gdext-c] _physics_process virtual frame=%d\n", g_physics_virtual_frame_count);
        fflush(stderr);
    }
    
    double delta = 0.016666667; // Default 60 FPS
    if (p_args && p_args[0]) {
        delta = *(const double*)p_args[0];
    }
    
    gdext_c_lifecycle_dispatch_physics(delta);
}

/**
 * @brief Get virtual method for GameNode (VERSION 2 with hash)
 * TDD Deep Dive: Return function pointer for _physics_process!
 */
static GDExtensionClassCallVirtual game_node_get_virtual(void *p_userdata, GDExtensionConstStringNamePtr p_name, uint32_t p_hash) {
    (void)p_userdata;
    (void)p_name;
    
    // HOT PATH - called by Godot for every virtual method query, no logging
    if (p_hash == 373806689) {
        return (GDExtensionClassCallVirtual)game_node_physics_process_virtual;
    }
    
    return NULL;
}

/**
 * @brief Register GameNode class with Godot
 */
void gdext_c_register_game_node_class(void *p_userdata, void *p_level) {
    (void)p_userdata;
    (void)p_level;
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    if (!iface) {
        fprintf(stderr, "[gdext-c] ❌ Failed to get GDExtension interface!\n");
        return;
    }
    
    StringName class_name;
    memset(&class_name, 0, sizeof(StringName));
    iface->string_name_new_with_latin1_chars((GDExtensionStringNamePtr)&class_name, "GameNode", 0);
    
    StringName parent_name;
    memset(&parent_name, 0, sizeof(StringName));
    iface->string_name_new_with_latin1_chars((GDExtensionStringNamePtr)&parent_name, "Node", 0);
    
    // Create ClassCreationInfo
    // TDD #156: Use VERSION 4/5! (Option C debugging found this!)
    GDExtensionClassCreationInfo4 creation_info;
    memset(&creation_info, 0, sizeof(GDExtensionClassCreationInfo4));
    
    // Essential boolean fields - version 4 has 4 bools (includes is_runtime!)
    creation_info.is_virtual = false;
    creation_info.is_abstract = false;
    creation_info.is_exposed = true;
    creation_info.is_runtime = true;  // CRITICAL! Godot 4.5 checks this!
    
    // icon_path (new in version 4) - NULL for now
    creation_info.icon_path = NULL;
    
    // Instance lifecycle - REQUIRED
    creation_info.create_instance_func = gdext_c_game_node_create_instance;
    creation_info.free_instance_func = gdext_c_game_node_free_instance;
    
    // Notifications - for _ready callback
    creation_info.notification_func = game_node_notification;
    
    // TDD Deep Dive: Virtual methods - THIS IS CRITICAL FOR PHYSICS!
    creation_info.get_virtual_func = game_node_get_virtual;
    
    // Register the class
    GDExtensionClassLibraryPtr lib_handle = gdext_c_get_library_handle();
    
    iface->classdb_register_extension_class4(
        lib_handle,
        (GDExtensionConstStringNamePtr)&class_name,
        (GDExtensionConstStringNamePtr)&parent_name,
        &creation_info
    );
    
    printf("[gdext-c] ✅ GameNode class registered\n");
    fflush(stdout);
}

// TDD 1.4: GameNode now acts as a BRIDGE to lifecycle callbacks
// This allows us to use lifecycle system immediately without complex signal work
// GameNode forwards its notifications to registered lifecycle callbacks
// (lifecycle header is included at top of file)

// OPTION B: Global reference to programmatically-created GameNode
// Prevents Godot and C from deleting it
static GDExtensionObjectPtr g_programmatic_game_node = NULL;

/**
 * @brief OPTION B: Create GameNode programmatically (pure GDExtension, no GDScript!)
 * 
 * FULL IMPLEMENTATION using universal scene tree helpers!
 * 
 * This demonstrates the proper way to programmatically create and add nodes
 * to the scene tree from ANY language binding (Go, Rust, Python, etc.)
 * 
 * Steps:
 * 1. Create GameNode instance
 * 2. Get /root node from SceneTree
 * 3. Add GameNode to /root using call_deferred
 * 4. Store global reference to prevent premature deletion
 * 
 * Benefits ALL language bindings - this is reusable infrastructure!
 */
void gdext_c_create_programmatic_game_node(void) {
    // Create GameNode instance
    GDExtensionObjectPtr game_node = gdext_c_game_node_create_instance(NULL, 1);
    
    if (!game_node) {
        fprintf(stderr, "[gdext-c] ❌ Failed to create GameNode instance!\n");
        return;
    }
    
    // Get /root node
    GDExtensionObjectPtr root = gdext_c_get_root_node();
    
    if (!root) {
        fprintf(stderr, "[gdext-c] ❌ Failed to get /root node!\n");
        return;
    }
    
    // Add GameNode to /root (deferred for safety)
    int add_result = gdext_c_add_child_deferred(root, game_node);
    
    if (!add_result) {
        fprintf(stderr, "[gdext-c] ❌ Failed to add GameNode to /root!\n");
        return;
    }
    
    // Store global reference to prevent premature deletion
    g_programmatic_game_node = game_node;
    
    printf("[gdext-c] ✅ GameNode created and added to scene tree\n");
    fflush(stdout);
}

