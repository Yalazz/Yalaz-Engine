// =============================================================================
// YALAZ ENGINE - Texture View Implementation
// =============================================================================
// Connected to engine - shows textures from loaded GLTF scenes
// =============================================================================

#include "TextureView.h"
#include "../../vk_engine.h"

namespace Yalaz::UI {

void TextureView::OnRender() {
    if (!BeginView(ImGuiWindowFlags_MenuBar)) {
        EndView();
        return;
    }

    if (ImGui::BeginMenuBar()) {
        // Channel selector
        const char* channels[] = { "RGBA", "R", "G", "B", "A" };
        ImGui::SetNextItemWidth(80);
        ImGui::Combo("##Channel", &m_ChannelMode, channels, 5);

        ImGui::SameLine();
        ImGui::Checkbox("Checkerboard", &m_ShowCheckerboard);

        ImGui::SameLine();
        ImGui::SetNextItemWidth(100);
        ImGui::SliderFloat("Zoom", &m_Zoom, 0.1f, 8.0f, "%.1fx");

        ImGui::EndMenuBar();
    }

    if (!m_Engine) {
        ImGui::TextDisabled("No engine context");
        EndView();
        return;
    }

    // Split: texture list | preview + info
    ImGui::Columns(2, nullptr, true);
    ImGui::SetColumnWidth(0, 200);

    RenderTextureList();

    ImGui::NextColumn();

    // Preview and metadata
    ImGui::BeginChild("TextureDetails");
    RenderPreview();
    ImGui::Spacing();
    RenderMetadata();
    ImGui::EndChild();

    ImGui::Columns(1);

    EndView();
}

void TextureView::RenderTextureList() {
    SectionHeader("Scene Textures");

    // Count textures
    size_t totalTextures = 0;
    for (const auto& [name, scene] : m_Engine->loadedScenes) {
        if (scene) totalTextures += scene->images.size();
    }

    if (totalTextures == 0) {
        ImGui::TextDisabled("No textures loaded");
        ImGui::TextWrapped("Load a GLTF/GLB scene to see its textures.");
        return;
    }

    ImGui::Text("Total: %zu textures", totalTextures);
    ImGui::Separator();

    ImGui::BeginChild("TextureListScroll", ImVec2(0, 0), false);

    int textureIndex = 0;
    for (const auto& [sceneName, scene] : m_Engine->loadedScenes) {
        if (!scene) continue;

        if (ImGui::TreeNode(sceneName.c_str())) {
            for (const auto& [imageName, image] : scene->images) {
                ImGui::PushID(textureIndex);

                bool isSelected = (m_SelectedScene == sceneName && m_SelectedTexture == imageName);

                // Texture icon based on format
                ImGui::TextColored(ImVec4(0.5f, 0.8f, 0.5f, 1.0f), "[TEX]");
                ImGui::SameLine();

                if (ImGui::Selectable(imageName.c_str(), isSelected)) {
                    m_SelectedScene = sceneName;
                    m_SelectedTexture = imageName;
                }

                ImGui::PopID();
                textureIndex++;
            }
            ImGui::TreePop();
        }
    }

    ImGui::EndChild();
}

void TextureView::RenderPreview() {
    SectionHeader("Preview");

    ImVec2 size = ImGui::GetContentRegionAvail();
    float previewHeight = std::min(size.y * 0.5f, 300.0f);

    ImGui::BeginChild("TexturePreview", ImVec2(0, previewHeight), true);

    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 previewSize = ImGui::GetContentRegionAvail();
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // Checkerboard background
    if (m_ShowCheckerboard) {
        int gridSize = 16;
        for (int y = 0; y < (int)previewSize.y; y += gridSize) {
            for (int x = 0; x < (int)previewSize.x; x += gridSize) {
                ImU32 col = ((x / gridSize + y / gridSize) % 2) ?
                    IM_COL32(60, 60, 60, 255) : IM_COL32(40, 40, 40, 255);
                drawList->AddRectFilled(
                    ImVec2(pos.x + x, pos.y + y),
                    ImVec2(pos.x + std::min((float)(x + gridSize), previewSize.x),
                           pos.y + std::min((float)(y + gridSize), previewSize.y)),
                    col);
            }
        }
    }

    // Check if we have a selected texture
    if (!m_SelectedScene.empty() && !m_SelectedTexture.empty()) {
        auto sceneIt = m_Engine->loadedScenes.find(m_SelectedScene);
        if (sceneIt != m_Engine->loadedScenes.end() && sceneIt->second) {
            auto& images = sceneIt->second->images;
            auto imageIt = images.find(m_SelectedTexture);
            if (imageIt != images.end()) {
                // Draw a placeholder representation of the texture
                // In a real implementation, we'd render the VkImageView to an ImGui texture
                float texSize = std::min(previewSize.x, previewSize.y) * 0.8f * m_Zoom;
                texSize = std::min(texSize, previewSize.x - 20);
                texSize = std::min(texSize, previewSize.y - 20);

                float offsetX = (previewSize.x - texSize) * 0.5f;
                float offsetY = (previewSize.y - texSize) * 0.5f;

                ImVec2 texMin(pos.x + offsetX, pos.y + offsetY);
                ImVec2 texMax(texMin.x + texSize, texMin.y + texSize);

                // Gradient to represent texture data
                drawList->AddRectFilledMultiColor(
                    texMin, texMax,
                    IM_COL32(100, 120, 140, 255),
                    IM_COL32(140, 100, 120, 255),
                    IM_COL32(120, 140, 100, 255),
                    IM_COL32(130, 130, 130, 255));

                // Border
                drawList->AddRect(texMin, texMax, IM_COL32(200, 200, 200, 255));

                // Label
                ImVec2 textPos(texMin.x + 5, texMin.y + 5);
                drawList->AddText(textPos, IM_COL32(255, 255, 255, 255), m_SelectedTexture.c_str());
            }
        }
    } else {
        // No selection
        ImGui::SetCursorPos(ImVec2(previewSize.x / 2 - 60, previewSize.y / 2 - 10));
        ImGui::TextDisabled("Select a texture");
    }

    ImGui::EndChild();

    // Mip level selector
    ImGui::Text("Mip Level:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100);
    ImGui::SliderInt("##Mip", &m_MipLevel, 0, 10);
}

void TextureView::RenderMetadata() {
    SectionHeader("Metadata");

    if (m_SelectedScene.empty() || m_SelectedTexture.empty()) {
        ImGui::TextDisabled("No texture selected");
        return;
    }

    auto sceneIt = m_Engine->loadedScenes.find(m_SelectedScene);
    if (sceneIt == m_Engine->loadedScenes.end() || !sceneIt->second) {
        ImGui::TextDisabled("Scene not found");
        return;
    }

    auto& images = sceneIt->second->images;
    auto imageIt = images.find(m_SelectedTexture);
    if (imageIt == images.end()) {
        ImGui::TextDisabled("Texture not found");
        return;
    }

    const auto& image = imageIt->second;

    ImGui::Text("Name: %s", m_SelectedTexture.c_str());
    ImGui::Text("Scene: %s", m_SelectedScene.c_str());
    ImGui::Text("Size: %d x %d", image.imageExtent.width, image.imageExtent.height);

    // Format info
    const char* formatStr = "Unknown";
    switch (image.imageFormat) {
        case VK_FORMAT_R8G8B8A8_SRGB: formatStr = "RGBA8 sRGB"; break;
        case VK_FORMAT_R8G8B8A8_UNORM: formatStr = "RGBA8 UNORM"; break;
        case VK_FORMAT_R8G8B8_SRGB: formatStr = "RGB8 sRGB"; break;
        case VK_FORMAT_R8G8B8_UNORM: formatStr = "RGB8 UNORM"; break;
        case VK_FORMAT_R8_UNORM: formatStr = "R8 UNORM"; break;
        case VK_FORMAT_R16G16B16A16_SFLOAT: formatStr = "RGBA16F"; break;
        case VK_FORMAT_R32G32B32A32_SFLOAT: formatStr = "RGBA32F"; break;
        default: formatStr = "Other"; break;
    }
    ImGui::Text("Format: %s", formatStr);

    // Mip levels (estimated from size)
    int estimatedMips = static_cast<int>(std::floor(std::log2(std::max(image.imageExtent.width, image.imageExtent.height)))) + 1;
    ImGui::Text("Mip Levels: ~%d", estimatedMips);

    // Estimate memory
    uint32_t bpp = 4;  // Assume 4 bytes per pixel
    uint64_t memSize = image.imageExtent.width * image.imageExtent.height * bpp;
    if (memSize > 1024 * 1024) {
        ImGui::Text("Memory: %.2f MB", memSize / (1024.0f * 1024.0f));
    } else {
        ImGui::Text("Memory: %.2f KB", memSize / 1024.0f);
    }

    ImGui::Spacing();

    // Additional info
    bool isSRGB = (image.imageFormat == VK_FORMAT_R8G8B8A8_SRGB ||
                   image.imageFormat == VK_FORMAT_R8G8B8_SRGB);
    ImGui::TextDisabled("sRGB: %s", isSRGB ? "Yes" : "No");
    ImGui::TextDisabled("Compressed: No");
}

void TextureView::RenderChannels() {
    SectionHeader("Channels");

    // Channel histograms placeholder
    ImGui::TextDisabled("Histogram (placeholder)");

    float empty[64] = {0};
    for (int i = 0; i < 64; ++i) {
        empty[i] = (float)(rand() % 100) / 100.0f * 0.5f + 0.1f;
    }

    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(1, 0, 0, 1));
    ImGui::PlotHistogram("##R", empty, 64, 0, nullptr, 0, 1, ImVec2(-1, 40));
    ImGui::PopStyleColor();

    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0, 1, 0, 1));
    ImGui::PlotHistogram("##G", empty, 64, 0, nullptr, 0, 1, ImVec2(-1, 40));
    ImGui::PopStyleColor();

    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0, 0, 1, 1));
    ImGui::PlotHistogram("##B", empty, 64, 0, nullptr, 0, 1, ImVec2(-1, 40));
    ImGui::PopStyleColor();
}

} // namespace Yalaz::UI
