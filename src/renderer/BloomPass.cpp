#include "BloomPass.h"
#include "vk_engine.h"
#include "vk_initializers.h"
#include "vk_pipelines.h"

namespace Yalaz::Renderer {

BloomPass::BloomPass(VulkanEngine* engine)
    : PostProcessPass(engine, "Bloom") {}

BloomPass::~BloomPass() {
    cleanup();
}

void BloomPass::init() {
    // Create sampler
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.maxLod = VK_LOD_CLAMP_NONE;

    VK_CHECK(vkCreateSampler(_engine->_device, &samplerInfo, nullptr, &_linearSampler));

    createPipelines();
    createDescriptors();
    // Use drawImage's actual extent (drawExtent may be 0 at init time)
    VkExtent2D initExtent = { _engine->_drawImage.imageExtent.width, _engine->_drawImage.imageExtent.height };
    if (initExtent.width > 0 && initExtent.height > 0) {
        createMipChain(initExtent);
    }
}

void BloomPass::cleanup() {
    vkDeviceWaitIdle(_engine->_device);

    destroyMipChain();

    if (_linearSampler != VK_NULL_HANDLE) {
        vkDestroySampler(_engine->_device, _linearSampler, nullptr);
        _linearSampler = VK_NULL_HANDLE;
    }
    if (_downsamplePipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(_engine->_device, _downsamplePipeline, nullptr);
        _downsamplePipeline = VK_NULL_HANDLE;
    }
    if (_downsampleKarisPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(_engine->_device, _downsampleKarisPipeline, nullptr);
        _downsampleKarisPipeline = VK_NULL_HANDLE;
    }
    if (_upsamplePipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(_engine->_device, _upsamplePipeline, nullptr);
        _upsamplePipeline = VK_NULL_HANDLE;
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

void BloomPass::execute(VkCommandBuffer cmd, AllocatedImage& input, AllocatedImage& output) {
    if (!settings.enabled || _actualMipLevels < 2) return;

    updateDescriptors(input, output);

    // Downsample chain
    downsample(cmd, input);

    // Upsample chain and blend back to output
    upsample(cmd, output);
}

void BloomPass::onResize(VkExtent2D newExtent) {
    destroyMipChain();
    createMipChain(newExtent);
}

void BloomPass::createMipChain(VkExtent2D baseExtent) {
    // Calculate number of mip levels based on resolution
    _actualMipLevels = std::min(settings.mipLevels, MAX_MIP_LEVELS);

    VkExtent2D extent = baseExtent;
    for (int i = 0; i < _actualMipLevels; ++i) {
        // Each mip is half the size of the previous
        extent.width = std::max(1u, extent.width / 2);
        extent.height = std::max(1u, extent.height / 2);

        VkExtent3D extent3D = { extent.width, extent.height, 1 };

        _mipChain[i].image = _engine->create_image(
            extent3D,
            VK_FORMAT_R16G16B16A16_SFLOAT,
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
        );
        _mipChain[i].extent = extent;

        // Stop if we've reached minimum size
        if (extent.width < 4 || extent.height < 4) {
            _actualMipLevels = i + 1;
            break;
        }
    }
}

void BloomPass::destroyMipChain() {
    for (int i = 0; i < _actualMipLevels; ++i) {
        if (_mipChain[i].image.image != VK_NULL_HANDLE) {
            _engine->destroy_image(_mipChain[i].image);
            _mipChain[i].image = {};
        }
    }
    _actualMipLevels = 0;
}

void BloomPass::createPipelines() {
    // Descriptor set layout: input sampled image + output storage image
    VkDescriptorSetLayoutBinding bindings[2] = {};

    // Binding 0: Input sampled image
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    // Binding 1: Output storage image
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
    pushConstant.size = sizeof(BloomPushConstants);

    // Pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &_descriptorLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstant;

    VK_CHECK(vkCreatePipelineLayout(_engine->_device, &pipelineLayoutInfo, nullptr, &_pipelineLayout));

    // Load compute shaders
    VkShaderModule downsampleShader = _engine->load_shader_module("../../shaders/bloom_downsample.comp.spv");
    VkShaderModule downsampleKarisShader = _engine->load_shader_module("../../shaders/bloom_downsample_karis.comp.spv");
    VkShaderModule upsampleShader = _engine->load_shader_module("../../shaders/bloom_upsample.comp.spv");

    // Create downsample pipeline
    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.layout = _pipelineLayout;
    pipelineInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipelineInfo.stage.module = downsampleShader;
    pipelineInfo.stage.pName = "main";

    VK_CHECK(vkCreateComputePipelines(_engine->_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_downsamplePipeline));

    // Create downsample Karis pipeline (first pass)
    pipelineInfo.stage.module = downsampleKarisShader;
    VK_CHECK(vkCreateComputePipelines(_engine->_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_downsampleKarisPipeline));

    // Create upsample pipeline
    pipelineInfo.stage.module = upsampleShader;
    VK_CHECK(vkCreateComputePipelines(_engine->_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_upsamplePipeline));

    // Cleanup shader modules
    vkDestroyShaderModule(_engine->_device, downsampleShader, nullptr);
    vkDestroyShaderModule(_engine->_device, downsampleKarisShader, nullptr);
    vkDestroyShaderModule(_engine->_device, upsampleShader, nullptr);
}

void BloomPass::createDescriptors() {
    // Allocate descriptor sets from the global allocator
    for (int i = 0; i < MAX_MIP_LEVELS; ++i) {
        _downsampleDescriptors[i] = _engine->globalDescriptorAllocator.allocate(_engine->_device, _descriptorLayout);
        _upsampleDescriptors[i] = _engine->globalDescriptorAllocator.allocate(_engine->_device, _descriptorLayout);
    }
}

void BloomPass::updateDescriptors(AllocatedImage& input, AllocatedImage& output) {
    // Update downsample descriptors
    for (int i = 0; i < _actualMipLevels; ++i) {
        VkDescriptorImageInfo srcImageInfo{};
        srcImageInfo.sampler = _linearSampler;
        srcImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        if (i == 0) {
            srcImageInfo.imageView = input.imageView;
        } else {
            srcImageInfo.imageView = _mipChain[i - 1].image.imageView;
        }

        VkDescriptorImageInfo dstImageInfo{};
        dstImageInfo.imageView = _mipChain[i].image.imageView;
        dstImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        VkWriteDescriptorSet writes[2] = {};

        writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[0].dstSet = _downsampleDescriptors[i];
        writes[0].dstBinding = 0;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[0].descriptorCount = 1;
        writes[0].pImageInfo = &srcImageInfo;

        writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[1].dstSet = _downsampleDescriptors[i];
        writes[1].dstBinding = 1;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        writes[1].descriptorCount = 1;
        writes[1].pImageInfo = &dstImageInfo;

        vkUpdateDescriptorSets(_engine->_device, 2, writes, 0, nullptr);
    }

    // Update upsample descriptors
    for (int i = _actualMipLevels - 2; i >= 0; --i) {
        VkDescriptorImageInfo srcImageInfo{};
        srcImageInfo.sampler = _linearSampler;
        srcImageInfo.imageView = _mipChain[i + 1].image.imageView;
        srcImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkDescriptorImageInfo dstImageInfo{};
        if (i == 0) {
            dstImageInfo.imageView = output.imageView;
        } else {
            dstImageInfo.imageView = _mipChain[i].image.imageView;
        }
        dstImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        VkWriteDescriptorSet writes[2] = {};

        writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[0].dstSet = _upsampleDescriptors[i];
        writes[0].dstBinding = 0;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[0].descriptorCount = 1;
        writes[0].pImageInfo = &srcImageInfo;

        writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[1].dstSet = _upsampleDescriptors[i];
        writes[1].dstBinding = 1;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        writes[1].descriptorCount = 1;
        writes[1].pImageInfo = &dstImageInfo;

        vkUpdateDescriptorSets(_engine->_device, 2, writes, 0, nullptr);
    }
}

void BloomPass::downsample(VkCommandBuffer cmd, AllocatedImage& input) {
    BloomPushConstants pc{};
    pc.threshold = settings.threshold;
    pc.softThreshold = settings.softThreshold;
    pc.filterRadius = settings.radius;
    pc.intensity = settings.intensity;

    for (int i = 0; i < _actualMipLevels; ++i) {

        // Transition source to shader read
        VkImageMemoryBarrier srcBarrier{};
        srcBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        srcBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
        srcBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        srcBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        srcBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        srcBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        srcBarrier.subresourceRange.baseMipLevel = 0;
        srcBarrier.subresourceRange.levelCount = 1;
        srcBarrier.subresourceRange.baseArrayLayer = 0;
        srcBarrier.subresourceRange.layerCount = 1;

        if (i == 0) {
            srcBarrier.image = input.image;
            // First pass: input comes from rasterization (color attachment write)
            srcBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_SHADER_WRITE_BIT;
            srcBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            vkCmdPipelineBarrier(cmd,
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &srcBarrier);
        } else {
            srcBarrier.image = _mipChain[i - 1].image.image;
            srcBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            srcBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &srcBarrier);
        }

        // Transition destination to general for storage write
        VkImageMemoryBarrier dstBarrier = srcBarrier;
        dstBarrier.image = _mipChain[i].image.image;
        dstBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        dstBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        dstBarrier.srcAccessMask = 0;
        dstBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &dstBarrier);

        // Use Karis average for first pass to reduce fireflies
        VkPipeline pipeline = (i == 0) ? _downsampleKarisPipeline : _downsamplePipeline;
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _pipelineLayout, 0, 1, &_downsampleDescriptors[i], 0, nullptr);
        vkCmdPushConstants(cmd, _pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(BloomPushConstants), &pc);

        // Dispatch: 8x8 thread groups
        uint32_t groupsX = (_mipChain[i].extent.width + 7) / 8;
        uint32_t groupsY = (_mipChain[i].extent.height + 7) / 8;
        vkCmdDispatch(cmd, groupsX, groupsY, 1);
    }
}

void BloomPass::upsample(VkCommandBuffer cmd, AllocatedImage& output) {
    BloomPushConstants pc{};
    pc.filterRadius = settings.radius;
    pc.intensity = settings.intensity;

    for (int i = _actualMipLevels - 2; i >= 0; --i) {

        // Transition source to shader read
        VkImageMemoryBarrier srcBarrier{};
        srcBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        srcBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
        srcBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        srcBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        srcBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        srcBarrier.image = _mipChain[i + 1].image.image;
        srcBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        srcBarrier.subresourceRange.baseMipLevel = 0;
        srcBarrier.subresourceRange.levelCount = 1;
        srcBarrier.subresourceRange.baseArrayLayer = 0;
        srcBarrier.subresourceRange.layerCount = 1;
        srcBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        srcBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &srcBarrier);

        // Transition destination
        VkImageMemoryBarrier dstBarrier = srcBarrier;
        if (i == 0) {
            dstBarrier.image = output.image;
        } else {
            dstBarrier.image = _mipChain[i].image.image;
        }
        // After downsample, all images (including output) are in SHADER_READ_ONLY_OPTIMAL
        dstBarrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        dstBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        dstBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dstBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &dstBarrier);

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _upsamplePipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _pipelineLayout, 0, 1, &_upsampleDescriptors[i], 0, nullptr);
        vkCmdPushConstants(cmd, _pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(BloomPushConstants), &pc);

        VkExtent2D extent = (i == 0) ? VkExtent2D{ output.imageExtent.width, output.imageExtent.height } : _mipChain[i].extent;
        uint32_t groupsX = (extent.width + 7) / 8;
        uint32_t groupsY = (extent.height + 7) / 8;
        vkCmdDispatch(cmd, groupsX, groupsY, 1);
    }
}

} // namespace Yalaz::Renderer
