#pragma once
// =============================================================================
// YALAZ ENGINE - Lighting Debug View
// =============================================================================

#include "EditorView.h"

namespace Yalaz::UI {

class LightingDebugView : public EditorView {
public:
    LightingDebugView() : EditorView("Lighting Debug", "[LD]", ViewCategory::Debug) {}
    void OnRender() override;

private:
    void RenderLightList();
    void RenderVisualization();
    void RenderStatistics();
    void RenderShadowMaps();
    void RenderCascades();
    void RenderLightContribution();
    void RenderProbes();
    void RenderAO();
    void RenderGI();

    int m_DebugMode = 0;  // 0=None, 1=Shadows, 2=Cascades, 3=Contribution, etc.
    int m_SelectedCascade = -1;

    ImVec4 m_CascadeColors[4] = {
        ImVec4(1.0f, 0.2f, 0.2f, 0.5f),
        ImVec4(0.2f, 1.0f, 0.2f, 0.5f),
        ImVec4(0.2f, 0.2f, 1.0f, 0.5f),
        ImVec4(1.0f, 1.0f, 0.2f, 0.5f)
    };

    float m_DepthScale = 1.0f;
    bool m_ShowLightVolumes = true;
    float m_ProbePreviewSize = 64.0f;
};

} // namespace Yalaz::UI
