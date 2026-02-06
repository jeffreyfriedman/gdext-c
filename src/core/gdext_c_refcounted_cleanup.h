#ifndef GDEXT_C_REFCOUNTED_CLEANUP_H
#define GDEXT_C_REFCOUNTED_CLEANUP_H

#include <stddef.h>

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
 * 2. Binding calls gdext_queue_refcounted_cleanup(ptr) - thread-safe!
 * 3. Queue stores pointer for later processing
 * 4. Game loop calls gdext_process_refcounted_cleanup() each frame
 * 5. Process function calls unreference() on main thread - safe!
 * 
 * Benefits:
 * - ✅ Thread-safe by construction
 * - ✅ Works for ANY language binding (Go, Rust, Python, C#)
 * - ✅ Bounded queue (graceful degradation if full)
 * - ✅ No finalizer timing issues
 * - ✅ Deterministic cleanup timing
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
 * Queue a RefCounted object for cleanup.
 * 
 * THREAD-SAFE: Can be called from ANY thread (including GC threads).
 * 
 * This function should be called from language binding finalizers when
 * a RefCounted object wrapper is being garbage collected.
 * 
 * @param object_ptr Pointer to the Godot RefCounted object
 * 
 * Note: If queue is full, the object will leak (safe fallback).
 * This is better than crashing due to thread safety violations.
 */
void gdext_queue_refcounted_cleanup(void* object_ptr);

/**
 * Process queued RefCounted cleanups.
 * 
 * MAIN THREAD ONLY: Must be called from the main game loop thread.
 * 
 * This function calls unreference() on all queued objects, which is
 * thread-safe because it's guaranteed to run on the main thread.
 * 
 * Should be called once per frame at the beginning of the game loop,
 * before any game logic runs.
 * 
 * @return Number of objects cleaned up this frame
 */
int gdext_process_refcounted_cleanup(void);

/**
 * Get statistics about the cleanup queue.
 * 
 * @param queued_count Output: Number of objects currently queued
 * @param total_processed Output: Total objects processed since init
 * @param total_dropped Output: Total objects dropped (queue full)
 */
void gdext_refcounted_cleanup_stats(int* queued_count, int* total_processed, int* total_dropped);

#ifdef __cplusplus
}
#endif

#endif // GDEXT_C_REFCOUNTED_CLEANUP_H
