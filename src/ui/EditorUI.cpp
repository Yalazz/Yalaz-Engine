// =============================================================================
// YALAZ ENGINE - Editor UI Implementation (Optimized)
// =============================================================================

#include "EditorUI.h"
#include "panels/SceneHierarchyPanel.h"
#include "panels/InspectorPanel.h"
#include "panels/ViewportPanel.h"
#include "panels/LightingPanel.h"
#include "panels/ConsolePanel.h"

namespace Yalaz::UI {

void EditorUI::Init(VulkanEngine* engine) {
    m_Engine = engine;

    // Apply professional theme
    EditorTheme::Get().Apply();

    // Register all panels and cache pointers
    m_SceneHierarchy = PanelManager::Get().AddPanel<SceneHierarchyPanel>();
    m_Inspector = PanelManager::Get().AddPanel<InspectorPanel>();
    m_Viewport = PanelManager::Get().AddPanel<ViewportPanel>();
    m_Lighting = PanelManager::Get().AddPanel<LightingPanel>();
    m_Console = PanelManager::Get().AddPanel<ConsolePanel>();

    // Initialize panels
    PanelManager::Get().Init(engine);
}

void EditorUI::Shutdown() {
    PanelManager::Get().Shutdown();
}

void EditorUI::Render() {
    // Setup layout only when viewport changes
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    if (viewport->Size.x != m_LastViewportSize.x || viewport->Size.y != m_LastViewportSize.y) {
        m_LastViewportSize = viewport->Size;
        m_LayoutDirty = true;
    }

    if (m_LayoutDirty) {
        CalculateLayout();
        m_LayoutDirty = false;
    }

    // Apply cached positions and render panels directly
    RenderPanels();
}

void EditorUI::CalculateLayout() {
    float w = m_LastViewportSize.x;
    float h = m_LastViewportSize.y;
    float top = Layout::MenuBarHeight;
    float mainH = h - top - Layout::BottomPanelHeight;

    // Cache all positions and sizes
    m_LayoutCache[0] = {ImVec2(0, top), ImVec2(Layout::LeftPanelWidth, mainH)};  // SceneHierarchy
    m_LayoutCache[1] = {ImVec2(w - Layout::RightPanelWidth, top), ImVec2(Layout::RightPanelWidth, mainH * 0.5f)};  // Inspector
    m_LayoutCache[2] = {ImVec2(w - Layout::RightPanelWidth, top + mainH * 0.5f), ImVec2(Layout::RightPanelWidth, mainH * 0.25f)};  // Viewport
    m_LayoutCache[3] = {ImVec2(w - Layout::RightPanelWidth, top + mainH * 0.75f), ImVec2(Layout::RightPanelWidth, mainH * 0.25f)};  // Lighting
    m_LayoutCache[4] = {ImVec2(0, h - Layout::BottomPanelHeight), ImVec2(w, Layout::BottomPanelHeight)};  // Console

    m_ViewportPos = ImVec2(Layout::LeftPanelWidth, top);
    m_ViewportSize = ImVec2(w - Layout::LeftPanelWidth - Layout::RightPanelWidth, mainH);
}

void EditorUI::RenderPanels() {
    // Render panels with cached positions (no string lookups!)
    if (m_SceneHierarchy && m_SceneHierarchy->IsOpen()) {
        ImGui::SetNextWindowPos(m_LayoutCache[0].pos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(m_LayoutCache[0].size, ImGuiCond_Always);
        m_SceneHierarchy->OnRender();
    }

    if (m_Inspector && m_Inspector->IsOpen()) {
        ImGui::SetNextWindowPos(m_LayoutCache[1].pos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(m_LayoutCache[1].size, ImGuiCond_Always);
        m_Inspector->OnRender();
    }

    if (m_Viewport && m_Viewport->IsOpen()) {
        ImGui::SetNextWindowPos(m_LayoutCache[2].pos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(m_LayoutCache[2].size, ImGuiCond_Always);
        m_Viewport->OnRender();
    }

    if (m_Lighting && m_Lighting->IsOpen()) {
        ImGui::SetNextWindowPos(m_LayoutCache[3].pos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(m_LayoutCache[3].size, ImGuiCond_Always);
        m_Lighting->OnRender();
    }

    if (m_Console && m_Console->IsOpen()) {
        ImGui::SetNextWindowPos(m_LayoutCache[4].pos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(m_LayoutCache[4].size, ImGuiCond_Always);
        m_Console->OnRender();
    }
}

} // namespace Yalaz::UI
