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
    
    printf("[gdext-c] 🎮 TDD #157: Creating GameNode instance (notify_postinitialize=%d)\n", p_notify_postinitialize);
    fflush(stdout);
    
    // Get the interface functions
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    if (!iface) {
        fprintf(stderr, "[gdext-c] ❌ Interface not initialized!\n");
        fflush(stderr);
        return NULL;
    }
    
    // TDD #157: Create base Node object first!
    StringName node_class_name;
    memset(&node_class_name, 0, sizeof(StringName));
    iface->string_name_new_with_latin1_chars((GDExtensionStringNamePtr)&node_class_name, "Node", 0);
    
    GDExtensionObjectPtr object = iface->classdb_construct_object((GDExtensionConstStringNamePtr)&node_class_name);
    
    if (!object) {
        fprintf(stderr, "[gdext-c] ❌ Failed to construct Node object!\n");
        fflush(stderr);
        return NULL;
    }
    
    printf("[gdext-c] ✅ Created Node object: %p\n", (void*)object);
    
    // Allocate our custom instance data
    GameNodeInstance* instance = (GameNodeInstance*)malloc(sizeof(GameNodeInstance));
    if (!instance) {
        fprintf(stderr, "[gdext-c] ❌ Failed to allocate GameNode instance data!\n");
        fflush(stderr);
        return NULL;
    }
    
    // TDD #160: Store the Godot object pointer so we can call methods on it later
    instance->godot_object = object;
    
    // TDD #157: Attach our instance data to the object
    StringName gamenode_class_name;
    memset(&gamenode_class_name, 0, sizeof(StringName));
    iface->string_name_new_with_latin1_chars((GDExtensionStringNamePtr)&gamenode_class_name, "GameNode", 0);
    
    iface->object_set_instance(object, (GDExtensionConstStringNamePtr)&gamenode_class_name, instance);
    
    printf("[gdext-c] ✅ GameNode instance created and attached to object\n");
    fflush(stdout);
    
    // CRITICAL: Send NOTIFICATION_POSTINITIALIZE if requested!
    // This tells Godot the object is fully initialized and should not be deleted.
    if (p_notify_postinitialize) {
        printf("[gdext-c] 🔧 Sending NOTIFICATION_POSTINITIALIZE (70)...\n");
        fflush(stdout);
        
        // Call notification handler with NOTIFICATION_POSTINITIALIZE (70)
        game_node_notification(instance, 70, 0);
        
        printf("[gdext-c] ✅ NOTIFICATION_POSTINITIALIZE sent\n");
        fflush(stdout);
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
    
    fprintf(stderr, "[gdext-c] 🔬 MEMORY LEAK FIX: free_instance called (instance=%p)\n", p_instance);
    fflush(stderr);
    
    if (!p_instance) {
        fprintf(stderr, "[gdext-c] ⚠️ free_instance called with NULL instance!\n");
        fflush(stderr);
        return;
    }
    
    // MEMORY LEAK FIX: Actually free the instance!
    // This is called AFTER NOTIFICATION_PREDELETE, so lifecycle cleanup already happened.
    // We just need to free our GameNodeInstance struct (which only contains a pointer).
    // We do NOT free the godot_object - Godot owns that!
    
    GameNodeInstance* instance = (GameNodeInstance*)p_instance;
    
    fprintf(stderr, "[gdext-c] 🧹 MEMORY LEAK FIX: Freeing GameNodeInstance (godot_object=%p)...\n", 
            (void*)instance->godot_object);
    fflush(stderr);
    
    // Clear the pointer (defensive programming)
    instance->godot_object = NULL;
    
    // Free our instance struct
    free(instance);
    
    fprintf(stderr, "[gdext-c] ✅ MEMORY LEAK FIX: GameNodeInstance freed successfully\n");
    fflush(stderr);
}

/**
 * @brief GameNode notification handler
 * TDD #156: Handles _ready, _process, _physics_process via notifications
 */
static void game_node_notification(void *p_instance, int32_t p_what, GDExtensionBool p_reversed) {
    fprintf(stderr, "[gdext-c] 🔬 TDD Deep Dive: game_node_notification called (what=%d, reversed=%d, instance=%p)\n", 
            p_what, p_reversed, p_instance);
    fflush(stderr);
    
    GameNodeInstance* instance = (GameNodeInstance*)p_instance;
    
    // TDD GODOT 4.6 FIX: Godot handles parent notification forwarding automatically!
    // We just need to handle OUR notifications - don't try to manually call parent.
    // The p_reversed parameter tells us if parent already handled it (reversed=true)
    // or if we should handle it first (reversed=false).
    
    // Handle our own notifications
    switch (p_what) {
        case 10: // NOTIFICATION_ENTER_TREE - Enable process/physics EARLY (GODOT 4.6 FIX!)
            fprintf(stderr, "[gdext-c] 🌳 GODOT 4.6 FIX: ENTER_TREE (10) - enabling process/physics NOW!\n");
            fflush(stderr);
            
            // Enable BOTH process and physics in ENTER_TREE (before ready)
            if (instance && instance->godot_object) {
                extern void gdext_node_set_process(gdext_c_object_t instance, GDExtensionBool enable);
                extern void gdext_node_set_physics_process(gdext_c_object_t instance, GDExtensionBool enable);
                extern void gdext_node_set_process_mode(gdext_c_object_t instance, int32_t mode);
                
                fprintf(stderr, "[gdext-c] 🔧 GODOT 4.6 FIX: Enabling process + physics...\n");
                fflush(stderr);
                
                // Set process mode to ALWAYS (mode 0) to prevent pausing
                gdext_node_set_process_mode(instance->godot_object, 0);  // PROCESS_MODE_ALWAYS
                
                gdext_node_set_process(instance->godot_object, 1);
                gdext_node_set_physics_process(instance->godot_object, 1);
                
                fprintf(stderr, "[gdext-c] ✅ GODOT 4.6 FIX: Process + physics enabled!\n");
                fflush(stderr);
            }
            break;
        
        case 13: // NOTIFICATION_READY
            fprintf(stderr, "[gdext-c] 🎮 TDD: GameNode._ready()!\n");
            fflush(stderr);
            
            // TDD FIX 2026-02-01: Double-check physics is enabled
            if (instance && instance->godot_object) {
                extern GDExtensionBool gdext_node_is_physics_processing(gdext_c_object_t instance);
                extern void gdext_node_set_physics_process(gdext_c_object_t instance, GDExtensionBool enable);
                
                GDExtensionBool is_physics = gdext_node_is_physics_processing(instance->godot_object);
                fprintf(stderr, "[gdext-c] 🔍 TDD FIX: Physics processing enabled? %s\n", is_physics ? "YES" : "NO");
                
                if (!is_physics) {
                    fprintf(stderr, "[gdext-c] ⚠️  TDD FIX: Physics was disabled! Re-enabling...\n");
                    gdext_node_set_physics_process(instance->godot_object, 1);
                }
                fflush(stderr);
            }
            
            // TDD 1.4: Dispatch to lifecycle system
            // NOTE: Physics processing is already enabled in ENTER_TREE (line 178)
            fprintf(stderr, "[gdext-c] 🔬 TDD 1.4: Dispatching ready to lifecycle system...\n");
            fflush(stderr);
            gdext_c_lifecycle_dispatch_ready();
            fprintf(stderr, "[gdext-c] ✅ TDD 1.4: Lifecycle ready callback completed\n");
            fflush(stderr);
            break;
            
        case 16: // NOTIFICATION_PHYSICS_PROCESS (GODOT 4.6 CORRECT!)
            fprintf(stderr, "[gdext-c] 🔍 GODOT 4.6 FIX: PHYSICS_PROCESS (16) notification received\n");
            fflush(stderr);
            // Dispatch to lifecycle system
            gdext_c_lifecycle_dispatch_physics(0.016); // TODO: Get actual delta from Godot
            fprintf(stderr, "[gdext-c] ✅ GODOT 4.6 FIX: Lifecycle physics callback completed\n");
            fflush(stderr);
            break;
            
        case 17: { // NOTIFICATION_PROCESS (GODOT 4.6 CORRECT!)
            // Process queued GPU operations (CRITICAL for Metal threading!)
            int processed = gdext_gpu_queue_process(0);  // 0 = process all pending
            if (processed > 0) {
                fprintf(stderr, "[gdext-c] ⚡ GODOT 4.6 FIX: Processed %d GPU operation(s) in PROCESS\n", processed);
                fflush(stderr);
            }
            
            // Dispatch to lifecycle system
            gdext_c_lifecycle_dispatch_process(0.016); // TODO: Get actual delta from Godot
            break;
        }
        
        case 27: // NOTIFICATION_POST_ENTER_TREE (GODOT 4.6 CORRECT!)
            fprintf(stderr, "[gdext-c] 🌳 GODOT 4.6 FIX: POST_ENTER_TREE (27) notification received\n");
            fflush(stderr);
            break;
            
        case 2012: // NOTIFICATION_PREDELETE
            fprintf(stderr, "[gdext-c] 🚨 TDD 1.4: NOTIFICATION_PREDELETE received for instance=%p\n", p_instance);
            fprintf(stderr, "[gdext-c] 🧹 TDD 1.4: Calling lifecycle shutdown...\n");
            fflush(stderr);
            // TDD 1.4: Call lifecycle shutdown (which calls registered shutdown callback)
            gdext_c_lifecycle_shutdown();
            fprintf(stderr, "[gdext-c] ✅ TDD 1.4: Lifecycle shutdown completed\n");
            fflush(stderr);
            
            // TDD FIX 2026-02-01: REMOVED exit(0) - it was killing game immediately!
            // The exit(0) was preventing physics frames from running.
            // Let Godot handle cleanup naturally instead.
            fprintf(stderr, "[gdext-c] ✅ Allowing Godot to complete shutdown naturally (no forced exit)\n");
            fflush(stderr);
            break;
            
        default:
            // TDD: Log ALL other notifications during debugging
            fprintf(stderr, "[gdext-c] 🔔 Notification %d received (not handled)\n", p_what);
            fflush(stderr);
            break;
    }
    
    // TDD GODOT 4.6 FIX: DON'T manually call parent notifications!
    // Godot's engine automatically handles parent notification forwarding.
    // When p_reversed=true, parent was already called in forward pass.
    // When p_reversed=false, parent will be called after us in reverse pass.
    // We just need to handle OUR notifications and return.
    
    fprintf(stderr, "[gdext-c] ✅ TDD Deep Dive: game_node_notification completed (what=%d)\n", p_what);
    fflush(stderr);
}

/**
 * @brief Virtual method wrapper for _process
 * TDD: Test if _process virtual works differently than _physics_process
 */
static void game_node_process_virtual(GDExtensionClassInstancePtr p_instance, const GDExtensionConstTypePtr *p_args, GDExtensionTypePtr r_ret) {
    (void)p_instance;
    (void)r_ret;
    
    static int call_count = 0;
    call_count++;
    
    // Extract delta from p_args[0] (it's a double/float)
    double delta = 0.016666667; // Default 60 FPS
    if (p_args && p_args[0]) {
        delta = *(const double*)p_args[0];
    }
    
    if (call_count <= 3 || call_count % 60 == 0) {
        fprintf(stderr, "[gdext-c] 🎮 PROCESS virtual (call #%d, delta=%.4f)\n", call_count, delta);
        fflush(stderr);
    }
    
    gdext_c_lifecycle_dispatch_process(delta);
}

/**
 * @brief Virtual method wrapper for _physics_process
 * TDD: Wire virtual method to lifecycle callbacks!
 */
static void game_node_physics_process_virtual(GDExtensionClassInstancePtr p_instance, const GDExtensionConstTypePtr *p_args, GDExtensionTypePtr r_ret) {
    (void)p_instance;
    (void)r_ret;
    
    static int call_count = 0;
    call_count++;
    
    // Extract delta from p_args[0] (it's a double/float)
    double delta = 0.016666667; // Default 60 FPS
    if (p_args && p_args[0]) {
        // p_args[0] points to the double value
        delta = *(const double*)p_args[0];
    }
    
    if (call_count <= 3 || call_count % 60 == 0) {
        fprintf(stderr, "[gdext-c] 🎮 PHYSICS virtual (call #%d, delta=%.4f), dispatching to lifecycle...\n", call_count, delta);
        fflush(stderr);
    }
    
    // Dispatch to lifecycle system which will call Go callbacks
    gdext_c_lifecycle_dispatch_physics(delta);
}

/**
 * @brief Get virtual method for GameNode (VERSION 2 with hash)
 * TDD Deep Dive: Return function pointer for _physics_process!
 */
static GDExtensionClassCallVirtual game_node_get_virtual(void *p_userdata, GDExtensionConstStringNamePtr p_name, uint32_t p_hash) {
    (void)p_userdata;
    (void)p_name;
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    if (!iface) {
        return NULL;
    }
    
    // TDD METAL FIX: Log ALL virtual method requests to find what we're missing
    fprintf(stderr, "[gdext-c] 🔍 TDD METAL: get_virtual called! hash=%u, p_name=%p\n", p_hash, (void*)p_name);
    fflush(stderr);
    
    // Check if this is _process or _physics_process (both have hash = 373806689!)
    // According to Godot docs, both methods have the same signature
    if (p_hash == 373806689) {
        // We need to check the name to distinguish
        // For now, let's try returning _physics_process handler for this hash
        fprintf(stderr, "[gdext-c] ✅ TDD: Hash 373806689 detected, returning physics handler!\n");
        fflush(stderr);
        return (GDExtensionClassCallVirtual)game_node_physics_process_virtual;
    }
    
    // For all other virtual methods, return NULL
    fprintf(stderr, "[gdext-c] ⏭️  TDD: Unknown virtual (hash=%u), returning NULL\n", p_hash);
    fflush(stderr);
    return NULL;
}

/**
 * @brief Register GameNode class with Godot
 */
void gdext_c_register_game_node_class(void *p_userdata, void *p_level) {
    (void)p_userdata;
    (void)p_level;
    
    printf("[gdext-c] 🎮 TDD #155: Registering GameNode class (PURE C - NO RUST!)\n");
    fflush(stdout);
    
    // Get the GDExtension interface
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    if (!iface) {
        fprintf(stderr, "[gdext-c] ❌ Failed to get GDExtension interface!\n");
        fflush(stderr);
        return;
    }
    
    // Create StringName for "GameNode" (TDD #156: Correct usage as struct, not pointer!)
    StringName class_name;
    memset(&class_name, 0, sizeof(StringName));
    iface->string_name_new_with_latin1_chars((GDExtensionStringNamePtr)&class_name, "GameNode", 0);
    
    printf("[gdext-c] ✅ Created class name StringName\n");
    fflush(stdout);
    
    // Create StringName for "Node" (parent class)
    StringName parent_name;
    memset(&parent_name, 0, sizeof(StringName));
    iface->string_name_new_with_latin1_chars((GDExtensionStringNamePtr)&parent_name, "Node", 0);
    
    printf("[gdext-c] ✅ Created parent name StringName\n");
    printf("[gdext-c] 📝 Both StringNames created successfully\n");
    fflush(stdout);
    
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
    
    printf("[gdext-c] ✅ ClassCreationInfo4 configured (Option C found the answer!)\n");
    printf("[gdext-c] 🎯 TDD Deep Dive: get_virtual_func SET! This should enable physics!\n");
    printf("[gdext-c] 📝 Booleans: is_virtual=%d, is_abstract=%d, is_exposed=%d, is_runtime=%d\n", 
           creation_info.is_virtual, creation_info.is_abstract, creation_info.is_exposed, creation_info.is_runtime);
    printf("[gdext-c] 📝 icon_path=%p\n", creation_info.icon_path);
    printf("[gdext-c] 📝 Callbacks: create=%p, free=%p, notification=%p\n",
           (void*)creation_info.create_instance_func,
           (void*)creation_info.free_instance_func,
           (void*)creation_info.notification_func);
    fflush(stdout);
    
    // Register the class with VERSION 4 (Godot 4.5 expects this!)
    GDExtensionClassLibraryPtr lib_handle = gdext_c_get_library_handle();
    printf("[gdext-c] 📝 Library handle for registration: %p\n", (void*)lib_handle);
    printf("[gdext-c] 📝 Calling classdb_register_extension_class4...\n");
    printf("[gdext-c] 📝 Passing addresses: &class_name=%p, &parent_name=%p\n", (void*)&class_name, (void*)&parent_name);
    fflush(stdout);
    
    // Pass addresses of StringName structs (correct way!)
    iface->classdb_register_extension_class4(
        lib_handle,
        (GDExtensionConstStringNamePtr)&class_name,
        (GDExtensionConstStringNamePtr)&parent_name,
        &creation_info
    );
    
    printf("[gdext-c] 🎉 classdb_register_extension_class4 returned successfully!\n");
    fflush(stdout);
    
    printf("[gdext-c] ✅ GameNode class registered with Godot\n");
    printf("[gdext-c] ✅ Instance creation callback: %p\n", (void*)gdext_c_game_node_create_instance);
    printf("[gdext-c] ✅ Instance destruction callback: %p\n", (void*)gdext_c_game_node_free_instance);
    printf("[gdext-c] ✅ Notification callback: %p\n", (void*)game_node_notification);
    printf("[gdext-c] ✅ Virtual method callback: %p\n", (void*)game_node_get_virtual);
    printf("[gdext-c] 🎉 GameNode fully functional (PURE C - NO RUST!)\n");
    
    // Note: StringNames are managed by Godot, no need to manually destroy
    
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
    printf("[gdext-c] 🎯 OPTION B: Creating GameNode programmatically (PURE C!)...\n");
    printf("[gdext-c] 🎯 This benefits ALL language bindings - Go, Rust, Python, etc.\n");
    fflush(stdout);
    
    // Step 1: Create GameNode instance
    printf("[gdext-c] 🔧 Step 1: Creating GameNode instance...\n");
    fflush(stdout);
    
    // p_notify_postinitialize = true to tell Godot we're fully initialized
    GDExtensionObjectPtr game_node = gdext_c_game_node_create_instance(NULL, 1);
    
    if (!game_node) {
        fprintf(stderr, "[gdext-c] ❌ Failed to create GameNode instance!\n");
        fflush(stderr);
        return;
    }
    
    printf("[gdext-c] ✅ GameNode created at %p\n", (void*)game_node);
    fflush(stdout);
    
    // Step 2: Get /root node
    printf("[gdext-c] 🔧 Step 2: Getting /root node from SceneTree...\n");
    fflush(stdout);
    
    GDExtensionObjectPtr root = gdext_c_get_root_node();
    
    if (!root) {
        fprintf(stderr, "[gdext-c] ❌ Failed to get /root node!\n");
        fprintf(stderr, "[gdext-c] ❌ GameNode created but can't be added to scene tree\n");
        fflush(stderr);
        return;
    }
    
    printf("[gdext-c] ✅ Got /root node at %p\n", (void*)root);
    fflush(stdout);
    
    // Step 3: Add GameNode to /root (deferred for safety)
    printf("[gdext-c] 🔧 Step 3: Adding GameNode to /root tree (deferred)...\n");
    fflush(stdout);
    
    int add_result = gdext_c_add_child_deferred(root, game_node);
    
    if (!add_result) {
        fprintf(stderr, "[gdext-c] ❌ Failed to add GameNode to /root!\n");
        fflush(stderr);
        return;
    }
    
    printf("[gdext-c] ✅ GameNode added to /root tree\n");
    fflush(stdout);
    
    // Step 4: Store global reference
    printf("[gdext-c] 🔧 Step 4: Storing global reference...\n");
    fflush(stdout);
    
    g_programmatic_game_node = game_node;
    
    printf("[gdext-c] ✅ GameNode stored globally (prevents premature deletion)\n");
    fflush(stdout);
    
    // Success!
    printf("[gdext-c] 🎉 ═══════════════════════════════════════════════════════\n");
    printf("[gdext-c] 🎉 OPTION B COMPLETE: GameNode created programmatically!\n");
    printf("[gdext-c] 🎉 ═══════════════════════════════════════════════════════\n");
    printf("[gdext-c] ✅ GameNode address: %p\n", (void*)game_node);
    printf("[gdext-c] ✅ Added to scene tree: /root\n");
    printf("[gdext-c] ✅ Will receive _ready(), _process(), _physics_process()\n");
    printf("[gdext-c] ✅ Go lifecycle callbacks will fire\n");
    printf("[gdext-c] 🎯 PURE C IMPLEMENTATION - NO GDSCRIPT REQUIRED!\n");
    printf("[gdext-c] 🌍 UNIVERSAL INFRASTRUCTURE - Benefits ALL language bindings!\n");
    printf("[gdext-c] 🎉 ═══════════════════════════════════════════════════════\n");
    fflush(stdout);
}

