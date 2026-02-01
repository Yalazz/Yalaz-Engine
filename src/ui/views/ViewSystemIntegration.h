#pragma once
// =============================================================================
// YALAZ ENGINE - View System Integration
// =============================================================================
// Provides convenient access to the view system
// =============================================================================

#include "Views.h"

namespace Yalaz::UI {

// =============================================================================
// View System Integration - convenient access to views
// =============================================================================
class ViewSystemIntegration {
public:
    static ViewSystemIntegration& Get() {
        static ViewSystemIntegration instance;
        return instance;
    }

    // Initialize the view system
    void Init(VulkanEngine* engine) {
        m_Engine = engine;

        // Register all view types
        RegisterAllViewTypes();

        // Initialize view manager
        ViewManager::Get().Init(engine);

        m_Initialized = true;
    }

    void Shutdown() {
        ViewManager::Get().Shutdown();
    }

    void Update(float deltaTime) {
        ViewManager::Get().Update(deltaTime);
    }

    void Render() {
        ViewManager::Get().Render();
    }

    bool IsInitialized() const { return m_Initialized; }

    // Convenience accessors for common views
    SceneView* GetSceneView() { return ViewManager::Get().GetView<SceneView>(); }
    GameView* GetGameView() { return ViewManager::Get().GetView<GameView>(); }
    HierarchyView* GetHierarchyView() { return ViewManager::Get().GetView<HierarchyView>(); }
    ObjectInspectorView* GetInspectorView() { return ViewManager::Get().GetView<ObjectInspectorView>(); }
    AssetBrowserView* GetAssetBrowserView() { return ViewManager::Get().GetView<AssetBrowserView>(); }
    ConsoleView* GetConsoleView() { return ViewManager::Get().GetView<ConsoleView>(); }
    ProfilerView* GetProfilerView() { return ViewManager::Get().GetView<ProfilerView>(); }

private:
    ViewSystemIntegration() = default;
    ~ViewSystemIntegration() = default;
    ViewSystemIntegration(const ViewSystemIntegration&) = delete;
    ViewSystemIntegration& operator=(const ViewSystemIntegration&) = delete;

    VulkanEngine* m_Engine = nullptr;
    bool m_Initialized = false;
};

// =============================================================================
// Helper Macros for Easy Access
// =============================================================================
#define VIEWS ViewSystemIntegration::Get()
#define VIEW_MANAGER ViewManager::Get()
#define SCENE_VIEW ViewSystemIntegration::Get().GetSceneView()
#define GAME_VIEW ViewSystemIntegration::Get().GetGameView()
#define HIERARCHY_VIEW ViewSystemIntegration::Get().GetHierarchyView()
#define INSPECTOR_VIEW ViewSystemIntegration::Get().GetInspectorView()
#define ASSET_BROWSER_VIEW ViewSystemIntegration::Get().GetAssetBrowserView()
#define CONSOLE_VIEW ViewSystemIntegration::Get().GetConsoleView()
#define PROFILER_VIEW ViewSystemIntegration::Get().GetProfilerView()

} // namespace Yalaz::UI
