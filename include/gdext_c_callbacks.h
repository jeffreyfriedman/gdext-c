/**
 * @file gdext_c_callbacks.h
 * @brief Pure C callback registration for Go game logic
 * 
 * TDD #154b: ELIMINATE RUST - Implement callback registration in pure C
 */

#ifndef GDEXT_C_CALLBACKS_H
#define GDEXT_C_CALLBACKS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Function pointer types for Go callbacks
 */
typedef void (*gdext_c_ready_callback)(void);
typedef void (*gdext_c_process_callback)(double delta);
typedef void (*gdext_c_physics_process_callback)(double delta);

/**
 * @brief Register Go callbacks with the C bridge
 * 
 * This replaces the Rust bridge's rust_register_go_callbacks.
 * Pure C implementation - NO RUST!
 * 
 * @param ready_cb Callback for _ready event
 * @param process_cb Callback for _process event  
 * @param physics_process_cb Callback for _physics_process event
 */
void c_register_go_callbacks(
    gdext_c_ready_callback ready_cb,
    gdext_c_process_callback process_cb,
    gdext_c_physics_process_callback physics_process_cb
);

/**
 * @brief Trigger the ready callback
 * Called by GameNode._ready()
 */
void c_trigger_ready_callback(void);

/**
 * @brief Trigger the process callback
 * Called by GameNode._process(delta)
 */
void c_trigger_process_callback(double delta);

/**
 * @brief Trigger the physics process callback
 * Called by GameNode._physics_process(delta)
 */
void c_trigger_physics_process_callback(double delta);

#ifdef __cplusplus
}
#endif

#endif // GDEXT_C_CALLBACKS_H


