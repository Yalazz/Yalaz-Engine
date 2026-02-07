#pragma once

#include <vk_types.h>
#include <vector>
#include <glm/glm.hpp>

class VulkanEngine;

namespace Yalaz::Renderer {

// =============================================================================
// PATH TRACER - Real-time GPU path tracing with BVH acceleration
// =============================================================================
// Professional-grade path tracer featuring:
// - BVH acceleration structure for fast ray-scene intersection
// - Multiple bounce global illumination
// - PBR material evaluation (Cook-Torrance BRDF)
// - Importance sampling for lights and materials
// - Temporal accumulation for progressive rendering
// - Optional denoising support
// =============================================================================

// GPU BVH node (32 bytes)
struct GPUBVHNode {
    glm::vec3 boundsMin;
    uint32_t leftFirst;     // If leaf: first primitive index, else: left child
    glm::vec3 boundsMax;
    uint32_t primCount;     // If 0: internal node, else: leaf with primCount primitives
};
static_assert(sizeof(GPUBVHNode) == 32, "GPUBVHNode must be 32 bytes");

// GPU Triangle (48 bytes)
struct GPUTriangle {
    glm::vec3 v0;
    uint32_t materialIndex;
    glm::vec3 v1;
    float _pad0;
    glm::vec3 v2;
    float _pad1;
};
static_assert(sizeof(GPUTriangle) == 48, "GPUTriangle must be 48 bytes");

// GPU Material for path tracing (64 bytes)
struct GPUPathTraceMaterial {
    glm::vec3 albedo;
    float metallic;
    glm::vec3 emission;
    float roughness;
    float ior;              // Index of refraction
    float transmission;     // 0 = opaque, 1 = fully transparent
    uint32_t albedoTexture; // Texture index (UINT32_MAX = none)
    uint32_t normalTexture;
};
static_assert(sizeof(GPUPathTraceMaterial) == 48, "GPUPathTraceMaterial must be 48 bytes");

// Path tracer settings
struct PathTracerSettings {
    int maxBounces = 4;
    int samplesPerPixel = 1;    // Samples per frame (accumulates over time)
    int maxAccumulatedFrames = 1024;
    bool enableAccumulation = true;
    bool enableNEE = true;      // Next Event Estimation (direct light sampling)
    bool enableRussianRoulette = true;
    float russianRouletteDepth = 3;
    bool enableDenoising = false;
    float exposure = 1.0f;
    float skyIntensity = 1.0f;
    glm::vec3 skyColor = glm::vec3(0.5f, 0.7f, 1.0f);
};

class PathTracer {
public:
    PathTracer(VulkanEngine* engine);
    ~PathTracer();

    void init();
    void cleanup();

    // Build acceleration structure from scene
    void buildBVH();

    // Render one frame (accumulates with previous frames)
    void render(VkCommandBuffer cmd);

    // Reset accumulation (call when camera moves)
    void resetAccumulation();

    // Set the engine's draw image for direct path tracer output
    void setDrawImage(VkImageView drawImageView);

    // Set environment cubemap for sky sampling
    void setEnvironmentCubemap(VkImageView cubemapView, VkSampler cubemapSampler);

    // Notify that scene geometry changed (load/unload) - triggers BVH rebuild
    void notifySceneChanged();

    // Check and perform pending BVH rebuild (call OUTSIDE command recording)
    void processPendingRebuild();

    // Check if environment cubemap is connected
    bool hasCubemap() const { return _envCubemapView != VK_NULL_HANDLE && _envCubemapSampler != VK_NULL_HANDLE; }

    // Settings
    PathTracerSettings settings;

    // Statistics
    struct Stats {
        uint32_t triangleCount = 0;
        uint32_t bvhNodeCount = 0;
        uint32_t accumulatedFrames = 0;
        float lastFrameTimeMs = 0.0f;
    };
    Stats stats;

private:
    VulkanEngine* _engine;

    // Draw image view (owned by engine, writes path traced geometry directly)
    VkImageView _drawImageView = VK_NULL_HANDLE;

    // Accumulation buffer for temporal averaging
    AllocatedImage _accumulationImage;

    // Scene data buffers
    AllocatedBuffer _triangleBuffer;
    AllocatedBuffer _bvhBuffer;
    AllocatedBuffer _materialBuffer;

    // Compute pipeline
    VkPipeline _pathTracePipeline = VK_NULL_HANDLE;
    VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout _descriptorLayout = VK_NULL_HANDLE;
    VkDescriptorSet _descriptorSet = VK_NULL_HANDLE;

    // Push constants
    struct PathTracePushConstants {
        glm::mat4 invView;
        glm::mat4 invProj;
        glm::vec4 cameraPos;
        glm::vec4 sunDirection;     // xyz = dir, w = intensity
        glm::vec4 sunColor;
        glm::vec4 skyColor;         // xyz = color, w = intensity
        uint32_t frameIndex;
        uint32_t accumulatedFrames;
        uint32_t triangleCount;
        uint32_t maxBounces;
        uint32_t samplesPerPixel;
        uint32_t enableNEE;
        uint32_t enableRR;
        float rrDepth;
        uint32_t useCubemap;
    };

    uint32_t _frameIndex = 0;
    glm::mat4 _lastViewMatrix;
    bool _imagesInitialized = false;
    bool _needsBVHRebuild = true;  // Build on first use
    VkExtent2D _imageExtent = {0, 0};

    void createImages();
    void createBuffers();
    void createPipeline();
    void updateDescriptors();
    void ensureImagesReady();
    void uploadSceneData();

    // Environment cubemap references (owned by EnvironmentMap)
    VkImageView _envCubemapView = VK_NULL_HANDLE;
    VkSampler _envCubemapSampler = VK_NULL_HANDLE;

    // Scene data (CPU side, used for BVH building)
    std::vector<GPUTriangle> _triangles;
    std::vector<GPUPathTraceMaterial> _materials;

    // BVH building
    struct BVHBuildNode {
        glm::vec3 boundsMin, boundsMax;
        int leftFirst, primCount;
    };
    std::vector<BVHBuildNode> _bvhNodes;
    std::vector<uint32_t> _primitiveIndices;

    void buildBVHRecursive(uint32_t nodeIdx, uint32_t start, uint32_t count);
    void updateNodeBounds(uint32_t nodeIdx);
    float evaluateSAH(uint32_t nodeIdx, int axis, float pos);
};

} // namespace Yalaz::Renderer
