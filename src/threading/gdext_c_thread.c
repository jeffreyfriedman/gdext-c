/**
 * @file gdext_c_thread.c
 * @brief Thread context detection implementation
 * 
 * TDD: Cross-platform thread detection for GPU safety
 */

#include "threading/gdext_c_thread.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

// Platform-specific thread APIs
#ifdef _WIN32
    #include <windows.h>
    typedef DWORD thread_id_t;
    #define GET_CURRENT_THREAD_ID() ((uint64_t)GetCurrentThreadId())
#else
    #include <pthread.h>
    typedef pthread_t thread_id_t;
    #define GET_CURRENT_THREAD_ID() ((uint64_t)(uintptr_t)pthread_self())
#endif

// Global state
static uint64_t g_main_thread_id = 0;
static bool g_initialized = false;

/**
 * @brief Initialize thread tracking system
 * 
 * TDD: Store main thread ID for later comparison
 */
void gdext_thread_init(void) {
    g_main_thread_id = GET_CURRENT_THREAD_ID();
    g_initialized = true;
    
    printf("[gdext-c] 🧵 TDD: Thread system initialized (main thread ID: %llu)\n", 
           (unsigned long long)g_main_thread_id);
    fflush(stdout);
}

/**
 * @brief Check if current thread is Godot's main thread
 * 
 * TDD: Compare current thread ID to stored main thread ID
 */
bool gdext_thread_is_main(void) {
    if (!g_initialized) {
        fprintf(stderr, "[gdext-c] ⚠️  TDD: Thread system not initialized! Call gdext_thread_init() first.\n");
        fflush(stderr);
        // Conservative: assume we're NOT on main thread if not initialized
        return false;
    }
    
    uint64_t current = GET_CURRENT_THREAD_ID();
    bool is_main = (current == g_main_thread_id);
    
    // Only log the first call and mismatches (non-main thread access)
    static int first_call = 1;
    if (first_call) {
        first_call = 0;
    } else if (!is_main) {
        fprintf(stderr, "[gdext-c] ⚠️ Non-main thread detected: current=%llu, main=%llu\n",
               (unsigned long long)current,
               (unsigned long long)g_main_thread_id);
    }
    
    return is_main;
}

/**
 * @brief Get current thread ID
 */
uint64_t gdext_thread_current_id(void) {
    return GET_CURRENT_THREAD_ID();
}

/**
 * @brief Get main thread ID
 */
uint64_t gdext_thread_main_id(void) {
    return g_main_thread_id;
}

/**
 * @brief Check if thread system is initialized
 */
bool gdext_thread_is_initialized(void) {
    return g_initialized;
}


