#pragma once
// =============================================================================
// YALAZ ENGINE - GPU Debug View
// =============================================================================

#include "EditorView.h"
#include <deque>

namespace Yalaz::UI {

class GPUDebugView : public EditorView {
public:
    GPUDebugView() : EditorView("GPU Debug", "[GPU]", ViewCategory::Debug) {}
    void OnUpdate(float deltaTime) override;
    void OnRender() override;

private:
    void RenderOverview();
    void RenderVisualization();
    void RenderDrawCalls();
    void RenderMemory();
    void RenderCounters();

    int m_DebugMode = 0;  // 0=None, 1=Overdraw, 2=Depth, 3=Normals, etc.

    // Stats
    size_t m_UsedMemory = 2ULL * 1024 * 1024 * 1024;
    size_t m_TotalMemory = 8ULL * 1024 * 1024 * 1024;

    // History
    std::deque<float> m_FrameTimeHistory;
    static constexpr size_t MAX_HISTORY = 120;

    bool m_SortByGPUTime = true;
    bool m_HighlightExpensive = true;
    float m_ExpensiveThreshold = 1.0f;
    bool m_GroupByType = true;
};

} // namespace Yalaz::UI
