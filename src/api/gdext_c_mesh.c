/**
 * gdext_c_mesh.c - Direct ArrayMesh mesh pipeline
 * 
 * Provides ArrayMesh.add_surface_from_arrays via object_method_bind_call,
 * which is the correct GDExtension calling convention for Object-derived classes.
 * 
 * The generic gdext_call_method (variant_call) wraps the Object receiver in a
 * Variant, causing double-wrapping issues with add_surface_from_arrays.
 * object_method_bind_call takes the raw ObjectPtr directly.
 */

#include "../../include/gdext_c.h"
#include "../core/gdext_c_core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Error codes for gdext_mesh_add_surface_from_arrays */
#define MESH_ERR_NOT_INITIALIZED   -1
#define MESH_ERR_NULL_MESH         -2
#define MESH_ERR_NULL_ARRAYS       -3
#define MESH_ERR_NO_METHOD_BIND    -4
#define MESH_ERR_VARIANT_ALLOC     -5
#define MESH_ERR_RET_ALLOC         -6
#define MESH_ERR_CALL_FAILED       -7

// Cached method binds
static GDExtensionMethodBindPtr g_add_surface_mb = NULL;
static GDExtensionMethodBindPtr g_set_flag_mb = NULL;          // BaseMaterial3D.set_flag
static GDExtensionMethodBindPtr g_set_albedo_mb = NULL;        // BaseMaterial3D.set_albedo
static GDExtensionMethodBindPtr g_surface_set_material_mb = NULL; // Mesh.surface_set_material

static void ensure_mesh_bindings_cached(void) {
    const GDExtensionInterface* iface = gdext_c_get_interface_functions();
    if (!iface) return;

    GDExtensionPtrDestructor sn_dtor = iface->variant_get_ptr_destructor(GDEXTENSION_VARIANT_TYPE_STRING_NAME);

    if (g_add_surface_mb == NULL) {
        char class_sn[64] = {0};
        iface->string_name_new_with_latin1_chars(class_sn, "ArrayMesh", 0);
        char method_sn[64] = {0};
        iface->string_name_new_with_latin1_chars(method_sn, "add_surface_from_arrays", 0);
        // Hash 1796411378: add_surface_from_arrays(primitive, arrays, blend_shapes, lods, flags)
        g_add_surface_mb = iface->classdb_get_method_bind(class_sn, method_sn, 1796411378);
        if (!g_add_surface_mb) {
            fprintf(stderr, "[gdext-c] MESH ERROR: Failed to get method bind for ArrayMesh.add_surface_from_arrays\n");
        }
        if (sn_dtor) { sn_dtor(class_sn); sn_dtor(method_sn); }
    }

    if (g_set_flag_mb == NULL) {
        char class_sn[64] = {0};
        iface->string_name_new_with_latin1_chars(class_sn, "BaseMaterial3D", 0);
        char method_sn[64] = {0};
        iface->string_name_new_with_latin1_chars(method_sn, "set_flag", 0);
        // Hash 3070159527: set_flag(flag: BaseMaterial3D.Flags, enable: bool)
        g_set_flag_mb = iface->classdb_get_method_bind(class_sn, method_sn, 3070159527);
        if (!g_set_flag_mb) {
            fprintf(stderr, "[gdext-c] MESH ERROR: Failed to get method bind for BaseMaterial3D.set_flag\n");
        }
        if (sn_dtor) { sn_dtor(class_sn); sn_dtor(method_sn); }
    }

    if (g_set_albedo_mb == NULL) {
        char class_sn[64] = {0};
        iface->string_name_new_with_latin1_chars(class_sn, "BaseMaterial3D", 0);
        char method_sn[64] = {0};
        iface->string_name_new_with_latin1_chars(method_sn, "set_albedo", 0);
        // Hash 2920490490: set_albedo(albedo: Color)
        g_set_albedo_mb = iface->classdb_get_method_bind(class_sn, method_sn, 2920490490);
        if (!g_set_albedo_mb) {
            fprintf(stderr, "[gdext-c] MESH ERROR: Failed to get method bind for BaseMaterial3D.set_albedo\n");
        }
        if (sn_dtor) { sn_dtor(class_sn); sn_dtor(method_sn); }
    }

    if (g_surface_set_material_mb == NULL) {
        char class_sn[64] = {0};
        iface->string_name_new_with_latin1_chars(class_sn, "Mesh", 0);
        char method_sn[64] = {0};
        iface->string_name_new_with_latin1_chars(method_sn, "surface_set_material", 0);
        // Hash 3671737478: surface_set_material(surf_idx: int, material: Material)
        g_surface_set_material_mb = iface->classdb_get_method_bind(class_sn, method_sn, 3671737478);
        if (!g_surface_set_material_mb) {
            fprintf(stderr, "[gdext-c] MESH ERROR: Failed to get method bind for Mesh.surface_set_material\n");
        }
        if (sn_dtor) { sn_dtor(class_sn); sn_dtor(method_sn); }
    }
}

/**
 * Call ArrayMesh.add_surface_from_arrays using object_method_bind_call.
 *
 * @param mesh_object   Raw GDExtensionObjectPtr for the ArrayMesh
 * @param primitive_type Mesh.PrimitiveType enum value (e.g. PRIMITIVE_TRIANGLES=3)
 * @param arrays_variant Variant containing the mesh data Array[13]
 * @return 0 on success, negative error code on failure
 */
int gdext_mesh_add_surface_from_arrays(void* mesh_object, int primitive_type, void* arrays_variant) {
    if (!gdext_c_is_initialized()) return MESH_ERR_NOT_INITIALIZED;
    if (!mesh_object) return MESH_ERR_NULL_MESH;
    if (!arrays_variant) return MESH_ERR_NULL_ARRAYS;

    ensure_mesh_bindings_cached();
    if (!g_add_surface_mb) return MESH_ERR_NO_METHOD_BIND;

    const GDExtensionInterface* iface = gdext_c_get_interface_functions();

    GDExtensionVariantPtr prim_variant = gdext_variant_from_int(primitive_type);
    if (!prim_variant) return MESH_ERR_VARIANT_ALLOC;

    GDExtensionVariantPtr ret = malloc(GDEXT_VARIANT_SIZE);
    if (!ret) {
        gdext_variant_free(prim_variant);
        return MESH_ERR_RET_ALLOC;
    }
    iface->variant_new_nil(ret);

    GDExtensionCallError error;
    memset(&error, 0, sizeof(error));

    // Only pass primitive_type and arrays; remaining 3 params use defaults
    GDExtensionConstVariantPtr args[2];
    args[0] = (GDExtensionConstVariantPtr)prim_variant;
    args[1] = (GDExtensionConstVariantPtr)arrays_variant;

    iface->object_method_bind_call(g_add_surface_mb, mesh_object, args, 2, ret, &error);

    gdext_variant_free(prim_variant);
    iface->variant_destroy(ret);
    free(ret);

    if (error.error != GDEXTENSION_CALL_OK) {
        fprintf(stderr, "[gdext-c] MESH ERROR: add_surface_from_arrays failed (error=%d, arg=%d, expected=%d)\n",
                error.error, error.argument, error.expected);
        return MESH_ERR_CALL_FAILED;
    }

    return 0;
}

/**
 * Create an ArrayMesh object via classdb_construct_object.
 *
 * @return GDExtensionObjectPtr for the new ArrayMesh, or NULL on failure
 */
void* gdext_create_array_mesh(void) {
    if (!gdext_c_is_initialized()) return NULL;

    const GDExtensionInterface* iface = gdext_c_get_interface_functions();

    char class_sn[64] = {0};
    iface->string_name_new_with_latin1_chars(class_sn, "ArrayMesh", 0);

    GDExtensionObjectPtr obj = iface->classdb_construct_object(class_sn);

    GDExtensionPtrDestructor sn_dtor = iface->variant_get_ptr_destructor(GDEXTENSION_VARIANT_TYPE_STRING_NAME);
    if (sn_dtor) sn_dtor(class_sn);

    if (!obj) {
        fprintf(stderr, "[gdext-c] MESH ERROR: Failed to construct ArrayMesh\n");
    }

    return obj;
}

/**
 * Create a StandardMaterial3D with FLAG_ALBEDO_FROM_VERTEX_COLOR enabled,
 * and apply it to surface 0 of the given ArrayMesh.
 *
 * This uses object_method_bind_call (same as add_surface_from_arrays fix)
 * to bypass the variant_call double-wrapping issue.
 *
 * @param mesh_object Raw GDExtensionObjectPtr for the ArrayMesh
 * @return 0 on success, negative error code on failure
 */
int gdext_mesh_apply_vertex_color_material(void* mesh_object) {
    if (!gdext_c_is_initialized()) return MESH_ERR_NOT_INITIALIZED;
    if (!mesh_object) return MESH_ERR_NULL_MESH;

    ensure_mesh_bindings_cached();
    if (!g_set_flag_mb) return MESH_ERR_NO_METHOD_BIND;
    if (!g_surface_set_material_mb) return MESH_ERR_NO_METHOD_BIND;

    const GDExtensionInterface* iface = gdext_c_get_interface_functions();

    // 1. Create StandardMaterial3D
    char class_sn[64] = {0};
    iface->string_name_new_with_latin1_chars(class_sn, "StandardMaterial3D", 0);
    GDExtensionObjectPtr mat_obj = iface->classdb_construct_object(class_sn);
    GDExtensionPtrDestructor sn_dtor = iface->variant_get_ptr_destructor(GDEXTENSION_VARIANT_TYPE_STRING_NAME);
    if (sn_dtor) sn_dtor(class_sn);

    if (!mat_obj) {
        fprintf(stderr, "[gdext-c] MESH ERROR: Failed to construct StandardMaterial3D\n");
        return -8;
    }

    // 2. Call set_flag(FLAG_ALBEDO_FROM_VERTEX_COLOR=1, true) on the material
    {
        GDExtensionVariantPtr flag_var = gdext_variant_from_int(1); // FLAG_ALBEDO_FROM_VERTEX_COLOR = 1
        GDExtensionVariantPtr true_var = gdext_variant_from_bool(1);
        GDExtensionVariantPtr ret = malloc(GDEXT_VARIANT_SIZE);
        iface->variant_new_nil(ret);

        GDExtensionCallError error;
        memset(&error, 0, sizeof(error));

        GDExtensionConstVariantPtr args[2];
        args[0] = (GDExtensionConstVariantPtr)flag_var;
        args[1] = (GDExtensionConstVariantPtr)true_var;

        iface->object_method_bind_call(g_set_flag_mb, mat_obj, args, 2, ret, &error);

        gdext_variant_free(flag_var);
        gdext_variant_free(true_var);
        iface->variant_destroy(ret);
        free(ret);

        if (error.error != GDEXTENSION_CALL_OK) {
            fprintf(stderr, "[gdext-c] MESH ERROR: set_flag(ALBEDO_FROM_VERTEX_COLOR) failed (error=%d, arg=%d, expected=%d)\n",
                    error.error, error.argument, error.expected);
            return -9;
        }
    }

    // 2b. NEW: Set SHADING_MODE_UNSHADED to make player fully bright!
    // This makes the material ignore all scene lighting
    {
        char method_name[64] = {0};
        iface->string_name_new_with_latin1_chars(method_name, "set_shading_mode", 0);
        
        char class_name[64] = {0};
        iface->string_name_new_with_latin1_chars(class_name, "BaseMaterial3D", 0);
        
        // get_method_bind for set_shading_mode (hash: 3347911991)
        GDExtensionMethodBindPtr set_shading_mb = iface->classdb_get_method_bind(class_name, method_name, 3347911991);
        
        GDExtensionPtrDestructor name_dtor = iface->variant_get_ptr_destructor(GDEXTENSION_VARIANT_TYPE_STRING_NAME);
        if (name_dtor) {
            name_dtor(method_name);
            name_dtor(class_name);
        }
        
        if (set_shading_mb) {
            GDExtensionVariantPtr unshaded_var = gdext_variant_from_int(0); // SHADING_MODE_UNSHADED = 0
            GDExtensionVariantPtr ret = malloc(GDEXT_VARIANT_SIZE);
            iface->variant_new_nil(ret);
            
            GDExtensionCallError error;
            memset(&error, 0, sizeof(error));
            
            GDExtensionConstVariantPtr args[1];
            args[0] = (GDExtensionConstVariantPtr)unshaded_var;
            
            iface->object_method_bind_call(set_shading_mb, mat_obj, args, 1, ret, &error);
            
            gdext_variant_free(unshaded_var);
            iface->variant_destroy(ret);
            free(ret);
            
            if (error.error != GDEXTENSION_CALL_OK) {
                fprintf(stderr, "[gdext-c] MESH WARNING: set_shading_mode(UNSHADED) failed (error=%d)\n", error.error);
            }
        } else {
            fprintf(stderr, "[gdext-c] MESH WARNING: Could not find set_shading_mode method\n");
        }
    }

    // 2c. NEW: Add EMISSION GLOW to override all distance darkening!
    // This makes player glow brightly at ANY distance, overriding fog/AO/distance fade
    {
        char method_name[64] = {0};
        char class_name[64] = {0};
        iface->string_name_new_with_latin1_chars(class_name, "BaseMaterial3D", 0);
        
        // Enable emission feature (FEATURE_EMISSION = 5)
        iface->string_name_new_with_latin1_chars(method_name, "set_feature", 0);
        GDExtensionMethodBindPtr set_feature_mb = iface->classdb_get_method_bind(class_name, method_name, 2819288693);
        
        if (set_feature_mb) {
            GDExtensionVariantPtr feature_var = gdext_variant_from_int(5); // FEATURE_EMISSION = 5
            GDExtensionVariantPtr enabled_var = gdext_variant_from_bool(1);
            GDExtensionVariantPtr ret = malloc(GDEXT_VARIANT_SIZE);
            iface->variant_new_nil(ret);
            
            GDExtensionCallError error;
            memset(&error, 0, sizeof(error));
            
            GDExtensionConstVariantPtr args[2] = {feature_var, enabled_var};
            iface->object_method_bind_call(set_feature_mb, mat_obj, args, 2, ret, &error);
            
            gdext_variant_free(feature_var);
            gdext_variant_free(enabled_var);
            iface->variant_destroy(ret);
            free(ret);
            
            if (error.error != GDEXTENSION_CALL_OK) {
                fprintf(stderr, "[gdext-c] MESH WARNING: set_feature(EMISSION) failed (error=%d)\n", error.error);
            }
        }
        
        // Set emission color (warm white glow)
        iface->string_name_new_with_latin1_chars(method_name, "set_emission", 0);
        GDExtensionMethodBindPtr set_emission_mb = iface->classdb_get_method_bind(class_name, method_name, 2920490490);
        
        if (set_emission_mb) {
            // Create Color(1.0, 0.95, 0.9, 1.0) - warm white emission
            GDExtensionVariantPtr color_var = malloc(GDEXT_VARIANT_SIZE);
            
            // Initialize Color variant
            GDExtensionTypePtr type_ptr = (GDExtensionTypePtr)iface->variant_get_ptr_constructor(GDEXTENSION_VARIANT_TYPE_COLOR, 2); // Constructor with 4 floats
            if (type_ptr) {
                float color_data[4] = {1.0f, 0.95f, 0.9f, 1.0f}; // Warm white
                type_ptr(color_var, (GDExtensionConstTypePtr*)&color_data);
            } else {
                iface->variant_new_nil(color_var);
            }
            
            GDExtensionVariantPtr ret = malloc(GDEXT_VARIANT_SIZE);
            iface->variant_new_nil(ret);
            
            GDExtensionCallError error;
            memset(&error, 0, sizeof(error));
            
            GDExtensionConstVariantPtr args[1] = {color_var};
            iface->object_method_bind_call(set_emission_mb, mat_obj, args, 1, ret, &error);
            
            iface->variant_destroy(color_var);
            free(color_var);
            iface->variant_destroy(ret);
            free(ret);
            
            if (error.error != GDEXTENSION_CALL_OK) {
                fprintf(stderr, "[gdext-c] MESH WARNING: set_emission(color) failed (error=%d)\n", error.error);
            }
        }
        
        // Set emission energy (3x brightness multiplier)
        iface->string_name_new_with_latin1_chars(method_name, "set_emission_energy_multiplier", 0);
        GDExtensionMethodBindPtr set_energy_mb = iface->classdb_get_method_bind(class_name, method_name, 373806689);
        
        if (set_energy_mb) {
            GDExtensionVariantPtr energy_var = gdext_variant_from_float(3.0); // 3x glow intensity
            GDExtensionVariantPtr ret = malloc(GDEXT_VARIANT_SIZE);
            iface->variant_new_nil(ret);
            
            GDExtensionCallError error;
            memset(&error, 0, sizeof(error));
            
            GDExtensionConstVariantPtr args[1] = {energy_var};
            iface->object_method_bind_call(set_energy_mb, mat_obj, args, 1, ret, &error);
            
            gdext_variant_free(energy_var);
            iface->variant_destroy(ret);
            free(ret);
            
            if (error.error != GDEXTENSION_CALL_OK) {
                fprintf(stderr, "[gdext-c] MESH WARNING: set_emission_energy_multiplier failed (error=%d)\n", error.error);
            } else {
                fprintf(stderr, "[gdext-c] MESH INFO: Player emission glow enabled (3x energy, warm white)\n");
            }
        }
        
        GDExtensionPtrDestructor name_dtor = iface->variant_get_ptr_destructor(GDEXTENSION_VARIANT_TYPE_STRING_NAME);
        if (name_dtor) {
            name_dtor(method_name);
            name_dtor(class_name);
        }
    }

    // 3. Call surface_set_material(0, material) on the ArrayMesh
    {
        GDExtensionVariantPtr idx_var = gdext_variant_from_int(0); // surface index 0
        GDExtensionVariantPtr mat_var = gdext_variant_from_object(mat_obj);
        GDExtensionVariantPtr ret = malloc(GDEXT_VARIANT_SIZE);
        iface->variant_new_nil(ret);

        GDExtensionCallError error;
        memset(&error, 0, sizeof(error));

        GDExtensionConstVariantPtr args[2];
        args[0] = (GDExtensionConstVariantPtr)idx_var;
        args[1] = (GDExtensionConstVariantPtr)mat_var;

        iface->object_method_bind_call(g_surface_set_material_mb, mesh_object, args, 2, ret, &error);

        gdext_variant_free(idx_var);
        gdext_variant_free(mat_var);
        iface->variant_destroy(ret);
        free(ret);

        if (error.error != GDEXTENSION_CALL_OK) {
            fprintf(stderr, "[gdext-c] MESH ERROR: surface_set_material failed (error=%d, arg=%d, expected=%d)\n",
                    error.error, error.argument, error.expected);
            return -10;
        }
    }

    return 0;
}

/**
 * Create a StandardMaterial3D with a specific albedo color and apply it
 * to a surface of the given mesh.
 *
 * @param mesh_object   Raw GDExtensionObjectPtr for the ArrayMesh
 * @param surface_idx   Which surface to apply the material to
 * @param r, g, b, a    Albedo color components (0.0 - 1.0)
 * @return 0 on success, negative error code on failure
 */
int gdext_mesh_apply_albedo_material(void* mesh_object, int surface_idx, float r, float g, float b, float a) {
    if (!gdext_c_is_initialized()) return MESH_ERR_NOT_INITIALIZED;
    if (!mesh_object) return MESH_ERR_NULL_MESH;

    ensure_mesh_bindings_cached();
    if (!g_set_albedo_mb || !g_surface_set_material_mb) return MESH_ERR_NO_METHOD_BIND;

    const GDExtensionInterface* iface = gdext_c_get_interface_functions();

    // 1. Create StandardMaterial3D
    char class_sn[64] = {0};
    iface->string_name_new_with_latin1_chars(class_sn, "StandardMaterial3D", 0);
    GDExtensionObjectPtr mat_obj = iface->classdb_construct_object(class_sn);
    GDExtensionPtrDestructor sn_dtor = iface->variant_get_ptr_destructor(GDEXTENSION_VARIANT_TYPE_STRING_NAME);
    if (sn_dtor) sn_dtor(class_sn);

    if (!mat_obj) return -8;

    // 2. Call set_albedo(Color) on the material
    {
        GDExtensionVariantPtr color_var = gdext_variant_from_color(r, g, b, a);
        GDExtensionVariantPtr ret = malloc(GDEXT_VARIANT_SIZE);
        iface->variant_new_nil(ret);

        GDExtensionCallError error;
        memset(&error, 0, sizeof(error));

        GDExtensionConstVariantPtr args[1];
        args[0] = (GDExtensionConstVariantPtr)color_var;

        iface->object_method_bind_call(g_set_albedo_mb, mat_obj, args, 1, ret, &error);

        gdext_variant_free(color_var);
        iface->variant_destroy(ret);
        free(ret);

        if (error.error != GDEXTENSION_CALL_OK) {
            fprintf(stderr, "[gdext-c] MESH ERROR: set_albedo failed (error=%d)\n", error.error);
            return -11;
        }
    }

    // 3. Call surface_set_material(surface_idx, material) on the mesh
    {
        GDExtensionVariantPtr idx_var = gdext_variant_from_int(surface_idx);
        GDExtensionVariantPtr mat_var = gdext_variant_from_object(mat_obj);
        GDExtensionVariantPtr ret = malloc(GDEXT_VARIANT_SIZE);
        iface->variant_new_nil(ret);

        GDExtensionCallError error;
        memset(&error, 0, sizeof(error));

        GDExtensionConstVariantPtr args[2];
        args[0] = (GDExtensionConstVariantPtr)idx_var;
        args[1] = (GDExtensionConstVariantPtr)mat_var;

        iface->object_method_bind_call(g_surface_set_material_mb, mesh_object, args, 2, ret, &error);

        gdext_variant_free(idx_var);
        gdext_variant_free(mat_var);
        iface->variant_destroy(ret);
        free(ret);

        if (error.error != GDEXTENSION_CALL_OK) {
            fprintf(stderr, "[gdext-c] MESH ERROR: surface_set_material (albedo) failed (error=%d)\n", error.error);
            return -12;
        }
    }

    return 0;
}
