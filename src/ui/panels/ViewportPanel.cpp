// =============================================================================
// YALAZ ENGINE - Viewport Panel Implementation
// =============================================================================

#include "ViewportPanel.h"
#include "../EditorTheme.h"
#include "../../vk_engine.h"
#include <imgui.h>

namespace Yalaz::UI {

ViewportPanel::ViewportPanel()
    : Panel("Viewport")
{
}

void ViewportPanel::OnRender() {
    if (BeginPanel(ImGuiWindowFlags_NoCollapse)) {
        RenderViewModeSection();
        ImGui::Spacing();
        RenderDisplaySection();
        ImGui::Spacing();
        RenderGridSection();
    }
    EndPanel();
}

void ViewportPanel::RenderViewModeSection() {
    if (!m_Engine) return;

    if (ImGui::CollapsingHeader("View Mode", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent(8.0f);

        const char* viewModes[] = { "Solid", "Shaded", "Material Preview", "Rendered", "Wireframe", "Normals", "UV Checker" };
        int currentMode = static_cast<int>(m_Engine->_currentViewMode);

        ImGui::SetNextItemWidth(-1);
        if (ImGui::Combo("##ViewMode", &currentMode, viewModes, IM_ARRAYSIZE(viewModes))) {
            m_Engine->_currentViewMode = static_cast<VulkanEngine::ViewMode>(currentMode);
        }

        ImGui::Unindent(8.0f);
    }
}

void ViewportPanel::RenderDisplaySection() {
    if (!m_Engine) return;

    if (ImGui::CollapsingHeader("Display", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent(8.0f);

        ImGui::Checkbox("Show Grid", &m_Engine->_showGrid);
        ImGui::Checkbox("Show Outlines", &m_Engine->_showOutline);

        ImGui::Unindent(8.0f);
    }
}

void ViewportPanel::RenderGridSection() {
    if (!m_Engine) return;

    if (ImGui::CollapsingHeader("Grid Settings")) {
        ImGui::Indent(8.0f);

        GridSettings& grid = m_Engine->_gridSettings;

        // Presets
        ImGui::Text("Presets:");
        if (ImGui::Button("Default", ImVec2(60, 0))) {
            grid = GridSettings();  // Reset to default
            grid.currentPreset = 0;
        }
        ImGui::SameLine();
        if (ImGui::Button("Blender", ImVec2(60, 0))) {
            grid.baseGridSize = 1.0f;
            grid.majorGridMultiplier = 10.0f;
            grid.majorLineColor = glm::vec3(0.3f, 0.3f, 0.3f);
            grid.minorLineColor = glm::vec3(0.2f, 0.2f, 0.2f);
            grid.currentPreset = 1;
        }
        ImGui::SameLine();
        if (ImGui::Button("Unity", ImVec2(60, 0))) {
            grid.baseGridSize = 1.0f;
            grid.majorGridMultiplier = 10.0f;
            grid.majorLineColor = glm::vec3(0.5f, 0.5f, 0.5f);
            grid.minorLineColor = glm::vec3(0.3f, 0.3f, 0.3f);
            grid.currentPreset = 2;
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Size
        ImGui::DragFloat("Base Grid Size", &grid.baseGridSize, 0.1f, 0.1f, 100.0f);
        ImGui::DragFloat("Major Multiplier", &grid.majorGridMultiplier, 1.0f, 2.0f, 100.0f);

        ImGui::Spacing();

        // Colors
        ImGui::ColorEdit3("Major Lines", &grid.majorLineColor.x);
        ImGui::ColorEdit3("Minor Lines", &grid.minorLineColor.x);

        ImGui::Spacing();

        // Axis colors
        ImGui::ColorEdit3("X Axis", &grid.xAxisColor.x);
        ImGui::ColorEdit3("Z Axis", &grid.zAxisColor.x);

        ImGui::Spacing();

        // Advanced
        ImGui::DragFloat("Line Width", &grid.lineWidth, 0.1f, 0.1f, 5.0f);
        ImGui::DragFloat("Fade Distance", &grid.fadeDistance, 50.0f, 10.0f, 50000.0f);
        ImGui::SliderFloat("Grid Opacity", &grid.gridOpacity, 0.0f, 1.0f);

        ImGui::Spacing();

        // Additional settings
        ImGui::Checkbox("Dynamic LOD", &grid.dynamicLOD);
        ImGui::Checkbox("Show Axis Colors", &grid.showAxisColors);
        ImGui::Checkbox("Anti-Aliasing", &grid.antiAliasing);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Grid Behavior
        ImGui::Text("Grid Behavior:");
        ImGui::Checkbox("Infinite Grid (Follow Camera)", &grid.infiniteGrid);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Grid position follows camera movement");
        }

        ImGui::Checkbox("Fade From Camera", &grid.fadeFromCamera);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("ON: Grid fades from camera position\nOFF: Grid fades from world origin (0,0)");
        }

        ImGui::Unindent(8.0f);
    }
}

} // namespace Yalaz::UI
