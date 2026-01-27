/**
 * @file gdext_c_gdextension.h
 * @brief Pure C GDExtension entry point (REPLACES Rust bridge!)
 * 
 * TDD #155: C GDExtension initialization
 * This provides the entry point for Godot to load our extension.
 */

#ifndef GDEXT_C_GDEXTENSION_H
#define GDEXT_C_GDEXTENSION_H

#include "gdextension_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Main GDExtension library initialization function
 * 
 * This is the entry point that Godot calls when loading the extension.
 * REPLACES Rust bridge's gdext_initialize function.
 * 
 * @param p_get_proc_address Function to get GDExtension API functions
 * @param p_library Library handle
 * @param r_initialization Initialization structure to fill
 * @return GDExtensionBool 1 on success, 0 on failure
 */
GDExtensionBool gdext_c_library_init(
    GDExtensionInterfaceGetProcAddress p_get_proc_address,
    const GDExtensionClassLibraryPtr p_library,
    GDExtensionInitialization *r_initialization
);

/**
 * @brief Initialize at specific level
 * Called by Godot for each initialization level
 */
void gdext_c_initialize_level(void *p_userdata, GDExtensionInitializationLevel p_level);

/**
 * @brief Deinitialize at specific level
 * Called by Godot when shutting down each level
 */
void gdext_c_deinitialize_level(void *p_userdata, GDExtensionInitializationLevel p_level);

/**
 * @brief Register GameNode class with Godot
 * Called during SCENE level initialization
 * NOTE: Being replaced by lifecycle system (will be removed in TDD 3.2)
 */
void gdext_c_register_game_node_class(void *p_userdata, void *p_level);

#ifdef __cplusplus
}
#endif

#endif // GDEXT_C_GDEXTENSION_H


