/**
 * @file gdext_c_lifecycle.h
 * @brief Universal lifecycle system for GDExtension language bindings
 * 
 * This replaces GameNode with a clean, signal-based callback system
 * that works for ALL language bindings (Go, Rust, Python, Zig, etc.)
 * 
 * Inspired by graphics.gd's architecture.
 */

#ifndef GDEXT_C_LIFECYCLE_H
#define GDEXT_C_LIFECYCLE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Callback types that language bindings must implement
 */
typedef void (*gdext_c_ready_callback)(void);
typedef void (*gdext_c_process_callback)(double delta);
typedef void (*gdext_c_physics_callback)(double delta);
typedef void (*gdext_c_shutdown_callback)(void);

/**
 * @brief TDD 1.1: Check if Godot engine is fully initialized and ready
 * 
 * This checks:
 * - SceneTree singleton is available
 * - RenderingDevice is initialized (for GPU operations)
 * - Engine singleton is accessible
 * 
 * @return 1 if ready, 0 if not
 */
int gdext_c_is_engine_ready(void);

/**
 * @brief TDD 1.1: Wait until engine is ready (blocking)
 * 
 * Blocks until gdext_c_is_engine_ready() returns 1.
 * Returns when safe to create nodes, access singletons, etc.
 * 
 * Uses a polling loop with small sleep intervals.
 */
void gdext_c_wait_for_engine_ready(void);

/**
 * @brief TDD 1.2: Register language callbacks with gdext-c
 * 
 * This replaces GameNode - callbacks are dispatched via SceneTree signals.
 * Any callback can be NULL (will be skipped).
 * 
 * @param ready Called once when engine is ready
 * @param process Called every frame (~60 FPS)
 * @param physics Called every physics frame (60 Hz)
 * @param shutdown Called before engine shuts down
 */
void gdext_c_register_lifecycle_callbacks(
    gdext_c_ready_callback ready,
    gdext_c_process_callback process,
    gdext_c_physics_callback physics,
    gdext_c_shutdown_callback shutdown
);

/**
 * @brief TDD 1.3: Connect to SceneTree signals
 * 
 * Connects to:
 * - "process_frame" signal (for process callback)
 * - "physics_frame" signal (for physics callback)
 * 
 * @return 1 on success, 0 on failure
 */
int gdext_c_connect_to_scene_tree(void);

/**
 * @brief TDD 1.5: Start the game loop (blocking until shutdown)
 * 
 * This function:
 * 1. Connects to SceneTree signals
 * 2. Calls ready callback once
 * 3. Blocks until Godot shuts down
 * 4. Calls shutdown callback before returning
 * 
 * SceneTree signals will dispatch process/physics callbacks while running.
 */
void gdext_c_run_game_loop(void);

#ifdef __cplusplus
}
#endif

#endif /* GDEXT_C_LIFECYCLE_H */

