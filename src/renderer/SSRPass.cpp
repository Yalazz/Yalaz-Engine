#include "SSRPass.h"
#include "vk_engine.h"
#include "vk_initializers.h"
#include "vk_pipelines.h"

namespace Yalaz::Renderer {

SSRPass::SSRPass(VulkanEngine* engine)
    : PostProcessPass(engine, "SSR") {}

SSRPass::~SSRPass() {
    cleanup();
}

void SSRPass::init() {
    // Create samplers
    VkSamplerCreateInfo pointSamplerInfo{};
    pointSamplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    pointSamplerInfo.magFilter = VK_FILTER_NEAREST;
    pointSamplerInfo.minFilter = VK_FILTER_NEAREST;
    pointSamplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    pointSamplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    pointSamplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    pointSamplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    pointSamplerInfo.maxLod = VK_LOD_CLAMP_NONE;

    VK_CHECK(vkCreateSampler(_engine->_device, &pointSamplerInfo, nullptr, &_pointSampler));

    VkSamplerCreateInfo linearSamplerInfo = pointSamplerInfo;
    linearSamplerInfo.magFilter = VK_FILTER_LINEAR;
    linearSamplerInfo.minFilter = VK_FILTER_LINEAR;
    linearSamplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    VK_CHECK(vkCreateSampler(_engine->_device, &linearSamplerInfo, nullptr, &_linearSampler));

    createPipelines();
    createDescriptors();
    createBuffers(_engine->_drawExtent);

    _prevViewProj = glm::mat4(1.0f);
}

void SSRPass::cleanup() {
    vkDeviceWaitIdle(_engine->_device);

    destroyBuffers();

    if (_pointSampler != VK_NULL_HANDLE) {
        vkDestroySampler(_engine->_device, _pointSampler, nullptr);
        _pointSampler = VK_NULL_HANDLE;
    }
    if (_linearSampler != VK_NULL_HANDLE) {
        vkDestroySampler(_engine->_device, _linearSampler, nullptr);
        _linearSampler = VK_NULL_HANDLE;
    }
    if (_ssrPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(_engine->_device, _ssrPipeline, nullptr);
        _ssrPipeline = VK_NULL_HANDLE;
    }
    if (_hiZPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(_engine->_device, _hiZPipeline, nullptr);
        _hiZPipeline = VK_NULL_HANDLE;
    }
    if (_temporalPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(_engine->_device, _temporalPipeline, nullptr);
        _temporalPipeline = VK_NULL_HANDLE;
    }
    if (_ssrPipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(_engine->_device, _ssrPipelineLayout, nullptr);
        _ssrPipelineLayout = VK_NULL_HANDLE;
    }
    if (_hiZPipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(_engine->_device, _hiZPipelineLayout, nullptr);
        _hiZPipelineLayout = VK_NULL_HANDLE;
    }
    if (_temporalPipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(_engine->_device, _temporalPipelineLayout, nullptr);
        _temporalPipelineLayout = VK_NULL_HANDLE;
    }
    if (_ssrDescriptorLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(_engine->_device, _ssrDescriptorLayout, nullptr);
        _ssrDescriptorLayout = VK_NULL_HANDLE;
    }
    if (_hiZDescriptorLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(_engine->_device, _hiZDescriptorLayout, nullptr);
        _hiZDescriptorLayout = VK_NULL_HANDLE;
    }
    if (_temporalDescriptorLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(_engine->_device, _temporalDescriptorLayout, nullptr);
        _temporalDescriptorLayout = VK_NULL_HANDLE;
    }
}

void SSRPass::execute(VkCommandBuffer cmd, AllocatedImage& input, AllocatedImage& output) {
    if (!settings.enabled) return;

    // Build hierarchical Z-buffer for acceleration
    buildHiZ(cmd, _engine->_depthImage);

    // Trace reflections
    traceReflections(cmd, input, _engine->_depthImage, _engine->_depthImage, _engine->_depthImage);

    // Apply temporal filter for stability
    if (settings.temporalFilter) {
        temporalFilter(cmd);
    }

    // Store current view-proj for next frame
    _prevViewProj = _engine->sceneData.viewproj;
}

void SSRPass::onResize(VkExtent2D newExtent) {
    destroyBuffers();
    createBuffers(newExtent);
}

void SSRPass::createBuffers(VkExtent2D extent) {
    _extent = extent;

    VkExtent3D extent3D = { extent.width, extent.height, 1 };

    // Reflection buffer
    _reflectionBuffer = _engine->create_image(
        extent3D,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
    );

    // Previous frame buffer for temporal filtering
    _prevReflectionBuffer = _engine->create_image(
        extent3D,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
    );

    // Hi-Z buffer with mip chain
    _hiZMipLevels = static_cast<int>(std::floor(std::log2(std::max(extent.width, extent.height)))) + 1;
    _hiZMipLevels = std::min(_hiZMipLevels, 12);  // Cap at reasonable level

    VkImageCreateInfo hiZInfo{};
    hiZInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    hiZInfo.imageType = VK_IMAGE_TYPE_2D;
    hiZInfo.format = VK_FORMAT_R32_SFLOAT;
    hiZInfo.extent = extent3D;
    hiZInfo.mipLevels = _hiZMipLevels;
    hiZInfo.arrayLayers = 1;
    hiZInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    hiZInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    hiZInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    vmaCreateImage(_engine->_allocator, &hiZInfo, &allocInfo, &_hiZBuffer.image, &_hiZBuffer.allocation, nullptr);

    _hiZBuffer.imageExtent = extent3D;
    _hiZBuffer.imageFormat = VK_FORMAT_R32_SFLOAT;

    // Create image view for full mip chain
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = _hiZBuffer.image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R32_SFLOAT;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = _hiZMipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VK_CHECK(vkCreateImageView(_engine->_device, &viewInfo, nullptr, &_hiZBuffer.imageView));
}

void SSRPass::destroyBuffers() {
    if (_reflectionBuffer.image != VK_NULL_HANDLE) {
        _engine->destroy_image(_reflectionBuffer);
        _reflectionBuffer = {};
    }
    if (_prevReflectionBuffer.image != VK_NULL_HANDLE) {
        _engine->destroy_image(_prevReflectionBuffer);
        _prevReflectionBuffer = {};
    }
    if (_hiZBuffer.image != VK_NULL_HANDLE) {
        if (_hiZBuffer.imageView != VK_NULL_HANDLE) {
            vkDestroyImageView(_engine->_device, _hiZBuffer.imageView, nullptr);
        }
        vmaDestroyImage(_engine->_allocator, _hiZBuffer.image, _hiZBuffer.allocation);
        _hiZBuffer = {};
    }
    _hiZDescriptorSets.clear();
}

void SSRPass::createPipelines() {
    // SSR descriptor layout
    VkDescriptorSetLayoutBinding ssrBindings[6] = {};

    // Color texture
    ssrBindings[0].binding = 0;
    ssrBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    ssrBindings[0].descriptorCount = 1;
    ssrBindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    // Depth texture (Hi-Z)
    ssrBindings[1].binding = 1;
    ssrBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    ssrBindings[1].descriptorCount = 1;
    ssrBindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    // Normal texture
    ssrBindings[2].binding = 2;
    ssrBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    ssrBindings[2].descriptorCount = 1;
    ssrBindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    // Roughness texture
    ssrBindings[3].binding = 3;
    ssrBindings[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    ssrBindings[3].descriptorCount = 1;
    ssrBindings[3].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    // Output reflection
    ssrBindings[4].binding = 4;
    ssrBindings[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    ssrBindings[4].descriptorCount = 1;
    ssrBindings[4].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    // Hi-Z buffer (with mips)
    ssrBindings[5].binding = 5;
    ssrBindings[5].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    ssrBindings[5].descriptorCount = 1;
    ssrBindings[5].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo ssrLayoutInfo{};
    ssrLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    ssrLayoutInfo.bindingCount = 6;
    ssrLayoutInfo.pBindings = ssrBindings;

    VK_CHECK(vkCreateDescriptorSetLayout(_engine->_device, &ssrLayoutInfo, nullptr, &_ssrDescriptorLayout));

    // Hi-Z descriptor layout
    VkDescriptorSetLayoutBinding hiZBindings[2] = {};

    hiZBindings[0].binding = 0;
    hiZBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    hiZBindings[0].descriptorCount = 1;
    hiZBindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    hiZBindings[1].binding = 1;
    hiZBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    hiZBindings[1].descriptorCount = 1;
    hiZBindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo hiZLayoutInfo{};
    hiZLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    hiZLayoutInfo.bindingCount = 2;
    hiZLayoutInfo.pBindings = hiZBindings;

    VK_CHECK(vkCreateDescriptorSetLayout(_engine->_device, &hiZLayoutInfo, nullptr, &_hiZDescriptorLayout));

    // Temporal descriptor layout
    VkDescriptorSetLayoutBinding temporalBindings[3] = {};

    temporalBindings[0].binding = 0;
    temporalBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    temporalBindings[0].descriptorCount = 1;
    temporalBindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    temporalBindings[1].binding = 1;
    temporalBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    temporalBindings[1].descriptorCount = 1;
    temporalBindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    temporalBindings[2].binding = 2;
    temporalBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    temporalBindings[2].descriptorCount = 1;
    temporalBindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo temporalLayoutInfo{};
    temporalLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    temporalLayoutInfo.bindingCount = 3;
    temporalLayoutInfo.pBindings = temporalBindings;

    VK_CHECK(vkCreateDescriptorSetLayout(_engine->_device, &temporalLayoutInfo, nullptr, &_temporalDescriptorLayout));

    // Push constant ranges
    VkPushConstantRange ssrPushConstant{};
    ssrPushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    ssrPushConstant.offset = 0;
    ssrPushConstant.size = sizeof(SSRPushConstants);

    VkPushConstantRange hiZPushConstant{};
    hiZPushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    hiZPushConstant.offset = 0;
    hiZPushConstant.size = sizeof(HiZPushConstants);

    VkPushConstantRange temporalPushConstant{};
    temporalPushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    temporalPushConstant.offset = 0;
    temporalPushConstant.size = sizeof(TemporalPushConstants);

    // Pipeline layouts
    VkPipelineLayoutCreateInfo ssrLayoutCreateInfo{};
    ssrLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    ssrLayoutCreateInfo.setLayoutCount = 1;
    ssrLayoutCreateInfo.pSetLayouts = &_ssrDescriptorLayout;
    ssrLayoutCreateInfo.pushConstantRangeCount = 1;
    ssrLayoutCreateInfo.pPushConstantRanges = &ssrPushConstant;

    VK_CHECK(vkCreatePipelineLayout(_engine->_device, &ssrLayoutCreateInfo, nullptr, &_ssrPipelineLayout));

    VkPipelineLayoutCreateInfo hiZLayoutCreateInfo{};
    hiZLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    hiZLayoutCreateInfo.setLayoutCount = 1;
    hiZLayoutCreateInfo.pSetLayouts = &_hiZDescriptorLayout;
    hiZLayoutCreateInfo.pushConstantRangeCount = 1;
    hiZLayoutCreateInfo.pPushConstantRanges = &hiZPushConstant;

    VK_CHECK(vkCreatePipelineLayout(_engine->_device, &hiZLayoutCreateInfo, nullptr, &_hiZPipelineLayout));

    VkPipelineLayoutCreateInfo temporalLayoutCreateInfo{};
    temporalLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    temporalLayoutCreateInfo.setLayoutCount = 1;
    temporalLayoutCreateInfo.pSetLayouts = &_temporalDescriptorLayout;
    temporalLayoutCreateInfo.pushConstantRangeCount = 1;
    temporalLayoutCreateInfo.pPushConstantRanges = &temporalPushConstant;

    VK_CHECK(vkCreatePipelineLayout(_engine->_device, &temporalLayoutCreateInfo, nullptr, &_temporalPipelineLayout));

    // Load shaders and create pipelines
    VkShaderModule ssrShader = _engine->load_shader_module("shaders/ssr.comp.spv");
    VkShaderModule hiZShader = _engine->load_shader_module("shaders/hiz_generate.comp.spv");
    VkShaderModule temporalShader = _engine->load_shader_module("shaders/ssr_temporal.comp.spv");

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipelineInfo.stage.pName = "main";

    // SSR pipeline
    pipelineInfo.layout = _ssrPipelineLayout;
    pipelineInfo.stage.module = ssrShader;
    VK_CHECK(vkCreateComputePipelines(_engine->_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_ssrPipeline));

    // Hi-Z pipeline
    pipelineInfo.layout = _hiZPipelineLayout;
    pipelineInfo.stage.module = hiZShader;
    VK_CHECK(vkCreateComputePipelines(_engine->_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_hiZPipeline));

    // Temporal pipeline
    pipelineInfo.layout = _temporalPipelineLayout;
    pipelineInfo.stage.module = temporalShader;
    VK_CHECK(vkCreateComputePipelines(_engine->_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_temporalPipeline));

    // Cleanup
    vkDestroyShaderModule(_engine->_device, ssrShader, nullptr);
    vkDestroyShaderModule(_engine->_device, hiZShader, nullptr);
    vkDestroyShaderModule(_engine->_device, temporalShader, nullptr);
}

void SSRPass::createDescriptors() {
    _ssrDescriptorSet = _engine->globalDescriptorAllocator.allocate(_engine->_device, _ssrDescriptorLayout);
    _temporalDescriptorSet = _engine->globalDescriptorAllocator.allocate(_engine->_device, _temporalDescriptorLayout);
}

void SSRPass::buildHiZ(VkCommandBuffer cmd, AllocatedImage& depthImage) {
    // First, copy depth to mip 0 of Hi-Z
    // Then generate subsequent mip levels by taking the max of 4 texels

    // Implementation would iterate through mip levels and dispatch compute shader
    // to downsample each level
}

void SSRPass::traceReflections(VkCommandBuffer cmd, AllocatedImage& colorImage, AllocatedImage& depthImage,
                               AllocatedImage& normalImage, AllocatedImage& roughnessImage) {
    SSRPushConstants pc{};
    pc.projection = _engine->sceneData.proj;
    pc.invProjection = glm::inverse(_engine->sceneData.proj);
    pc.view = _engine->sceneData.view;
    pc.invView = glm::inverse(_engine->sceneData.view);
    pc.params = glm::vec4(settings.maxSteps, settings.maxDistance, settings.thickness, settings.stride);
    pc.params2 = glm::vec4(settings.roughnessThreshold, settings.fadeDist, settings.intensity, settings.strideZCutoff);
    pc.params3 = glm::vec4(settings.binarySearchSteps, _extent.width, _extent.height, _hiZMipLevels);
    pc.cameraPos = _engine->sceneData.cameraPosition;

    // Bind and dispatch
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _ssrPipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _ssrPipelineLayout, 0, 1, &_ssrDescriptorSet, 0, nullptr);
    vkCmdPushConstants(cmd, _ssrPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(SSRPushConstants), &pc);

    uint32_t groupsX = (_extent.width + 7) / 8;
    uint32_t groupsY = (_extent.height + 7) / 8;
    vkCmdDispatch(cmd, groupsX, groupsY, 1);
}

void SSRPass::temporalFilter(VkCommandBuffer cmd) {
    TemporalPushConstants pc{};
    pc.prevViewProj = _prevViewProj;
    pc.invViewProj = glm::inverse(_engine->sceneData.viewproj);
    pc.temporalWeight = settings.temporalWeight;

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _temporalPipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _temporalPipelineLayout, 0, 1, &_temporalDescriptorSet, 0, nullptr);
    vkCmdPushConstants(cmd, _temporalPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(TemporalPushConstants), &pc);

    uint32_t groupsX = (_extent.width + 7) / 8;
    uint32_t groupsY = (_extent.height + 7) / 8;
    vkCmdDispatch(cmd, groupsX, groupsY, 1);
}

} // namespace Yalaz::Renderer
