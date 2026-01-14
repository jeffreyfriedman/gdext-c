// TDD #109: StringName helper for GameNode registration
// PURE C IMPLEMENTATION - No Rust dependencies!
// Uses Godot C API (string_name_new_with_latin1_chars) via proc_address
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../c-bridge/gdextension_interface.h"

extern void* gdext_c_entry_get_proc_address(void);

static GDExtensionInterfaceStringNameNewWithLatin1Chars string_name_new_with_latin1 = NULL;

// TDD #109: Pure C StringName creation (no Rust!)
void* gdext_create_string_name(const char* str) {
    if (!string_name_new_with_latin1) {
        void* proc_addr = gdext_c_entry_get_proc_address();
        if (!proc_addr) {
            fprintf(stderr, "[StringName] ❌ proc_address is NULL\n");
            return NULL;
        }
        
        typedef void* (*GetProcFunc)(const char*);
        GetProcFunc get_proc = (GetProcFunc)proc_addr;
        
        string_name_new_with_latin1 = (GDExtensionInterfaceStringNameNewWithLatin1Chars)
            get_proc("string_name_new_with_latin1_chars");
    }
    
    if (!string_name_new_with_latin1) {
        fprintf(stderr, "[StringName] ❌ Failed to get string_name_new_with_latin1_chars\n");
        return NULL;
    }
    
    // Allocate StringName (opaque type, size unknown - use large buffer)
    void* string_name = malloc(256);
    if (!string_name) {
        fprintf(stderr, "[StringName] ❌ Failed to allocate StringName\n");
        return NULL;
    }
    
    // Initialize StringName with the string
    string_name_new_with_latin1(string_name, str, 1); // p_is_static = 1 for string literals
    
    return string_name;
}

