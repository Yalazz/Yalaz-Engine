#pragma once

#include "PostProcess.h"
#include <array>
#include <random>

namespace Yalaz::Renderer {

// =============================================================================
// SSAO PASS - Screen Space Ambient Occlusion using HBAO+ style algorithm
// =============================================================================
// High-quality ambient occlusion using horizon-based sampling with:
// - Hemisphere sampling in view space
// - Depth-aware bilateral blur for clean edges
// - Temporal stability options
// Based on NVIDIA HBAO+ and Crytek SSAO techniques.
// =============================================================================

class SSAOPass : public PostProcessPass {
public:
    static constexpr int MAX_KERNEL_SIZE = 64;
    static constexpr int NOISE_SIZE = 4;

    struct Settings {
        bool enabled = true;
        int samples = 32;              // Number of samples (8-64)
        float radius = 0.5f;           // Sampling radius in world units
        float intensity = 1.0f;        // AO intensity multiplier
        float bias = 0.025f;           // Depth bias to prevent self-occlusion
        float power = 2.0f;            // Power curve for contrast
        int blurPasses = 2;            // Number of bilateral blur passes
        float blurSharpness = 4.0f;    // Edge-aware blur sharpness
        bool halfResolution = false;   // Render at half resolution for performance
        float maxDistance = 100.0f;    // Max distance for AO (fade out)
        float fadeStart = 50.0f;       // Distance where AO starts fading
    };
    Settings settings;

    SSAOPass(VulkanEngine* engine);
    ~SSAOPass() override;

    void init() override;
    void cleanup() override;
    void execute(VkCommandBuffer cmd, AllocatedImage& input, AllocatedImage& output) override;
    void onResize(VkExtent2D newExtent) override;

    // Get the SSAO texture for use in main rendering
    AllocatedImage& getSSAOTexture() { return _ssaoBuffer; }

private:
    // SSAO kernel and noise
    std::array<glm::vec4, MAX_KERNEL_SIZE> _ssaoKernel;
    std::array<glm::vec4, NOISE_SIZE * NOISE_SIZE> _ssaoNoise;

    // GPU resources
    AllocatedImage _ssaoBuffer;           // Raw SSAO output
    AllocatedImage _ssaoBlurBuffer;       // Blurred SSAO
    AllocatedImage _noiseTexture;         // Random rotation vectors
    AllocatedBuffer _kernelBuffer;        // SSAO kernel uniform buffer

    // Pipelines
    VkPipeline _ssaoPipeline = VK_NULL_HANDLE;
    VkPipeline _blurPipeline = VK_NULL_HANDLE;
    VkPipelineLayout _ssaoPipelineLayout = VK_NULL_HANDLE;
    VkPipelineLayout _blurPipelineLayout = VK_NULL_HANDLE;

    // Descriptors
    VkDescriptorSetLayout _ssaoDescriptorLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout _blurDescriptorLayout = VK_NULL_HANDLE;
    VkDescriptorSet _ssaoDescriptorSet = VK_NULL_HANDLE;
    VkDescriptorSet _blurDescriptorSetH = VK_NULL_HANDLE;  // Horizontal blur
    VkDescriptorSet _blurDescriptorSetV = VK_NULL_HANDLE;  // Vertical blur

    VkSampler _pointSampler = VK_NULL_HANDLE;
    VkSampler _linearSampler = VK_NULL_HANDLE;

    VkExtent2D _ssaoExtent;

    // Push constants
    struct SSAOPushConstants {
        glm::mat4 projection;
        glm::mat4 invProjection;
        glm::vec4 params;    // x=radius, y=bias, z=intensity, w=power
        glm::vec4 params2;   // x=samples, y=maxDistance, z=fadeStart, w=noiseScale
        glm::vec2 resolution;
        glm::vec2 invResolution;
    };

    struct BlurPushConstants {
        glm::vec2 direction;  // (1,0) for horizontal, (0,1) for vertical
        float sharpness;
        float _pad;
    };

    void generateKernel();
    void generateNoise();
    void createBuffers(VkExtent2D extent);
    void destroyBuffers();
    void createPipelines();
    void createDescriptors();
    void updateDescriptors(AllocatedImage& depthImage, AllocatedImage& normalImage);

    void renderSSAO(VkCommandBuffer cmd, AllocatedImage& depthImage, AllocatedImage& normalImage);
    void blurSSAO(VkCommandBuffer cmd);
};

} // namespace Yalaz::Renderer
