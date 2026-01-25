# TDD #152: Simplified PackedByteArray for SVO Renderer

**Status**: In progress (simplifying approach)

## Problem

Buffer_update needs PackedByteArray:
```go
func (r *RenderingDevice) Buffer_update(
    buffer *RID,
    offset int64,
    size_bytes int64,
    data PackedByteArray,  // ← Need this!
) int64
```

## Current Blocker

Tried to implement full PackedByteArray in C with builtin method bindings, but:
- API function names are complex (`variant_get_ptr_builtin_method`)
- Need to understand Godot's builtin method calling convention
- Too much infrastructure for a single use case

## Simplified Approach

**For v0.1.0, we DON'T need full PackedByteArray support!**

We just need to:
1. Pass Go `[]byte` → PackedByteArray argument
2. That's it!

## Solution: Use Existing classdb Functions

Looking at our existing code, we already have ways to create Godot builtin types:

```go
// In gdext-go/pkg/classdb/variant.go (or similar)
func NewPackedByteArray(data []byte) *PackedByteArray {
    // Use existing variant construction
    // Convert Go []byte to Godot PackedByteArray
}
```

**Key insight**: We don't need to implement C functions! We can use:
- `classdb.CreatePackedByteArray(data []byte)` → Returns opaque handle
- Pass that handle to `Buffer_update()`
- Godot's existing FFI handles the rest!

## Next Steps (Revised)

1. **Skip the C implementation** (defer to v0.2.0)
2. **Add PackedByteArray to gdext-go types** (simple wrapper)
3. **Update wrapper generator** to NOT skip methods with PackedByteArray
4. **Test with buffer_update**

## Estimated Time

- Original approach: 2+ hours (stuck on API)
- Simplified approach: 30 minutes

---

*Lesson*: Don't overengineer for v0.1.0. Ship the minimum that validates the infrastructure!


