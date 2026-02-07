#pragma once

#include "EditorView.h"
#include <renderer/PostProcess.h>
#include <glm/glm.hpp>

namespace Yalaz::UI {

// =============================================================================
// RENDER SETTINGS VIEW - UI panel for post-processing, path tracing, environment
// =============================================================================
class RenderSettingsView : public EditorView {
public:
    RenderSettingsView();
    ~RenderSettingsView() override = default;

    // EditorView interface
    const char* GetDisplayName() const override { return "Render Settings"; }
    ViewCategory GetCategory() const override { return ViewCategory::Rendering; }
    ViewFlags GetFlags() const override { return ViewFlags::Default; }

    void OnInit(VulkanEngine* engine) override;

    // Get current render settings
    Yalaz::Renderer::RenderSettings& getSettings() { return _settings; }

protected:
    void OnRenderContent() override;

private:
    Yalaz::Renderer::RenderSettings _settings;
    char _cubemapPath[512] = "../../assets/cubemapping";

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
