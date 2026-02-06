#include "SSAOPass.h"
#include "vk_engine.h"
#include "vk_initializers.h"
#include "vk_pipelines.h"
#include <random>

namespace Yalaz::Renderer {

SSAOPass::SSAOPass(VulkanEngine* engine)
    : PostProcessPass(engine, "SSAO") {}

SSAOPass::~SSAOPass() {
    cleanup();
}

void SSAOPass::init() {
    // Create samplers
    VkSamplerCreateInfo pointSamplerInfo{};
    pointSamplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    pointSamplerInfo.magFilter = VK_FILTER_NEAREST;
    pointSamplerInfo.minFilter = VK_FILTER_NEAREST;
    pointSamplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    pointSamplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    pointSamplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    pointSamplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

    VK_CHECK(vkCreateSampler(_engine->_device, &pointSamplerInfo, nullptr, &_pointSampler));

    VkSamplerCreateInfo linearSamplerInfo = pointSamplerInfo;
    linearSamplerInfo.magFilter = VK_FILTER_LINEAR;
    linearSamplerInfo.minFilter = VK_FILTER_LINEAR;

    VK_CHECK(vkCreateSampler(_engine->_device, &linearSamplerInfo, nullptr, &_linearSampler));

    generateKernel();
    generateNoise();
    createPipelines();
    createDescriptors();
    createBuffers(_engine->_drawExtent);
}

void SSAOPass::cleanup() {
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
    if (_ssaoPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(_engine->_device, _ssaoPipeline, nullptr);
        _ssaoPipeline = VK_NULL_HANDLE;
    }
    if (_blurPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(_engine->_device, _blurPipeline, nullptr);
        _blurPipeline = VK_NULL_HANDLE;
    }
    if (_ssaoPipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(_engine->_device, _ssaoPipelineLayout, nullptr);
        _ssaoPipelineLayout = VK_NULL_HANDLE;
    }
    if (_blurPipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(_engine->_device, _blurPipelineLayout, nullptr);
        _blurPipelineLayout = VK_NULL_HANDLE;
    }
    if (_ssaoDescriptorLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(_engine->_device, _ssaoDescriptorLayout, nullptr);
        _ssaoDescriptorLayout = VK_NULL_HANDLE;
    }
    if (_blurDescriptorLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(_engine->_device, _blurDescriptorLayout, nullptr);
        _blurDescriptorLayout = VK_NULL_HANDLE;
    }
}

void SSAOPass::execute(VkCommandBuffer cmd, AllocatedImage& input, AllocatedImage& output) {
    if (!settings.enabled) return;

    // Note: SSAO needs depth and normal buffers from G-buffer
    // For now, we'll use the depth image from the engine
    renderSSAO(cmd, _engine->_depthImage, _engine->_depthImage);  // Using depth for normals reconstruction

    if (settings.blurPasses > 0) {
        blurSSAO(cmd);
    }
}

void SSAOPass::onResize(VkExtent2D newExtent) {
    destroyBuffers();
    createBuffers(newExtent);
}

void SSAOPass::generateKernel() {
    std::default_random_engine generator;
    std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f);

    for (int i = 0; i < MAX_KERNEL_SIZE; ++i) {
        // Random point in hemisphere
        glm::vec3 sample(
            randomFloats(generator) * 2.0f - 1.0f,
            randomFloats(generator) * 2.0f - 1.0f,
            randomFloats(generator)  // Only positive Z (hemisphere)
        );

        sample = glm::normalize(sample);
        sample *= randomFloats(generator);

        // Scale samples to be more clustered toward the origin
        // This gives better quality with fewer samples
        float scale = static_cast<float>(i) / static_cast<float>(MAX_KERNEL_SIZE);
        scale = glm::mix(0.1f, 1.0f, scale * scale);  // Lerp with quadratic falloff
        sample *= scale;

        _ssaoKernel[i] = glm::vec4(sample, 0.0f);
    }

    // Create kernel buffer
    _kernelBuffer = _engine->create_buffer(
        sizeof(glm::vec4) * MAX_KERNEL_SIZE,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VMA_MEMORY_USAGE_CPU_TO_GPU
    );

    // Upload kernel data
    void* data;
    vmaMapMemory(_engine->_allocator, _kernelBuffer.allocation, &data);
    memcpy(data, _ssaoKernel.data(), sizeof(glm::vec4) * MAX_KERNEL_SIZE);
    vmaUnmapMemory(_engine->_allocator, _kernelBuffer.allocation);
}

void SSAOPass::generateNoise() {
    std::default_random_engine generator;
    std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f);

    // Generate random rotation vectors (tangent space)
    for (int i = 0; i < NOISE_SIZE * NOISE_SIZE; ++i) {
        glm::vec3 noise(
            randomFloats(generator) * 2.0f - 1.0f,
            randomFloats(generator) * 2.0f - 1.0f,
            0.0f  // Rotate around Z axis
        );
        _ssaoNoise[i] = glm::vec4(glm::normalize(noise), 0.0f);
    }

    // Create noise texture
    VkExtent3D noiseExtent = { NOISE_SIZE, NOISE_SIZE, 1 };
    _noiseTexture = _engine->create_image(
        _ssaoNoise.data(),
        noiseExtent,
        VK_FORMAT_R32G32B32A32_SFLOAT,
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
    );
}

void SSAOPass::createBuffers(VkExtent2D extent) {
    _ssaoExtent = settings.halfResolution ?
        VkExtent2D{ extent.width / 2, extent.height / 2 } : extent;

    VkExtent3D extent3D = { _ssaoExtent.width, _ssaoExtent.height, 1 };

    // SSAO output buffer (single channel float)
    _ssaoBuffer = _engine->create_image(
        extent3D,
        VK_FORMAT_R8_UNORM,
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
    );

    // Blur buffer
    _ssaoBlurBuffer = _engine->create_image(
        extent3D,
        VK_FORMAT_R8_UNORM,
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
    );
}

void SSAOPass::destroyBuffers() {
    if (_ssaoBuffer.image != VK_NULL_HANDLE) {
        _engine->destroy_image(_ssaoBuffer);
        _ssaoBuffer = {};
    }
    if (_ssaoBlurBuffer.image != VK_NULL_HANDLE) {
        _engine->destroy_image(_ssaoBlurBuffer);
        _ssaoBlurBuffer = {};
    }
    if (_noiseTexture.image != VK_NULL_HANDLE) {
        _engine->destroy_image(_noiseTexture);
        _noiseTexture = {};
    }
    if (_kernelBuffer.buffer != VK_NULL_HANDLE) {
        _engine->destroy_buffer(_kernelBuffer);
        _kernelBuffer = {};
    }
}

void SSAOPass::createPipelines() {
    // SSAO descriptor layout
    VkDescriptorSetLayoutBinding ssaoBindings[5] = {};

    // Depth texture
    ssaoBindings[0].binding = 0;
    ssaoBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    ssaoBindings[0].descriptorCount = 1;
    ssaoBindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    // Normal texture (or reconstructed from depth)
    ssaoBindings[1].binding = 1;
    ssaoBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    ssaoBindings[1].descriptorCount = 1;
    ssaoBindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    // Noise texture
    ssaoBindings[2].binding = 2;
    ssaoBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    ssaoBindings[2].descriptorCount = 1;
    ssaoBindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    // Kernel buffer
    ssaoBindings[3].binding = 3;
    ssaoBindings[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ssaoBindings[3].descriptorCount = 1;
    ssaoBindings[3].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    // Output SSAO image
    ssaoBindings[4].binding = 4;
    ssaoBindings[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    ssaoBindings[4].descriptorCount = 1;
    ssaoBindings[4].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo ssaoLayoutInfo{};
    ssaoLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    ssaoLayoutInfo.bindingCount = 5;
    ssaoLayoutInfo.pBindings = ssaoBindings;

    VK_CHECK(vkCreateDescriptorSetLayout(_engine->_device, &ssaoLayoutInfo, nullptr, &_ssaoDescriptorLayout));

    // Blur descriptor layout
    VkDescriptorSetLayoutBinding blurBindings[2] = {};

    blurBindings[0].binding = 0;
    blurBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    blurBindings[0].descriptorCount = 1;
    blurBindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    blurBindings[1].binding = 1;
    blurBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    blurBindings[1].descriptorCount = 1;
    blurBindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo blurLayoutInfo{};
    blurLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    blurLayoutInfo.bindingCount = 2;
    blurLayoutInfo.pBindings = blurBindings;

    VK_CHECK(vkCreateDescriptorSetLayout(_engine->_device, &blurLayoutInfo, nullptr, &_blurDescriptorLayout));

    // Push constant ranges
    VkPushConstantRange ssaoPushConstant{};
    ssaoPushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    ssaoPushConstant.offset = 0;
    ssaoPushConstant.size = sizeof(SSAOPushConstants);

    VkPushConstantRange blurPushConstant{};
    blurPushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    blurPushConstant.offset = 0;
    blurPushConstant.size = sizeof(BlurPushConstants);

    // SSAO pipeline layout
    VkPipelineLayoutCreateInfo ssaoPipelineLayoutInfo{};
    ssaoPipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    ssaoPipelineLayoutInfo.setLayoutCount = 1;
    ssaoPipelineLayoutInfo.pSetLayouts = &_ssaoDescriptorLayout;
    ssaoPipelineLayoutInfo.pushConstantRangeCount = 1;
    ssaoPipelineLayoutInfo.pPushConstantRanges = &ssaoPushConstant;

    VK_CHECK(vkCreatePipelineLayout(_engine->_device, &ssaoPipelineLayoutInfo, nullptr, &_ssaoPipelineLayout));

    // Blur pipeline layout
    VkPipelineLayoutCreateInfo blurPipelineLayoutInfo{};
    blurPipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    blurPipelineLayoutInfo.setLayoutCount = 1;
    blurPipelineLayoutInfo.pSetLayouts = &_blurDescriptorLayout;
    blurPipelineLayoutInfo.pushConstantRangeCount = 1;
    blurPipelineLayoutInfo.pPushConstantRanges = &blurPushConstant;

    VK_CHECK(vkCreatePipelineLayout(_engine->_device, &blurPipelineLayoutInfo, nullptr, &_blurPipelineLayout));

    // Load shaders
    VkShaderModule ssaoShader = _engine->load_shader_module("shaders/ssao.comp.spv");
    VkShaderModule blurShader = _engine->load_shader_module("shaders/ssao_blur.comp.spv");

    // Create SSAO pipeline
    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.layout = _ssaoPipelineLayout;
    pipelineInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipelineInfo.stage.module = ssaoShader;
    pipelineInfo.stage.pName = "main";

    VK_CHECK(vkCreateComputePipelines(_engine->_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_ssaoPipeline));

    // Create blur pipeline
    pipelineInfo.layout = _blurPipelineLayout;
    pipelineInfo.stage.module = blurShader;

    VK_CHECK(vkCreateComputePipelines(_engine->_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_blurPipeline));

    // Cleanup shader modules
    vkDestroyShaderModule(_engine->_device, ssaoShader, nullptr);
    vkDestroyShaderModule(_engine->_device, blurShader, nullptr);
}

void SSAOPass::createDescriptors() {
    _ssaoDescriptorSet = _engine->globalDescriptorAllocator.allocate(_engine->_device, _ssaoDescriptorLayout);
    _blurDescriptorSetH = _engine->globalDescriptorAllocator.allocate(_engine->_device, _blurDescriptorLayout);
    _blurDescriptorSetV = _engine->globalDescriptorAllocator.allocate(_engine->_device, _blurDescriptorLayout);
}

void SSAOPass::updateDescriptors(AllocatedImage& depthImage, AllocatedImage& normalImage) {
    // Update SSAO descriptor set
    VkDescriptorImageInfo depthInfo{};
    depthInfo.sampler = _pointSampler;
    depthInfo.imageView = depthImage.imageView;
    depthInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkDescriptorImageInfo normalInfo{};
    normalInfo.sampler = _pointSampler;
    normalInfo.imageView = normalImage.imageView;
    normalInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkDescriptorImageInfo noiseInfo{};
    noiseInfo.sampler = _pointSampler;
    noiseInfo.imageView = _noiseTexture.imageView;
    noiseInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkDescriptorBufferInfo kernelInfo{};
    kernelInfo.buffer = _kernelBuffer.buffer;
    kernelInfo.offset = 0;
    kernelInfo.range = sizeof(glm::vec4) * MAX_KERNEL_SIZE;

    VkDescriptorImageInfo ssaoOutputInfo{};
    ssaoOutputInfo.imageView = _ssaoBuffer.imageView;
    ssaoOutputInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

    VkWriteDescriptorSet writes[5] = {};

    writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].dstSet = _ssaoDescriptorSet;
    writes[0].dstBinding = 0;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[0].descriptorCount = 1;
    writes[0].pImageInfo = &depthInfo;

    writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[1].dstSet = _ssaoDescriptorSet;
    writes[1].dstBinding = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[1].descriptorCount = 1;
    writes[1].pImageInfo = &normalInfo;

    writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[2].dstSet = _ssaoDescriptorSet;
    writes[2].dstBinding = 2;
    writes[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[2].descriptorCount = 1;
    writes[2].pImageInfo = &noiseInfo;

    writes[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[3].dstSet = _ssaoDescriptorSet;
    writes[3].dstBinding = 3;
    writes[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes[3].descriptorCount = 1;
    writes[3].pBufferInfo = &kernelInfo;

    writes[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[4].dstSet = _ssaoDescriptorSet;
    writes[4].dstBinding = 4;
    writes[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    writes[4].descriptorCount = 1;
    writes[4].pImageInfo = &ssaoOutputInfo;

    vkUpdateDescriptorSets(_engine->_device, 5, writes, 0, nullptr);
}

void SSAOPass::renderSSAO(VkCommandBuffer cmd, AllocatedImage& depthImage, AllocatedImage& normalImage) {
    updateDescriptors(depthImage, normalImage);

    // Prepare push constants
    SSAOPushConstants pc{};
    pc.projection = _engine->sceneData.proj;
    pc.invProjection = glm::inverse(_engine->sceneData.proj);
    pc.params = glm::vec4(settings.radius, settings.bias, settings.intensity, settings.power);
    pc.params2 = glm::vec4(
        static_cast<float>(settings.samples),
        settings.maxDistance,
        settings.fadeStart,
        static_cast<float>(_ssaoExtent.width) / static_cast<float>(NOISE_SIZE)
    );
    pc.resolution = glm::vec2(_ssaoExtent.width, _ssaoExtent.height);
    pc.invResolution = glm::vec2(1.0f / _ssaoExtent.width, 1.0f / _ssaoExtent.height);

    // Transition SSAO buffer for writing
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = _ssaoBuffer.image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;

    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    // Bind and dispatch
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _ssaoPipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _ssaoPipelineLayout, 0, 1, &_ssaoDescriptorSet, 0, nullptr);
    vkCmdPushConstants(cmd, _ssaoPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(SSAOPushConstants), &pc);

    uint32_t groupsX = (_ssaoExtent.width + 7) / 8;
    uint32_t groupsY = (_ssaoExtent.height + 7) / 8;
    vkCmdDispatch(cmd, groupsX, groupsY, 1);
}

void SSAOPass::blurSSAO(VkCommandBuffer cmd) {
    BlurPushConstants pc{};
    pc.sharpness = settings.blurSharpness;

    for (int pass = 0; pass < settings.blurPasses; ++pass) {
        // Horizontal blur pass
        VkImageMemoryBarrier barriers[2] = {};

        // Source: SSAO buffer -> shader read
        barriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barriers[0].oldLayout = VK_IMAGE_LAYOUT_GENERAL;
        barriers[0].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barriers[0].image = (pass == 0) ? _ssaoBuffer.image : _ssaoBlurBuffer.image;
        barriers[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barriers[0].subresourceRange.baseMipLevel = 0;
        barriers[0].subresourceRange.levelCount = 1;
        barriers[0].subresourceRange.baseArrayLayer = 0;
        barriers[0].subresourceRange.layerCount = 1;
        barriers[0].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        barriers[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        // Destination: blur buffer -> general
        barriers[1] = barriers[0];
        barriers[1].image = _ssaoBlurBuffer.image;
        barriers[1].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barriers[1].newLayout = VK_IMAGE_LAYOUT_GENERAL;
        barriers[1].srcAccessMask = 0;
        barriers[1].dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 2, barriers);

        // Update blur descriptor for horizontal pass
        VkDescriptorImageInfo srcInfo{};
        srcInfo.sampler = _linearSampler;
        srcInfo.imageView = (pass == 0) ? _ssaoBuffer.imageView : _ssaoBlurBuffer.imageView;
        srcInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkDescriptorImageInfo dstInfo{};
        dstInfo.imageView = _ssaoBlurBuffer.imageView;
        dstInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        VkWriteDescriptorSet writes[2] = {};
        writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[0].dstSet = _blurDescriptorSetH;
        writes[0].dstBinding = 0;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[0].descriptorCount = 1;
        writes[0].pImageInfo = &srcInfo;

        writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[1].dstSet = _blurDescriptorSetH;
        writes[1].dstBinding = 1;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        writes[1].descriptorCount = 1;
        writes[1].pImageInfo = &dstInfo;

        vkUpdateDescriptorSets(_engine->_device, 2, writes, 0, nullptr);

        // Dispatch horizontal blur
        pc.direction = glm::vec2(1.0f, 0.0f);
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _blurPipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _blurPipelineLayout, 0, 1, &_blurDescriptorSetH, 0, nullptr);
        vkCmdPushConstants(cmd, _blurPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(BlurPushConstants), &pc);

        uint32_t groupsX = (_ssaoExtent.width + 7) / 8;
        uint32_t groupsY = (_ssaoExtent.height + 7) / 8;
        vkCmdDispatch(cmd, groupsX, groupsY, 1);

        // Vertical blur pass (back to SSAO buffer or keep in blur buffer for final)
        // Similar transitions and dispatch...
    }
}

} // namespace Yalaz::Renderer
