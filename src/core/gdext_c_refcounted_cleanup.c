#include "gdext_c_refcounted_cleanup.h"
#include "gdext_c_core.h"
#include "gdext_c_generated.h"  // For gdext_ref_counted_unreference() - CORRECT hash from codegen!
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

// Queue entry: stores both object pointer AND instance ID for validation
typedef struct {
    void*    object_ptr;
    uint64_t instance_id;  // 0 = no validation (legacy), >0 = validate before unreference
} cleanup_entry_t;

// Circular queue structure
typedef struct {
    cleanup_entry_t queue[CLEANUP_QUEUE_SIZE];
    int head;  // Next write position
    int tail;  // Next read position
    int count; // Current number of items
    
    mutex_t lock;
    
    // Statistics (atomic for thread-safe reads)
    atomic_int total_processed;
    atomic_int total_dropped;
    atomic_int total_skipped_dead;  // Objects already freed by Godot
    
    int initialized;
} cleanup_queue_t;

static cleanup_queue_t g_cleanup_queue = {0};

// ============================================================
// Initialization / Shutdown
// ============================================================

void gdext_refcounted_cleanup_init(void) {
    if (g_cleanup_queue.initialized) {
        return;
    }
    
    g_cleanup_queue.head = 0;
    g_cleanup_queue.tail = 0;
    g_cleanup_queue.count = 0;
    atomic_init(&g_cleanup_queue.total_processed, 0);
    atomic_init(&g_cleanup_queue.total_dropped, 0);
    atomic_init(&g_cleanup_queue.total_skipped_dead, 0);
    
    MUTEX_INIT(&g_cleanup_queue.lock);
    
    g_cleanup_queue.initialized = 1;
    
    fprintf(stderr, "[gdext-c] ✅ RefCounted cleanup queue initialized (size=%d, instance-ID validation enabled)\n", CLEANUP_QUEUE_SIZE);
}

void gdext_refcounted_cleanup_shutdown(void) {
    if (!g_cleanup_queue.initialized) {
        return;
    }
    
    // Process remaining items
    int remaining = gdext_process_refcounted_cleanup();
    
    // Print final stats
    int queued, processed, dropped;
    gdext_refcounted_cleanup_stats(&queued, &processed, &dropped);
    int skipped = atomic_load(&g_cleanup_queue.total_skipped_dead);
    fprintf(stderr, "[gdext-c] 📊 RefCounted cleanup final stats: processed=%d, skipped_dead=%d, dropped=%d, remaining=%d\n",
            processed, skipped, dropped, remaining);
    
    MUTEX_DESTROY(&g_cleanup_queue.lock);
    g_cleanup_queue.initialized = 0;
}

// ============================================================
// Queue Operation (Called from ANY thread - GC, finalizers, etc.)
// ============================================================

// Internal helper to enqueue
static void enqueue_cleanup(void* object_ptr, uint64_t instance_id) {
    if (!g_cleanup_queue.initialized || object_ptr == NULL) {
        return;
    }
    
    MUTEX_LOCK(&g_cleanup_queue.lock);
    
    if (g_cleanup_queue.count >= CLEANUP_QUEUE_SIZE) {
        MUTEX_UNLOCK(&g_cleanup_queue.lock);
        
        int dropped = atomic_fetch_add(&g_cleanup_queue.total_dropped, 1) + 1;
        if (dropped % 100 == 1) {
            fprintf(stderr, "[gdext-c] ⚠️ RefCounted cleanup queue full! Dropped %d objects (they will leak)\n", dropped);
        }
        return;
    }
    
    g_cleanup_queue.queue[g_cleanup_queue.head].object_ptr = object_ptr;
    g_cleanup_queue.queue[g_cleanup_queue.head].instance_id = instance_id;
    g_cleanup_queue.head = (g_cleanup_queue.head + 1) % CLEANUP_QUEUE_SIZE;
    g_cleanup_queue.count++;
    
    MUTEX_UNLOCK(&g_cleanup_queue.lock);
}

void gdext_queue_refcounted_cleanup(void* object_ptr) {
    enqueue_cleanup(object_ptr, 0);  // 0 = legacy, no validation
}

void gdext_queue_refcounted_cleanup_with_id(void* object_ptr, uint64_t instance_id) {
    enqueue_cleanup(object_ptr, instance_id);
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
        return 0;
    }
    
    int processed_count = 0;
    int skipped_count = 0;
    
    while (1) {
        cleanup_entry_t entry = {0};
        
        // Get next item from queue (under lock)
        MUTEX_LOCK(&g_cleanup_queue.lock);
        
        if (g_cleanup_queue.count == 0) {
            MUTEX_UNLOCK(&g_cleanup_queue.lock);
            break;
        }
        
        entry = g_cleanup_queue.queue[g_cleanup_queue.tail];
        g_cleanup_queue.tail = (g_cleanup_queue.tail + 1) % CLEANUP_QUEUE_SIZE;
        g_cleanup_queue.count--;
        
        MUTEX_UNLOCK(&g_cleanup_queue.lock);
        
        if (entry.object_ptr == NULL) {
            continue;
        }
        
        // Validate object is still alive using instance ID
        if (entry.instance_id != 0) {
            // Use object_get_instance_from_id to check if Godot still knows about this object.
            // If it returns NULL, Godot already freed it — skip silently.
            GDExtensionObjectPtr validated = iface->object_get_instance_from_id(entry.instance_id);
            if (validated == NULL) {
                // Object already freed by Godot — this is normal and expected.
                skipped_count++;
                continue;
            }
            // Use the validated pointer (guaranteed alive) instead of the potentially-stale one
            entry.object_ptr = validated;
        }
        
        // Call the GENERATED unreference function — correct hash guaranteed by codegen!
        // No hand-written method bind lookup needed.
        gdext_ref_counted_unreference((GDExtensionObjectPtr)entry.object_ptr);
        processed_count++;
    }
    
    if (processed_count > 0) {
        atomic_fetch_add(&g_cleanup_queue.total_processed, processed_count);
    }
    if (skipped_count > 0) {
        atomic_fetch_add(&g_cleanup_queue.total_skipped_dead, skipped_count);
    }
    
    // Periodic summary log (every 500 total processed)
    int total = atomic_load(&g_cleanup_queue.total_processed);
    if (total > 0 && total % 500 == 0) {
        int skipped_total = atomic_load(&g_cleanup_queue.total_skipped_dead);
        fprintf(stderr, "[gdext-c] 🧹 RefCounted cleanup: processed=%d, skipped_dead=%d\n", total, skipped_total);
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
