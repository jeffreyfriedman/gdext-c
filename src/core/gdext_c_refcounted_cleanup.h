#ifndef GDEXT_C_REFCOUNTED_CLEANUP_H
#define GDEXT_C_REFCOUNTED_CLEANUP_H

#include <stddef.h>
#include <stdint.h>

/*
 * RefCounted Cleanup Queue - Platform-Level Solution
 * 
 * Problem:
 * - Language bindings (Go, Rust, Python) use finalizers/destructors
 * - Finalizers run on GC threads, NOT the main game thread
 * - Godot methods MUST be called from main thread only
 * - Calling unreference() from GC thread → crashes, corruption
 * 
 * Solution:
 * - Finalizers just QUEUE cleanup (thread-safe, no Godot calls)
 * - Main game thread PROCESSES queue (safe Godot calls)
 * - Universal solution for all language bindings
 * 
 * Architecture:
 * 1. Language binding detects object is being GC'd (finalizer/destructor)
 * 2. Binding calls gdext_queue_refcounted_cleanup_with_id(ptr, instance_id) - thread-safe!
 * 3. Queue stores {ptr, instance_id} for later processing
 * 4. Game loop calls gdext_process_refcounted_cleanup() each frame
 * 5. Process function validates via object_get_instance_from_id() then
 *    calls the GENERATED gdext_ref_counted_unreference() - safe!
 * 
 * Key Design Decisions:
 * - Uses instance ID to validate objects are still alive (prevents use-after-free)
 * - Calls the codegen'd gdext_ref_counted_unreference() (correct hash guaranteed)
 * - Never duplicates method bind lookups that codegen already handles
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize the RefCounted cleanup system.
 * Called once during GDExtension initialization.
 */
void gdext_refcounted_cleanup_init(void);

/**
 * Shutdown the RefCounted cleanup system.
 * Called during GDExtension deinitialization.
 */
void gdext_refcounted_cleanup_shutdown(void);

/**
 * Queue a RefCounted object for cleanup (DEPRECATED - use _with_id variant).
 * Kept for backward compatibility; instance_id will be 0 (skips validation).
 */
void gdext_queue_refcounted_cleanup(void* object_ptr);

/**
 * Queue a RefCounted object for cleanup WITH instance ID for validation.
 * 
 * THREAD-SAFE: Can be called from ANY thread (including GC threads).
 * 
 * The instance_id is used to validate the object is still alive before
 * calling unreference(). This prevents use-after-free when Go GC runs
 * the finalizer after Godot has already freed the object.
 * 
 * @param object_ptr  Pointer to the Godot RefCounted object
 * @param instance_id The Godot instance ID (from object_get_instance_id)
 */
void gdext_queue_refcounted_cleanup_with_id(void* object_ptr, uint64_t instance_id);

/**
 * Process queued RefCounted cleanups.
 * 
 * MAIN THREAD ONLY: Must be called from the main game loop thread.
 * 
 * For each queued object:
 * 1. If instance_id != 0, validate via object_get_instance_from_id()
 * 2. If still alive, call gdext_ref_counted_unreference() (generated code)
 * 3. If dead (already freed by Godot), skip silently
 * 
 * @return Number of objects cleaned up this frame
 */
int gdext_process_refcounted_cleanup(void);

/**
 * Get statistics about the cleanup queue.
 */
void gdext_refcounted_cleanup_stats(int* queued_count, int* total_processed, int* total_dropped);

#ifdef __cplusplus
}
#endif

#endif // GDEXT_C_REFCOUNTED_CLEANUP_H
