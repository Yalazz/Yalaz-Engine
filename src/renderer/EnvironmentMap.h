#pragma once

#include <vk_types.h>
#include <string>
#include <glm/glm.hpp>

class VulkanEngine;

namespace Yalaz::Renderer {

// =============================================================================
// ENVIRONMENT MAP - Skybox and IBL (Image-Based Lighting) system
// =============================================================================
// Features:
// - HDR environment map loading (equirectangular or cubemap)
// - Procedural sky generation
// - Irradiance map for diffuse IBL
// - Pre-filtered environment map for specular IBL
// - BRDF LUT generation
// =============================================================================

struct EnvironmentSettings {
    // Sky gradient (used when no cubemap is loaded)
    glm::vec3 skyColorTop = glm::vec3(0.5f, 0.7f, 1.0f);      // Zenith
    glm::vec3 skyColorHorizon = glm::vec3(0.8f, 0.85f, 0.9f); // Horizon
    glm::vec3 groundColor = glm::vec3(0.3f, 0.25f, 0.2f);     // Ground

    float skyIntensity = 1.0f;
    float exposure = 1.0f;
    float rotation = 0.0f;  // Y-axis rotation in radians

    // IBL settings
    bool useIBL = true;
    float iblIntensity = 1.0f;
    int irradianceSamples = 64;
    int prefilterSamples = 1024;
};

class EnvironmentMap {
public:
    EnvironmentMap(VulkanEngine* engine);
    ~EnvironmentMap();

    void init();
    void cleanup();

    // Load HDR environment map from file (equirectangular)
    bool loadFromFile(const std::string& path);

    // Load cubemap from 6 face files
    bool loadCubemapFaces(const std::string paths[6]);

    // Generate procedural sky
    void generateProceduralSky();

    // Render skybox (call during main render pass)
    void renderSkybox(VkCommandBuffer cmd, const glm::mat4& viewProj);

    // Get textures for PBR shaders
    VkImageView getEnvironmentCubemap() const { return _envCubemap.imageView; }
    VkImageView getIrradianceMap() const { return _irradianceMap.imageView; }
    VkImageView getPrefilteredMap() const { return _prefilteredMap.imageView; }
    VkImageView getBRDFLut() const { return _brdfLUT.imageView; }
    VkSampler getSampler() const { return _cubemapSampler; }

    EnvironmentSettings settings;

    // Statistics
    struct Stats {
        uint32_t cubemapSize = 0;
        uint32_t irradianceSize = 0;
        uint32_t prefilteredMipLevels = 0;
        bool isHDR = false;
        std::string loadedPath;
    };
    Stats stats;

private:
    VulkanEngine* _engine;

    // Cubemap textures
    AllocatedImage _envCubemap;          // Main environment cubemap
    AllocatedImage _irradianceMap;       // Diffuse IBL (low-res)
    AllocatedImage _prefilteredMap;      // Specular IBL (mipmapped roughness)
    AllocatedImage _brdfLUT;             // Pre-computed BRDF integration LUT

    // Samplers
    VkSampler _cubemapSampler = VK_NULL_HANDLE;
    VkSampler _brdfSampler = VK_NULL_HANDLE;

    // Skybox rendering pipeline
    VkPipeline _skyboxPipeline = VK_NULL_HANDLE;
    VkPipelineLayout _skyboxPipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout _skyboxDescriptorLayout = VK_NULL_HANDLE;
    VkDescriptorSet _skyboxDescriptorSet = VK_NULL_HANDLE;

    // Equirectangular to cubemap conversion
    VkPipeline _equiToCubePipeline = VK_NULL_HANDLE;
    VkPipelineLayout _equiToCubePipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout _equiToCubeDescriptorLayout = VK_NULL_HANDLE;

    // Irradiance convolution pipeline
    VkPipeline _irradiancePipeline = VK_NULL_HANDLE;
    VkPipelineLayout _irradiancePipelineLayout = VK_NULL_HANDLE;

    // Prefilter pipeline
    VkPipeline _prefilterPipeline = VK_NULL_HANDLE;
    VkPipelineLayout _prefilterPipelineLayout = VK_NULL_HANDLE;

    // BRDF LUT generation pipeline
    VkPipeline _brdfPipeline = VK_NULL_HANDLE;
    VkPipelineLayout _brdfPipelineLayout = VK_NULL_HANDLE;

    bool _initialized = false;

    void createSamplers();
    void createCubemap(uint32_t size, VkFormat format, bool mipmapped = false);
    void createIrradianceMap();
    void createPrefilteredMap();
    void createBRDFLUT();
    void createPipelines();
    void createSkyboxPipeline();

    void convertEquirectangularToCubemap(AllocatedImage& equirectangular);
    void generateIrradianceMap();
    void generatePrefilteredMap();
    void generateBRDFLUT();
};

} // namespace Yalaz::Renderer
