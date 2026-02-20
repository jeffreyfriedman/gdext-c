#!/bin/bash
# TDD #206: Regression test to ensure entry point is exported
# This test verifies the fix for missing GDE_EXPORT macro

set -e

echo "🔍 Testing GDExtension entry point export..."

# Find the library
if [ -f "../action-adventure-framework/bin/macos/libgdext_c.dylib" ]; then
    LIB="../action-adventure-framework/bin/macos/libgdext_c.dylib"
elif [ -f "bin/macos/libgdext_c.dylib" ]; then
    LIB="bin/macos/libgdext_c.dylib"
else
    echo "❌ Library not found!"
    echo "   Expected: ../action-adventure-framework/bin/macos/libgdext_c.dylib"
    echo "   Or: bin/macos/libgdext_c.dylib"
    exit 1
fi

echo "📦 Testing library: $LIB"

# Check if entry point is exported
SYMBOL=$(nm -g "$LIB" | grep gdext_c_library_init || true)

if [ -z "$SYMBOL" ]; then
    echo "❌ FAIL: gdext_c_library_init symbol NOT found!"
    echo "   This means GDE_EXPORT is missing or not working."
    echo "   The extension will NOT load without this!"
    exit 1
fi

# Check if it's a TEXT symbol (code), not UNDEFINED
if echo "$SYMBOL" | grep -q " T "; then
    echo "✅ PASS: gdext_c_library_init is exported correctly"
    echo "   Symbol: $SYMBOL"
    exit 0
elif echo "$SYMBOL" | grep -q " U "; then
    echo "❌ FAIL: gdext_c_library_init is UNDEFINED"
    echo "   This means the function is declared but not implemented."
    echo "   Symbol: $SYMBOL"
    exit 1
else
    echo "⚠️  WARN: gdext_c_library_init has unexpected type"
    echo "   Symbol: $SYMBOL"
    echo "   Expected: T (text/code symbol)"
    exit 1
fi
