# =============================================================
# YALAZ ENGINE - CROSS-PLATFORM TOOLCHAIN
# =============================================================
# This toolchain file auto-detects the host platform and configures
# the appropriate compiler settings. Used for custom/cross-compilation.
#
# Usage: cmake -DCMAKE_TOOLCHAIN_FILE=toolchain.cmake ..
#
# NOTE: For most builds, use CMakePresets.json instead:
#   cmake --preset macos-debug      # macOS
#   cmake --preset linux-debug      # Linux
#   cmake --preset debug            # Windows (Ninja+MinGW)
#   cmake --preset vs2022           # Windows (Visual Studio)
# =============================================================

# Detect host platform using CMake's native detection
# CMAKE_HOST_SYSTEM_NAME is available in toolchain files
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
    # macOS / Apple Silicon / Intel Mac
    set(CMAKE_SYSTEM_NAME Darwin)
    set(CMAKE_C_COMPILER "/usr/bin/clang")
    set(CMAKE_CXX_COMPILER "/usr/bin/clang++")

    # Let CMake detect the architecture (arm64 or x86_64)
    # Don't hardcode - supports both Apple Silicon and Intel

elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    # Linux
    set(CMAKE_SYSTEM_NAME Linux)

    # Prefer clang if available, fallback to gcc
    find_program(CLANG_FOUND clang)
    if(CLANG_FOUND)
        set(CMAKE_C_COMPILER "clang")
        set(CMAKE_CXX_COMPILER "clang++")
    else()
        set(CMAKE_C_COMPILER "gcc")
        set(CMAKE_CXX_COMPILER "g++")
    endif()

elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    # Windows
    set(CMAKE_SYSTEM_NAME Windows)
    set(CMAKE_SYSTEM_PROCESSOR x86_64)

    # Try to find MSVC first, then MinGW
    if(EXISTS "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC")
        # Use Visual Studio - let CMake auto-detect the compiler
        # Don't set compilers explicitly for VS
    elseif(EXISTS "C:/msys64/mingw64/bin/g++.exe")
        # Use MinGW-w64
        set(CMAKE_C_COMPILER "C:/msys64/mingw64/bin/gcc.exe")
        set(CMAKE_CXX_COMPILER "C:/msys64/mingw64/bin/g++.exe")
    endif()

else()
    message(FATAL_ERROR "Unsupported host platform: ${CMAKE_HOST_SYSTEM_NAME}")
endif()

# =============================================================
# COMMON SETTINGS
# =============================================================
# Export compile commands for IDE integration (clangd, etc.)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "" FORCE)
