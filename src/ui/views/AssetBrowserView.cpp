// =============================================================================
// YALAZ ENGINE - Asset Browser View Implementation
// =============================================================================
// Professional asset browser with:
// - Multiple scene loading support
// - Grid and list view modes
// - Thumbnail size adjustment
// - Search filtering
// - Asset type detection
// - GLTF/OBJ/texture loading support
// - Loaded scenes management
// =============================================================================

#include "AssetBrowserView.h"
#include "../EditorTheme.h"
#include "../../vk_engine.h"
#include "../../vk_loader.h"
#include <filesystem>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;

namespace Yalaz::UI {

void AssetBrowserView::OnInit(VulkanEngine* engine) {
    EditorView::OnInit(engine);

    // Try to find the assets folder by checking multiple possible locations
    std::vector<std::string> possiblePaths = {
        "assets",                           // Direct (if running from project root)
        "../assets",                        // One level up
        "../../assets",                     // Two levels up (from bin/Debug or bin/Release)
        "../../../assets",                  // Three levels up
        "../../../../assets",               // Four levels up (from build/bin/Release etc.)
    };

    // Also try absolute paths based on executable location
    #ifdef _WIN32
    char exePath[MAX_PATH];
    if (GetModuleFileNameA(NULL, exePath, MAX_PATH)) {
        fs::path exeDir = fs::path(exePath).parent_path();
        // Try going up from executable directory
        for (int i = 0; i < 5; i++) {
            fs::path assetsPath = exeDir / "assets";
            if (fs::exists(assetsPath) && fs::is_directory(assetsPath)) {
                m_CurrentPath = assetsPath.string();
                m_RootPath = m_CurrentPath;
                RefreshDirectory();
                return;
            }
            exeDir = exeDir.parent_path();
        }
    }
    #endif

    // Try relative paths
    for (const auto& path : possiblePaths) {
        if (fs::exists(path) && fs::is_directory(path)) {
            m_CurrentPath = fs::absolute(path).string();
            m_RootPath = m_CurrentPath;
            RefreshDirectory();
            return;
        }
    }

    // Fallback to current directory
    m_CurrentPath = fs::current_path().string();
    m_RootPath = m_CurrentPath;
    RefreshDirectory();
}

void AssetBrowserView::OnRender() {
    if (!BeginView(ImGuiWindowFlags_MenuBar)) {
        EndView();
        return;
    }

    // Menu bar
    if (ImGui::BeginMenuBar()) {
        // View toggle
        if (ImGui::Button(m_GridView ? "Grid" : "List")) {
            m_GridView = !m_GridView;
        }

        ImGui::SameLine();
        if (ImGui::Button("Refresh")) {
            RefreshDirectory();
        }

        // Thumbnail size slider
        if (m_GridView) {
            ImGui::SameLine();
            ImGui::SetNextItemWidth(100);
            ImGui::SliderInt("##Size", &m_ThumbnailSize, 40, 150);
        }

        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();

        // Show loaded scenes toggle
        ImGui::Checkbox("Scenes", &m_ShowLoadedScenes);

        ImGui::EndMenuBar();
    }

    // Main content with optional scenes panel
    if (m_ShowLoadedScenes) {
        ImGui::Columns(2, "AssetBrowserColumns", true);
        ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * 0.7f);

        // Left: File browser
        RenderFileBrowser();

        ImGui::NextColumn();

        // Right: Loaded scenes
        RenderLoadedScenes();

        ImGui::Columns(1);
    } else {
        RenderFileBrowser();
    }

    EndView();
}

void AssetBrowserView::RenderFileBrowser() {
    // Path bar
    RenderPathBar();

    ImGui::Separator();

    // Search
    ImGui::SetNextItemWidth(-1);
    if (ImGui::InputTextWithHint("##Search", "Search assets...", m_SearchBuffer, sizeof(m_SearchBuffer))) {
        // Search changed
    }

    ImGui::Separator();

    // Asset grid/list
    ImGui::BeginChild("AssetContent");
    RenderAssetGrid();
    ImGui::EndChild();
}

void AssetBrowserView::RenderLoadedScenes() {
    SectionHeader("Loaded Scenes");

    if (!m_Engine) {
        ImGui::TextDisabled("No engine context");
        return;
    }

    if (m_Engine->loadedScenes.empty()) {
        ImGui::TextDisabled("No scenes loaded");
        ImGui::Spacing();
        ImGui::TextWrapped("Click on .gltf or .glb files to load scenes.");
        return;
    }

    ImGui::BeginChild("ScenesList", ImVec2(0, 0), true);

    int sceneIndex = 0;
    std::string sceneToRemove;

    for (auto& [sceneName, scene] : m_Engine->loadedScenes) {
        ImGui::PushID(sceneIndex++);

        bool isSelected = (m_SelectedScene == sceneName);

        // Scene card
        ImVec4 cardColor = isSelected ?
            ImVec4(0.25f, 0.4f, 0.55f, 1.0f) :
            ImVec4(0.18f, 0.18f, 0.22f, 1.0f);

        ImGui::PushStyleColor(ImGuiCol_ChildBg, cardColor);
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);

        if (ImGui::BeginChild("SceneCard", ImVec2(-1, 70), true)) {
            // Scene icon and name
            ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "[3D]");
            ImGui::SameLine();
            ImGui::Text("%s", sceneName.c_str());

            // Node count
            if (scene) {
                ImGui::TextDisabled("%zu nodes", scene->nodes.size());
            }

            // Actions
            if (ImGui::Button("Focus", ImVec2(50, 0))) {
                // Focus camera on scene - use topNodes for safer access
                if (scene && !scene->topNodes.empty()) {
                    auto& firstNode = scene->topNodes[0];
                    if (firstNode) {
                        glm::vec3 pos = glm::vec3(firstNode->worldTransform[3]);
                        m_Engine->mainCamera.focusOnPoint(pos, 10.0f);
                    }
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Unload", ImVec2(50, 0))) {
                sceneToRemove = sceneName;
            }

            // Selection handling
            if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0)) {
                m_SelectedScene = sceneName;
            }
        }
        ImGui::EndChild();

        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
        ImGui::PopID();

        ImGui::Spacing();
    }

    // Remove scene outside the loop - must wait for GPU before destroying resources
    if (!sceneToRemove.empty()) {
        // Wait for GPU to finish using the scene's resources
        if (m_Engine->_device != VK_NULL_HANDLE) {
            vkDeviceWaitIdle(m_Engine->_device);
        }

        // Clear selected node if it belonged to this scene
        if (m_Engine->selectedNode != nullptr) {
            // Check if selected node is from this scene
            auto it = m_Engine->loadedScenes.find(sceneToRemove);
            if (it != m_Engine->loadedScenes.end() && it->second) {
                for (const auto& [name, node] : it->second->nodes) {
                    if (node.get() == m_Engine->selectedNode) {
                        m_Engine->selectedNode = nullptr;
                        m_Engine->selectedObjectName.clear();
                        break;
                    }
                }
            }
        }

        m_Engine->loadedScenes.erase(sceneToRemove);
        if (m_SelectedScene == sceneToRemove) {
            m_SelectedScene.clear();
        }

        // Notify path tracer that scene geometry changed
        if (m_Engine->_pathTracer) {
            m_Engine->_pathTracer->notifySceneChanged();
        }
    }

    ImGui::EndChild();

    // Stats
    ImGui::Separator();
    ImGui::TextDisabled("%zu scenes loaded", m_Engine->loadedScenes.size());
}

void AssetBrowserView::RenderPathBar() {
    ImGui::Text("Path:");
    ImGui::SameLine();

    // Up button
    if (ImGui::Button("^")) {
        fs::path p(m_CurrentPath);
        if (p.has_parent_path() && p.parent_path() != p) {
            m_CurrentPath = p.parent_path().string();
            RefreshDirectory();
        }
    }
    ImGui::SameLine();

    // Home button - go to root assets folder
    if (ImGui::Button("Home")) {
        if (!m_RootPath.empty() && fs::exists(m_RootPath)) {
            m_CurrentPath = m_RootPath;
        } else {
            m_CurrentPath = fs::current_path().string();
        }
        RefreshDirectory();
    }
    ImGui::SameLine();

    // Path segments
    fs::path current(m_CurrentPath);
    fs::path accumulated;

    for (const auto& part : current) {
        accumulated /= part;
        if (ImGui::Button(part.string().c_str())) {
            m_CurrentPath = accumulated.string();
            RefreshDirectory();
        }
        ImGui::SameLine();
        ImGui::TextDisabled("/");
        ImGui::SameLine();
    }
    ImGui::NewLine();
}

void AssetBrowserView::RenderAssetGrid() {
    float cellSize = static_cast<float>(m_ThumbnailSize) + 20;
    float panelWidth = ImGui::GetContentRegionAvail().x;
    int columns = std::max(1, static_cast<int>(panelWidth / cellSize));

    std::string lowerSearch = m_SearchBuffer;
    std::transform(lowerSearch.begin(), lowerSearch.end(), lowerSearch.begin(), ::tolower);

    ImGui::Columns(columns, nullptr, false);

    for (auto& asset : m_Assets) {
        // Search filter
        std::string lowerName = asset.name;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
        if (!lowerSearch.empty() && lowerName.find(lowerSearch) == std::string::npos) {
            continue;
        }

        ImGui::PushID(asset.path.c_str());

        // Icon/thumbnail button with type-based color
        ImVec4 buttonColor;
        switch (asset.type) {
            case AssetType::Folder:  buttonColor = ImVec4(0.3f, 0.3f, 0.5f, 1.0f); break;
            case AssetType::Model:   buttonColor = ImVec4(0.2f, 0.4f, 0.3f, 1.0f); break;
            case AssetType::Texture: buttonColor = ImVec4(0.4f, 0.3f, 0.2f, 1.0f); break;
            case AssetType::Shader:  buttonColor = ImVec4(0.4f, 0.2f, 0.4f, 1.0f); break;
            case AssetType::Scene:   buttonColor = ImVec4(0.2f, 0.3f, 0.4f, 1.0f); break;
            default:                 buttonColor = ImVec4(0.2f, 0.2f, 0.25f, 1.0f); break;
        }

        ImGui::PushStyleColor(ImGuiCol_Button, buttonColor);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
            ImVec4(buttonColor.x + 0.1f, buttonColor.y + 0.1f, buttonColor.z + 0.1f, 1.0f));

        // Single click to select, double click to open/load
        if (ImGui::Button(GetAssetIcon(asset.type), ImVec2((float)m_ThumbnailSize, (float)m_ThumbnailSize))) {
            // Single click - for folders, navigate; for files, just select
            if (asset.isDirectory) {
                m_CurrentPath = asset.path;
                RefreshDirectory();
            }
        }

        // Double-click to load models/scenes
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
            if (!asset.isDirectory) {
                HandleAssetClick(asset);  // Load the asset
            }
        }

        // Drag source for textures
        if (asset.type == AssetType::Texture && ImGui::BeginDragDropSource()) {
            ImGui::SetDragDropPayload("TEXTURE_PATH", asset.path.c_str(), asset.path.size() + 1);
            ImGui::Text("Texture: %s", asset.name.c_str());
            ImGui::EndDragDropSource();
        }

        // Context menu
        if (ImGui::BeginPopupContextItem()) {
            RenderAssetContextMenu(asset);
            ImGui::EndPopup();
        }

        ImGui::PopStyleColor(2);

        // Tooltip
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("%s", asset.name.c_str());
            ImGui::TextDisabled("Type: %s", GetAssetTypeName(asset.type));
            ImGui::TextDisabled("Path: %s", asset.path.c_str());
            ImGui::EndTooltip();
        }

        // Name label
        ImGui::TextWrapped("%s", asset.name.c_str());

        ImGui::PopID();
        ImGui::NextColumn();
    }

    ImGui::Columns(1);
}

void AssetBrowserView::HandleAssetClick(const AssetEntry& asset) {
    // This is called on double-click for loading files
    if (asset.isDirectory) {
        return;  // Folders are handled by single-click in RenderAssetGrid
    }

    if (!m_Engine) return;

    std::string ext = fs::path(asset.name).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == ".gltf" || ext == ".glb") {
        // Load GLTF model
        auto result = loadGltf(m_Engine, asset.path);
        if (result.has_value()) {
            std::string sceneName = fs::path(asset.name).stem().string();

            // Handle duplicate names
            int counter = 1;
            std::string baseName = sceneName;
            while (m_Engine->loadedScenes.find(sceneName) != m_Engine->loadedScenes.end()) {
                sceneName = baseName + "_" + std::to_string(counter++);
            }

            m_Engine->loadedScenes[sceneName] = result.value();
            m_SelectedScene = sceneName;
            m_ShowLoadedScenes = true;
        }
    } else if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga" || ext == ".hdr") {
        // Store selected texture path for material editor
        m_LastSelectedTexture = asset.path;
    }
}

void AssetBrowserView::RenderAssetContextMenu(const AssetEntry& asset) {
    if (asset.isDirectory) {
        if (ImGui::MenuItem("Open")) {
            m_CurrentPath = asset.path;
            RefreshDirectory();
        }
    } else {
        std::string ext = fs::path(asset.name).extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        if (ext == ".gltf" || ext == ".glb") {
            if (ImGui::MenuItem("Load Scene")) {
                HandleAssetClick(asset);
            }
            if (ImGui::MenuItem("Load as New Instance")) {
                // Load with unique name
                HandleAssetClick(asset);
            }
        }

        if (asset.type == AssetType::Texture) {
            if (ImGui::MenuItem("Set as Albedo")) {
                // TODO: Set texture in material system
            }
            if (ImGui::MenuItem("Set as Normal")) {
                // TODO: Set texture in material system
            }
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Show in Explorer")) {
            // Platform specific file explorer
#ifdef _WIN32
            std::string cmd = "explorer /select,\"" + asset.path + "\"";
            system(cmd.c_str());
#endif
        }
    }

    ImGui::Separator();

    if (ImGui::MenuItem("Copy Path")) {
        ImGui::SetClipboardText(asset.path.c_str());
    }
}

void AssetBrowserView::RefreshDirectory() {
    m_Assets.clear();

    if (!fs::exists(m_CurrentPath)) return;

    try {
        for (const auto& entry : fs::directory_iterator(m_CurrentPath)) {
            AssetEntry asset;
            asset.name = entry.path().filename().string();
            asset.path = entry.path().string();
            asset.isDirectory = entry.is_directory();

            if (asset.isDirectory) {
                asset.type = AssetType::Folder;
            } else {
                std::string ext = entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                asset.type = GetAssetType(ext);
            }

            // Skip hidden files
            if (!asset.name.empty() && asset.name[0] == '.') continue;

            m_Assets.push_back(asset);
        }

        // Sort: folders first, then by name
        std::sort(m_Assets.begin(), m_Assets.end(), [](const AssetEntry& a, const AssetEntry& b) {
            if (a.isDirectory != b.isDirectory) return a.isDirectory > b.isDirectory;
            return a.name < b.name;
        });
    } catch (...) {
        // Handle permission errors, etc.
    }
}

AssetType AssetBrowserView::GetAssetType(const std::string& ext) {
    if (ext == ".gltf" || ext == ".glb" || ext == ".obj" || ext == ".fbx") {
        return AssetType::Model;
    }
    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga" || ext == ".hdr" || ext == ".bmp") {
        return AssetType::Texture;
    }
    if (ext == ".glsl" || ext == ".vert" || ext == ".frag" || ext == ".comp" || ext == ".spv") {
        return AssetType::Shader;
    }
    if (ext == ".scene" || ext == ".json") {
        return AssetType::Scene;
    }
    if (ext == ".mtl" || ext == ".mat") {
        return AssetType::Material;
    }
    return AssetType::Unknown;
}

const char* AssetBrowserView::GetAssetIcon(AssetType type) {
    switch (type) {
        case AssetType::Folder:   return "[DIR]";
        case AssetType::Model:    return "[3D]";
        case AssetType::Texture:  return "[TEX]";
        case AssetType::Shader:   return "[SHD]";
        case AssetType::Scene:    return "[SCN]";
        case AssetType::Material: return "[MAT]";
        default:                  return "[?]";
    }
}

const char* AssetBrowserView::GetAssetTypeName(AssetType type) {
    switch (type) {
        case AssetType::Folder:   return "Folder";
        case AssetType::Model:    return "3D Model";
        case AssetType::Texture:  return "Texture";
        case AssetType::Shader:   return "Shader";
        case AssetType::Scene:    return "Scene";
        case AssetType::Material: return "Material";
        default:                  return "Unknown";
    }
}

} // namespace Yalaz::UI
