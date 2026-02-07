/**
 * @file gdext_c_callbacks.c
 * @brief Pure C callback registration implementation
 * 
 * TDD #154b: ELIMINATE RUST - Implement callback registration in pure C
 * This replaces rust_register_go_callbacks from the Rust bridge.
 */

#include "gdext_c_callbacks.h"
#include <stdio.h>
#include <stdbool.h>

// Global callback storage
static gdext_c_ready_callback g_ready_callback = NULL;
static gdext_c_process_callback g_process_callback = NULL;
static gdext_c_physics_process_callback g_physics_process_callback = NULL;
static bool g_callbacks_registered = false;

/**
 * @brief Register Go callbacks with the C bridge
 * Pure C implementation - NO RUST!
 */
void c_register_go_callbacks(
    gdext_c_ready_callback ready_cb,
    gdext_c_process_callback process_cb,
    gdext_c_physics_process_callback physics_process_cb
) {
    printf("[gdext-c] 🚀 TDD #154b: c_register_go_callbacks called (PURE C - NO RUST!)\n");
    fflush(stdout);
    
    g_ready_callback = ready_cb;
    g_process_callback = process_cb;
    g_physics_process_callback = physics_process_cb;
    g_callbacks_registered = true;
    
    printf("[gdext-c] ✅ Ready callback: %s\n", ready_cb ? "registered" : "NULL");
    printf("[gdext-c] ✅ Process callback: %s\n", process_cb ? "registered" : "NULL");
    printf("[gdext-c] ✅ Physics process callback: %s\n", physics_process_cb ? "registered" : "NULL");
    fflush(stdout);
}

/**
 * @brief Trigger the ready callback
 */
void c_trigger_ready_callback(void) {
    if (g_ready_callback) {
        g_ready_callback();
    } else {
        fprintf(stderr, "[gdext-c] ⚠️ Ready callback not registered!\n");
        fflush(stderr);
    }
}

/**
 * @brief Trigger the process callback (HOT PATH - no logging)
 */
void c_trigger_process_callback(double delta) {
    if (g_process_callback) {
        g_process_callback(delta);
    }
}

/**
 * @brief Trigger the physics process callback (HOT PATH - no logging)
 */
void c_trigger_physics_process_callback(double delta) {
    if (g_physics_process_callback) {
        g_physics_process_callback(delta);
    }
}

/**
 * @brief Check if callbacks are registered (for debugging)
 */
bool c_are_callbacks_registered(void) {
    return g_callbacks_registered;
}


