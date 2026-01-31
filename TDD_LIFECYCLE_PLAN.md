# TDD Plan: Universal Lifecycle System for GDExtension

**Date:** 2026-01-26 22:30  
**Goal:** Replace GameNode with universal lifecycle system  
**Benefit:** All language bindings (Go, Rust, Python, Zig, etc.)

---

## 🎯 **Architecture Overview**

```
┌─────────────────────────────────────────────────────────────┐
│                    Language Bindings                        │
│  (gdext-go, gdext-rust, gdext-python, gdext-zig, etc.)    │
└────────────────────┬────────────────────────────────────────┘
                     │ Register callbacks
                     ▼
┌─────────────────────────────────────────────────────────────┐
│              gdext-c Lifecycle (Universal)                  │
│  • Engine ready detection                                   │
│  • SceneTree signal connection                              │
│  • Callback dispatch                                        │
│  • Clean shutdown coordination                              │
└────────────────────┬────────────────────────────────────────┘
                     │ GDExtension API
                     ▼
┌─────────────────────────────────────────────────────────────┐
│                    Godot Engine                             │
│  • SceneTree signals (process_frame, physics_frame)        │
│  • RenderingServer (is_rendering_device_initialized)       │
│  • Engine singleton                                         │
└─────────────────────────────────────────────────────────────┘
```

---

## 📋 **Phase 1: gdext-c Lifecycle Infrastructure**

**Duration:** 2-3 hours  
**Goal:** Universal infrastructure for all languages

---

### **TDD 1.1: Engine Ready Detection**

**Test:** Can detect when Godot engine is fully initialized

**Implementation:**
```c
// gdext-c/src/lifecycle/gdext_c_lifecycle.c

int gdext_c_is_engine_ready(void) {
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    if (!iface) return 0;
    
    // Check 1: SceneTree exists
    // Check 2: RenderingDevice initialized (for GPU operations)
    // Check 3: Engine singleton accessible
    
    // Return 1 if all checks pass, 0 otherwise
}
```

**Test Command:**
```bash
cd gdext-c && make test_lifecycle
# Expected: Test passes when engine is ready
```

**Success Criteria:**
- ✅ Returns 0 before engine ready
- ✅ Returns 1 after engine ready
- ✅ Can be called multiple times safely

---

### **TDD 1.2: Callback Registration**

**Test:** Can register language callbacks

**Implementation:**
```c
// gdext-c/src/lifecycle/gdext_c_lifecycle.c

static gdext_c_ready_callback g_ready_callback = NULL;
static gdext_c_process_callback g_process_callback = NULL;
static gdext_c_physics_callback g_physics_callback = NULL;
static gdext_c_shutdown_callback g_shutdown_callback = NULL;

void gdext_c_register_lifecycle_callbacks(
    gdext_c_ready_callback ready,
    gdext_c_process_callback process,
    gdext_c_physics_callback physics,
    gdext_c_shutdown_callback shutdown
) {
    g_ready_callback = ready;
    g_process_callback = process;
    g_physics_callback = physics;
    g_shutdown_callback = shutdown;
    
    printf("[gdext-c] ✅ Lifecycle callbacks registered\n");
}
```

**Test Command:**
```c
// Test function
void test_callback_registration() {
    int ready_called = 0;
    
    void test_ready() { ready_called = 1; }
    
    gdext_c_register_lifecycle_callbacks(test_ready, NULL, NULL, NULL);
    
    // Trigger ready callback
    g_ready_callback();
    
    assert(ready_called == 1);
    printf("✅ Callback registration test passed\n");
}
```

**Success Criteria:**
- ✅ Callbacks stored correctly
- ✅ Can be called successfully
- ✅ NULL callbacks are safe (no crash)

---

### **TDD 1.3: SceneTree Signal Connection**

**Test:** Can connect to SceneTree signals

**Implementation:**
```c
// gdext-c/src/lifecycle/gdext_c_lifecycle.c

static void* g_scene_tree = NULL;
static void* g_process_callable = NULL;
static void* g_physics_callable = NULL;

int gdext_c_connect_to_scene_tree(void) {
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    
    // Get Engine singleton
    // Get SceneTree from Engine.get_main_loop()
    // Create Callable for process_frame
    // Create Callable for physics_frame
    // Connect signals
    
    printf("[gdext-c] ✅ Connected to SceneTree signals\n");
    return 1;
}
```

**Test Command:**
```bash
# Run game for 5 seconds, verify callbacks fire
gdextctl playtest --timeout 5 --simulate
grep "process_frame callback" test.log
# Expected: Multiple process_frame callbacks logged
```

**Success Criteria:**
- ✅ SceneTree singleton retrieved
- ✅ Signals connected successfully
- ✅ No memory leaks

---

### **TDD 1.4: Callback Dispatch from Signals**

**Test:** Signals trigger registered callbacks

**Implementation:**
```c
// gdext-c/src/lifecycle/gdext_c_lifecycle.c

// Signal handler for process_frame
static void on_process_frame_signal(void* userdata, int32_t arg_count, void** args) {
    (void)userdata;
    (void)arg_count;
    
    // Extract delta from args
    double delta = 0.016; // TODO: Get actual delta
    
    // Dispatch to registered callback
    if (g_process_callback) {
        g_process_callback(delta);
    }
}

// Signal handler for physics_frame
static void on_physics_frame_signal(void* userdata, int32_t arg_count, void** args) {
    (void)userdata;
    (void)arg_count;
    
    double delta = 0.016; // TODO: Get actual delta
    
    if (g_physics_callback) {
        g_physics_callback(delta);
    }
}
```

**Test Command:**
```c
// Test callback dispatch
int process_count = 0;
void test_process(double delta) { process_count++; }

gdext_c_register_lifecycle_callbacks(NULL, test_process, NULL, NULL);
gdext_c_run_game_loop(); // Run for 1 second

assert(process_count >= 50); // At least 50 frames in 1 second
printf("✅ Callback dispatch test passed (%d frames)\n", process_count);
```

**Success Criteria:**
- ✅ Process callback fires every frame
- ✅ Physics callback fires at 60 Hz
- ✅ Delta values are reasonable (0.016 ± 0.005)

---

### **TDD 1.5: Game Loop (Blocking)**

**Test:** `gdext_c_run_game_loop()` blocks until shutdown

**Implementation:**
```c
void gdext_c_run_game_loop(void) {
    printf("[gdext-c] 🎮 Starting game loop...\n");
    
    // Connect to SceneTree signals
    if (!gdext_c_connect_to_scene_tree()) {
        fprintf(stderr, "[gdext-c] ❌ Failed to connect to SceneTree\n");
        return;
    }
    
    // Trigger ready callback
    if (g_ready_callback) {
        printf("[gdext-c] 🎯 Calling ready callback...\n");
        g_ready_callback();
    }
    
    // Block until engine shuts down
    // (Godot's signals will keep calling our callbacks)
    // This is handled by Godot's main loop
    
    printf("[gdext-c] ✅ Game loop started (blocking until shutdown)\n");
    
    // TODO: How to block here? We need to wait for Godot to shut down
    // Option: Use a semaphore that shutdown callback signals
}
```

**Test Command:**
```bash
# Start game, let it run 10 seconds, verify it shuts down cleanly
timeout 12 gdextctl playtest --timeout 10 --simulate
echo $? # Should be 0 (clean exit)
```

**Success Criteria:**
- ✅ Blocks until shutdown
- ✅ Callbacks fire during game loop
- ✅ Clean exit (exit code 0)

---

## 📋 **Phase 2: gdext-go Lifecycle Wrapper**

**Duration:** 1-2 hours  
**Goal:** Go-idiomatic wrapper around gdext-c

---

### **TDD 2.1: Go CGO Bindings**

**Test:** Go can call gdext-c lifecycle functions

**Implementation:**
```go
// gdext-go/pkg/lifecycle/lifecycle.go
package lifecycle

/*
#cgo CFLAGS: -I${SRCDIR}/../../gdext-c/include
#cgo LDFLAGS: -L${SRCDIR}/../../gdext-c -lgdext_c
#include "gdext_c_lifecycle.h"
*/
import "C"

// IsEngineReady returns true if Godot is fully initialized
func IsEngineReady() bool {
    return C.gdext_c_is_engine_ready() != 0
}

// WaitForEngineReady blocks until Godot is ready
func WaitForEngineReady() {
    C.gdext_c_wait_for_engine_ready()
}

// RunGameLoop blocks until Godot shuts down
func RunGameLoop() {
    C.gdext_c_run_game_loop()
}
```

**Test Command:**
```bash
cd gdext-go/pkg/lifecycle && go test -v
```

**Test Code:**
```go
func TestIsEngineReady(t *testing.T) {
    // Before engine starts, should return false
    if IsEngineReady() {
        t.Error("Engine should not be ready yet")
    }
}

func TestWaitForEngineReady(t *testing.T) {
    // This will block until engine ready
    // (requires actual Godot running)
    WaitForEngineReady()
    
    // After waiting, should be ready
    if !IsEngineReady() {
        t.Error("Engine should be ready after waiting")
    }
}
```

**Success Criteria:**
- ✅ Compiles without errors
- ✅ CGO linkage works
- ✅ Functions callable from Go

---

### **TDD 2.2: Go Callback Exports**

**Test:** Go callbacks can be registered with C

**Implementation:**
```go
// gdext-go/pkg/lifecycle/callbacks.go
package lifecycle

import "C"

var (
    readyCallback    func()
    processCallback  func(delta float64)
    physicsCallback  func(delta float64)
    shutdownCallback func()
)

// RegisterCallbacks registers Go callbacks
func RegisterCallbacks(
    ready func(),
    process func(delta float64),
    physics func(delta float64),
    shutdown func(),
) {
    readyCallback = ready
    processCallback = process
    physicsCallback = physics
    shutdownCallback = shutdown
    
    // Register with C
    C.gdext_c_register_lifecycle_callbacks(
        C.gdext_c_ready_callback(C.go_ready_trampoline),
        C.gdext_c_process_callback(C.go_process_trampoline),
        C.gdext_c_physics_callback(C.go_physics_trampoline),
        C.gdext_c_shutdown_callback(C.go_shutdown_trampoline),
    )
}

//export go_ready_trampoline
func go_ready_trampoline() {
    if readyCallback != nil {
        readyCallback()
    }
}

//export go_process_trampoline
func go_process_trampoline(delta C.double) {
    if processCallback != nil {
        processCallback(float64(delta))
    }
}

//export go_physics_trampoline
func go_physics_trampoline(delta C.double) {
    if physicsCallback != nil {
        physicsCallback(float64(delta))
    }
}

//export go_shutdown_trampoline
func go_shutdown_trampoline() {
    if shutdownCallback != nil {
        shutdownCallback()
    }
}
```

**Test Command:**
```go
func TestCallbackRegistration(t *testing.T) {
    readyCalled := false
    processCalled := false
    
    RegisterCallbacks(
        func() { readyCalled = true },
        func(delta float64) { processCalled = true },
        nil,
        nil,
    )
    
    // Trigger callbacks (requires engine)
    // ...
    
    if !readyCalled {
        t.Error("Ready callback not called")
    }
    if !processCalled {
        t.Error("Process callback not called")
    }
}
```

**Success Criteria:**
- ✅ Callbacks export correctly
- ✅ Can be called from C
- ✅ Go functions execute

---

## 📋 **Phase 3: Framework Integration**

**Duration:** 1 hour  
**Goal:** Integrate into action-adventure-framework

---

### **TDD 3.1: Refactor main.go**

**Test:** Game initializes using new lifecycle

**Implementation:**
```go
// action-adventure-framework/main.go
package main

import (
    "fmt"
    "github.com/jeffreyfriedman/action-adventure-framework/src/game"
    "github.com/jeffreyfriedman/gdext-go/pkg/lifecycle"
)

var theGame *game.Game

func main() {
    fmt.Println("[Main] 🎮 Starting with lifecycle system...")
    
    // Wait for Godot to be ready
    lifecycle.WaitForEngineReady()
    fmt.Println("[Main] ✅ Engine ready!")
    
    // Register callbacks
    lifecycle.RegisterCallbacks(
        onReady,
        onProcess,
        onPhysicsProcess,
        onShutdown,
    )
    
    // Run game loop (blocks until shutdown)
    lifecycle.RunGameLoop()
    
    fmt.Println("[Main] ✅ Clean shutdown complete")
}

func onReady() {
    fmt.Println("[Main] 🎯 Ready callback - initializing game...")
    theGame = game.NewGame()
    fmt.Println("[Main] ✅ Game initialized")
}

func onProcess(delta float64) {
    // Optional: Process callback if needed
}

func onPhysicsProcess(delta float64) {
    if theGame != nil {
        theGame.Update(delta)
    }
}

func onShutdown() {
    fmt.Println("[Main] 🧹 Shutdown callback - cleaning up...")
    if theGame != nil {
        theGame.Cleanup()
    }
    fmt.Println("[Main] ✅ Cleanup complete")
}
```

**Test Command:**
```bash
cd action-adventure-framework
go build -buildmode=c-shared -o bin/macos/game_logic.dylib .
gdextctl playtest --timeout 10 --simulate
echo $? # Should be 0
```

**Success Criteria:**
- ✅ Compiles successfully
- ✅ Game initializes
- ✅ Runs for 10 seconds
- ✅ Clean shutdown (exit code 0)

---

### **TDD 3.2: Remove Old GameNode Code**

**Test:** No GameNode dependencies remain

**Commands:**
```bash
# Remove GameNode registration from gdext-c
# (Already removed from .tscn files)

# Verify no GameNode references
grep -r "GameNode" gdext-c/ action-adventure-framework/
# Expected: No results (except comments/docs)
```

**Success Criteria:**
- ✅ No GameNode class registration
- ✅ No GameNode in scene files
- ✅ No GameNode references in code

---

### **TDD 3.3: Clean Shutdown Test**

**Test:** Game shuts down without crashes

**Test Scenarios:**
```bash
# Test 1: Short run (10s)
gdextctl playtest --timeout 10 --simulate
echo "Exit code: $?"

# Test 2: Medium run (30s)
gdextctl playtest --timeout 30 --simulate
echo "Exit code: $?"

# Test 3: Long run (60s)
gdextctl playtest --timeout 60 --simulate
echo "Exit code: $?"

# Test 4: Multiple consecutive runs
for i in {1..5}; do
    gdextctl playtest --timeout 15 --simulate || exit 1
done
echo "All 5 runs passed!"
```

**Success Criteria:**
- ✅ Exit code 0 every time
- ✅ No crash messages
- ✅ No "handle_crash" in logs
- ✅ No "propagate_notification" errors

---

### **TDD 3.4: All Features Work**

**Test:** All 24 AAA features still functional

**Test Command:**
```bash
# Run comprehensive feature test
gdextctl playtest --scenario test_scenarios/vertical_slice.yaml --simulate
```

**Features to Verify:**
- ✅ Movement (WASD)
- ✅ Combat (shooting)
- ✅ Camera (mouse look)
- ✅ Health system
- ✅ Quests
- ✅ Inventory
- ✅ Skills
- ✅ (21 more features...)

**Success Criteria:**
- ✅ All features work as before
- ✅ No regressions
- ✅ Performance unchanged

---

## 📊 **Progress Tracking**

### **Phase 1: gdext-c** (2-3 hours)
- [ ] TDD 1.1: Engine ready detection
- [ ] TDD 1.2: Callback registration
- [ ] TDD 1.3: SceneTree signal connection
- [ ] TDD 1.4: Callback dispatch
- [ ] TDD 1.5: Game loop blocking

### **Phase 2: gdext-go** (1-2 hours)
- [ ] TDD 2.1: Go CGO bindings
- [ ] TDD 2.2: Go callback exports

### **Phase 3: Framework** (1 hour)
- [ ] TDD 3.1: Refactor main.go
- [ ] TDD 3.2: Remove GameNode
- [ ] TDD 3.3: Clean shutdown test
- [ ] TDD 3.4: All features work

---

## ✅ **Final Acceptance Criteria**

**Ship when ALL of these are true:**

- [ ] Game initializes cleanly
- [ ] Game runs for 60+ seconds
- [ ] All 24 AAA features work
- [ ] Clean shutdown (exit code 0)
- [ ] No crash messages
- [ ] No threading violations
- [ ] 5 consecutive test runs succeed
- [ ] Manual window close works

**Only when ALL checkboxes checked → 100% COMPLETE!**

---

**Estimated Total Time:** 4-6 hours  
**Success Probability:** 90% (based on graphics.gd proof)  
**Benefit:** Universal solution for all GDExtension language bindings!



