# gdext-c

**Pure C GDExtension layer for Godot 4.x** - No Rust, No C++, Just C!

🎯 **Goal:** Provide a minimal, clean C interface to Godot's GDExtension API that can be used as a foundation for bindings in any language (Go, Ruby, Python, etc.)

## ✨ Features

- ✅ **Pure C89** - Maximum portability
- ✅ **Zero Dependencies** - Only `gdextension_interface.h` from Godot
- ✅ **Object Creation** - Create any Godot class via `classdb_construct_object`
- ✅ **Scene Tree Access** - Get root node, navigate scene tree
- ✅ **Variant Helpers** - Convert between C types and Godot Variants
- ✅ **Method Calling** - Call methods on Godot objects
- ✅ **Array Support** - PackedInt32Array, PackedVector3Array, etc.
- ✅ **Singleton Access** - Get Engine, DisplayServer, etc.

## 🏗️ Architecture

```
┌─────────────────────────────────────────────────────────┐
│                    Your Application                      │
│                  (Go, Ruby, Python, etc.)                │
└────────────────────┬────────────────────────────────────┘
                     │
                     │ CGO / FFI
                     ▼
┌─────────────────────────────────────────────────────────┐
│                      gdext-c                             │
│          (Pure C wrapper around GDExtension API)         │
└────────────────────┬────────────────────────────────────┘
                     │
                     │ GDExtensionInterface
                     ▼
┌─────────────────────────────────────────────────────────┐
│                    Godot Engine                          │
│             (Provides GDExtension API)                   │
└─────────────────────────────────────────────────────────┘
```

### Key Design Decisions

1. **Centralized Interface:** All GDExtension function pointers stored in `GDExtensionInterface` struct
2. **No Global State:** All functions check initialization before proceeding
3. **Explicit Memory Management:** Caller owns returned pointers
4. **Simple API Surface:** Minimal, orthogonal functions

## 📦 Components

```
gdext-c/
├── include/
│   ├── gdext_c.h              # Public API
│   └── gdextension_interface.h # From Godot (generated)
├── src/
│   ├── core/
│   │   ├── gdext_c_core.c     # Initialization
│   │   └── gdext_c_core.h     # Internal interface struct
│   ├── api/
│   │   ├── gdext_c_objects.c  # Object creation
│   │   ├── gdext_c_variants.c # Variant conversions
│   │   ├── gdext_c_method_calling.c # Method calls
│   │   └── gdext_c_arrays.c   # Array handling
│   └── scene/
│       └── scene_access.c     # Scene tree operations
└── Makefile                   # Build configuration
```

## 🚀 Quick Start

### 1. Build the Library

```bash
make
```

This produces `libgdext_c.dylib` (macOS), `libgdext_c.so` (Linux), or `libgdext_c.dll` (Windows).

### 2. Initialize in Your GDExtension

```c
#include "gdext_c.h"

GDExtensionBool GDN_EXPORT my_extension_init(
    GDExtensionInterfaceGetProcAddress p_get_proc_address,
    GDExtensionClassLibraryPtr p_library,
    GDExtensionInitialization *r_initialization
) {
    // Initialize gdext-c with Godot's proc_address
    if (!gdext_c_initialize(p_get_proc_address)) {
        return false;
    }
    
    // Now you can use gdext-c functions!
    return true;
}
```

### 3. Create Objects and Call Methods

```c
// Create a Node3D
gdext_c_object_t node = gdext_c_create_object("Node3D");

// Create a variant
GDExtensionVariantPtr pos_x = gdext_c_variant_from_float(10.5);

// Call a method
GDExtensionVariantPtr result = gdext_c_call_method1(
    node, "set_position_x", pos_x
);

// Clean up
gdext_c_variant_free(pos_x);
gdext_c_variant_free(result);
```

## 🔧 API Reference

### Initialization

```c
bool gdext_c_initialize(GDExtensionInterfaceGetProcAddress proc_address);
bool gdext_c_is_initialized(void);
const GDExtensionInterface* gdext_c_get_interface_functions(void);
```

### Object Creation

```c
gdext_c_object_t gdext_c_create_object(const char* class_name);
```

### Scene Tree

```c
gdext_c_object_t gdext_c_get_root_node(void);
gdext_c_object_t gdext_c_get_node(const char* path);
```

### Variants

```c
// Creation
GDExtensionVariantPtr gdext_c_variant_new_nil(void);
GDExtensionVariantPtr gdext_c_variant_from_int(int64_t value);
GDExtensionVariantPtr gdext_c_variant_from_float(double value);
GDExtensionVariantPtr gdext_c_variant_from_bool(int value);
GDExtensionVariantPtr gdext_c_variant_from_string(const char* value);
GDExtensionVariantPtr gdext_c_variant_from_vector3(float x, float y, float z);
GDExtensionVariantPtr gdext_c_variant_from_color(float r, float g, float b, float a);
GDExtensionVariantPtr gdext_c_variant_from_object(gdext_c_object_t object);

// Extraction
int64_t gdext_c_variant_to_int(GDExtensionConstVariantPtr variant);
double gdext_c_variant_to_float(GDExtensionConstVariantPtr variant);
bool gdext_c_variant_to_bool(GDExtensionConstVariantPtr variant);
// ... etc

// Cleanup
void gdext_c_variant_free(GDExtensionVariantPtr variant);
```

### Method Calling

```c
GDExtensionVariantPtr gdext_c_call_method(
    gdext_c_object_t object,
    const char* method_name,
    GDExtensionConstVariantPtr* args,
    int arg_count
);

// Convenience wrappers
GDExtensionVariantPtr gdext_c_call_method0(gdext_c_object_t object, const char* method_name);
GDExtensionVariantPtr gdext_c_call_method1(gdext_c_object_t object, const char* method_name, GDExtensionConstVariantPtr arg1);
// ... up to gdext_c_call_method3
```

### Arrays

```c
GDExtensionVariantPtr gdext_c_variant_from_packed_int32_array(int32_t* values, int count);
GDExtensionVariantPtr gdext_c_variant_from_packed_vector3_array(float* values, int count);
```

## 📚 Example: Complete Workflow

```c
#include "gdext_c.h"

void example() {
    // 1. Create a MeshInstance3D
    gdext_c_object_t mesh_inst = gdext_c_create_object("MeshInstance3D");
    
    // 2. Create a BoxMesh
    gdext_c_object_t box_mesh = gdext_c_create_object("BoxMesh");
    
    // 3. Set mesh size
    GDExtensionVariantPtr size_x = gdext_c_variant_from_float(2.0);
    GDExtensionVariantPtr size_y = gdext_c_variant_from_float(2.0);
    GDExtensionVariantPtr size_z = gdext_c_variant_from_float(2.0);
    GDExtensionVariantPtr size = gdext_c_variant_from_vector3(2.0, 2.0, 2.0);
    gdext_c_call_method1(box_mesh, "set_size", size);
    
    // 4. Assign mesh to instance
    GDExtensionVariantPtr mesh_variant = gdext_c_variant_from_object(box_mesh);
    gdext_c_call_method1(mesh_inst, "set_mesh", mesh_variant);
    
    // 5. Add to scene tree
    gdext_c_object_t root = gdext_c_get_root_node();
    GDExtensionVariantPtr node_variant = gdext_c_variant_from_object(mesh_inst);
    gdext_c_call_method1(root, "add_child", node_variant);
    
    // 6. Clean up
    gdext_c_variant_free(size_x);
    gdext_c_variant_free(size_y);
    gdext_c_variant_free(size_z);
    gdext_c_variant_free(size);
    gdext_c_variant_free(mesh_variant);
    gdext_c_variant_free(node_variant);
}
```

## 🧪 Testing

The library is tested by integration with a complete AAA game framework: [action-adventure-framework](https://github.com/jeffreyfriedman/action-adventure-framework)

## 🎯 Roadmap

- [x] Object creation
- [x] Scene tree access
- [x] Variant conversions (primitives)
- [x] Method calling
- [x] PackedArrays
- [ ] Property getters/setters (in progress)
- [ ] Signal connections
- [ ] Resource loading
- [ ] Dictionary support
- [ ] More PackedArray types
- [ ] Error handling improvements
- [ ] Comprehensive test suite

## 🤝 Contributing

This is a **dogfooding project** - we're using it to build real games! Contributions welcome, especially:
- Bug fixes
- Additional Variant types
- Platform support (Windows, Linux)
- Documentation improvements
- Example projects in other languages

## 📝 License

MIT License - See LICENSE file

## 🔗 Related Projects

- [gdext-go](https://github.com/jeffreyfriedman/gdext-go) - Go bindings using gdext-c
- [action-adventure-framework](https://github.com/jeffreyfriedman/action-adventure-framework) - Complete AAA game using gdext-go
- [godot-rust](https://github.com/godot-rust/gdext) - Rust bindings (different approach)

## 💡 Why gdext-c?

**Problem:** GDExtension API is complex and error-prone to use directly. Most language bindings re-implement the same wrapper code.

**Solution:** gdext-c provides a clean, tested C layer that:
- Handles the complexity of `proc_address` lookups
- Provides type-safe variant conversions
- Simplifies method calling
- Works as a foundation for any language binding

**Result:** Write your language binding once, get all the benefits of gdext-c for free!

---

**Status:** ✅ Core functionality complete, actively developed, production-ready for basic use cases.

**Note:** Property setters have a known crash bug that is actively being debugged. Object creation, method calling, and variant conversions all work correctly.
