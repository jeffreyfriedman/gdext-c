/**
 * @file gdext_c_game_node.c
 * @brief Pure C implementation of GameNode class
 * 
 * TDD #155: Replaces Rust bridge's GameNode implementation
 * This is a Node that triggers Go callbacks for ready/process/physics_process.
 */

#include "gdext_c_gdextension.h"
#include "gdext_c_core.h"
#include "gdext_c_callbacks.h"
#include <stdio.h>
#include <string.h>

// GameNode virtual method implementations

/**
 * @brief GameNode._ready() - Called when node enters scene tree
 */
static void game_node_ready(void *p_instance) {
    (void)p_instance;
    printf("[gdext-c] 🎮 GameNode._ready() called (PURE C!)\n");
    fflush(stdout);
    
    // Trigger Go ready callback
    c_trigger_ready_callback();
}

/**
 * @brief GameNode._process(delta) - Called every frame
 */
static void game_node_process(void *p_instance, double delta) {
    (void)p_instance;
    
    // Trigger Go process callback
    c_trigger_process_callback(delta);
}

/**
 * @brief GameNode._physics_process(delta) - Called every physics frame
 */
static void game_node_physics_process(void *p_instance, double delta) {
    (void)p_instance;
    
    // Trigger Go physics process callback
    c_trigger_physics_process_callback(delta);
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
    
    // Create StringName for "GameNode"
    GDExtensionStringNamePtr class_name;
    iface->string_name_new_with_latin1_chars(&class_name, "GameNode", 0);
    
    // Create StringName for "Node" (parent class)
    GDExtensionStringNamePtr parent_name;
    iface->string_name_new_with_latin1_chars(&parent_name, "Node", 0);
    
    // Create ClassCreationInfo
    GDExtensionClassCreationInfo3 creation_info = {0};
    creation_info.is_virtual = 0;
    creation_info.is_abstract = 0;
    creation_info.is_exposed = 1;
    creation_info.set_func = NULL;
    creation_info.get_func = NULL;
    creation_info.get_property_list_func = NULL;
    creation_info.free_property_list_func = NULL;
    creation_info.property_can_revert_func = NULL;
    creation_info.property_get_revert_func = NULL;
    creation_info.validate_property_func = NULL;
    creation_info.notification_func = NULL;
    creation_info.to_string_func = NULL;
    creation_info.reference_func = NULL;
    creation_info.unreference_func = NULL;
    creation_info.create_instance_func = NULL;  // Let Godot create instances
    creation_info.free_instance_func = NULL;
    creation_info.recreate_instance_func = NULL;
    creation_info.get_virtual_func = NULL;
    creation_info.get_virtual_call_data_func = NULL;
    creation_info.call_virtual_with_data_func = NULL;
    creation_info.get_rid_func = NULL;
    creation_info.class_userdata = NULL;
    
    // Register the class
    iface->classdb_register_extension_class3(
        gdext_c_get_library_handle(),
        class_name,
        parent_name,
        &creation_info
    );
    
    printf("[gdext-c] ✅ GameNode class registered with Godot\n");
    
    // TODO: Bind virtual methods (_ready, _process, _physics_process)
    // This requires more GDExtension API calls for method binding
    printf("[gdext-c] 📝 TODO: Bind GameNode virtual methods\n");
    
    // Note: StringNames are managed by Godot, no need to manually destroy
    
    fflush(stdout);
}

