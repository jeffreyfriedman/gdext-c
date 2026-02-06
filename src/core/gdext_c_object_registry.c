#include "gdext_c_object_registry.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Platform-specific synchronization (same as gdext_c_gpu_queue.c)
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

// Simple hash table for object tracking
#define REGISTRY_SIZE 4096
#define MAX_OBJECTS 8192

typedef struct registry_entry {
    gdext_object_info_t info;
    struct registry_entry* next;
} registry_entry_t;

static struct {
    registry_entry_t* buckets[REGISTRY_SIZE];
    int total_objects;
    int active_objects;
    bool enabled;
    bool initialized;
    mutex_t lock;
} g_registry = {0};

// Hash function for pointer addresses
static uint32_t hash_ptr(void* ptr) {
    uintptr_t addr = (uintptr_t)ptr;
    return (uint32_t)((addr >> 4) % REGISTRY_SIZE);
}

void gdext_registry_init(void) {
    if (g_registry.initialized) {
        return;
    }
    
    memset(&g_registry, 0, sizeof(g_registry));
    MUTEX_INIT(&g_registry.lock);
    g_registry.enabled = true;
    g_registry.initialized = true;
    
    printf("[gdext-c] 📋 Object registry initialized\n");
    fflush(stdout);
}

void gdext_registry_shutdown(void) {
    if (!g_registry.initialized) {
        return;
    }
    
    MUTEX_LOCK(&g_registry.lock);
    
    printf("[gdext-c] 📋 Object registry shutting down...\n");
    fflush(stdout);
    
    // Find and report leaks
    int leaks = gdext_registry_find_leaks(0); // All active objects are leaks
    
    // Free all entries
    for (int i = 0; i < REGISTRY_SIZE; i++) {
        registry_entry_t* entry = g_registry.buckets[i];
        while (entry != NULL) {
            registry_entry_t* next = entry->next;
            free(entry);
            entry = next;
        }
        g_registry.buckets[i] = NULL;
    }
    
    MUTEX_UNLOCK(&g_registry.lock);
    MUTEX_DESTROY(&g_registry.lock);
    
    g_registry.initialized = false;
    
    printf("[gdext-c] ✅ Object registry shutdown complete (%d leaks found)\n", leaks);
    fflush(stdout);
}

void gdext_registry_enable(void) {
    g_registry.enabled = true;
}

void gdext_registry_disable(void) {
    g_registry.enabled = false;
}

void gdext_registry_track_object(void* ptr, const char* class_name, bool is_ref_counted) {
    if (!g_registry.initialized || !g_registry.enabled || ptr == NULL) {
        return;
    }
    
    MUTEX_LOCK(&g_registry.lock);
    
    uint32_t hash = hash_ptr(ptr);
    
    // Check if already tracked
    registry_entry_t* entry = g_registry.buckets[hash];
    while (entry != NULL) {
        if (entry->info.ptr == ptr) {
            fprintf(stderr, "[gdext-c] ⚠️ Object %p (%s) already tracked!\n", ptr, class_name);
            fflush(stderr);
            MUTEX_UNLOCK(&g_registry.lock);
            return;
        }
        entry = entry->next;
    }
    
    // Create new entry
    registry_entry_t* new_entry = (registry_entry_t*)malloc(sizeof(registry_entry_t));
    if (new_entry == NULL) {
        fprintf(stderr, "[gdext-c] ❌ Failed to allocate registry entry!\n");
        fflush(stderr);
        MUTEX_UNLOCK(&g_registry.lock);
        return;
    }
    
    memset(new_entry, 0, sizeof(registry_entry_t));
    new_entry->info.ptr = ptr;
    strncpy(new_entry->info.class_name, class_name, sizeof(new_entry->info.class_name) - 1);
    new_entry->info.is_ref_counted = is_ref_counted;
    new_entry->info.ref_count = is_ref_counted ? 1 : 0;
    new_entry->info.created_at = time(NULL);
    new_entry->info.queued_for_deletion = false;
    new_entry->info.deleted = false;
    
    // Add to bucket
    new_entry->next = g_registry.buckets[hash];
    g_registry.buckets[hash] = new_entry;
    
    g_registry.total_objects++;
    g_registry.active_objects++;
    
    MUTEX_UNLOCK(&g_registry.lock);
}

void gdext_registry_mark_queued(void* ptr) {
    if (!g_registry.initialized || !g_registry.enabled || ptr == NULL) {
        return;
    }
    
    MUTEX_LOCK(&g_registry.lock);
    
    uint32_t hash = hash_ptr(ptr);
    registry_entry_t* entry = g_registry.buckets[hash];
    
    while (entry != NULL) {
        if (entry->info.ptr == ptr) {
            entry->info.queued_for_deletion = true;
            MUTEX_UNLOCK(&g_registry.lock);
            return;
        }
        entry = entry->next;
    }
    
    fprintf(stderr, "[gdext-c] ⚠️ Attempted to mark unknown object %p as queued\n", ptr);
    fflush(stderr);
    
    MUTEX_UNLOCK(&g_registry.lock);
}

void gdext_registry_mark_deleted(void* ptr) {
    if (!g_registry.initialized || !g_registry.enabled || ptr == NULL) {
        return;
    }
    
    MUTEX_LOCK(&g_registry.lock);
    
    uint32_t hash = hash_ptr(ptr);
    registry_entry_t* entry = g_registry.buckets[hash];
    
    while (entry != NULL) {
        if (entry->info.ptr == ptr) {
            entry->info.deleted = true;
            g_registry.active_objects--;
            MUTEX_UNLOCK(&g_registry.lock);
            return;
        }
        entry = entry->next;
    }
    
    fprintf(stderr, "[gdext-c] ⚠️ Attempted to mark unknown object %p as deleted\n", ptr);
    fflush(stderr);
    
    MUTEX_UNLOCK(&g_registry.lock);
}

int gdext_registry_check_valid(void* ptr, const char* operation, char* error_buf, size_t error_buf_size) {
    if (!g_registry.initialized || !g_registry.enabled) {
        return 0; // Assume valid if tracking disabled
    }
    
    if (ptr == NULL) {
        snprintf(error_buf, error_buf_size, "NULL pointer passed to %s", operation);
        return 1;
    }
    
    MUTEX_LOCK(&g_registry.lock);
    
    uint32_t hash = hash_ptr(ptr);
    registry_entry_t* entry = g_registry.buckets[hash];
    
    while (entry != NULL) {
        if (entry->info.ptr == ptr) {
            if (entry->info.deleted) {
                snprintf(error_buf, error_buf_size,
                    "USE-AFTER-FREE DETECTED: %s on deleted object %p (class: %s)",
                    operation, ptr, entry->info.class_name);
                MUTEX_UNLOCK(&g_registry.lock);
                return 1;
            }
            if (entry->info.queued_for_deletion) {
                snprintf(error_buf, error_buf_size,
                    "USE-AFTER-QUEUE_FREE DETECTED: %s on queued object %p (class: %s)",
                    operation, ptr, entry->info.class_name);
                MUTEX_UNLOCK(&g_registry.lock);
                return 1;
            }
            MUTEX_UNLOCK(&g_registry.lock);
            return 0; // Valid
        }
        entry = entry->next;
    }
    
    // Object not found in registry - might be created externally by Godot
    MUTEX_UNLOCK(&g_registry.lock);
    return 0; // Assume valid (external object)
}

void gdext_registry_increment_refcount(void* ptr) {
    if (!g_registry.initialized || !g_registry.enabled || ptr == NULL) {
        return;
    }
    
    MUTEX_LOCK(&g_registry.lock);
    
    uint32_t hash = hash_ptr(ptr);
    registry_entry_t* entry = g_registry.buckets[hash];
    
    while (entry != NULL) {
        if (entry->info.ptr == ptr) {
            if (entry->info.is_ref_counted) {
                entry->info.ref_count++;
            }
            MUTEX_UNLOCK(&g_registry.lock);
            return;
        }
        entry = entry->next;
    }
    
    MUTEX_UNLOCK(&g_registry.lock);
}

void gdext_registry_decrement_refcount(void* ptr) {
    if (!g_registry.initialized || !g_registry.enabled || ptr == NULL) {
        return;
    }
    
    MUTEX_LOCK(&g_registry.lock);
    
    uint32_t hash = hash_ptr(ptr);
    registry_entry_t* entry = g_registry.buckets[hash];
    
    while (entry != NULL) {
        if (entry->info.ptr == ptr) {
            if (entry->info.is_ref_counted) {
                entry->info.ref_count--;
                if (entry->info.ref_count <= 0) {
                    entry->info.deleted = true;
                    g_registry.active_objects--;
                }
            }
            MUTEX_UNLOCK(&g_registry.lock);
            return;
        }
        entry = entry->next;
    }
    
    MUTEX_UNLOCK(&g_registry.lock);
}

void gdext_registry_get_stats(gdext_registry_stats_t* stats) {
    if (!g_registry.initialized || stats == NULL) {
        return;
    }
    
    MUTEX_LOCK(&g_registry.lock);
    
    memset(stats, 0, sizeof(gdext_registry_stats_t));
    stats->total_objects = g_registry.total_objects;
    stats->active_objects = g_registry.active_objects;
    
    // Count deleted, queued, and refcounted
    for (int i = 0; i < REGISTRY_SIZE; i++) {
        registry_entry_t* entry = g_registry.buckets[i];
        while (entry != NULL) {
            if (entry->info.deleted) {
                stats->deleted_objects++;
            }
            if (entry->info.queued_for_deletion) {
                stats->queued_objects++;
            }
            if (entry->info.is_ref_counted) {
                stats->ref_counted_objects++;
            }
            entry = entry->next;
        }
    }
    
    MUTEX_UNLOCK(&g_registry.lock);
}

void gdext_registry_print_stats(void) {
    gdext_registry_stats_t stats;
    gdext_registry_get_stats(&stats);
    
    printf("╔═══════════════════════════════════════════════════════════════╗\n");
    printf("║               OBJECT REGISTRY STATISTICS                     ║\n");
    printf("╠═══════════════════════════════════════════════════════════════╣\n");
    printf("║ Total objects created:     %6d                           ║\n", stats.total_objects);
    printf("║ Active objects:            %6d                           ║\n", stats.active_objects);
    printf("║ Deleted objects:           %6d                           ║\n", stats.deleted_objects);
    printf("║ Queued for deletion:       %6d                           ║\n", stats.queued_objects);
    printf("║ RefCounted objects:        %6d                           ║\n", stats.ref_counted_objects);
    printf("╚═══════════════════════════════════════════════════════════════╝\n");
    fflush(stdout);
}

int gdext_registry_find_leaks(int threshold_seconds) {
    if (!g_registry.initialized) {
        return 0;
    }
    
    MUTEX_LOCK(&g_registry.lock);
    
    time_t now = time(NULL);
    int leak_count = 0;
    
    printf("[gdext-c] 🔍 Scanning for memory leaks (threshold: %ds)...\n", threshold_seconds);
    fflush(stdout);
    
    for (int i = 0; i < REGISTRY_SIZE; i++) {
        registry_entry_t* entry = g_registry.buckets[i];
        while (entry != NULL) {
            if (!entry->info.deleted && !entry->info.queued_for_deletion) {
                int age = (int)difftime(now, entry->info.created_at);
                if (age >= threshold_seconds) {
                    leak_count++;
                    if (leak_count <= 10) { // Only print first 10
                        printf("[gdext-c] 💧 LEAK: Object %p (%s) alive for %ds, refcount=%d\n",
                            entry->info.ptr, entry->info.class_name, age, entry->info.ref_count);
                        fflush(stdout);
                    }
                }
            }
            entry = entry->next;
        }
    }
    
    if (leak_count > 10) {
        printf("[gdext-c] ... and %d more leaks (not shown)\n", leak_count - 10);
        fflush(stdout);
    }
    
    MUTEX_UNLOCK(&g_registry.lock);
    
    return leak_count;
}
