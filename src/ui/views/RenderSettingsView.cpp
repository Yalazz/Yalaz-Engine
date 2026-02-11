#include "RenderSettingsView.h"
#include "vk_engine.h"
#include "renderer/PathTracer.h"
#include "renderer/EnvironmentMap.h"
#include <imgui.h>
#include <filesystem>
#include <fmt/core.h>

namespace Yalaz::UI {

RenderSettingsView::RenderSettingsView()
    : EditorView("Render Settings", "[R]", ViewCategory::Rendering) {}

void RenderSettingsView::OnInit(VulkanEngine* engine) {
    EditorView::OnInit(engine);
    // Sync local settings with engine's render settings
    if (engine) {
        _settings = engine->_renderSettings;
    }
}

void RenderSettingsView::OnRenderContent() {
    if (!m_Engine) return;

    // Sync engine render settings with UI settings each frame
    m_Engine->_renderSettings = _settings;

    // Tab bar for different categories
    if (ImGui::BeginTabBar("RenderSettingsTabs")) {

        if (ImGui::BeginTabItem("Path Tracer")) {
            drawPathTracerSettings();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Environment")) {
            drawEnvironmentSettings();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Post Process")) {
            drawBloomSettings();
            ImGui::Separator();
            drawToneMappingSettings();
            ImGui::Separator();
            drawColorGradingSettings();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("SSAO")) {
            drawSSAOSettings();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("SSR")) {
            drawSSRSettings();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Shadows")) {
            drawShadowSettings();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Lights")) {
            drawSpotLightSettings();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Performance")) {
            drawPerformanceStats();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}

void RenderSettingsView::drawSSAOSettings() {
    ImGui::Text("Screen Space Ambient Occlusion");

    ImGui::Checkbox("Enable SSAO", &_settings.ssaoEnabled);

    if (_settings.ssaoEnabled) {
        ImGui::SliderInt("Samples", &_settings.ssaoSamples, 8, 64);
        ImGui::SliderFloat("Radius", &_settings.ssaoRadius, 0.1f, 2.0f);
        ImGui::SliderFloat("Intensity", &_settings.ssaoIntensity, 0.0f, 3.0f);
        ImGui::SliderFloat("Bias", &_settings.ssaoBias, 0.001f, 0.1f);
        ImGui::SliderInt("Blur Passes", &_settings.ssaoBlurPasses, 0, 4);

        if (ImGui::CollapsingHeader("Quality Presets")) {
            if (ImGui::Button("Low")) {
                _settings.ssaoSamples = 8;
                _settings.ssaoBlurPasses = 1;
            }
            ImGui::SameLine();
            if (ImGui::Button("Medium")) {
                _settings.ssaoSamples = 16;
                _settings.ssaoBlurPasses = 2;
            }
            ImGui::SameLine();
            if (ImGui::Button("High")) {
                _settings.ssaoSamples = 32;
                _settings.ssaoBlurPasses = 2;
            }
            ImGui::SameLine();
            if (ImGui::Button("Ultra")) {
                _settings.ssaoSamples = 64;
                _settings.ssaoBlurPasses = 3;
            }
        }
    }
}

void RenderSettingsView::drawBloomSettings() {
    ImGui::Text("Bloom");

    ImGui::Checkbox("Enable Bloom", &_settings.bloomEnabled);

    if (_settings.bloomEnabled) {
        ImGui::SliderFloat("Threshold", &_settings.bloomThreshold, 0.0f, 5.0f);
        ImGui::SliderFloat("Intensity", &_settings.bloomIntensity, 0.0f, 2.0f);
        ImGui::SliderInt("Mip Levels", &_settings.bloomMipLevels, 2, 8);
        ImGui::SliderFloat("Radius", &_settings.bloomRadius, 0.5f, 2.0f);
    }
}

void RenderSettingsView::drawToneMappingSettings() {
    ImGui::Text("Tone Mapping");

    ImGui::Checkbox("Enable Tone Mapping", &_settings.tonemappingEnabled);

    if (_settings.tonemappingEnabled) {
        const char* operators[] = { "ACES", "Reinhard", "Uncharted 2", "Linear", "AgX" };
        ImGui::Combo("Operator", &_settings.tonemapOperator, operators, 5);

        ImGui::SliderFloat("Exposure", &_settings.exposure, -5.0f, 5.0f);
        ImGui::SliderFloat("Gamma", &_settings.gamma, 1.0f, 3.0f);
    }
}

void RenderSettingsView::drawColorGradingSettings() {
    ImGui::Text("Color Grading");

    ImGui::SliderFloat("Contrast", &_settings.contrast, 0.5f, 2.0f);
    ImGui::SliderFloat("Saturation", &_settings.saturation, 0.0f, 2.0f);
    ImGui::SliderFloat("Temperature", &_settings.temperature, -1.0f, 1.0f);
    ImGui::SliderFloat("Tint", &_settings.tint, -1.0f, 1.0f);

    if (ImGui::Button("Reset Color Grading")) {
        _settings.contrast = 1.0f;
        _settings.saturation = 1.0f;
        _settings.temperature = 0.0f;
        _settings.tint = 0.0f;
    }
}

void RenderSettingsView::drawSSRSettings() {
    ImGui::Text("Screen Space Reflections");

    ImGui::Checkbox("Enable SSR", &_settings.ssrEnabled);

    if (_settings.ssrEnabled) {
        ImGui::SliderInt("Max Steps", &_settings.ssrMaxSteps, 32, 256);
        ImGui::SliderFloat("Max Distance", &_settings.ssrMaxDistance, 10.0f, 200.0f);
        ImGui::SliderFloat("Thickness", &_settings.ssrThickness, 0.1f, 2.0f);
        ImGui::SliderFloat("Roughness Threshold", &_settings.ssrRoughnessThreshold, 0.1f, 1.0f);

        if (ImGui::CollapsingHeader("Quality Presets")) {
            if (ImGui::Button("Low##ssr")) {
                _settings.ssrMaxSteps = 32;
                _settings.ssrMaxDistance = 50.0f;
            }
            ImGui::SameLine();
            if (ImGui::Button("Medium##ssr")) {
                _settings.ssrMaxSteps = 64;
                _settings.ssrMaxDistance = 100.0f;
            }
            ImGui::SameLine();
            if (ImGui::Button("High##ssr")) {
                _settings.ssrMaxSteps = 128;
                _settings.ssrMaxDistance = 150.0f;
            }
            ImGui::SameLine();
            if (ImGui::Button("Ultra##ssr")) {
                _settings.ssrMaxSteps = 256;
                _settings.ssrMaxDistance = 200.0f;
            }
        }
    }
}

void RenderSettingsView::drawShadowSettings() {
    ImGui::Text("Shadow Quality");

    // PCSS
    ImGui::Checkbox("Enable PCSS (Soft Shadows)", &_settings.pcssEnabled);
    if (_settings.pcssEnabled) {
        ImGui::SliderInt("Blocker Samples", &_settings.pcssBlockerSamples, 8, 64);
        ImGui::SliderInt("PCF Samples", &_settings.pcssPCFSamples, 16, 128);
        ImGui::SliderFloat("Light Size", &_settings.pcssLightSize, 0.001f, 0.1f);
        ImGui::SliderFloat("Min Penumbra", &_settings.pcssMinPenumbra, 0.0f, 0.5f);
    }

    ImGui::Separator();

    // Contact Shadows
    ImGui::Checkbox("Enable Contact Shadows", &_settings.contactShadowsEnabled);
    if (_settings.contactShadowsEnabled) {
        ImGui::SliderInt("Ray Steps", &_settings.contactShadowSteps, 8, 64);
        ImGui::SliderFloat("Shadow Length", &_settings.contactShadowLength, 0.1f, 2.0f);
        ImGui::SliderFloat("Fade Start", &_settings.contactShadowFadeStart, 0.5f, 1.0f);
    }

    ImGui::Separator();

    // Shadow map quality presets
    if (ImGui::CollapsingHeader("Quality Presets")) {
        if (ImGui::Button("Performance")) {
            _settings.pcssEnabled = false;
            _settings.contactShadowsEnabled = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Balanced")) {
            _settings.pcssEnabled = true;
            _settings.pcssBlockerSamples = 16;
            _settings.pcssPCFSamples = 32;
            _settings.contactShadowsEnabled = true;
            _settings.contactShadowSteps = 16;
        }
        ImGui::SameLine();
        if (ImGui::Button("Quality")) {
            _settings.pcssEnabled = true;
            _settings.pcssBlockerSamples = 32;
            _settings.pcssPCFSamples = 64;
            _settings.contactShadowsEnabled = true;
            _settings.contactShadowSteps = 32;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cinematic")) {
            _settings.pcssEnabled = true;
            _settings.pcssBlockerSamples = 64;
            _settings.pcssPCFSamples = 128;
            _settings.contactShadowsEnabled = true;
            _settings.contactShadowSteps = 64;
        }
    }
}

void RenderSettingsView::drawSpotLightSettings() {
    ImGui::Text("Spot Lights");

    ImGui::Checkbox("Enable Spot Lights", &_settings.spotLightsEnabled);

    if (_settings.spotLightsEnabled) {
        ImGui::SliderInt("Max Spot Lights", &_settings.maxSpotLights, 1, 32);
        ImGui::Checkbox("Spot Light Shadows", &_settings.spotLightShadowsEnabled);
    }

    ImGui::Separator();

    // TODO: List existing spot lights with edit controls
    ImGui::Text("Active Spot Lights: 0");

    if (ImGui::Button("Add Spot Light")) {
        // TODO: Add spot light creation
    }
}

void RenderSettingsView::drawPerformanceStats() {
    ImGui::Text("Performance Statistics");

    ImGui::Text("Frame Time: %.3f ms", m_Engine->stats.frametime * 1000.0f);
    ImGui::Text("FPS: %.1f", 1.0f / m_Engine->stats.frametime);
    ImGui::Text("Draw Calls: %d", m_Engine->stats.drawcall_count);
    ImGui::Text("Triangles: %d", m_Engine->stats.triangle_count);
    ImGui::Text("Visible Objects: %d", m_Engine->stats.visible_count);

    ImGui::Separator();

    // Estimated GPU times (would need actual timing queries)
    ImGui::Text("Estimated Pass Times:");
    ImGui::Text("  Shadows: ~%.1f ms", 0.5f);
    ImGui::Text("  SSAO: ~%.1f ms", _settings.ssaoEnabled ? 1.5f : 0.0f);
    ImGui::Text("  SSR: ~%.1f ms", _settings.ssrEnabled ? 2.0f : 0.0f);
    ImGui::Text("  Bloom: ~%.1f ms", _settings.bloomEnabled ? 0.8f : 0.0f);
    ImGui::Text("  Tone Mapping: ~%.1f ms", _settings.tonemappingEnabled ? 0.2f : 0.0f);

    ImGui::Separator();

    ImGui::Text("Memory Usage:");
    // TODO: Track actual memory usage
    ImGui::Text("  Post-process buffers: ~%.1f MB", 50.0f);
    ImGui::Text("  Shadow maps: ~%.1f MB", 32.0f);
}

void RenderSettingsView::drawPathTracerSettings() {
    ImGui::Text("GPU Path Tracer");

    // Check if path tracer is available
    if (!m_Engine->_pathTracer) {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Path tracer not initialized");
        return;
    }

    auto& settings = m_Engine->_pathTracer->settings;
    auto& stats = m_Engine->_pathTracer->stats;

    // Mode toggle - directly control view mode
    bool isPathTracedMode = (m_Engine->_currentViewMode == VulkanEngine::ViewMode::PathTraced);
    if (ImGui::Checkbox("Enable Path Tracing", &isPathTracedMode)) {
        m_Engine->_currentViewMode = isPathTracedMode
            ? VulkanEngine::ViewMode::PathTraced
            : VulkanEngine::ViewMode::Rendered;
    }

    ImGui::Separator();

    // Statistics
    ImGui::Text("Accumulated Frames: %d / %d", stats.accumulatedFrames, settings.maxAccumulatedFrames);

    float progress = static_cast<float>(stats.accumulatedFrames) / static_cast<float>(settings.maxAccumulatedFrames);
    ImGui::ProgressBar(progress, ImVec2(-1, 0), "");

    ImGui::Text("Triangles: %d", stats.triangleCount);
    ImGui::Text("BVH Nodes: %d", stats.bvhNodeCount);

    // Cubemap status
    if (m_Engine->_pathTracer->hasCubemap()) {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Cubemap: Connected");
    } else {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Cubemap: Not Connected");
    }

    if (ImGui::Button("Reset Accumulation")) {
        m_Engine->_pathTracer->resetAccumulation();
    }
    ImGui::SameLine();
    if (ImGui::Button("Rebuild BVH")) {
        m_Engine->_pathTracer->buildBVH();
    }

    ImGui::Separator();

    // Quality Settings
    ImGui::Text("Quality Settings");

    ImGui::SliderInt("Max Bounces", &settings.maxBounces, 1, 16);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Higher = better GI but slower");
    }

    ImGui::SliderInt("Samples/Pixel", &settings.samplesPerPixel, 1, 8);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Samples per frame (accumulates over time)");
    }

    ImGui::SliderInt("Max Accumulated", &settings.maxAccumulatedFrames, 64, 4096);

    ImGui::Separator();

    // Advanced Options
    if (ImGui::CollapsingHeader("Advanced")) {
        ImGui::Checkbox("Enable Accumulation", &settings.enableAccumulation);
        ImGui::Checkbox("Next Event Estimation", &settings.enableNEE);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Direct light sampling for faster convergence");
        }

        ImGui::Checkbox("Russian Roulette", &settings.enableRussianRoulette);
        if (settings.enableRussianRoulette) {
            ImGui::SliderFloat("RR Depth", &settings.russianRouletteDepth, 1.0f, 8.0f);
        }

        ImGui::SliderFloat("Exposure", &settings.exposure, 0.1f, 5.0f);
    }

    ImGui::Separator();

    // Quality Presets
    if (ImGui::CollapsingHeader("Quality Presets")) {
        if (ImGui::Button("Preview")) {
            settings.maxBounces = 2;
            settings.samplesPerPixel = 1;
            settings.maxAccumulatedFrames = 64;
            settings.enableNEE = false;
            settings.enableRussianRoulette = false;
            m_Engine->_pathTracer->resetAccumulation();
        }
        ImGui::SameLine();
        if (ImGui::Button("Low")) {
            settings.maxBounces = 4;
            settings.samplesPerPixel = 1;
            settings.maxAccumulatedFrames = 256;
            settings.enableNEE = true;
            settings.enableRussianRoulette = true;
            m_Engine->_pathTracer->resetAccumulation();
        }
        ImGui::SameLine();
        if (ImGui::Button("Medium")) {
            settings.maxBounces = 6;
            settings.samplesPerPixel = 1;
            settings.maxAccumulatedFrames = 512;
            settings.enableNEE = true;
            settings.enableRussianRoulette = true;
            m_Engine->_pathTracer->resetAccumulation();
        }
        ImGui::SameLine();
        if (ImGui::Button("High")) {
            settings.maxBounces = 8;
            settings.samplesPerPixel = 2;
            settings.maxAccumulatedFrames = 1024;
            settings.enableNEE = true;
            settings.enableRussianRoulette = true;
            m_Engine->_pathTracer->resetAccumulation();
        }
        ImGui::SameLine();
        if (ImGui::Button("Final")) {
            settings.maxBounces = 12;
            settings.samplesPerPixel = 4;
            settings.maxAccumulatedFrames = 4096;
            settings.enableNEE = true;
            settings.enableRussianRoulette = true;
            m_Engine->_pathTracer->resetAccumulation();
        }
    }
}

void RenderSettingsView::drawEnvironmentSettings() {
    // === Background Effect Selection ===
    if (ImGui::CollapsingHeader("Background Effect", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (!m_Engine->backgroundEffects.empty()) {
            bool bgChanged = false;

            for (int i = 0; i < static_cast<int>(m_Engine->backgroundEffects.size()); i++) {
                bool isSelected = (m_Engine->currentBackgroundEffect == i);
                if (ImGui::RadioButton(m_Engine->backgroundEffects[i].name, isSelected)) {
                    m_Engine->currentBackgroundEffect = i;
                    bgChanged = true;
                }
            }

            ImGui::Spacing();

            // Push constants for the current effect
            auto& effect = m_Engine->backgroundEffects[m_Engine->currentBackgroundEffect];

            if (strcmp(effect.name, "gradient") == 0) {
                if (ImGui::ColorEdit4("Top Color", &effect.data.data1.x)) bgChanged = true;
                if (ImGui::ColorEdit4("Bottom Color", &effect.data.data2.x)) bgChanged = true;
            } else if (strcmp(effect.name, "sky") == 0) {
                if (ImGui::ColorEdit4("Sky Color", &effect.data.data1.x)) bgChanged = true;
            }

            // Reset path tracer accumulation when background changes
            if (bgChanged && m_Engine->_pathTracer) {
                m_Engine->_pathTracer->resetAccumulation();
            }
        } else {
            ImGui::TextDisabled("No background effects available");
        }
    }

    ImGui::Separator();

    // === Environment Map ===
    ImGui::Text("Environment / Skybox");

    // Check if environment map is available
    if (!m_Engine->_environmentMap) {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Environment map not initialized");
        return;
    }

    auto& settings = m_Engine->_environmentMap->settings;
    auto& stats = m_Engine->_environmentMap->stats;

    // Load Cubemap from files
    if (ImGui::CollapsingHeader("Load Cubemap", ImGuiTreeNodeFlags_DefaultOpen)) {
        // Helper lambda to load a cubemap from a folder and switch to skybox background
        auto loadCubemapFromFolder = [&](const std::string& folderPath) {
            const char* extensions[] = { ".jpg", ".png", ".bmp", ".tga" };
            const char* faceNames[] = { "posx", "negx", "posy", "negy", "posz", "negz" };

            for (const char* ext : extensions) {
                std::string testPath = folderPath + "/" + faceNames[0] + ext;
                if (std::filesystem::exists(testPath)) {
                    std::string paths[6];
                    for (int i = 0; i < 6; i++) {
                        paths[i] = folderPath + "/" + faceNames[i] + ext;
                    }

                    vkDeviceWaitIdle(m_Engine->_device);

                    if (m_Engine->_environmentMap->loadCubemapFaces(paths)) {
                        if (m_Engine->_pathTracer) {
                            m_Engine->_pathTracer->setEnvironmentCubemap(
                                m_Engine->_environmentMap->getEnvironmentCubemap(),
                                m_Engine->_environmentMap->getSampler());
                            m_Engine->_pathTracer->resetAccumulation();
                        }
                        m_Engine->updateSkyboxBgDescriptor();

                        // Auto-switch to skybox background
                        for (int i = 0; i < static_cast<int>(m_Engine->backgroundEffects.size()); i++) {
                            if (strcmp(m_Engine->backgroundEffects[i].name, "skybox") == 0) {
                                m_Engine->currentBackgroundEffect = i;
                                break;
                            }
                        }
                        return true;
                    }
                    return false;
                }
            }
            return false;
        };

        // Scan assets/skyboxes/ for available cubemap folders
        const std::string skyboxDir = "../../assets/skyboxes";
        if (std::filesystem::exists(skyboxDir) && std::filesystem::is_directory(skyboxDir)) {
            for (auto& entry : std::filesystem::directory_iterator(skyboxDir)) {
                if (entry.is_directory()) {
                    std::string name = entry.path().filename().string();
                    bool isCurrent = (m_Engine->_environmentMap->stats.loadedPath == name);

                    if (isCurrent) {
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
                    }

                    if (ImGui::Button(name.c_str(), ImVec2(-1, 0))) {
                        if (loadCubemapFromFolder(entry.path().string())) {
                            m_Engine->_environmentMap->stats.loadedPath = name;
                        }
                    }

                    if (isCurrent) {
                        ImGui::PopStyleColor();
                    }
                }
            }
        } else {
            ImGui::TextDisabled("No skyboxes folder found (assets/skyboxes/)");
        }

        ImGui::Separator();

        // Custom path input
        ImGui::Text("Custom folder:");
        ImGui::InputText("##cubemappath", _cubemapPath, sizeof(_cubemapPath));
        ImGui::SameLine();
        if (ImGui::Button("Load")) {
            if (loadCubemapFromFolder(_cubemapPath)) {
                m_Engine->_environmentMap->stats.loadedPath = std::filesystem::path(_cubemapPath).filename().string();
            }
        }

        // Show current cubemap status
        if (!m_Engine->_environmentMap->stats.loadedPath.empty()) {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Active: %s",
                m_Engine->_environmentMap->stats.loadedPath.c_str());
        }
    }

    ImGui::Separator();

    // Sky Colors
    if (ImGui::CollapsingHeader("Sky Colors", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::ColorEdit3("Sky Top", &settings.skyColorTop.x);
        ImGui::ColorEdit3("Horizon", &settings.skyColorHorizon.x);
        ImGui::ColorEdit3("Ground", &settings.groundColor.x);
        ImGui::SliderFloat("Sky Intensity", &settings.skyIntensity, 0.1f, 5.0f);

        if (ImGui::Button("Apply Sky Colors")) {
            m_Engine->_environmentMap->generateProceduralSky();
            // Re-connect cubemap to path tracer and reset accumulation
            if (m_Engine->_pathTracer) {
                m_Engine->_pathTracer->setEnvironmentCubemap(
                    m_Engine->_environmentMap->getEnvironmentCubemap(),
                    m_Engine->_environmentMap->getSampler());
                m_Engine->_pathTracer->resetAccumulation();
            }
            // Update skybox background descriptor with new cubemap
            m_Engine->updateSkyboxBgDescriptor();
        }
    }

    ImGui::Separator();

    // IBL Settings
    if (ImGui::CollapsingHeader("Image-Based Lighting")) {
        ImGui::Checkbox("Enable IBL", &settings.useIBL);
        if (settings.useIBL) {
            ImGui::SliderFloat("IBL Intensity", &settings.iblIntensity, 0.0f, 3.0f);
        }

        ImGui::SliderFloat("Exposure", &settings.exposure, 0.1f, 5.0f);
        ImGui::SliderFloat("Rotation", &settings.rotation, 0.0f, 6.28318f, "%.2f rad");
    }

    ImGui::Separator();

    // Statistics
    if (ImGui::CollapsingHeader("Statistics")) {
        ImGui::Text("Cubemap Size: %dx%d", stats.cubemapSize, stats.cubemapSize);
        ImGui::Text("Irradiance Size: %dx%d", stats.irradianceSize, stats.irradianceSize);
        ImGui::Text("Prefiltered Mips: %d", stats.prefilteredMipLevels);
        ImGui::Text("HDR: %s", stats.isHDR ? "Yes" : "No");
        if (!stats.loadedPath.empty()) {
            ImGui::Text("Loaded: %s", stats.loadedPath.c_str());
        }
    }

    ImGui::Separator();

    // Presets
    if (ImGui::CollapsingHeader("Sky Presets")) {
        // Helper to apply preset and update path tracer + skybox background
        auto applyPreset = [&]() {
            m_Engine->_environmentMap->generateProceduralSky();
            if (m_Engine->_pathTracer) {
                m_Engine->_pathTracer->setEnvironmentCubemap(
                    m_Engine->_environmentMap->getEnvironmentCubemap(),
                    m_Engine->_environmentMap->getSampler());
                m_Engine->_pathTracer->resetAccumulation();
            }
            m_Engine->updateSkyboxBgDescriptor();
        };

        if (ImGui::Button("Clear Day")) {
            settings.skyColorTop = glm::vec3(0.4f, 0.6f, 1.0f);
            settings.skyColorHorizon = glm::vec3(0.7f, 0.8f, 0.95f);
            settings.groundColor = glm::vec3(0.4f, 0.35f, 0.3f);
            settings.skyIntensity = 1.0f;
            applyPreset();
        }
        ImGui::SameLine();
        if (ImGui::Button("Sunset")) {
            settings.skyColorTop = glm::vec3(0.2f, 0.3f, 0.5f);
            settings.skyColorHorizon = glm::vec3(1.0f, 0.5f, 0.2f);
            settings.groundColor = glm::vec3(0.15f, 0.1f, 0.1f);
            settings.skyIntensity = 1.2f;
            applyPreset();
        }
        ImGui::SameLine();
        if (ImGui::Button("Overcast")) {
            settings.skyColorTop = glm::vec3(0.5f, 0.55f, 0.6f);
            settings.skyColorHorizon = glm::vec3(0.6f, 0.62f, 0.65f);
            settings.groundColor = glm::vec3(0.35f, 0.3f, 0.28f);
            settings.skyIntensity = 0.8f;
            applyPreset();
        }

        if (ImGui::Button("Night")) {
            settings.skyColorTop = glm::vec3(0.02f, 0.03f, 0.08f);
            settings.skyColorHorizon = glm::vec3(0.05f, 0.06f, 0.1f);
            settings.groundColor = glm::vec3(0.02f, 0.02f, 0.02f);
            settings.skyIntensity = 0.3f;
            applyPreset();
        }
        ImGui::SameLine();
        if (ImGui::Button("Studio")) {
            settings.skyColorTop = glm::vec3(0.18f, 0.18f, 0.2f);
            settings.skyColorHorizon = glm::vec3(0.2f, 0.2f, 0.22f);
            settings.groundColor = glm::vec3(0.1f, 0.1f, 0.1f);
            settings.skyIntensity = 0.5f;
            applyPreset();
        }
    }
}

} // namespace Yalaz::UI
