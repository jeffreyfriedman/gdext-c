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
#include "gdext_c_defs.h"  // TDD #206: Export macros for entry point

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Main GDExtension library initialization function
 * 
 * This is the entry point that Godot calls when loading the extension.
 * REPLACES Rust bridge's gdext_initialize function.
 * 
 * TDD #206: MUST use GDE_EXPORT to make this symbol visible!
 * Without this, Godot cannot find the entry point and the extension won't load.
 * 
 * @param p_get_proc_address Function to get GDExtension API functions
 * @param p_library Library handle
 * @param r_initialization Initialization structure to fill
 * @return GDExtensionBool 1 on success, 0 on failure
 */
GDExtensionBool GDE_EXPORT gdext_c_library_init(
    GDExtensionInterfaceGetProcAddress p_get_proc_address,
    const GDExtensionClassLibraryPtr p_library,
    GDExtensionInitialization *r_initialization
);

/**
 * @brief Godot entry point (standard name expected by GDExtension)
 * TDD: Wrapper that calls gdext_c_library_init
 * This allows Godot to find the standard "gdextension_init" symbol
 */
GDExtensionBool GDE_EXPORT gdextension_init(
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
 * TDD 3.1: GameNode acts as a bridge to dispatch lifecycle callbacks
 */
void gdext_c_register_game_node_class(void *p_userdata, void *p_level);

/**
 * @brief OPTION B: Create GameNode programmatically (pure GDExtension!)
 * Creates a GameNode instance, adds it to /root, and manages its lifecycle.
 * This is the "no GDScript" approach that keeps the node alive via ref counting.
 */
void gdext_c_create_programmatic_game_node(void);

#ifdef __cplusplus
}
#endif

#endif // GDEXT_C_GDEXTENSION_H


