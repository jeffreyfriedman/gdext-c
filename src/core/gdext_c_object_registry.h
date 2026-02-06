#ifndef GDEXT_C_OBJECT_REGISTRY_H
#define GDEXT_C_OBJECT_REGISTRY_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// Object tracking for detecting use-after-free and memory leaks
// This tracks ALL objects created through gdext-c, providing:
// 1. Use-after-free detection
// 2. Use-after-queue_free detection
// 3. Reference counting for RefCounted objects
// 4. Memory leak detection

typedef struct {
    void* ptr;
    char class_name[64];
    bool is_ref_counted;
    int32_t ref_count;
    time_t created_at;
    bool queued_for_deletion;
    bool deleted;
} gdext_object_info_t;

// Initialize the registry (called once at startup)
void gdext_registry_init(void);

// Shutdown the registry (called once at shutdown)
void gdext_registry_shutdown(void);

// Enable/disable tracking (for performance tuning)
void gdext_registry_enable(void);
void gdext_registry_disable(void);

// Track object creation
void gdext_registry_track_object(void* ptr, const char* class_name, bool is_ref_counted);

// Mark object as queued for deletion (queue_free called)
void gdext_registry_mark_queued(void* ptr);

// Mark object as actually deleted
void gdext_registry_mark_deleted(void* ptr);

// Check if object is valid (returns 0 if valid, 1 if error)
// If invalid, writes error message to error_buf
int gdext_registry_check_valid(void* ptr, const char* operation, char* error_buf, size_t error_buf_size);

// Increment/decrement reference count for RefCounted objects
void gdext_registry_increment_refcount(void* ptr);
void gdext_registry_decrement_refcount(void* ptr);

// Get statistics
typedef struct {
    int total_objects;
    int active_objects;
    int deleted_objects;
    int queued_objects;
    int ref_counted_objects;
} gdext_registry_stats_t;

void gdext_registry_get_stats(gdext_registry_stats_t* stats);
void gdext_registry_print_stats(void);

// Find potential memory leaks (objects older than threshold)
int gdext_registry_find_leaks(int threshold_seconds);

#ifdef __cplusplus
}
#endif

#endif // GDEXT_C_OBJECT_REGISTRY_H
