#pragma once
// =============================================================================
// YALAZ ENGINE - View Manager
// =============================================================================
// Professional view management with:
// - Type-safe view registration and creation
// - Singleton pattern for global access
// - Category-based menu organization
// - Dynamic view type registry
// =============================================================================

#include "EditorView.h"
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include <string>

class VulkanEngine;

namespace Yalaz::UI {

// =============================================================================
// View Type Info - Metadata about registered view types
// =============================================================================
struct ViewTypeInfo {
    std::string typeId;
    std::string displayName;
    std::string shortcut;
    bool isSingleton = true;
    std::function<std::unique_ptr<EditorView>()> factory;
};

// =============================================================================
// View Manager - Centralized view management
// =============================================================================
class ViewManager {
public:
    static ViewManager& Get() {
        static ViewManager instance;
        return instance;
    }

    // ==========================================================================
    // Lifecycle
    // ==========================================================================
    void Init(VulkanEngine* engine);
    void Shutdown();
    void Update(float deltaTime);
    void Render();

    // ==========================================================================
    // View Type Registration (template-based)
    // ==========================================================================
    template<typename T>
    void RegisterViewType(const std::string& typeId,
                          const std::string& displayName,
                          const std::string& shortcut = "",
                          bool isSingleton = true) {
        ViewTypeInfo info;
        info.typeId = typeId;
        info.displayName = displayName;
        info.shortcut = shortcut;
        info.isSingleton = isSingleton;
        info.factory = []() { return std::make_unique<T>(); };
        m_ViewTypes[typeId] = info;
    }

    // ==========================================================================
    // View Creation and Access
    // ==========================================================================

    // Create a view by type ID
    EditorView* CreateView(const std::string& typeId);

    // Get view by type (template version)
    template<typename T>
    T* GetView() {
        for (auto& view : m_Views) {
            if (auto* v = dynamic_cast<T*>(view.get())) {
                return v;
            }
        }
        return nullptr;
    }

    // Alias for GetView (compatibility)
    template<typename T>
    T* GetViewOfType() {
        return GetView<T>();
    }

    // Get all views
    const std::vector<std::unique_ptr<EditorView>>& GetViews() const { return m_Views; }

    // Get views by category
    std::vector<EditorView*> GetViewsByCategory(ViewCategory category);

    // Get view type info
    const ViewTypeInfo* GetViewTypeInfo(const std::string& typeId) const;

    // Get all registered type IDs
    std::vector<std::string> GetRegisteredTypeIds() const;

    // ==========================================================================
    // UI Rendering
    // ==========================================================================

    // Render the View menu (for main menu bar)
    void RenderViewMenu();

    // Show/hide a view by type
    void ShowView(const std::string& typeId);
    void HideView(const std::string& typeId);
    void ToggleView(const std::string& typeId);

private:
    ViewManager() = default;
    ~ViewManager() = default;
    ViewManager(const ViewManager&) = delete;
    ViewManager& operator=(const ViewManager&) = delete;

    void CreateAllViews();

    std::vector<std::unique_ptr<EditorView>> m_Views;
    std::unordered_map<std::string, ViewTypeInfo> m_ViewTypes;
    VulkanEngine* m_Engine = nullptr;
};

} // namespace Yalaz::UI
