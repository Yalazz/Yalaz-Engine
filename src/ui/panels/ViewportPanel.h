#pragma once
// =============================================================================
// YALAZ ENGINE - Viewport Settings Panel
// =============================================================================
// Controls for viewport rendering options
// =============================================================================

#include "../Panel.h"

namespace Yalaz::UI {

class ViewportPanel : public Panel {
public:
    ViewportPanel();

    void OnRender() override;

private:
    void RenderViewModeSection();
    void RenderDisplaySection();
    void RenderBackgroundSection();
    void RenderGridSection();
};

} // namespace Yalaz::UI
