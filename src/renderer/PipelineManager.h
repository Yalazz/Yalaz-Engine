#pragma once

#include "core/ISubsystem.h"
#include "ViewMode.h"
#include <vulkan/vulkan.h>
#include <unordered_map>
#include <string>

namespace Yalaz::Renderer {

/**
 * @brief Pipeline identifiers for all engine pipelines
 */
enum class PipelineId {
    // View mode pipelines
    Wireframe,
    Solid,
    Shaded,
    Normals,
    UVChecker,
    MaterialPreview,
    Rendered,

    // Special pipelines
    Outline,
    WireframeOutline,
    Primitive,
    PointLightVis,
    Grid,
    Emissive,
    Mesh,
    Triangle,
    Gradient,
    PathTrace,
    PathTracePresent,
    Plane2D,
    Plane2DDoubleSided,

    Count
};

/**
 * @brief Manages graphics and compute pipelines
 *
 * Design Patterns:
 * - Singleton: Central pipeline management
 * - Factory: Creates pipelines on demand
 * - Cache: Stores compiled pipelines for reuse
 *
 * SOLID:
 * - Single Responsibility: Only manages pipelines
 * - Open/Closed: New pipelines added via enum, not code changes
 */
class PipelineManager : public Core::ISubsystem {
public:
    static PipelineManager& Get() {
        static PipelineManager instance;
        return instance;
    }

    // ISubsystem interface
    void OnInit() override;
    void OnShutdown() override;
    const char* GetName() const override { return "PipelineManager"; }

    // =========================================================================
    // Pipeline Access
    // =========================================================================

    /**
     * @brief Get a pipeline by ID
     * @return Pipeline handle (VK_NULL_HANDLE if not initialized)
     */
    VkPipeline GetPipeline(PipelineId id) const;

    /**
     * @brief Get a pipeline layout by ID
     * @return Pipeline layout handle
     */
    VkPipelineLayout GetPipelineLayout(PipelineId id) const;

    /**
     * @brief Get pipeline for current view mode
     */
    VkPipeline GetViewModePipeline(ViewMode mode) const;
    VkPipelineLayout GetViewModePipelineLayout(ViewMode mode) const;

    // =========================================================================
    // Migration Helpers (called by VulkanEngine during migration)
    // =========================================================================

    void SetDevice(VkDevice device) { m_Device = device; }
    void SetRenderPass(VkRenderPass renderPass) { m_RenderPass = renderPass; }

    /**
     * @brief Register an existing pipeline (migration helper)
     */
    void RegisterPipeline(PipelineId id, VkPipeline pipeline, VkPipelineLayout layout);

    /**
     * @brief Register view mode pipeline mapping
     */
    void RegisterViewModePipeline(ViewMode mode, PipelineId pipelineId);

private:
    PipelineManager() = default;
    ~PipelineManager() = default;

    struct PipelineEntry {
        VkPipeline pipeline = VK_NULL_HANDLE;
        VkPipelineLayout layout = VK_NULL_HANDLE;
    };

    VkDevice m_Device = VK_NULL_HANDLE;
    VkRenderPass m_RenderPass = VK_NULL_HANDLE;

    // Pipeline storage
    std::unordered_map<PipelineId, PipelineEntry> m_Pipelines;

    // View mode to pipeline mapping
    std::unordered_map<ViewMode, PipelineId> m_ViewModePipelines;

    bool m_OwnsResources = false;
};

} // namespace Yalaz::Renderer
