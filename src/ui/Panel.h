#pragma once
// =============================================================================
// YALAZ ENGINE - Panel Base Class
// =============================================================================
// Abstract base class for all editor panels
// Provides common interface and helper functions
// =============================================================================

#include <string>
#include <imgui.h>

// Forward declaration
class VulkanEngine;

namespace Yalaz::UI {

// Panel base class - all editor panels inherit from this
class Panel {
public:
    explicit Panel(const std::string& name)
        : m_Name(name), m_IsOpen(true), m_Engine(nullptr) {}

    virtual ~Panel() = default;

    // Lifecycle methods
    virtual void OnInit(VulkanEngine* engine) { m_Engine = engine; }
    virtual void OnShutdown() {}
    virtual void OnUpdate(float deltaTime) {}
    virtual void OnRender() = 0;  // Pure virtual - must be implemented

    // Accessors
    const std::string& GetName() const { return m_Name; }
    bool IsOpen() const { return m_IsOpen; }
    void SetOpen(bool open) { m_IsOpen = open; }
    void ToggleOpen() { m_IsOpen = !m_IsOpen; }

    // Position tracking for snapping
    ImVec2 GetLastPos() const { return m_LastPos; }
    ImVec2 GetLastSize() const { return m_LastSize; }
    bool IsBeingDragged() const { return m_IsBeingDragged; }

protected:
    std::string m_Name;
    bool m_IsOpen;
    VulkanEngine* m_Engine;

    // Position tracking
    ImVec2 m_LastPos = {0, 0};
    ImVec2 m_LastSize = {0, 0};
    bool m_IsBeingDragged = false;

    // Helper: Begin panel window with consistent styling
    bool BeginPanel(ImGuiWindowFlags extraFlags = 0) {
        if (!m_IsOpen) return false;
        bool result = ImGui::Begin(m_Name.c_str(), &m_IsOpen, extraFlags);
        // Track position after window is created
        m_LastPos = ImGui::GetWindowPos();
        m_LastSize = ImGui::GetWindowSize();
        // Check if window is being dragged (title bar hovered + mouse down)
        m_IsBeingDragged = ImGui::IsWindowFocused() && ImGui::IsMouseDragging(0);
        return result;
    }

    void EndPanel() {
        // Update position before closing
        m_LastPos = ImGui::GetWindowPos();
        m_LastSize = ImGui::GetWindowSize();
        ImGui::End();
    }
};

} // namespace Yalaz::UI
