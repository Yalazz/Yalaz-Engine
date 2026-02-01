// =============================================================================
// YALAZ ENGINE - View Manager Implementation
// =============================================================================

#include "ViewManager.h"
#include "../EditorTheme.h"
#include "../EditorUI.h"
#include "../../vk_engine.h"

// Include all views
#include "SceneView.h"
#include "GameView.h"
#include "HierarchyView.h"
#include "ObjectInspectorView.h"
#include "AssetBrowserView.h"
#include "ConsoleView.h"
#include "MaterialView.h"
#include "TextureView.h"
#include "UVView.h"
#include "ProfilerView.h"
#include "ShaderDebugView.h"
#include "GPUDebugView.h"
#include "LightingDebugView.h"
#include "AnimationView.h"
#include "SettingsView.h"
#include "PluginManagerView.h"
#include "PhysicsDebugView.h"
#include "CameraSequencerView.h"

namespace Yalaz::UI {

void ViewManager::Init(VulkanEngine* engine) {
    m_Engine = engine;
    CreateAllViews();

    // Initialize all views
    for (auto& view : m_Views) {
        view->OnInit(m_Engine);
    }

    // Open only core views by default
    for (auto& view : m_Views) {
        const std::string& name = view->GetName();
        if (name == "Hierarchy" || name == "Inspector" ||
            name == "Console" || name == "Asset Browser" || name == "Scene") {
            view->SetOpen(true);
        }
    }
}

void ViewManager::Shutdown() {
    for (auto& view : m_Views) {
        view->OnShutdown();
    }
    m_Views.clear();
    m_ViewTypes.clear();
}

void ViewManager::Update(float deltaTime) {
    for (auto& view : m_Views) {
        if (view->IsOpen()) {
            view->OnUpdate(deltaTime);
        }
    }
}

void ViewManager::Render() {
    for (auto& view : m_Views) {
        if (view->IsOpen()) {
            view->OnRender();
        }
    }
}

EditorView* ViewManager::CreateView(const std::string& typeId) {
    auto it = m_ViewTypes.find(typeId);
    if (it == m_ViewTypes.end()) {
        return nullptr;
    }

    // Check if singleton already exists
    if (it->second.isSingleton) {
        for (auto& view : m_Views) {
            if (view->GetName() == it->second.displayName) {
                return view.get();
            }
        }
    }

    // Create new view
    auto view = it->second.factory();
    if (view) {
        view->OnInit(m_Engine);
        EditorView* rawPtr = view.get();
        m_Views.push_back(std::move(view));
        return rawPtr;
    }

    return nullptr;
}

std::vector<EditorView*> ViewManager::GetViewsByCategory(ViewCategory category) {
    std::vector<EditorView*> result;
    for (auto& view : m_Views) {
        if (view->GetCategory() == category) {
            result.push_back(view.get());
        }
    }
    return result;
}

const ViewTypeInfo* ViewManager::GetViewTypeInfo(const std::string& typeId) const {
    auto it = m_ViewTypes.find(typeId);
    if (it != m_ViewTypes.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<std::string> ViewManager::GetRegisteredTypeIds() const {
    std::vector<std::string> result;
    for (const auto& pair : m_ViewTypes) {
        result.push_back(pair.first);
    }
    return result;
}

void ViewManager::ShowView(const std::string& typeId) {
    for (auto& view : m_Views) {
        if (view->GetName() == typeId) {
            view->SetOpen(true);
            return;
        }
    }
    // Try to create if not found
    if (auto* view = CreateView(typeId)) {
        view->SetOpen(true);
    }
}

void ViewManager::HideView(const std::string& typeId) {
    for (auto& view : m_Views) {
        if (view->GetName() == typeId) {
            view->SetOpen(false);
            return;
        }
    }
}

void ViewManager::ToggleView(const std::string& typeId) {
    for (auto& view : m_Views) {
        if (view->GetName() == typeId) {
            view->ToggleOpen();
            return;
        }
    }
}

void ViewManager::CreateAllViews() {
    // Phase 1 - Core Editor
    m_Views.push_back(std::make_unique<SceneView>());
    m_Views.push_back(std::make_unique<GameView>());
    m_Views.push_back(std::make_unique<HierarchyView>());
    m_Views.push_back(std::make_unique<ObjectInspectorView>());
    m_Views.push_back(std::make_unique<AssetBrowserView>());
    m_Views.push_back(std::make_unique<ConsoleView>());

    // Phase 2 - Graphics Tools
    m_Views.push_back(std::make_unique<MaterialView>());
    m_Views.push_back(std::make_unique<TextureView>());
    m_Views.push_back(std::make_unique<UVView>());
    m_Views.push_back(std::make_unique<ProfilerView>());
    m_Views.push_back(std::make_unique<ShaderDebugView>());

    // Phase 3 - Advanced Debug
    m_Views.push_back(std::make_unique<GPUDebugView>());
    m_Views.push_back(std::make_unique<LightingDebugView>());
    m_Views.push_back(std::make_unique<AnimationView>());
    m_Views.push_back(std::make_unique<SettingsView>());
    m_Views.push_back(std::make_unique<PluginManagerView>());
    m_Views.push_back(std::make_unique<PhysicsDebugView>());
    m_Views.push_back(std::make_unique<CameraSequencerView>());
}

void ViewManager::RenderViewMenu() {
    // Helper lambda to render menu for a category
    auto renderCategoryMenu = [this](const char* name, ViewCategory category) {
        if (ImGui::BeginMenu(name)) {
            for (auto& view : m_Views) {
                if (view->GetCategory() == category) {
                    bool isOpen = view->IsOpen();
                    std::string label = view->GetIcon() + "  " + view->GetName();
                    if (ImGui::MenuItem(label.c_str(), nullptr, &isOpen)) {
                        view->SetOpen(isOpen);
                    }
                }
            }
            ImGui::EndMenu();
        }
    };

    // Core views
    renderCategoryMenu("Core", ViewCategory::Core);

    // Assets views
    renderCategoryMenu("Assets", ViewCategory::Assets);

    // Graphics views
    renderCategoryMenu("Graphics", ViewCategory::Graphics);

    // Debug views
    renderCategoryMenu("Debug", ViewCategory::Debug);

    // Animation views
    renderCategoryMenu("Animation", ViewCategory::Animation);

    // System views
    renderCategoryMenu("System", ViewCategory::System);

    ImGui::Separator();

    // Layout lock toggle
    bool isLocked = EditorUI::Get().IsLayoutLocked();
    if (ImGui::MenuItem(isLocked ? "Unlock Layout (Allow Moving)" : "Lock Layout (Auto-Arrange)", nullptr, isLocked)) {
        EditorUI::Get().SetLayoutLocked(!isLocked);
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(isLocked ?
            "Click to unlock: Panels can be freely moved" :
            "Click to lock: Panels auto-arrange and resize with window");
    }

    ImGui::Separator();

    // Quick access
    if (ImGui::MenuItem("Show All")) {
        for (auto& view : m_Views) {
            view->SetOpen(true);
            view->ResetLayout();  // Reset layout so positions are recalculated
        }
        EditorUI::Get().ForceLayoutRecalc();
    }

    if (ImGui::MenuItem("Hide All")) {
        for (auto& view : m_Views) {
            view->SetOpen(false);
            view->ClearDynamicLayout();
        }
    }

    if (ImGui::MenuItem("Reset Layout")) {
        // Reset to default layout
        for (auto& view : m_Views) {
            view->SetOpen(false);
            view->ResetLayout();
            view->ClearDynamicLayout();
        }
        // Open default views
        for (auto& view : m_Views) {
            auto cat = view->GetCategory();
            if (cat == ViewCategory::Core || cat == ViewCategory::Assets) {
                view->SetOpen(true);
            }
        }
        // Always show console
        if (auto* console = GetView<ConsoleView>()) {
            console->SetOpen(true);
        }
        EditorUI::Get().ForceLayoutRecalc();
    }
}

} // namespace Yalaz::UI
