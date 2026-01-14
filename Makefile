# gdext-c: Universal C Library for Godot GDExtension
# Simple Makefile for quick builds

CC := cc
CFLAGS := -fPIC -std=c11 -O2 -Wall -Wextra -Iinclude
LDFLAGS := -dynamiclib

# Source files
SRC_CORE := $(wildcard src/core/*.c)
SRC_SCENE := $(wildcard src/scene/*.c)
SRC_API := $(wildcard src/api/*.c)
SRC_MATH := $(wildcard src/math/*.c)
SRCS := $(SRC_CORE) $(SRC_SCENE) $(SRC_API) $(SRC_MATH)

# Object files
OBJS := $(SRCS:.c=.o)

# Output library
ifeq ($(OS),Windows_NT)
    LIB := libgdext_c.dll
else ifeq ($(shell uname),Darwin)
    LIB := libgdext_c.dylib
else
    LIB := libgdext_c.so
endif

# Default target
all: $(LIB)

# Build library
$(LIB): $(OBJS)
	@echo "🔗 Linking $@..."
	@$(CC) $(LDFLAGS) -o $@ $^
	@echo "✅ Built gdext-c: $@"

# Compile source files
%.o: %.c
	@echo "🔨 Compiling $<..."
	@$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	@echo "🧹 Cleaning..."
	@rm -f $(OBJS) $(LIB)
	@echo "✅ Clean complete"

# Install (copy to standard location)
install: $(LIB)
	@echo "📦 Installing gdext-c..."
	@mkdir -p /usr/local/lib
	@cp $(LIB) /usr/local/lib/
	@mkdir -p /usr/local/include/gdext_c
	@cp include/*.h /usr/local/include/gdext_c/
	@echo "✅ Installed to /usr/local"

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

.PHONY: all clean install uninstall help

