# gdext-c: Universal C Library for Godot GDExtension
# Simple Makefile for quick builds

CC := cc
CFLAGS := -fPIC -std=c11 -O2 -Wall -Wextra -Iinclude -Isrc -Isrc/core -Igenerated
LDFLAGS := -dynamiclib
# TDD #133: Set install_name to @rpath so the library can be found at runtime
ifeq ($(shell uname),Darwin)
    LDFLAGS += -install_name @rpath/libgdext_c.dylib
endif

# Source files
SRC_CORE := $(wildcard src/core/*.c)
SRC_SCENE := $(wildcard src/scene/*.c)
SRC_API := $(wildcard src/api/*.c)
SRC_MATH := $(wildcard src/math/*.c)
SRC_GDEXTENSION := $(wildcard src/gdextension/*.c)
SRC_LIFECYCLE := $(wildcard src/lifecycle/*.c)
SRC_THREADING := $(wildcard src/threading/*.c)
SRC_GENERATED := generated/gdext_c_generated.c
SRCS := $(SRC_CORE) $(SRC_SCENE) $(SRC_API) $(SRC_MATH) $(SRC_GDEXTENSION) $(SRC_LIFECYCLE) $(SRC_THREADING) $(SRC_GENERATED)

# Note: SRC_SCENE now includes gdext_c_scene_tree.c (universal infrastructure!)

# Object files
OBJS := $(SRCS:.c=.o)

# Output library
ifeq ($(OS),Windows_NT)
    LIB := libgdext_c.dll
    STATIC_LIB := libgdext_c.a
else ifeq ($(shell uname),Darwin)
    LIB := libgdext_c.dylib
    STATIC_LIB := libgdext_c.a
else
    LIB := libgdext_c.so
    STATIC_LIB := libgdext_c.a
endif

# TDD #137: Code generation
API_JSON ?= /Users/jeffreyfriedman/src/gamedev/gdext-go/extension_api.json

.PHONY: generate
generate:
	@echo "🔧 TDD #137: Generating C bindings from extension_api.json..."
	@cd cmd/generate && go run main.go \
		--input $(API_JSON) \
		--output ../../generated \
		--max-classes 0
	@echo "✅ Generated bindings in generated/"

# Default target
all: $(LIB) $(STATIC_LIB)

# Build shared library
$(LIB): $(OBJS)
	@echo "🔗 Linking shared library $@..."
	@$(CC) $(LDFLAGS) -o $@ $^
	@echo "✅ Built gdext-c shared library: $@"

# Build static library (for standalone linking)
$(STATIC_LIB): $(OBJS)
	@echo "📚 Creating static library $@..."
	@ar rcs $@ $^
	@echo "✅ Built gdext-c static library: $@"

# Ensure generated code exists before compiling
generated/gdext_c_generated.o: generated/gdext_c_generated.c

# Generate code if source doesn't exist
generated/gdext_c_generated.c:
	@if [ ! -f $@ ]; then $(MAKE) generate; fi

# Compile source files
%.o: %.c
	@echo "🔨 Compiling $<..."
	@$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	@echo "🧹 Cleaning..."
	@rm -f $(OBJS) $(LIB) $(STATIC_LIB)
	@echo "✅ Clean complete"

# Install (copy to standard location)
install: $(LIB)
	@echo "📦 Installing gdext-c..."
	@mkdir -p /usr/local/lib
	@cp $(LIB) /usr/local/lib/
	@mkdir -p /usr/local/include/gdext_c
	@cp include/*.h /usr/local/include/gdext_c/
	@echo "✅ Installed to /usr/local"

# Install to game project (for development)
# Detects action-adventure-framework automatically
install-game: $(LIB)
	@echo "📦 Installing gdext-c to game project..."
	@if [ -d "../action-adventure-framework/bin/macos" ]; then \
		cp $(LIB) ../action-adventure-framework/bin/macos/$(LIB); \
		echo "✅ Installed to ../action-adventure-framework/bin/macos/"; \
	elif [ -d "$(GAME_PROJECT)/bin/macos" ]; then \
		cp $(LIB) $(GAME_PROJECT)/bin/macos/$(LIB); \
		echo "✅ Installed to $(GAME_PROJECT)/bin/macos/"; \
	else \
		echo "❌ Game project not found. Set GAME_PROJECT=/path/to/project"; \
		exit 1; \
	fi

# Convenience: build and install to game in one step
game: all install-game

# Uninstall
uninstall:
	@echo "🗑️  Uninstalling gdext-c..."
	@rm -f /usr/local/lib/$(LIB)
	@rm -rf /usr/local/include/gdext_c
	@echo "✅ Uninstalled"

# Show help
help:
	@echo "gdext-c Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all        - Build the library (default)"
	@echo "  clean      - Remove build artifacts"
	@echo "  install    - Install to /usr/local"
	@echo "  uninstall  - Remove from /usr/local"
	@echo "  help       - Show this help"

.PHONY: clean install uninstall help generate

