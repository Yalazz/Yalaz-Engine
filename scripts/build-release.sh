#!/bin/bash
# =============================================================
# Yalaz Engine - Release Build Script
# =============================================================
# Usage: ./scripts/build-release.sh [platform]
# Platforms: windows, linux, macos, auto (default)

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

PLATFORM="${1:-auto}"
if [ "$PLATFORM" = "auto" ]; then
    PLATFORM=$(detect_platform)
fi

echo "========================================"
echo "Yalaz Engine - Release Build"
echo "Platform: $PLATFORM"
echo "========================================"

case "$PLATFORM" in
    windows)
        PRESET="package"
        BUILD_PRESET="package-build"
        PACKAGE_PRESET="windows-zip-only"
        ;;
    linux)
        PRESET="linux-package"
        BUILD_PRESET="linux-package-build"
        PACKAGE_PRESET="linux-tgz-only"
        ;;
    macos)
        PRESET="macos-package"
        BUILD_PRESET="macos-package-build"
        PACKAGE_PRESET="macos-dmg-only"
        ;;
    *)
        echo "Error: Unknown platform '$PLATFORM'"
        echo "Supported platforms: windows, linux, macos"
        exit 1
        ;;
esac

echo ""
echo "Step 1: Configure..."
cmake --preset "$PRESET"

echo ""
echo "Step 2: Build..."
cmake --build --preset "$BUILD_PRESET" --parallel

echo ""
echo "Step 3: Package..."
cd "build/package"
cpack -G "${PACKAGE_PRESET##*-}"

echo ""
echo "========================================"
echo "Build complete!"
echo "Package location: build/package/"
echo "========================================"

# List generated packages
ls -la *.zip *.dmg *.tar.gz *.deb 2>/dev/null || true
