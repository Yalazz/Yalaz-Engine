#pragma once
// =============================================================================
// YALAZ ENGINE - Editor UI Controller (Optimized)
// =============================================================================
// Main editor UI system - handles layout, menu bar, and panel coordination
// Includes layout preset system for saving/loading UI configurations
// =============================================================================

#include <imgui.h>
#include <string>
#include <vector>
#include <unordered_map>
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

// Layout constants (defaults)
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

// Panel state for presets
struct PanelState {
    bool isOpen = true;
    float widthRatio = 1.0f;   // Ratio of screen width
    float heightRatio = 1.0f;  // Ratio of screen height
    float xRatio = 0.0f;       // Position as ratio
    float yRatio = 0.0f;
};

// Layout preset structure
struct LayoutPreset {
    std::string name;
    std::string icon;  // Unicode icon for display
    std::string description;
    bool isBuiltIn = false;

    // Panel states (using ratios for resolution independence)
    float leftPanelWidth = Layout::LeftPanelWidth;
    float rightPanelWidth = Layout::RightPanelWidth;
    float bottomPanelHeight = Layout::BottomPanelHeight;

    // Panel visibility
    bool sceneHierarchyOpen = true;
    bool inspectorOpen = true;
    bool viewportOpen = true;
    bool lightingOpen = true;
    bool consoleOpen = true;

    // Inspector/Viewport/Lighting height ratios (of main area)
    float inspectorHeightRatio = 0.5f;
    float viewportHeightRatio = 0.25f;
    float lightingHeightRatio = 0.25f;
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

    // Preset management
    void SavePreset(const std::string& name);
    void LoadPreset(const std::string& name);
    void DeletePreset(const std::string& name);
    const std::vector<LayoutPreset>& GetPresets() const { return m_Presets; }
    const std::string& GetCurrentPresetName() const { return m_CurrentPresetName; }

private:
    EditorUI() = default;
    ~EditorUI() = default;
    EditorUI(const EditorUI&) = delete;
    EditorUI& operator=(const EditorUI&) = delete;

    void CalculateLayout();
    void RenderMenuBar();
    void RenderPanels();
    void RenderLayoutPresetWindow();

    // Preset helpers
    void InitBuiltInPresets();
    void LoadPresetsFromFile();
    void SavePresetsToFile();
    void ApplyPreset(const LayoutPreset& preset);
    LayoutPreset CaptureCurrentLayout(const std::string& name);

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

    // Current layout settings (can be modified by presets)
    float m_LeftPanelWidth = Layout::LeftPanelWidth;
    float m_RightPanelWidth = Layout::RightPanelWidth;
    float m_BottomPanelHeight = Layout::BottomPanelHeight;
    float m_InspectorHeightRatio = 0.5f;
    float m_ViewportHeightRatio = 0.25f;
    float m_LightingHeightRatio = 0.25f;

    // Preset system
    std::vector<LayoutPreset> m_Presets;
    std::string m_CurrentPresetName = "Default";
    bool m_ShowPresetWindow = false;
    bool m_ShowSavePresetPopup = false;
    char m_NewPresetName[64] = "";
    char m_NewPresetDescription[256] = "";
    int m_SelectedPresetIndex = -1;
};

} // namespace Yalaz::UI
