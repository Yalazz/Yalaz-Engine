#pragma once

#include <renderer/PostProcess.h>
#include <glm/glm.hpp>

// Forward declarations
class VulkanEngine;

namespace Yalaz::UI {

// =============================================================================
// RENDER SETTINGS VIEW - UI panel for post-processing and render quality
// =============================================================================
class RenderSettingsView {
public:
    RenderSettingsView(VulkanEngine* engine);

    // Draw the ImGui panel
    void draw();

    // Get current render settings
    Yalaz::Renderer::RenderSettings& getSettings() { return _settings; }

private:
    VulkanEngine* _engine;
    Yalaz::Renderer::RenderSettings _settings;

    // Panel state
    bool _showPanel = true;
    int _selectedTab = 0;

    // Draw individual sections
    void drawSSAOSettings();
    void drawBloomSettings();
    void drawToneMappingSettings();
    void drawColorGradingSettings();
    void drawSSRSettings();
    void drawShadowSettings();
    void drawSpotLightSettings();
    void drawPerformanceStats();
    void drawPathTracerSettings();
    void drawEnvironmentSettings();
};

} // namespace Yalaz::UI
