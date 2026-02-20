/**
 * @file gdext_c_defs.h
 * @brief Common definitions and export macros for GDExtension
 * 
 * TDD #206: Add GDE_EXPORT macro to properly export entry point
 * Based on Godot documentation: https://docs.godotengine.org/en/4.6/tutorials/scripting/gdextension/gdextension_c_example.html
 */

#ifndef GDEXT_C_DEFS_H
#define GDEXT_C_DEFS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Export macro for functions that should be visible in the shared library
 * 
 * This macro ensures the GDExtension entry point is properly exported
 * so Godot can find and call it when loading the extension.
 */
#if !defined(GDE_EXPORT)
    #if defined(_WIN32)
        #define GDE_EXPORT __declspec(dllexport)
    #elif defined(__GNUC__)
        #define GDE_EXPORT __attribute__((visibility("default")))
    #else
        #define GDE_EXPORT
    #endif
#endif // ! GDE_EXPORT

#endif // GDEXT_C_DEFS_H
