#pragma once
// =============================================================================
// YALAZ ENGINE - Lighting Panel
// =============================================================================
// Light settings and management
// =============================================================================

#include "../Panel.h"
#include <glm/glm.hpp>

namespace Yalaz::UI {

class LightingPanel : public Panel {
public:
    LightingPanel();

    void OnRender() override;

private:
    void RenderAmbientSection();
    void RenderDirectionalSection();
    void RenderPointLightsSection();

    // Ambient light temp values
    glm::vec3 m_AmbientColor = glm::vec3(0.1f);
    float m_AmbientIntensity = 0.3f;

    // Directional light temp values
    glm::vec3 m_SunDirection = glm::vec3(-0.5f, -1.0f, -0.3f);
    glm::vec3 m_SunColor = glm::vec3(1.0f, 0.95f, 0.8f);
    float m_SunIntensity = 1.0f;
};

} // namespace Yalaz::UI
