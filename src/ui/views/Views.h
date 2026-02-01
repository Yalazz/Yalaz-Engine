#pragma once
// =============================================================================
// YALAZ ENGINE - Views Master Header
// =============================================================================
// Includes all view classes and provides registration functions
// Complete professional view system with all phases implemented
// =============================================================================

// Base classes
#include "EditorView.h"
#include "ViewManager.h"

// Phase 1 Views - Core Editor
#include "SceneView.h"
#include "GameView.h"
#include "HierarchyView.h"
#include "ObjectInspectorView.h"
#include "AssetBrowserView.h"
#include "ConsoleView.h"

// Phase 2 Views - Graphics Tools
#include "ProfilerView.h"
#include "MaterialView.h"
#include "TextureView.h"
#include "UVView.h"
#include "ShaderDebugView.h"

// Phase 3 Views - Advanced Debug & Production
#include "GPUDebugView.h"
#include "LightingDebugView.h"
#include "AnimationView.h"
#include "SettingsView.h"
#include "PluginManagerView.h"
#include "PhysicsDebugView.h"
#include "CameraSequencerView.h"

namespace Yalaz::UI {

// =============================================================================
// Register all built-in view types with the ViewManager
// =============================================================================
inline void RegisterAllViewTypes() {
    auto& vm = ViewManager::Get();

    // Phase 1 - Core Editor Views
    vm.RegisterViewType<SceneView>("SceneView", "Scene", "S", true);
    vm.RegisterViewType<GameView>("GameView", "Game", "G", true);
    vm.RegisterViewType<HierarchyView>("HierarchyView", "Hierarchy", "H", true);
    vm.RegisterViewType<ObjectInspectorView>("InspectorView", "Inspector", "I", true);
    vm.RegisterViewType<AssetBrowserView>("AssetBrowserView", "Asset Browser", "A", true);
    vm.RegisterViewType<ConsoleView>("ConsoleView", "Console", "C", true);

    // Phase 2 - Graphics Tools
    vm.RegisterViewType<ProfilerView>("ProfilerView", "Profiler", "P", true);
    vm.RegisterViewType<MaterialView>("MaterialView", "Material Editor", "M", false);
    vm.RegisterViewType<TextureView>("TextureView", "Texture Inspector", "T", false);
    vm.RegisterViewType<UVView>("UVView", "UV Editor", "U", false);
    vm.RegisterViewType<ShaderDebugView>("ShaderDebugView", "Shader Debug", "D", true);

    // Phase 3 - Advanced Debug & Production
    vm.RegisterViewType<GPUDebugView>("GPUDebugView", "GPU Debug", "G", true);
    vm.RegisterViewType<LightingDebugView>("LightingDebugView", "Lighting Debug", "L", true);
    vm.RegisterViewType<AnimationView>("AnimationView", "Animation", "N", true);
    vm.RegisterViewType<SettingsView>("SettingsView", "Settings", "O", true);
    vm.RegisterViewType<PluginManagerView>("PluginManagerView", "Plugins", "K", true);
    vm.RegisterViewType<PhysicsDebugView>("PhysicsDebugView", "Physics Debug", "P", true);
    vm.RegisterViewType<CameraSequencerView>("CameraSequencerView", "Camera Sequencer", "Q", true);
}

// =============================================================================
// Create default views for a new editor session
// =============================================================================
inline void CreateDefaultViews() {
    auto& vm = ViewManager::Get();

    // Create core singleton views
    vm.CreateView("SceneView");
    vm.CreateView("HierarchyView");
    vm.CreateView("InspectorView");
    vm.CreateView("ConsoleView");
    vm.CreateView("AssetBrowserView");
}

// =============================================================================
// Create all available views (for testing/development)
// =============================================================================
inline void CreateAllViews() {
    auto& vm = ViewManager::Get();

    // Phase 1
    vm.CreateView("SceneView");
    vm.CreateView("GameView");
    vm.CreateView("HierarchyView");
    vm.CreateView("InspectorView");
    vm.CreateView("AssetBrowserView");
    vm.CreateView("ConsoleView");

    // Phase 2
    vm.CreateView("ProfilerView");
    vm.CreateView("MaterialView");
    vm.CreateView("TextureView");
    vm.CreateView("UVView");
    vm.CreateView("ShaderDebugView");

    // Phase 3
    vm.CreateView("GPUDebugView");
    vm.CreateView("LightingDebugView");
    vm.CreateView("AnimationView");
    vm.CreateView("SettingsView");
    vm.CreateView("PluginManagerView");
    vm.CreateView("PhysicsDebugView");
    vm.CreateView("CameraSequencerView");
}

// =============================================================================
// View System Info
// =============================================================================
struct ViewSystemInfo {
    static constexpr const char* Version = "2.1.0";

    static constexpr int Phase1ViewCount = 6;
    static constexpr int Phase2ViewCount = 5;
    static constexpr int Phase3ViewCount = 7;  // +2 (Physics Debug, Camera Sequencer)
    static constexpr int TotalViewCount = Phase1ViewCount + Phase2ViewCount + Phase3ViewCount;  // 18 total

    static constexpr const char* Phase1Views[] = {
        "Scene View", "Game View", "Hierarchy", "Inspector", "Asset Browser", "Console"
    };

    static constexpr const char* Phase2Views[] = {
        "Profiler", "Material Editor", "Texture Inspector", "UV Editor", "Shader Debug"
    };

    static constexpr const char* Phase3Views[] = {
        "GPU Debug", "Lighting Debug", "Animation", "Settings", "Plugin Manager",
        "Physics Debug", "Camera Sequencer"
    };

    // Feature matrix
    static constexpr bool HasSceneView = true;
    static constexpr bool HasGameView = true;
    static constexpr bool HasMaterialEditor = true;
    static constexpr bool HasProfiler = true;
    static constexpr bool HasGPUDebug = true;
    static constexpr bool HasAnimationEditor = true;
    static constexpr bool HasPluginManager = true;
    static constexpr bool HasPhysicsDebug = true;
    static constexpr bool HasCameraSequencer = true;
};

// =============================================================================
// View Category Icons
// =============================================================================
inline const char* GetCategoryIcon(ViewCategory category) {
    switch (category) {
        case ViewCategory::Core: return "[C]";
        case ViewCategory::Scene: return "[S]";
        case ViewCategory::Layout: return "[L]";
        case ViewCategory::Assets: return "[A]";
        case ViewCategory::Debug: return "[D]";
        case ViewCategory::Animation: return "[N]";
        case ViewCategory::Rendering: return "[R]";
        case ViewCategory::Graphics: return "[G]";
        case ViewCategory::System: return "[Y]";
        default: return "[?]";
    }
}

} // namespace Yalaz::UI
