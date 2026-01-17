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

target_include_directories(Yalaz_Engine PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"                             # src/
    "${CMAKE_CURRENT_SOURCE_DIR}/ui"                          # UI system
    "${CMAKE_CURRENT_SOURCE_DIR}/ui/panels"                   # UI panels
    "${CMAKE_SOURCE_DIR}/third_party/fastgltf/include"        # fastgltf
    "${CMAKE_SOURCE_DIR}/third_party/tinyobjloader"           # tinyobjloader
    "${CMAKE_SOURCE_DIR}/third_party/ImGuizmo"                # ImGuizmo header dosyaları
)

target_sources(Yalaz_Engine PRIVATE
        ${CMAKE_SOURCE_DIR}/third_party/tinyobjloader/tiny_obj_loader.cc
        "${CMAKE_SOURCE_DIR}/third_party/ImGuizmo/ImGuizmo.cpp"
)


# Bağlantı yapılacak kütüphaneler
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

add_dependencies(Yalaz_Engine Shaders)
