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

#ifdef __cplusplus
}
#endif

#endif // GDEXT_C_GO_HELPERS_H

