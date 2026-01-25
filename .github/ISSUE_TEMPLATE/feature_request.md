---
name: Feature request
about: Suggest an idea for gdext-c
title: '[FEATURE] '
labels: enhancement
assignees: ''
---

## Feature Summary
A clear and concise description of the feature you'd like.

## Problem/Use Case
What problem does this feature solve? What would you use it for?

**Example:**
"I need to pass dictionary data to Godot methods but there's no convenient way to create dictionaries from C."

## Proposed Solution
Describe the solution you'd like. Include proposed API if applicable.

**Example API:**
```c
GDExtensionVariantPtr gdext_c_variant_from_dictionary(
    const char** keys, 
    GDExtensionConstVariantPtr* values, 
    int count
);
```

**Example Usage:**
```c
const char* keys[] = {"name", "age", "active"};
GDExtensionVariantPtr values[] = {
    gdext_c_variant_from_string("John"),
    gdext_c_variant_from_int(30),
    gdext_c_variant_from_bool(1)
};

GDExtensionVariantPtr dict = gdext_c_variant_from_dictionary(keys, values, 3);
```

## Alternatives Considered
Describe alternatives you've considered.

**Example:**
- Using GDScript to create dictionaries (not performant)
- Directly calling GDExtension dictionary API (too complex)
- Creating helper functions in my own codebase (not reusable)

## Implementation Notes
If you have ideas on how to implement this, share them here.

**Example:**
- Would use `GDExtensionInterfaceDictionaryNew` from GDExtension API
- Similar pattern to existing array creation functions
- Need to handle key/value type validation

## Additional Context
Add any other context, screenshots, or examples about the feature request here.

## Alignment with Project Goals
How does this feature align with gdext-c's goals?
- [ ] Maintains simplicity (doesn't complicate the API)
- [ ] Maintains portability (C89 compatible)
- [ ] Solves a common use case (not too specific)
- [ ] Fits the C-level abstraction (not too high-level)

## Checklist
- [ ] I've checked if this already exists
- [ ] I've described the use case
- [ ] I've proposed an API (if applicable)
- [ ] I've considered alternatives
- [ ] This aligns with project goals


