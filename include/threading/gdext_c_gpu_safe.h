/**
 * @file gdext_c_gpu_safe.h
 * @brief Thread-safe wrappers for GPU operations
 * 
 * TDD Phase 3: Safe wrappers for RenderingDevice and GPU-touching APIs
 * Automatically routes through GPU queue if called from worker thread
 * 
 * @author gdext-c contributors
 * @date 2026-01-29
 */

#ifndef GDEXT_C_GPU_SAFE_H
#define GDEXT_C_GPU_SAFE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef void* gdext_c_object_t;

/**
 * @brief Check if RenderingDevice is ready for GPU operations
 * 
 * Safe to call from any thread.
 * Should be called before attempting GPU operations.
 * 
 * @return true if RenderingDevice is initialized and ready
 */
bool gdext_gpu_is_rendering_device_ready(void);

/**
 * @brief Get RenderingDevice singleton (thread-safe)
 * 
 * Automatically queues if called from worker thread.
 * 
 * @return RenderingDevice singleton (NULL if not ready)
 */
gdext_c_object_t gdext_gpu_get_rendering_device(void);

/**
 * @brief Create buffer (thread-safe)
 * 
 * Safe wrapper for RenderingDevice.buffer_create()
 * Automatically queues if called from worker thread.
 * 
 * @param size_bytes Buffer size in bytes
 * @param usage Buffer usage flags (RenderingDevice.BufferUsageBits)
 * @param data Initial data (can be NULL)
 * @return RID of created buffer (or invalid RID on failure)
 * 
 * @note If data is provided, it will be copied before queueing
 */
uint64_t gdext_gpu_buffer_create_safe(size_t size_bytes, uint32_t usage, const void* data);

/**
 * @brief Update buffer (thread-safe)
 * 
 * Safe wrapper for RenderingDevice.buffer_update()
 * Automatically queues if called from worker thread.
 * 
 * @param buffer_rid RID of buffer to update
 * @param offset Offset in bytes
 * @param size_bytes Number of bytes to update
 * @param data Source data
 * @return true on success
 * 
 * @note Data will be copied before queueing
 */
bool gdext_gpu_buffer_update_safe(uint64_t buffer_rid, size_t offset, size_t size_bytes, const void* data);

/**
 * @brief Create shader from SPIR-V (thread-safe)
 * 
 * Safe wrapper for RenderingDevice.shader_create_from_spirv()
 * Automatically queues if called from worker thread.
 * 
 * @param spirv_data Array of RDShaderSPIRV objects
 * @param spirv_count Number of shader stages
 * @return RID of created shader (or invalid RID on failure)
 */
uint64_t gdext_gpu_shader_create_from_spirv_safe(gdext_c_object_t* spirv_data, size_t spirv_count);

/**
 * @brief Create uniform set (thread-safe)
 * 
 * Safe wrapper for RenderingDevice.uniform_set_create()
 * Automatically queues if called from worker thread.
 * 
 * @param uniforms Array of RDUniform objects
 * @param uniform_count Number of uniforms
 * @param shader_rid Shader RID
 * @param set_index Uniform set index
 * @return RID of created uniform set (or invalid RID on failure)
 */
uint64_t gdext_gpu_uniform_set_create_safe(
    gdext_c_object_t* uniforms,
    size_t uniform_count,
    uint64_t shader_rid,
    uint32_t set_index
);

/**
 * @brief Create compute pipeline (thread-safe)
 * 
 * Safe wrapper for RenderingDevice.compute_pipeline_create()
 * Automatically queues if called from worker thread.
 * 
 * @param shader_rid Shader RID
 * @return RID of created compute pipeline (or invalid RID on failure)
 */
uint64_t gdext_gpu_compute_pipeline_create_safe(uint64_t shader_rid);

/**
 * @brief Create texture (thread-safe)
 * 
 * Safe wrapper for RenderingDevice.texture_create()
 * Automatically queues if called from worker thread.
 * 
 * @param format Texture format
 * @param view Texture view
 * @param data Texture data (can be NULL)
 * @return RID of created texture (or invalid RID on failure)
 */
uint64_t gdext_gpu_texture_create_safe(
    gdext_c_object_t format,
    gdext_c_object_t view,
    gdext_c_object_t data
);

/**
 * @brief Free RID (thread-safe)
 * 
 * Safe wrapper for RenderingDevice.free_rid()
 * Automatically queues if called from worker thread.
 * 
 * @param rid RID to free
 */
void gdext_gpu_free_rid_safe(uint64_t rid);

/**
 * @brief Submit GPU work (thread-safe)
 * 
 * Safe wrapper for RenderingDevice.submit()
 * Automatically queues if called from worker thread.
 * 
 * @return true on success
 */
bool gdext_gpu_submit_safe(void);

/**
 * @brief Sync GPU (thread-safe)
 * 
 * Safe wrapper for RenderingDevice.sync()
 * Automatically queues if called from worker thread.
 */
void gdext_gpu_sync_safe(void);

/**
 * @brief Initialize GPU safe wrappers
 * 
 * Must be called during GDExtension initialization.
 * Caches RenderingDevice singleton for fast access.
 */
void gdext_gpu_safe_init(void);

/**
 * @brief Shutdown GPU safe wrappers
 * 
 * Cleans up cached references.
 */
void gdext_gpu_safe_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif // GDEXT_C_GPU_SAFE_H

