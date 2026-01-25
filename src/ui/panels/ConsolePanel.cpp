// =============================================================================
// YALAZ ENGINE - Console Panel Implementation
// =============================================================================

#include "ConsolePanel.h"
#include "../EditorTheme.h"
#include "../../vk_engine.h"
#include <imgui.h>

namespace Yalaz::UI {

ConsolePanel::ConsolePanel()
    : Panel("Console")
{
}

void ConsolePanel::OnRender() {
    // Use ImGui's built-in delta time (much more efficient)
    float deltaTime = ImGui::GetIO().DeltaTime;

    if (deltaTime > 0.0f && deltaTime < 1.0f) {
        // Smooth FPS using exponential moving average
        float instantFps = 1.0f / deltaTime;
        m_CurrentFps = m_CurrentFps * 0.95f + instantFps * 0.05f;
        m_FrameTime = deltaTime * 1000.0f;

        // Update history less frequently (every 4 frames)
        m_FrameCounter++;
        if (m_FrameCounter >= 4) {
            m_FpsHistory[m_FpsHistoryIndex] = m_CurrentFps;
            m_FpsHistoryIndex = (m_FpsHistoryIndex + 1) % 120;
            m_FrameCounter = 0;
        }
    }

    if (BeginPanel(ImGuiWindowFlags_NoCollapse)) {
        if (ImGui::BeginTabBar("ConsoleTabs")) {
            if (ImGui::BeginTabItem("Stats")) {
                RenderStatsSection();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Performance")) {
                RenderPerformanceSection();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }
    EndPanel();
}

void ConsolePanel::RenderStatsSection() {
    if (!m_Engine) return;

    ImGui::Columns(3, nullptr, false);

    // FPS
    ImGui::PushStyleColor(ImGuiCol_Text, m_CurrentFps >= 60.0f ? Colors::TextPrimary :
                          (m_CurrentFps >= 30.0f ? ImVec4(1.0f, 0.8f, 0.2f, 1.0f) :
                                                   ImVec4(1.0f, 0.3f, 0.3f, 1.0f)));
    ImGui::Text("FPS: %.0f", m_CurrentFps);
    ImGui::PopStyleColor();

    ImGui::NextColumn();

    // Frame time
    ImGui::Text("Frame: %.2f ms", m_FrameTime);

    ImGui::NextColumn();

    // Primitives count
    ImGui::Text("Primitives: %zu", m_Engine->static_shapes.size());

    ImGui::Columns(1);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Additional stats
    ImGui::Columns(2, nullptr, false);

    ImGui::Text("Point Lights: %zu", m_Engine->scenePointLights.size());
    ImGui::NextColumn();

    ImGui::Text("View Mode: %s",
        m_Engine->_currentViewMode == VulkanEngine::ViewMode::Wireframe ? "Wireframe" :
        m_Engine->_currentViewMode == VulkanEngine::ViewMode::Shaded ? "Shaded" :
        m_Engine->_currentViewMode == VulkanEngine::ViewMode::MaterialPreview ? "Material" :
        m_Engine->_currentViewMode == VulkanEngine::ViewMode::Rendered ? "Rendered" : "Path Traced");

    ImGui::Columns(1);

    ImGui::Spacing();

    // Loaded scenes
    if (!m_Engine->loadedScenes.empty()) {
        ImGui::Text("Loaded Scenes:");
        for (auto& [name, scene] : m_Engine->loadedScenes) {
            ImGui::BulletText("%s", name.c_str());
        }
    }
}

void ConsolePanel::RenderPerformanceSection() {
    // FPS Graph - use simple fixed scale
    ImGui::Text("FPS History:");

    ImGui::PlotLines("##FPSGraph", m_FpsHistory, 120, m_FpsHistoryIndex,
                     nullptr, 0.0f, 144.0f, ImVec2(-1, 80));

    ImGui::Spacing();
    ImGui::TextDisabled("Target: 60 FPS (16.67 ms)");
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Display smoothed current FPS (already calculated in OnRender)
    ImGui::Text("Current FPS: %.1f", m_CurrentFps);
    ImGui::Text("Frame Time: %.2f ms", m_FrameTime);
}

} // namespace Yalaz::UI
