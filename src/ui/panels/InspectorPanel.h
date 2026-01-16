#pragma once
// =============================================================================
// YALAZ ENGINE - Inspector Panel
// =============================================================================
// Property editor for selected objects
// =============================================================================

#include "../Panel.h"
#include <glm/glm.hpp>

namespace Yalaz::UI {

class InspectorPanel : public Panel {
public:
    InspectorPanel();

    void OnInit(VulkanEngine* engine) override;
    void OnRender() override;

private:
    void RenderNoSelection();
    void RenderPrimitiveInspector(int index);
    void RenderSceneNodeInspector();
    void RenderLightInspector(int index);

    // Reusable UI components
    void RenderTransformSection(glm::vec3& position, glm::vec3& rotation, glm::vec3& scale);
    void RenderColorSection(glm::vec4& mainColor, bool& useFaceColors, glm::vec4* faceColors);
    bool RenderVec3Control(const char* label, glm::vec3& values, float resetValue = 0.0f, float speed = 0.1f);
};

} // namespace Yalaz::UI
