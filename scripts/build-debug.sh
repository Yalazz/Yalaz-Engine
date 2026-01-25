#!/bin/bash
# =============================================================
# Yalaz Engine - Debug Build Script
# =============================================================
# Usage: ./scripts/build-debug.sh

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

cd "$PROJECT_DIR"

# Detect platform
detect_platform() {
    case "$(uname -s)" in
        Linux*)     echo "linux";;
        Darwin*)    echo "macos";;
        MINGW*|MSYS*|CYGWIN*)  echo "windows";;
        *)          echo "unknown";;
    esac
}

PLATFORM=$(detect_platform)

echo "========================================"
echo "Yalaz Engine - Debug Build"
echo "Platform: $PLATFORM"
echo "========================================"

case "$PLATFORM" in
    windows)
        PRESET="debug"
        BUILD_PRESET="debug-build"
        ;;
    linux)
        PRESET="linux-debug"
        BUILD_PRESET="linux-debug-build"
        ;;
    macos)
        PRESET="macos-debug"
        BUILD_PRESET="macos-debug-build"
        ;;
    *)
        echo "Error: Unknown platform"
        exit 1
        ;;
esac

echo ""
echo "Configuring..."
cmake --preset "$PRESET"

echo ""
echo "Building..."
cmake --build --preset "$BUILD_PRESET" --parallel

echo ""
echo "========================================"
echo "Debug build complete!"
echo "Executable: bin/Debug/Yalaz_Engine"
echo "========================================"
