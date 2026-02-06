#include "ToneMappingPass.h"
#include "vk_engine.h"
#include "vk_initializers.h"
#include "vk_pipelines.h"

namespace Yalaz::Renderer {

ToneMappingPass::ToneMappingPass(VulkanEngine* engine)
    : PostProcessPass(engine, "ToneMapping") {}

ToneMappingPass::~ToneMappingPass() {
    cleanup();
}

void ToneMappingPass::init() {
    // Create sampler
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

    VK_CHECK(vkCreateSampler(_engine->_device, &samplerInfo, nullptr, &_linearSampler));

    createPipeline();
}

void ToneMappingPass::cleanup() {
    vkDeviceWaitIdle(_engine->_device);

    if (_linearSampler != VK_NULL_HANDLE) {
        vkDestroySampler(_engine->_device, _linearSampler, nullptr);
        _linearSampler = VK_NULL_HANDLE;
    }
    if (_pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(_engine->_device, _pipeline, nullptr);
        _pipeline = VK_NULL_HANDLE;
    }
    if (_pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(_engine->_device, _pipelineLayout, nullptr);
        _pipelineLayout = VK_NULL_HANDLE;
    }
    if (_descriptorLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(_engine->_device, _descriptorLayout, nullptr);
        _descriptorLayout = VK_NULL_HANDLE;
    }
}

void ToneMappingPass::execute(VkCommandBuffer cmd, AllocatedImage& input, AllocatedImage& output) {
    if (!settings.enabled) return;

    _time += 1.0f / 60.0f;  // Approximate frame time for grain animation

    updateDescriptors(input, output);

    // Transition input to shader read
    VkImageMemoryBarrier srcBarrier{};
    srcBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    srcBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
    srcBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    srcBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    srcBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    srcBarrier.image = input.image;
    srcBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    srcBarrier.subresourceRange.baseMipLevel = 0;
    srcBarrier.subresourceRange.levelCount = 1;
    srcBarrier.subresourceRange.baseArrayLayer = 0;
    srcBarrier.subresourceRange.layerCount = 1;
    srcBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    srcBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &srcBarrier);

    // Transition output to general
    VkImageMemoryBarrier dstBarrier = srcBarrier;
    dstBarrier.image = output.image;
    dstBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    dstBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    dstBarrier.srcAccessMask = 0;
    dstBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;

    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &dstBarrier);

    // Prepare push constants
    TonemapPushConstants pc{};
    pc.tonemapOperator = static_cast<int>(settings.tonemapOperator);
    pc.exposure = settings.exposure;
    pc.gamma = settings.gamma;
    pc.contrast = settings.contrast;
    pc.saturation = settings.saturation;
    pc.brightness = settings.brightness;
    pc.temperature = settings.temperature;
    pc.tint = settings.tint;
    pc.shadowTint = glm::vec4(settings.shadowTint, settings.shadowTintStrength);
    pc.highlightTint = glm::vec4(settings.highlightTint, settings.highlightTintStrength);
    pc.vignetteEnabled = settings.vignetteEnabled ? 1 : 0;
    pc.vignetteIntensity = settings.vignetteIntensity;
    pc.vignetteSmoothness = settings.vignetteSmoothness;
    pc.vignetteRoundness = settings.vignetteRoundness;
    pc.chromaticAberrationEnabled = settings.chromaticAberrationEnabled ? 1 : 0;
    pc.chromaticAberrationIntensity = settings.chromaticAberrationIntensity;
    pc.filmGrainEnabled = settings.filmGrainEnabled ? 1 : 0;
    pc.filmGrainIntensity = settings.filmGrainIntensity;
    pc.filmGrainResponse = settings.filmGrainResponse;
    pc.ditheringEnabled = settings.ditheringEnabled ? 1 : 0;
    pc.time = _time;

    // Bind pipeline and dispatch
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _pipelineLayout, 0, 1, &_descriptorSet, 0, nullptr);
    vkCmdPushConstants(cmd, _pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(TonemapPushConstants), &pc);

    uint32_t groupsX = (output.imageExtent.width + 7) / 8;
    uint32_t groupsY = (output.imageExtent.height + 7) / 8;
    vkCmdDispatch(cmd, groupsX, groupsY, 1);

    // Transition output for next use
    VkImageMemoryBarrier finalBarrier = dstBarrier;
    finalBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
    finalBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    finalBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    finalBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &finalBarrier);
}

void ToneMappingPass::onResize(VkExtent2D newExtent) {
    // Tone mapping doesn't need resolution-dependent resources
}

void ToneMappingPass::createPipeline() {
    // Descriptor set layout
    VkDescriptorSetLayoutBinding bindings[2] = {};

    // Input sampled image
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    // Output storage image
    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 2;
    layoutInfo.pBindings = bindings;

    VK_CHECK(vkCreateDescriptorSetLayout(_engine->_device, &layoutInfo, nullptr, &_descriptorLayout));

    // Push constant range
    VkPushConstantRange pushConstant{};
    pushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pushConstant.offset = 0;
    pushConstant.size = sizeof(TonemapPushConstants);

    // Pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &_descriptorLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstant;

    VK_CHECK(vkCreatePipelineLayout(_engine->_device, &pipelineLayoutInfo, nullptr, &_pipelineLayout));

    // Load compute shader
    VkShaderModule shader = _engine->load_shader_module("shaders/tonemap.comp.spv");

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.layout = _pipelineLayout;
    pipelineInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipelineInfo.stage.module = shader;
    pipelineInfo.stage.pName = "main";

    VK_CHECK(vkCreateComputePipelines(_engine->_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_pipeline));

    vkDestroyShaderModule(_engine->_device, shader, nullptr);

    // Allocate descriptor set
    _descriptorSet = _engine->globalDescriptorAllocator.allocate(_engine->_device, _descriptorLayout);
}

void ToneMappingPass::updateDescriptors(AllocatedImage& input, AllocatedImage& output) {
    VkDescriptorImageInfo srcImageInfo{};
    srcImageInfo.sampler = _linearSampler;
    srcImageInfo.imageView = input.imageView;
    srcImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkDescriptorImageInfo dstImageInfo{};
    dstImageInfo.imageView = output.imageView;
    dstImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

    VkWriteDescriptorSet writes[2] = {};

    writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].dstSet = _descriptorSet;
    writes[0].dstBinding = 0;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[0].descriptorCount = 1;
    writes[0].pImageInfo = &srcImageInfo;

    writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[1].dstSet = _descriptorSet;
    writes[1].dstBinding = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    writes[1].descriptorCount = 1;
    writes[1].pImageInfo = &dstImageInfo;

    vkUpdateDescriptorSets(_engine->_device, 2, writes, 0, nullptr);
}

} // namespace Yalaz::Renderer
