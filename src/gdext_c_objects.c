/**
 * @file gdext_c_objects.c
 * @brief Object creation and management for gdext-c
 * 
 * TDD #122: Implement object creation to fix particle crash
 * This replaces the Rust bridge for object creation
 */

#include "../include/gdext_c.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Import global proc_address from core
extern GDExtensionInterfaceGetProcAddress g_get_proc_address;
extern GDExtensionInterfaceStringNameNewWithLatin1Chars g_string_name_new_with_latin1_chars;

// Function pointer types we need
typedef GDExtensionObjectPtr (*ClassDBConstructObjectFunc)(GDExtensionConstStringNamePtr);
typedef void (*ObjectSetInstanceFunc)(GDExtensionObjectPtr, GDExtensionConstStringNamePtr, GDExtensionClassInstancePtr);

/**
 * @brief Create a Godot object by class name
 * 
 * TDD #122: Pure C object creation (no Rust bridge!)
 * Uses Godot's classdb_construct_object API
 * 
 * @param class_name Name of the Godot class (e.g. "Node3D", "CPUParticles3D")
 * @return Object pointer, or NULL on failure
 */
gdext_c_object_t gdext_c_create_object(const char* class_name) {
    if (!class_name) {
        fprintf(stderr, "[gdext-c] ❌ TDD #122: NULL class_name\n");
        return NULL;
    }
    
    if (!g_get_proc_address) {
        fprintf(stderr, "[gdext-c] ❌ TDD #122: Library not initialized! Call gdext_c_initialize() first\n");
        return NULL;
    }
    
    printf("[gdext-c] 🔧 TDD #122: Creating object of class: %s (PURE C, NO RUST!)\n", class_name);
    fflush(stdout);
    
    // Step 1: Get classdb_construct_object function
    ClassDBConstructObjectFunc classdb_construct_object = 
        (ClassDBConstructObjectFunc)g_get_proc_address("classdb_construct_object");
    
    if (!classdb_construct_object) {
        fprintf(stderr, "[gdext-c] ❌ TDD #122: classdb_construct_object not available!\n");
        return NULL;
    }
    
    // Step 2: Create StringName for the class
    if (!g_string_name_new_with_latin1_chars) {
        fprintf(stderr, "[gdext-c] ❌ TDD #122: string_name_new_with_latin1_chars not initialized!\n");
        return NULL;
    }
    
    char class_name_sn[256] = {0};
    g_string_name_new_with_latin1_chars(class_name_sn, class_name, 0);
    
    // Step 3: Construct the object
    GDExtensionObjectPtr object = classdb_construct_object(class_name_sn);
    
    if (!object) {
        fprintf(stderr, "[gdext-c] ❌ TDD #122: Failed to construct object of class '%s'\n", class_name);
        return NULL;
    }
    
    printf("[gdext-c] ✅ TDD #122: Created %s object: %p (PURE C!)\n", class_name, object);
    fflush(stdout);
    
    return (gdext_c_object_t)object;
}

/**
 * @brief Free a Godot object
 * 
 * Note: Usually not needed - Godot manages object lifetime via reference counting.
 * Only use if you explicitly need to free an unmanaged object.
 * 
 * @param object Object to free
 */
void gdext_c_free_object(gdext_c_object_t object) {
    if (!object) {
        return;
    }
    
    // TODO: Implement proper object freeing if needed
    // For now, Godot handles reference counting automatically
    printf("[gdext-c] 💡 TDD #122: Object free requested (Godot manages lifetime)\n");
    fflush(stdout);
}



