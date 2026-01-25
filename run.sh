#!/bin/bash
# =============================================================
# YALAZ ENGINE - Quick Run Script
# =============================================================
# Usage: ./run.sh [debug|release]
# Default: debug

set -e
cd "$(dirname "$0")"

CONFIG="${1:-debug}"
CONFIG_LOWER=$(echo "$CONFIG" | tr '[:upper:]' '[:lower:]')

case "$CONFIG_LOWER" in
    debug)
        EXE_DIR="bin/Debug"
        ;;
    release)
        EXE_DIR="bin/Release"
        ;;
    *)
        echo "Usage: $0 [debug|release]"
        exit 1
        ;;
esac

if [ ! -f "$EXE_DIR/Yalaz_Engine" ]; then
    echo "Error: Executable not found at $EXE_DIR/Yalaz_Engine"
    echo ""
    echo "Build first with:"
    echo "  cmake --preset macos-$CONFIG_LOWER && cmake --build build/$CONFIG_LOWER -j"
    exit 1
fi

echo "Starting Yalaz Engine ($CONFIG_LOWER)..."
cd "$EXE_DIR"
exec ./Yalaz_Engine
