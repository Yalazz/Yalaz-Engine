#pragma once

#include "PostProcess.h"
#include <array>

namespace Yalaz::Renderer {

// =============================================================================
// BLOOM PASS - Multi-stage HDR bloom effect using compute shaders
// =============================================================================
// Implementation uses progressive downsampling with 13-tap filter (Karis average
// for first downsample to reduce fireflies), then upsampling with tent filter.
// Based on Call of Duty: Advanced Warfare and Unreal Engine 4 bloom techniques.
// =============================================================================

class BloomPass : public PostProcessPass {
public:
    static constexpr int MAX_MIP_LEVELS = 8;

    struct Settings {
        float threshold = 1.0f;        // Brightness threshold for bloom
        float softThreshold = 0.5f;    // Soft knee for smooth transition
        float intensity = 0.5f;        // Final bloom intensity
        float radius = 1.0f;           // Blur radius multiplier
        int mipLevels = 6;             // Number of downsample passes (2-8)
        bool enabled = true;
    };
    Settings settings;

    BloomPass(VulkanEngine* engine);
    ~BloomPass() override;

    void init() override;
    void cleanup() override;
    void execute(VkCommandBuffer cmd, AllocatedImage& input, AllocatedImage& output) override;
    void onResize(VkExtent2D newExtent) override;

private:
    // Mip chain for bloom processing
    struct MipLevel {
        AllocatedImage image;
        VkExtent2D extent;
    };
    std::array<MipLevel, MAX_MIP_LEVELS> _mipChain;
    int _actualMipLevels = 0;

    // Compute pipelines
    VkPipeline _downsamplePipeline = VK_NULL_HANDLE;
    VkPipeline _downsampleKarisPipeline = VK_NULL_HANDLE;  // First pass with Karis average
    VkPipeline _upsamplePipeline = VK_NULL_HANDLE;
    VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;

    // Descriptor sets for each mip level
    VkDescriptorSetLayout _descriptorLayout = VK_NULL_HANDLE;
    std::array<VkDescriptorSet, MAX_MIP_LEVELS> _downsampleDescriptors;
    std::array<VkDescriptorSet, MAX_MIP_LEVELS> _upsampleDescriptors;

    // Sampler for texture sampling
    VkSampler _linearSampler = VK_NULL_HANDLE;

    // Push constants
    struct BloomPushConstants {
        float threshold;
        float softThreshold;
        float filterRadius;
        int srcMipLevel;
    };

    void createMipChain(VkExtent2D baseExtent);
    void destroyMipChain();
    void createPipelines();
    void createDescriptors();
    void updateDescriptors(AllocatedImage& input, AllocatedImage& output);

    void downsample(VkCommandBuffer cmd, AllocatedImage& input);
    void upsample(VkCommandBuffer cmd, AllocatedImage& output);
};

} // namespace Yalaz::Renderer
