# Godot Lifecycle and GDExtension Integration

**Target Audience**: Developers creating language bindings on top of gdext-c (Go, Ruby, Python, etc.)

---

## 🎯 **Purpose**

This document explains Godot's engine lifecycle and how GDExtension code should integrate with it to avoid common pitfalls like race conditions, crashes, and undefined behavior.

---

## 📊 **Godot Engine Initialization Phases**

When Godot loads a GDExtension, it goes through several initialization phases:

```
┌─────────────────────────────────────────────────────┐
│ 1. EXTENSION LOAD                                   │
│    - Your extension library (.so/.dylib/.dll) loads │
│    - gdextension_initialization() called            │
│    - Core API functions available                   │
│    Status: ❌ DON'T create visual nodes            │
├─────────────────────────────────────────────────────┤
│ 2. SCENE TREE INITIALIZATION                        │
│    - Main scene being set up                        │
│    - Scene nodes being added                        │
│    - Systems calling Initialize()                   │
│    Status: ⚠️  DANGER ZONE (see below)             │
├─────────────────────────────────────────────────────┤
│ 3. ENGINE READY                                     │
│    - All initialization complete                    │
│    - First Update() /loop begins                   │
│    Status: ✅ SAFE to create all visuals           │
├─────────────────────────────────────────────────────┤
│ 4. NORMAL OPERATION                                 │
│    - Update() called each frame                     │
│    - Physics process running                        │
│    Status: ✅ Normal operation                      │
└─────────────────────────────────────────────────────┘
```

---

## ⚠️ **The Danger Zone: Phase 2**

### **Problem**: Race Conditions During Init

During **Phase 2 (Scene Tree Initialization)**:
- Multiple systems may try to create resources simultaneously
- On **macOS**, Metal/Objective-C has strict class initialization rules
- Creating many visual nodes can trigger: `objc[PID]: thread is already initializing this class!`
- This manifests as: `signal: abort trap`

### **Why This Happens**

macOS Objective-C runtime enforces that a class can only be initialized by ONE thread at a time. When multiple systems create Metal-backed resources (MeshInstance3D, Lights, Particles) during Initialize():

1. System A starts creating MeshInstance3D → initializes Metal class
2. System B starts creating OmniLight3D → tries to initialize same Metal class
3. macOS detects reentrancy → **CRASHES**

This is a **platform-specific manifestation** of general timing issues. Other platforms may work but are still fragile.

---

## ✅ **The Solution: Deferred Visual Creation**

### **Pattern**: Defer ALL Visual Node Creation

```c
typedef struct {
    bool visuals_initialized;
    void* cached_scene;
    void* cached_world;
} MySystem;

/**
 * Initialize() - Called during Phase 2 (DANGER ZONE)
 * 
 * DO:
 * - Set up data structures
 * - Cache scene/world pointers
 * - Initialize logic-only components
 * 
 * DON'T:
 * - Create MeshInstance3D, Lights, Particles
 * - Call add_child() extensively
 * - Create GPU resources
 */
void my_system_initialize(MySystem* sys, void* scene, void* world) {
    // Data structures ONLY
    sys->visuals_initialized = false;
    sys->cached_scene = scene;
    sys->cached_world = world;
    
    // Logic setup is fine
    initialize_game_logic(sys);
}

/**
 * Update() - Called during Phase 3+ (SAFE)
 * 
 * First call: Engine is ready, create visuals now
 * Subsequent calls: Normal operation
 */
void my_system_update(MySystem* sys, double delta) {
    // Lazy initialization on first update
    if (!sys->visuals_initialized && sys->cached_scene) {
        create_all_visuals(sys);
        sys->visuals_initialized = true;
    }
    
    // Normal update logic
    update_game_logic(sys, delta);
}
```

---

## 🎨 **What Counts as "Visual Creation"?**

**Must be deferred** (Phase 3+):
- ✅ `MeshInstance3D`, `Sprite3D`, `Label3D`
- ✅ `OmniLight3D`, `DirectionalLight3D`, `SpotLight3D`
- ✅ `CPUParticles3D`, `GPUParticles3D`
- ✅ `StaticBody3D` + `CollisionShape3D` (for visuals)
- ✅ `Camera3D` (if creating dynamically)
- ✅ Calling `add_child()` > 10-20 times
- ✅ Creating `ShaderMaterial`, `StandardMaterial3D`
- ✅ GPU resource creation (RenderingDevice, textures)

**Safe during Initialize()** (Phase 2):
- ✅ Creating ECS entities/components
- ✅ Initializing game state
- ✅ Loading data from files (configs, saves)
- ✅ Setting up logic systems
- ✅ Creating a FEW nodes (<10) if absolutely needed

---

## 🔍 **Detecting Lifecycle Issues**

### **Symptoms**

Your extension might have lifecycle issues if:
- ❌ Crashes on **macOS** but works on Linux/Windows
- ❌ Error: `objc[PID]: thread is already initializing this class!`
- ❌ `signal: abort trap` during initialization
- ❌ Crashes when creating 50+ nodes in Initialize()
- ❌ Works with small number of entities, crashes with many

### **Diagnosis**

```bash
# Check initialization crash
$ lldb ./godot -- --path /path/to/project
(lldb) run
# If crashes during init, check backtrace
(lldb) bt

# Common pattern: crash in class_initialize or similar
```

### **Fix**

Apply the deferred initialization pattern shown above to ALL systems that create visual nodes.

---

## 📋 **Best Practices Checklist**

### **For System Developers**

```c
// System structure
typedef struct {
    // ✅ GOOD: Flags and cached references
    bool visuals_initialized;
    void* cached_scene;
    
    // ✅ GOOD: Data structures
    int entity_count;
    MyData* data_array;
    
    // ❌ BAD: Direct node references created in init
    // void* mesh_instance; // Create in Update() instead!
} MySystem;

// Initialize (Phase 2)
void initialize(MySystem* sys, void* scene) {
    // ✅ Cache references
    sys->cached_scene = scene;
    
    // ✅ Set up data
    sys->data_array = malloc(sizeof(MyData) * 100);
    
    // ❌ DON'T create visuals here!
    // sys->mesh_instance = create_mesh_instance_3d(); // NO!
}

// Update (Phase 3+)
void update(MySystem* sys, double delta) {
    // ✅ Lazy init on first frame
    if (!sys->visuals_initialized) {
        // NOW it's safe to create visuals
        create_all_visual_nodes(sys);
        sys->visuals_initialized = true;
    }
    
    // Normal logic
}
```

### **For Language Binding Developers**

When wrapping gdext-c for your language:
1. ✅ Document this lifecycle pattern prominently
2. ✅ Provide helper functions for deferred initialization
3. ✅ Show examples in tutorials
4. ✅ Warn in docs about Initialize() vs Update()
5. ✅ Consider providing DeferredInitializer helper class

---

## 🚀 **Example: Particle Pool (Real-World Case)**

### **Before** (Crashes on macOS):

```c
ParticlePool* particle_pool_new(void* scene) {
    ParticlePool* pool = malloc(sizeof(ParticlePool));
    
    // ❌ BAD: Creating 100 particles during init
    for (int i = 0; i < 100; i++) {
        void* particle = create_cpu_particles_3d();
        add_child_safe(scene, particle);
    }
    
    return pool; // CRASHES on macOS!
}
```

### **After** (Works Everywhere):

```c
ParticlePool* particle_pool_new(void* scene) {
    ParticlePool* pool = malloc(sizeof(ParticlePool));
    pool->scene = scene;
    pool->visuals_created = false; // Flag for lazy init
    return pool;
}

void particle_pool_ensure_visuals(ParticlePool* pool) {
    if (pool->visuals_created) return;
    
    // ✅ GOOD: Create visuals on first use
    for (int i = 0; i < 100; i++) {
        void* particle = create_cpu_particles_3d();
        add_child_safe(pool->scene, particle);
    }
    
    pool->visuals_created = true;
}

void* particle_pool_acquire(ParticlePool* pool) {
    particle_pool_ensure_visuals(pool); // Lazy init
    // ... return particle
}
```

---

## 🔗 **Platform-Specific Notes**

### **macOS**
- **Strictest enforcement** of initialization rules
- Metal + Objective-C class init is single-threaded
- Use this as your **test platform** for lifecycle issues
- If it works on macOS, it works everywhere

### **Linux**
- More permissive, may hide timing issues
- Still benefits from deferred init (better practice)

### **Windows**
- Similar to Linux in permissiveness
- D3D12 has some threading requirements but less strict

---

## 📚 **Further Reading**

- **GDExtension Docs**: [docs.godotengine.org/gdextension](https://docs.godotengine.org/en/stable/tutorials/scripting/gdextension/what_is_gdextension.html)
- **Godot Scene Lifecycle**: [docs.godotengine.org/scene-tree](https://docs.godotengine.org/en/stable/tutorials/scripting/nodes_and_scene_instances.html)
- **Metal Threading**: Apple's Metal Best Practices Guide

---

## ❓ **FAQ**

**Q: Why does this only crash on macOS?**  
A: macOS's Objective-C runtime is stricter about class initialization. Other platforms allow the race condition to pass undetected.

**Q: Can I create a few nodes in Initialize()?**  
A: Yes, but keep it under ~10 nodes total across ALL systems. Better to defer everything.

**Q: What about call_deferred?**  
A: `call_deferred` is for scene tree operations, not initialization timing. Use it for `add_child()` always, but still defer the node creation itself.

**Q: Does this apply to headless mode?**  
A: Less critical in headless (no visuals), but still good practice for consistency.

---

**Summary**: Defer all visual node creation to first Update(). Simple rule, avoids 99% of initialization issues.

---

*Document Version*: 1.0  
*Last Updated*: 2026-01-19  
*Applies To*: Godot 4.x + GDExtension  
*Validated On*: macOS (Metal), Linux (Vulkan), Windows (D3D12)



