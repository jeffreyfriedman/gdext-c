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

#include <pthread.h>  // TDD: Thread-safe object creation

// TDD: Global mutex for object creation (prevents Metal/GPU race conditions on macOS)
static pthread_mutex_t object_creation_mutex = PTHREAD_MUTEX_INITIALIZER;
#include "../core/gdext_c_core.h"

// Function pointer types we need
typedef GDExtensionObjectPtr (*ClassDBConstructObjectFunc)(GDExtensionConstStringNamePtr);
typedef void (*ObjectSetInstanceFunc)(GDExtensionObjectPtr, GDExtensionConstStringNamePtr, GDExtensionClassInstancePtr);

/**
 * @brief Create a Godot object by class name
 * 
 * Pure C object creation (no Rust bridge!)
 * Uses Godot's classdb_construct_object API
 * 
 * @param class_name Name of the Godot class (e.g. "Node3D", "CPUParticles3D")
 * @return Object pointer, or NULL on failure
 */
gdext_c_object_t gdext_c_create_object(const char* class_name) {
    if (!class_name) {
        fprintf(stderr, "[gdext-c] ❌ gdext_c_create_object: NULL class_name\n");
        return NULL;
    }
    
    if (!gdext_c_is_initialized()) {
        fprintf(stderr, "[gdext-c] ❌ gdext_c_create_object: Library not initialized!\n");
        return NULL;
    }
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    if (!iface || !iface->classdb_construct_object || !iface->string_name_new_with_latin1_chars) {
        fprintf(stderr, "[gdext-c] ❌ gdext_c_create_object: Required functions not available!\n");
        return NULL;
    }
    
    // Create StringName for the class
    char class_name_sn[256] = {0};
    iface->string_name_new_with_latin1_chars(class_name_sn, class_name, 0);
    
    // Construct the object (mutex-protected for macOS Metal safety)
    pthread_mutex_lock(&object_creation_mutex);
    GDExtensionObjectPtr object = iface->classdb_construct_object(class_name_sn);
    pthread_mutex_unlock(&object_creation_mutex);
    
    if (!object) {
        fprintf(stderr, "[gdext-c] ❌ gdext_c_create_object: Failed to construct '%s'\n", class_name);
        return NULL;
    }
    
    // RefCounted lifecycle is managed by Go finalizers (see gdext-go/pkg/classdb/refcounted.go)
    // No need to check or call reference() here — Godot gives refcount=1 on construction.
    
    return (gdext_c_object_t)object;
}

/**
 * @brief Free a Godot object
 */
void gdext_c_free_object(gdext_c_object_t object) {
    if (!object) {
        return;
    }
    // Godot manages object lifetime via reference counting
}

/**
 * @brief Cleanup object creation resources
 */
void gdext_c_objects_shutdown(void) {
    pthread_mutex_destroy(&object_creation_mutex);
}
