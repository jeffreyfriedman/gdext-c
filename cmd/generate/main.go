package main

import (
	"encoding/json"
	"flag"
	"fmt"
	"log"
	"os"
	"path/filepath"
	"strings"
)

// ExtensionAPI represents the root structure of extension_api.json
type ExtensionAPI struct {
	Header          Header            `json:"header"`
	Classes         []ClassDef        `json:"classes"`
	BuiltinClasses  []BuiltinClass    `json:"builtin_classes"`
	Singletons      []Singleton       `json:"singletons"`
	GlobalEnums     []GlobalEnum      `json:"global_enums"`
	UtilityFunctions []UtilityFunction `json:"utility_functions"`
}

type Header struct {
	VersionMajor int    `json:"version_major"`
	VersionMinor int    `json:"version_minor"`
	VersionPatch int    `json:"version_patch"`
	VersionFull  string `json:"version_full_name"`
}

type ClassDef struct {
	Name           string      `json:"name"`
	Inherits       string      `json:"inherits"`
	APIType        string      `json:"api_type"`
	IsInstantiable bool        `json:"is_instantiable"`
	IsRefCounted   bool        `json:"is_refcounted"`
	Methods        []MethodDef `json:"methods"`
	Properties     []Property  `json:"properties"`
	Constants      []Constant  `json:"constants"`
	Enums          []EnumDef   `json:"enums"`
}

type MethodDef struct {
	Name               string     `json:"name"`
	IsConst            bool       `json:"is_const"`
	IsVararg           bool       `json:"is_vararg"`
	IsStatic           bool       `json:"is_static"`
	IsVirtual          bool       `json:"is_virtual"`
	Hash               int64      `json:"hash"`
	HashCompatibility  []int64    `json:"hash_compatibility"`
	ReturnValue        *ReturnVal `json:"return_value"`
	Arguments          []Argument `json:"arguments"`
}

type Argument struct {
	Name         string `json:"name"`
	Type         string `json:"type"`
	Meta         string `json:"meta"`
	DefaultValue string `json:"default_value"`
}

type ReturnVal struct {
	Type string `json:"type"`
	Meta string `json:"meta"`
}

type Property struct {
	Name   string `json:"name"`
	Type   string `json:"type"`
	Getter string `json:"getter"`
	Setter string `json:"setter"`
}

type Constant struct {
	Name  string `json:"name"`
	Value int    `json:"value"`
}

type EnumDef struct {
	Name   string          `json:"name"`
	Values []EnumValueDef  `json:"values"`
}

type EnumValueDef struct {
	Name  string `json:"name"`
	Value int    `json:"value"`
}

type BuiltinClass struct {
	Name         string          `json:"name"`
	IsKeyed      bool            `json:"is_keyed"`
	Members      []BuiltinMember `json:"members"`
	Methods      []MethodDef     `json:"methods"`
	Constructors []Constructor   `json:"constructors"`
	Operators    []Operator      `json:"operators"`
}

type BuiltinMember struct {
	Name string `json:"name"`
	Type string `json:"type"`
}

type Constructor struct {
	Index     int        `json:"index"`
	Arguments []Argument `json:"arguments"`
}

type Operator struct {
	Name       string      `json:"name"`
	ReturnType string      `json:"return_type"`
	RightType  string      `json:"right_type"`
}

type Singleton struct {
	Name string `json:"name"`
	Type string `json:"type"`
}

type GlobalEnum struct {
	Name   string         `json:"name"`
	Values []EnumValueDef `json:"values"`
}

type UtilityFunction struct {
	Name string `json:"name"`
}

// Generator configuration
type GeneratorConfig struct {
	InputFile      string
	OutputDir      string
	HeaderFile     string
	ImplFile       string
	BuiltinHeader  string
	BuiltinImpl    string
	MaxClasses     int  // 0 = all classes
	GenBuiltins    bool // Generate builtin types
}

func main() {
	config := GeneratorConfig{}
	flag.StringVar(&config.InputFile, "input", "", "Path to extension_api.json")
	flag.StringVar(&config.OutputDir, "output", "generated", "Output directory")
	flag.IntVar(&config.MaxClasses, "max-classes", 0, "Maximum classes to generate (0=all)")
	flag.Parse()

	if config.InputFile == "" {
		log.Fatal("--input is required")
	}

	config.HeaderFile = filepath.Join(config.OutputDir, "gdext_c_generated.h")
	config.ImplFile = filepath.Join(config.OutputDir, "gdext_c_generated.c")

	log.Printf("📋 TDD #136: Parsing extension_api.json...")
	api, err := parseExtensionAPI(config.InputFile)
	if err != nil {
		log.Fatalf("Failed to parse API: %v", err)
	}

	log.Printf("✅ Parsed Godot %s (%d classes, %d builtin classes)",
		api.Header.VersionFull, len(api.Classes), len(api.BuiltinClasses))

	// Create output directory
	if err := os.MkdirAll(config.OutputDir, 0755); err != nil {
		log.Fatalf("Failed to create output dir: %v", err)
	}

	log.Printf("🔧 TDD #137: Generating code...")
	if err := generateCode(api, config); err != nil {
		log.Fatalf("Failed to generate code: %v", err)
	}

	log.Printf("✅ Generated:")
	log.Printf("   Header: %s", config.HeaderFile)
	log.Printf("   Impl:   %s", config.ImplFile)

	// TDD #142: Generate builtin types
	log.Printf("🔧 TDD #142: Generating builtin types...")
	if err := generateBuiltinTypes(api, &config); err != nil {
		log.Fatalf("Failed to generate builtin types: %v", err)
	}
	log.Printf("✅ Generated builtin types header")
}

func generateBuiltinTypes(api *ExtensionAPI, config *GeneratorConfig) error {
	// For now, just generate struct definitions for core types
	builtinHeader := filepath.Join(config.OutputDir, "gdext_c_builtin.h")
	f, err := os.Create(builtinHeader)
	if err != nil {
		return err
	}
	defer f.Close()

	fmt.Fprintf(f, `/**
 * @file gdext_c_builtin.h
 * @brief Godot builtin types (Vector2, Vector3, Color, etc.)
 * 
 * Generated for Godot %s - TDD #142
 */

#ifndef GDEXT_C_BUILTIN_H
#define GDEXT_C_BUILTIN_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Vector2 */
typedef struct {
    float x;
    float y;
} gdext_c_vector2_t;

/* Vector3 */
typedef struct {
    float x;
    float y;
    float z;
} gdext_c_vector3_t;

/* Vector4 */
typedef struct {
    float x, y, z, w;
} gdext_c_vector4_t;

/* Color */
typedef struct {
    float r, g, b, a;
} gdext_c_color_t;

/* Rect2 */
typedef struct {
    gdext_c_vector2_t position;
    gdext_c_vector2_t size;
} gdext_c_rect2_t;

/* PackedByteArray - TDD #152 for SVO renderer buffer_update */
/* This is an opaque 16-byte builtin type (on 64-bit systems) */
typedef struct {
    uint8_t opaque[16];
} gdext_c_packed_byte_array_t;

/* TODO: More builtin types (Transform2D, Transform3D, Quaternion, etc.) */

#ifdef __cplusplus
}
#endif

#endif /* GDEXT_C_BUILTIN_H */
`, api.Header.VersionFull)

	return nil
}

func parseExtensionAPI(filepath string) (*ExtensionAPI, error) {
	data, err := os.ReadFile(filepath)
	if err != nil {
		return nil, err
	}

	var api ExtensionAPI
	if err := json.Unmarshal(data, &api); err != nil {
		return nil, err
	}

	return &api, nil
}

func generateCode(api *ExtensionAPI, config GeneratorConfig) error {
	// Filter classes if needed
	classes := api.Classes
	if config.MaxClasses > 0 && len(classes) > config.MaxClasses {
		classes = classes[:config.MaxClasses]
	}

	// Generate header
	if err := generateHeader(api, classes, config.HeaderFile); err != nil {
		return fmt.Errorf("generate header: %w", err)
	}

	// Generate implementation
	if err := generateImpl(api, classes, config.ImplFile); err != nil {
		return fmt.Errorf("generate impl: %w", err)
	}

	return nil
}

func generateHeader(api *ExtensionAPI, classes []ClassDef, outputPath string) error {
	f, err := os.Create(outputPath)
	if err != nil {
		return err
	}
	defer f.Close()

	// Write header guard and includes
	fmt.Fprintf(f, `/**
 * @file gdext_c_generated.h
 * @brief Auto-generated Godot GDExtension C bindings
 * 
 * Generated from extension_api.json for Godot %s
 * 
 * DO NOT EDIT MANUALLY - regenerate with: make generate
 */

#ifndef GDEXT_C_GENERATED_H
#define GDEXT_C_GENERATED_H

#include "gdext_c.h"
#include "gdextension_interface.h"
#include "gdext_c_builtin.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Version Information (TDD #143)
 * ============================================================================ */

#define GDEXT_C_GODOT_VERSION_MAJOR %d
#define GDEXT_C_GODOT_VERSION_MINOR %d
#define GDEXT_C_GODOT_VERSION_PATCH %d
#define GDEXT_C_GODOT_VERSION_STRING "%s"

/**
 * @brief Get the Godot version this library was generated for
 * @return Version string (e.g., "Godot Engine v4.5.stable.official")
 */
const char* gdext_c_get_godot_version(void);

/**
 * @brief Check if runtime Godot version is compatible
 * @param major Major version (e.g., 4)
 * @param minor Minor version (e.g., 5)
 * @return true if compatible, false otherwise
 */
bool gdext_c_check_version_compatible(int major, int minor);

`, api.Header.VersionFull, 
    api.Header.VersionMajor, 
    api.Header.VersionMinor, 
    api.Header.VersionPatch,
    api.Header.VersionFull)

	// Generate function declarations for each class
	for _, class := range classes {
		// Skip virtual-only classes
		if hasOnlyVirtualMethods(class) {
			continue
		}

		fmt.Fprintf(f, "/* ============================================================================\n")
		fmt.Fprintf(f, " * Class: %s\n", class.Name)
		if class.Inherits != "" {
			fmt.Fprintf(f, " * Inherits: %s\n", class.Inherits)
		}
		fmt.Fprintf(f, " * ============================================================================ */\n\n")

		for _, method := range class.Methods {
			// Skip virtual methods (must be overridden)
			if method.IsVirtual {
				continue
			}

			// Generate function declaration
			funcName := generateFunctionName(class.Name, method.Name)
			fmt.Fprintf(f, "/**\n")
			fmt.Fprintf(f, " * @brief %s.%s\n", class.Name, method.Name)
			if method.IsStatic {
				fmt.Fprintf(f, " * @note Static method\n")
			}
			fmt.Fprintf(f, " */\n")

			// Return type
			retType := "void"
			if method.ReturnValue != nil {
				retType = mapGodotTypeToCType(method.ReturnValue.Type)
			}

			fmt.Fprintf(f, "%s %s(", retType, funcName)

			// Instance argument (if not static)
			if !method.IsStatic {
				fmt.Fprintf(f, "gdext_c_object_t instance")
				if len(method.Arguments) > 0 {
					fmt.Fprintf(f, ", ")
				}
			}

			// Method arguments
			for i, arg := range method.Arguments {
				cType := mapGodotTypeToCType(arg.Type)
				fmt.Fprintf(f, "%s %s", cType, sanitizeName(arg.Name))
				if i < len(method.Arguments)-1 {
					fmt.Fprintf(f, ", ")
				}
			}

			fmt.Fprintf(f, ");\n\n")
		}
	}

	fmt.Fprintf(f, `
#ifdef __cplusplus
}
#endif

#endif /* GDEXT_C_GENERATED_H */
`)

	return nil
}

func generateImpl(api *ExtensionAPI, classes []ClassDef, outputPath string) error {
	f, err := os.Create(outputPath)
	if err != nil {
		return err
	}
	defer f.Close()

	fmt.Fprintf(f, `/**
 * @file gdext_c_generated.c
 * @brief Auto-generated Godot GDExtension C bindings implementation
 * 
 * Generated from extension_api.json for Godot %s
 */

#include "gdext_c_generated.h"
#include "core/gdext_c_core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* Global GDExtension interface (defined in gdext_c_core.c) */
extern GDExtensionInterface iface_impl;
#define iface (&iface_impl)

/* ============================================================================
 * Version Information (TDD #143)
 * ============================================================================ */

const char* gdext_c_get_godot_version(void) {
    return GDEXT_C_GODOT_VERSION_STRING;
}

bool gdext_c_check_version_compatible(int major, int minor) {
    // Compatible if same major version and equal or newer minor version
    if (major != GDEXT_C_GODOT_VERSION_MAJOR) {
        fprintf(stderr, "[gdext-c] ⚠️  TDD #143: Version mismatch! Generated for %%d.%%d, runtime is %%d.%%d\\n",
                GDEXT_C_GODOT_VERSION_MAJOR, GDEXT_C_GODOT_VERSION_MINOR, major, minor);
        return false;
    }
    if (minor < GDEXT_C_GODOT_VERSION_MINOR) {
        fprintf(stderr, "[gdext-c] ⚠️  TDD #143: Runtime Godot %%d.%%d is older than generated %%d.%%d\\n",
                major, minor, GDEXT_C_GODOT_VERSION_MAJOR, GDEXT_C_GODOT_VERSION_MINOR);
        return false;
    }
    return true;
}

`, api.Header.VersionFull)

	// Generate implementations
	for _, class := range classes {
		if hasOnlyVirtualMethods(class) {
			continue
		}

		fmt.Fprintf(f, "/* ============================================================================\n")
		fmt.Fprintf(f, " * Class: %s\n", class.Name)
		fmt.Fprintf(f, " * ============================================================================ */\n\n")

		for _, method := range class.Methods {
			if method.IsVirtual {
				continue
			}

			if err := generateMethodImpl(f, class, method); err != nil {
				return err
			}
		}
	}

	return nil
}

func generateMethodImpl(f *os.File, class ClassDef, method MethodDef) error {
	funcName := generateFunctionName(class.Name, method.Name)
	
	// Return type
	retType := "void"
	hasReturn := false
	if method.ReturnValue != nil {
		retType = mapGodotTypeToCType(method.ReturnValue.Type)
		hasReturn = true
	}

	// Function signature
	fmt.Fprintf(f, "%s %s(", retType, funcName)
	if !method.IsStatic {
		fmt.Fprintf(f, "gdext_c_object_t instance")
		if len(method.Arguments) > 0 {
			fmt.Fprintf(f, ", ")
		}
	}
	for i, arg := range method.Arguments {
		cType := mapGodotTypeToCType(arg.Type)
		fmt.Fprintf(f, "%s %s", cType, sanitizeName(arg.Name))
		if i < len(method.Arguments)-1 {
			fmt.Fprintf(f, ", ")
		}
	}
	fmt.Fprintf(f, ") {\n")

	// Get StringName for class and method
	fmt.Fprintf(f, "    // Get method bind\n")
	fmt.Fprintf(f, "    static GDExtensionMethodBindPtr method_bind = NULL;\n")
	fmt.Fprintf(f, "    if (method_bind == NULL) {\n")
	fmt.Fprintf(f, "        char class_sn[64];\n")
	fmt.Fprintf(f, "        char method_sn[64];\n")
	fmt.Fprintf(f, "        iface->string_name_new_with_latin1_chars(class_sn, \"%s\", 0);\n", class.Name)
	fmt.Fprintf(f, "        iface->string_name_new_with_latin1_chars(method_sn, \"%s\", 0);\n", method.Name)
	fmt.Fprintf(f, "        method_bind = iface->classdb_get_method_bind(class_sn, method_sn, %d);\n", method.Hash)
	fmt.Fprintf(f, "        if (method_bind == NULL) {\n")
	fmt.Fprintf(f, "            fprintf(stderr, \"[gdext-c] ERROR: Failed to get method bind for %s.%s\\n\");\n", class.Name, method.Name)
	if hasReturn {
		fmt.Fprintf(f, "            return (%s){0};\n", retType)
	} else {
		fmt.Fprintf(f, "            return;\n")
	}
	fmt.Fprintf(f, "        }\n")
	fmt.Fprintf(f, "    }\n\n")

	// Prepare arguments array
	if len(method.Arguments) > 0 {
		fmt.Fprintf(f, "    // Prepare arguments\n")
		fmt.Fprintf(f, "    GDExtensionConstTypePtr args[%d];\n", len(method.Arguments))
		for i, arg := range method.Arguments {
			argName := sanitizeName(arg.Name)
			if isPointerType(arg.Type) {
				fmt.Fprintf(f, "    args[%d] = (GDExtensionConstTypePtr)&%s;\n", i, argName)
			} else {
				fmt.Fprintf(f, "    args[%d] = (GDExtensionConstTypePtr)&%s;\n", i, argName)
			}
		}
		fmt.Fprintf(f, "\n")
	}

	// Prepare return value
	if hasReturn {
		fmt.Fprintf(f, "    // Prepare return value\n")
		fmt.Fprintf(f, "    %s ret;\n", retType)
		fmt.Fprintf(f, "    GDExtensionTypePtr ret_ptr = (GDExtensionTypePtr)&ret;\n\n")
	}

	// Call method
	fmt.Fprintf(f, "    // Call method\n")
	// Static methods pass NULL as instance
	if method.IsStatic {
		fmt.Fprintf(f, "    iface->object_method_bind_ptrcall(method_bind, NULL, ")
	} else {
		fmt.Fprintf(f, "    iface->object_method_bind_ptrcall(method_bind, instance, ")
	}
	if len(method.Arguments) > 0 {
		fmt.Fprintf(f, "args, ")
	} else {
		fmt.Fprintf(f, "NULL, ")
	}
	if hasReturn {
		fmt.Fprintf(f, "ret_ptr);\n\n")
		fmt.Fprintf(f, "    return ret;\n")
	} else {
		fmt.Fprintf(f, "NULL);\n")
	}

	fmt.Fprintf(f, "}\n\n")
	return nil
}

// Helper functions

func generateFunctionName(className, methodName string) string {
	// Convert CamelCase to snake_case and add prefix
	className = camelToSnake(className)
	methodName = camelToSnake(methodName)
	return fmt.Sprintf("gdext_%s_%s", className, methodName)
}

func camelToSnake(s string) string {
	var result strings.Builder
	for i, r := range s {
		if i > 0 && r >= 'A' && r <= 'Z' {
			result.WriteRune('_')
		}
		result.WriteRune(r)
	}
	return strings.ToLower(result.String())
}

func mapGodotTypeToCType(godotType string) string {
	// Handle pointers
	if strings.HasPrefix(godotType, "enum::") {
		return "int32_t"
	}

	switch godotType {
	case "int", "Int":
		return "int64_t"
	case "float", "Float":
		return "double"
	case "bool", "Bool":
		return "GDExtensionBool"
	case "String":
		return "gdext_c_object_t" // String is an object
	case "Vector2":
		return "gdext_c_vector2_t" // Match typedef in gdext_c_builtin.h
	case "Vector3":
		return "gdext_c_vector3_t" // Match typedef in gdext_c_builtin.h
	case "Color":
		return "gdext_c_color_t" // Match typedef in gdext_c_builtin.h
	case "PackedByteArray": // TDD #152 for SVO renderer
		return "gdext_c_packed_byte_array_t"
	case "void":
		return "void"
	default:
		// All class types are objects
		return "gdext_c_object_t"
	}
}

func sanitizeName(name string) string {
	// Replace reserved C keywords AND conflicting parameter names
	reserved := map[string]string{
		"default":  "default_value",
		"class":    "class_name",
		"new":      "new_value",
		"char":     "char_value",
		"enum":     "enum_value",
		"int":      "int_value",
		"float":    "float_value",
		"bool":     "bool_value",
		"void":     "void_value",
		"const":    "const_value",
		"static":   "static_value",
		"struct":   "struct_value",
		"union":    "union_value",
		"typedef":  "typedef_value",
		"sizeof":   "sizeof_value",
		"volatile": "volatile_value",
		"register": "register_value",
		"extern":   "extern_value",
		"auto":     "auto_value",
		"instance": "instance_arg", // Conflicts with our instance parameter!
		"args":     "args_value",   // Conflicts with our args array!
		"ret":      "ret_value",    // Conflicts with our ret variable!
	}
	if replacement, ok := reserved[name]; ok {
		return replacement
	}
	return name
}

func isPointerType(godotType string) bool {
	// Objects are already pointers
	switch godotType {
	case "int", "float", "bool", "enum":
		return false
	default:
		return strings.HasPrefix(godotType, "enum::") == false
	}
}

func hasOnlyVirtualMethods(class ClassDef) bool {
	if len(class.Methods) == 0 {
		return true
	}
	for _, method := range class.Methods {
		if !method.IsVirtual {
			return false
		}
	}
	return true
}

