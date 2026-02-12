// =============================================================================
// YALAZ ENGINE - Settings View Implementation
// =============================================================================

#include "SettingsView.h"
#include "../../vk_engine.h"
#include <imgui.h>
#include <algorithm>
#include <fstream>

// Optional JSON support - define if nlohmann/json is available
#ifdef YALAZ_HAS_JSON
#include <nlohmann/json.hpp>
using json = nlohmann::json;
#endif

namespace Yalaz::UI {

SettingsView::SettingsView()
    : EditorView("Settings", "[S]", ViewCategory::System) {
    // Initialize quality presets
    m_QualityPresets.push_back({"Low", "Best performance, reduced visuals",
                                 0, 0, 0, 0, false, false, false, false, 0});
    m_QualityPresets.push_back({"Medium", "Balanced performance and quality",
                                 1, 1, 1, 1, false, false, false, false, 1});
    m_QualityPresets.push_back({"High", "Good quality with smooth performance",
                                 2, 2, 1, 1, true, true, false, false, 2});
    m_QualityPresets.push_back({"Ultra", "Maximum visual quality",
                                 3, 3, 2, 2, true, true, true, true, 3});

    // Initialize resolutions
    m_Resolutions = {
        {1280, 720},
        {1366, 768},
        {1600, 900},
        {1920, 1080},
        {2560, 1440},
        {3840, 2160}
    };

    for (const auto& res : m_Resolutions) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%dx%d", res.first, res.second);
        m_ResolutionNames.push_back(buf);
    }
}

ViewFlags SettingsView::GetFlags() const {
    return ViewFlags::CanDock | ViewFlags::CanTab | ViewFlags::CanFloat |
           ViewFlags::HasToolbar | ViewFlags::Singleton;
}

void SettingsView::OnInit(VulkanEngine* engine) {
    EditorView::OnInit(engine);
    LoadSettings();
    ApplyToEngine();  // Apply loaded settings to engine
}

void SettingsView::ApplyToEngine() {
    if (!m_Engine) return;

    // Apply camera speed
    m_Engine->mainCamera.moveSpeed = m_Editor.cameraMoveSpeed;
    // Note: Camera rotateSpeed would need to be added to Camera class

    // Apply grid settings
    m_Engine->_showGrid = m_Editor.showGrid;
    m_Engine->_gridSettings.baseGridSize = std::max(0.01f, m_Editor.gridSize);
    m_Engine->_gridSettings.gridOpacity = std::clamp(m_Editor.gridOpacity, 0.0f, 1.0f);

    // Apply snapping
    m_Engine->snapEnabled = m_Editor.snapToGrid;
    m_Engine->snapPositionValue = std::max(0.01f, m_Editor.snapValue);

    // Apply render settings used by renderer/runtime
    auto& rs = m_Engine->_renderSettings;
    rs.bloomEnabled = m_Graphics.bloom;
    rs.bloomIntensity = m_Graphics.bloomIntensity;
    rs.bloomThreshold = m_Graphics.bloomThreshold;

    rs.ssaoEnabled = m_Graphics.ambientOcclusion;
    rs.ssaoRadius = m_Graphics.aoRadius;
    rs.ssaoIntensity = m_Graphics.aoIntensity;
    switch (m_Graphics.aoQuality) {
        case 0: rs.ssaoSamples = 8;  rs.ssaoBlurPasses = 1; break;   // SSAO
        case 1: rs.ssaoSamples = 16; rs.ssaoBlurPasses = 2; break;   // HBAO
        default: rs.ssaoSamples = 32; rs.ssaoBlurPasses = 3; break;  // GTAO
    }

    rs.ssrEnabled = m_Graphics.screenSpaceReflections;
    rs.exposure = m_Graphics.exposure;
    rs.gamma = m_Graphics.gamma;
    rs.contrast = m_Graphics.contrast;
    rs.saturation = m_Graphics.saturation;
    rs.sharpness = std::clamp(m_Graphics.taaSharpness, 0.0f, 1.0f);

    // Apply shadow quality to active shadow toggles and quality-related settings.
    m_Engine->shadowsEnabled = (m_Graphics.shadowQuality > 0);
    switch (m_Graphics.shadowQuality) {
        case 0:
            rs.pcssEnabled = false;
            rs.contactShadowsEnabled = false;
            break;
        case 1:
            rs.pcssEnabled = true;
            rs.pcssBlockerSamples = 8;
            rs.pcssPCFSamples = 16;
            rs.contactShadowsEnabled = false;
            break;
        case 2:
            rs.pcssEnabled = true;
            rs.pcssBlockerSamples = 16;
            rs.pcssPCFSamples = 32;
            rs.contactShadowsEnabled = true;
            rs.contactShadowSteps = 16;
            break;
        default:
            rs.pcssEnabled = true;
            rs.pcssBlockerSamples = 32;
            rs.pcssPCFSamples = 64;
            rs.contactShadowsEnabled = true;
            rs.contactShadowSteps = 32;
            break;
    }
}

void SettingsView::SaveSettings(const std::string& path) {
#ifdef YALAZ_HAS_JSON
    try {
        json j;

        // Graphics
        j["graphics"]["windowMode"] = m_Graphics.windowMode;
        j["graphics"]["resolutionIndex"] = m_Graphics.resolutionIndex;
        j["graphics"]["vsync"] = m_Graphics.vsync;
        j["graphics"]["qualityPreset"] = m_Graphics.qualityPreset;
        j["graphics"]["shadowQuality"] = m_Graphics.shadowQuality;
        j["graphics"]["textureQuality"] = m_Graphics.textureQuality;
        j["graphics"]["antiAliasing"] = m_Graphics.antiAliasing;
        j["graphics"]["ambientOcclusion"] = m_Graphics.ambientOcclusion;
        j["graphics"]["screenSpaceReflections"] = m_Graphics.screenSpaceReflections;
        j["graphics"]["bloom"] = m_Graphics.bloom;
        j["graphics"]["motionBlur"] = m_Graphics.motionBlur;
        j["graphics"]["depthOfField"] = m_Graphics.depthOfField;
        j["graphics"]["exposure"] = m_Graphics.exposure;
        j["graphics"]["gamma"] = m_Graphics.gamma;

        // Editor
        j["editor"]["showGrid"] = m_Editor.showGrid;
        j["editor"]["gridSize"] = m_Editor.gridSize;
        j["editor"]["gizmoSize"] = m_Editor.gizmoSize;
        j["editor"]["snapToGrid"] = m_Editor.snapToGrid;
        j["editor"]["cameraMoveSpeed"] = m_Editor.cameraMoveSpeed;
        j["editor"]["uiScale"] = m_Editor.uiScale;
        j["editor"]["showFPS"] = m_Editor.showFPS;
        j["editor"]["autosave"] = m_Editor.autosave;

        std::ofstream file(path);
        file << j.dump(2);

        m_HasUnsavedChanges = false;
    } catch (...) {
        // Handle error
    }
#else
    (void)path;
    m_HasUnsavedChanges = false;
#endif
}

void SettingsView::LoadSettings(const std::string& path) {
#ifdef YALAZ_HAS_JSON
    try {
        std::ifstream file(path);
        if (!file.is_open()) return;

        json j;
        file >> j;

        // Graphics
        if (j.contains("graphics")) {
            auto& g = j["graphics"];
            m_Graphics.windowMode = g.value("windowMode", 1);
            m_Graphics.resolutionIndex = g.value("resolutionIndex", 0);
            m_Graphics.vsync = g.value("vsync", true);
            m_Graphics.qualityPreset = g.value("qualityPreset", 2);
            m_Graphics.shadowQuality = g.value("shadowQuality", 2);
            m_Graphics.textureQuality = g.value("textureQuality", 2);
            m_Graphics.antiAliasing = g.value("antiAliasing", 1);
            m_Graphics.ambientOcclusion = g.value("ambientOcclusion", true);
            m_Graphics.screenSpaceReflections = g.value("screenSpaceReflections", true);
            m_Graphics.bloom = g.value("bloom", true);
            m_Graphics.motionBlur = g.value("motionBlur", false);
            m_Graphics.depthOfField = g.value("depthOfField", false);
            m_Graphics.exposure = g.value("exposure", 1.0f);
            m_Graphics.gamma = g.value("gamma", 2.2f);
        }

        // Editor
        if (j.contains("editor")) {
            auto& e = j["editor"];
            m_Editor.showGrid = e.value("showGrid", true);
            m_Editor.gridSize = e.value("gridSize", 1.0f);
            m_Editor.gizmoSize = e.value("gizmoSize", 1.0f);
            m_Editor.snapToGrid = e.value("snapToGrid", false);
            m_Editor.cameraMoveSpeed = e.value("cameraMoveSpeed", 5.0f);
            m_Editor.uiScale = e.value("uiScale", 1.0f);
            m_Editor.showFPS = e.value("showFPS", true);
            m_Editor.autosave = e.value("autosave", true);
        }

    } catch (...) {
        // Use defaults
    }
#else
    (void)path;
#endif
}

void SettingsView::ResetToDefaults() {
    m_Graphics = GraphicsSettings();
    m_Editor = EditorSettings();
    m_HasUnsavedChanges = true;
}

void SettingsView::ApplyQualityPreset(int preset) {
    if (preset < 0 || preset >= static_cast<int>(m_QualityPresets.size())) return;

    const auto& p = m_QualityPresets[preset];

    m_Graphics.shadowQuality = p.shadowQuality;
    m_Graphics.textureQuality = p.textureQuality;
    m_Graphics.antiAliasing = p.antiAliasing;
    m_Graphics.aoQuality = p.ambientOcclusion;
    m_Graphics.screenSpaceReflections = p.screenSpaceReflections;
    m_Graphics.motionBlur = p.motionBlur;
    m_Graphics.depthOfField = p.depthOfField;
    m_Graphics.qualityPreset = preset;

    m_HasUnsavedChanges = true;
}

void SettingsView::OnRenderToolbar() {
    struct Tab {
        const char* name;
        SettingsCategory category;
    };
    static const Tab categories[] = {
        {"Rendering", SettingsCategory::Rendering},
        {"Editor", SettingsCategory::Editor},
        {"Input", SettingsCategory::Input},
        {"Performance", SettingsCategory::Performance}
    };

    for (int i = 0; i < static_cast<int>(IM_ARRAYSIZE(categories)); ++i) {
        if (i > 0) ImGui::SameLine();

        bool selected = (m_CurrentCategory == categories[i].category);
        if (selected) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
        }

        if (ImGui::Button(categories[i].name)) {
            m_CurrentCategory = categories[i].category;
        }

        if (selected) {
            ImGui::PopStyleColor();
        }
    }

    ImGui::SameLine(ImGui::GetWindowWidth() - 120);

    if (m_HasUnsavedChanges) {
        ImGui::TextColored(ImVec4(1, 0.8f, 0.3f, 1), "*");
        ImGui::SameLine();
    }

    if (ImGui::Button("Reset")) {
        ResetToDefaults();
    }
}

void SettingsView::OnRenderContent() {
    ImGui::BeginChild("SettingsContent", ImVec2(0, 0), true);

    switch (m_CurrentCategory) {
        case SettingsCategory::Rendering:
            RenderRenderingSettings();
            break;
        case SettingsCategory::Editor:
            RenderEditorSettings();
            break;
        case SettingsCategory::Input:
            RenderInputSettings();
            break;
        case SettingsCategory::Performance:
            RenderPerformanceSettings();
            break;
        default:
            RenderRenderingSettings();
            break;
    }

    // Apply settings live so all settings panels affect rendering/editor immediately.
    ApplyToEngine();

    ImGui::EndChild();
}

void SettingsView::RenderSectionHeader(const char* title) {
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", title);
    ImGui::Separator();
    ImGui::Spacing();
}

bool SettingsView::RenderQualitySlider(const char* label, int* value, const char** names, int count) {
    bool changed = false;

    ImGui::Text("%s", label);
    ImGui::SameLine(150);
    ImGui::SetNextItemWidth(200);

    ImGui::PushID(label);
    if (ImGui::SliderInt("##slider", value, 0, count - 1, names[*value])) {
        changed = true;
        m_HasUnsavedChanges = true;
    }
    ImGui::PopID();

    return changed;
}

void SettingsView::RenderGraphicsSettings() {
    RenderSectionHeader("Display");

    // Window Mode
    const char* windowModes[] = {"Windowed", "Borderless Windowed", "Fullscreen"};
    ImGui::Text("Window Mode");
    ImGui::SameLine(150);
    ImGui::SetNextItemWidth(200);
    if (ImGui::Combo("##WindowMode", &m_Graphics.windowMode, windowModes, 3)) {
        m_HasUnsavedChanges = true;
    }

    // Resolution
    ImGui::Text("Resolution");
    ImGui::SameLine(150);
    ImGui::SetNextItemWidth(200);

    std::vector<const char*> resNames;
    for (const auto& name : m_ResolutionNames) {
        resNames.push_back(name.c_str());
    }

    if (ImGui::Combo("##Resolution", &m_Graphics.resolutionIndex,
                     resNames.data(), static_cast<int>(resNames.size()))) {
        m_HasUnsavedChanges = true;
    }

    // VSync
    ImGui::Text("V-Sync");
    ImGui::SameLine(150);
    if (ImGui::Checkbox("##VSync", &m_Graphics.vsync)) {
        m_HasUnsavedChanges = true;
    }

    RenderSectionHeader("Quality Preset");

    const char* presets[] = {"Low", "Medium", "High", "Ultra", "Custom"};
    ImGui::Text("Preset");
    ImGui::SameLine(150);
    ImGui::SetNextItemWidth(200);
    if (ImGui::Combo("##Preset", &m_Graphics.qualityPreset, presets, 5)) {
        if (m_Graphics.qualityPreset < 4) {
            ApplyQualityPreset(m_Graphics.qualityPreset);
        }
        m_HasUnsavedChanges = true;
    }

    // Preset description
    if (m_Graphics.qualityPreset < 4) {
        ImGui::TextDisabled("%s", m_QualityPresets[m_Graphics.qualityPreset].description.c_str());
    }

    RenderSectionHeader("Shadows");

    const char* shadowQualities[] = {"Off", "Low", "Medium", "High"};
    RenderQualitySlider("Shadow Quality", &m_Graphics.shadowQuality, shadowQualities, 4);

    ImGui::Text("Shadow Distance");
    ImGui::SameLine(150);
    ImGui::SetNextItemWidth(200);
    if (ImGui::SliderFloat("##ShadowDist", &m_Graphics.shadowDistance, 10.0f, 500.0f, "%.0f m")) {
        m_HasUnsavedChanges = true;
        m_Graphics.qualityPreset = 4;  // Custom
    }

    RenderSectionHeader("Textures");

    const char* textureQualities[] = {"Low", "Medium", "High", "Ultra"};
    RenderQualitySlider("Texture Quality", &m_Graphics.textureQuality, textureQualities, 4);

    ImGui::Text("Anisotropic");
    ImGui::SameLine(150);
    ImGui::SetNextItemWidth(200);
    const char* anisoLevels[] = {"Off", "2x", "4x", "8x", "16x"};
    int anisoIndex = 0;
    if (m_Graphics.anisotropicFiltering >= 16) anisoIndex = 4;
    else if (m_Graphics.anisotropicFiltering >= 8) anisoIndex = 3;
    else if (m_Graphics.anisotropicFiltering >= 4) anisoIndex = 2;
    else if (m_Graphics.anisotropicFiltering >= 2) anisoIndex = 1;

    if (ImGui::Combo("##Aniso", &anisoIndex, anisoLevels, 5)) {
        int values[] = {1, 2, 4, 8, 16};
        m_Graphics.anisotropicFiltering = values[anisoIndex];
        m_HasUnsavedChanges = true;
    }

    RenderSectionHeader("Anti-Aliasing");

    const char* aaTypes[] = {"Off", "FXAA", "TAA", "MSAA"};
    RenderQualitySlider("Type", &m_Graphics.antiAliasing, aaTypes, 4);

    if (m_Graphics.antiAliasing == 2) {  // TAA
        ImGui::Text("TAA Sharpness");
        ImGui::SameLine(150);
        ImGui::SetNextItemWidth(200);
        if (ImGui::SliderFloat("##TAASharp", &m_Graphics.taaSharpness, 0.0f, 1.0f)) {
            m_HasUnsavedChanges = true;
        }
    }

    if (m_Graphics.antiAliasing == 3) {  // MSAA
        ImGui::Text("MSAA Samples");
        ImGui::SameLine(150);
        const char* msaaSamples[] = {"2x", "4x", "8x"};
        int msaaIndex = (m_Graphics.msaaSamples == 2) ? 0 :
                        (m_Graphics.msaaSamples == 4) ? 1 : 2;
        if (ImGui::Combo("##MSAA", &msaaIndex, msaaSamples, 3)) {
            int values[] = {2, 4, 8};
            m_Graphics.msaaSamples = values[msaaIndex];
            m_HasUnsavedChanges = true;
        }
    }
}

void SettingsView::RenderRenderingSettings() {
    RenderSectionHeader("Ambient Occlusion");

    ImGui::Text("Enable");
    ImGui::SameLine(150);
    if (ImGui::Checkbox("##AOEnable", &m_Graphics.ambientOcclusion)) {
        m_HasUnsavedChanges = true;
    }

    if (m_Graphics.ambientOcclusion) {
        const char* aoQualities[] = {"SSAO", "HBAO", "GTAO"};
        RenderQualitySlider("Quality", &m_Graphics.aoQuality, aoQualities, 3);

        ImGui::Text("Radius");
        ImGui::SameLine(150);
        ImGui::SetNextItemWidth(200);
        if (ImGui::SliderFloat("##AORadius", &m_Graphics.aoRadius, 0.1f, 2.0f)) {
            m_HasUnsavedChanges = true;
        }

        ImGui::Text("Intensity");
        ImGui::SameLine(150);
        ImGui::SetNextItemWidth(200);
        if (ImGui::SliderFloat("##AOIntensity", &m_Graphics.aoIntensity, 0.0f, 2.0f)) {
            m_HasUnsavedChanges = true;
        }
    }

    RenderSectionHeader("Bloom");

    ImGui::Text("Enable");
    ImGui::SameLine(150);
    if (ImGui::Checkbox("##BloomEnable", &m_Graphics.bloom)) {
        m_HasUnsavedChanges = true;
    }

    if (m_Graphics.bloom) {
        ImGui::Text("Intensity");
        ImGui::SameLine(150);
        ImGui::SetNextItemWidth(200);
        if (ImGui::SliderFloat("##BloomIntensity", &m_Graphics.bloomIntensity, 0.0f, 2.0f)) {
            m_HasUnsavedChanges = true;
        }

        ImGui::Text("Threshold");
        ImGui::SameLine(150);
        ImGui::SetNextItemWidth(200);
        if (ImGui::SliderFloat("##BloomThreshold", &m_Graphics.bloomThreshold, 0.0f, 5.0f)) {
            m_HasUnsavedChanges = true;
        }
    }

    RenderSectionHeader("Screen Space Reflections");

    ImGui::Text("Enable");
    ImGui::SameLine(150);
    if (ImGui::Checkbox("##SSREnable", &m_Graphics.screenSpaceReflections)) {
        m_HasUnsavedChanges = true;
    }

    RenderSectionHeader("Color Grading");

    ImGui::Text("Exposure");
    ImGui::SameLine(150);
    ImGui::SetNextItemWidth(200);
    if (ImGui::SliderFloat("##Exposure", &m_Graphics.exposure, 0.1f, 5.0f)) {
        m_HasUnsavedChanges = true;
    }

    ImGui::Text("Gamma");
    ImGui::SameLine(150);
    ImGui::SetNextItemWidth(200);
    if (ImGui::SliderFloat("##Gamma", &m_Graphics.gamma, 1.0f, 3.0f)) {
        m_HasUnsavedChanges = true;
    }

    ImGui::Text("Contrast");
    ImGui::SameLine(150);
    ImGui::SetNextItemWidth(200);
    if (ImGui::SliderFloat("##Contrast", &m_Graphics.contrast, 0.5f, 2.0f)) {
        m_HasUnsavedChanges = true;
    }

    ImGui::Text("Saturation");
    ImGui::SameLine(150);
    ImGui::SetNextItemWidth(200);
    if (ImGui::SliderFloat("##Saturation", &m_Graphics.saturation, 0.0f, 2.0f)) {
        m_HasUnsavedChanges = true;
    }

    ImGui::Text("Sharpness");
    ImGui::SameLine(150);
    ImGui::SetNextItemWidth(200);
    if (ImGui::SliderFloat("##Sharpness", &m_Graphics.taaSharpness, 0.0f, 1.0f)) {
        m_HasUnsavedChanges = true;
    }
}

void SettingsView::RenderEditorSettings() {
    RenderSectionHeader("Grid");

    ImGui::Text("Show Grid");
    ImGui::SameLine(150);
    if (ImGui::Checkbox("##ShowGrid", &m_Editor.showGrid)) {
        m_HasUnsavedChanges = true;
        if (m_Engine) m_Engine->_showGrid = m_Editor.showGrid;
    }

    ImGui::Text("Grid Size");
    ImGui::SameLine(150);
    ImGui::SetNextItemWidth(200);
    if (ImGui::SliderFloat("##GridSize", &m_Editor.gridSize, 0.1f, 10.0f)) {
        m_HasUnsavedChanges = true;
    }

    ImGui::Text("Grid Opacity");
    ImGui::SameLine(150);
    ImGui::SetNextItemWidth(200);
    if (ImGui::SliderFloat("##GridOpacity", &m_Editor.gridOpacity, 0.0f, 1.0f)) {
        m_HasUnsavedChanges = true;
    }

    RenderSectionHeader("Gizmos");

    ImGui::Text("Gizmo Size");
    ImGui::SameLine(150);
    ImGui::SetNextItemWidth(200);
    if (ImGui::SliderFloat("##GizmoSize", &m_Editor.gizmoSize, 0.5f, 3.0f)) {
        m_HasUnsavedChanges = true;
    }

    ImGui::Text("Snap to Grid");
    ImGui::SameLine(150);
    if (ImGui::Checkbox("##SnapGrid", &m_Editor.snapToGrid)) {
        m_HasUnsavedChanges = true;
    }

    if (m_Editor.snapToGrid) {
        ImGui::Text("Snap Value");
        ImGui::SameLine(150);
        ImGui::SetNextItemWidth(200);
        if (ImGui::SliderFloat("##SnapValue", &m_Editor.snapValue, 0.1f, 10.0f)) {
            m_HasUnsavedChanges = true;
        }
    }

    RenderSectionHeader("Camera");

    ImGui::Text("Move Speed");
    ImGui::SameLine(150);
    ImGui::SetNextItemWidth(200);
    if (ImGui::SliderFloat("##CamMove", &m_Editor.cameraMoveSpeed, 1.0f, 20.0f)) {
        m_HasUnsavedChanges = true;
        if (m_Engine) m_Engine->mainCamera.moveSpeed = m_Editor.cameraMoveSpeed;
    }

    ImGui::Text("Rotate Speed");
    ImGui::SameLine(150);
    ImGui::SetNextItemWidth(200);
    if (ImGui::SliderFloat("##CamRotate", &m_Editor.cameraRotateSpeed, 0.1f, 1.0f)) {
        m_HasUnsavedChanges = true;
        // Note: Camera rotate speed would need to be added to Camera class
    }

    ImGui::Text("Invert Y");
    ImGui::SameLine(150);
    if (ImGui::Checkbox("##InvertY", &m_Editor.invertY)) {
        m_HasUnsavedChanges = true;
    }

    RenderSectionHeader("User Interface");

    ImGui::Text("UI Scale");
    ImGui::SameLine(150);
    ImGui::SetNextItemWidth(200);
    if (ImGui::SliderFloat("##UIScale", &m_Editor.uiScale, 0.75f, 2.0f)) {
        m_HasUnsavedChanges = true;
    }

    ImGui::Text("Show FPS");
    ImGui::SameLine(150);
    if (ImGui::Checkbox("##ShowFPS", &m_Editor.showFPS)) {
        m_HasUnsavedChanges = true;
    }

    ImGui::Text("Show Stats");
    ImGui::SameLine(150);
    if (ImGui::Checkbox("##ShowStats", &m_Editor.showStats)) {
        m_HasUnsavedChanges = true;
    }

    RenderSectionHeader("Autosave");

    ImGui::Text("Enable");
    ImGui::SameLine(150);
    if (ImGui::Checkbox("##Autosave", &m_Editor.autosave)) {
        m_HasUnsavedChanges = true;
    }

    if (m_Editor.autosave) {
        ImGui::Text("Interval");
        ImGui::SameLine(150);
        ImGui::SetNextItemWidth(200);
        if (ImGui::SliderInt("##AutosaveInt", &m_Editor.autosaveInterval, 60, 600, "%d s")) {
            m_HasUnsavedChanges = true;
        }
    }
}

void SettingsView::RenderInputSettings() {
    ImGui::TextDisabled("Input settings coming soon...");

    RenderSectionHeader("Mouse");
    ImGui::TextDisabled("Mouse sensitivity and button mapping");

    RenderSectionHeader("Keyboard");
    ImGui::TextDisabled("Keyboard shortcuts and bindings");

    RenderSectionHeader("Gamepad");
    ImGui::TextDisabled("Controller support and mapping");
}

void SettingsView::RenderPerformanceSettings() {
    RenderSectionHeader("Frame Rate");

    ImGui::Text("Limit Frame Rate");
    ImGui::SameLine(150);
    if (ImGui::Checkbox("##LimitFPS", &m_Graphics.limitFrameRate)) {
        m_HasUnsavedChanges = true;
    }

    if (m_Graphics.limitFrameRate) {
        ImGui::Text("Target FPS");
        ImGui::SameLine(150);
        ImGui::SetNextItemWidth(200);
        if (ImGui::SliderInt("##TargetFPS", &m_Graphics.targetFrameRate, 30, 240)) {
            m_HasUnsavedChanges = true;
        }
    }

    RenderSectionHeader("Memory");

    ImGui::TextDisabled("GPU Memory: 2.1 GB / 8.0 GB");
    ImGui::TextDisabled("Texture Cache: 512 MB");
    ImGui::TextDisabled("Mesh Cache: 256 MB");

    if (ImGui::Button("Clear Caches")) {
        // Would clear caches
    }

    RenderSectionHeader("Statistics");

    ImGui::TextDisabled("Draw Calls: 150");
    ImGui::TextDisabled("Triangles: 1.2M");
    ImGui::TextDisabled("Vertices: 800K");
    ImGui::TextDisabled("Shaders: 45");
    ImGui::TextDisabled("Textures: 128");
}

void SettingsView::RenderAdvancedSettings() {
    RenderSectionHeader("Rendering Backend");

    ImGui::TextDisabled("Vulkan 1.3");
    ImGui::TextDisabled("GPU: NVIDIA GeForce RTX 3080");
    ImGui::TextDisabled("Driver: 536.40");

    RenderSectionHeader("Debug Options");

    static bool wireframe = false;
    ImGui::Text("Wireframe Mode");
    ImGui::SameLine(150);
    ImGui::Checkbox("##Wireframe", &wireframe);

    static bool showBounds = false;
    ImGui::Text("Show Bounds");
    ImGui::SameLine(150);
    ImGui::Checkbox("##ShowBounds", &showBounds);

    static bool showNormals = false;
    ImGui::Text("Show Normals");
    ImGui::SameLine(150);
    ImGui::Checkbox("##ShowNormals", &showNormals);

    RenderSectionHeader("Experimental");

    ImGui::TextColored(ImVec4(1, 0.5f, 0.5f, 1), "Warning: These features are experimental");

    static bool rayTracing = false;
    ImGui::Text("Ray Tracing");
    ImGui::SameLine(150);
    ImGui::Checkbox("##RayTracing", &rayTracing);

    static bool meshShaders = false;
    ImGui::Text("Mesh Shaders");
    ImGui::SameLine(150);
    ImGui::Checkbox("##MeshShaders", &meshShaders);
}

} // namespace Yalaz::UI
