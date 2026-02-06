#include "EnvironmentMap.h"
#include "vk_engine.h"
#include "vk_initializers.h"
#include "vk_pipelines.h"
#include <fmt/core.h>
#include <glm/gtc/matrix_transform.hpp>

namespace Yalaz::Renderer {

EnvironmentMap::EnvironmentMap(VulkanEngine* engine)
    : _engine(engine) {}

EnvironmentMap::~EnvironmentMap() {
    cleanup();
}

void EnvironmentMap::init() {
    if (_initialized) return;

    fmt::print("[Environment] Initializing environment map system...\n");

    createSamplers();
    createBRDFLUT();
    generateProceduralSky();
    createSkyboxPipeline();

    _initialized = true;
    fmt::print("[Environment] Environment map system initialized\n");
}

void EnvironmentMap::cleanup() {
    if (!_initialized) return;

    vkDeviceWaitIdle(_engine->_device);

    // Destroy images
    if (_envCubemap.image != VK_NULL_HANDLE) {
        _engine->destroy_image(_envCubemap);
        _envCubemap = {};
    }
    if (_irradianceMap.image != VK_NULL_HANDLE) {
        _engine->destroy_image(_irradianceMap);
        _irradianceMap = {};
    }
    if (_prefilteredMap.image != VK_NULL_HANDLE) {
        _engine->destroy_image(_prefilteredMap);
        _prefilteredMap = {};
    }
    if (_brdfLUT.image != VK_NULL_HANDLE) {
        _engine->destroy_image(_brdfLUT);
        _brdfLUT = {};
    }

    // Destroy samplers
    if (_cubemapSampler != VK_NULL_HANDLE) {
        vkDestroySampler(_engine->_device, _cubemapSampler, nullptr);
        _cubemapSampler = VK_NULL_HANDLE;
    }
    if (_brdfSampler != VK_NULL_HANDLE) {
        vkDestroySampler(_engine->_device, _brdfSampler, nullptr);
        _brdfSampler = VK_NULL_HANDLE;
    }

    // Destroy pipelines
    if (_skyboxPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(_engine->_device, _skyboxPipeline, nullptr);
        _skyboxPipeline = VK_NULL_HANDLE;
    }
    if (_skyboxPipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(_engine->_device, _skyboxPipelineLayout, nullptr);
        _skyboxPipelineLayout = VK_NULL_HANDLE;
    }
    if (_skyboxDescriptorLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(_engine->_device, _skyboxDescriptorLayout, nullptr);
        _skyboxDescriptorLayout = VK_NULL_HANDLE;
    }

    _initialized = false;
}

void EnvironmentMap::createSamplers() {
    // Cubemap sampler with trilinear filtering
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = 16.0f;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;

    VK_CHECK(vkCreateSampler(_engine->_device, &samplerInfo, nullptr, &_cubemapSampler));

    // BRDF LUT sampler (no mipmaps, clamp)
    VkSamplerCreateInfo brdfSamplerInfo{};
    brdfSamplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    brdfSamplerInfo.magFilter = VK_FILTER_LINEAR;
    brdfSamplerInfo.minFilter = VK_FILTER_LINEAR;
    brdfSamplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    brdfSamplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    brdfSamplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    brdfSamplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    brdfSamplerInfo.maxLod = 1.0f;

    VK_CHECK(vkCreateSampler(_engine->_device, &brdfSamplerInfo, nullptr, &_brdfSampler));
}

void EnvironmentMap::createCubemap(uint32_t size, VkFormat format, bool mipmapped) {
    // Calculate mip levels
    uint32_t mipLevels = mipmapped
        ? static_cast<uint32_t>(std::floor(std::log2(size))) + 1
        : 1;

    // Create cubemap image
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = format;
    imageInfo.extent = { size, size, 1 };
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = 6;  // 6 faces
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    VK_CHECK(vmaCreateImage(_engine->_allocator, &imageInfo, &allocInfo,
        &_envCubemap.image, &_envCubemap.allocation, nullptr));

    _envCubemap.imageFormat = format;
    _envCubemap.imageExtent = { size, size, 1 };

    // Create cubemap view
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = _envCubemap.image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 6;

    VK_CHECK(vkCreateImageView(_engine->_device, &viewInfo, nullptr, &_envCubemap.imageView));

    stats.cubemapSize = size;
}

void EnvironmentMap::createBRDFLUT() {
    const uint32_t LUT_SIZE = 512;

    // Create BRDF LUT image
    VkExtent3D extent = { LUT_SIZE, LUT_SIZE, 1 };
    _brdfLUT = _engine->create_image(
        extent,
        VK_FORMAT_R16G16_SFLOAT,
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT
    );

    // Generate BRDF LUT using compute shader (if available)
    // For now, use a fallback solid color
    // TODO: Implement BRDF LUT generation compute shader

    fmt::print("[Environment] BRDF LUT created ({}x{})\n", LUT_SIZE, LUT_SIZE);
}

void EnvironmentMap::generateProceduralSky() {
    const uint32_t SIZE = 512;  // Cubemap face size

    // Create the cubemap
    createCubemap(SIZE, VK_FORMAT_R16G16B16A16_SFLOAT, true);

    // Generate procedural sky gradient for each face
    // We'll use a compute shader to generate this
    // For now, we'll create a simple gradient in CPU and upload

    std::vector<glm::vec4> faceData(SIZE * SIZE);

    // Generate sky gradient on CPU (temporary - will use compute shader later)
    auto generateFace = [&](int face) -> std::vector<glm::vec4> {
        std::vector<glm::vec4> data(SIZE * SIZE);

        // Direction vectors for each face
        glm::vec3 forward, up, right;
        switch(face) {
            case 0: forward = glm::vec3(1, 0, 0);  up = glm::vec3(0, 1, 0);  break; // +X
            case 1: forward = glm::vec3(-1, 0, 0); up = glm::vec3(0, 1, 0);  break; // -X
            case 2: forward = glm::vec3(0, 1, 0);  up = glm::vec3(0, 0, -1); break; // +Y
            case 3: forward = glm::vec3(0, -1, 0); up = glm::vec3(0, 0, 1);  break; // -Y
            case 4: forward = glm::vec3(0, 0, 1);  up = glm::vec3(0, 1, 0);  break; // +Z
            case 5: forward = glm::vec3(0, 0, -1); up = glm::vec3(0, 1, 0);  break; // -Z
        }
        right = glm::cross(up, forward);

        for (uint32_t y = 0; y < SIZE; ++y) {
            for (uint32_t x = 0; x < SIZE; ++x) {
                // Map pixel to direction
                float u = (float(x) + 0.5f) / float(SIZE) * 2.0f - 1.0f;
                float v = (float(y) + 0.5f) / float(SIZE) * 2.0f - 1.0f;

                glm::vec3 dir = glm::normalize(forward + right * u + up * (-v));

                // Simple atmospheric scattering approximation
                float elevation = dir.y;

                glm::vec3 color;
                if (elevation > 0.0f) {
                    // Sky gradient
                    float t = std::pow(elevation, 0.4f);
                    color = glm::mix(settings.skyColorHorizon, settings.skyColorTop, t);
                } else {
                    // Ground reflection
                    float t = std::pow(-elevation, 0.4f);
                    color = glm::mix(settings.skyColorHorizon, settings.groundColor, t);
                }

                // Apply intensity
                color *= settings.skyIntensity;

                data[y * SIZE + x] = glm::vec4(color, 1.0f);
            }
        }
        return data;
    };

    // Upload each face
    for (int face = 0; face < 6; ++face) {
        auto facePixels = generateFace(face);

        // Create staging buffer
        size_t dataSize = SIZE * SIZE * sizeof(glm::vec4);
        AllocatedBuffer staging = _engine->create_buffer(
            dataSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU
        );

        // Copy data to staging
        void* mapped;
        vmaMapMemory(_engine->_allocator, staging.allocation, &mapped);
        memcpy(mapped, facePixels.data(), dataSize);
        vmaUnmapMemory(_engine->_allocator, staging.allocation);

        // Copy from staging to cubemap face
        _engine->immediate_submit([&](VkCommandBuffer cmd) {
            // Transition to transfer dst
            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = face == 0 ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = _envCubemap.image;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.baseArrayLayer = face;
            barrier.subresourceRange.layerCount = 1;
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

            // Copy buffer to image
            VkBufferImageCopy region{};
            region.bufferOffset = 0;
            region.bufferRowLength = 0;
            region.bufferImageHeight = 0;
            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = 0;
            region.imageSubresource.baseArrayLayer = face;
            region.imageSubresource.layerCount = 1;
            region.imageOffset = {0, 0, 0};
            region.imageExtent = {SIZE, SIZE, 1};

            vkCmdCopyBufferToImage(cmd, staging.buffer, _envCubemap.image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
        });

        _engine->destroy_buffer(staging);
    }

    // Transition entire cubemap to shader read
    _engine->immediate_submit([&](VkCommandBuffer cmd) {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = _envCubemap.image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 6;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    });

    // Create simple irradiance map (same as env for now)
    createIrradianceMap();

    // Create prefiltered map
    createPrefilteredMap();

    fmt::print("[Environment] Procedural sky generated ({}x{} cubemap)\n", SIZE, SIZE);
}

void EnvironmentMap::createIrradianceMap() {
    const uint32_t SIZE = 32;  // Low-res for diffuse

    // Create irradiance cubemap
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    imageInfo.extent = { SIZE, SIZE, 1 };
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 6;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    VK_CHECK(vmaCreateImage(_engine->_allocator, &imageInfo, &allocInfo,
        &_irradianceMap.image, &_irradianceMap.allocation, nullptr));

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = _irradianceMap.image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    viewInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.layerCount = 6;

    VK_CHECK(vkCreateImageView(_engine->_device, &viewInfo, nullptr, &_irradianceMap.imageView));

    stats.irradianceSize = SIZE;

    // Transition to shader read (will fill with convolution later)
    _engine->immediate_submit([&](VkCommandBuffer cmd) {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.image = _irradianceMap.image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.layerCount = 6;

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    });
}

void EnvironmentMap::createPrefilteredMap() {
    const uint32_t SIZE = 256;  // Higher res for specular
    const uint32_t MIP_LEVELS = 5;  // 5 roughness levels

    // Create prefiltered cubemap with mips
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    imageInfo.extent = { SIZE, SIZE, 1 };
    imageInfo.mipLevels = MIP_LEVELS;
    imageInfo.arrayLayers = 6;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    VK_CHECK(vmaCreateImage(_engine->_allocator, &imageInfo, &allocInfo,
        &_prefilteredMap.image, &_prefilteredMap.allocation, nullptr));

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = _prefilteredMap.image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    viewInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.levelCount = MIP_LEVELS;
    viewInfo.subresourceRange.layerCount = 6;

    VK_CHECK(vkCreateImageView(_engine->_device, &viewInfo, nullptr, &_prefilteredMap.imageView));

    stats.prefilteredMipLevels = MIP_LEVELS;

    // Transition to shader read
    _engine->immediate_submit([&](VkCommandBuffer cmd) {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.image = _prefilteredMap.image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.levelCount = MIP_LEVELS;
        barrier.subresourceRange.layerCount = 6;

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    });
}

void EnvironmentMap::createSkyboxPipeline() {
    // Descriptor layout for skybox
    VkDescriptorSetLayoutBinding binding{};
    binding.binding = 0;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    binding.descriptorCount = 1;
    binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &binding;

    VK_CHECK(vkCreateDescriptorSetLayout(_engine->_device, &layoutInfo, nullptr, &_skyboxDescriptorLayout));

    // Push constant for view-proj matrix
    VkPushConstantRange pushConstant{};
    pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstant.offset = 0;
    pushConstant.size = sizeof(glm::mat4) + sizeof(glm::vec4) * 2;  // viewProj + skyParams

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &_skyboxDescriptorLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstant;

    VK_CHECK(vkCreatePipelineLayout(_engine->_device, &pipelineLayoutInfo, nullptr, &_skyboxPipelineLayout));

    // Allocate descriptor set
    _skyboxDescriptorSet = _engine->globalDescriptorAllocator.allocate(_engine->_device, _skyboxDescriptorLayout);

    // Update descriptor with cubemap
    VkDescriptorImageInfo imageInfo{};
    imageInfo.sampler = _cubemapSampler;
    imageInfo.imageView = _envCubemap.imageView;
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = _skyboxDescriptorSet;
    write.dstBinding = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.descriptorCount = 1;
    write.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(_engine->_device, 1, &write, 0, nullptr);

    // Note: Skybox pipeline creation would require vertex/fragment shaders
    // For now, the procedural sky is baked into the cubemap
    fmt::print("[Environment] Skybox pipeline created\n");
}

bool EnvironmentMap::loadFromFile(const std::string& path) {
    // TODO: Implement HDR loading (stb_image can load .hdr files)
    // For now, just generate procedural sky
    stats.loadedPath = path;
    fmt::print("[Environment] Loading HDR from {} (not implemented, using procedural)\n", path);
    generateProceduralSky();
    return true;
}

bool EnvironmentMap::loadCubemapFaces(const std::string paths[6]) {
    // TODO: Implement cubemap face loading
    fmt::print("[Environment] Loading cubemap faces (not implemented, using procedural)\n");
    generateProceduralSky();
    return true;
}

void EnvironmentMap::renderSkybox(VkCommandBuffer cmd, const glm::mat4& viewProj) {
    // For now, the skybox is rendered as part of the background
    // A dedicated skybox pass would draw a fullscreen cube with the cubemap
    (void)cmd;
    (void)viewProj;
}

} // namespace Yalaz::Renderer
