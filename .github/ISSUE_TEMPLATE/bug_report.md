---
name: Bug report
about: Create a report to help us improve
title: '[BUG] '
labels: bug
assignees: ''
---

## Bug Description
A clear and concise description of what the bug is.

## Environment
- **OS:** (e.g., macOS 14.2, Ubuntu 22.04, Windows 11)
- **Godot Version:** (e.g., 4.2.1 stable, 4.3.0 rc1)
- **gdext-c Commit:** (run `git rev-parse HEAD`)
- **Compiler:** (e.g., GCC 11.3, Clang 15.0, MSVC 2022)

## Steps to Reproduce
1. Initialize gdext-c with `gdext_c_initialize(proc_address)`
2. Call `gdext_c_function_name(args...)`
3. Observe crash/error

## Minimal Code Example
```c
#include "gdext_c.h"

void reproduce_bug() {
    // Minimal code that triggers the bug
    GDExtensionVariantPtr v = gdext_c_variant_from_int(42);
    // ... bug occurs here
}
```

## Expected Behavior
A clear and concise description of what you expected to happen.

## Actual Behavior
A clear and concise description of what actually happened.

## Logs/Stack Trace
```
Paste any error messages, logs, or stack traces here
```

## Screenshots
If applicable, add screenshots to help explain your problem.

## Additional Context
Add any other context about the problem here.

## Checklist
- [ ] I've checked existing issues
- [ ] I've tested with the latest commit
- [ ] I've included a minimal reproduction case
- [ ] I've included environment details


