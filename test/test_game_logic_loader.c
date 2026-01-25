/**
 * @file test_game_logic_loader.c
 * @brief TDD #158: Test loading Go game logic library
 */

#include <stdio.h>
#include <assert.h>
#include <dlfcn.h>
#include <string.h>

// Test that we can load the game logic library
int main() {
    printf("🧪 TDD #158: Testing game logic library loading...\n");
    
    // Try to load the game logic library (relative to test binary)
    const char* lib_path = "../action-adventure-framework/bin/macos/game_logic.dylib";
    
    printf("📂 Attempting to load: %s\n", lib_path);
    
    void* handle = dlopen(lib_path, RTLD_NOW | RTLD_LOCAL);
    if (!handle) {
        fprintf(stderr, "❌ Failed to load game_logic.dylib: %s\n", dlerror());
        return 1;
    }
    
    printf("✅ Successfully loaded game_logic.dylib\n");
    printf("📍 Handle: %p\n", handle);
    
    // Go's init() should run automatically when the library is loaded
    // We can't test if it ran yet, but we can verify the library loaded
    
    // Clean up
    dlclose(handle);
    printf("✅ Closed library\n");
    
    printf("🎉 TDD #158: Game logic library loading WORKS!\n");
    return 0;
}


