# Go Wrappers Design (TDD #144)

**Goal**: Generate type-safe, idiomatic Go API for all 971 Godot classes

---

## Current API (CGO Direct)

```go
// Creating nodes - verbose, error-prone
parent := classdb.CreateNode3D()
child := classdb.CreateNode3D()

// Calling methods - no type safety!
classdb.AddChildSafe(parent, child)

// Setting properties - string-based, no autocomplete
pos := classdb.NewVector3(1.0, 2.0, 3.0).ToVariant()
defer pos.Free()
parent.SetProperty("position", pos.ptr)
```

**Problems**:
- No type safety
- No autocomplete/IntelliSense  
- Manual memory management (ToVariant, Free)
- String-based property access
- Easy to make mistakes

---

## Proposed API (Generated Wrappers)

```go
// Creating nodes - typed!
parent := gdext.NewNode3D()
child := gdext.NewNode3D()

// Calling methods - type-safe!
parent.AddChild(child, false, gdext.InternalModeDisabled)

// Setting properties - typed!
parent.SetPosition(gdext.Vec3{X: 1.0, Y: 2.0, Z: 3.0})

// Getting properties - typed!
pos := parent.GetPosition() // Returns gdext.Vec3
```

**Benefits**:
- ✅ Full type safety
- ✅ Autocomplete/IntelliSense
- ✅ Automatic memory management
- ✅ Typed property access
- ✅ Compile-time error checking

---

## Architecture

```
┌──────────────────────────────────────┐
│          Go Application              │
│    (Your game code)                  │
└───────────────┬──────────────────────┘
                │
                │ import "gdext"
                ▼
┌──────────────────────────────────────┐
│     gdext (Generated Wrappers)       │
│   - Type-safe Go structs             │
│   - Method wrappers                  │
│   - Property accessors               │
└───────────────┬──────────────────────┘
                │
                │ CGO calls
                ▼
┌──────────────────────────────────────┐
│        gdext-c (C Library)           │
│   - 14,534 generated methods         │
│   - Pure C GDExtension bindings      │
└───────────────┬──────────────────────┘
                │
                │ GDExtension API
                ▼
┌──────────────────────────────────────┐
│           Godot Engine               │
└──────────────────────────────────────┘
```

---

## Generated Code Structure

### For Each Godot Class

```go
// gdext/node3d.go (auto-generated)
package gdext

/*
#cgo LDFLAGS: -L${SRCDIR}/../gdext-c/bin/macos -lgdext_c
#cgo CFLAGS: -I${SRCDIR}/../gdext-c/include
#include <gdext_c_generated.h>
*/
import "C"

// Node3D wraps a Godot Node3D object
type Node3D struct {
    handle C.gdext_c_object_t
}

// NewNode3D creates a new Node3D
func NewNode3D() *Node3D {
    handle := C.gdext_c_create_object(C.CString("Node3D"))
    return &Node3D{handle: handle}
}

// AddChild adds a child node
func (n *Node3D) AddChild(child *Node3D, forceReadable bool, internal InternalMode) {
    C.gdext_node_add_child(n.handle, child.handle, 
                           C.GDExtensionBool(boolToInt(forceReadable)),
                           C.int32_t(internal))
}

// SetPosition sets the node's position
func (n *Node3D) SetPosition(pos Vec3) {
    // Convert Go Vec3 to C and call setter
    posVariant := vectorToVariant(pos)
    defer C.gdext_variant_free(posVariant)
    C.gdext_c_object_set_property(n.handle, C.CString("position"), posVariant)
}

// GetPosition gets the node's position
func (n *Node3D) GetPosition() Vec3 {
    // Call C getter and convert to Go Vec3
    variant := C.gdext_c_object_get_property(n.handle, C.CString("position"))
    defer C.gdext_variant_free(variant)
    return variantToVector(variant)
}
```

### Common Types

```go
// gdext/types.go
package gdext

// Vec3 represents a 3D vector
type Vec3 struct {
    X, Y, Z float64
}

// Vec2 represents a 2D vector
type Vec2 struct {
    X, Y float64
}

// Color represents an RGBA color
type Color struct {
    R, G, B, A float64
}

// InternalMode enum
type InternalMode int32

const (
    InternalModeDisabled InternalMode = 0
    InternalModeFront    InternalMode = 1
    InternalModeBack     InternalMode = 2
)
```

---

## Generation Strategy

### Phase 1: Core Types (Quick Win)
Generate wrappers for most-used classes:
- Node, Node3D, Node2D
- MeshInstance3D, Sprite2D
- CharacterBody3D, RigidBody3D
- Camera3D, Light3D

**Time**: 1 hour  
**Value**: Immediate usability improvement

### Phase 2: All Classes (Complete)
Generate wrappers for all 971 classes:
- Parse extension_api.json
- Generate one .go file per class
- Package as `gdext-go/pkg/gdext/`

**Time**: 2 hours  
**Value**: Complete type-safe API

### Phase 3: Properties (Polish)
Add typed property accessors for all properties:
- SetPosition(), GetPosition()
- SetRotation(), GetRotation()
- SetVisible(), GetVisible()
- etc.

**Time**: 1 hour  
**Value**: Maximum convenience

---

## Generator Implementation

```go
// gdext-c/cmd/generate-go/main.go
package main

func generateGoWrapper(class ClassDef) string {
    var buf strings.Builder
    
    // Package and imports
    buf.WriteString("package gdext\n\n")
    buf.WriteString("/*\n")
    buf.WriteString("#cgo LDFLAGS: -lgdext_c\n")
    buf.WriteString("#include <gdext_c_generated.h>\n")
    buf.WriteString("*/\n")
    buf.WriteString("import \"C\"\n\n")
    
    // Struct definition
    fmt.Fprintf(&buf, "// %s wraps a Godot %s object\n", class.Name, class.Name)
    fmt.Fprintf(&buf, "type %s struct {\n", class.Name)
    fmt.Fprintf(&buf, "    handle C.gdext_c_object_t\n")
    fmt.Fprintf(&buf, "}\n\n")
    
    // Constructor
    fmt.Fprintf(&buf, "// New%s creates a new %s\n", class.Name, class.Name)
    fmt.Fprintf(&buf, "func New%s() *%s {\n", class.Name, class.Name)
    fmt.Fprintf(&buf, "    h := C.gdext_c_create_object(C.CString(\"%s\"))\n", class.Name)
    fmt.Fprintf(&buf, "    return &%s{handle: h}\n", class.Name)
    fmt.Fprintf(&buf, "}\n\n")
    
    // Methods
    for _, method := range class.Methods {
        if method.IsVirtual {
            continue
        }
        buf.WriteString(generateGoMethod(class.Name, method))
    }
    
    return buf.String()
}

func generateGoMethod(className string, method MethodDef) string {
    // Convert method signature to Go
    // Generate C call
    // Handle type conversions
    // Return Go types
}
```

---

## Benefits

### For Developers
- ✅ **Type safety**: Catch errors at compile time
- ✅ **Autocomplete**: Full IntelliSense support
- ✅ **Less boilerplate**: No manual ToVariant/Free
- ✅ **Clearer code**: `parent.AddChild(child)` vs C calls

### For Maintenance
- ✅ **Auto-generated**: Update extension_api.json, regenerate
- ✅ **Consistent**: All classes use same pattern
- ✅ **Documented**: Generate godoc from Godot docs

### For Performance
- ✅ **Zero overhead**: Thin wrappers over C calls
- ✅ **Inlined**: Go compiler can inline simple wrappers
- ✅ **No allocations**: Direct struct marshaling

---

## Timeline

**Today (TDD #144)**:
- [x] Design document
- [ ] Generator skeleton (30 mins)
- [ ] Generate Node3D wrapper (30 mins)
- [ ] Test in game (30 mins)
- [ ] Generate all classes (1 hour)

**Total**: ~3 hours

**Future**:
- Property accessors
- Signal support
- Godoc generation
- Examples and tests

---

## Success Criteria

✅ **Phase 1**: Can create Node3D and call AddChild with types  
✅ **Phase 2**: All 971 classes have wrappers  
✅ **Phase 3**: Game code uses `gdext` instead of `classdb`  

---

*Design for TDD #144*  
*Created: January 15, 2026, 11:05 PM*



