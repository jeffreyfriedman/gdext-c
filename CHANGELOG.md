# Changelog

All notable changes to gdext-c will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- Pure C GDExtension implementation
- Object creation (`gdext_c_create_object`)
- Scene tree access (`gdext_c_get_root_node`, `gdext_c_get_node`)
- Variant conversions for primitive types (int, float, bool, string)
- Variant conversions for vector types (Vector2, Vector3)
- Variant conversions for Color
- Variant conversions for Object pointers
- Method calling (`gdext_c_call_method` with variadic helpers)
- PackedInt32Array support
- PackedVector3Array support
- Singleton access
- Centralized GDExtensionInterface struct
- Comprehensive README with examples
- MIT License
- Contributing guidelines
- GitHub issue templates

### Known Issues
- Property setters crash (under investigation)
- Dictionary support not yet implemented
- Signal connections not yet implemented
- Resource loading not yet implemented

## [0.1.0] - 2026-01-14

### Added
- Initial public release
- Core GDExtension wrapper functionality
- Published to GitHub: https://github.com/jeffreyfriedman/gdext-c

### Notes
- This is an alpha release
- Battle-tested through integration with [action-adventure-framework](https://github.com/jeffreyfriedman/action-adventure-framework)
- Object creation, method calling, and basic variants work reliably
- Property setters have a known crash bug

[Unreleased]: https://github.com/jeffreyfriedman/gdext-c/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/jeffreyfriedman/gdext-c/releases/tag/v0.1.0


