/**
 * @file gdext_c_thread.h
 * @brief Threading context detection for gdext-c callbacks
 * 
 * Provides thread context information to help Go code determine
 * which Godot thread a callback is executing on.
 * 
 * This is critical for macOS Metal threading safety, where GPU
 * operations MUST occur on the same OS thread.
 */

#ifndef GDEXT_C_THREAD_H
#define GDEXT_C_THREAD_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Thread context types for Godot callbacks
 */
typedef enum {
    GDEXT_C_THREAD_UNKNOWN = 0,     // Unknown/uninitialized
    GDEXT_C_THREAD_MAIN,            // Main game thread (scene tree, rendering setup)
    GDEXT_C_THREAD_RENDER,          // GPU rendering thread (Metal/Vulkan command submission)
    GDEXT_C_THREAD_PHYSICS,         // Physics simulation thread (CharacterBody3D, RigidBody)
    GDEXT_C_THREAD_WORKER           // Background worker thread (async loading, etc.)
} GDExtCThreadContext;

/**
 * Get the current thread context.
 * 
 * This is determined heuristically based on which callback is executing:
 * - _ready, _process callbacks: MAIN thread
 * - _physics_process callbacks: PHYSICS thread
 * - Async operations: WORKER thread
 * 
 * @return Current thread context, or UNKNOWN if cannot be determined
 */
GDExtCThreadContext gdext_c_get_thread_context(void);

/**
 * Set the thread context for the current callback.
 * 
 * This is called internally by gdext-c before invoking Go callbacks.
 * Go code can then query the context to determine threading safety.
 * 
 * @param context The thread context for this callback
 */
void gdext_c_set_thread_context(GDExtCThreadContext context);

/**
 * Get a human-readable string for a thread context.
 * 
 * @param context Thread context to convert
 * @return String representation (e.g., "MAIN", "PHYSICS")
 */
const char* gdext_c_thread_context_to_string(GDExtCThreadContext context);

/**
 * Check if the current thread context is safe for GPU operations.
 * 
 * On macOS Metal, GPU operations should only occur on the MAIN thread.
 * On other platforms, this may be less restrictive.
 * 
 * @return true if GPU operations are safe, false otherwise
 */
bool gdext_c_is_gpu_safe_context(void);

#ifdef __cplusplus
}
#endif

#endif // GDEXT_C_THREAD_H


