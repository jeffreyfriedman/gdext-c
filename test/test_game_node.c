/**
 * @file test_game_node.c
 * @brief TDD #156: Test GameNode class registration and callbacks
 * 
 * GameNode is a custom Godot class that:
 * 1. Extends Node
 * 2. Triggers Go callbacks for _ready, _process, _physics_process
 * 3. Can be instantiated from the scene tree
 */

#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include "../include/gdext_c.h"
#include "../include/gdext_c_gdextension.h"
#include "../include/gdext_c_callbacks.h"

// Test state tracking
static bool g_ready_callback_called = false;
static bool g_process_callback_called = false;
static bool g_physics_process_callback_called = false;
static double g_last_delta = 0.0;

/**
 * @brief Mock Go ready callback
 */
static void mock_go_ready_callback(void) {
    g_ready_callback_called = true;
    printf("  📞 Mock Go ready callback called\n");
}

/**
 * @brief Mock Go process callback
 */
static void mock_go_process_callback(double delta) {
    g_process_callback_called = true;
    g_last_delta = delta;
    printf("  📞 Mock Go process callback called (delta: %.3f)\n", delta);
}

/**
 * @brief Mock Go physics process callback
 */
static void mock_go_physics_process_callback(double delta) {
    g_physics_process_callback_called = true;
    g_last_delta = delta;
    printf("  📞 Mock Go physics process callback called (delta: %.3f)\n", delta);
}

/**
 * @brief Test: GameNode class registration
 */
void test_game_node_registration(void) {
    printf("TEST: GameNode class registration\n");
    
    // Reset state
    g_ready_callback_called = false;
    g_process_callback_called = false;
    g_physics_process_callback_called = false;
    
    // Register mock callbacks
    c_register_go_callbacks(
        mock_go_ready_callback,
        mock_go_process_callback,
        mock_go_physics_process_callback
    );
    
    printf("  ✅ Mock callbacks registered\n");
    
    // Note: We can't actually test class registration without a running Godot instance
    // This test verifies the structure and compilation
    printf("  ✅ GameNode registration structure verified\n");
}

/**
 * @brief Test: Callback triggering
 */
void test_callback_triggering(void) {
    printf("TEST: Callback triggering\n");
    
    // Reset state
    g_ready_callback_called = false;
    g_process_callback_called = false;
    g_physics_process_callback_called = false;
    g_last_delta = 0.0;
    
    // Register mock callbacks
    c_register_go_callbacks(
        mock_go_ready_callback,
        mock_go_process_callback,
        mock_go_physics_process_callback
    );
    
    // Trigger ready callback
    c_trigger_ready_callback();
    assert(g_ready_callback_called);
    printf("  ✅ Ready callback triggered successfully\n");
    
    // Trigger process callback
    c_trigger_process_callback(0.016);
    assert(g_process_callback_called);
    assert(g_last_delta == 0.016);
    printf("  ✅ Process callback triggered successfully\n");
    
    // Trigger physics process callback
    c_trigger_physics_process_callback(0.016);
    assert(g_physics_process_callback_called);
    assert(g_last_delta == 0.016);
    printf("  ✅ Physics process callback triggered successfully\n");
}

/**
 * @brief Test: GameNode instance creation structure
 */
void test_game_node_instance_creation(void) {
    printf("TEST: GameNode instance creation structure\n");
    
    // Verify that instance creation callback exists
    // Note: We can't test actual instance creation without Godot
    extern void* gdext_c_game_node_create_instance(void *p_userdata);
    (void)gdext_c_game_node_create_instance; // Mark as used
    
    printf("  ✅ Instance creation callback exists\n");
}

/**
 * @brief Test: GameNode instance destruction structure
 */
void test_game_node_instance_destruction(void) {
    printf("TEST: GameNode instance destruction structure\n");
    
    // Verify that instance destruction callback exists
    extern void gdext_c_game_node_free_instance(void *p_userdata, void *p_instance);
    (void)gdext_c_game_node_free_instance; // Mark as used
    
    printf("  ✅ Instance destruction callback exists\n");
}

/**
 * @brief Run all tests
 */
int main(void) {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║ TDD #156: GameNode Class Tests (PURE C - NO RUST!)        ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    test_game_node_registration();
    test_callback_triggering();
    test_game_node_instance_creation();
    test_game_node_instance_destruction();
    
    printf("\n");
    printf("✅ All GameNode tests PASSED!\n");
    printf("📝 Next: Implement GameNode to make all tests pass\n");
    printf("\n");
    
    return 0;
}


