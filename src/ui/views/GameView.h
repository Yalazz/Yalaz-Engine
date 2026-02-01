#pragma once
// =============================================================================
// YALAZ ENGINE - Game View
// =============================================================================

#include "EditorView.h"

namespace Yalaz::UI {

class GameView : public EditorView {
public:
    GameView() : EditorView("Game", "[G]", ViewCategory::Core) {}
    void OnRender() override;

private:
    void RenderScenePreview(ImVec2 pos, ImVec2 size, ImDrawList* drawList);
    void RenderStatsOverlay(ImVec2 pos);

    bool m_ShowStats = true;
    bool m_ShowGizmos = true;
    bool m_IsPlaying = false;
};

} // namespace Yalaz::UI
