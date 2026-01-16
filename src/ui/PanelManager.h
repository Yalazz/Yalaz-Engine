#pragma once
// =============================================================================
// YALAZ ENGINE - Panel Manager
// =============================================================================
// Manages all editor panels - registration, lifecycle, and rendering
// =============================================================================

#include <vector>
#include <memory>
#include <unordered_map>
#include <string>
#include "Panel.h"

class VulkanEngine;

namespace Yalaz::UI {

class PanelManager {
public:
    static PanelManager& Get() {
        static PanelManager instance;
        return instance;
    }

    // Initialize all panels
    void Init(VulkanEngine* engine) {
        m_Engine = engine;
        for (auto& panel : m_Panels) {
            panel->OnInit(engine);
        }
    }

    // Shutdown all panels
    void Shutdown() {
        for (auto& panel : m_Panels) {
            panel->OnShutdown();
        }
        m_Panels.clear();
        m_PanelMap.clear();
    }

    // Update all panels
    void Update(float deltaTime) {
        for (auto& panel : m_Panels) {
            panel->OnUpdate(deltaTime);
        }
    }

    // Render all open panels
    void RenderAll() {
        for (auto& panel : m_Panels) {
            if (panel->IsOpen()) {
                panel->OnRender();
            }
        }
    }

    // Register a new panel
    template<typename T, typename... Args>
    T* AddPanel(Args&&... args) {
        auto panel = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = panel.get();

        // Initialize if engine is already set
        if (m_Engine) {
            panel->OnInit(m_Engine);
        }

        m_PanelMap[panel->GetName()] = ptr;
        m_Panels.push_back(std::move(panel));
        return ptr;
    }

    // Get panel by name
    Panel* GetPanel(const std::string& name) {
        auto it = m_PanelMap.find(name);
        return (it != m_PanelMap.end()) ? it->second : nullptr;
    }

    // Get panel by type
    template<typename T>
    T* GetPanelOfType() {
        for (auto& panel : m_Panels) {
            if (T* p = dynamic_cast<T*>(panel.get())) {
                return p;
            }
        }
        return nullptr;
    }

    // Get all panels
    const std::vector<std::unique_ptr<Panel>>& GetPanels() const {
        return m_Panels;
    }

    // Toggle panel visibility
    void TogglePanel(const std::string& name) {
        if (Panel* p = GetPanel(name)) {
            p->ToggleOpen();
        }
    }

    // Show panel
    void ShowPanel(const std::string& name) {
        if (Panel* p = GetPanel(name)) {
            p->SetOpen(true);
        }
    }

    // Hide panel
    void HidePanel(const std::string& name) {
        if (Panel* p = GetPanel(name)) {
            p->SetOpen(false);
        }
    }

private:
    PanelManager() = default;
    ~PanelManager() = default;
    PanelManager(const PanelManager&) = delete;
    PanelManager& operator=(const PanelManager&) = delete;

    std::vector<std::unique_ptr<Panel>> m_Panels;
    std::unordered_map<std::string, Panel*> m_PanelMap;
    VulkanEngine* m_Engine = nullptr;
};

} // namespace Yalaz::UI
