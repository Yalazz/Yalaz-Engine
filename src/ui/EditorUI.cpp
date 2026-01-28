// =============================================================================
// YALAZ ENGINE - Editor UI Implementation (Optimized)
// =============================================================================

#include "EditorUI.h"
#include "panels/SceneHierarchyPanel.h"
#include "panels/InspectorPanel.h"
#include "panels/ViewportPanel.h"
#include "panels/LightingPanel.h"
#include "panels/ConsolePanel.h"
#include "../vk_engine.h"

namespace Yalaz::UI {

void EditorUI::Init(VulkanEngine* engine) {
    m_Engine = engine;

    // Apply professional theme
    EditorTheme::Get().Apply();

    // Register all panels and cache pointers
    m_SceneHierarchy = PanelManager::Get().AddPanel<SceneHierarchyPanel>();
    m_Inspector = PanelManager::Get().AddPanel<InspectorPanel>();
    m_Viewport = PanelManager::Get().AddPanel<ViewportPanel>();
    m_Lighting = PanelManager::Get().AddPanel<LightingPanel>();
    m_Console = PanelManager::Get().AddPanel<ConsolePanel>();

    // Initialize panels
    PanelManager::Get().Init(engine);
}

void EditorUI::Shutdown() {
    PanelManager::Get().Shutdown();
}

void EditorUI::Render() {
    // Setup layout only when viewport changes
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    if (viewport->Size.x != m_LastViewportSize.x || viewport->Size.y != m_LastViewportSize.y) {
        m_LastViewportSize = viewport->Size;
        m_LayoutDirty = true;
    }

    if (m_LayoutDirty) {
        CalculateLayout();
    }

    // Render menu bar
    RenderMenuBar();

    // Apply cached positions and render panels directly
    RenderPanels();

    // Clear dirty flag after panels are rendered (so they get the forced positions)
    m_LayoutDirty = false;
}

void EditorUI::RenderMenuBar() {
    if (!m_Engine) return;

    if (ImGui::BeginMainMenuBar()) {
        // === FILE MENU ===
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Scene", "Ctrl+N")) {
                m_Engine->resetState();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Save Scene", "Ctrl+S")) {
                m_Engine->saveState("scene.json");
            }
            if (ImGui::MenuItem("Load Scene", "Ctrl+O")) {
                m_Engine->loadState("scene.json");
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "Alt+F4")) {
                // Will be handled by SDL
            }
            ImGui::EndMenu();
        }

        // === EDIT MENU ===
        if (ImGui::BeginMenu("Edit")) {
            // Delete selected object
            bool hasSelection = (m_Engine->selectedNode != nullptr) ||
                               (m_Engine->selectedPrimitiveIndex >= 0 &&
                                m_Engine->selectedPrimitiveIndex < static_cast<int>(m_Engine->static_shapes.size()));

            if (ImGui::MenuItem("Delete Selected", "Delete", false, hasSelection)) {
                if (m_Engine->selectedPrimitiveIndex >= 0 &&
                    m_Engine->selectedPrimitiveIndex < static_cast<int>(m_Engine->static_shapes.size())) {
                    m_Engine->static_shapes.erase(m_Engine->static_shapes.begin() + m_Engine->selectedPrimitiveIndex);
                    m_Engine->selectedPrimitiveIndex = -1;
                }
                if (m_Engine->selectedNode != nullptr) {
                    m_Engine->selectedNode = nullptr;
                    m_Engine->selectedObjectName.clear();
                }
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Reset Camera", "Home")) {
                m_Engine->mainCamera.position = glm::vec3(0.f, 5.f, 10.f);
                m_Engine->mainCamera.pitch = -0.3f;
                m_Engine->mainCamera.yaw = 0.f;
                m_Engine->mainCamera.targetPosition = m_Engine->mainCamera.position;
                m_Engine->mainCamera.targetPitch = m_Engine->mainCamera.pitch;
                m_Engine->mainCamera.targetYaw = m_Engine->mainCamera.yaw;
                m_Engine->mainCamera.focusActive = false;
                m_Engine->mainCamera.updateProjectionMatrix();
            }
            ImGui::EndMenu();
        }

        // === VIEW MENU ===
        if (ImGui::BeginMenu("View")) {
            if (m_SceneHierarchy) {
                bool open = m_SceneHierarchy->IsOpen();
                if (ImGui::MenuItem("Scene Hierarchy", nullptr, &open)) {
                    m_SceneHierarchy->SetOpen(open);
                }
            }
            if (m_Inspector) {
                bool open = m_Inspector->IsOpen();
                if (ImGui::MenuItem("Inspector", nullptr, &open)) {
                    m_Inspector->SetOpen(open);
                }
            }
            if (m_Viewport) {
                bool open = m_Viewport->IsOpen();
                if (ImGui::MenuItem("Viewport Settings", nullptr, &open)) {
                    m_Viewport->SetOpen(open);
                }
            }
            if (m_Lighting) {
                bool open = m_Lighting->IsOpen();
                if (ImGui::MenuItem("Lighting", nullptr, &open)) {
                    m_Lighting->SetOpen(open);
                }
            }
            if (m_Console) {
                bool open = m_Console->IsOpen();
                if (ImGui::MenuItem("Console", nullptr, &open)) {
                    m_Console->SetOpen(open);
                }
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Reset Layout")) {
                if (m_SceneHierarchy) m_SceneHierarchy->SetOpen(true);
                if (m_Inspector) m_Inspector->SetOpen(true);
                if (m_Viewport) m_Viewport->SetOpen(true);
                if (m_Lighting) m_Lighting->SetOpen(true);
                if (m_Console) m_Console->SetOpen(true);
                m_LayoutDirty = true;
            }
            ImGui::EndMenu();
        }

        // === RIGHT-ALIGNED INFO ===
        float rightOffset = ImGui::GetWindowWidth() - 200.0f;
        ImGui::SetCursorPosX(rightOffset);
        ImGui::Text("FPS: %.0f | %.2f ms", 1000.0f / m_Engine->stats.frametime, m_Engine->stats.frametime);

        ImGui::EndMainMenuBar();
    }

    // Keyboard shortcuts
    ImGuiIO& io = ImGui::GetIO();
    if (!io.WantTextInput) {
        // Ctrl shortcuts
        if (io.KeyCtrl) {
            if (ImGui::IsKeyPressed(ImGuiKey_S, false)) {
                m_Engine->saveState("scene.json");
            }
            if (ImGui::IsKeyPressed(ImGuiKey_O, false)) {
                m_Engine->loadState("scene.json");
            }
            if (ImGui::IsKeyPressed(ImGuiKey_N, false)) {
                m_Engine->resetState();
            }
        }

        // Delete key - delete selected
        if (ImGui::IsKeyPressed(ImGuiKey_Delete, false)) {
            if (m_Engine->selectedPrimitiveIndex >= 0 &&
                m_Engine->selectedPrimitiveIndex < static_cast<int>(m_Engine->static_shapes.size())) {
                m_Engine->static_shapes.erase(m_Engine->static_shapes.begin() + m_Engine->selectedPrimitiveIndex);
                m_Engine->selectedPrimitiveIndex = -1;
            }
            if (m_Engine->selectedNode != nullptr) {
                m_Engine->selectedNode = nullptr;
                m_Engine->selectedObjectName.clear();
            }
        }

        // Home key - reset camera
        if (ImGui::IsKeyPressed(ImGuiKey_Home, false)) {
            m_Engine->mainCamera.position = glm::vec3(0.f, 5.f, 10.f);
            m_Engine->mainCamera.pitch = -0.3f;
            m_Engine->mainCamera.yaw = 0.f;
            m_Engine->mainCamera.targetPosition = m_Engine->mainCamera.position;
            m_Engine->mainCamera.targetPitch = m_Engine->mainCamera.pitch;
            m_Engine->mainCamera.targetYaw = m_Engine->mainCamera.yaw;
            m_Engine->mainCamera.focusActive = false;
            m_Engine->mainCamera.updateProjectionMatrix();
        }
    }
}

void EditorUI::CalculateLayout() {
    float w = m_LastViewportSize.x;
    float h = m_LastViewportSize.y;
    float top = Layout::MenuBarHeight;
    float mainH = h - top - Layout::BottomPanelHeight;

    // Cache all positions and sizes
    m_LayoutCache[0] = {ImVec2(0, top), ImVec2(Layout::LeftPanelWidth, mainH)};  // SceneHierarchy
    m_LayoutCache[1] = {ImVec2(w - Layout::RightPanelWidth, top), ImVec2(Layout::RightPanelWidth, mainH * 0.5f)};  // Inspector
    m_LayoutCache[2] = {ImVec2(w - Layout::RightPanelWidth, top + mainH * 0.5f), ImVec2(Layout::RightPanelWidth, mainH * 0.25f)};  // Viewport
    m_LayoutCache[3] = {ImVec2(w - Layout::RightPanelWidth, top + mainH * 0.75f), ImVec2(Layout::RightPanelWidth, mainH * 0.25f)};  // Lighting
    m_LayoutCache[4] = {ImVec2(0, h - Layout::BottomPanelHeight), ImVec2(w, Layout::BottomPanelHeight)};  // Console

    m_ViewportPos = ImVec2(Layout::LeftPanelWidth, top);
    m_ViewportSize = ImVec2(w - Layout::LeftPanelWidth - Layout::RightPanelWidth, mainH);
}

void EditorUI::RenderPanels() {
    // Use FirstUseEver to set initial positions, but allow user to move/resize afterward
    ImGuiCond layoutCond = m_LayoutDirty ? ImGuiCond_Always : ImGuiCond_FirstUseEver;

    if (m_SceneHierarchy && m_SceneHierarchy->IsOpen()) {
        ImGui::SetNextWindowPos(m_LayoutCache[0].pos, layoutCond);
        ImGui::SetNextWindowSize(m_LayoutCache[0].size, layoutCond);
        m_SceneHierarchy->OnRender();
    }

    if (m_Inspector && m_Inspector->IsOpen()) {
        ImGui::SetNextWindowPos(m_LayoutCache[1].pos, layoutCond);
        ImGui::SetNextWindowSize(m_LayoutCache[1].size, layoutCond);
        m_Inspector->OnRender();
    }

    if (m_Viewport && m_Viewport->IsOpen()) {
        ImGui::SetNextWindowPos(m_LayoutCache[2].pos, layoutCond);
        ImGui::SetNextWindowSize(m_LayoutCache[2].size, layoutCond);
        m_Viewport->OnRender();
    }

    if (m_Lighting && m_Lighting->IsOpen()) {
        ImGui::SetNextWindowPos(m_LayoutCache[3].pos, layoutCond);
        ImGui::SetNextWindowSize(m_LayoutCache[3].size, layoutCond);
        m_Lighting->OnRender();
    }

    if (m_Console && m_Console->IsOpen()) {
        ImGui::SetNextWindowPos(m_LayoutCache[4].pos, layoutCond);
        ImGui::SetNextWindowSize(m_LayoutCache[4].size, layoutCond);
        m_Console->OnRender();
    }
}

} // namespace Yalaz::UI
