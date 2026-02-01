#pragma once
// =============================================================================
// YALAZ ENGINE - Scene View
// =============================================================================
// Main 3D viewport with professional UX:
// - Clean toolbar with essential controls
// - Magnet snap with visual indicator
// - View mode selector
// - Grid/Outline/Gizmo toggles
// - Settings popup panel
// =============================================================================

#include "EditorView.h"
#include <glm/glm.hpp>

namespace Yalaz::UI {

class SceneView : public EditorView {
public:
    SceneView() : EditorView("Scene", "[S]", ViewCategory::Core) {}

    void OnRender() override;
    void OnUpdate(float deltaTime) override;

    // Public API for snap settings (used by other views)
    bool IsSnapEnabled() const { return m_SnapEnabled; }
    float GetSnapValue() const { return m_SnapValue; }
    float GetSnapRotation() const { return m_SnapRotationAngle; }
    float GetSnapScale() const { return m_SnapScaleValue; }

    void SetSnapEnabled(bool enabled) { m_SnapEnabled = enabled; }

private:
    void RenderToolbar();
    void RenderViewport();
    void RenderOverlay();
    void RenderSettingsPopup();

    // Settings sections
    void RenderViewModeSection();
    void RenderDisplaySection();
    void RenderBackgroundSection();
    void RenderGridSection();
    void RenderSnapSection();
    void RenderCameraSection();

    // View settings
    int m_ViewMode = 1;  // 0=Solid, 1=Shaded, 2=Material, 3=Rendered, 4=Wireframe, 5=Normals, 6=UV
    bool m_ShowGrid = true;
    bool m_ShowOutlines = true;
    bool m_ShowGizmos = true;
    bool m_ShowStats = true;

    // Settings popup
    bool m_ShowSettingsPopup = false;
    int m_SettingsTab = 0;

    // Magnet Snap settings
    bool m_SnapEnabled = false;
    float m_SnapValue = 1.0f;
    bool m_SnapRotationEnabled = false;
    float m_SnapRotationAngle = 15.0f;
    bool m_SnapScaleEnabled = false;
    float m_SnapScaleValue = 0.1f;

    // Quick snap presets
    float m_SnapPresets[5] = {0.1f, 0.5f, 1.0f, 2.0f, 5.0f};

    // Stats
    float m_Fps = 0.0f;
    float m_FrameTime = 0.0f;
    float m_FpsHistory[60] = {0};
    int m_FpsIndex = 0;
};

} // namespace Yalaz::UI
