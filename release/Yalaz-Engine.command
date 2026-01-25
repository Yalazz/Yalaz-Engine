#!/bin/bash
# =============================================================
# Yalaz Engine Launcher (Unix/macOS)
# =============================================================
# Double-click this file or run from terminal to start the engine

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Set library path for bundled libraries
if [[ "$OSTYPE" == "darwin"* ]]; then
    # macOS - set up MoltenVK environment
    export DYLD_LIBRARY_PATH="$SCRIPT_DIR/bin:$DYLD_LIBRARY_PATH"

    # Use bundled MoltenVK if available
    if [ -f "$SCRIPT_DIR/bin/libMoltenVK.dylib" ]; then
        export VK_ICD_FILENAMES=""
        export VK_DRIVER_FILES=""
    fi
else
    # Linux
    export LD_LIBRARY_PATH="$SCRIPT_DIR/bin:$LD_LIBRARY_PATH"
fi

# Change to release directory so relative paths work
cd "$SCRIPT_DIR"

# Find the executable
if [ -f "$SCRIPT_DIR/bin/Yalaz_Engine" ]; then
    EXECUTABLE="$SCRIPT_DIR/bin/Yalaz_Engine"
elif [ -f "$SCRIPT_DIR/bin/Yalaz_Engine-"* ]; then
    EXECUTABLE=$(ls "$SCRIPT_DIR/bin/Yalaz_Engine-"* 2>/dev/null | head -1)
else
    echo "Error: Could not find Yalaz_Engine executable"
    exit 1
fi

# Launch the engine
exec "$EXECUTABLE" "$@"
