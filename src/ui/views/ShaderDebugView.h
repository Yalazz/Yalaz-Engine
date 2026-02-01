#pragma once
// =============================================================================
// YALAZ ENGINE - Shader Debug View
// =============================================================================

#include "EditorView.h"
#include <vector>
#include <string>

namespace Yalaz::UI {

struct ShaderEntry {
    std::string name;
    std::string path;
    bool isCompiled = true;
    std::string compileLog;
    float compileTimeMs = 0.0f;
    int instructionCount = 0;
    int registerCount = 0;
    int textureCount = 0;
};

class ShaderDebugView : public EditorView {
public:
    ShaderDebugView() : EditorView("Shader Debug", "[SD]", ViewCategory::Debug) {}
    void OnInit(VulkanEngine* engine) override;
    void OnRender() override;

private:
    void SyncWithEngine();
    void RenderShaderList();
    void RenderShaderDetails();
    void RenderUniforms();
    void RenderStatistics();

    std::vector<ShaderEntry> m_Shaders;
    int m_SelectedShader = -1;
    char m_SearchBuffer[128] = "";
    float m_ListWidth = 180.0f;
};

} // namespace Yalaz::UI
