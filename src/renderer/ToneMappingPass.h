#pragma once

#include "PostProcess.h"

namespace Yalaz::Renderer {

// =============================================================================
// TONE MAPPING PASS - HDR to LDR conversion with color grading
// =============================================================================
// Implements multiple tone mapping operators:
// - ACES Filmic (Academy Color Encoding System)
// - Reinhard (simple, classic)
// - Uncharted 2 (filmic, good for games)
// - Neutral (minimal manipulation)
// Plus full color grading pipeline for professional color control.
// =============================================================================

class ToneMappingPass : public PostProcessPass {
public:
    enum class Operator {
        ACES = 0,
        Reinhard = 1,
        Uncharted2 = 2,
        Neutral = 3,
        AgX = 4,  // Blender-style AgX
        Count
    };

    struct Settings {
        Operator tonemapOperator = Operator::ACES;
        float exposure = 1.0f;
        float gamma = 2.2f;

        // Color grading
        float contrast = 1.0f;
        float saturation = 1.0f;
        float brightness = 0.0f;

        // White balance
        float temperature = 0.0f;  // -1.0 (cool) to 1.0 (warm)
        float tint = 0.0f;         // -1.0 (green) to 1.0 (magenta)

        // Split toning
        glm::vec3 shadowTint = glm::vec3(0.0f);
        glm::vec3 highlightTint = glm::vec3(0.0f);
        float shadowTintStrength = 0.0f;
        float highlightTintStrength = 0.0f;

        // Vignette
        bool vignetteEnabled = false;
        float vignetteIntensity = 0.3f;
        float vignetteSmoothness = 0.5f;
        float vignetteRoundness = 1.0f;

        // Chromatic aberration
        bool chromaticAberrationEnabled = false;
        float chromaticAberrationIntensity = 0.5f;

        // Film grain
        bool filmGrainEnabled = false;
        float filmGrainIntensity = 0.1f;
        float filmGrainResponse = 0.5f;

        // Dithering (reduce banding)
        bool ditheringEnabled = true;

        bool enabled = true;
    };
    Settings settings;

    ToneMappingPass(VulkanEngine* engine);
    ~ToneMappingPass() override;

    void init() override;
    void cleanup() override;
    void execute(VkCommandBuffer cmd, AllocatedImage& input, AllocatedImage& output) override;
    void onResize(VkExtent2D newExtent) override;

private:
    VkPipeline _pipeline = VK_NULL_HANDLE;
    VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout _descriptorLayout = VK_NULL_HANDLE;
    VkDescriptorSet _descriptorSet = VK_NULL_HANDLE;
    VkSampler _linearSampler = VK_NULL_HANDLE;

    // Push constants for tone mapping
    struct TonemapPushConstants {
        // Basic settings
        int tonemapOperator;
        float exposure;
        float gamma;
        float contrast;

        // Color grading
        float saturation;
        float brightness;
        float temperature;
        float tint;

        // Split toning
        glm::vec4 shadowTint;     // rgb + strength
        glm::vec4 highlightTint;  // rgb + strength

        // Vignette
        int vignetteEnabled;
        float vignetteIntensity;
        float vignetteSmoothness;
        float vignetteRoundness;

        // Film effects
        int chromaticAberrationEnabled;
        float chromaticAberrationIntensity;
        int filmGrainEnabled;
        float filmGrainIntensity;

        // Other
        float filmGrainResponse;
        int ditheringEnabled;
        float time;  // For animated grain
        float _pad;
    };

    float _time = 0.0f;

    void createPipeline();
    void updateDescriptors(AllocatedImage& input, AllocatedImage& output);
};

} // namespace Yalaz::Renderer
