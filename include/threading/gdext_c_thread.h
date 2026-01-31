/**
 * @file gdext_c_thread.h
 * @brief Thread context detection for GPU operations
 * 
 * TDD: Universal solution for macOS Metal threading issues
 * Benefits ALL language bindings (Go, Rust, C++, future bindings)
 * 
 * @author gdext-c contributors
 * @date 2026-01-29
 */

#ifndef GDEXT_C_THREAD_H
#define GDEXT_C_THREAD_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize thread tracking system
 * 
 * MUST be called from Godot's main thread during GDExtension initialization.
 * Stores the main thread ID for later comparison.
 * 
 * @note This should be called in gdextension_initialize()
 */
void gdext_thread_init(void);

/**
 * @brief Check if current thread is Godot's main thread
 * 
 * Used to determine if GPU operations can execute immediately
 * or need to be queued for main thread execution.
 * 
 * @return true if current thread is main thread, false otherwise
 * 
 * @example
 * if (gdext_thread_is_main()) {
 *     // Safe to call GPU operations directly
 *     create_buffer();
 * } else {
 *     // Must queue for main thread
 *     gdext_gpu_queue_submit(create_buffer_func, data, NULL);
 * }
 */
bool gdext_thread_is_main(void);

/**
 * @brief Get current thread ID
 * 
 * Platform-specific thread identifier.
 * - macOS/Linux: pthread_t (cast to uint64_t)
 * - Windows: DWORD thread ID
 * 
 * @return Unique identifier for current thread
 */
uint64_t gdext_thread_current_id(void);

/**
 * @brief Get main thread ID
 * 
 * Returns the thread ID that was stored during gdext_thread_init().
 * Useful for debugging and logging.
 * 
 * @return Main thread ID (0 if not initialized)
 */
uint64_t gdext_thread_main_id(void);

/**
 * @brief Check if thread system is initialized
 * 
 * @return true if gdext_thread_init() has been called
 */
bool gdext_thread_is_initialized(void);

#ifdef __cplusplus
}
#endif

#endif // GDEXT_C_THREAD_H


