#pragma once
// =============================================================================
// YALAZ ENGINE - Settings View
// =============================================================================
// Comprehensive settings panel with categories:
// - Rendering (AO, bloom, SSR, color grading)
// - Editor (grid, gizmos, camera, UI)
// - Input (mouse, keyboard, gamepad)
// - Performance (frame rate, memory)
// =============================================================================

#include "EditorView.h"
#include <string>
#include <vector>

namespace Yalaz::UI {

// =============================================================================
// Settings Categories
// =============================================================================
enum class SettingsCategory {
    Rendering,
    Editor,
    Input,
    Performance
};

// =============================================================================
// Quality Preset
// =============================================================================
struct QualityPreset {
    std::string name;
    std::string description;

    int shadowQuality = 2;
    int textureQuality = 2;
    int antiAliasing = 1;
    int ambientOcclusion = 1;

    bool screenSpaceReflections = true;
    bool motionBlur = false;
    bool depthOfField = false;
    bool volumetricLighting = false;

    int level = 2;  // 0=Low, 1=Medium, 2=High, 3=Ultra
};

// =============================================================================
// Graphics Settings
// =============================================================================
struct GraphicsSettings {
    // Display
    int windowMode = 1;          // 0=Windowed, 1=Borderless, 2=Fullscreen
    int resolutionIndex = 0;
    bool vsync = true;

    // Quality
    int qualityPreset = 2;       // 0=Low, 1=Medium, 2=High, 3=Ultra, 4=Custom
    int shadowQuality = 2;
    int textureQuality = 2;
    int antiAliasing = 1;        // 0=Off, 1=FXAA, 2=TAA, 3=MSAA
    int aoQuality = 1;           // 0=SSAO, 1=HBAO, 2=GTAO

    // Shadow settings
    float shadowDistance = 200.0f;

    // Texture settings
    int anisotropicFiltering = 16;

    // AA settings
    int msaaSamples = 4;
    float taaSharpness = 0.5f;

    // Post-processing
    bool ambientOcclusion = true;
    bool screenSpaceReflections = true;
    bool bloom = true;
    bool motionBlur = false;
    bool depthOfField = false;

    // Bloom settings
    float bloomIntensity = 0.5f;
    float bloomThreshold = 1.0f;

    // AO settings
    float aoRadius = 0.5f;
    float aoIntensity = 1.0f;

    // Color grading
    float exposure = 1.0f;
    float gamma = 2.2f;
    float contrast = 1.0f;
    float saturation = 1.0f;

    // Frame rate
    bool limitFrameRate = false;
    int targetFrameRate = 60;
};

// =============================================================================
// Editor Settings
// =============================================================================
struct EditorSettings {
    // Grid
    bool showGrid = true;
    float gridSize = 1.0f;
    float gridOpacity = 0.5f;

    // Gizmos
    float gizmoSize = 1.0f;
    bool snapToGrid = false;
    float snapValue = 1.0f;

    // Camera
    float cameraMoveSpeed = 5.0f;
    float cameraRotateSpeed = 0.3f;
    bool invertY = false;

    // UI
    float uiScale = 1.0f;
    bool showFPS = true;
    bool showStats = false;

    // Autosave
    bool autosave = true;
    int autosaveInterval = 300;  // seconds
};

// =============================================================================
// Settings View
// =============================================================================
class SettingsView : public EditorView {
public:
    SettingsView();
    ~SettingsView() override = default;

    // View interface
    const char* GetDisplayName() const override { return "Settings"; }
    ViewCategory GetCategory() const override { return ViewCategory::System; }
    ViewFlags GetFlags() const override;

    // Lifecycle
    void OnInit(VulkanEngine* engine) override;

    // Settings management
    void SaveSettings(const std::string& path = "settings.json");
    void LoadSettings(const std::string& path = "settings.json");
    void ResetToDefaults();
    void ApplyQualityPreset(int preset);
    void ApplyToEngine();  // Apply settings to engine in real-time

    // Accessors
    GraphicsSettings& GetGraphicsSettings() { return m_Graphics; }
    EditorSettings& GetEditorSettings() { return m_Editor; }
    bool HasUnsavedChanges() const { return m_HasUnsavedChanges; }

protected:
    void OnRenderToolbar() override;
    void OnRenderContent() override;

private:
    void RenderGraphicsSettings();
    void RenderRenderingSettings();
    void RenderEditorSettings();
    void RenderInputSettings();
    void RenderPerformanceSettings();
    void RenderAdvancedSettings();

    void RenderSectionHeader(const char* title);
    bool RenderQualitySlider(const char* label, int* value, const char** names, int count);

    SettingsCategory m_CurrentCategory = SettingsCategory::Rendering;

    GraphicsSettings m_Graphics;
    EditorSettings m_Editor;

    std::vector<QualityPreset> m_QualityPresets;
    std::vector<std::pair<int, int>> m_Resolutions;
    std::vector<std::string> m_ResolutionNames;

    bool m_HasUnsavedChanges = false;
};

} // namespace Yalaz::UI
