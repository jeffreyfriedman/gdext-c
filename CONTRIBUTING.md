# Contributing to gdext-c

Thank you for your interest in contributing to gdext-c! This document provides guidelines and instructions for contributing.

## 🎯 Project Goals

- **Simplicity:** Keep the API minimal and easy to understand
- **Portability:** C89 compatible, works everywhere
- **Zero Dependencies:** Only Godot's `gdextension_interface.h`
- **Production Ready:** Battle-tested through real game development

## 🤝 How to Contribute

### Reporting Bugs

**Before submitting a bug report:**
- Check if the issue already exists in [GitHub Issues](https://github.com/jeffreyfriedman/gdext-c/issues)
- Verify you're using the latest version
- Test with a minimal reproduction case

**When submitting a bug report, include:**
- **OS and version:** (e.g., macOS 14.2, Ubuntu 22.04, Windows 11)
- **Godot version:** (e.g., 4.2.1, 4.3.0)
- **gdext-c commit hash:** Run `git rev-parse HEAD` in the repo
- **Minimal code to reproduce:** Smallest possible example that triggers the bug
- **Expected behavior:** What should happen
- **Actual behavior:** What actually happens
- **Stack trace/logs:** If applicable

**Example:**
```
**Environment:**
- OS: macOS 14.2 (23C64)
- Godot: 4.2.1 stable
- gdext-c: commit abc123

**Bug:** Crash when calling gdext_c_variant_from_string with NULL

**Expected:** Should return NULL or handle gracefully
**Actual:** Segmentation fault

**Reproduction:**
```c
GDExtensionVariantPtr v = gdext_c_variant_from_string(NULL);
```

**Stack trace:**
```
#0  0x000000010000abcd in gdext_c_variant_from_string
#1  0x000000010000def0 in main
```
```

### Suggesting Features

**Before suggesting a feature:**
- Check if it's already requested in [GitHub Discussions](https://github.com/jeffreyfriedman/gdext-c/discussions)
- Consider if it aligns with project goals (simplicity, portability)
- Think about whether it belongs in gdext-c or in a higher-level binding

**When suggesting a feature, include:**
- **Use case:** Why do you need this?
- **Proposed API:** What would the function signature look like?
- **Alternatives considered:** Other ways to achieve the same goal
- **Implementation notes:** If you have ideas on how to implement it

**Example:**
```
**Feature:** Add gdext_c_variant_from_dictionary

**Use case:** Need to pass dictionary data to Godot methods

**Proposed API:**
```c
GDExtensionVariantPtr gdext_c_variant_from_dictionary(
    const char** keys, 
    GDExtensionConstVariantPtr* values, 
    int count
);
```

**Alternatives:**
- Use GDScript to create dictionaries (not ideal for performance)
- Directly use GDExtension dictionary API (complex)

**Implementation notes:**
- Would need to use `GDExtensionInterfaceDictionaryNew`
- Similar pattern to array creation
```

### Contributing Code

#### 1. Fork and Clone

```bash
# Fork the repository on GitHub, then:
git clone https://github.com/YOUR_USERNAME/gdext-c.git
cd gdext-c
git remote add upstream https://github.com/jeffreyfriedman/gdext-c.git
```

#### 2. Create a Branch

```bash
# For bug fixes:
git checkout -b fix/issue-123-description

# For features:
git checkout -b feature/feature-name

# For documentation:
git checkout -b docs/what-you-changed
```

#### 3. Make Your Changes

**Code Style:**
- **C89 compatible** - No C99/C11 features (no `//` comments, no mixed declarations)
- **4-space indentation** - No tabs
- **80-character line limit** - For readability
- **Function naming:** `gdext_c_` prefix for public API
- **Error checking:** Check all return values, handle NULL gracefully
- **Documentation:** Add comments for public functions

**Example:**
```c
/**
 * @brief Creates a variant from a boolean value
 * 
 * @param value Boolean value (0 = false, non-zero = true)
 * @return GDExtensionVariantPtr Newly created variant, or NULL on failure
 * 
 * @note Caller must free the returned variant with gdext_c_variant_free()
 */
GDExtensionVariantPtr gdext_c_variant_from_bool(int value) {
    if (!gdext_c_is_initialized()) {
        fprintf(stderr, "[gdext-c] ❌ Library not initialized!\n");
        return NULL;
    }
    
    /* Implementation */
    /* ... */
    
    return variant;
}
```

**File Organization:**
- `include/gdext_c.h` - Public API declarations
- `src/core/` - Initialization and core infrastructure
- `src/api/` - Variant, method calling, arrays
- `src/scene/` - Scene tree operations
- `src/utils/` - Optional helper functions

#### 4. Test Your Changes

**Build:**
```bash
make clean && make
```

**Test with action-adventure-framework:**
```bash
cd ../action-adventure-framework
gdextctl build
gdextctl playtest
```

**Manual testing:**
```c
// Create a simple test program
#include "gdext_c.h"

int main() {
    // Test your changes here
    GDExtensionVariantPtr v = gdext_c_your_new_function();
    if (v == NULL) {
        printf("FAILED!\n");
        return 1;
    }
    printf("PASSED!\n");
    gdext_c_variant_free(v);
    return 0;
}
```

#### 5. Commit Your Changes

**Commit message format:**
```
<type>: <subject>

<body>

<footer>
```

**Types:**
- `feat:` New feature
- `fix:` Bug fix
- `docs:` Documentation only
- `style:` Code style (formatting, whitespace)
- `refactor:` Code refactoring
- `test:` Adding tests
- `chore:` Build system, dependencies

**Example:**
```
feat: Add dictionary variant support

Implements gdext_c_variant_from_dictionary and 
gdext_c_variant_to_dictionary for converting between C key-value
pairs and Godot Dictionary type.

Closes #42
```

#### 6. Push and Create Pull Request

```bash
git push origin feature/feature-name
```

Then create a Pull Request on GitHub.

**PR Checklist:**
- [ ] Code builds successfully (`make`)
- [ ] Tested with action-adventure-framework (or equivalent)
- [ ] No compiler warnings
- [ ] Documentation updated (if adding public API)
- [ ] CHANGELOG.md updated (if applicable)
- [ ] Follows code style guidelines
- [ ] Commit messages are clear

## 📝 Documentation

### Updating README.md

When adding new features, update the README:

1. **API Reference section** - Add function signatures
2. **Examples section** - Add usage examples
3. **Roadmap** - Move from "planned" to "implemented"

### Writing API Documentation

Use this format for public functions:

```c
/**
 * @brief One-line description
 * 
 * Detailed description explaining what the function does,
 * when to use it, and any important notes.
 * 
 * @param param1 Description of first parameter
 * @param param2 Description of second parameter
 * @return Description of return value
 * 
 * @note Important notes about usage, memory management, etc.
 * @warning Warnings about potential issues
 * 
 * @example
 * ```c
 * // Example usage
 * GDExtensionVariantPtr v = gdext_c_function(arg1, arg2);
 * if (v == NULL) {
 *     // Handle error
 * }
 * gdext_c_variant_free(v);
 * ```
 */
```

## 🧪 Testing

### Unit Tests (TODO)

We don't have a formal test suite yet. Contributions to add one are welcome!

**Ideal setup:**
- Minimal test framework (avoid heavy dependencies)
- Tests for each public API function
- Mock GDExtension interface for testing
- CI/CD integration

### Integration Testing

Currently, gdext-c is tested through integration with [action-adventure-framework](https://github.com/jeffreyfriedman/action-adventure-framework):

- 79+ game systems
- 10,000+ lines of code
- Creates 50+ Godot object types
- Calls 200+ methods
- Converts 1000+ variants

This provides real-world validation but makes it hard to isolate issues.

## 🎨 Code Review Process

1. **Automated checks:** CI runs (when available)
2. **Manual review:** Maintainer reviews code
3. **Testing:** Changes tested with action-adventure-framework
4. **Discussion:** Any questions/concerns addressed
5. **Approval:** Maintainer approves and merges

**What we look for:**
- ✅ Follows code style
- ✅ Well-documented
- ✅ Handles errors gracefully
- ✅ No memory leaks
- ✅ Maintains backward compatibility
- ✅ Solves the stated problem

## 🚀 Release Process

1. Update `CHANGELOG.md`
2. Update version in `Makefile`
3. Tag release: `git tag -a v0.x.0 -m "Release v0.x.0"`
4. Push tag: `git push origin v0.x.0`
5. Create GitHub release with changelog

## 💬 Communication

- **Questions:** [GitHub Discussions](https://github.com/jeffreyfriedman/gdext-c/discussions)
- **Bugs:** [GitHub Issues](https://github.com/jeffreyfriedman/gdext-c/issues)
- **Pull Requests:** [GitHub Pull Requests](https://github.com/jeffreyfriedman/gdext-c/pulls)

## 📜 License

By contributing, you agree that your contributions will be licensed under the MIT License.

## 🙏 Recognition

All contributors will be recognized in the README.md Contributors section.

Thank you for making gdext-c better! 🎉



