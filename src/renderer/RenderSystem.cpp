#include "RenderSystem.h"
#include <fmt/core.h>

namespace Yalaz::Renderer {

void RenderSystem::OnInit() {
    fmt::print("[RenderSystem] Initialized\n");

    // Default viewport (will be updated by VulkanEngine)
    m_Viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = 1700.0f,
        .height = 900.0f,
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };

    m_Scissor = {
        .offset = { 0, 0 },
        .extent = { 1700, 900 }
    };
}

void RenderSystem::OnShutdown() {
    fmt::print("[RenderSystem] Shutdown\n");
}

void RenderSystem::OnRender(VkCommandBuffer cmd) {
    // During migration, VulkanEngine handles actual rendering.
    // This will be populated when draw methods are migrated.
    (void)cmd;
}

void RenderSystem::ResetStats() {
    m_Stats = {};
}

} // namespace Yalaz::Renderer
