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

protected:
    std::string m_Name;
    bool m_IsOpen;
    VulkanEngine* m_Engine;

    // Helper: Begin panel window with consistent styling
    bool BeginPanel(ImGuiWindowFlags extraFlags = 0) {
        if (!m_IsOpen) return false;
        return ImGui::Begin(m_Name.c_str(), &m_IsOpen, extraFlags);
    }

    void EndPanel() {
        ImGui::End();
    }
};

} // namespace Yalaz::UI
