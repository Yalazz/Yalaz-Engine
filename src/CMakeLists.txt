# =============================================================
# YALAZ ENGINE - Main Executable Target
# =============================================================

# Configure version header
configure_file(
    "${CMAKE_SOURCE_DIR}/cmake/version_config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/version_config.h"
    @ONLY
)

# =============================================================
# EXECUTABLE DEFINITION
# =============================================================
add_executable(Yalaz_Engine
    main.cpp
    vk_engine.cpp
    vk_engine.h
    vk_types.h
    vk_initializers.cpp
    vk_initializers.h
    vk_images.h
    vk_images.cpp
    vk_descriptors.h
    vk_descriptors.cpp
    vk_pipelines.h
    vk_pipelines.cpp
    vk_loader.h
    vk_loader.cpp
    camera.cpp
    camera.h

    # UI System - Professional OOP Architecture
    ui/Panel.h
    ui/EditorTheme.h
    ui/EditorSelection.h
    ui/PanelManager.h
    ui/EditorUI.h
    ui/EditorUI.cpp
    ui/panels/SceneHierarchyPanel.h
    ui/panels/SceneHierarchyPanel.cpp
    ui/panels/InspectorPanel.h
    ui/panels/InspectorPanel.cpp
    ui/panels/ViewportPanel.h
    ui/panels/ViewportPanel.cpp
    ui/panels/LightingPanel.h
    ui/panels/LightingPanel.cpp
    ui/panels/ConsolePanel.h
    ui/panels/ConsolePanel.cpp
)

# =============================================================
# EXECUTABLE PROPERTIES
# =============================================================

# Set executable properties for different platforms
set_target_properties(Yalaz_Engine PROPERTIES
    OUTPUT_NAME "Yalaz_Engine"
    VERSION "${PROJECT_VERSION}"
)

# Windows specific: Set as GUI application (no console window in Release)
if(WIN32)
    set_target_properties(Yalaz_Engine PROPERTIES
        WIN32_EXECUTABLE $<NOT:$<CONFIG:Debug>>
    )
endif()

# macOS specific: Create app bundle for Release
if(APPLE)
    # Get Vulkan SDK library path for rpath
    get_filename_component(VULKAN_LIB_DIR "${Vulkan_LIBRARY}" DIRECTORY)

    set_target_properties(Yalaz_Engine PROPERTIES
        MACOSX_BUNDLE FALSE  # Set to TRUE if you want .app bundle
        # Include both executable path (for SDL2) and Vulkan SDK (for libvulkan)
        BUILD_RPATH "${VULKAN_LIB_DIR}"
        INSTALL_RPATH "@executable_path"
        BUILD_WITH_INSTALL_RPATH FALSE
    )
endif()

# Linux specific: Set RPATH
if(UNIX AND NOT APPLE)
    set_target_properties(Yalaz_Engine PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_WITH_INSTALL_RPATH TRUE
    )
endif()

# =============================================================
# PRECOMPILED HEADERS
# =============================================================
target_precompile_headers(Yalaz_Engine PUBLIC
    <optional>
    <vector>
    <memory>
    <string>
    <unordered_map>
    <glm/mat4x4.hpp>
    <glm/vec4.hpp>
    <vulkan/vulkan.h>
)

# =============================================================
# INCLUDE DIRECTORIES
# =============================================================
target_include_directories(Yalaz_Engine PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/ui/panels"
    "${CMAKE_CURRENT_BINARY_DIR}"  # For version_config.h
    "${CMAKE_SOURCE_DIR}/third_party/fastgltf/include"
    "${CMAKE_SOURCE_DIR}/third_party/tinyobjloader"
    "${CMAKE_SOURCE_DIR}/third_party/ImGuizmo"
)

# =============================================================
# ADDITIONAL SOURCES
# =============================================================
target_sources(Yalaz_Engine PRIVATE
    ${CMAKE_SOURCE_DIR}/third_party/tinyobjloader/tiny_obj_loader.cc
    "${CMAKE_SOURCE_DIR}/third_party/ImGuizmo/ImGuizmo.cpp"
)

# =============================================================
# PLATFORM-SPECIFIC LIBRARIES
# =============================================================

# Core libraries (all platforms)
target_link_libraries(Yalaz_Engine PUBLIC
    SDL2::SDL2main
    SDL2::SDL2
    vkbootstrap
    vma
    glm
    fmt
    imgui
    fastgltf
    stb_image
    tinyobjloader
    Vulkan::Vulkan
)

# Platform-specific libraries
if(WIN32)
    # Windows-specific libraries
    target_link_libraries(Yalaz_Engine PUBLIC
        # Add Windows specific libs if needed
    )
elseif(APPLE)
    # macOS-specific libraries
    find_library(COCOA_LIBRARY Cocoa REQUIRED)
    find_library(IOKIT_LIBRARY IOKit REQUIRED)
    find_library(COREVIDEO_LIBRARY CoreVideo REQUIRED)
    find_library(QUARTZCORE_LIBRARY QuartzCore REQUIRED)
    find_library(METAL_LIBRARY Metal REQUIRED)

    target_link_libraries(Yalaz_Engine PUBLIC
        ${COCOA_LIBRARY}
        ${IOKIT_LIBRARY}
        ${COREVIDEO_LIBRARY}
        ${QUARTZCORE_LIBRARY}
        ${METAL_LIBRARY}
    )

    # Enable MoltenVK portability subset
    target_compile_definitions(Yalaz_Engine PUBLIC VK_ENABLE_BETA_EXTENSIONS)

elseif(UNIX)
    # Linux-specific libraries
    find_package(Threads REQUIRED)
    find_package(X11 REQUIRED)

    target_link_libraries(Yalaz_Engine PUBLIC
        Threads::Threads
        ${X11_LIBRARIES}
        ${CMAKE_DL_LIBS}
    )

    # Check for Wayland support (optional)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(WAYLAND wayland-client wayland-cursor wayland-egl)
        if(WAYLAND_FOUND)
            target_link_libraries(Yalaz_Engine PUBLIC ${WAYLAND_LIBRARIES})
            target_compile_definitions(Yalaz_Engine PUBLIC SDL_VIDEO_DRIVER_WAYLAND)
        endif()
    endif()
endif()

# Shader dependency
add_dependencies(Yalaz_Engine Shaders)

# =============================================================
# POST-BUILD: COPY RUNTIME DEPENDENCIES
# =============================================================

if(WIN32)
    # Windows: Copy SDL2.dll to the output directory
    add_custom_command(TARGET Yalaz_Engine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL2::SDL2>
            $<TARGET_FILE_DIR:Yalaz_Engine>
        COMMENT "Copying SDL2.dll to output directory"
    )
elseif(APPLE)
    # macOS: Copy SDL2 dylib to the output directory
    add_custom_command(TARGET Yalaz_Engine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL2::SDL2>
            $<TARGET_FILE_DIR:Yalaz_Engine>
        COMMENT "Copying SDL2 library to output directory"
    )

    # macOS: Copy Vulkan library to the output directory
    get_filename_component(VULKAN_LIB_DIR "${Vulkan_LIBRARY}" DIRECTORY)
    add_custom_command(TARGET Yalaz_Engine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${VULKAN_LIB_DIR}/libvulkan.1.dylib"
            $<TARGET_FILE_DIR:Yalaz_Engine>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${VULKAN_LIB_DIR}/libMoltenVK.dylib"
            $<TARGET_FILE_DIR:Yalaz_Engine>
        COMMENT "Copying Vulkan/MoltenVK libraries to output directory"
    )

    # Fix the rpath so the executable finds local libraries
    add_custom_command(TARGET Yalaz_Engine POST_BUILD
        COMMAND install_name_tool -change
            "@rpath/$<TARGET_FILE_NAME:SDL2::SDL2>"
            "@executable_path/$<TARGET_FILE_NAME:SDL2::SDL2>"
            $<TARGET_FILE:Yalaz_Engine>
        COMMAND install_name_tool -change
            "@rpath/libvulkan.1.dylib"
            "@executable_path/libvulkan.1.dylib"
            $<TARGET_FILE:Yalaz_Engine>
        COMMENT "Fixing library rpaths for standalone distribution"
    )

    # Create double-clickable .command launcher for Finder
    add_custom_command(TARGET Yalaz_Engine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "#!/bin/bash" > "$<TARGET_FILE_DIR:Yalaz_Engine>/Yalaz_Engine.command"
        COMMAND ${CMAKE_COMMAND} -E echo "cd \\\"\\$$\(dirname \\\"\\$$0\\\"\)\\\"" >> "$<TARGET_FILE_DIR:Yalaz_Engine>/Yalaz_Engine.command"
        COMMAND ${CMAKE_COMMAND} -E echo "./$<TARGET_FILE_NAME:Yalaz_Engine>" >> "$<TARGET_FILE_DIR:Yalaz_Engine>/Yalaz_Engine.command"
        COMMAND chmod +x "$<TARGET_FILE_DIR:Yalaz_Engine>/Yalaz_Engine.command"
        COMMENT "Creating Finder launcher (.command)"
    )
elseif(UNIX)
    # Linux: Copy SDL2 library to the output directory
    add_custom_command(TARGET Yalaz_Engine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL2::SDL2>
            $<TARGET_FILE_DIR:Yalaz_Engine>
        COMMENT "Copying SDL2 library to output directory"
    )
endif()
