cmake_minimum_required(VERSION 3.15)

# Vulkan SDK'yı bul
find_package(Vulkan REQUIRED)

# simdjson configuration for fastgltf
# Note: simdjson has compilation issues with MinGW due to always_inline mismatch
# We disable the built-in download and provide simdjson as a separate library

set(FASTGLTF_DOWNLOAD_SIMDJSON OFF CACHE BOOL "" FORCE)

# Build simdjson as a separate static library for MinGW compatibility
set(SIMDJSON_DIR "${CMAKE_CURRENT_SOURCE_DIR}/fastgltf/deps/simdjson")
if (EXISTS "${SIMDJSON_DIR}/simdjson.h")
    add_library(simdjson STATIC "${SIMDJSON_DIR}/simdjson.cpp")
    target_include_directories(simdjson PUBLIC
        $<BUILD_INTERFACE:${SIMDJSON_DIR}>
        $<INSTALL_INTERFACE:include>)
    target_compile_features(simdjson PRIVATE cxx_std_17)
    # Disable problematic optimizations for GCC/MinGW
    if (NOT MSVC)
        target_compile_options(simdjson PRIVATE
            -fno-tree-vectorize
            -fno-strict-aliasing
        )
        # Override simdjson_inline to remove always_inline which causes issues on MinGW
        target_compile_definitions(simdjson PRIVATE
            SIMDJSON_NO_PORTABILITY_WARNING=1
            simdjson_inline=inline
            simdjson_warn_unused=
        )
    endif()
    add_library(simdjson::simdjson ALIAS simdjson)

    # Export simdjson so fastgltf install rules work
    install(TARGETS simdjson
        EXPORT fastgltf-targets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
    )

    message(STATUS "simdjson: Built as standalone library")
else()
    message(FATAL_ERROR "simdjson not found at ${SIMDJSON_DIR}")
endif()

# fastgltf will use find_package(simdjson) which will find our target
add_subdirectory(fastgltf)

# fmt ve SDL dış bağımlılıkları derleme dışında bırakılıyor
add_subdirectory(fmt EXCLUDE_FROM_ALL)
add_subdirectory(SDL EXCLUDE_FROM_ALL)

# vkbootstrap kütüphanesi
add_library(vkbootstrap STATIC)
target_sources(vkbootstrap PRIVATE 
  vkbootstrap/VkBootstrap.h
  vkbootstrap/VkBootstrap.cpp
)
target_include_directories(vkbootstrap PUBLIC vkbootstrap)
target_link_libraries(vkbootstrap PUBLIC Vulkan::Vulkan $<$<BOOL:UNIX>:${CMAKE_DL_LIBS}>)
set_property(TARGET vkbootstrap PROPERTY CXX_STANDARD 20)

# glm (başlık dosyası)
add_library(glm INTERFACE)
target_include_directories(glm INTERFACE glm)

# vma (başlık dosyası)
add_library(vma INTERFACE)
target_include_directories(vma INTERFACE vma)

# stb_image (başlık dosyası)
add_library(stb_image INTERFACE)
target_include_directories(stb_image INTERFACE stb_image)

# tinyobjloader (header-only)
add_library(tinyobjloader INTERFACE)
target_include_directories(tinyobjloader INTERFACE tinyobjloader)

# ImGui kütüphanesi
add_library(imgui STATIC)
target_include_directories(imgui PUBLIC imgui)
target_sources(imgui PRIVATE 
  imgui/imgui.h
  imgui/imgui.cpp
  imgui/imgui_demo.cpp
  imgui/imgui_draw.cpp
  imgui/imgui_widgets.cpp
  imgui/imgui_tables.cpp
  imgui/imgui_impl_vulkan.cpp
  imgui/imgui_impl_sdl2.cpp
)
target_link_libraries(imgui PUBLIC Vulkan::Vulkan SDL2::SDL2)

# ✅ ImGuizmo kütüphanesi
add_library(ImGuizmo STATIC
    ImGuizmo/ImGuizmo.cpp
)
target_include_directories(ImGuizmo PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/ImGuizmo)
target_link_libraries(ImGuizmo PUBLIC imgui)
