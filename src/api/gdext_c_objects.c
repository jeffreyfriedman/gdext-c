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

// TDD #127: Use new interface-based approach
#include "../core/gdext_c_core.h"

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
    
    if (!gdext_c_is_initialized()) {
        fprintf(stderr, "[gdext-c] ❌ TDD #122: Library not initialized! Call gdext_c_initialize() first\n");
        return NULL;
    }
    
    printf("[gdext-c] 🔧 TDD #122: Creating object of class: %s (PURE C, NO RUST!)\n", class_name);
    fflush(stdout);
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    if (!iface || !iface->classdb_construct_object || !iface->string_name_new_with_latin1_chars) {
        fprintf(stderr, "[gdext-c] ❌ TDD #122: Required functions not available!\n");
        return NULL;
    }
    
    // Step 2: Create StringName for the class
    char class_name_sn[256] = {0};
    iface->string_name_new_with_latin1_chars(class_name_sn, class_name, 0);
    
    // Step 3: Construct the object
    GDExtensionObjectPtr object = iface->classdb_construct_object(class_name_sn);
    
    if (!object) {
        fprintf(stderr, "[gdext-c] ❌ TDD #122: Failed to construct object of class '%s'\n", class_name);
        return NULL;
    }
    
    fprintf(stderr, "[gdext-c] 🔍 DEBUG: Object created, about to check for RefCounted...\n");
    fflush(stderr);
    
    // TDD REFCOUNTED: Try to call .reference() on ALL objects
    // For RefCounted objects, this will increment the refcount and return true
    // For regular Objects, this will fail (return false) but that's OK
    {
        // Create StringNames for method binding
        uint8_t refcounted_class_sn[16] = {0};
        uint8_t reference_method_sn[16] = {0};
        
        iface->string_name_new_with_latin1_chars(refcounted_class_sn, "RefCounted", 0);
        iface->string_name_new_with_latin1_chars(reference_method_sn, "reference", 0);
        
        // Get method bind for RefCounted.reference() (only exists on RefCounted, not Object)
        fprintf(stderr, "[gdext-c] 🔍 Attempting to get method bind for RefCounted.reference...\n");
        fflush(stderr);
        GDExtensionMethodBindPtr method_bind = iface->classdb_get_method_bind(
            refcounted_class_sn,
            reference_method_sn,
            2240911060 // Hash for reference() -> bool
        );
        
        fprintf(stderr, "[gdext-c] 🔍 method_bind = %p\n", method_bind);
        fflush(stderr);
        
        if (method_bind) {
            // Call reference() to try incrementing refcount
            uint8_t ret_val = 0;
            iface->object_method_bind_ptrcall(method_bind, object, NULL, &ret_val);
            
            if (ret_val) {
                fprintf(stderr, "[gdext-c] ✅ REFCOUNTED: %s is RefCounted, refcount incremented!\n", class_name);
            } else {
                // Normal Object (not RefCounted) - this is expected
                fprintf(stderr, "[gdext-c] ℹ️  %s is regular Object (not RefCounted), no reference() needed\n", class_name);
            }
        }
        
        // Cleanup StringNames
        typedef void (*GDExtensionPtrDestructor)(GDExtensionTypePtr);
        typedef GDExtensionPtrDestructor (*GetPtrDestructorFunc)(GDExtensionVariantType);
        extern gdext_c_proc_address_func gdext_c_get_proc_address_internal(void);
        GetPtrDestructorFunc get_destructor_func = (GetPtrDestructorFunc)gdext_c_get_proc_address_internal()("variant_get_ptr_destructor");
        if (get_destructor_func) {
            GDExtensionPtrDestructor sn_destructor = get_destructor_func(21); // StringName type
            if (sn_destructor) {
                sn_destructor(refcounted_class_sn);
                sn_destructor(reference_method_sn);
            }
        }
    }
    
    printf("[gdext-c] ✅ TDD #122: Created %s object: %p\n", class_name, object);
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

