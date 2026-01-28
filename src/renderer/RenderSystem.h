#pragma once

#include "core/ISubsystem.h"
#include "ViewMode.h"
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

namespace Yalaz::Renderer {

/**
 * @brief High-level rendering system
 *
 * Handles view mode dispatching, draw call management, and render state.
 *
 * Design Patterns:
 * - Singleton: Central rendering coordination
 * - Strategy: ViewMode determines rendering algorithm
 * - Facade: Simplifies complex Vulkan rendering
 *
 * SOLID:
 * - Single Responsibility: Only handles rendering logic
 */
class RenderSystem : public Core::ISubsystem, public Core::IRenderable {
public:
    static RenderSystem& Get() {
        static RenderSystem instance;
        return instance;
    }

    // ISubsystem interface
    void OnInit() override;
    void OnShutdown() override;
    const char* GetName() const override { return "RenderSystem"; }

    // IRenderable interface
    void OnRender(VkCommandBuffer cmd) override;

    // =========================================================================
    // View Mode Management
    // =========================================================================

    ViewMode GetCurrentViewMode() const { return m_CurrentViewMode; }
    void SetViewMode(ViewMode mode) { m_CurrentViewMode = mode; }

    // =========================================================================
    // Render Settings
    // =========================================================================

    bool IsGridEnabled() const { return m_ShowGrid; }
    void SetGridEnabled(bool enabled) { m_ShowGrid = enabled; }

    bool IsOutlineEnabled() const { return m_ShowOutline; }
    void SetOutlineEnabled(bool enabled) { m_ShowOutline = enabled; }

    bool IsBackfaceCullingEnabled() const { return m_EnableBackfaceCulling; }
    void SetBackfaceCullingEnabled(bool enabled) { m_EnableBackfaceCulling = enabled; }

    // =========================================================================
    // Render Statistics
    // =========================================================================

    struct RenderStats {
        uint32_t drawCallCount = 0;
        uint32_t triangleCount = 0;
        uint32_t visibleObjectCount = 0;
        float frameTime = 0.0f;
    };

    const RenderStats& GetStats() const { return m_Stats; }
    void ResetStats();

    // =========================================================================
    // Viewport Management
    // =========================================================================

    void SetViewport(const VkViewport& viewport) { m_Viewport = viewport; }
    void SetScissor(const VkRect2D& scissor) { m_Scissor = scissor; }

    const VkViewport& GetViewport() const { return m_Viewport; }
    const VkRect2D& GetScissor() const { return m_Scissor; }

    // =========================================================================
    // Migration Helpers
    // =========================================================================

    void SetDevice(VkDevice device) { m_Device = device; }

private:
    RenderSystem() = default;
    ~RenderSystem() = default;

    VkDevice m_Device = VK_NULL_HANDLE;

    // Current state
    ViewMode m_CurrentViewMode = ViewMode::Shaded;
    bool m_ShowGrid = true;
    bool m_ShowOutline = true;
    bool m_EnableBackfaceCulling = true;

    // Viewport state
    VkViewport m_Viewport{};
    VkRect2D m_Scissor{};

    // Statistics
    RenderStats m_Stats{};
};

} // namespace Yalaz::Renderer
