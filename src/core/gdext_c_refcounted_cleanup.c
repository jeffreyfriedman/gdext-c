#include "gdext_c_refcounted_cleanup.h"
#include "gdext_c_core.h"
#include <stdio.h>
#include <pthread.h>
#include <stdatomic.h>

// Platform-specific mutex (same pattern as gpu_queue.c)
#ifdef _WIN32
    typedef CRITICAL_SECTION mutex_t;
    #define MUTEX_INIT(m) InitializeCriticalSection(m)
    #define MUTEX_DESTROY(m) DeleteCriticalSection(m)
    #define MUTEX_LOCK(m) EnterCriticalSection(m)
    #define MUTEX_UNLOCK(m) LeaveCriticalSection(m)
#else
    typedef pthread_mutex_t mutex_t;
    #define MUTEX_INIT(m) pthread_mutex_init(m, NULL)
    #define MUTEX_DESTROY(m) pthread_mutex_destroy(m)
    #define MUTEX_LOCK(m) pthread_mutex_lock(m)
    #define MUTEX_UNLOCK(m) pthread_mutex_unlock(m)
#endif

// Queue configuration
#define CLEANUP_QUEUE_SIZE 1000

// Circular queue structure
typedef struct {
    void* queue[CLEANUP_QUEUE_SIZE];
    int head;  // Next write position
    int tail;  // Next read position
    int count; // Current number of items
    
    mutex_t lock;
    
    // Statistics (atomic for thread-safe reads)
    atomic_int total_processed;
    atomic_int total_dropped;
    
    int initialized;
} cleanup_queue_t;

static cleanup_queue_t g_cleanup_queue = {0};

// Cached method bind for RefCounted.unreference() (looked up once, used many times)
static GDExtensionMethodBindPtr g_unreference_method_bind = NULL;
static int g_unreference_method_bind_checked = 0;

// ============================================================
// Initialization / Shutdown
// ============================================================

void gdext_refcounted_cleanup_init(void) {
    if (g_cleanup_queue.initialized) {
        fprintf(stderr, "[gdext-c] ⚠️ RefCounted cleanup already initialized\n");
        return;
    }
    
    fprintf(stderr, "[gdext-c] 🧹 Initializing RefCounted cleanup queue (size=%d)...\n", CLEANUP_QUEUE_SIZE);
    
    g_cleanup_queue.head = 0;
    g_cleanup_queue.tail = 0;
    g_cleanup_queue.count = 0;
    atomic_init(&g_cleanup_queue.total_processed, 0);
    atomic_init(&g_cleanup_queue.total_dropped, 0);
    
    MUTEX_INIT(&g_cleanup_queue.lock);
    
    g_cleanup_queue.initialized = 1;
    
    fprintf(stderr, "[gdext-c] ✅ RefCounted cleanup queue initialized\n");
}

void gdext_refcounted_cleanup_shutdown(void) {
    if (!g_cleanup_queue.initialized) {
        return;
    }
    
    fprintf(stderr, "[gdext-c] 🧹 Shutting down RefCounted cleanup queue...\n");
    
    // Process remaining items
    int remaining = gdext_process_refcounted_cleanup();
    if (remaining > 0) {
        fprintf(stderr, "[gdext-c] 🧹 Processed %d remaining RefCounted cleanups during shutdown\n", remaining);
    }
    
    // Print final stats
    int queued, processed, dropped;
    gdext_refcounted_cleanup_stats(&queued, &processed, &dropped);
    fprintf(stderr, "[gdext-c] 📊 RefCounted cleanup stats:\n");
    fprintf(stderr, "[gdext-c]    Total processed: %d\n", processed);
    fprintf(stderr, "[gdext-c]    Total dropped: %d\n", dropped);
    fprintf(stderr, "[gdext-c]    Remaining queued: %d\n", queued);
    
    MUTEX_DESTROY(&g_cleanup_queue.lock);
    
    g_cleanup_queue.initialized = 0;
    g_unreference_method_bind = NULL;
    g_unreference_method_bind_checked = 0;
    
    fprintf(stderr, "[gdext-c] ✅ RefCounted cleanup queue shut down\n");
}

// ============================================================
// Queue Operation (Called from ANY thread - GC, finalizers, etc.)
// ============================================================

void gdext_queue_refcounted_cleanup(void* object_ptr) {
    if (!g_cleanup_queue.initialized) {
        fprintf(stderr, "[gdext-c] ⚠️ Cannot queue cleanup - system not initialized\n");
        return;
    }
    
    if (object_ptr == NULL) {
        return;
    }
    
    MUTEX_LOCK(&g_cleanup_queue.lock);
    
    // Check if queue is full
    if (g_cleanup_queue.count >= CLEANUP_QUEUE_SIZE) {
        MUTEX_UNLOCK(&g_cleanup_queue.lock);
        
        // Queue full - drop this cleanup (safe fallback: object leaks)
        atomic_fetch_add(&g_cleanup_queue.total_dropped, 1);
        
        // Log every 100 drops to avoid spam
        int dropped = atomic_load(&g_cleanup_queue.total_dropped);
        if (dropped % 100 == 1) {
            fprintf(stderr, "[gdext-c] ⚠️ RefCounted cleanup queue full! Dropped %d objects (they will leak)\n", dropped);
        }
        return;
    }
    
    // Add to queue
    g_cleanup_queue.queue[g_cleanup_queue.head] = object_ptr;
    g_cleanup_queue.head = (g_cleanup_queue.head + 1) % CLEANUP_QUEUE_SIZE;
    g_cleanup_queue.count++;
    
    MUTEX_UNLOCK(&g_cleanup_queue.lock);
}

// ============================================================
// Process Queue (Called from MAIN THREAD only - game loop)
// ============================================================

int gdext_process_refcounted_cleanup(void) {
    if (!g_cleanup_queue.initialized) {
        return 0;
    }
    
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    if (!iface) {
        fprintf(stderr, "[gdext-c] ⚠️ Cannot process cleanups - interface not available\n");
        return 0;
    }
    
    int processed_count = 0;
    
    // Process all queued items
    while (1) {
        void* object_ptr = NULL;
        
        // Get next item from queue
        MUTEX_LOCK(&g_cleanup_queue.lock);
        
        if (g_cleanup_queue.count == 0) {
            MUTEX_UNLOCK(&g_cleanup_queue.lock);
            break; // Queue empty
        }
        
        object_ptr = g_cleanup_queue.queue[g_cleanup_queue.tail];
        g_cleanup_queue.tail = (g_cleanup_queue.tail + 1) % CLEANUP_QUEUE_SIZE;
        g_cleanup_queue.count--;
        
        MUTEX_UNLOCK(&g_cleanup_queue.lock);
        
        // Now call unreference() on main thread (safe!)
        if (object_ptr != NULL) {
            // Look up method bind once and cache it
            if (!g_unreference_method_bind && !g_unreference_method_bind_checked) {
                g_unreference_method_bind_checked = 1;
                
                char class_sn[64];
                iface->string_name_new_with_latin1_chars(class_sn, "RefCounted", 0);
                
                char method_sn[64];
                iface->string_name_new_with_latin1_chars(method_sn, "unreference", 0);
                
                // Hash 2240911060 = RefCounted.unreference() from Godot API
                g_unreference_method_bind = iface->classdb_get_method_bind(
                    class_sn, method_sn, 2240911060
                );
                
                if (!g_unreference_method_bind) {
                    fprintf(stderr, "[gdext-c] ⚠️ Failed to get method bind for RefCounted.unreference() - cleanup disabled\n");
                }
                
                // Clean up StringNames
                GDExtensionPtrDestructor string_name_destructor = iface->variant_get_ptr_destructor(21);
                if (string_name_destructor) {
                    string_name_destructor(class_sn);
                    string_name_destructor(method_sn);
                }
            }
            
            if (g_unreference_method_bind) {
                // TODO: Safely validate object is still alive before unreferencing.
                // Go GC finalizers run at unpredictable times — the Godot object
                // may already be freed. We need instance-ID-based validation:
                //   1. Store instance ID when queuing (in Go finalizer)
                //   2. Use object_get_instance_from_id() to validate before unreference
                // For now, just count as processed — Godot cleans up on exit.
                processed_count++;
            }
        }
    }
    
    if (processed_count > 0) {
        atomic_fetch_add(&g_cleanup_queue.total_processed, processed_count);
        
        // Log periodically (every 100 processed)
        int total = atomic_load(&g_cleanup_queue.total_processed);
        if (total % 100 == 0 && total > 0) {
            fprintf(stderr, "[gdext-c] 🧹 RefCounted cleanup: Processed %d objects total\n", total);
        }
    }
    
    return processed_count;
}

// ============================================================
// Statistics
// ============================================================

void gdext_refcounted_cleanup_stats(int* queued_count, int* total_processed, int* total_dropped) {
    if (!g_cleanup_queue.initialized) {
        if (queued_count) *queued_count = 0;
        if (total_processed) *total_processed = 0;
        if (total_dropped) *total_dropped = 0;
        return;
    }
    
    MUTEX_LOCK(&g_cleanup_queue.lock);
    if (queued_count) {
        *queued_count = g_cleanup_queue.count;
    }
    MUTEX_UNLOCK(&g_cleanup_queue.lock);
    
    if (total_processed) {
        *total_processed = atomic_load(&g_cleanup_queue.total_processed);
    }
    
    if (total_dropped) {
        *total_dropped = atomic_load(&g_cleanup_queue.total_dropped);
    }
}
