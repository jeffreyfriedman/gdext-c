/**
 * @file gdext_c_gpu_queue.c
 * @brief Thread-safe GPU operation queue implementation
 * 
 * TDD: Universal solution for macOS Metal threading
 */

#include "threading/gdext_c_gpu_queue.h"
#include "threading/gdext_c_thread.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>  // For INT_MAX

// Platform-specific synchronization
#ifdef _WIN32
    #include <windows.h>
    typedef CRITICAL_SECTION mutex_t;
    #define MUTEX_INIT(m) InitializeCriticalSection(m)
    #define MUTEX_DESTROY(m) DeleteCriticalSection(m)
    #define MUTEX_LOCK(m) EnterCriticalSection(m)
    #define MUTEX_UNLOCK(m) LeaveCriticalSection(m)
#else
    #include <pthread.h>
    typedef pthread_mutex_t mutex_t;
    #define MUTEX_INIT(m) pthread_mutex_init(m, NULL)
    #define MUTEX_DESTROY(m) pthread_mutex_destroy(m)
    #define MUTEX_LOCK(m) pthread_mutex_lock(m)
    #define MUTEX_UNLOCK(m) pthread_mutex_unlock(m)
#endif

/**
 * @brief Operation state
 */
typedef enum {
    OP_STATE_QUEUED,      // Waiting in queue
    OP_STATE_PROCESSING,  // Currently executing
    OP_STATE_COMPLETED,   // Finished successfully
    OP_STATE_FAILED       // Execution failed
} operation_state_t;

/**
 * @brief Queued GPU operation
 */
typedef struct gdext_gpu_operation_handle {
    gdext_gpu_func_t func;           // Operation function
    void* userdata;                  // User context
    gdext_gpu_callback_t callback;   // Completion callback (optional)
    void* result;                    // Operation result
    operation_state_t state;         // Current state
    struct gdext_gpu_operation_handle* next;  // Next in queue
} gpu_operation_t;

/**
 * @brief Global queue state
 */
static struct {
    gpu_operation_t* head;     // Queue head (first to process)
    gpu_operation_t* tail;     // Queue tail (last added)
    mutex_t lock;              // Queue synchronization
    int pending_count;         // Number of queued operations
    bool initialized;          // Init flag
} g_gpu_queue = {0};

/**
 * @brief Initialize GPU operation queue
 */
void gdext_gpu_queue_init(void) {
    if (g_gpu_queue.initialized) {
        printf("[gdext-c] ⚠️  TDD: GPU queue already initialized\n");
        fflush(stdout);
        return;
    }
    
    MUTEX_INIT(&g_gpu_queue.lock);
    g_gpu_queue.head = NULL;
    g_gpu_queue.tail = NULL;
    g_gpu_queue.pending_count = 0;
    g_gpu_queue.initialized = true;
    
    printf("[gdext-c] 🎮 TDD: GPU operation queue initialized\n");
    fflush(stdout);
}

/**
 * @brief Shutdown GPU operation queue
 */
void gdext_gpu_queue_shutdown(void) {
    if (!g_gpu_queue.initialized) {
        return;
    }
    
    printf("[gdext-c] 🧹 TDD: Shutting down GPU queue (pending: %d)...\n", 
           g_gpu_queue.pending_count);
    fflush(stdout);
    
    // Process remaining operations
    while (g_gpu_queue.pending_count > 0) {
        gdext_gpu_queue_process(0);  // Process all
    }
    
    MUTEX_DESTROY(&g_gpu_queue.lock);
    g_gpu_queue.initialized = false;
    
    printf("[gdext-c] ✅ TDD: GPU queue shutdown complete\n");
    fflush(stdout);
}

/**
 * @brief Queue a GPU operation
 */
gdext_gpu_operation_t gdext_gpu_queue_submit(
    gdext_gpu_func_t func,
    void* userdata,
    gdext_gpu_callback_t callback
) {
    if (!g_gpu_queue.initialized) {
        fprintf(stderr, "[gdext-c] ❌ TDD: GPU queue not initialized!\n");
        fflush(stderr);
        return NULL;
    }
    
    // If on main thread, execute immediately (zero overhead path)
    if (gdext_thread_is_main()) {
        void* result = func(userdata);
        if (callback) {
            callback(result, userdata);
        }
        return NULL;  // NULL = executed immediately
    }
    
    // Worker thread: queue for later
    gpu_operation_t* op = (gpu_operation_t*)malloc(sizeof(gpu_operation_t));
    if (!op) {
        fprintf(stderr, "[gdext-c] ❌ TDD: Failed to allocate GPU operation!\n");
        fflush(stderr);
        return NULL;
    }
    
    op->func = func;
    op->userdata = userdata;
    op->callback = callback;
    op->result = NULL;
    op->state = OP_STATE_QUEUED;
    op->next = NULL;
    
    // Add to queue (thread-safe)
    MUTEX_LOCK(&g_gpu_queue.lock);
    
    if (g_gpu_queue.tail) {
        g_gpu_queue.tail->next = op;
    } else {
        g_gpu_queue.head = op;
    }
    g_gpu_queue.tail = op;
    g_gpu_queue.pending_count++;
    
    MUTEX_UNLOCK(&g_gpu_queue.lock);
    
    printf("[gdext-c] 📦 TDD: GPU operation queued (pending: %d)\n", 
           g_gpu_queue.pending_count);
    fflush(stdout);
    
    return op;
}

/**
 * @brief Process queued GPU operations (main thread only!)
 */
int gdext_gpu_queue_process(int max_operations) {
    if (!g_gpu_queue.initialized) {
        return 0;
    }
    
    // Safety check: must be on main thread
    if (!gdext_thread_is_main()) {
        fprintf(stderr, "[gdext-c] ⚠️  TDD: gdext_gpu_queue_process() called from worker thread! Ignoring.\n");
        fflush(stderr);
        return 0;
    }
    
    int processed = 0;
    int limit = (max_operations <= 0) ? INT_MAX : max_operations;
    
    while (processed < limit) {
        gpu_operation_t* op = NULL;
        
        // Pop from queue (thread-safe)
        MUTEX_LOCK(&g_gpu_queue.lock);
        
        if (g_gpu_queue.head) {
            op = g_gpu_queue.head;
            g_gpu_queue.head = op->next;
            if (!g_gpu_queue.head) {
                g_gpu_queue.tail = NULL;
            }
            g_gpu_queue.pending_count--;
        }
        
        MUTEX_UNLOCK(&g_gpu_queue.lock);
        
        if (!op) {
            break;  // Queue empty
        }
        
        // Execute operation (outside lock for concurrency)
        op->state = OP_STATE_PROCESSING;
        op->result = op->func(op->userdata);
        op->state = OP_STATE_COMPLETED;
        
        // Call completion callback if provided
        if (op->callback) {
            op->callback(op->result, op->userdata);
        }
        
        // Free operation (callback must have copied result if needed)
        free(op);
        
        processed++;
    }
    
    if (processed > 0) {
        printf("[gdext-c] ⚡ TDD: Processed %d GPU operation(s)\n", processed);
        fflush(stdout);
    }
    
    return processed;
}

/**
 * @brief Wait for a specific operation to complete
 */
bool gdext_gpu_queue_wait(gdext_gpu_operation_t op, int timeout_ms) {
    (void)timeout_ms;  // TODO: implement timeout
    
    if (!op) {
        return true;  // NULL handle = already executed
    }
    
    // Spin until operation completes
    // TODO: implement proper condition variable
    while (op->state != OP_STATE_COMPLETED && op->state != OP_STATE_FAILED) {
        // If we're on main thread, process queue to make progress
        if (gdext_thread_is_main()) {
            gdext_gpu_queue_process(1);
        }
        // TODO: add sleep/yield to avoid busy-wait
    }
    
    return (op->state == OP_STATE_COMPLETED);
}

/**
 * @brief Execute immediately if on main thread, otherwise queue and wait
 */
void* gdext_gpu_execute_safe(gdext_gpu_func_t func, void* userdata) {
    if (!g_gpu_queue.initialized) {
        fprintf(stderr, "[gdext-c] ❌ TDD: GPU queue not initialized!\n");
        fflush(stderr);
        return NULL;
    }
    
    // Main thread: execute immediately (zero overhead)
    if (gdext_thread_is_main()) {
        return func(userdata);
    }
    
    // Worker thread: queue without callback (we'll access result directly)
    gdext_gpu_operation_t op = gdext_gpu_queue_submit(func, userdata, NULL);
    
    // Wait for completion
    if (op) {
        gdext_gpu_queue_wait(op, 0);
        // Access result from operation structure
        void* result = op->result;
        return result;
    }
    
    return NULL;
}

/**
 * @brief Get number of pending operations
 */
int gdext_gpu_queue_get_pending_count(void) {
    if (!g_gpu_queue.initialized) {
        return 0;
    }
    
    MUTEX_LOCK(&g_gpu_queue.lock);
    int count = g_gpu_queue.pending_count;
    MUTEX_UNLOCK(&g_gpu_queue.lock);
    
    return count;
}

/**
 * @brief Check if queue is initialized
 */
bool gdext_gpu_queue_is_initialized(void) {
    return g_gpu_queue.initialized;
}

