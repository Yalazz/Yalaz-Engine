cmake_minimum_required(VERSION 3.15)

# =============================================================
# PROJECT CONFIGURATION
# =============================================================
# Include version before project() to set version
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
include(cmake/Version.cmake)

project(Yalaz_Engine
    VERSION ${YALAZ_VERSION}
    LANGUAGES CXX
    DESCRIPTION "Professional Vulkan Rendering Engine"
)

cmake_policy(SET CMP0079 NEW)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# =============================================================
# BUILD OPTIONS
# =============================================================
option(YALAZ_BUILD_STANDALONE "Build standalone release (bundle all dependencies)" OFF)
option(YALAZ_INCLUDE_ASSETS "Include sample assets in release package" ON)
option(YALAZ_STATIC_RUNTIME "Link against static runtime (MSVC only)" OFF)

# =============================================================
# BUILD TYPE CONFIGURATION (Debug / Release / RelWithDebInfo / MinSizeRel)
# =============================================================
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to 'Release' as none was specified.")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

message(STATUS "========================================")
message(STATUS "Yalaz Engine v${YALAZ_VERSION}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Platform: ${CMAKE_SYSTEM_NAME} (${CMAKE_SYSTEM_PROCESSOR})")
message(STATUS "========================================")

# =============================================================
# COMPILER FLAGS FOR DEBUG / RELEASE
# =============================================================
if(MSVC)
    # MSVC Compiler (Visual Studio 2017+)
    add_compile_options(/MP /Zc:preprocessor /utf-8)

    # Static runtime linking for standalone builds
    if(YALAZ_STATIC_RUNTIME)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()

    # Debug: Full debug info, no optimization, validation enabled
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od /Zi /RTC1")

    # Release: Full optimization, NDEBUG defined (disables validation)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /Ob2 /DNDEBUG /GL")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG")

    # RelWithDebInfo: Optimization with debug symbols
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /O2 /Zi /DNDEBUG")

    # MinSizeRel: Optimize for size
    set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} /O1 /DNDEBUG")

else()
    # GCC / MinGW / Clang
    add_compile_options(-Wall -Wextra -Wno-unknown-pragmas)

    # Debug: No optimization, debug symbols, validation enabled
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g3 -ggdb")

    # Release: Full optimization, NDEBUG defined (disables validation)
    if(APPLE)
        # macOS: Let compiler choose optimal settings for both Intel and ARM
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG -flto")
    else()
        # Linux/MinGW: Use portable x86-64 baseline with LTO
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG -flto")
    endif()
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -flto")

    # RelWithDebInfo: Optimization with debug symbols
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -O2 -g -DNDEBUG")

    # MinSizeRel: Optimize for size
    set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -Os -DNDEBUG -flto")
endif()

# Print compiler flags for current build type
message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Validation Layers: ENABLED")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    message(STATUS "Validation Layers: DISABLED (NDEBUG defined)")
    message(STATUS "Link Time Optimization: ENABLED")
endif()

# =============================================================
# Vulkan SDK & glslangValidator - Cross-Platform Configuration
# =============================================================

# Set VULKAN_SDK_PATH based on platform for finding tools
if(WIN32)
    set(VULKAN_SDK_PATH $ENV{VULKAN_SDK})
elseif(APPLE)
    if(DEFINED ENV{VULKAN_SDK})
        set(VULKAN_SDK_PATH $ENV{VULKAN_SDK})
    elseif(EXISTS "$ENV{HOME}/VulkanSDK")
        file(GLOB VULKAN_SDK_VERSIONS "$ENV{HOME}/VulkanSDK/*")
        list(LENGTH VULKAN_SDK_VERSIONS VULKAN_SDK_COUNT)
        if(VULKAN_SDK_COUNT GREATER 0)
            list(GET VULKAN_SDK_VERSIONS -1 VULKAN_SDK_PATH)
        endif()
    elseif(EXISTS "/usr/local/share/vulkan")
        set(VULKAN_SDK_PATH "/usr/local")
    else()
        set(VULKAN_SDK_PATH "/usr/local")
    endif()

    # macOS SDK has libs under macOS/ subdirectory
    if(EXISTS "${VULKAN_SDK_PATH}/macOS/lib/libvulkan.dylib")
        set(VULKAN_SDK_PATH "${VULKAN_SDK_PATH}/macOS")
    endif()

    # Override Vulkan paths to use the SDK
    if(EXISTS "${VULKAN_SDK_PATH}/include/vulkan" AND EXISTS "${VULKAN_SDK_PATH}/lib/libvulkan.dylib")
        set(Vulkan_INCLUDE_DIR "${VULKAN_SDK_PATH}/include")
        set(Vulkan_LIBRARY "${VULKAN_SDK_PATH}/lib/libvulkan.dylib")
        message(STATUS "Using Vulkan SDK from: ${VULKAN_SDK_PATH}")
    endif()

    add_compile_definitions(VK_USE_PLATFORM_MACOS_MVK)

elseif(UNIX)
    if(DEFINED ENV{VULKAN_SDK})
        set(VULKAN_SDK_PATH $ENV{VULKAN_SDK})
    elseif(EXISTS "/usr/share/vulkan")
        set(VULKAN_SDK_PATH "/usr")
    elseif(EXISTS "/usr/local/share/vulkan")
        set(VULKAN_SDK_PATH "/usr/local")
    else()
        set(VULKAN_SDK_PATH "/usr")
    endif()

    add_compile_definitions(VK_USE_PLATFORM_XLIB_KHR)
endif()

find_package(Vulkan REQUIRED)

message(STATUS "Vulkan SDK Path: ${VULKAN_SDK_PATH}")
message(STATUS "Vulkan Include: ${Vulkan_INCLUDE_DIRS}")
message(STATUS "Vulkan Library: ${Vulkan_LIBRARIES}")

# Find glslangValidator (shader compiler)
find_program(GLSL_VALIDATOR glslangValidator
    HINTS
        ${VULKAN_SDK_PATH}/Bin/
        ${VULKAN_SDK_PATH}/Bin32/
        ${VULKAN_SDK_PATH}/bin/
        ${Vulkan_INCLUDE_DIRS}/../Bin/
        /usr/bin
        /usr/local/bin
        /opt/vulkan/bin
        $ENV{HOME}/VulkanSDK/*/macOS/bin/
        $ENV{HOME}/VulkanSDK/*/x86_64/bin/
)
if(NOT GLSL_VALIDATOR)
    message(FATAL_ERROR "glslangValidator not found! Please install Vulkan SDK.")
endif()
message(STATUS "glslangValidator: ${GLSL_VALIDATOR}")

# =============================================================
# OUTPUT DIRECTORIES
# =============================================================
# Organize outputs by build type
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/lib/${CMAKE_BUILD_TYPE}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/lib/${CMAKE_BUILD_TYPE}")

# For multi-config generators (Visual Studio, Xcode)
foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} "${PROJECT_SOURCE_DIR}/bin/${OUTPUTCONFIG}")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} "${PROJECT_SOURCE_DIR}/lib/${OUTPUTCONFIG}")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} "${PROJECT_SOURCE_DIR}/lib/${OUTPUTCONFIG}")
endforeach()

# =============================================================
# SUBDIRECTORIES
# =============================================================
add_subdirectory(third_party)
add_subdirectory(src)

# =============================================================
# SHADER COMPILATION
# =============================================================
set(SHADER_SRC_DIR ${PROJECT_SOURCE_DIR}/shaders)
set(SHADER_BIN_DIR ${PROJECT_SOURCE_DIR}/shaders)
file(MAKE_DIRECTORY "${SHADER_BIN_DIR}")

file(GLOB_RECURSE GLSL_SOURCE_FILES
    "${SHADER_SRC_DIR}/*.frag"
    "${SHADER_SRC_DIR}/*.vert"
    "${SHADER_SRC_DIR}/*.comp"
)
list(REMOVE_DUPLICATES GLSL_SOURCE_FILES)

set(SHADER_INCLUDE_DIRS "${SHADER_SRC_DIR}")
set(SHADER_INCLUDE_ARGS "")
foreach(inc ${SHADER_INCLUDE_DIRS})
    file(TO_CMAKE_PATH "${inc}" inc_norm)
    list(APPEND SHADER_INCLUDE_ARGS "-I${inc_norm}")
endforeach()

set(SPIRV_BINARY_FILES "")
foreach(GLSL ${GLSL_SOURCE_FILES})
    get_filename_component(GLSL_DIR_ABS "${GLSL}" DIRECTORY)
    file(RELATIVE_PATH REL_DIR "${SHADER_SRC_DIR}" "${GLSL_DIR_ABS}")

    if(REL_DIR STREQUAL "")
        set(OUT_DIR "${SHADER_BIN_DIR}")
    else()
        set(OUT_DIR "${SHADER_BIN_DIR}/${REL_DIR}")
    endif()

    get_filename_component(FILE_NAME "${GLSL}" NAME)
    set(SPIRV "${OUT_DIR}/${FILE_NAME}.spv")

    add_custom_command(
        OUTPUT "${SPIRV}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${OUT_DIR}"
        COMMAND "${GLSL_VALIDATOR}" -V ${SHADER_INCLUDE_ARGS} ${SHADER_DEFINES} "${GLSL}" -o "${SPIRV}"
        DEPENDS "${GLSL}"
        COMMENT "Compiling shader: ${REL_DIR}/${FILE_NAME}"
        VERBATIM
        USES_TERMINAL
    )

    list(APPEND SPIRV_BINARY_FILES "${SPIRV}")
endforeach()

add_custom_target(Shaders ALL DEPENDS ${SPIRV_BINARY_FILES})
add_dependencies(Yalaz_Engine Shaders)

# =============================================================
# INSTALL RULES FOR RELEASE PACKAGING
# =============================================================

# Main executable
install(TARGETS Yalaz_Engine
    RUNTIME DESTINATION bin COMPONENT runtime
    BUNDLE DESTINATION . COMPONENT runtime
)

# SDL2 runtime library
if(WIN32)
    install(FILES $<TARGET_FILE:SDL2::SDL2> DESTINATION bin COMPONENT runtime)
elseif(APPLE)
    install(FILES $<TARGET_FILE:SDL2::SDL2> DESTINATION bin COMPONENT runtime)

    # Also copy MoltenVK/Vulkan libraries for standalone distribution
    if(EXISTS "${VULKAN_SDK_PATH}/lib/libvulkan.1.dylib")
        install(FILES
            "${VULKAN_SDK_PATH}/lib/libvulkan.1.dylib"
            DESTINATION bin COMPONENT runtime
            OPTIONAL
        )
    endif()
    if(EXISTS "${VULKAN_SDK_PATH}/lib/libMoltenVK.dylib")
        install(FILES
            "${VULKAN_SDK_PATH}/lib/libMoltenVK.dylib"
            DESTINATION bin COMPONENT runtime
            OPTIONAL
        )
    endif()

    # Copy Vulkan ICD configuration
    if(EXISTS "${VULKAN_SDK_PATH}/share/vulkan/icd.d")
        install(DIRECTORY "${VULKAN_SDK_PATH}/share/vulkan/icd.d"
            DESTINATION share/vulkan COMPONENT runtime
            OPTIONAL
        )
    endif()
elseif(UNIX)
    install(FILES $<TARGET_FILE:SDL2::SDL2> DESTINATION bin COMPONENT runtime)
endif()

# Compiled shaders
install(DIRECTORY ${PROJECT_SOURCE_DIR}/shaders/
    DESTINATION shaders COMPONENT shaders
    FILES_MATCHING
    PATTERN "*.spv"
    PATTERN "*.frag" EXCLUDE
    PATTERN "*.vert" EXCLUDE
    PATTERN "*.comp" EXCLUDE
    PATTERN "*.glsl" EXCLUDE
)

# Assets (optional)
if(YALAZ_INCLUDE_ASSETS AND EXISTS "${PROJECT_SOURCE_DIR}/assets")
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/assets/
        DESTINATION assets COMPONENT assets
        PATTERN "*.blend" EXCLUDE
        PATTERN "*.blend1" EXCLUDE
        PATTERN "*.fbx" EXCLUDE
        PATTERN ".git*" EXCLUDE
    )
endif()

# =============================================================
# RELEASE FOLDER TARGET
# =============================================================
# Creates a ready-to-distribute release folder

set(RELEASE_DIR "${PROJECT_SOURCE_DIR}/release")

# Custom target to create release folder
add_custom_target(create_release
    COMMAND ${CMAKE_COMMAND} -E make_directory "${RELEASE_DIR}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${RELEASE_DIR}/bin"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${RELEASE_DIR}/shaders"
    COMMENT "Creating release directory structure..."
)

# Copy executable to release folder
add_custom_target(release_executable
    DEPENDS Yalaz_Engine create_release
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:Yalaz_Engine>
        "${RELEASE_DIR}/bin/"
    COMMENT "Copying executable to release folder..."
)

# Copy SDL2 to release folder
add_custom_target(release_sdl2
    DEPENDS Yalaz_Engine create_release
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:SDL2::SDL2>
        "${RELEASE_DIR}/bin/"
    COMMENT "Copying SDL2 to release folder..."
)

# Copy only compiled shaders (.spv) to release folder
add_custom_target(release_shaders
    DEPENDS Shaders create_release
    COMMAND ${CMAKE_COMMAND} -E make_directory "${RELEASE_DIR}/shaders"
    COMMAND ${CMAKE_COMMAND}
        -DSOURCE_DIR="${PROJECT_SOURCE_DIR}/shaders"
        -DDEST_DIR="${RELEASE_DIR}/shaders"
        -P "${PROJECT_SOURCE_DIR}/cmake/CopyShaders.cmake"
    COMMENT "Copying compiled shaders to release folder..."
)

# Platform-specific release dependencies
if(APPLE)
    # Copy MoltenVK/Vulkan libraries for macOS
    add_custom_target(release_vulkan
        DEPENDS create_release
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${VULKAN_SDK_PATH}/lib/libMoltenVK.dylib"
            "${RELEASE_DIR}/bin/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${VULKAN_SDK_PATH}/lib/libvulkan.1.dylib"
            "${RELEASE_DIR}/bin/"
        COMMENT "Copying Vulkan libraries to release folder..."
    )

    # Create launcher script
    add_custom_target(release_launcher
        DEPENDS create_release
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${PROJECT_SOURCE_DIR}/scripts/run-yalaz.sh"
            "${RELEASE_DIR}/Yalaz-Engine.command"
        COMMAND chmod +x "${RELEASE_DIR}/Yalaz-Engine.command"
        COMMENT "Creating macOS launcher..."
    )

    # Fix RPATH for release executable
    add_custom_target(release_fix_rpath
        DEPENDS release_executable release_sdl2
        COMMAND install_name_tool -change
            "@rpath/$<TARGET_FILE_NAME:SDL2::SDL2>"
            "@executable_path/$<TARGET_FILE_NAME:SDL2::SDL2>"
            "${RELEASE_DIR}/bin/$<TARGET_FILE_NAME:Yalaz_Engine>"
        COMMENT "Fixing RPATH for standalone distribution..."
    )

    # Create symlink for easier access
    add_custom_target(release_symlink
        DEPENDS release_executable
        COMMAND ${CMAKE_COMMAND} -E create_symlink
            "Yalaz_Engine-${YALAZ_VERSION}"
            "${RELEASE_DIR}/bin/Yalaz_Engine"
        COMMENT "Creating executable symlink..."
    )

    # Main release target for macOS
    add_custom_target(release
        DEPENDS release_executable release_sdl2 release_shaders release_vulkan release_launcher release_fix_rpath release_symlink
        COMMENT "Release folder created at: ${RELEASE_DIR}"
    )

elseif(WIN32)
    # Create Windows launcher
    add_custom_target(release_launcher
        DEPENDS create_release
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${PROJECT_SOURCE_DIR}/scripts/run-yalaz.bat"
            "${RELEASE_DIR}/Yalaz-Engine.bat"
        COMMENT "Creating Windows launcher..."
    )

    # Copy MinGW runtime DLLs if using MinGW
    if(MINGW)
        get_filename_component(MINGW_BIN_PATH ${CMAKE_CXX_COMPILER} DIRECTORY)
        add_custom_target(release_mingw_runtime
            DEPENDS create_release
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${MINGW_BIN_PATH}/libgcc_s_seh-1.dll"
                "${RELEASE_DIR}/bin/" || true
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${MINGW_BIN_PATH}/libstdc++-6.dll"
                "${RELEASE_DIR}/bin/" || true
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${MINGW_BIN_PATH}/libwinpthread-1.dll"
                "${RELEASE_DIR}/bin/" || true
            COMMENT "Copying MinGW runtime DLLs..."
        )
    endif()

    # Main release target for Windows
    add_custom_target(release
        DEPENDS release_executable release_sdl2 release_shaders release_launcher
        $<$<BOOL:${MINGW}>:release_mingw_runtime>
        COMMENT "Release folder created at: ${RELEASE_DIR}"
    )

elseif(UNIX)
    # Create Linux launcher
    add_custom_target(release_launcher
        DEPENDS create_release
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${PROJECT_SOURCE_DIR}/scripts/run-yalaz.sh"
            "${RELEASE_DIR}/yalaz-engine.sh"
        COMMAND chmod +x "${RELEASE_DIR}/yalaz-engine.sh"
        COMMENT "Creating Linux launcher..."
    )

    # Create symlink for easier access
    add_custom_target(release_symlink
        DEPENDS release_executable
        COMMAND ${CMAKE_COMMAND} -E create_symlink
            "Yalaz_Engine-${YALAZ_VERSION}"
            "${RELEASE_DIR}/bin/Yalaz_Engine"
        COMMENT "Creating executable symlink..."
    )

    # Main release target for Linux
    add_custom_target(release
        DEPENDS release_executable release_sdl2 release_shaders release_launcher release_symlink
        COMMENT "Release folder created at: ${RELEASE_DIR}"
    )
endif()

# Optional: Include assets in release
if(YALAZ_INCLUDE_ASSETS AND EXISTS "${PROJECT_SOURCE_DIR}/assets")
    add_custom_target(release_assets
        DEPENDS create_release
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${PROJECT_SOURCE_DIR}/assets"
            "${RELEASE_DIR}/assets"
        COMMENT "Copying assets to release folder..."
    )
    add_dependencies(release release_assets)
endif()

# =============================================================
# PACKAGING (CPack)
# =============================================================
include(cmake/Packaging.cmake)

message(STATUS "========================================")
message(STATUS "Configuration complete!")
message(STATUS "")
message(STATUS "Build commands:")
message(STATUS "  cmake --build <build_dir>              # Build the project")
message(STATUS "  cmake --build <build_dir> -t release   # Create release/ folder")
message(STATUS "  cmake --build <build_dir> -t package   # Create installer package")
message(STATUS "")
message(STATUS "Output locations:")
message(STATUS "  Executable: bin/${CMAKE_BUILD_TYPE}/")
message(STATUS "  Release:    release/")
message(STATUS "========================================")
