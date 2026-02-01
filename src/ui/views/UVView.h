#pragma once
// =============================================================================
// YALAZ ENGINE - UV View
// =============================================================================

#include "EditorView.h"
#include <vector>

namespace Yalaz::UI {

class UVView : public EditorView {
public:
    UVView() : EditorView("UV Editor", "[UV]", ViewCategory::Graphics) {}
    void OnInit(VulkanEngine* engine) override;
    void OnRender() override;

private:
    void SyncWithEngine();
    void GenerateUVsForPrimitive(int primitiveType);
    void GeneratePlaceholderUVs(uint32_t vertexCount);
    void RenderCanvas();
    void RenderStatistics();

    std::string m_SelectedMeshName;

    float m_Zoom = 1.0f;
    float m_PanX = 0.0f;
    float m_PanY = 0.0f;

    bool m_ShowWireframe = true;
    bool m_ShowFilled = false;
    bool m_ShowGrid = true;
    bool m_ShowSeams = true;

    int m_UVChannel = 0;
    int m_SelectMode = 0;  // 0=Vertex, 1=Edge, 2=Face, 3=Island

    // Demo UV data
    std::vector<float> m_DemoUVs;
    std::vector<uint32_t> m_DemoIndices;
};

} // namespace Yalaz::UI
