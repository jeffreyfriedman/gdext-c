/**
 * @file gdext_c_go_helpers.h
 * @brief Helper functions for Go game logic
 * 
 * TDD #159: Port helper functions from old bridge to pure C
 */

#ifndef GDEXT_C_GO_HELPERS_H
#define GDEXT_C_GO_HELPERS_H

#include <stdbool.h>
#include "core/gdext_c_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Check if a key is currently pressed
 * @param keycode The key code to check (Godot Key enum value)
 * @return 1 if pressed, 0 if not
 */
int gdext_go_is_key_pressed(int keycode);

/**
 * @brief Check if a key is currently pressed (direct Input.is_key_pressed)
 * @param keycode The key code to check (Godot Key enum value)
 * @return 1 if pressed, 0 if not
 */
int gdext_is_key_pressed(int keycode);

/**
 * @brief Check if a key is currently pressed using event system
 * 
 * TDD: Alias for gdext_is_key_pressed (Go compatibility, from old Rust bridge).
 * Both functions do the same thing - check if a key is pressed.
 * 
 * @param keycode The key code to check (Godot Key enum value)
 * @return 1 if pressed, 0 if not
 */
int gdext_is_key_pressed_from_events(int keycode);

/**
 * @brief Get a node by path from the scene tree
 * @param path Node path (e.g., "/root/Main", "Player")
 * @return Node pointer or NULL if not found
 */
void* gdext_go_get_node(const char* path);

/**
 * @brief Update a node's 2D position
 * @param node Node pointer
 * @param x X coordinate
 * @param y Y coordinate
 */
void gdext_go_update_node_position(void* node, float x, float y);

/**
 * @brief Set text on a Label node
 * @param node Label node pointer
 * @param text Text to set
 */
void gdext_go_set_node_text(void* node, const char* text);

/**
 * @brief Set size on a Control node
 * @param node Control node pointer
 * @param width Width
 * @param height Height
 */
void gdext_go_set_node_size(void* node, float width, float height);

// ============================================================================
// Operation Queue System (Old Rust Bridge Compatibility)
// ============================================================================
// These functions allow thread-safe queuing of Godot operations.
// Currently implemented as no-op stubs.

/**
 * @brief Show a UI control node
 */
void gdext_queue_show_control(const char* node_path);

/**
 * @brief Hide a UI control node
 */
void gdext_queue_hide_control(const char* node_path);

/**
 * @brief Set text on a Label node
 */
void gdext_queue_set_text(const char* node_path, const char* text);

/**
 * @brief Queue a scene change
 */
void gdext_queue_change_scene(const char* scene_path);

/**
 * @brief Play a sound effect
 */
void gdext_queue_play_sound(const char* sound_path, float volume);

/**
 * @brief Play an animation on a node
 */
void gdext_queue_play_animation(const char* node_path, const char* animation_name);

#ifdef __cplusplus
}
#endif

#endif // GDEXT_C_GO_HELPERS_H

