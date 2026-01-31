/**
 * @file gdext_c_gpu_queue.h
 * @brief Thread-safe GPU operation queue for deferred execution
 * 
 * TDD: Solves macOS Metal threading issues universally
 * Operations queued from worker threads are executed on main thread
 * Operations from main thread execute immediately (zero overhead)
 * 
 * @author gdext-c contributors
 * @date 2026-01-29
 */

#ifndef GDEXT_C_GPU_QUEUE_H
#define GDEXT_C_GPU_QUEUE_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Opaque handle for a queued GPU operation
 */
typedef struct gdext_gpu_operation_handle* gdext_gpu_operation_t;

/**
 * @brief Function pointer type for GPU operations
 * 
 * @param userdata Context passed during queue submission
 * @return Result pointer (operation-specific, can be NULL)
 * 
 * @note The function MUST be thread-safe for data access
 * @note Return value lifetime is managed by caller
 */
typedef void* (*gdext_gpu_func_t)(void* userdata);

/**
 * @brief Callback for operation completion
 * 
 * @param result Result from GPU operation (can be NULL)
 * @param userdata Original context from submission
 * 
 * @note Called on main thread after operation completes
 * @note Callback is optional (can be NULL)
 */
typedef void (*gdext_gpu_callback_t)(void* result, void* userdata);

/**
 * @brief Initialize GPU operation queue
 * 
 * Must be called during GDExtension initialization (main thread).
 * Creates internal queue structures and synchronization primitives.
 * 
 * @note Idempotent - safe to call multiple times
 */
void gdext_gpu_queue_init(void);

/**
 * @brief Shutdown GPU operation queue
 * 
 * Waits for all pending operations to complete, then destroys queue.
 * Should be called during GDExtension deinitialization.
 * 
 * @note Blocks until queue is empty
 * @warning Do not queue new operations after calling this!
 */
void gdext_gpu_queue_shutdown(void);

/**
 * @brief Queue a GPU operation for main thread execution
 * 
 * If called from main thread: executes immediately and returns NULL
 * If called from worker thread: queues operation and returns handle
 * 
 * @param func GPU operation to execute
 * @param userdata Context for operation (copied, not referenced)
 * @param callback Optional completion callback (can be NULL)
 * @return Operation handle (NULL if executed immediately)
 * 
 * @note userdata is NOT automatically freed - caller manages lifetime
 * @note For immediate execution: result via return value
 * @note For queued execution: result via callback
 */
gdext_gpu_operation_t gdext_gpu_queue_submit(
    gdext_gpu_func_t func,
    void* userdata,
    gdext_gpu_callback_t callback
);

/**
 * @brief Process queued GPU operations (CALL FROM MAIN THREAD ONLY!)
 * 
 * Processes up to max_operations from the queue.
 * Should be called every frame from main thread (e.g., _process callback).
 * 
 * @param max_operations Maximum operations to process this frame (0 = unlimited)
 * @return Number of operations processed
 * 
 * @warning MUST be called from main thread!
 * @note Thread-safe: worker threads can queue while this processes
 */
int gdext_gpu_queue_process(int max_operations);

/**
 * @brief Wait for a specific operation to complete
 * 
 * Blocks until the operation finishes or timeout expires.
 * Useful for synchronous GPU operations from worker threads.
 * 
 * @param op Operation handle from gdext_gpu_queue_submit()
 * @param timeout_ms Timeout in milliseconds (0 = wait forever)
 * @return true if completed, false if timeout or invalid handle
 * 
 * @note Calling thread will block during wait
 * @note Main thread will process queue while waiting
 */
bool gdext_gpu_queue_wait(gdext_gpu_operation_t op, int timeout_ms);

/**
 * @brief Execute immediately if on main thread, otherwise queue
 * 
 * **This is the recommended way to call GPU operations!**
 * 
 * Automatically detects current thread:
 * - Main thread: executes immediately, returns result
 * - Worker thread: queues operation, blocks until complete, returns result
 * 
 * @param func GPU operation
 * @param userdata Context
 * @return Result from GPU operation
 * 
 * @note Blocks if called from worker thread (waits for main thread processing)
 * @note Zero overhead if called from main thread (direct execution)
 * 
 * @example
 * void* create_buffer_func(void* userdata) {
 *     BufferArgs* args = (BufferArgs*)userdata;
 *     return rendering_device_create_buffer(args->size, args->usage);
 * }
 * 
 * Buffer* buffer = gdext_gpu_execute_safe(create_buffer_func, &args);
 * // Works from any thread!
 */
void* gdext_gpu_execute_safe(gdext_gpu_func_t func, void* userdata);

/**
 * @brief Get number of pending operations in queue
 * 
 * @return Count of unprocessed operations
 * 
 * @note Thread-safe
 * @note Useful for debugging and performance monitoring
 */
int gdext_gpu_queue_get_pending_count(void);

/**
 * @brief Check if queue system is initialized
 * 
 * @return true if gdext_gpu_queue_init() has been called
 */
bool gdext_gpu_queue_is_initialized(void);

#ifdef __cplusplus
}
#endif

#endif // GDEXT_C_GPU_QUEUE_H


