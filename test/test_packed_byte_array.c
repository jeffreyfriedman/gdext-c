#include "gdext_c_packed_byte_array.h"
#include <stdio.h>
#include <assert.h>

// TDD #152: PackedByteArray tests

void test_create_empty() {
    printf("Test 1: Create empty PackedByteArray...\n");
    
    gdext_c_packed_byte_array_t arr;
    gdext_c_packed_byte_array_create(&arr);
    
    size_t size = gdext_c_packed_byte_array_size(&arr);
    assert(size == 0);
    
    gdext_c_packed_byte_array_destroy(&arr);
    
    printf("✅ Test 1 PASSED: Empty array has size 0\n");
}

void test_create_from_bytes() {
    printf("Test 2: Create PackedByteArray from bytes...\n");
    
    uint8_t data[] = {1, 2, 3, 4, 5, 10, 20, 30, 40, 50};
    gdext_c_packed_byte_array_t arr;
    gdext_c_packed_byte_array_from_bytes(&arr, data, 10);
    
    size_t size = gdext_c_packed_byte_array_size(&arr);
    assert(size == 10);
    
    printf("✅ Test 2 PASSED: Array has size 10\n");
    
    // Verify data
    for (size_t i = 0; i < 10; i++) {
        uint8_t value = gdext_c_packed_byte_array_get(&arr, i);
        assert(value == data[i]);
    }
    
    printf("✅ Test 2 PASSED: All bytes match\n");
    
    gdext_c_packed_byte_array_destroy(&arr);
}

void test_set_and_get() {
    printf("Test 3: Set and get bytes...\n");
    
    uint8_t data[] = {100, 200};
    gdext_c_packed_byte_array_t arr;
    gdext_c_packed_byte_array_from_bytes(&arr, data, 2);
    
    // Modify
    gdext_c_packed_byte_array_set(&arr, 0, 50);
    gdext_c_packed_byte_array_set(&arr, 1, 150);
    
    // Verify
    assert(gdext_c_packed_byte_array_get(&arr, 0) == 50);
    assert(gdext_c_packed_byte_array_get(&arr, 1) == 150);
    
    printf("✅ Test 3 PASSED: Set/get works correctly\n");
    
    gdext_c_packed_byte_array_destroy(&arr);
}

void test_svo_use_case() {
    printf("Test 4: SVO renderer use case (large byte array)...\n");
    
    // Simulate octree data (1KB)
    uint8_t octree_data[1024];
    for (size_t i = 0; i < 1024; i++) {
        octree_data[i] = (uint8_t)(i % 256);
    }
    
    gdext_c_packed_byte_array_t arr;
    gdext_c_packed_byte_array_from_bytes(&arr, octree_data, 1024);
    
    size_t size = gdext_c_packed_byte_array_size(&arr);
    assert(size == 1024);
    
    // Spot check
    assert(gdext_c_packed_byte_array_get(&arr, 0) == 0);
    assert(gdext_c_packed_byte_array_get(&arr, 255) == 255);
    assert(gdext_c_packed_byte_array_get(&arr, 256) == 0);
    assert(gdext_c_packed_byte_array_get(&arr, 1023) == 255);
    
    printf("✅ Test 4 PASSED: Large array (1KB) works correctly\n");
    
    gdext_c_packed_byte_array_destroy(&arr);
}

int main() {
    printf("\n");
    printf("═══════════════════════════════════════════════\n");
    printf("  TDD #152: PackedByteArray Test Suite\n");
    printf("═══════════════════════════════════════════════\n");
    printf("\n");
    
    printf("⚠️  NOTE: These tests require Godot engine to be initialized!\n");
    printf("⚠️  Run via: gdextctl test (not standalone)\n");
    printf("\n");
    
    test_create_empty();
    test_create_from_bytes();
    test_set_and_get();
    test_svo_use_case();
    
    printf("\n");
    printf("═══════════════════════════════════════════════\n");
    printf("  ✅ ALL TESTS PASSED (4/4)\n");
    printf("═══════════════════════════════════════════════\n");
    printf("\n");
    
    return 0;
}



