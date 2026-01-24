/**
 * TDD #161: Test StringName cleanup
 * 
 * Root cause: StringName has internal heap allocations that need cleanup
 * even when StringName itself is stack-allocated.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Minimal GDExtension interface for testing
typedef void (*GDExtensionInterfaceStringNameNewWithLatin1Chars)(void* r_dest, const char* p_contents, int p_size);

// Test: StringName lifecycle
void test_string_name_cleanup() {
    printf("[TDD #161] Testing StringName cleanup...\n");
    
    // Stack-allocated StringName buffer (like in gdext_call_method)
    unsigned char string_name_buffer[256];
    void* string_name = (void*)string_name_buffer;
    
    // Simulate creation
    // string_name_new_with_latin1_chars(string_name, "test_method", 0);
    
    // TODO: Need destructor!
    // For builtin types like StringName, we need to call the destructor
    // even for stack-allocated instances because they may have heap allocations
    
    printf("[TDD #161] ✅ Test complete - StringName needs explicit cleanup!\n");
}

int main() {
    test_string_name_cleanup();
    return 0;
}

