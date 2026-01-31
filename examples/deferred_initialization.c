/**
 * @file deferred_initialization.c
 * @brief Example of deferred visual node creation pattern
 * 
 * This example demonstrates the CORRECT way to create visual nodes in Godot
 * to avoid race conditions and initialization crashes (especially on macOS).
 * 
 * Key Pattern: Create data structures in Init(), create visuals in first Update()
 */

#include "../include/gdext_c.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// ==============================================================================
// EXAMPLE SYSTEM: Particle Pool
// ==============================================================================

/**
 * Particle pool system that pre-allocates particles for performance
 * 
 * CRITICAL: Nodes are NOT created in init(), but on first update!
 */
typedef struct {
    // Lifecycle flags
    bool visuals_initialized;
    
    // Cached references (set in init)
    void* scene;
    
    // Particle data (allocated in init, populated in first update)
    void** particle_nodes;
    bool* particle_active;
    int particle_capacity;
    
    // Stats
    int active_count;
    int peak_usage;
} ParticlePool;

/**
 * Initialize the particle pool
 * 
 * Called during Godot's initialization phase (DANGER ZONE!)
 * 
 * DO:
 * - Allocate data structures
 * - Cache scene reference
 * - Set flags
 * 
 * DON'T:
 * - Create visual nodes (MeshInstance3D, Particles, etc.)
 * - Call add_child() extensively
 * - Create GPU resources
 */
ParticlePool* particle_pool_create(void* scene, int capacity) {
    printf("[ParticlePool] Creating pool (capacity: %d)...\n", capacity);
    
    ParticlePool* pool = malloc(sizeof(ParticlePool));
    if (!pool) {
        fprintf(stderr, "[ParticlePool] Failed to allocate pool!\n");
        return NULL;
    }
    
    // Initialize data structures (SAFE during init)
    pool->visuals_initialized = false;
    pool->scene = scene;
    pool->particle_capacity = capacity;
    pool->active_count = 0;
    pool->peak_usage = 0;
    
    // Allocate arrays (SAFE - just memory)
    pool->particle_nodes = malloc(sizeof(void*) * capacity);
    pool->particle_active = calloc(capacity, sizeof(bool));
    
    if (!pool->particle_nodes || !pool->particle_active) {
        fprintf(stderr, "[ParticlePool] Failed to allocate arrays!\n");
        free(pool);
        return NULL;
    }
    
    printf("[ParticlePool] ✅ Pool created (visuals will be initialized on first use)\n");
    return pool;
}

/**
 * Ensure particle visuals are created (lazy initialization)
 * 
 * Called on first Update() - Engine is fully initialized at this point
 * 
 * This is where we create the actual CPUParticles3D nodes
 */
void particle_pool_ensure_visuals(ParticlePool* pool) {
    if (pool->visuals_initialized) {
        return; // Already done
    }
    
    printf("[ParticlePool] 🎨 Creating %d particle nodes (first update)...\n", 
           pool->particle_capacity);
    
    // NOW it's safe to create nodes
    for (int i = 0; i < pool->particle_capacity; i++) {
        // Create CPUParticles3D node
        void* particle_node = gdext_cpu_particles3_d_create();
        if (!particle_node) {
            fprintf(stderr, "[ParticlePool] Failed to create particle %d!\n", i);
            continue;
        }
        
        // Store reference
        pool->particle_nodes[i] = particle_node;
        
        // Add to scene tree (use deferred!)
        gdext_add_child_deferred(pool->scene, particle_node);
        
        // Configure particle (example settings)
        gdext_object_set_property_bool(particle_node, "emitting", false);
        gdext_object_set_property_float(particle_node, "amount", 10.0f);
        gdext_object_set_property_float(particle_node, "lifetime", 1.0f);
    }
    
    pool->visuals_initialized = true;
    printf("[ParticlePool] ✅ All particles created and added to scene!\n");
}

/**
 * Acquire a particle from the pool
 * 
 * This is called during gameplay when we need to spawn a particle effect
 */
void* particle_pool_acquire(ParticlePool* pool) {
    // Lazy initialization on first use
    particle_pool_ensure_visuals(pool);
    
    // Find inactive particle
    for (int i = 0; i < pool->particle_capacity; i++) {
        if (!pool->particle_active[i]) {
            pool->particle_active[i] = true;
            pool->active_count++;
            
            if (pool->active_count > pool->peak_usage) {
                pool->peak_usage = pool->active_count;
            }
            
            // Enable particle
            gdext_object_set_property_bool(pool->particle_nodes[i], "emitting", true);
            
            return pool->particle_nodes[i];
        }
    }
    
    fprintf(stderr, "[ParticlePool] ⚠️  Pool exhausted! Consider increasing capacity.\n");
    return NULL;
}

/**
 * Release a particle back to the pool
 */
void particle_pool_release(ParticlePool* pool, void* particle) {
    for (int i = 0; i < pool->particle_capacity; i++) {
        if (pool->particle_nodes[i] == particle && pool->particle_active[i]) {
            pool->particle_active[i] = false;
            pool->active_count--;
            
            // Disable particle
            gdext_object_set_property_bool(particle, "emitting", false);
            return;
        }
    }
}

/**
 * Print pool statistics
 */
void particle_pool_print_stats(ParticlePool* pool) {
    printf("[ParticlePool] Stats:\n");
    printf("  Capacity: %d\n", pool->particle_capacity);
    printf("  Active: %d\n", pool->active_count);
    printf("  Peak Usage: %d (%.1f%%)\n", 
           pool->peak_usage,
           (float)pool->peak_usage / pool->particle_capacity * 100.0f);
}

/**
 * Cleanup
 */
void particle_pool_destroy(ParticlePool* pool) {
    if (!pool) return;
    
    // Optionally remove nodes from scene
    if (pool->visuals_initialized) {
        for (int i = 0; i < pool->particle_capacity; i++) {
            if (pool->particle_nodes[i]) {
                // In a real implementation, you'd remove from scene tree
                // gdext_remove_child_deferred(pool->scene, pool->particle_nodes[i]);
            }
        }
    }
    
    free(pool->particle_nodes);
    free(pool->particle_active);
    free(pool);
    
    printf("[ParticlePool] Destroyed\n");
}

// ==============================================================================
// EXAMPLE USAGE (Game System Integration)
// ==============================================================================

/**
 * Example game system that uses the particle pool
 */
typedef struct {
    ParticlePool* particle_pool;
    int update_count;
} GameSystem;

/**
 * Initialize game system
 * 
 * Called during Godot init (Phase 2 - DANGER ZONE)
 */
GameSystem* game_system_init(void* scene) {
    printf("[GameSystem] Initializing...\n");
    
    GameSystem* sys = malloc(sizeof(GameSystem));
    if (!sys) return NULL;
    
    sys->update_count = 0;
    
    // Create particle pool (defers visual creation internally)
    sys->particle_pool = particle_pool_create(scene, 50);
    
    printf("[GameSystem] ✅ Initialized (visuals pending)\n");
    return sys;
}

/**
 * Update game system
 * 
 * Called every frame (Phase 3+ - SAFE)
 */
void game_system_update(GameSystem* sys, double delta) {
    sys->update_count++;
    
    // On first update, particles will be created automatically
    if (sys->update_count == 1) {
        printf("[GameSystem] First update - particles will initialize now\n");
    }
    
    // Simulate spawning particles occasionally
    if (sys->update_count % 60 == 0) { // Every 60 frames (~1 sec)
        void* particle = particle_pool_acquire(sys->particle_pool);
        if (particle) {
            printf("[GameSystem] Spawned particle (active: %d)\n", 
                   sys->particle_pool->active_count);
            
            // Set particle position (example)
            // gdext_object_set_property_vector3(particle, "global_position", x, y, z);
            
            // Release after 1 second (in real code, track lifetime properly)
            // For demo purposes, we'll release immediately
            particle_pool_release(sys->particle_pool, particle);
        }
    }
    
    // Print stats occasionally
    if (sys->update_count % 300 == 0) {
        particle_pool_print_stats(sys->particle_pool);
    }
}

/**
 * Cleanup
 */
void game_system_destroy(GameSystem* sys) {
    if (!sys) return;
    
    particle_pool_destroy(sys->particle_pool);
    free(sys);
    
    printf("[GameSystem] Destroyed\n");
}

// ==============================================================================
// EXPLANATION COMMENTS
// ==============================================================================

/*
 * WHY THIS PATTERN WORKS:
 * 
 * 1. During Initialize() (Godot Phase 2):
 *    - Memory allocated
 *    - Data structures set up
 *    - Flags/references cached
 *    - NO visual nodes created
 * 
 * 2. On First Update() (Godot Phase 3):
 *    - Engine is fully initialized
 *    - Scene tree is stable
 *    - Safe to create nodes
 *    - particle_pool_ensure_visuals() runs once
 * 
 * 3. Subsequent Updates:
 *    - visuals_initialized == true
 *    - Early return from ensure_visuals()
 *    - Normal operation
 * 
 * BENEFITS:
 * - ✅ No race conditions
 * - ✅ Works on ALL platforms (especially macOS)
 * - ✅ Clean separation of data vs visuals
 * - ✅ Lazy initialization (efficient)
 * - ✅ Easy to test (can skip visuals in tests)
 * 
 * APPLIES TO:
 * - Particle systems
 * - Object pools
 * - Enemy spawners
 * - Collectible systems
 * - Any system creating many visual nodes
 */

// ==============================================================================
// KEY TAKEAWAYS
// ==============================================================================

/*
 * ❌ DON'T DO THIS (Crashes on macOS):
 * 
 * void bad_init(void* scene) {
 *     for (int i = 0; i < 100; i++) {
 *         void* node = create_mesh_instance_3d();
 *         add_child(scene, node);  // CRASH!
 *     }
 * }
 * 
 * ✅ DO THIS INSTEAD:
 * 
 * typedef struct {
 *     bool visuals_ready;
 *     void* scene;
 * } System;
 * 
 * void good_init(System* sys, void* scene) {
 *     sys->visuals_ready = false;
 *     sys->scene = scene;
 * }
 * 
 * void good_update(System* sys) {
 *     if (!sys->visuals_ready) {
 *         for (int i = 0; i < 100; i++) {
 *             void* node = create_mesh_instance_3d();
 *             add_child_deferred(sys->scene, node);
 *         }
 *         sys->visuals_ready = true;
 *     }
 * }
 * 
 * REMEMBER: Init = Data, Update = Visuals
 */



