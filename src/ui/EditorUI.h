#pragma once
// =============================================================================
// YALAZ ENGINE - Editor UI Controller
// =============================================================================
// Main editor UI system using the new professional view architecture
// Features:
// - Workspace layout presets (multiple design types)
// - Save/Load workspace configurations
// - Professional view management
// =============================================================================

#include <imgui.h>
#include <string>
#include <vector>
#include <unordered_map>
#include "EditorTheme.h"

// New View System
#include "views/Views.h"
#include "views/ViewSystemIntegration.h"

class VulkanEngine;

namespace Yalaz::UI {

// =============================================================================
// Workspace Layout - Save/Load view configurations
// =============================================================================
struct ViewLayoutState {
    std::string viewName;
    bool isOpen = true;
    ImVec2 position = {0, 0};
    ImVec2 size = {400, 300};
};

struct WorkspaceLayout {
    std::string name;
    std::string description;
    std::string icon;
    bool isBuiltIn = false;
    std::vector<ViewLayoutState> viewStates;
};

// =============================================================================
// Editor UI Controller
// =============================================================================
class EditorUI {
public:
    static EditorUI& Get() {
        static EditorUI instance;
        return instance;
    }

    void Init(VulkanEngine* engine);
    void Shutdown();
    void Render();
    void Update(float deltaTime);

    // Workspace Management
    void SaveWorkspace(const std::string& name, const std::string& description = "");
    void LoadWorkspace(const std::string& name);
    void DeleteWorkspace(const std::string& name);
    const std::vector<WorkspaceLayout>& GetWorkspaces() const { return m_Workspaces; }
    const std::string& GetCurrentWorkspaceName() const { return m_CurrentWorkspaceName; }

    // View shortcuts
    void ShowView(const std::string& viewName);
    void HideView(const std::string& viewName);
    void ToggleView(const std::string& viewName);

    // Layout control
    void ForceLayoutRecalc() { m_ForceLayoutRecalc = true; }
    bool IsLayoutLocked() const { return m_LayoutLocked; }
    void SetLayoutLocked(bool locked) { m_LayoutLocked = locked; if (locked) m_ForceLayoutRecalc = true; }

private:
    EditorUI() = default;
    ~EditorUI() = default;
    EditorUI(const EditorUI&) = delete;
    EditorUI& operator=(const EditorUI&) = delete;

    void SetupDockspace();
    void CalculateLayout();
    void RenderMenuBar();
    void RenderViewMenu();
    void RenderWorkspaceMenu();
    void RenderWorkspaceWindow();

    // Workspace management
    void InitBuiltInWorkspaces();
    void LoadWorkspacesFromFile();
    void SaveWorkspacesToFile();
    void ApplyWorkspace(const WorkspaceLayout& layout);
    WorkspaceLayout CaptureCurrentWorkspace(const std::string& name);

    VulkanEngine* m_Engine = nullptr;

    // Workspace system
    std::vector<WorkspaceLayout> m_Workspaces;
    std::string m_CurrentWorkspaceName = "Default";
    bool m_ShowWorkspaceWindow = false;
    bool m_ShowSaveWorkspacePopup = false;
    int m_SelectedWorkspaceIndex = -1;
    char m_NewWorkspaceName[64] = "";
    char m_NewWorkspaceDescription[256] = "";

    // Docking
    bool m_DockspaceInitialized = false;

    // Dynamic layout tracking
    float m_LastViewportWidth = 0.0f;
    float m_LastViewportHeight = 0.0f;
    bool m_ForceLayoutRecalc = true;  // Force recalc on startup
    bool m_LayoutLocked = true;       // When true, panels auto-position; when false, panels are freely movable
    std::unordered_map<std::string, bool> m_PreviousOpenStates;
};

} // namespace Yalaz::UI
