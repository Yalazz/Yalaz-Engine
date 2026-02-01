#pragma once
// =============================================================================
// YALAZ ENGINE - Profiler View
// =============================================================================

#include "EditorView.h"

namespace Yalaz::UI {

class ProfilerView : public EditorView {
public:
    ProfilerView() : EditorView("Profiler", "[P]", ViewCategory::Debug) {}
    void OnUpdate(float deltaTime) override;
    void OnRender() override;

private:
    void RenderOverview();
    void RenderTimeline();
    void RenderStatistics();
    void RenderMemory();

    float m_FpsHistory[300] = {0};
    float m_FrameTimeHistory[300] = {0};
    float m_CpuHistory[300] = {0};
    float m_GpuHistory[300] = {0};
    int m_HistoryIndex = 0;

    float m_CurrentFps = 0.0f;
    float m_CurrentFrameTime = 0.0f;
    float m_AverageFps = 0.0f;
    float m_MinFps = 9999.0f;
    float m_MaxFps = 0.0f;

    bool m_Paused = false;
    bool m_ShowCpu = true;
    bool m_ShowGpu = true;
};

} // namespace Yalaz::UI
