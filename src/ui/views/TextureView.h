#pragma once
// =============================================================================
// YALAZ ENGINE - Texture View
// =============================================================================

#include "EditorView.h"

namespace Yalaz::UI {

class TextureView : public EditorView {
public:
    TextureView() : EditorView("Texture Inspector", "[T]", ViewCategory::Graphics) {}
    void OnRender() override;

private:
    void RenderTextureList();
    void RenderPreview();
    void RenderMetadata();
    void RenderChannels();

    int m_ChannelMode = 0;  // 0=RGBA, 1=R, 2=G, 3=B, 4=A
    int m_MipLevel = 0;
    float m_Zoom = 1.0f;
    bool m_ShowCheckerboard = true;

    // Selection
    std::string m_SelectedScene;
    std::string m_SelectedTexture;
};

} // namespace Yalaz::UI
