// TDD #99: MINIMAL C GameNode - Just enough for game loop callbacks (NO GDSCRIPT!)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../c-bridge/gdextension_interface.h"

// Forward declarations
extern void* gdext_c_entry_get_proc_address(void);

// Go callback function pointers (set by c_register_go_callbacks)
extern void (*go_ready_callback)();
extern void (*go_process_callback)(double);
extern void (*go_physics_process_callback)(double);

// GameNode instance data
// TDD #101: Simple instance structure
typedef struct {
    void* godot_object_ptr;  // Pointer to the actual Godot object
    int ready_called;         // Track if _ready was called
    char padding[56];         // Extra padding to ensure enough space
} GameNodeData;

// Godot notification constants
#define NOTIFICATION_READY 13
#define NOTIFICATION_PHYSICS_PROCESS 16
#define NOTIFICATION_PROCESS 17

// Forward declarations
static void gamenode_notification(GDExtensionClassInstancePtr instance, int32_t what, GDExtensionBool reversed);
static GDExtensionBool gamenode_set(GDExtensionClassInstancePtr instance, GDExtensionConstStringNamePtr name, GDExtensionConstVariantPtr value);
static GDExtensionBool gamenode_get(GDExtensionClassInstancePtr instance, GDExtensionConstStringNamePtr name, GDExtensionVariantPtr ret);

// GDExtension function pointers
static GDExtensionInterfaceClassdbRegisterExtensionClass3 classdb_register_class3 = NULL;
static GDExtensionInterfaceClassdbConstructObject classdb_construct_object = NULL;
static GDExtensionInterfaceObjectSetInstance object_set_instance = NULL;
static GDExtensionInterfaceClassdbGetMethodBind classdb_get_method_bind = NULL;
static GDExtensionInterfaceObjectMethodBindPtrcall object_method_bind_ptrcall = NULL;

// TDD #109: String name helper (PURE C, not Rust!)
// Implemented in string_name_helper.c - uses Godot C API via proc_address
extern void* gdext_create_string_name(const char* str);

// Initialize function pointers
static void init_classdb_functions() {
    if (classdb_register_class3) return;
    
    void* proc_addr = gdext_c_entry_get_proc_address();
    if (!proc_addr) {
        fprintf(stderr, "[GameNode] ❌ proc_address is NULL\n");
        return;
    }
    
    typedef void* (*GetProcFunc)(const char*);
    GetProcFunc get_proc = (GetProcFunc)proc_addr;
    
    classdb_register_class3 = (GDExtensionInterfaceClassdbRegisterExtensionClass3)
        get_proc("classdb_register_extension_class3");
    classdb_construct_object = (GDExtensionInterfaceClassdbConstructObject)
        get_proc("classdb_construct_object");
    object_set_instance = (GDExtensionInterfaceObjectSetInstance)
        get_proc("object_set_instance");
    classdb_get_method_bind = (GDExtensionInterfaceClassdbGetMethodBind)
        get_proc("classdb_get_method_bind");
    object_method_bind_ptrcall = (GDExtensionInterfaceObjectMethodBindPtrcall)
        get_proc("object_method_bind_ptrcall");
    
    printf("[GameNode] 🎯 TDD #102: Got function pointers:\n");
    printf("[GameNode]    classdb_register_class3: %p\n", (void*)classdb_register_class3);
    printf("[GameNode]    classdb_construct_object: %p\n", (void*)classdb_construct_object);
    printf("[GameNode]    object_set_instance: %p\n", (void*)object_set_instance);
    printf("[GameNode]    classdb_get_method_bind: %p\n", (void*)classdb_get_method_bind);
    printf("[GameNode]    object_method_bind_ptrcall: %p\n", (void*)object_method_bind_ptrcall);
    fflush(stdout);
}

// Instance creation - TDD #102: THE CORRECT PATTERN!
static void* gamenode_create(void* userdata) {
    printf("[GameNode] 🎮 TDD #102: Creating GameNode instance...\n");
    fflush(stdout);
    
    // STEP 1: Create the PARENT class object (Node, NOT GameNode!)
    // This avoids infinite recursion
    void* parent_class_name = gdext_create_string_name("Node");
    void* godot_object = classdb_construct_object(parent_class_name);
    
    if (!godot_object) {
        fprintf(stderr, "[GameNode] ❌ classdb_construct_object(Node) failed!\n");
        return NULL;
    }
    
    printf("[GameNode] ✅ TDD #102: Created Node object: %p\n", godot_object);
    fflush(stdout);
    
    // STEP 2: Allocate our extension data
    GameNodeData* extension_data = (GameNodeData*)malloc(sizeof(GameNodeData));
    if (!extension_data) {
        fprintf(stderr, "[GameNode] ❌ malloc failed!\n");
        return NULL;
    }
    
    // Initialize extension data
    memset(extension_data, 0, sizeof(GameNodeData));
    extension_data->godot_object_ptr = godot_object;
    extension_data->ready_called = 0;
    
    printf("[GameNode] ✅ TDD #102: Created extension data: %p\n", extension_data);
    fflush(stdout);
    
    // STEP 3: Bind extension data to Godot object with OUR class name
    void* our_class_name = gdext_create_string_name("GameNode");
    object_set_instance(godot_object, our_class_name, extension_data);
    
    printf("[GameNode] ✅ TDD #102: Bound extension data to Node object as GameNode!\n");
    printf("[GameNode] 🎯 TDD #102: Node→GameNode pattern (avoids recursion)!\n");
    fflush(stdout);
    
    // STEP 4: TDD #108: Enable process notifications!
    printf("[GameNode] 🔧 TDD #108: Enabling process notifications...\n");
    fflush(stdout);
    
    // Get method binds for set_process and set_physics_process
    void* node_class = gdext_create_string_name("Node");
    void* set_process_name = gdext_create_string_name("set_process");
    void* set_physics_process_name = gdext_create_string_name("set_physics_process");
    
    void* set_process_bind = classdb_get_method_bind(node_class, set_process_name, 2586408642); // hash from extension_api.json
    void* set_physics_process_bind = classdb_get_method_bind(node_class, set_physics_process_name, 2586408642); // hash from extension_api.json
    
    if (set_process_bind && set_physics_process_bind) {
        // Call set_process(true)
        uint8_t enable_true = 1;
        const void* args1[] = { &enable_true };
        object_method_bind_ptrcall(set_process_bind, godot_object, args1, NULL);
        
        // Call set_physics_process(true)
        object_method_bind_ptrcall(set_physics_process_bind, godot_object, args1, NULL);
        
        printf("[GameNode] ✅ TDD #108: Process notifications enabled!\n");
        fflush(stdout);
    } else {
        fprintf(stderr, "[GameNode] ❌ TDD #108: Failed to get method binds for set_process/set_physics_process!\n");
        fflush(stderr);
    }
    
    // STEP 5: Return the Godot object (NOT our extension data!)
    return godot_object;
}

// Instance destruction
static void gamenode_free(void* userdata, void* instance) {
    printf("[GameNode] 🔽 Freeing instance...\n");
    free(instance);
}

// TDD #101: Notification handler - MUCH simpler than virtual methods!
// This single function handles _ready, _process, and _physics_process via notifications
static void gamenode_notification(GDExtensionClassInstancePtr instance, int32_t what, GDExtensionBool reversed) {
    // TDD #103: Counters for process/physics_process
    static int process_count = 0;
    static int physics_count = 0;
    
    // Only process forward notifications (reversed is for cleanup)
    if (reversed) {
        return;
    }
    
    switch (what) {
        case NOTIFICATION_READY:
            printf("\n");
            printf("==================================================================\n");
            printf("[GameNode] 🎮🎮🎮 TDD #101: NOTIFICATION_READY received!!!\n");
            printf("[GameNode] 🎮 instance=%p\n", instance);
            printf("==================================================================\n");
            fflush(stdout);
            
            if (go_ready_callback) {
                printf("[GameNode] 🎮 Calling Go ready callback...\n");
                fflush(stdout);
                go_ready_callback();
                printf("[GameNode] ✅ Go ready callback complete!\n");
                fflush(stdout);
            } else {
                printf("[GameNode] ❌ go_ready_callback is NULL!\n");
                fflush(stdout);
            }
            break;
            
        case NOTIFICATION_PROCESS:
            // Called every frame
            if (go_process_callback) {
                go_process_callback(0.016); // ~60 FPS
                process_count++;
                if (process_count == 1 || process_count % 60 == 0) {
                    printf("[GameNode] 🎮 TDD #103: _process called %d times (60 FPS)\n", process_count);
                    fflush(stdout);
                }
            }
            break;
            
        case NOTIFICATION_PHYSICS_PROCESS:
            // Called every physics frame (60 Hz)
            if (go_physics_process_callback) {
                go_physics_process_callback(0.016); // ~60 FPS
                physics_count++;
                if (physics_count == 1 || physics_count % 60 == 0) {
                    printf("[GameNode] 🎮 TDD #103: _physics_process called %d times (60 Hz)\n", physics_count);
                    fflush(stdout);
                }
            }
            break;
            
        default:
            // Ignore other notifications
            break;
    }
}

// TDD #101: Dummy property set (GameNode has no properties)
static GDExtensionBool gamenode_set(GDExtensionClassInstancePtr instance, GDExtensionConstStringNamePtr name, GDExtensionConstVariantPtr value) {
    // Return 0 (false) to indicate property not handled
    return 0;
}

// TDD #101: Dummy property get (GameNode has no properties)
static GDExtensionBool gamenode_get(GDExtensionClassInstancePtr instance, GDExtensionConstStringNamePtr name, GDExtensionVariantPtr ret) {
    // Return 0 (false) to indicate property not handled
    return 0;
}

// Register GameNode class
void register_gamenode_minimal_c(void* library) {
    printf("[GameNode] 📋 TDD #99: Registering MINIMAL C GameNode (NO GDSCRIPT!)\n");
    printf("[GameNode] 🎯 Pure C + Go game loop!\n");
    fflush(stdout);
    
    init_classdb_functions();
    
    if (!classdb_register_class3) {
        fprintf(stderr, "[GameNode] ❌ classdb_register_class3 is NULL\n");
        return;
    }
    
    // TDD #101: Minimal setup - just enough to not crash
    GDExtensionClassCreationInfo3 class_info = {0};
    class_info.is_virtual = 0;
    class_info.is_abstract = 0;
    class_info.is_exposed = 1;  // Must be exposed to use in scenes
    class_info.is_runtime = 0;
    
    // TDD #101: Try setting ALL function pointers to prevent crashes
    class_info.set_func = gamenode_set;
    class_info.get_func = gamenode_get;
    class_info.get_property_list_func = NULL;
    class_info.free_property_list_func = NULL;
    class_info.property_can_revert_func = NULL;
    class_info.property_get_revert_func = NULL;
    class_info.validate_property_func = NULL;
    class_info.notification_func = gamenode_notification;
    class_info.to_string_func = NULL;
    class_info.reference_func = NULL;
    class_info.unreference_func = NULL;
    class_info.create_instance_func = gamenode_create;
    class_info.free_instance_func = gamenode_free;
    class_info.recreate_instance_func = NULL;
    class_info.get_virtual_func = NULL;
    class_info.get_virtual_call_data_func = NULL;
    class_info.call_virtual_with_data_func = NULL;
    class_info.get_rid_func = NULL;
    class_info.class_userdata = NULL;
    
    printf("[GameNode] 🎯 TDD #101: Using notification_func instead of virtual methods!\n");
    printf("[GameNode] 🎯 TDD #101: notification_func = %p\n", (void*)gamenode_notification);
    printf("[GameNode] 🎯 TDD #101: This is MUCH simpler than get_virtual_func!\n");
    fflush(stdout);
    
    // Use string names from helper
    void* class_name = gdext_create_string_name("GameNode");
    void* parent_name = gdext_create_string_name("Node");
    
    // Register class with modern API
    classdb_register_class3(library, class_name, parent_name, &class_info);
    
    printf("[GameNode] ✅ GameNode class registered!\n");
    printf("[GameNode] 💡 TDD #101: Using notification callbacks instead of virtual methods!\n");
    printf("[GameNode] 💡 TDD #101: NOTIFICATION_READY=%d, PROCESS=%d, PHYSICS=%d\n", 
           NOTIFICATION_READY, NOTIFICATION_PROCESS, NOTIFICATION_PHYSICS_PROCESS);
    printf("[GameNode] ✅ TDD #101: Pure C GameNode complete (NO GDSCRIPT, NO RUST!)!\n");
    fflush(stdout);
}

