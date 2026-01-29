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
    
    // TDD #157: Return the Object pointer, not just instance data!
    return object;
}

/**
 * @brief Free a GameNode instance
 * TDD #156: Instance destruction callback
 */
void gdext_c_game_node_free_instance(void *p_userdata, void *p_instance) {
    (void)p_userdata;
    
    fprintf(stderr, "[gdext-c] 🔬 TDD: free_instance called (instance=%p)\n", p_instance);
    fflush(stderr);
    
    if (!p_instance) {
        fprintf(stderr, "[gdext-c] ⚠️ TDD: free_instance called with NULL instance!\n");
        fflush(stderr);
        return;
    }
    
    fprintf(stderr, "[gdext-c] 🗑️ TDD: About to call free() on instance %p...\n", p_instance);
    fflush(stderr);
    
    free(p_instance);
    
    fprintf(stderr, "[gdext-c] ✅ TDD: free() completed successfully\n");
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
    
    // TDD Deep Dive Option D: CRITICAL FIX!
    // godot-cpp chains notifications through inheritance!
    // We MUST call Node's notification handler first (if not reversed)
    // or last (if reversed) just like godot-cpp does!
    
    GameNodeInstance* instance = (GameNodeInstance*)p_instance;
    if (instance && instance->godot_object) {
        const GDExtensionInterface* iface = gdext_c_get_interface_functions();
        
        // Get Node's notification function and call it!
        // This is what godot-cpp does with m_inherits::notification_bind(...)
        if (!p_reversed) {
            fprintf(stderr, "[gdext-c] 🔗 TDD Deep Dive: Calling PARENT Node notification (before our handler)...\n");
            fflush(stderr);
            
            // Create StringName for "Node" class
            StringName node_class_name;
            memset(&node_class_name, 0, sizeof(StringName));
            iface->string_name_new_with_latin1_chars((GDExtensionStringNamePtr)&node_class_name, "Node", 0);
            
            // Create StringName for "_notification" method
            StringName notification_method;
            memset(&notification_method, 0, sizeof(StringName));
            iface->string_name_new_with_latin1_chars((GDExtensionStringNamePtr)&notification_method, "_notification", 0);
            
            GDExtensionMethodBindPtr notification_bind = iface->classdb_get_method_bind((GDExtensionConstStringNamePtr)&node_class_name, (GDExtensionConstStringNamePtr)&notification_method, 3058944179);
            
            if (notification_bind) {
                // Call Node._notification(p_what)
                GDExtensionInt args[1] = {(GDExtensionInt)p_what};
                iface->object_method_bind_ptrcall(notification_bind, instance->godot_object, (const GDExtensionConstTypePtr*)args, NULL);
                fprintf(stderr, "[gdext-c] ✅ TDD Deep Dive: Parent Node notification called!\n");
                fflush(stderr);
            } else {
                fprintf(stderr, "[gdext-c] ⚠️  TDD Deep Dive: Could not get Node._notification bind (may be OK)\n");
                fflush(stderr);
            }
            
            // Note: StringNames are managed by Godot, cleanup not critical
        }
    }
    
    // Now handle our own notifications
    switch (p_what) {
        case 13: // NOTIFICATION_READY
            fprintf(stderr, "[gdext-c] 🎮 TDD Deep Dive: GameNode._ready() (Parent Node already handled!)\n");
            fflush(stderr);
            
            // TDD Deep Dive: NO NEED to call set_process/set_physics_process!
            // Node's notification handler already set up everything internally!
            fprintf(stderr, "[gdext-c] ✨ TDD Deep Dive: Node parent already configured process/physics!\n");
            fflush(stderr);
            
            // TDD 1.4: Dispatch to lifecycle system
            fprintf(stderr, "[gdext-c] 🔬 TDD 1.4: Dispatching ready to lifecycle system...\n");
            fflush(stderr);
            gdext_c_lifecycle_dispatch_ready();
            fprintf(stderr, "[gdext-c] ✅ TDD 1.4: Lifecycle ready callback completed\n");
            fflush(stderr);
            break;
            
        case 10: // NOTIFICATION_PROCESS
            fprintf(stderr, "[gdext-c] 🔍 TDD 1.4: _process notification received\n");
            fflush(stderr);
            // TDD 1.4: Dispatch to lifecycle system (replaces old c_trigger_process_callback)
            gdext_c_lifecycle_dispatch_process(0.016); // TODO: Get actual delta from Godot
            fprintf(stderr, "[gdext-c] ✅ TDD 1.4: Lifecycle process callback completed\n");
            fflush(stderr);
            break;
        
        case 16: // NOTIFICATION_PHYSICS_PROCESS
            fprintf(stderr, "[gdext-c] 🔍 TDD 1.4: _physics_process notification received (START)\n");
            fflush(stderr);
            // TDD 1.4: Dispatch to lifecycle system (replaces old c_trigger_physics_process_callback)
            gdext_c_lifecycle_dispatch_physics(0.016); // TODO: Get actual delta from Godot
            fprintf(stderr, "[gdext-c] ✅ TDD 1.4: Lifecycle physics callback completed (END)\n");
            fflush(stderr);
            break;
            
        case 17: // NOTIFICATION_POST_ENTER_TREE
        case 18: // NOTIFICATION_ENTER_TREE
            // TDD Deep Dive: No need to do anything here!
            // Node parent already handles ENTER_TREE properly
            fprintf(stderr, "[gdext-c] 🌳 TDD Deep Dive: ENTER_TREE (what=%d) - parent Node handled it!\n", p_what);
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
            break;
            
        default:
            // Ignore other notifications silently (don't log to reduce spam)
            break;
    }
    
    // TDD Deep Dive: Call parent Node notification AFTER our handler if reversed
    if (instance && instance->godot_object && p_reversed) {
        const GDExtensionInterface* iface = gdext_c_get_interface_functions();
        
        fprintf(stderr, "[gdext-c] 🔗 TDD Deep Dive: Calling PARENT Node notification (after our handler, reversed)...\n");
        fflush(stderr);
        
        // Create StringName for "Node" class
        StringName node_class_name;
        memset(&node_class_name, 0, sizeof(StringName));
        iface->string_name_new_with_latin1_chars((GDExtensionStringNamePtr)&node_class_name, "Node", 0);
        
        // Create StringName for "_notification" method
        StringName notification_method;
        memset(&notification_method, 0, sizeof(StringName));
        iface->string_name_new_with_latin1_chars((GDExtensionStringNamePtr)&notification_method, "_notification", 0);
        
        GDExtensionMethodBindPtr notification_bind = iface->classdb_get_method_bind((GDExtensionConstStringNamePtr)&node_class_name, (GDExtensionConstStringNamePtr)&notification_method, 3058944179);
        
        if (notification_bind) {
            // Call Node._notification(p_what)
            GDExtensionInt args[1] = {(GDExtensionInt)p_what};
            iface->object_method_bind_ptrcall(notification_bind, instance->godot_object, (const GDExtensionConstTypePtr*)args, NULL);
            fprintf(stderr, "[gdext-c] ✅ TDD Deep Dive: Parent Node notification called (reversed)!\n");
            fflush(stderr);
        }
        
        // Note: StringNames are managed by Godot, cleanup not critical
    }
    
    fprintf(stderr, "[gdext-c] ✅ TDD Deep Dive: game_node_notification completed (what=%d)\n", p_what);
    fflush(stderr);
}

/**
 * @brief Virtual method wrapper for _physics_process
 * TDD Deep Dive: THIS IS THE MISSING PIECE!
 * Physics process REQUIRES virtual method registration!
 */
static void game_node_physics_process_virtual(GDExtensionClassInstancePtr p_instance, const GDExtensionConstTypePtr *p_args) {
    fprintf(stderr, "[gdext-c] 🎯🎯🎯 VIRTUAL: _physics_process called! THIS IS IT!\n");
    fflush(stderr);
    
    // The virtual method is the trigger, but notification does the work
    // By returning a function pointer here, we tell Godot we override _physics_process
}

/**
 * @brief Get virtual method for GameNode (VERSION 2 with hash)
 * TDD Deep Dive: Return function pointer for _physics_process!
 */
static GDExtensionClassCallVirtual game_node_get_virtual(void *p_userdata, GDExtensionConstStringNamePtr p_name, uint32_t p_hash) {
    (void)p_userdata;
    (void)p_hash;  // Hash for _physics_process is 373806689
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    if (!iface) {
        return NULL;
    }
    
    fprintf(stderr, "[gdext-c] 🔍 TDD Deep Dive: get_virtual called! (hash=%u)\n", p_hash);
    fflush(stderr);
    
    // Check if this is _physics_process (hash = 373806689)
    if (p_hash == 373806689) {
        fprintf(stderr, "[gdext-c] ✅✅✅ TDD Deep Dive: Returning _physics_process virtual handler!\n");
        fflush(stderr);
        return (GDExtensionClassCallVirtual)game_node_physics_process_virtual;
    }
    
    // For all other virtual methods, return NULL
    fprintf(stderr, "[gdext-c] ⏭️  TDD Deep Dive: Unknown virtual (hash=%u), returning NULL\n", p_hash);
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

