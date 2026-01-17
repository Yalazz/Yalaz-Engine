cmake_minimum_required(VERSION 3.15)

find_package(Vulkan REQUIRED)

# ==========================================================
# fastgltf (simdjson will be downloaded automatically)
# ==========================================================

set(FASTGLTF_DOWNLOAD_SIMDJSON ON CACHE BOOL "" FORCE)

add_subdirectory(fastgltf)

# ==========================================================
# fmt (header-only)
# ==========================================================

add_library(fmt INTERFACE)
target_include_directories(fmt INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/fmt/include
)
target_compile_definitions(fmt INTERFACE FMT_HEADER_ONLY=1)

# ==========================================================
# SDL
# ==========================================================

add_subdirectory(SDL EXCLUDE_FROM_ALL)

# ==========================================================
# vkbootstrap
# ==========================================================

add_library(vkbootstrap STATIC
        vkbootstrap/VkBootstrap.h
        vkbootstrap/VkBootstrap.cpp
)

target_include_directories(vkbootstrap PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/vkbootstrap
)

target_link_libraries(vkbootstrap PUBLIC
        Vulkan::Vulkan
        $<$<BOOL:UNIX>:${CMAKE_DL_LIBS}>
)

set_property(TARGET vkbootstrap PROPERTY CXX_STANDARD 20)

# ==========================================================
# glm
# ==========================================================

add_library(glm INTERFACE)
target_include_directories(glm INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/glm
)

# ==========================================================
# vma
# ==========================================================

add_library(vma INTERFACE)
target_include_directories(vma INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/vma
)

# ==========================================================
# stb_image
# ==========================================================

add_library(stb_image INTERFACE)
target_include_directories(stb_image INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/stb_image
)

# ==========================================================
# tinyobjloader
# ==========================================================

add_library(tinyobjloader INTERFACE)
target_include_directories(tinyobjloader INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/tinyobjloader
)

# ==========================================================
# ImGui
# ==========================================================

add_library(imgui STATIC
        imgui/imgui.h
        imgui/imgui.cpp
        imgui/imgui_demo.cpp
        imgui/imgui_draw.cpp
        imgui/imgui_widgets.cpp
        imgui/imgui_tables.cpp
        imgui/imgui_impl_vulkan.cpp
        imgui/imgui_impl_sdl2.cpp
)

target_include_directories(imgui PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui
)

target_link_libraries(imgui PUBLIC
        Vulkan::Vulkan
        SDL2::SDL2
)

# ==========================================================
# ImGuizmo (klasör boşsa hata kırılmasın)
# ==========================================================

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ImGuizmo/ImGuizmo.cpp")
    add_library(ImGuizmo STATIC
            ${CMAKE_CURRENT_SOURCE_DIR}/ImGuizmo/ImGuizmo.cpp
    )

    target_include_directories(ImGuizmo PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/ImGuizmo
    )

    target_link_libraries(ImGuizmo PUBLIC
            imgui
    )
else()
    message(WARNING "ImGuizmo.cpp bulunamadı — ImGuizmo hedefi oluşturulmadı.")
endif()
