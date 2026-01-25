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
    fprintf(stderr, "[gdext-c] 🔬 TDD: c_trigger_ready_callback called\n");
    fflush(stderr);
    
    if (g_ready_callback) {
        fprintf(stderr, "[gdext-c] 🔬 TDD: About to call ready_callback at %p...\n", (void*)g_ready_callback);
        fflush(stderr);
        
        g_ready_callback();
        
        fprintf(stderr, "[gdext-c] ✅ TDD: ready_callback returned successfully\n");
        fflush(stderr);
    } else {
        fprintf(stderr, "[gdext-c] ⚠️ Ready callback not registered!\n");
        fflush(stderr);
    }
}

/**
 * @brief Trigger the process callback
 */
void c_trigger_process_callback(double delta) {
    fprintf(stderr, "[gdext-c] 🔬 TDD: c_trigger_process_callback called (delta=%.6f)\n", delta);
    fflush(stderr);
    
    if (g_process_callback) {
        fprintf(stderr, "[gdext-c] 🔬 TDD: About to call process_callback at %p...\n", (void*)g_process_callback);
        fflush(stderr);
        
        g_process_callback(delta);
        
        fprintf(stderr, "[gdext-c] ✅ TDD: process_callback returned successfully\n");
        fflush(stderr);
    }
    // Note: Process callback is optional, so no warning if NULL
}

/**
 * @brief Trigger the physics process callback
 */
void c_trigger_physics_process_callback(double delta) {
    fprintf(stderr, "[gdext-c] 🔬 TDD: c_trigger_physics_process_callback called (delta=%.6f)\n", delta);
    fflush(stderr);
    
    if (g_physics_process_callback) {
        fprintf(stderr, "[gdext-c] 🔬 TDD: About to call physics_process_callback at %p...\n", (void*)g_physics_process_callback);
        fflush(stderr);
        
        g_physics_process_callback(delta);
        
        fprintf(stderr, "[gdext-c] ✅ TDD: physics_process_callback returned successfully\n");
        fflush(stderr);
    } else {
        fprintf(stderr, "[gdext-c] ℹ️  TDD: physics_process_callback is NULL (not registered)\n");
        fflush(stderr);
    }
}

/**
 * @brief Check if callbacks are registered (for debugging)
 */
bool c_are_callbacks_registered(void) {
    return g_callbacks_registered;
}


