// =============================================================================
// YALAZ ENGINE - Asset Browser View Implementation
// =============================================================================
// Professional asset browser with:
// - Thumbnail previews for textures (actual image thumbnails)
// - Custom vector icons for folders, models, shaders, audio, etc.
// - Multiple scene loading support
// - Grid and list view modes
// - Search filtering and asset type detection
// =============================================================================

#include "AssetBrowserView.h"
#include "../EditorTheme.h"
#include "../../vk_engine.h"
#include "../../vk_loader.h"
#include <filesystem>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <stb_image.h>
#include <imgui_impl_vulkan.h>

#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;

namespace Yalaz::UI {

// Thumbnail resolution - images are downscaled to this size on CPU before GPU upload
static constexpr int THUMB_RES = 128;

AssetBrowserView::~AssetBrowserView() {
    ClearThumbnailCache();
}

void AssetBrowserView::OnInit(VulkanEngine* engine) {
    EditorView::OnInit(engine);

    // Try to find the assets folder by checking multiple possible locations
    std::vector<std::string> possiblePaths = {
        "assets",
        "../assets",
        "../../assets",
        "../../../assets",
        "../../../../assets",
    };

    #ifdef _WIN32
    char exePath[MAX_PATH];
    if (GetModuleFileNameA(NULL, exePath, MAX_PATH)) {
        fs::path exeDir = fs::path(exePath).parent_path();
        for (int i = 0; i < 5; i++) {
            fs::path assetsPath = exeDir / "assets";
            if (fs::exists(assetsPath) && fs::is_directory(assetsPath)) {
            m_CurrentPath = assetsPath.string();
            m_RootPath = m_CurrentPath;
            strncpy(m_PathBuffer, m_CurrentPath.c_str(), sizeof(m_PathBuffer) - 1);
            m_PathBuffer[sizeof(m_PathBuffer) - 1] = '\0';
            RefreshDirectory();
            return;
            }
            exeDir = exeDir.parent_path();
        }
    }
    #endif

    for (const auto& path : possiblePaths) {
        if (fs::exists(path) && fs::is_directory(path)) {
            m_CurrentPath = fs::absolute(path).string();
            m_RootPath = m_CurrentPath;
            strncpy(m_PathBuffer, m_CurrentPath.c_str(), sizeof(m_PathBuffer) - 1);
            m_PathBuffer[sizeof(m_PathBuffer) - 1] = '\0';
            RefreshDirectory();
            return;
        }
    }

    m_CurrentPath = fs::current_path().string();
    m_RootPath = m_CurrentPath;
    strncpy(m_PathBuffer, m_CurrentPath.c_str(), sizeof(m_PathBuffer) - 1);
    m_PathBuffer[sizeof(m_PathBuffer) - 1] = '\0';
    RefreshDirectory();
}

void AssetBrowserView::OnRender() {
    if (!BeginView(ImGuiWindowFlags_MenuBar)) {
        EndView();
        return;
    }

    // Menu bar
    if (ImGui::BeginMenuBar()) {
        if (ImGui::Button(m_GridView ? "Grid" : "List")) {
            m_GridView = !m_GridView;
        }

        ImGui::SameLine();
        if (ImGui::Button("Refresh")) {
            ClearThumbnailCache();
            RefreshDirectory();
        }

        if (m_GridView) {
            ImGui::SameLine();
            ImGui::SetNextItemWidth(100);
            ImGui::SliderInt("##Size", &m_ThumbnailSize, 50, 180);
        }

        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();

        ImGui::Checkbox("Scenes", &m_ShowLoadedScenes);

        ImGui::EndMenuBar();
    }

    // Main content with optional scenes panel
    if (m_ShowLoadedScenes) {
        ImGui::Columns(2, "AssetBrowserColumns", true);
        ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * 0.7f);

        RenderFileBrowser();
        ImGui::NextColumn();
        RenderLoadedScenes();
        ImGui::Columns(1);
    } else {
        RenderFileBrowser();
    }

    EndView();
}

void AssetBrowserView::RenderFileBrowser() {
    RenderPathBar();
    ImGui::Separator();

    // Search
    ImGui::SetNextItemWidth(-1);
    if (ImGui::InputTextWithHint("##Search", "Search assets...", m_SearchBuffer, sizeof(m_SearchBuffer))) {
        // Search changed
    }

    ImGui::Separator();

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
        ImGui::TextWrapped("Double-click model files (.gltf/.glb/.obj/.fbx/.dae/.mtl) to load scenes.");
        return;
    }

    ImGui::BeginChild("ScenesList", ImVec2(0, 0), true);

    int sceneIndex = 0;
    std::string sceneToRemove;

    for (auto& [sceneName, scene] : m_Engine->loadedScenes) {
        ImGui::PushID(sceneIndex++);

        bool isSelected = (m_SelectedScene == sceneName);

        ImVec4 cardColor = isSelected ?
            ImVec4(0.25f, 0.4f, 0.55f, 1.0f) :
            ImVec4(0.18f, 0.18f, 0.22f, 1.0f);

        ImGui::PushStyleColor(ImGuiCol_ChildBg, cardColor);
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);

        if (ImGui::BeginChild("SceneCard", ImVec2(-1, 70), true)) {
            ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "[3D]");
            ImGui::SameLine();
            ImGui::Text("%s", sceneName.c_str());

            if (scene) {
                ImGui::TextDisabled("%zu nodes", scene->nodes.size());
            }

            if (ImGui::Button("Focus", ImVec2(50, 0))) {
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

    if (!sceneToRemove.empty()) {
        // Defer scene destruction to start of next frame (after GPU fence wait)
        // This prevents destroying resources while the current command buffer still references them
        bool alreadyQueued = false;
        for (const auto& queued : m_Engine->_pendingSceneUnloads) {
            if (queued == sceneToRemove) {
                alreadyQueued = true;
                break;
            }
        }
        if (!alreadyQueued) {
            m_Engine->_pendingSceneUnloads.push_back(sceneToRemove);
        }

        if (m_SelectedScene == sceneToRemove) {
            m_SelectedScene.clear();
        }
    }

    ImGui::EndChild();

    ImGui::Separator();
    ImGui::TextDisabled("%zu scenes loaded", m_Engine->loadedScenes.size());
}

void AssetBrowserView::RenderPathBar() {
    ImGui::Text("Path:");
    ImGui::SameLine();

    if (ImGui::Button("^")) {
        fs::path p(m_CurrentPath);
        if (p.has_parent_path() && p.parent_path() != p) {
            m_CurrentPath = p.parent_path().string();
            strncpy(m_PathBuffer, m_CurrentPath.c_str(), sizeof(m_PathBuffer) - 1);
            m_PathBuffer[sizeof(m_PathBuffer) - 1] = '\0';
            RefreshDirectory();
        }
    }
    ImGui::SameLine();

    if (ImGui::Button("Home")) {
        if (!m_RootPath.empty() && fs::exists(m_RootPath)) {
            m_CurrentPath = m_RootPath;
        } else {
            m_CurrentPath = fs::current_path().string();
        }
        strncpy(m_PathBuffer, m_CurrentPath.c_str(), sizeof(m_PathBuffer) - 1);
        m_PathBuffer[sizeof(m_PathBuffer) - 1] = '\0';
        RefreshDirectory();
    }
    ImGui::SameLine();

    ImGui::SetNextItemWidth(260.0f);
    if (ImGui::InputText("##AssetPath", m_PathBuffer, sizeof(m_PathBuffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
        fs::path p(m_PathBuffer);
        if (fs::exists(p) && fs::is_directory(p)) {
            m_CurrentPath = fs::absolute(p).string();
            RefreshDirectory();
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Go")) {
        fs::path p(m_PathBuffer);
        if (fs::exists(p) && fs::is_directory(p)) {
            m_CurrentPath = fs::absolute(p).string();
            RefreshDirectory();
        }
    }
    ImGui::SameLine();

    fs::path current(m_CurrentPath);
    fs::path accumulated;

    for (const auto& part : current) {
        accumulated /= part;
        if (ImGui::Button(part.string().c_str())) {
            m_CurrentPath = accumulated.string();
            strncpy(m_PathBuffer, m_CurrentPath.c_str(), sizeof(m_PathBuffer) - 1);
            m_PathBuffer[sizeof(m_PathBuffer) - 1] = '\0';
            RefreshDirectory();
        }
        ImGui::SameLine();
        ImGui::TextDisabled("/");
        ImGui::SameLine();
    }
    ImGui::NewLine();
}

void AssetBrowserView::RenderAssetGrid() {
    float cellSize = static_cast<float>(m_ThumbnailSize) + 24;
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
        RenderAssetCard(asset, static_cast<float>(m_ThumbnailSize));
        ImGui::PopID();
        ImGui::NextColumn();
    }

    ImGui::Columns(1);

    // Defer directory changes until after iteration to avoid invalidating m_Assets mid-loop.
    if (!m_PendingNavigatePath.empty()) {
        m_CurrentPath = m_PendingNavigatePath;
        strncpy(m_PathBuffer, m_CurrentPath.c_str(), sizeof(m_PathBuffer) - 1);
        m_PathBuffer[sizeof(m_PathBuffer) - 1] = '\0';
        m_PendingNavigatePath.clear();
        RefreshDirectory();
    }
}

void AssetBrowserView::RenderAssetCard(const AssetEntry& asset, float size) {
    ImVec2 cardSize(size, size);
    ImVec2 cursorPos = ImGui::GetCursorScreenPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // Background colors per asset type
    ImU32 bgColor, bgColorHover;
    switch (asset.type) {
        case AssetType::Folder:
            bgColor = IM_COL32(50, 50, 75, 255);
            bgColorHover = IM_COL32(65, 65, 100, 255);
            break;
        case AssetType::Model:
            bgColor = IM_COL32(35, 70, 55, 255);
            bgColorHover = IM_COL32(45, 90, 70, 255);
            break;
        case AssetType::Texture:
        case AssetType::HDRI:
            bgColor = IM_COL32(55, 45, 35, 255);
            bgColorHover = IM_COL32(75, 60, 45, 255);
            break;
        case AssetType::Shader:
            bgColor = IM_COL32(60, 35, 60, 255);
            bgColorHover = IM_COL32(80, 45, 80, 255);
            break;
        case AssetType::Scene:
            bgColor = IM_COL32(35, 50, 70, 255);
            bgColorHover = IM_COL32(45, 65, 90, 255);
            break;
        case AssetType::Audio:
            bgColor = IM_COL32(60, 55, 30, 255);
            bgColorHover = IM_COL32(80, 75, 40, 255);
            break;
        default:
            bgColor = IM_COL32(40, 40, 48, 255);
            bgColorHover = IM_COL32(55, 55, 65, 255);
            break;
    }

    // Invisible button for interaction
    bool clicked = ImGui::InvisibleButton("##card", cardSize);
    bool hovered = ImGui::IsItemHovered();
    bool doubleClicked = hovered && ImGui::IsMouseDoubleClicked(0);

    // Draw rounded card background
    float rounding = 6.0f;
    ImU32 bg = hovered ? bgColorHover : bgColor;
    drawList->AddRectFilled(cursorPos,
        ImVec2(cursorPos.x + cardSize.x, cursorPos.y + cardSize.y),
        bg, rounding);

    // Subtle border
    ImU32 borderColor = hovered ? IM_COL32(120, 140, 180, 200) : IM_COL32(70, 70, 85, 150);
    drawList->AddRect(cursorPos,
        ImVec2(cursorPos.x + cardSize.x, cursorPos.y + cardSize.y),
        borderColor, rounding, 0, 1.0f);

    // Content area (with padding)
    float pad = 4.0f;
    ImVec2 contentPos(cursorPos.x + pad, cursorPos.y + pad);
    ImVec2 contentSize(cardSize.x - pad * 2, cardSize.y - pad * 2);

    // Draw content based on type
    bool hasImageThumbnail = false;

    if (asset.type == AssetType::Texture || asset.type == AssetType::HDRI || asset.type == AssetType::Model) {
        // Try to show actual image thumbnail
        ThumbnailEntry* thumb = GetOrLoadThumbnail(asset);
        if (thumb && thumb->loaded && thumb->imguiDescriptor != VK_NULL_HANDLE) {
            // Draw the actual image thumbnail
            drawList->AddImage(
                (ImTextureID)thumb->imguiDescriptor,
                ImVec2(contentPos.x, contentPos.y),
                ImVec2(contentPos.x + contentSize.x, contentPos.y + contentSize.y),
                ImVec2(0, 0), ImVec2(1, 1),
                IM_COL32(255, 255, 255, 255)
            );
            hasImageThumbnail = true;
        }
    }

    if (!hasImageThumbnail) {
        // Draw vector icon based on type
        switch (asset.type) {
            case AssetType::Folder:
                RenderFolderIcon(drawList, contentPos, contentSize);
                break;
            case AssetType::Model:
                RenderModelIcon(drawList, contentPos, contentSize);
                break;
            case AssetType::Shader:
                RenderShaderIcon(drawList, contentPos, contentSize);
                break;
            case AssetType::Audio:
                RenderAudioIcon(drawList, contentPos, contentSize);
                break;
            case AssetType::Scene:
                RenderSceneIcon(drawList, contentPos, contentSize);
                break;
            default: {
                std::string ext = fs::path(asset.name).extension().string();
                RenderFileIcon(drawList, contentPos, contentSize, ext.c_str());
                break;
            }
        }
    }

    // Type badge (small colored tag in corner)
    if (!asset.isDirectory) {
        std::string ext = fs::path(asset.name).extension().string();
        if (!ext.empty()) {
            // Remove the dot
            std::string extLabel = ext.substr(1);
            std::transform(extLabel.begin(), extLabel.end(), extLabel.begin(), ::toupper);

            ImVec2 textSize = ImGui::CalcTextSize(extLabel.c_str());
            float badgeW = textSize.x + 6;
            float badgeH = textSize.y + 2;
            ImVec2 badgePos(cursorPos.x + cardSize.x - badgeW - 3, cursorPos.y + 3);

            drawList->AddRectFilled(badgePos,
                ImVec2(badgePos.x + badgeW, badgePos.y + badgeH),
                IM_COL32(0, 0, 0, 180), 3.0f);
            drawList->AddText(ImVec2(badgePos.x + 3, badgePos.y + 1),
                IM_COL32(200, 200, 200, 255), extLabel.c_str());
        }
    }

    // Handle click
    if (clicked) {
        if (asset.isDirectory) {
            m_PendingNavigatePath = fs::absolute(asset.path).string();
        }
    }

    // Handle double-click
    if (doubleClicked && !asset.isDirectory) {
        HandleAssetClick(asset);
    }

    // Drag source for textures
    if ((asset.type == AssetType::Texture || asset.type == AssetType::HDRI) && ImGui::BeginDragDropSource()) {
        ImGui::SetDragDropPayload("TEXTURE_PATH", asset.path.c_str(), asset.path.size() + 1);
        ImGui::Text("Texture: %s", asset.name.c_str());
        ImGui::EndDragDropSource();
    }

    // Context menu
    if (ImGui::BeginPopupContextItem()) {
        RenderAssetContextMenu(asset);
        ImGui::EndPopup();
    }

    // Tooltip
    if (hovered) {
        ImGui::BeginTooltip();
        ImGui::Text("%s", asset.name.c_str());
        ImGui::TextDisabled("Type: %s", GetAssetTypeName(asset.type));
        if (asset.fileSize > 0) {
            ImGui::TextDisabled("Size: %s", FormatFileSize(asset.fileSize).c_str());
        }
        ImGui::TextDisabled("Path: %s", asset.path.c_str());
        ImGui::EndTooltip();
    }

    // Name label below card
    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + size);
    std::string displayName = asset.name;
    if (displayName.length() > 16) {
        displayName = displayName.substr(0, 13) + "...";
    }
    ImGui::TextWrapped("%s", displayName.c_str());
    ImGui::PopTextWrapPos();
}

// =============================================================================
// VECTOR ICONS - Custom drawn icons for each asset type
// =============================================================================

void AssetBrowserView::RenderFolderIcon(ImDrawList* drawList, ImVec2 pos, ImVec2 size) {
    float cx = pos.x + size.x * 0.5f;
    float cy = pos.y + size.y * 0.5f;
    float w = size.x * 0.7f;
    float h = size.y * 0.5f;

    // Folder tab
    float tabW = w * 0.4f;
    float tabH = h * 0.2f;
    drawList->AddRectFilled(
        ImVec2(cx - w * 0.5f, cy - h * 0.5f - tabH),
        ImVec2(cx - w * 0.5f + tabW, cy - h * 0.5f),
        IM_COL32(180, 160, 80, 255), 3.0f);

    // Folder body
    drawList->AddRectFilled(
        ImVec2(cx - w * 0.5f, cy - h * 0.5f),
        ImVec2(cx + w * 0.5f, cy + h * 0.5f),
        IM_COL32(200, 180, 90, 255), 4.0f);

    // Folder fold line
    drawList->AddLine(
        ImVec2(cx - w * 0.5f + tabW, cy - h * 0.5f),
        ImVec2(cx - w * 0.5f + tabW + tabH, cy - h * 0.5f + tabH * 0.5f),
        IM_COL32(170, 150, 70, 255), 1.5f);
}

void AssetBrowserView::RenderModelIcon(ImDrawList* drawList, ImVec2 pos, ImVec2 size) {
    float cx = pos.x + size.x * 0.5f;
    float cy = pos.y + size.y * 0.5f;
    float s = std::min(size.x, size.y) * 0.35f;

    // 3D cube wireframe
    ImU32 col = IM_COL32(100, 200, 140, 255);
    ImU32 colDark = IM_COL32(60, 140, 90, 200);

    // Front face
    float ox = s * 0.3f;
    float oy = s * 0.2f;

    ImVec2 fl(cx - s, cy);
    ImVec2 fr(cx, cy + oy);
    ImVec2 ft(cx, cy - s + oy);
    ImVec2 ftl(cx - s, cy - s);

    ImVec2 br(cx + s, cy);
    ImVec2 bt(cx + s, cy - s);

    // Left face (darker)
    drawList->AddQuadFilled(fl, ImVec2(cx, cy + oy), ImVec2(cx, cy - s + oy), ftl, colDark);
    // Right face
    drawList->AddQuadFilled(fr, br, bt, ft, IM_COL32(80, 170, 110, 200));
    // Top face
    drawList->AddQuadFilled(ftl, ft, bt, ImVec2(cx - s + s, cy - s - oy + oy), IM_COL32(120, 220, 160, 200));

    // Edges
    drawList->AddLine(fl, ftl, col, 1.5f);
    drawList->AddLine(ftl, ft, col, 1.5f);
    drawList->AddLine(ft, fr, col, 1.5f);
    drawList->AddLine(fl, fr, col, 1.5f);
    drawList->AddLine(fr, br, col, 1.5f);
    drawList->AddLine(ft, bt, col, 1.5f);
    drawList->AddLine(br, bt, col, 1.5f);
}

void AssetBrowserView::RenderShaderIcon(ImDrawList* drawList, ImVec2 pos, ImVec2 size) {
    float cx = pos.x + size.x * 0.5f;
    float cy = pos.y + size.y * 0.5f;
    float s = std::min(size.x, size.y) * 0.3f;

    ImU32 col = IM_COL32(180, 120, 220, 255);

    // Left angle bracket <
    drawList->AddLine(ImVec2(cx - s * 0.3f, cy - s * 0.6f), ImVec2(cx - s, cy), col, 2.5f);
    drawList->AddLine(ImVec2(cx - s, cy), ImVec2(cx - s * 0.3f, cy + s * 0.6f), col, 2.5f);

    // Right angle bracket >
    drawList->AddLine(ImVec2(cx + s * 0.3f, cy - s * 0.6f), ImVec2(cx + s, cy), col, 2.5f);
    drawList->AddLine(ImVec2(cx + s, cy), ImVec2(cx + s * 0.3f, cy + s * 0.6f), col, 2.5f);

    // Slash /
    drawList->AddLine(ImVec2(cx + s * 0.15f, cy - s * 0.8f), ImVec2(cx - s * 0.15f, cy + s * 0.8f), IM_COL32(220, 160, 255, 200), 2.0f);
}

void AssetBrowserView::RenderAudioIcon(ImDrawList* drawList, ImVec2 pos, ImVec2 size) {
    float cx = pos.x + size.x * 0.5f;
    float cy = pos.y + size.y * 0.5f;
    float s = std::min(size.x, size.y) * 0.3f;

    ImU32 col = IM_COL32(220, 200, 80, 255);

    // Speaker shape
    drawList->AddRectFilled(
        ImVec2(cx - s * 0.8f, cy - s * 0.3f),
        ImVec2(cx - s * 0.3f, cy + s * 0.3f),
        col, 2.0f);

    // Speaker cone
    drawList->AddTriangleFilled(
        ImVec2(cx - s * 0.3f, cy - s * 0.3f),
        ImVec2(cx - s * 0.3f, cy + s * 0.3f),
        ImVec2(cx + s * 0.2f, cy + s * 0.7f),
        col);
    drawList->AddTriangleFilled(
        ImVec2(cx - s * 0.3f, cy - s * 0.3f),
        ImVec2(cx + s * 0.2f, cy - s * 0.7f),
        ImVec2(cx + s * 0.2f, cy + s * 0.7f),
        col);

    // Sound waves
    for (int i = 1; i <= 3; i++) {
        float r = s * 0.3f * i;
        float alpha = 255 - i * 60;
        ImU32 waveCol = IM_COL32(220, 200, 80, (int)alpha);
        drawList->PathArcTo(ImVec2(cx + s * 0.2f, cy), r, -0.6f, 0.6f, 12);
        drawList->PathStroke(waveCol, 0, 1.5f);
    }
}

void AssetBrowserView::RenderSceneIcon(ImDrawList* drawList, ImVec2 pos, ImVec2 size) {
    float cx = pos.x + size.x * 0.5f;
    float cy = pos.y + size.y * 0.5f;
    float s = std::min(size.x, size.y) * 0.35f;

    // Film clapperboard
    ImU32 col = IM_COL32(100, 160, 220, 255);
    ImU32 colDark = IM_COL32(60, 100, 150, 255);

    // Board body
    drawList->AddRectFilled(
        ImVec2(cx - s, cy - s * 0.4f),
        ImVec2(cx + s, cy + s * 0.8f),
        col, 4.0f);

    // Clapper top
    drawList->AddRectFilled(
        ImVec2(cx - s, cy - s * 0.8f),
        ImVec2(cx + s, cy - s * 0.4f),
        colDark, 4.0f);

    // Diagonal stripes on clapper
    for (float x = cx - s + s * 0.3f; x < cx + s; x += s * 0.4f) {
        drawList->AddLine(
            ImVec2(x, cy - s * 0.8f),
            ImVec2(x - s * 0.25f, cy - s * 0.4f),
            IM_COL32(40, 70, 110, 255), 2.0f);
    }

    // Play triangle
    float triS = s * 0.25f;
    drawList->AddTriangleFilled(
        ImVec2(cx - triS * 0.5f, cy + triS * 0.6f - triS),
        ImVec2(cx - triS * 0.5f, cy + triS * 0.6f + triS),
        ImVec2(cx + triS, cy + triS * 0.6f),
        IM_COL32(255, 255, 255, 200));
}

void AssetBrowserView::RenderFileIcon(ImDrawList* drawList, ImVec2 pos, ImVec2 size, const char* ext) {
    float cx = pos.x + size.x * 0.5f;
    float cy = pos.y + size.y * 0.5f;
    float w = size.x * 0.5f;
    float h = size.y * 0.6f;

    // File shape (rectangle with folded corner)
    float foldSize = w * 0.35f;
    ImVec2 p1(cx - w * 0.5f, cy - h * 0.5f);
    ImVec2 p2(cx + w * 0.5f - foldSize, cy - h * 0.5f);
    ImVec2 p3(cx + w * 0.5f, cy - h * 0.5f + foldSize);
    ImVec2 p4(cx + w * 0.5f, cy + h * 0.5f);
    ImVec2 p5(cx - w * 0.5f, cy + h * 0.5f);

    // File body
    drawList->AddQuadFilled(p1, p2, ImVec2(p2.x, p3.y), p5, IM_COL32(160, 160, 170, 255));
    drawList->AddTriangleFilled(p2, p3, ImVec2(p2.x, p3.y), IM_COL32(160, 160, 170, 255));
    drawList->AddRectFilled(ImVec2(p2.x, p3.y), p4, IM_COL32(160, 160, 170, 255));

    // Fold
    drawList->AddTriangleFilled(p2, p3, ImVec2(p2.x, p3.y), IM_COL32(120, 120, 130, 255));

    // Extension text centered
    if (ext && ext[0] != '\0') {
        std::string label = ext;
        if (label[0] == '.') label = label.substr(1);
        std::transform(label.begin(), label.end(), label.begin(), ::toupper);
        ImVec2 textSize = ImGui::CalcTextSize(label.c_str());
        drawList->AddText(
            ImVec2(cx - textSize.x * 0.5f, cy + h * 0.1f - textSize.y * 0.5f),
            IM_COL32(60, 60, 70, 255), label.c_str());
    }
}

// =============================================================================
// THUMBNAIL LOADING
// =============================================================================

ThumbnailEntry* AssetBrowserView::GetOrLoadThumbnail(const AssetEntry& asset) {
    if (!m_Engine) return nullptr;

    // Check cache
    auto it = m_ThumbnailCache.find(asset.path);
    if (it != m_ThumbnailCache.end()) {
        return &it->second;
    }

    // Try to load thumbnail
    ThumbnailEntry entry;
    entry.loaded = false;
    entry.failed = false;

    // Load thumbnails for textures/hdr and model files
    if (asset.type != AssetType::Texture && asset.type != AssetType::HDRI && asset.type != AssetType::Model) {
        entry.failed = true;
        m_ThumbnailCache[asset.path] = entry;
        return &m_ThumbnailCache[asset.path];
    }

    int width = 0, height = 0;
    std::vector<uint8_t> thumbData;
    int thumbW = THUMB_RES;
    int thumbH = THUMB_RES;

    if (asset.type == AssetType::Model) {
        if (!generateModelPreviewRGBA(asset.path, THUMB_RES, thumbData, thumbW, thumbH)) {
            entry.failed = true;
            m_ThumbnailCache[asset.path] = entry;
            return &m_ThumbnailCache[asset.path];
        }
    } else if (asset.type == AssetType::HDRI) {
        // HDR/EXR path: load float data and tonemap to LDR thumbnail
        int channels = 0;
        float* dataf = stbi_loadf(asset.path.c_str(), &width, &height, &channels, 4);
        if (!dataf) {
            entry.failed = true;
            m_ThumbnailCache[asset.path] = entry;
            return &m_ThumbnailCache[asset.path];
        }

        float aspect = (float)width / (float)height;
        if (aspect > 1.0f) {
            thumbH = std::max(1, (int)(THUMB_RES / aspect));
        } else {
            thumbW = std::max(1, (int)(THUMB_RES * aspect));
        }

        thumbData.resize(thumbW * thumbH * 4);

        float xStep = (float)width / thumbW;
        float yStep = (float)height / thumbH;

        auto toByte = [](float v) -> uint8_t {
            v = std::max(0.0f, v);
            // Simple Reinhard tonemap + gamma
            float mapped = v / (1.0f + v);
            float srgb = std::pow(mapped, 1.0f / 2.2f);
            int iv = static_cast<int>(srgb * 255.0f + 0.5f);
            iv = std::clamp(iv, 0, 255);
            return static_cast<uint8_t>(iv);
        };

        for (int y = 0; y < thumbH; y++) {
            for (int x = 0; x < thumbW; x++) {
                int srcX = std::min((int)(x * xStep), width - 1);
                int srcY = std::min((int)(y * yStep), height - 1);
                int srcIdx = (srcY * width + srcX) * 4;
                int dstIdx = (y * thumbW + x) * 4;

                float r = std::isfinite(dataf[srcIdx + 0]) ? dataf[srcIdx + 0] : 0.0f;
                float g = std::isfinite(dataf[srcIdx + 1]) ? dataf[srcIdx + 1] : 0.0f;
                float b = std::isfinite(dataf[srcIdx + 2]) ? dataf[srcIdx + 2] : 0.0f;
                float a = std::isfinite(dataf[srcIdx + 3]) ? dataf[srcIdx + 3] : 1.0f;

                thumbData[dstIdx + 0] = toByte(r);
                thumbData[dstIdx + 1] = toByte(g);
                thumbData[dstIdx + 2] = toByte(b);
                thumbData[dstIdx + 3] = static_cast<uint8_t>(std::clamp(a, 0.0f, 1.0f) * 255.0f);
            }
        }

        stbi_image_free(dataf);
    } else {
        // Load image with stb_image
        int channels = 0;
        unsigned char* data = stbi_load(asset.path.c_str(), &width, &height, &channels, 4);
        if (!data) {
            entry.failed = true;
            m_ThumbnailCache[asset.path] = entry;
            return &m_ThumbnailCache[asset.path];
        }

        // Maintain aspect ratio
        float aspect = (float)width / (float)height;
        if (aspect > 1.0f) {
            thumbH = std::max(1, (int)(THUMB_RES / aspect));
        } else {
            thumbW = std::max(1, (int)(THUMB_RES * aspect));
        }

        thumbData.resize(thumbW * thumbH * 4);

        float xStep = (float)width / thumbW;
        float yStep = (float)height / thumbH;

        for (int y = 0; y < thumbH; y++) {
            for (int x = 0; x < thumbW; x++) {
                int srcX = std::min((int)(x * xStep), width - 1);
                int srcY = std::min((int)(y * yStep), height - 1);
                int srcIdx = (srcY * width + srcX) * 4;
                int dstIdx = (y * thumbW + x) * 4;
                thumbData[dstIdx + 0] = data[srcIdx + 0];
                thumbData[dstIdx + 1] = data[srcIdx + 1];
                thumbData[dstIdx + 2] = data[srcIdx + 2];
                thumbData[dstIdx + 3] = data[srcIdx + 3];
            }
        }
        stbi_image_free(data);
    }

    // Upload to GPU
    VkExtent3D imageSize{};
    imageSize.width = static_cast<uint32_t>(thumbW);
    imageSize.height = static_cast<uint32_t>(thumbH);
    imageSize.depth = 1;

    AllocatedImage thumbImage = m_Engine->create_image(
        thumbData.data(), imageSize,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_USAGE_SAMPLED_BIT,
        false
    );

    if (thumbImage.image == VK_NULL_HANDLE) {
        entry.failed = true;
        m_ThumbnailCache[asset.path] = entry;
        return &m_ThumbnailCache[asset.path];
    }

    // Register with ImGui
    VkDescriptorSet ds = ImGui_ImplVulkan_AddTexture(
        m_Engine->_defaultSamplerLinear,
        thumbImage.imageView,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );

    if (ds == VK_NULL_HANDLE) {
        // Cleanup
        m_Engine->destroy_image(thumbImage);
        entry.failed = true;
        m_ThumbnailCache[asset.path] = entry;
        return &m_ThumbnailCache[asset.path];
    }

    entry.imguiDescriptor = ds;
    entry.gpuImage = thumbImage;
    entry.loaded = true;
    entry.failed = false;

    m_ThumbnailCache[asset.path] = entry;
    return &m_ThumbnailCache[asset.path];
}

void AssetBrowserView::ClearThumbnailCache() {
    if (!m_Engine) {
        m_ThumbnailCache.clear();
        return;
    }

    // ImGui texture descriptors are allocated from descriptor pools that can still
    // be referenced by submitted command buffers. Wait for GPU idle before freeing.
    vkDeviceWaitIdle(m_Engine->_device);

    for (auto& [path, entry] : m_ThumbnailCache) {
        if (entry.imguiDescriptor != VK_NULL_HANDLE) {
            ImGui_ImplVulkan_RemoveTexture(entry.imguiDescriptor);
            entry.imguiDescriptor = VK_NULL_HANDLE;
        }
        if (entry.gpuImage.image != VK_NULL_HANDLE) {
            m_Engine->destroy_image(entry.gpuImage);
            entry.gpuImage = {};
        }
    }
    m_ThumbnailCache.clear();
}

std::string AssetBrowserView::FormatFileSize(size_t bytes) {
    if (bytes < 1024) return std::to_string(bytes) + " B";
    if (bytes < 1024 * 1024) return std::to_string(bytes / 1024) + " KB";
    if (bytes < 1024 * 1024 * 1024) return std::to_string(bytes / (1024 * 1024)) + " MB";
    return std::to_string(bytes / (1024 * 1024 * 1024)) + " GB";
}

// =============================================================================
// ASSET HANDLING
// =============================================================================

void AssetBrowserView::HandleAssetClick(const AssetEntry& asset) {
    if (asset.isDirectory) {
        return;
    }

    if (!m_Engine) return;

    std::string ext = fs::path(asset.name).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == ".gltf" || ext == ".glb" || ext == ".obj" || ext == ".fbx" || ext == ".dae" || ext == ".mtl") {
        auto result = loadSceneAsset(m_Engine, asset.path);
        if (result.has_value()) {
            std::string sceneName = fs::path(asset.name).stem().string();
            int counter = 1;
            std::string baseName = sceneName;
            while (m_Engine->loadedScenes.find(sceneName) != m_Engine->loadedScenes.end()) {
                sceneName = baseName + "_" + std::to_string(counter++);
            }
            m_Engine->loadedScenes[sceneName] = result.value();
            m_Engine->sceneFilePaths[sceneName] = asset.path;
            m_SelectedScene = sceneName;
            m_ShowLoadedScenes = true;

            // Auto-focus camera on first top node so imported scenes are immediately visible.
            auto loadedIt = m_Engine->loadedScenes.find(sceneName);
            if (loadedIt != m_Engine->loadedScenes.end() && loadedIt->second && !loadedIt->second->topNodes.empty()) {
                for (auto& top : loadedIt->second->topNodes) {
                    if (!top) continue;
                    glm::vec3 pos = glm::vec3(top->worldTransform[3]);
                    if (std::isfinite(pos.x) && std::isfinite(pos.y) && std::isfinite(pos.z)) {
                        m_Engine->mainCamera.focusOnPoint(pos, 12.0f);
                        break;
                    }
                }
            }
        }
    } else if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga" || ext == ".hdr" || ext == ".bmp") {
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

        if (ext == ".gltf" || ext == ".glb" || ext == ".obj" || ext == ".fbx" || ext == ".dae" || ext == ".mtl") {
            if (ImGui::MenuItem("Load Scene")) {
                HandleAssetClick(asset);
            }
        }

        if (asset.type == AssetType::Texture || asset.type == AssetType::HDRI) {
            if (ImGui::MenuItem("Set as Albedo")) {
                m_LastSelectedTexture = asset.path;
            }
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Show in Explorer")) {
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

    // Clear thumbnail cache when changing directories
    if (m_CachedDirectory != m_CurrentPath) {
        ClearThumbnailCache();
        m_CachedDirectory = m_CurrentPath;
    }

    try {
        m_CurrentPath = fs::weakly_canonical(fs::path(m_CurrentPath)).string();
    } catch (...) {
        m_CurrentPath = fs::absolute(fs::path(m_CurrentPath)).string();
    }

    if (!fs::exists(m_CurrentPath) || !fs::is_directory(m_CurrentPath)) return;

    try {
        for (const auto& entry : fs::directory_iterator(m_CurrentPath, fs::directory_options::skip_permission_denied)) {
            AssetEntry asset;
            try {
                asset.name = entry.path().filename().string();
                asset.path = entry.path().string();
                asset.isDirectory = entry.is_directory();
            } catch (...) {
                continue;
            }

            if (asset.isDirectory) {
                asset.type = AssetType::Folder;
            } else {
                std::string ext = entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                asset.type = GetAssetType(ext);

                try {
                    asset.fileSize = static_cast<size_t>(entry.file_size());
                } catch (...) {
                    asset.fileSize = 0;
                }
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
        // Handle permission errors
    }
}

AssetType AssetBrowserView::GetAssetType(const std::string& ext) {
    if (ext == ".gltf" || ext == ".glb" || ext == ".obj" || ext == ".fbx" || ext == ".dae") {
        return AssetType::Model;
    }
    if (ext == ".hdr" || ext == ".exr") {
        return AssetType::HDRI;
    }
    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga" || ext == ".bmp") {
        return AssetType::Texture;
    }
    if (ext == ".glsl" || ext == ".vert" || ext == ".frag" || ext == ".comp" || ext == ".spv" || ext == ".hlsl") {
        return AssetType::Shader;
    }
    if (ext == ".scene" || ext == ".json") {
        return AssetType::Scene;
    }
    if (ext == ".mtl" || ext == ".mat") {
        return AssetType::Material;
    }
    if (ext == ".wav" || ext == ".mp3" || ext == ".ogg" || ext == ".flac") {
        return AssetType::Audio;
    }
    return AssetType::Unknown;
}

const char* AssetBrowserView::GetAssetTypeName(AssetType type) {
    switch (type) {
        case AssetType::Folder:   return "Folder";
        case AssetType::Model:    return "3D Model";
        case AssetType::Texture:  return "Texture";
        case AssetType::HDRI:     return "HDR Image";
        case AssetType::Shader:   return "Shader";
        case AssetType::Scene:    return "Scene";
        case AssetType::Material: return "Material";
        case AssetType::Audio:    return "Audio";
        default:                  return "Unknown";
    }
}

} // namespace Yalaz::UI
