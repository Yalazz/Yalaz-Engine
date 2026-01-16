#pragma once
// =============================================================================
// YALAZ ENGINE - Editor UI Controller (Optimized)
// =============================================================================
// Main editor UI system - handles layout, menu bar, and panel coordination
// =============================================================================

#include <imgui.h>
#include "EditorTheme.h"
#include "PanelManager.h"
#include "EditorSelection.h"

class VulkanEngine;

namespace Yalaz::UI {

// Forward declarations
class SceneHierarchyPanel;
class InspectorPanel;
class ViewportPanel;
class LightingPanel;
class ConsolePanel;

// Layout constants
namespace Layout {
    constexpr float LeftPanelWidth = 280.0f;
    constexpr float RightPanelWidth = 320.0f;
    constexpr float BottomPanelHeight = 200.0f;
    constexpr float MenuBarHeight = 22.0f;
}

// Cached layout data
struct LayoutRect {
    ImVec2 pos;
    ImVec2 size;
};

class EditorUI {
public:
    static EditorUI& Get() {
        static EditorUI instance;
        return instance;
    }

    void Init(VulkanEngine* engine);
    void Shutdown();
    void Render();

    ImVec2 GetViewportPos() const { return m_ViewportPos; }
    ImVec2 GetViewportSize() const { return m_ViewportSize; }

private:
    EditorUI() = default;
    ~EditorUI() = default;
    EditorUI(const EditorUI&) = delete;
    EditorUI& operator=(const EditorUI&) = delete;

    void CalculateLayout();
    void RenderPanels();

    VulkanEngine* m_Engine = nullptr;

    // Cached panel pointers (no string lookups!)
    SceneHierarchyPanel* m_SceneHierarchy = nullptr;
    InspectorPanel* m_Inspector = nullptr;
    ViewportPanel* m_Viewport = nullptr;
    LightingPanel* m_Lighting = nullptr;
    ConsolePanel* m_Console = nullptr;

    // Layout caching
    ImVec2 m_LastViewportSize = {0, 0};
    bool m_LayoutDirty = true;
    LayoutRect m_LayoutCache[5];

    // Viewport state
    ImVec2 m_ViewportPos = {0, 0};
    ImVec2 m_ViewportSize = {0, 0};
};

} // namespace Yalaz::UI
