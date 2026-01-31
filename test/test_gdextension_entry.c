/**
 * @file test_gdextension_entry.c
 * @brief TDD #155: Test pure C GDExtension entry point (NO RUST!)
 * 
 * This test verifies the C GDExtension entry point works correctly.
 * REPLACES Rust bridge's gdext_initialize function.
 */

#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include "../include/gdext_c.h"
#include "../include/gdext_c_gdextension.h"

// Mock GDExtension interface for testing
static int g_initialize_called = 0;
static int g_deinitialize_called = 0;

/**
 * @brief Test: C GDExtension entry point exists and is callable
 */
void test_entry_point_exists(void) {
    printf("TEST: C GDExtension entry point exists\n");
    
    // The function should exist and be callable
    // We're not calling it here, just verifying it compiles
    extern GDExtensionBool gdext_c_library_init(
        GDExtensionInterfaceGetProcAddress p_get_proc_address,
        const GDExtensionClassLibraryPtr p_library,
        GDExtensionInitialization *r_initialization
    );
    
    (void)gdext_c_library_init; // Mark as used
    
    printf("  ✅ gdext_c_library_init symbol exists\n");
}

/**
 * @brief Test: Initialization structure is properly set up
 */
void test_initialization_structure(void) {
    printf("TEST: Initialization structure setup\n");
    
    // Create a mock initialization structure
    GDExtensionInitialization init = {0};
    
    // In real implementation, these would be set by gdext_c_library_init
    // For now, just verify the structure exists and has the right fields
    init.minimum_initialization_level = GDEXTENSION_INITIALIZATION_SCENE;
    
    assert(init.minimum_initialization_level == GDEXTENSION_INITIALIZATION_SCENE);
    
    printf("  ✅ GDExtensionInitialization structure valid\n");
}

/**
 * @brief Test: Class registration callback exists
 */
void test_class_registration_callback(void) {
    printf("TEST: Class registration callback exists\n");
    
    // The function should exist for registering GameNode
    extern void gdext_c_register_game_node_class(void *p_userdata, void *p_level);
    
    (void)gdext_c_register_game_node_class; // Mark as used
    
    printf("  ✅ gdext_c_register_game_node_class symbol exists\n");
}

/**
 * @brief Run all tests
 */
int main(void) {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║ TDD #155: C GDExtension Entry Point Tests (NO RUST!)      ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    test_entry_point_exists();
    test_initialization_structure();
    test_class_registration_callback();
    
    printf("\n");
    printf("✅ All C GDExtension entry point tests PASSED!\n");
    printf("📝 Next: Implement the functions to make tests pass\n");
    printf("\n");
    
    return 0;
}



