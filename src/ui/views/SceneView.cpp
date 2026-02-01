// =============================================================================
// YALAZ ENGINE - Scene View Implementation
// =============================================================================
// Professional viewport toolbar and overlays
// =============================================================================

#include "SceneView.h"
#include "../../vk_engine.h"
#include "../EditorTheme.h"
#include <algorithm>
#include <cmath>

namespace Yalaz::UI {

// Layout constants
static const float MENU_BAR_HEIGHT = 25.0f;
static const float TOOLBAR_HEIGHT = 35.0f;

// Helper function to calculate dynamic panel widths
static void GetDynamicLayout(float vw, float& leftW, float& rightW) {
    leftW = std::max(200.0f, vw * 0.15f);   // ~15% of width, min 200
    rightW = std::max(280.0f, vw * 0.20f);  // ~20% of width, min 280
}

void SceneView::OnUpdate(float deltaTime) {
    // Update FPS stats from engine
    if (m_Engine) {
        m_FrameTime = m_Engine->stats.frametime;
        if (m_FrameTime > 0.001f) {
            m_Fps = 1000.0f / m_FrameTime;
        }

        m_FpsHistory[m_FpsIndex] = m_Fps;
        m_FpsIndex = (m_FpsIndex + 1) % 60;
    }
}

void SceneView::OnRender() {
    if (!m_IsOpen || !m_Engine) return;

    // Get viewport dimensions
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    float vw = viewport->WorkSize.x;
    float vh = viewport->WorkSize.y;

    // Calculate dynamic panel widths based on viewport
    float leftW, rightW;
    GetDynamicLayout(vw, leftW, rightW);

    // Calculate toolbar dimensions (centered between left and right panels)
    float toolbarX = viewport->WorkPos.x + leftW;
    float toolbarY = viewport->WorkPos.y + MENU_BAR_HEIGHT;
    float toolbarWidth = vw - leftW - rightW;

    // Render toolbar bar at top of viewport
    ImGui::SetNextWindowPos(ImVec2(toolbarX, toolbarY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(toolbarWidth, TOOLBAR_HEIGHT), ImGuiCond_Always);

    ImGuiWindowFlags toolbarFlags = ImGuiWindowFlags_NoTitleBar |
                                    ImGuiWindowFlags_NoResize |
                                    ImGuiWindowFlags_NoMove |
                                    ImGuiWindowFlags_NoScrollbar |
                                    ImGuiWindowFlags_NoCollapse |
                                    ImGuiWindowFlags_NoSavedSettings;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 6));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.15f, 0.15f, 0.18f, 0.95f));

    if (ImGui::Begin("##SceneToolbar", nullptr, toolbarFlags)) {
        RenderToolbar();
    }
    ImGui::End();

    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);

    // Render stats overlay
    RenderOverlay();

    // Settings popup
    if (m_ShowSettingsPopup) {
        RenderSettingsPopup();
    }
}

void SceneView::RenderToolbar() {
    if (!m_Engine) return;

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));

    // === VIEW MODE ===
    const char* viewModes[] = {"Solid", "Shaded", "Material", "Rendered", "Wireframe", "Normals", "UV"};
    ImGui::SetNextItemWidth(90);
    if (ImGui::Combo("##ViewMode", &m_ViewMode, viewModes, IM_ARRAYSIZE(viewModes))) {
        m_Engine->_currentViewMode = static_cast<VulkanEngine::ViewMode>(m_ViewMode);
    }

    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine();

    // === DISPLAY TOGGLES ===
    // Grid
    if (m_ShowGrid) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.3f, 1.0f));
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
    }
    if (ImGui::Button("Grid", ImVec2(40, 0))) {
        m_ShowGrid = !m_ShowGrid;
        m_Engine->_showGrid = m_ShowGrid;
    }
    ImGui::PopStyleColor();

    ImGui::SameLine();

    // Outline
    if (m_ShowOutlines) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.3f, 1.0f));
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
    }
    if (ImGui::Button("Outline", ImVec2(50, 0))) {
        m_ShowOutlines = !m_ShowOutlines;
        m_Engine->_showOutline = m_ShowOutlines;
    }
    ImGui::PopStyleColor();

    ImGui::SameLine();

    // Gizmo
    if (m_ShowGizmos) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.3f, 1.0f));
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
    }
    if (ImGui::Button("Gizmo", ImVec2(45, 0))) {
        m_ShowGizmos = !m_ShowGizmos;
        // Gizmo setting (not yet in engine)
    }
    ImGui::PopStyleColor();

    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine();

    // === MAGNET SNAP ===
    if (m_SnapEnabled) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.9f, 0.5f, 0.1f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.6f, 0.2f, 1.0f));
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
    }

    if (ImGui::Button("[M]", ImVec2(30, 0))) {
        m_SnapEnabled = !m_SnapEnabled;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Magnet Snap (M)");
    }
    ImGui::PopStyleColor(2);

    ImGui::SameLine();

    // Snap value input
    ImGui::SetNextItemWidth(50);
    if (ImGui::DragFloat("##SnapValue", &m_SnapValue, 0.1f, 0.01f, 100.0f, "%.2f")) {
        // Value updated
    }

    ImGui::SameLine();

    // Snap presets dropdown
    if (ImGui::BeginCombo("##SnapPresets", "", ImGuiComboFlags_NoPreview | ImGuiComboFlags_PopupAlignLeft)) {
        for (int i = 0; i < 5; i++) {
            char label[32];
            snprintf(label, sizeof(label), "%.1f", m_SnapPresets[i]);
            if (ImGui::Selectable(label)) {
                m_SnapValue = m_SnapPresets[i];
            }
        }
        ImGui::EndCombo();
    }

    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine();

    // === STATS TOGGLE ===
    if (m_ShowStats) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.3f, 1.0f));
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
    }
    if (ImGui::Button("Stats", ImVec2(40, 0))) {
        m_ShowStats = !m_ShowStats;
    }
    ImGui::PopStyleColor();

    ImGui::SameLine();

    // === SETTINGS ===
    if (ImGui::Button("...", ImVec2(25, 0))) {
        m_ShowSettingsPopup = !m_ShowSettingsPopup;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Viewport Settings");
    }

    ImGui::PopStyleVar();
}

void SceneView::RenderViewport() {
    // Not used - 3D render happens in engine
}

void SceneView::RenderOverlay() {
    if (!m_ShowStats || !m_Engine) return;

    // Position in viewport area
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    float leftW, rightW;
    GetDynamicLayout(viewport->WorkSize.x, leftW, rightW);
    ImVec2 overlayPos = ImVec2(viewport->WorkPos.x + leftW + 10,
                               viewport->WorkPos.y + MENU_BAR_HEIGHT + TOOLBAR_HEIGHT + 10);

    ImGui::SetNextWindowPos(overlayPos, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.7f);

    ImGuiWindowFlags overlayFlags = ImGuiWindowFlags_NoDecoration |
                                    ImGuiWindowFlags_AlwaysAutoResize |
                                    ImGuiWindowFlags_NoSavedSettings |
                                    ImGuiWindowFlags_NoFocusOnAppearing |
                                    ImGuiWindowFlags_NoNav |
                                    ImGuiWindowFlags_NoMove;

    if (ImGui::Begin("##StatsOverlay", nullptr, overlayFlags)) {
        // FPS with color coding
        ImVec4 fpsColor = m_Fps > 60 ? ImVec4(0.3f, 0.8f, 0.3f, 1.0f) :
                          m_Fps > 30 ? ImVec4(0.8f, 0.8f, 0.3f, 1.0f) :
                                       ImVec4(0.8f, 0.3f, 0.3f, 1.0f);
        ImGui::TextColored(fpsColor, "FPS: %.0f", m_Fps);
        ImGui::TextDisabled("Frame: %.2f ms", m_FrameTime);

        ImGui::Separator();

        // Draw calls and triangles
        ImGui::Text("Draws: %d", m_Engine->stats.drawcall_count);
        ImGui::Text("Tris: %d", m_Engine->stats.triangle_count);

        // Snap indicator
        if (m_SnapEnabled) {
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.9f, 0.6f, 0.2f, 1.0f), "[M] Snap: %.2f", m_SnapValue);
        }
    }
    ImGui::End();
}

void SceneView::RenderSettingsPopup() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    float leftW, rightW;
    GetDynamicLayout(viewport->WorkSize.x, leftW, rightW);
    ImVec2 popupPos = ImVec2(viewport->WorkPos.x + viewport->WorkSize.x - rightW - 310,
                             viewport->WorkPos.y + MENU_BAR_HEIGHT + TOOLBAR_HEIGHT + 10);

    ImGui::SetNextWindowPos(popupPos, ImGuiCond_Appearing);
    ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Viewport Settings", &m_ShowSettingsPopup, ImGuiWindowFlags_NoCollapse)) {
        if (ImGui::BeginTabBar("SettingsTabs")) {
            if (ImGui::BeginTabItem("Display")) {
                ImGui::Spacing();
                RenderViewModeSection();
                ImGui::Spacing();
                RenderDisplaySection();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Grid")) {
                ImGui::Spacing();
                RenderGridSection();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Snap")) {
                ImGui::Spacing();
                RenderSnapSection();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Background")) {
                ImGui::Spacing();
                RenderBackgroundSection();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Camera")) {
                ImGui::Spacing();
                RenderCameraSection();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}

void SceneView::RenderViewModeSection() {
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "View Mode");
    ImGui::Separator();

    const char* modes[] = {"Solid", "Shaded", "Material Preview", "Rendered", "Wireframe", "Normals", "UV Checker"};
    for (int i = 0; i < 7; i++) {
        if (ImGui::RadioButton(modes[i], m_ViewMode == i)) {
            m_ViewMode = i;
            if (m_Engine) {
                m_Engine->_currentViewMode = static_cast<VulkanEngine::ViewMode>(m_ViewMode);
            }
        }
    }
}

void SceneView::RenderDisplaySection() {
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Display Options");
    ImGui::Separator();

    if (ImGui::Checkbox("Show Grid", &m_ShowGrid)) {
        if (m_Engine) m_Engine->_showGrid = m_ShowGrid;
    }
    if (ImGui::Checkbox("Show Outlines", &m_ShowOutlines)) {
        if (m_Engine) m_Engine->_showOutline = m_ShowOutlines;
    }
    ImGui::Checkbox("Show Gizmos", &m_ShowGizmos);
    ImGui::Checkbox("Show Stats Overlay", &m_ShowStats);
}

void SceneView::RenderGridSection() {
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Grid Settings");
    ImGui::Separator();

    if (ImGui::Checkbox("Enable Grid", &m_ShowGrid)) {
        if (m_Engine) m_Engine->_showGrid = m_ShowGrid;
    }

    ImGui::Spacing();
    ImGui::TextDisabled("Grid Presets:");

    if (ImGui::Button("Default", ImVec2(-1, 0))) {
        // Apply default grid settings
    }
    if (ImGui::Button("Blender Style", ImVec2(-1, 0))) {
        // Apply Blender-style grid
    }
    if (ImGui::Button("Unity Style", ImVec2(-1, 0))) {
        // Apply Unity-style grid
    }
}

void SceneView::RenderSnapSection() {
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Snap Settings");
    ImGui::Separator();

    ImGui::Checkbox("Enable Snap", &m_SnapEnabled);

    ImGui::Spacing();
    ImGui::Text("Position Snap:");
    ImGui::SetNextItemWidth(-1);
    ImGui::DragFloat("##PosSnap", &m_SnapValue, 0.1f, 0.01f, 100.0f, "%.2f units");

    ImGui::Spacing();
    ImGui::TextDisabled("Quick Presets:");
    ImGui::BeginGroup();
    for (int i = 0; i < 5; i++) {
        if (i > 0) ImGui::SameLine();
        char label[16];
        snprintf(label, sizeof(label), "%.1f", m_SnapPresets[i]);
        if (ImGui::Button(label, ImVec2(45, 0))) {
            m_SnapValue = m_SnapPresets[i];
        }
    }
    ImGui::EndGroup();

    ImGui::Spacing();
    ImGui::Separator();

    ImGui::Checkbox("Rotation Snap", &m_SnapRotationEnabled);
    ImGui::SetNextItemWidth(-1);
    ImGui::DragFloat("##RotSnap", &m_SnapRotationAngle, 1.0f, 1.0f, 90.0f, "%.0f deg");

    ImGui::Spacing();

    ImGui::Checkbox("Scale Snap", &m_SnapScaleEnabled);
    ImGui::SetNextItemWidth(-1);
    ImGui::DragFloat("##ScaleSnap", &m_SnapScaleValue, 0.01f, 0.01f, 1.0f, "%.2f");
}

void SceneView::RenderBackgroundSection() {
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Background");
    ImGui::Separator();

    static int bgType = 0;
    ImGui::RadioButton("Gradient", &bgType, 0);
    ImGui::RadioButton("Solid Color", &bgType, 1);
    ImGui::RadioButton("Environment Map", &bgType, 2);
}

void SceneView::RenderCameraSection() {
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Camera Settings");
    ImGui::Separator();

    if (m_Engine) {
        ImGui::Text("Position:");
        ImGui::DragFloat3("##CamPos", &m_Engine->mainCamera.position.x, 0.1f);

        ImGui::Text("Rotation:");
        float pitch = glm::degrees(m_Engine->mainCamera.pitch);
        float yaw = glm::degrees(m_Engine->mainCamera.yaw);
        if (ImGui::DragFloat("Pitch", &pitch, 0.5f, -89.0f, 89.0f)) {
            m_Engine->mainCamera.pitch = glm::radians(pitch);
        }
        if (ImGui::DragFloat("Yaw", &yaw, 0.5f)) {
            m_Engine->mainCamera.yaw = glm::radians(yaw);
        }

        ImGui::Spacing();
        if (ImGui::Button("Reset Camera", ImVec2(-1, 0))) {
            m_Engine->mainCamera.position = glm::vec3(0.f, 5.f, 10.f);
            m_Engine->mainCamera.pitch = -0.3f;
            m_Engine->mainCamera.yaw = 0.f;
        }
    }
}

} // namespace Yalaz::UI
