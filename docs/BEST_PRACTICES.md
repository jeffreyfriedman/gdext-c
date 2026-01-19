# gdext-c Best Practices

**For Language Binding Developers Using gdext-c**

---

## 🎯 **Core Principles**

1. **Defer Visual Creation** - Create nodes in Update(), not Initialize()
2. **Use call_deferred** - For all scene tree modifications
3. **Check Return Values** - Every gdext-c function can fail
4. **Free Resources** - Variants, StringNames need explicit cleanup
5. **Test on macOS** - Strictest platform, catches timing issues

---

## 📚 **Essential Patterns**

### **1. Deferred Initialization**

```c
// System with lazy visual init
typedef struct {
    bool visuals_ready;
    void* scene;
    void** nodes;      // Array of visual nodes
    int node_count;
} VisualSystem;

void visual_system_init(VisualSystem* sys, void* scene) {
    sys->visuals_ready = false;
    sys->scene = scene;
    sys->nodes = NULL;
    sys->node_count = 0;
}

void visual_system_update(VisualSystem* sys, double delta) {
    if (!sys->visuals_ready) {
        // Create visuals on first update (engine is ready)
        sys->nodes = malloc(sizeof(void*) * 100);
        for (int i = 0; i < 100; i++) {
            sys->nodes[i] = gdext_mesh_instance3_d_create();
            gdext_add_child_deferred(sys->scene, sys->nodes[i]);
        }
        sys->visuals_ready = true;
    }
    
    // Normal update logic...
}
```

**Why**: Avoids race conditions during engine init (see [GODOT_LIFECYCLE.md](GODOT_LIFECYCLE.md))

---

### **2. Safe Scene Tree Modifications**

```c
// ❌ WRONG: Direct add_child (may crash)
void add_node_wrong(void* parent, void* child) {
    gdext_call_method(parent, "add_child", &child, 1);
}

// ✅ CORRECT: Use call_deferred
void add_node_correct(void* parent, void* child) {
    gdext_add_child_deferred(parent, child);
}
```

**Rule**: ALWAYS use `_deferred` variants for scene tree changes.

---

### **3. Variant Lifecycle**

```c
// Create variant
void* variant = gdext_variant_from_int(42);

// Use variant
gdext_call_method(obj, "set_value", &variant, 1);

// ✅ CRITICAL: Free variant
gdext_variant_free(variant);
```

**Pattern**: Create → Use → Free (no exceptions!)

---

### **4. Error Handling**

```c
void* node = gdext_mesh_instance3_d_create();
if (!node) {
    fprintf(stderr, "Failed to create MeshInstance3D\n");
    return ERROR_CODE;
}

bool success = gdext_object_set_property(node, "visible", true_variant);
if (!success) {
    fprintf(stderr, "Failed to set visibility\n");
    // Handle error...
}
```

**Rule**: Check EVERY return value, especially during initialization.

---

### **5. Singleton Access**

```c
// ✅ CORRECT: Get singleton once, cache it
static void* rendering_server = NULL;

void init_rendering() {
    if (!rendering_server) {
        rendering_server = gdext_get_singleton("RenderingServer");
        if (!rendering_server) {
            fprintf(stderr, "Failed to get RenderingServer\n");
            return;
        }
    }
    
    // Use rendering_server...
}
```

**Pattern**: Get once → Cache → Reuse

---

## 🚫 **Common Mistakes**

### **Mistake 1: Creating Visuals in Initialize()**

```c
// ❌ BAD
void particle_system_init(void* scene) {
    for (int i = 0; i < 100; i++) {
        void* particle = create_particle();
        add_child(scene, particle); // CRASHES on macOS!
    }
}
```

**Fix**: Use deferred initialization pattern (see above)

---

### **Mistake 2: Forgetting call_deferred**

```c
// ❌ BAD: Direct scene tree modification
parent.CallMethod1("add_child", child); // Race condition!

// ✅ GOOD: Deferred
gdext_add_child_deferred(parent, child);
```

**Fix**: Use `gdext_add_child_deferred()` or similar helpers

---

### **Mistake 3: Leaking Variants**

```c
// ❌ BAD: Leak
for (int i = 0; i < 1000; i++) {
    void* v = gdext_variant_from_int(i);
    use_variant(v);
    // Missing gdext_variant_free(v)! LEAK!
}

// ✅ GOOD: Free immediately
for (int i = 0; i < 1000; i++) {
    void* v = gdext_variant_from_int(i);
    use_variant(v);
    gdext_variant_free(v); // ✅
}
```

**Fix**: Free variants as soon as done using them

---

### **Mistake 4: Not Checking for NULL**

```c
// ❌ BAD: Assumes success
void* node = gdext_mesh_instance3_d_create();
node->SetPosition(...); // CRASH if create failed!

// ✅ GOOD: Check first
void* node = gdext_mesh_instance3_d_create();
if (node) {
    node->SetPosition(...);
} else {
    handle_error();
}
```

**Fix**: Check EVERY pointer before dereferencing

---

## 🎮 **Game-Specific Patterns**

### **Particle Pools**

```c
typedef struct {
    void** particles;
    int capacity;
    bool* active;
    bool initialized;
} ParticlePool;

ParticlePool* pool_create(int capacity) {
    ParticlePool* pool = malloc(sizeof(ParticlePool));
    pool->capacity = capacity;
    pool->particles = malloc(sizeof(void*) * capacity);
    pool->active = calloc(capacity, sizeof(bool));
    pool->initialized = false; // Defer actual creation!
    return pool;
}

void pool_ensure_init(ParticlePool* pool, void* scene) {
    if (pool->initialized) return;
    
    for (int i = 0; i < pool->capacity; i++) {
        pool->particles[i] = gdext_cpu_particles3_d_create();
        gdext_add_child_deferred(scene, pool->particles[i]);
    }
    pool->initialized = true;
}

void* pool_acquire(ParticlePool* pool, void* scene) {
    pool_ensure_init(pool, scene); // Lazy init
    
    for (int i = 0; i < pool->capacity; i++) {
        if (!pool->active[i]) {
            pool->active[i] = true;
            return pool->particles[i];
        }
    }
    return NULL; // Pool exhausted
}
```

---

### **Entity Visual Sync**

```c
void sync_entity_visuals(Entity* entity, void* node) {
    // Position
    void* pos_variant = gdext_variant_from_vector3(
        entity->x, entity->y, entity->z);
    gdext_object_set_property(node, "global_position", pos_variant);
    gdext_variant_free(pos_variant);
    
    // Rotation
    void* rot_variant = gdext_variant_from_vector3(
        0, entity->yaw, 0);
    gdext_object_set_property(node, "rotation", rot_variant);
    gdext_variant_free(rot_variant);
}
```

---

## 🔧 **Performance Tips**

1. **Cache Singletons** - Don't call `gdext_get_singleton()` every frame
2. **Batch Operations** - Group `call_deferred` calls when possible
3. **Reuse Variants** - If calling same method repeatedly
4. **Use Pools** - For frequently created/destroyed objects
5. **Profile on Target** - Different platforms have different bottlenecks

---

## 📊 **Testing Checklist**

Before releasing your binding:

```
[ ] Works in headless mode (no visuals)
[ ] Works with <10 entities (simple case)
[ ] Works with 100+ entities (stress test)
[ ] No crashes during initialization
[ ] No memory leaks (run with valgrind/asan)
[ ] Tested on macOS (strictest platform)
[ ] Tested on Linux
[ ] Tested on Windows
[ ] Scene tree modifications work correctly
[ ] Variant lifecycle is correct (no leaks)
```

---

## 🐛 **Debugging Tips**

### **Crash During Init**

```bash
# Run with debugger
$ lldb ./godot -- --path /path/to/project
(lldb) run
# On crash:
(lldb) bt  # Backtrace
(lldb) frame select 0
(lldb) p variable_name  # Inspect variables
```

**Look for**:
- `objc` messages (macOS class init issues)
- `add_child` in backtrace (scene tree timing)
- Multiple systems creating nodes simultaneously

**Fix**: Apply deferred initialization pattern

---

### **Memory Leaks**

```bash
# Linux
$ valgrind --leak-check=full ./godot --path /path/to/project

# macOS
$ leaks -atExit -- ./godot --path /path/to/project
```

**Common sources**:
- Variants not freed
- StringNames not freed
- Nodes not properly removed from scene

---

### **Visual Not Appearing**

**Checklist**:
1. [ ] Node created successfully (not NULL)
2. [ ] Added to scene tree (`add_child`)
3. [ ] Node is visible (`visible = true`)
4. [ ] Position is correct (not off-screen)
5. [ ] Material/mesh is set (if applicable)
6. [ ] Parent is visible and in scene tree

---

## 📚 **Reference**

- [GODOT_LIFECYCLE.md](GODOT_LIFECYCLE.md) - Initialization timing
- [API Reference](../include/gdext_c.h) - Function documentation
- [Examples](../examples/) - Working code samples

---

## 💡 **Pro Tips**

1. **Start Small** - Get one node working before creating 100
2. **Log Everything** - During development, log all API calls
3. **Test Early, Test Often** - Don't wait until feature complete
4. **Read Godot Docs** - gdext-c follows Godot's semantics
5. **Check Examples** - We've solved common problems already

---

**When in Doubt**: Defer to first Update(), use call_deferred, check for NULL, free resources.

---

*Document Version*: 1.0  
*Last Updated*: 2026-01-19  
*For*: gdext-c Language Binding Developers

