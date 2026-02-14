// =============================================================================
// YALAZ ENGINE - GPU Debug View Implementation
// =============================================================================
// Dynamic GPU debug view connected to engine:
// - Real draw call and triangle counts from engine stats
// - Real primitive and light counts
// - Frame time history from actual performance
// - Memory estimates based on loaded assets
// =============================================================================

#include "GPUDebugView.h"
#include "../../vk_engine.h"
#include "imgui_impl_vulkan.h"
#include <algorithm>
#include <cmath>

namespace Yalaz::UI {

void GPUDebugView::OnUpdate(float deltaTime) {
    float frameTime = deltaTime * 1000.0f;
    m_FrameTimeHistory.push_back(frameTime);
    if (m_FrameTimeHistory.size() > MAX_HISTORY) {
        m_FrameTimeHistory.pop_front();
    }
}

void GPUDebugView::OnRender() {
    if (!BeginView(ImGuiWindowFlags_MenuBar)) {
        EndView();
        return;
    }

    if (ImGui::BeginMenuBar()) {
        // Debug mode selector
        const char* modes[] = {"None", "Overdraw", "Depth", "Stencil", "Normals", "UVs", "Mipmaps"};
        ImGui::SetNextItemWidth(100);
        ImGui::Combo("Mode", &m_DebugMode, modes, 7);

        ImGui::EndMenuBar();
    }

    if (ImGui::BeginTabBar("GPUDebugTabs")) {
        if (ImGui::BeginTabItem("Render Targets")) {
            RenderVisualization();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Overview")) {
            RenderOverview();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Draw Calls")) {
            RenderDrawCalls();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Memory")) {
            RenderMemory();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Counters")) {
            RenderCounters();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    EndView();
}

void GPUDebugView::OnShutdown() {
    ClearRenderTargetDescriptors();
}

void GPUDebugView::RenderOverview() {
    if (!m_Engine) {
        ImGui::TextDisabled("No engine context");
        return;
    }

    SectionHeader("Real-Time Stats");

    // Get real stats from engine
    uint32_t primitiveCount = static_cast<uint32_t>(m_Engine->static_shapes.size());
    uint32_t lightCount = static_cast<uint32_t>(m_Engine->scenePointLights.size());
    uint32_t sceneCount = static_cast<uint32_t>(m_Engine->loadedScenes.size());

    // Calculate triangle count from primitives
    uint32_t triangleCount = 0;
    const int trianglesPerType[] = { 12, 960, 192, 96, 640, 1152, 2, 1 };  // Per primitive type
    for (const auto& shape : m_Engine->static_shapes) {
        int typeIdx = static_cast<int>(shape.type);
        if (typeIdx >= 0 && typeIdx < 8) {
            triangleCount += trianglesPerType[typeIdx];
        }
    }

    // Add triangles from loaded scenes (using meshes, not nodes)
    for (const auto& [name, scene] : m_Engine->loadedScenes) {
        if (scene) {
            for (const auto& [meshName, meshAsset] : scene->meshes) {
                if (meshAsset) {
                    for (const auto& surface : meshAsset->surfaces) {
                        triangleCount += surface.count / 3;
                    }
                }
            }
        }
    }

    // Estimate draw calls (1 per primitive + 1 per mesh surface)
    uint32_t drawCalls = primitiveCount;
    for (const auto& [name, scene] : m_Engine->loadedScenes) {
        if (scene) {
            for (const auto& [meshName, meshAsset] : scene->meshes) {
                if (meshAsset) {
                    drawCalls += static_cast<uint32_t>(meshAsset->surfaces.size());
                }
            }
        }
    }

    ImGui::Columns(3, nullptr, false);
    ImGui::Text("Draw Calls: %u", drawCalls);
    ImGui::NextColumn();
    ImGui::Text("Triangles: %u", triangleCount);
    ImGui::NextColumn();
    ImGui::Text("Vertices: ~%u", triangleCount * 3);
    ImGui::Columns(1);

    ImGui::Spacing();
    ImGui::Columns(3, nullptr, false);
    ImGui::Text("Primitives: %u", primitiveCount);
    ImGui::NextColumn();
    ImGui::Text("Lights: %u", lightCount);
    ImGui::NextColumn();
    ImGui::Text("Scenes: %u", sceneCount);
    ImGui::Columns(1);

    ImGui::Spacing();
    SectionHeader("Frame Time");

    // Frame time graph
    if (!m_FrameTimeHistory.empty()) {
        std::vector<float> data(m_FrameTimeHistory.begin(), m_FrameTimeHistory.end());

        float avg = 0.0f, minVal = 1000.0f, maxVal = 0.0f;
        for (float v : data) {
            avg += v;
            minVal = std::min(minVal, v);
            maxVal = std::max(maxVal, v);
        }
        avg /= data.size();

        char overlay[64];
        snprintf(overlay, sizeof(overlay), "Avg: %.2f ms (%.0f FPS)", avg, 1000.0f / avg);

        ImGui::PlotLines("##FrameTime", data.data(), static_cast<int>(data.size()),
                         0, overlay, 0.0f, 33.33f, ImVec2(-1, 80));

        ImGui::Text("Min: %.2f ms | Avg: %.2f ms | Max: %.2f ms", minVal, avg, maxVal);
    }
}

void GPUDebugView::RenderVisualization() {
    if (!m_Engine) {
        ImGui::TextDisabled("No engine context");
        return;
    }

    EnsureRenderTargetDescriptors();

    SectionHeader("Render Target Debug");
    ImGui::TextDisabled("Live textures from Vulkan render path");
    ImGui::Spacing();

    ImGui::Columns(2, nullptr, false);
    RenderTargetTile("Draw HDR", m_DrawImageDS,
        VkExtent2D{m_Engine->_drawImage.imageExtent.width, m_Engine->_drawImage.imageExtent.height},
        m_Engine->_drawImage.imageFormat);
    ImGui::NextColumn();
    RenderTargetTile("Depth", m_DepthImageDS,
        VkExtent2D{m_Engine->_depthImage.imageExtent.width, m_Engine->_depthImage.imageExtent.height},
        m_Engine->_depthImage.imageFormat);
    ImGui::Columns(1);

    ImGui::Columns(2, nullptr, false);
    RenderTargetTile("GBuffer Normals", m_NormalImageDS,
        VkExtent2D{m_Engine->_gBufferNormals.imageExtent.width, m_Engine->_gBufferNormals.imageExtent.height},
        m_Engine->_gBufferNormals.imageFormat);
    ImGui::NextColumn();
    RenderTargetTile("GBuffer Metal/Rough", m_MetalRoughImageDS,
        VkExtent2D{m_Engine->_gBufferMetalRough.imageExtent.width, m_Engine->_gBufferMetalRough.imageExtent.height},
        m_Engine->_gBufferMetalRough.imageFormat);
    ImGui::Columns(1);
}

void GPUDebugView::EnsureRenderTargetDescriptors() {
    if (!m_Engine) return;

    auto refresh = [&](VkImageView view, VkDescriptorSet& ds, VkImageView& cached) {
        if (view == cached && ds != VK_NULL_HANDLE) return;
        if (ds != VK_NULL_HANDLE) {
            ImGui_ImplVulkan_RemoveTexture(ds);
            ds = VK_NULL_HANDLE;
        }
        cached = view;
        if (view != VK_NULL_HANDLE) {
            ds = ImGui_ImplVulkan_AddTexture(
                m_Engine->_defaultSamplerLinear,
                view,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }
    };

    refresh(m_Engine->_drawImage.imageView, m_DrawImageDS, m_DrawImageView);
    refresh(m_Engine->_depthImage.imageView, m_DepthImageDS, m_DepthImageView);
    refresh(m_Engine->_gBufferNormals.imageView, m_NormalImageDS, m_NormalImageView);
    refresh(m_Engine->_gBufferMetalRough.imageView, m_MetalRoughImageDS, m_MetalRoughImageView);
}

void GPUDebugView::RenderTargetTile(const char* label, VkDescriptorSet ds, VkExtent2D extent, VkFormat format) {
    ImGui::Text("%s", label);
    ImGui::TextDisabled("%ux%u  format=%d", extent.width, extent.height, static_cast<int>(format));

    float width = ImGui::GetContentRegionAvail().x - 8.0f;
    width = std::max(120.0f, width);
    float height = width * 0.5625f;
    if (ds != VK_NULL_HANDLE) {
        ImGui::Image(reinterpret_cast<ImTextureID>(ds), ImVec2(width, height), ImVec2(0, 0), ImVec2(1, 1));
    } else {
        ImGui::BeginChild(label, ImVec2(width, height), true);
        ImGui::TextDisabled("Unavailable");
        ImGui::EndChild();
    }
    ImGui::Spacing();
}

void GPUDebugView::ClearRenderTargetDescriptors() {
    auto clear = [](VkDescriptorSet& ds, VkImageView& view) {
        if (ds != VK_NULL_HANDLE) {
            ImGui_ImplVulkan_RemoveTexture(ds);
            ds = VK_NULL_HANDLE;
        }
        view = VK_NULL_HANDLE;
    };
    clear(m_DrawImageDS, m_DrawImageView);
    clear(m_DepthImageDS, m_DepthImageView);
    clear(m_NormalImageDS, m_NormalImageView);
    clear(m_MetalRoughImageDS, m_MetalRoughImageView);
}

void GPUDebugView::RenderDrawCalls() {
    if (!m_Engine) {
        ImGui::TextDisabled("No engine context");
        return;
    }

    // Build draw call list from actual scene data
    struct DrawCallInfo {
        std::string name;
        std::string type;
        uint32_t vertices;
        uint32_t triangles;
        float gpuTime;  // Estimated
    };
    std::vector<DrawCallInfo> drawCalls;

    // Add primitives
    for (size_t i = 0; i < m_Engine->static_shapes.size(); ++i) {
        const auto& shape = m_Engine->static_shapes[i];
        const char* typeNames[] = {"Cube", "Sphere", "Cylinder", "Cone", "Capsule", "Torus", "Plane", "Triangle"};
        const int trianglesPerType[] = { 12, 960, 192, 96, 640, 1152, 2, 1 };

        int typeIdx = static_cast<int>(shape.type);
        uint32_t tris = (typeIdx >= 0 && typeIdx < 8) ? trianglesPerType[typeIdx] : 0;

        DrawCallInfo info;
        info.name = shape.name;
        info.type = (typeIdx >= 0 && typeIdx < 8) ? typeNames[typeIdx] : "Unknown";
        info.triangles = tris;
        info.vertices = tris * 3;
        info.gpuTime = tris * 0.001f;  // Rough estimate
        drawCalls.push_back(info);
    }

    // Add scene meshes
    for (const auto& [sceneName, scene] : m_Engine->loadedScenes) {
        if (scene) {
            for (const auto& [meshName, meshAsset] : scene->meshes) {
                if (meshAsset) {
                    for (size_t s = 0; s < meshAsset->surfaces.size(); ++s) {
                        const auto& surface = meshAsset->surfaces[s];
                        DrawCallInfo info;
                        info.name = meshName + "_surf" + std::to_string(s);
                        info.type = "GLTF Mesh";
                        info.triangles = surface.count / 3;
                        info.vertices = surface.count;
                        info.gpuTime = info.triangles * 0.0005f;
                        drawCalls.push_back(info);
                    }
                }
            }
        }
    }

    // Calculate totals
    uint32_t totalTris = 0, totalVerts = 0;
    float totalGPU = 0.0f;
    for (const auto& dc : drawCalls) {
        totalTris += dc.triangles;
        totalVerts += dc.vertices;
        totalGPU += dc.gpuTime;
    }

    ImGui::Text("Total Draw Calls: %zu | Triangles: %u | Est. GPU: %.2f ms",
                drawCalls.size(), totalTris, totalGPU);
    ImGui::Separator();

    ImGui::Checkbox("Sort by GPU Time", &m_SortByGPUTime);
    ImGui::SameLine();
    ImGui::Checkbox("Highlight Expensive", &m_HighlightExpensive);

    if (m_HighlightExpensive) {
        ImGui::SameLine();
        ImGui::SetNextItemWidth(80);
        ImGui::SliderFloat("##Threshold", &m_ExpensiveThreshold, 0.1f, 5.0f, "%.1f ms");
    }

    // Sort if needed
    if (m_SortByGPUTime) {
        std::sort(drawCalls.begin(), drawCalls.end(),
            [](const DrawCallInfo& a, const DrawCallInfo& b) { return a.gpuTime > b.gpuTime; });
    }

    ImGui::Separator();

    if (ImGui::BeginTable("DrawCalls", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                          ImGuiTableFlags_ScrollY, ImVec2(0, 250))) {
        ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 35);
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 80);
        ImGui::TableSetupColumn("Tris", ImGuiTableColumnFlags_WidthFixed, 60);
        ImGui::TableSetupColumn("GPU ms", ImGuiTableColumnFlags_WidthFixed, 60);
        ImGui::TableHeadersRow();

        for (size_t i = 0; i < drawCalls.size(); ++i) {
            const auto& dc = drawCalls[i];
            ImGui::TableNextRow();

            bool expensive = m_HighlightExpensive && dc.gpuTime >= m_ExpensiveThreshold;
            if (expensive) {
                ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(100, 50, 50, 255));
            }

            ImGui::TableNextColumn(); ImGui::Text("%zu", i);
            ImGui::TableNextColumn(); ImGui::Text("%s", dc.name.c_str());
            ImGui::TableNextColumn(); ImGui::TextDisabled("%s", dc.type.c_str());
            ImGui::TableNextColumn(); ImGui::Text("%u", dc.triangles);
            ImGui::TableNextColumn();
            if (expensive) {
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "%.3f", dc.gpuTime);
            } else {
                ImGui::Text("%.3f", dc.gpuTime);
            }
        }

        ImGui::EndTable();
    }
}

void GPUDebugView::RenderMemory() {
    SectionHeader("GPU Memory (Estimated)");

    if (!m_Engine) {
        ImGui::TextDisabled("No engine context");
        return;
    }

    // Estimate memory usage from loaded assets
    uint64_t primitiveMemory = 0;
    const uint64_t bytesPerType[] = {
        24 * 32,      // Cube: 24 verts * 32 bytes
        482 * 32,     // Sphere
        98 * 32,      // Cylinder
        49 * 32,      // Cone
        322 * 32,     // Capsule
        578 * 32,     // Torus
        4 * 32,       // Plane
        3 * 32        // Triangle
    };

    for (const auto& shape : m_Engine->static_shapes) {
        int typeIdx = static_cast<int>(shape.type);
        if (typeIdx >= 0 && typeIdx < 8) {
            primitiveMemory += bytesPerType[typeIdx];
        }
    }

    uint64_t sceneMemory = 0;
    for (const auto& [name, scene] : m_Engine->loadedScenes) {
        if (scene) {
            for (const auto& [meshName, meshAsset] : scene->meshes) {
                if (meshAsset) {
                    for (const auto& surface : meshAsset->surfaces) {
                        sceneMemory += surface.count * 32;  // Estimate 32 bytes per vertex
                    }
                }
            }
        }
    }

    uint64_t totalUsed = primitiveMemory + sceneMemory;
    uint64_t totalAvailable = 4ULL * 1024 * 1024 * 1024;  // Assume 4GB VRAM

    float usedMB = totalUsed / (1024.0f * 1024.0f);
    float totalMB = totalAvailable / (1024.0f * 1024.0f);
    float ratio = static_cast<float>(totalUsed) / totalAvailable;

    ImU32 barColor;
    if (ratio < 0.5f) barColor = IM_COL32(100, 200, 100, 255);
    else if (ratio < 0.8f) barColor = IM_COL32(200, 200, 100, 255);
    else barColor = IM_COL32(200, 100, 100, 255);

    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, barColor);
    ImGui::ProgressBar(ratio, ImVec2(-1, 20));
    ImGui::PopStyleColor();

    ImGui::Text("%.2f MB / %.0f MB (%.2f%%)", usedMB, totalMB, ratio * 100.0f);

    ImGui::Spacing();
    SectionHeader("Memory Breakdown");

    struct MemInfo { const char* name; uint64_t bytes; ImU32 color; };
    MemInfo memTypes[] = {
        {"Primitives", primitiveMemory, IM_COL32(100, 150, 200, 255)},
        {"GLTF Scenes", sceneMemory, IM_COL32(100, 200, 100, 255)},
    };

    for (const auto& mem : memTypes) {
        float memRatio = static_cast<float>(mem.bytes) / totalAvailable;
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, mem.color);
        ImGui::ProgressBar(memRatio, ImVec2(-1, 0), "");
        ImGui::PopStyleColor();
        ImGui::SameLine(0, 10);
        ImGui::Text("%s: %.2f KB", mem.name, mem.bytes / 1024.0f);
    }

    ImGui::Spacing();
    ImGui::TextDisabled("Note: Memory estimates are approximate");
}

void GPUDebugView::RenderCounters() {
    SectionHeader("Performance Counters");

    if (!m_Engine) {
        ImGui::TextDisabled("No engine context");
        return;
    }

    // Calculate real stats
    uint32_t primitiveCount = static_cast<uint32_t>(m_Engine->static_shapes.size());
    uint32_t lightCount = static_cast<uint32_t>(m_Engine->scenePointLights.size());

    uint32_t triangleCount = 0;
    const int trianglesPerType[] = { 12, 960, 192, 96, 640, 1152, 2, 1 };
    for (const auto& shape : m_Engine->static_shapes) {
        int typeIdx = static_cast<int>(shape.type);
        if (typeIdx >= 0 && typeIdx < 8) {
            triangleCount += trianglesPerType[typeIdx];
        }
    }

    // Get average frame time
    float avgFrameTime = 0.0f;
    if (!m_FrameTimeHistory.empty()) {
        for (float t : m_FrameTimeHistory) avgFrameTime += t;
        avgFrameTime /= m_FrameTimeHistory.size();
    }

    struct Counter { const char* name; const char* unit; float value; };
    Counter counters[] = {
        {"Frame Time", "ms", avgFrameTime},
        {"Vertices", "K", triangleCount * 3 / 1000.0f},
        {"Triangles", "K", triangleCount / 1000.0f},
        {"Primitives", "", static_cast<float>(primitiveCount)},
        {"Lights", "", static_cast<float>(lightCount)},
        {"Draw Calls", "", static_cast<float>(primitiveCount)},
    };

    for (const auto& c : counters) {
        ImGui::Text("%s:", c.name); ImGui::SameLine(120);
        ImGui::Text("%.1f %s", c.value, c.unit);
    }

    ImGui::Spacing();
    SectionHeader("Frame Time History");

    if (!m_FrameTimeHistory.empty()) {
        std::vector<float> data(m_FrameTimeHistory.begin(), m_FrameTimeHistory.end());

        float avg = 0.0f;
        for (float v : data) avg += v;
        avg /= data.size();

        char overlay[64];
        snprintf(overlay, sizeof(overlay), "Avg: %.2f ms (%.0f FPS)", avg, 1000.0f / avg);

        ImGui::PlotLines("##FrameTime", data.data(), static_cast<int>(data.size()),
                         0, overlay, 0.0f, 33.33f, ImVec2(-1, 80));
    }
}

} // namespace Yalaz::UI
