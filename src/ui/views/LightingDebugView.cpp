// =============================================================================
// YALAZ ENGINE - Lighting Debug View Implementation
// =============================================================================
// Dynamic lighting debug view connected to engine:
// - Real-time light list from scenePointLights
// - Light properties visualization
// - Attenuation preview
// - Light coverage visualization
// =============================================================================

#include "LightingDebugView.h"
#include "../../vk_engine.h"
#include <algorithm>
#include <cstdlib>
#include <cmath>

namespace Yalaz::UI {

void LightingDebugView::OnRender() {
    if (!BeginView(ImGuiWindowFlags_MenuBar)) {
        EndView();
        return;
    }

    if (ImGui::BeginMenuBar()) {
        const char* modes[] = {"Light List", "Visualization", "Shadow Maps", "Statistics"};
        ImGui::SetNextItemWidth(120);
        ImGui::Combo("View", &m_DebugMode, modes, 4);
        ImGui::EndMenuBar();
    }

    if (!m_Engine) {
        ImGui::TextDisabled("No engine context");
        EndView();
        return;
    }

    switch (m_DebugMode) {
        case 0: RenderLightList(); break;
        case 1: RenderVisualization(); break;
        case 2: RenderShadowMaps(); break;
        case 3: RenderStatistics(); break;
    }

    EndView();
}

void LightingDebugView::RenderLightList() {
    SectionHeader("Scene Point Lights");

    if (m_Engine->scenePointLights.empty()) {
        ImGui::TextDisabled("No lights in scene");
        ImGui::Spacing();
        if (ImGui::Button("Add Point Light", ImVec2(-1, 0))) {
            PointLight newLight;
            newLight.position = m_Engine->mainCamera.position + glm::vec3(0, 2, 0);
            newLight.color = glm::vec3(1.0f, 0.9f, 0.8f);
            newLight.intensity = 10.0f;
            newLight.radius = 10.0f;
            m_Engine->scenePointLights.push_back(newLight);
        }
        return;
    }

    ImGui::Text("Total Lights: %zu", m_Engine->scenePointLights.size());
    ImGui::Separator();

    // Light table
    if (ImGui::BeginTable("LightTable", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                          ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable, ImVec2(0, 200))) {
        ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed, 30);
        ImGui::TableSetupColumn("Color", ImGuiTableColumnFlags_WidthFixed, 35);
        ImGui::TableSetupColumn("Position", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Intensity", ImGuiTableColumnFlags_WidthFixed, 65);
        ImGui::TableSetupColumn("Radius", ImGuiTableColumnFlags_WidthFixed, 55);
        ImGui::TableSetupColumn("Act", ImGuiTableColumnFlags_WidthFixed, 60);
        ImGui::TableHeadersRow();

        int lightToDelete = -1;

        for (size_t i = 0; i < m_Engine->scenePointLights.size(); ++i) {
            auto& light = m_Engine->scenePointLights[i];
            ImGui::TableNextRow();

            bool isSelected = (m_Engine->selectedLightIndex == static_cast<int>(i));

            // Index
            ImGui::TableNextColumn();
            if (ImGui::Selectable(std::to_string(i).c_str(), isSelected,
                                  ImGuiSelectableFlags_SpanAllColumns)) {
                m_Engine->selectedLightIndex = static_cast<int>(i);
                m_Engine->selectedPrimitiveIndex = -1;
                m_Engine->selectedNode = nullptr;
            }

            // Color preview
            ImGui::TableNextColumn();
            ImVec4 lightColor(light.color.r, light.color.g, light.color.b, 1.0f);
            ImGui::ColorButton("##Color", lightColor, ImGuiColorEditFlags_NoTooltip, ImVec2(20, 18));

            // Position
            ImGui::TableNextColumn();
            ImGui::Text("%.1f, %.1f, %.1f", light.position.x, light.position.y, light.position.z);

            // Intensity
            ImGui::TableNextColumn();
            ImGui::Text("%.1f", light.intensity);

            // Radius
            ImGui::TableNextColumn();
            ImGui::Text("%.1f", light.radius);

            // Actions
            ImGui::TableNextColumn();
            ImGui::PushID(static_cast<int>(i));
            if (ImGui::SmallButton("F")) {
                m_Engine->mainCamera.position = light.position + glm::vec3(0, 3, 5);
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Focus Camera");
            ImGui::SameLine();
            if (ImGui::SmallButton("X")) {
                lightToDelete = static_cast<int>(i);
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Delete Light");
            ImGui::PopID();
        }

        ImGui::EndTable();

        // Delete light outside table
        if (lightToDelete >= 0) {
            m_Engine->scenePointLights.erase(m_Engine->scenePointLights.begin() + lightToDelete);
            if (m_Engine->selectedLightIndex == lightToDelete) {
                m_Engine->selectedLightIndex = -1;
            } else if (m_Engine->selectedLightIndex > lightToDelete) {
                m_Engine->selectedLightIndex--;
            }
        }
    }

    // Add light button
    ImGui::Spacing();
    if (ImGui::Button("Add Point Light", ImVec2(-1, 0))) {
        PointLight newLight;
        newLight.position = m_Engine->mainCamera.position + glm::vec3(0, 2, 0);
        newLight.color = glm::vec3(1.0f, 0.9f, 0.8f);
        newLight.intensity = 10.0f;
        newLight.radius = 10.0f;
        m_Engine->scenePointLights.push_back(newLight);
    }

    // Selected light details
    if (m_Engine->selectedLightIndex >= 0 &&
        m_Engine->selectedLightIndex < static_cast<int>(m_Engine->scenePointLights.size())) {
        ImGui::Spacing();
        SectionHeader("Selected Light Details");

        auto& light = m_Engine->scenePointLights[m_Engine->selectedLightIndex];

        // Editable properties
        ImGui::DragFloat3("Position", &light.position.x, 0.1f);

        float col[3] = { light.color.r, light.color.g, light.color.b };
        if (ImGui::ColorEdit3("Color", col)) {
            light.color = glm::vec3(col[0], col[1], col[2]);
        }

        ImGui::SliderFloat("Intensity", &light.intensity, 0.0f, 100.0f);
        ImGui::SliderFloat("Radius", &light.radius, 0.1f, 100.0f);

        // Attenuation preview
        ImGui::Spacing();
        ImGui::TextDisabled("Attenuation at distances:");
        for (float d : {1.0f, 5.0f, 10.0f, 20.0f}) {
            float atten = 1.0f / (d * d);
            float effectiveIntensity = light.intensity * atten;

            ImGui::Text("  %.0fm: ", d);
            ImGui::SameLine(60);

            // Visual bar
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram,
                ImVec4(light.color.r, light.color.g, light.color.b, 1.0f));
            ImGui::ProgressBar(effectiveIntensity / light.intensity, ImVec2(80, 0), "");
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::Text("%.2f", effectiveIntensity);
        }
    }
}

void LightingDebugView::RenderVisualization() {
    SectionHeader("Light Coverage (Top-Down View)");

    if (m_Engine->scenePointLights.empty()) {
        ImGui::TextDisabled("No lights to visualize");
        return;
    }

    ImGui::Checkbox("Show Radius", &m_ShowLightVolumes);

    // Top-down view of light positions
    ImVec2 avail = ImGui::GetContentRegionAvail();
    float vizSize = std::min(avail.x - 20, avail.y - 50);
    vizSize = std::max(vizSize, 200.0f);

    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // Background
    drawList->AddRectFilled(pos, ImVec2(pos.x + vizSize, pos.y + vizSize),
                            IM_COL32(20, 20, 25, 255));

    // Grid
    float gridStep = vizSize / 10;
    for (int i = 0; i <= 10; ++i) {
        float offset = i * gridStep;
        drawList->AddLine(ImVec2(pos.x + offset, pos.y),
                          ImVec2(pos.x + offset, pos.y + vizSize),
                          IM_COL32(40, 40, 45, 255));
        drawList->AddLine(ImVec2(pos.x, pos.y + offset),
                          ImVec2(pos.x + vizSize, pos.y + offset),
                          IM_COL32(40, 40, 45, 255));
    }

    // Calculate bounds for mapping
    float minX = 1e10f, maxX = -1e10f;
    float minZ = 1e10f, maxZ = -1e10f;

    for (const auto& light : m_Engine->scenePointLights) {
        minX = std::min(minX, light.position.x - light.radius);
        maxX = std::max(maxX, light.position.x + light.radius);
        minZ = std::min(minZ, light.position.z - light.radius);
        maxZ = std::max(maxZ, light.position.z + light.radius);
    }

    float rangeX = maxX - minX;
    float rangeZ = maxZ - minZ;
    float range = std::max(rangeX, rangeZ) * 1.2f;
    if (range < 1.0f) range = 20.0f;

    float centerX = (minX + maxX) * 0.5f;
    float centerZ = (minZ + maxZ) * 0.5f;

    // Draw lights
    for (size_t i = 0; i < m_Engine->scenePointLights.size(); ++i) {
        const auto& light = m_Engine->scenePointLights[i];

        // Map position to visualization
        float normX = (light.position.x - centerX) / range + 0.5f;
        float normZ = (light.position.z - centerZ) / range + 0.5f;

        ImVec2 lightPos(pos.x + normX * vizSize, pos.y + normZ * vizSize);

        // Draw radius circle if enabled
        if (m_ShowLightVolumes) {
            float radiusPixels = (light.radius / range) * vizSize;
            ImU32 radiusColor = IM_COL32(
                static_cast<int>(light.color.r * 80),
                static_cast<int>(light.color.g * 80),
                static_cast<int>(light.color.b * 80),
                60);
            drawList->AddCircleFilled(lightPos, radiusPixels, radiusColor);
        }

        // Draw light point
        ImU32 lightColor = IM_COL32(
            static_cast<int>(light.color.r * 255),
            static_cast<int>(light.color.g * 255),
            static_cast<int>(light.color.b * 255),
            255);

        float pointSize = 5.0f + light.intensity * 0.2f;
        pointSize = std::min(pointSize, 15.0f);

        bool isSelected = (m_Engine->selectedLightIndex == static_cast<int>(i));
        if (isSelected) {
            drawList->AddCircle(lightPos, pointSize + 3, IM_COL32(255, 255, 0, 255), 0, 2.0f);
        }

        drawList->AddCircleFilled(lightPos, pointSize, lightColor);

        // Label
        char label[8];
        snprintf(label, sizeof(label), "%zu", i);
        drawList->AddText(ImVec2(lightPos.x + pointSize + 2, lightPos.y - 6),
                          IM_COL32(200, 200, 200, 255), label);
    }

    // Border
    drawList->AddRect(pos, ImVec2(pos.x + vizSize, pos.y + vizSize),
                      IM_COL32(60, 60, 70, 255));

    ImGui::Dummy(ImVec2(vizSize, vizSize));

    // Legend
    ImGui::TextDisabled("XZ plane | Scale: %.1f units", range);
}

void LightingDebugView::RenderShadowMaps() {
    SectionHeader("Shadow Maps (Preview)");

    ImGui::TextDisabled("Shadow map visualization");
    ImGui::SliderFloat("Depth Scale", &m_DepthScale, 0.1f, 10.0f);
    ImGui::Spacing();

    // Demo shadow map previews
    float previewSize = 150.0f;
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    for (int i = 0; i < 2; ++i) {
        if (i > 0) ImGui::SameLine();

        ImGui::BeginGroup();

        ImVec2 pos = ImGui::GetCursorScreenPos();

        // Draw depth gradient
        drawList->AddRectFilled(pos, ImVec2(pos.x + previewSize, pos.y + previewSize),
                                IM_COL32(20, 20, 20, 255));

        for (int y = 0; y < static_cast<int>(previewSize); ++y) {
            float depth = static_cast<float>(y) / previewSize * m_DepthScale;
            depth = std::min(1.0f, depth);
            uint8_t gray = static_cast<uint8_t>(depth * 255);
            drawList->AddLine(ImVec2(pos.x, pos.y + y),
                              ImVec2(pos.x + previewSize, pos.y + y),
                              IM_COL32(gray, gray, gray, 255));
        }

        drawList->AddRect(pos, ImVec2(pos.x + previewSize, pos.y + previewSize),
                          IM_COL32(80, 80, 80, 255));

        ImGui::Dummy(ImVec2(previewSize, previewSize));
        ImGui::Text("Shadow Map %d", i);
        ImGui::TextDisabled("2048x2048");

        ImGui::EndGroup();
    }

    ImGui::Spacing();
    ImGui::TextDisabled("Note: Placeholder visualization");
}

void LightingDebugView::RenderStatistics() {
    SectionHeader("Lighting Statistics");

    if (m_Engine->scenePointLights.empty()) {
        ImGui::TextDisabled("No lights in scene");
        return;
    }

    // Calculate statistics
    float totalIntensity = 0.0f;
    float maxIntensity = 0.0f;
    float minIntensity = 1000000.0f;
    float totalRadius = 0.0f;

    glm::vec3 avgColor(0.0f);
    glm::vec3 lightCenter(0.0f);

    for (const auto& light : m_Engine->scenePointLights) {
        totalIntensity += light.intensity;
        maxIntensity = std::max(maxIntensity, light.intensity);
        minIntensity = std::min(minIntensity, light.intensity);
        totalRadius += light.radius;
        avgColor += light.color;
        lightCenter += light.position;
    }

    size_t count = m_Engine->scenePointLights.size();
    float avgIntensity = totalIntensity / count;
    avgColor /= static_cast<float>(count);
    lightCenter /= static_cast<float>(count);

    ImGui::Columns(2, nullptr, false);

    ImGui::Text("Light Count:"); ImGui::NextColumn();
    ImGui::Text("%zu", count); ImGui::NextColumn();

    ImGui::Text("Total Intensity:"); ImGui::NextColumn();
    ImGui::Text("%.1f", totalIntensity); ImGui::NextColumn();

    ImGui::Text("Avg Intensity:"); ImGui::NextColumn();
    ImGui::Text("%.1f", avgIntensity); ImGui::NextColumn();

    ImGui::Text("Max Intensity:"); ImGui::NextColumn();
    ImGui::Text("%.1f", maxIntensity); ImGui::NextColumn();

    ImGui::Text("Min Intensity:"); ImGui::NextColumn();
    ImGui::Text("%.1f", minIntensity); ImGui::NextColumn();

    ImGui::Text("Avg Radius:"); ImGui::NextColumn();
    ImGui::Text("%.1f", totalRadius / count); ImGui::NextColumn();

    ImGui::Columns(1);

    ImGui::Spacing();
    SectionHeader("Average Color");

    ImVec4 avgColorVec(avgColor.r, avgColor.g, avgColor.b, 1.0f);
    ImGui::ColorButton("##AvgColor", avgColorVec, 0, ImVec2(80, 25));
    ImGui::SameLine();
    ImGui::Text("(%.2f, %.2f, %.2f)", avgColor.r, avgColor.g, avgColor.b);

    ImGui::Spacing();
    SectionHeader("Light Center");
    ImGui::Text("(%.1f, %.1f, %.1f)", lightCenter.x, lightCenter.y, lightCenter.z);

    if (ImGui::Button("Focus Camera on Center")) {
        m_Engine->mainCamera.position = lightCenter + glm::vec3(0, 5, 10);
    }

    // Intensity distribution
    ImGui::Spacing();
    SectionHeader("Intensity Distribution");

    std::vector<float> intensities;
    for (const auto& light : m_Engine->scenePointLights) {
        intensities.push_back(light.intensity);
    }

    if (!intensities.empty()) {
        ImGui::PlotHistogram("##IntensityHist", intensities.data(),
                             static_cast<int>(intensities.size()), 0, nullptr,
                             0.0f, maxIntensity * 1.1f, ImVec2(-1, 60));
    }
}

void LightingDebugView::RenderCascades() {
    // Kept for compatibility
}

void LightingDebugView::RenderLightContribution() {
    // Kept for compatibility
}

void LightingDebugView::RenderProbes() {
    // Kept for compatibility
}

void LightingDebugView::RenderAO() {
    // Kept for compatibility
}

void LightingDebugView::RenderGI() {
    // Kept for compatibility
}

} // namespace Yalaz::UI
