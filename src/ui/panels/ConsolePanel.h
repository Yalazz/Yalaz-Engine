#pragma once
// =============================================================================
// YALAZ ENGINE - Console Panel
// =============================================================================
// Stats display and debug console
// =============================================================================

#include "../Panel.h"

namespace Yalaz::UI {

class ConsolePanel : public Panel {
public:
    ConsolePanel();

    void OnRender() override;

private:
    void RenderStatsSection();
    void RenderPerformanceSection();

    // FPS tracking (uses ImGui's DeltaTime for efficiency)
    float m_FpsHistory[120] = {0};
    int m_FpsHistoryIndex = 0;
    int m_FrameCounter = 0;
    float m_CurrentFps = 60.0f;
    float m_FrameTime = 16.67f;
};

} // namespace Yalaz::UI
