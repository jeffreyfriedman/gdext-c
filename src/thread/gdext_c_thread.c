/**
 * @file gdext_c_thread.c
 * @brief Implementation of threading context tracking
 */

#include "../../include/gdext_c_thread.h"
#include <pthread.h>
#include <stdio.h>

// Thread-local storage for current context
static __thread GDExtCThreadContext current_context = GDEXT_C_THREAD_UNKNOWN;

// Main thread ID (set during initialization)
static pthread_t main_thread_id = 0;
static bool main_thread_id_set = false;

/**
 * Initialize thread tracking (called once at startup)
 */
void gdext_c_thread_init(void) {
    main_thread_id = pthread_self();
    main_thread_id_set = true;
    current_context = GDEXT_C_THREAD_MAIN;
    
    fprintf(stderr, "[gdext-c] Thread tracking initialized (main thread: %p)\n", 
            (void*)main_thread_id);
}

/**
 * Get the current thread context
 */
GDExtCThreadContext gdext_c_get_thread_context(void) {
    return current_context;
}

/**
 * Set the thread context for the current callback
 */
void gdext_c_set_thread_context(GDExtCThreadContext context) {
    pthread_t current_thread = pthread_self();
    
    // Debug logging (can be disabled in production)
    if (current_context != context) {
        fprintf(stderr, "[gdext-c] Thread context changed: %s -> %s (thread: %p)\n",
                gdext_c_thread_context_to_string(current_context),
                gdext_c_thread_context_to_string(context),
                (void*)current_thread);
    }
    
    current_context = context;
}

/**
 * Get a human-readable string for a thread context
 */
const char* gdext_c_thread_context_to_string(GDExtCThreadContext context) {
    switch (context) {
        case GDEXT_C_THREAD_MAIN:    return "MAIN";
        case GDEXT_C_THREAD_RENDER:  return "RENDER";
        case GDEXT_C_THREAD_PHYSICS: return "PHYSICS";
        case GDEXT_C_THREAD_WORKER:  return "WORKER";
        case GDEXT_C_THREAD_UNKNOWN:
        default:                     return "UNKNOWN";
    }
}

/**
 * Check if the current thread context is safe for GPU operations
 */
bool gdext_c_is_gpu_safe_context(void) {
    GDExtCThreadContext ctx = gdext_c_get_thread_context();
    
    // On macOS Metal, GPU operations should only occur on MAIN or RENDER threads
    // Physics thread is NOT safe for GPU operations
    switch (ctx) {
        case GDEXT_C_THREAD_MAIN:
        case GDEXT_C_THREAD_RENDER:
            return true;
            
        case GDEXT_C_THREAD_PHYSICS:
        case GDEXT_C_THREAD_WORKER:
        case GDEXT_C_THREAD_UNKNOWN:
        default:
            return false;
    }
}

/**
 * Detect thread context based on current thread ID
 * (Heuristic: main thread = main, other threads = physics/worker)
 */
GDExtCThreadContext gdext_c_detect_thread_context(void) {
    if (!main_thread_id_set) {
        return GDEXT_C_THREAD_UNKNOWN;
    }
    
    pthread_t current_thread = pthread_self();
    
    if (pthread_equal(current_thread, main_thread_id)) {
        return GDEXT_C_THREAD_MAIN;
    }
    
    // Other threads are likely physics or worker threads
    // We can't distinguish without more context, so default to PHYSICS
    // (Conservative choice - physics is marked as NOT GPU-safe)
    return GDEXT_C_THREAD_PHYSICS;
}

/**
 * Auto-detect and set thread context
 * Called at the start of each callback if context is UNKNOWN
 */
void gdext_c_auto_detect_thread_context(void) {
    if (current_context == GDEXT_C_THREAD_UNKNOWN) {
        GDExtCThreadContext detected = gdext_c_detect_thread_context();
        gdext_c_set_thread_context(detected);
    }
}

