#include "PipelineManager.h"
#include <fmt/core.h>

namespace Yalaz::Renderer {

void PipelineManager::OnInit() {
    fmt::print("[PipelineManager] Initialized\n");

    // Set up default view mode mappings
    m_ViewModePipelines[ViewMode::Solid] = PipelineId::Solid;
    m_ViewModePipelines[ViewMode::Shaded] = PipelineId::Shaded;
    m_ViewModePipelines[ViewMode::MaterialPreview] = PipelineId::MaterialPreview;
    m_ViewModePipelines[ViewMode::Rendered] = PipelineId::Rendered;
    m_ViewModePipelines[ViewMode::Wireframe] = PipelineId::Wireframe;
    m_ViewModePipelines[ViewMode::Normals] = PipelineId::Normals;
    m_ViewModePipelines[ViewMode::UVChecker] = PipelineId::UVChecker;
}

void PipelineManager::OnShutdown() {
    fmt::print("[PipelineManager] Shutdown\n");

    if (m_OwnsResources && m_Device) {
        for (auto& [id, entry] : m_Pipelines) {
            if (entry.pipeline) {
                vkDestroyPipeline(m_Device, entry.pipeline, nullptr);
            }
            if (entry.layout) {
                vkDestroyPipelineLayout(m_Device, entry.layout, nullptr);
            }
        }
    }

    m_Pipelines.clear();
    m_ViewModePipelines.clear();
}

VkPipeline PipelineManager::GetPipeline(PipelineId id) const {
    auto it = m_Pipelines.find(id);
    if (it != m_Pipelines.end()) {
        return it->second.pipeline;
    }
    return VK_NULL_HANDLE;
}

VkPipelineLayout PipelineManager::GetPipelineLayout(PipelineId id) const {
    auto it = m_Pipelines.find(id);
    if (it != m_Pipelines.end()) {
        return it->second.layout;
    }
    return VK_NULL_HANDLE;
}

VkPipeline PipelineManager::GetViewModePipeline(ViewMode mode) const {
    auto modeIt = m_ViewModePipelines.find(mode);
    if (modeIt != m_ViewModePipelines.end()) {
        return GetPipeline(modeIt->second);
    }
    return VK_NULL_HANDLE;
}

VkPipelineLayout PipelineManager::GetViewModePipelineLayout(ViewMode mode) const {
    auto modeIt = m_ViewModePipelines.find(mode);
    if (modeIt != m_ViewModePipelines.end()) {
        return GetPipelineLayout(modeIt->second);
    }
    return VK_NULL_HANDLE;
}

void PipelineManager::RegisterPipeline(PipelineId id, VkPipeline pipeline, VkPipelineLayout layout) {
    m_Pipelines[id] = { pipeline, layout };
}

void PipelineManager::RegisterViewModePipeline(ViewMode mode, PipelineId pipelineId) {
    m_ViewModePipelines[mode] = pipelineId;
}

} // namespace Yalaz::Renderer
