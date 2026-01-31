# GPU Threading Architecture for GDExtension

**Date:** 2026-01-29  
**Problem:** macOS Metal requires GPU initialization on main thread  
**Solution:** Thread-safe GPU operation queue in gdext-c (benefits ALL bindings)

---

## 🎯 Design Goals

1. **Universal:** Works for Go, Rust, C++, any language binding
2. **Transparent:** Existing code continues to work
3. **Safe:** No threading violations on macOS Metal
4. **Performant:** Minimal overhead on non-Apple platforms
5. **Simple:** Easy to integrate into existing systems

---

## 🏗️ Architecture Overview

```
Language Binding (Go/Rust/etc.)
        ↓
   gdext-c API call (e.g., create_buffer)
        ↓
   Check: Are we on main thread?
        ↓
   ┌─────────────────┬─────────────────┐
   │  YES (main)     │  NO (worker)    │
   │  Execute now ✅ │  Queue for later│
   └─────────────────┴─────────────────┘
                     ↓
            GPU Operation Queue (C)
                     ↓
         Process on main thread (idle)
                     ↓
              Result callback
                     ↓
          Language binding notified
```

---

## 📦 Components

### 1. Thread Detection (gdext_c_thread.h)

```c
/**
 * @file gdext_c_thread.h
 * @brief Thread context detection for GPU operations
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
 * Must be called from Godot's main thread during GDExtension init
 */
void gdext_thread_init(void);

/**
 * @brief Check if current thread is Godot's main thread
 * @return true if on main thread, false otherwise
 */
bool gdext_thread_is_main(void);

/**
 * @brief Get current thread ID
 * @return Platform-specific thread identifier
 */
uint64_t gdext_thread_current_id(void);

#ifdef __cplusplus
}
#endif

#endif // GDEXT_C_THREAD_H
```

**Implementation Strategy:**
- Store main thread ID during init
- Compare current thread ID to stored ID
- Use platform-specific APIs:
  - macOS: `pthread_self()`
  - Windows: `GetCurrentThreadId()`
  - Linux: `pthread_self()`

---

### 2. GPU Operation Queue (gdext_c_gpu_queue.h)

```c
/**
 * @file gdext_c_gpu_queue.h
 * @brief Thread-safe GPU operation queue for deferred execution
 */

#ifndef GDEXT_C_GPU_QUEUE_H
#define GDEXT_C_GPU_QUEUE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Opaque handle for a queued GPU operation
 */
typedef struct gdext_gpu_operation* gdext_gpu_operation_t;

/**
 * @brief Function pointer type for GPU operations
 * @param userdata Context passed during queue submission
 * @return Result pointer (operation-specific)
 */
typedef void* (*gdext_gpu_func_t)(void* userdata);

/**
 * @brief Callback for operation completion
 * @param result Result from GPU operation
 * @param userdata Original context
 */
typedef void (*gdext_gpu_callback_t)(void* result, void* userdata);

/**
 * @brief Initialize GPU operation queue
 */
void gdext_gpu_queue_init(void);

/**
 * @brief Shutdown GPU operation queue
 */
void gdext_gpu_queue_shutdown(void);

/**
 * @brief Queue a GPU operation for main thread execution
 * @param func GPU operation to execute
 * @param userdata Context for operation
 * @param callback Optional completion callback (can be NULL)
 * @return Operation handle (can be used to cancel/wait)
 */
gdext_gpu_operation_t gdext_gpu_queue_submit(
    gdext_gpu_func_t func,
    void* userdata,
    gdext_gpu_callback_t callback
);

/**
 * @brief Process queued GPU operations (call from main thread!)
 * Should be called every frame from main thread
 * @return Number of operations processed
 */
int gdext_gpu_queue_process(void);

/**
 * @brief Wait for a specific operation to complete
 * @param op Operation handle
 * @param timeout_ms Timeout in milliseconds (0 = no timeout)
 * @return true if completed, false if timeout
 */
bool gdext_gpu_queue_wait(gdext_gpu_operation_t op, int timeout_ms);

/**
 * @brief Execute immediately if on main thread, otherwise queue
 * This is the recommended way to call GPU operations!
 * @param func GPU operation
 * @param userdata Context
 * @return Result (NULL if queued)
 */
void* gdext_gpu_execute_safe(gdext_gpu_func_t func, void* userdata);

#ifdef __cplusplus
}
#endif

#endif // GDEXT_C_GPU_QUEUE_H
```

---

### 3. High-Level GPU Wrappers

**Instead of calling Godot API directly:**

```c
// ❌ OLD WAY (unsafe on non-main thread):
RenderingDevice* rd = get_rendering_device();
Buffer* buffer = rd->buffer_create(size, usage, data);

// ✅ NEW WAY (automatically thread-safe):
typedef struct {
    size_t size;
    int usage;
    void* data;
} BufferCreateArgs;

void* buffer_create_impl(void* userdata) {
    BufferCreateArgs* args = (BufferCreateArgs*)userdata;
    RenderingDevice* rd = get_rendering_device();
    return rd->buffer_create(args->size, args->usage, args->data);
}

Buffer* gdext_rd_buffer_create_safe(size_t size, int usage, void* data) {
    BufferCreateArgs args = { size, usage, data };
    return (Buffer*)gdext_gpu_execute_safe(buffer_create_impl, &args);
}
```

---

## 🔧 Integration Points

### For Language Bindings:

**1. Initialization (during GDExtension load):**
```c
void gdextension_initialize(const GDExtensionInterface* p_interface, 
                           GDExtensionClassLibraryPtr p_library) {
    // MUST call from main thread!
    gdext_thread_init();
    gdext_gpu_queue_init();
    
    // ... rest of initialization
}
```

**2. Per-Frame Update (from GameNode or SceneTree signal):**
```c
void on_process_frame(double delta) {
    // Process queued GPU operations on main thread
    int processed = gdext_gpu_queue_process();
    
    // ... rest of frame logic
}
```

**3. Shutdown:**
```c
void gdextension_deinitialize() {
    gdext_gpu_queue_shutdown();
    
    // ... rest of cleanup
}
```

---

### For Go Bindings:

**Option A: Transparent (no code changes):**
```go
// Existing code works unchanged!
buffer := renderingDevice.BufferCreate(size, usage, data)
// → Internally routes through gdext_gpu_execute_safe()
```

**Option B: Explicit (for performance-critical code):**
```go
// Queue operation explicitly
op := metal.QueueGPUOperation(func() interface{} {
    return renderingDevice.BufferCreate(size, usage, data)
})

// Wait for result
buffer := metal.Wait(op).(Buffer)
```

**Option C: Async (for non-blocking):**
```go
metal.QueueGPUOperationAsync(func() interface{} {
    return renderingDevice.BufferCreate(size, usage, data)
}, func(result interface{}) {
    buffer := result.(Buffer)
    // Use buffer...
})
```

---

## 💡 Implementation Strategy

### Phase 1: Core Infrastructure (2 hours)
1. Implement `gdext_c_thread.{h,c}` (thread detection)
2. Implement `gdext_c_gpu_queue.{h,c}` (operation queue)
3. Add mutex/lock primitives for thread safety
4. Integrate with GameNode's `_process` for queue flushing

### Phase 2: Wrapper Functions (3 hours)
1. Identify all GPU-touching Godot API calls
2. Create safe wrappers for:
   - RenderingDevice operations
   - Shader compilation
   - Buffer/texture creation
   - Compute pipeline setup
3. Add compile-time checks (detect unsafe calls)

### Phase 3: Go Integration (1 hour)
1. Expose queue API to Go via cgo
2. Update `gdext-go/pkg/metal` to use C queue
3. Make transparent: `classdb.RenderingDevice.BufferCreate()` auto-routes
4. Update documentation

### Phase 4: Testing (1 hour)
1. Test from multiple goroutines simultaneously
2. Verify no macOS Metal crashes
3. Benchmark overhead (should be <1% on non-queued path)
4. Test with SVO renderer

**Total Time:** ~7 hours for complete solution

---

## 🎯 Benefits

### For gdext-c Users:
- ✅ **Thread-safe by default** - Hard to use wrong
- ✅ **Cross-platform** - Works on all platforms
- ✅ **Zero overhead** on main thread (direct execution)
- ✅ **Minimal overhead** on worker threads (queue + callback)

### For Language Bindings:
- ✅ **Universal solution** - Rust, C++, Zig, etc. all benefit
- ✅ **Simple integration** - Just call `gdext_gpu_queue_process()` per frame
- ✅ **Flexible** - Sync or async execution models supported

### For Game Developers:
- ✅ **No macOS Metal crashes** - Ever!
- ✅ **Multi-threaded GPU** - Safe concurrent access
- ✅ **Predictable behavior** - Same on all platforms

---

## 🚨 Potential Challenges

### Challenge 1: Identifying ALL GPU Operations

**Problem:** Hard to know which Godot API calls touch GPU  
**Solution:** 
- Document known GPU-touching classes (RenderingDevice, Shader, etc.)
- Add runtime assertions in debug builds
- Create allowlist of safe classes

### Challenge 2: Data Lifetime

**Problem:** Queued operations may reference freed memory  
**Solution:**
- Copy data in queue submission
- Provide `gdext_gpu_queue_submit_with_cleanup()`
- Use reference counting for complex data

### Challenge 3: Synchronous APIs

**Problem:** Some code expects immediate results  
**Solution:**
- `gdext_gpu_execute_safe()` blocks if not on main thread
- Returns immediately if on main thread
- Provides `gdext_gpu_try_execute()` for non-blocking checks

---

## 📚 Prior Art

### godot-cpp Approach:
- No explicit threading solution
- Assumes single-threaded usage
- Developers responsible for thread safety

### godot-rust Approach:
- Uses Rust's type system for thread safety
- `Send` + `Sync` traits prevent incorrect usage
- Still has runtime checks for Godot calls

### SwiftGodot Approach:
- Uses `@MainActor` attribute
- Compile-time enforcement
- macOS-specific solution

### Our Advantage:
- **Language-agnostic** C solution
- Benefits all bindings
- Runtime + optional compile-time checks
- Works on all platforms

---

## 🎯 Success Criteria

1. ✅ No macOS Metal crashes during concurrent GPU init
2. ✅ SVO renderer works from background goroutine
3. ✅ <5% performance overhead in worst case
4. ✅ <1% overhead when called from main thread
5. ✅ Other bindings (hypothetical Rust binding) can use same infrastructure
6. ✅ Existing Go code works without changes (transparent mode)

---

## 📝 Next Steps

1. Create `gdext-c/src/threading/` directory
2. Implement `gdext_c_thread.c` (thread detection)
3. Implement `gdext_c_gpu_queue.c` (operation queue)
4. Update GameNode to call `gdext_gpu_queue_process()` each frame
5. Create safe wrappers for RenderingDevice operations
6. Update Go bindings to use C queue
7. Test with SVORenderer from background goroutine

---

**Decision:** Proceed with this architecture?  
**Estimated Time:** ~7 hours for complete solution  
**Benefit:** Solves Metal threading for ALL current and future language bindings!


