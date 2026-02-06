#include "PostProcess.h"
#include "vk_engine.h"
#include "vk_initializers.h"
#include "vk_pipelines.h"

namespace Yalaz::Renderer {

// =============================================================================
// POST PROCESS PASS - Base Implementation
// =============================================================================

PostProcessPass::PostProcessPass(VulkanEngine* engine, const std::string& name)
    : _engine(engine), _name(name) {}

// =============================================================================
// POST PROCESS MANAGER - Implementation
// =============================================================================

PostProcessManager::PostProcessManager(VulkanEngine* engine)
    : _engine(engine) {}

PostProcessManager::~PostProcessManager() {
    cleanup();
}

void PostProcessManager::init() {
    _extent = _engine->_drawExtent;
    createPingPongBuffers();

    // Initialize all passes
    for (auto& pass : _passes) {
        pass->init();
    }
}

void PostProcessManager::cleanup() {
    // Cleanup all passes
    for (auto& pass : _passes) {
        pass->cleanup();
    }
    _passes.clear();

    destroyPingPongBuffers();
}

void PostProcessManager::execute(VkCommandBuffer cmd, AllocatedImage& sceneColor, AllocatedImage& finalOutput) {
    if (!settings.enabled || _passes.empty()) {
        // No post-processing, just copy scene to output if they differ
        if (sceneColor.image != finalOutput.image) {
            // Transition images for copy
            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = sceneColor.image;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;
            barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

            VkImageMemoryBarrier barrier2 = barrier;
            barrier2.image = finalOutput.image;
            barrier2.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
            barrier2.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier2.srcAccessMask = 0;
            barrier2.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier2);

            VkImageCopy copyRegion{};
            copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copyRegion.srcSubresource.mipLevel = 0;
            copyRegion.srcSubresource.baseArrayLayer = 0;
            copyRegion.srcSubresource.layerCount = 1;
            copyRegion.dstSubresource = copyRegion.srcSubresource;
            copyRegion.extent = sceneColor.imageExtent;

            vkCmdCopyImage(cmd, sceneColor.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                finalOutput.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

            // Transition back
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

            barrier2.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier2.newLayout = VK_IMAGE_LAYOUT_GENERAL;
            barrier2.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier2.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier2);
        }
        return;
    }

    // Execute enabled passes in order
    _currentIsPing = true;
    AllocatedImage* currentInput = &sceneColor;

    int enabledPassCount = 0;
    for (auto& pass : _passes) {
        if (pass->isEnabled()) enabledPassCount++;
    }

    int passIndex = 0;
    for (auto& pass : _passes) {
        if (!pass->isEnabled()) continue;

        // Determine output: use ping-pong buffer or final output for last pass
        bool isLastPass = (passIndex == enabledPassCount - 1);
        AllocatedImage* output = isLastPass ? &finalOutput : &getCurrentBuffer();

        pass->execute(cmd, *currentInput, *output);

        // Prepare for next pass
        if (!isLastPass) {
            currentInput = output;
            swapPingPong();
        }
        passIndex++;
    }
}

void PostProcessManager::onResize(VkExtent2D newExtent) {
    _extent = newExtent;

    // Recreate ping-pong buffers
    destroyPingPongBuffers();
    createPingPongBuffers();

    // Notify all passes
    for (auto& pass : _passes) {
        pass->onResize(newExtent);
    }
}

void PostProcessManager::addPass(std::unique_ptr<PostProcessPass> pass) {
    _passes.push_back(std::move(pass));
}

PostProcessPass* PostProcessManager::getPass(const std::string& name) {
    for (auto& pass : _passes) {
        if (pass->getName() == name) {
            return pass.get();
        }
    }
    return nullptr;
}

void PostProcessManager::createPingPongBuffers() {
    VkExtent3D extent3D = {
        _extent.width,
        _extent.height,
        1
    };

    // Create ping buffer (RGBA16F for HDR)
    _pingBuffer = _engine->create_image(
        extent3D,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
    );

    // Create pong buffer
    _pongBuffer = _engine->create_image(
        extent3D,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
    );
}

void PostProcessManager::destroyPingPongBuffers() {
    if (_pingBuffer.image != VK_NULL_HANDLE) {
        _engine->destroy_image(_pingBuffer);
        _pingBuffer = {};
    }
    if (_pongBuffer.image != VK_NULL_HANDLE) {
        _engine->destroy_image(_pongBuffer);
        _pongBuffer = {};
    }
}

} // namespace Yalaz::Renderer
