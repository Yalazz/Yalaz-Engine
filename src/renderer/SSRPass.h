#pragma once

#include "PostProcess.h"

namespace Yalaz::Renderer {

// =============================================================================
// SSR PASS - Screen Space Reflections using hierarchical ray marching
// =============================================================================
// High-quality screen space reflections using:
// - Hi-Z (hierarchical Z-buffer) accelerated ray marching
// - Roughness-based cone tracing approximation
// - Temporal reprojection for stability
// - Edge fade and sky fallback
// Based on techniques from Stingray Engine and Frostbite.
// =============================================================================

class SSRPass : public PostProcessPass {
public:
    struct Settings {
        bool enabled = false;
        int maxSteps = 128;              // Maximum ray march steps
        int binarySearchSteps = 8;       // Binary search refinement steps
        float maxDistance = 100.0f;      // Max reflection distance
        float thickness = 0.5f;          // Depth thickness for hit detection
        float stride = 1.0f;             // Initial step size multiplier
        float strideZCutoff = 100.0f;    // Z distance to switch to larger stride
        float roughnessThreshold = 0.5f; // Don't reflect surfaces rougher than this
        float fadeDist = 5.0f;           // Edge fade distance
        float intensity = 1.0f;          // Reflection intensity
        bool temporalFilter = true;      // Enable temporal filtering
        float temporalWeight = 0.9f;     // Weight of previous frame
    };
    Settings settings;

    SSRPass(VulkanEngine* engine);
    ~SSRPass() override;

    void init() override;
    void cleanup() override;
    void execute(VkCommandBuffer cmd, AllocatedImage& input, AllocatedImage& output) override;
    void onResize(VkExtent2D newExtent) override;

    // Get reflection buffer for compositing
    AllocatedImage& getReflectionBuffer() { return _reflectionBuffer; }

private:
    // Reflection output buffer
    AllocatedImage _reflectionBuffer;
    AllocatedImage _prevReflectionBuffer;  // For temporal filtering

    // Hi-Z buffer (depth mip chain)
    AllocatedImage _hiZBuffer;
    int _hiZMipLevels = 0;

    // Pipelines
    VkPipeline _ssrPipeline = VK_NULL_HANDLE;
    VkPipeline _hiZPipeline = VK_NULL_HANDLE;
    VkPipeline _temporalPipeline = VK_NULL_HANDLE;
    VkPipelineLayout _ssrPipelineLayout = VK_NULL_HANDLE;
    VkPipelineLayout _hiZPipelineLayout = VK_NULL_HANDLE;
    VkPipelineLayout _temporalPipelineLayout = VK_NULL_HANDLE;

    // Descriptors
    VkDescriptorSetLayout _ssrDescriptorLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout _hiZDescriptorLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout _temporalDescriptorLayout = VK_NULL_HANDLE;
    VkDescriptorSet _ssrDescriptorSet = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> _hiZDescriptorSets;  // One per mip level
    VkDescriptorSet _temporalDescriptorSet = VK_NULL_HANDLE;

    VkSampler _pointSampler = VK_NULL_HANDLE;
    VkSampler _linearSampler = VK_NULL_HANDLE;

    VkExtent2D _extent;

    // Push constants
    struct SSRPushConstants {
        glm::mat4 projection;
        glm::mat4 invProjection;
        glm::mat4 view;
        glm::mat4 invView;
        glm::vec4 params;     // x=maxSteps, y=maxDist, z=thickness, w=stride
        glm::vec4 params2;    // x=roughnessThreshold, y=fadeDist, z=intensity, w=strideZCutoff
        glm::vec4 params3;    // x=binarySteps, y=screenWidth, z=screenHeight, w=hiZMips
        glm::vec4 cameraPos;
    };

    struct HiZPushConstants {
        int srcMipLevel;
        int _pad[3];
    };

    struct TemporalPushConstants {
        glm::mat4 prevViewProj;
        glm::mat4 invViewProj;
        float temporalWeight;
        float _pad[3];
    };

    glm::mat4 _prevViewProj;

    void createBuffers(VkExtent2D extent);
    void destroyBuffers();
    void createPipelines();
    void createDescriptors();

    void buildHiZ(VkCommandBuffer cmd, AllocatedImage& depthImage);
    void traceReflections(VkCommandBuffer cmd, AllocatedImage& colorImage, AllocatedImage& depthImage,
                          AllocatedImage& normalImage, AllocatedImage& roughnessImage);
    void temporalFilter(VkCommandBuffer cmd);
};

} // namespace Yalaz::Renderer
