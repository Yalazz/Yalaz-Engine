#pragma once
// =============================================================================
// YALAZ ENGINE - GPU Debug View
// =============================================================================

#include "EditorView.h"
#include <deque>
#include <vulkan/vulkan.h>

namespace Yalaz::UI {

class GPUDebugView : public EditorView {
public:
    GPUDebugView() : EditorView("GPU Debug", "[GPU]", ViewCategory::Debug) {}
    ~GPUDebugView() override = default;
    void OnUpdate(float deltaTime) override;
    void OnRender() override;
    void OnShutdown() override;

private:
    void RenderOverview();
    void RenderVisualization();
    void RenderDrawCalls();
    void RenderMemory();
    void RenderCounters();
    void EnsureRenderTargetDescriptors();
    void RenderTargetTile(const char* label, VkDescriptorSet ds, VkExtent2D extent, VkFormat format);
    void ClearRenderTargetDescriptors();

    int m_DebugMode = 0;  // 0=None, 1=Overdraw, 2=Depth, 3=Normals, etc.

    // Stats
    size_t m_UsedMemory = 2ULL * 1024 * 1024 * 1024;
    size_t m_TotalMemory = 8ULL * 1024 * 1024 * 1024;

    // History
    std::deque<float> m_FrameTimeHistory;
    static constexpr size_t MAX_HISTORY = 120;

    bool m_SortByGPUTime = true;
    bool m_HighlightExpensive = true;
    float m_ExpensiveThreshold = 1.0f;
    bool m_GroupByType = true;

    VkImageView m_DrawImageView = VK_NULL_HANDLE;
    VkImageView m_DepthImageView = VK_NULL_HANDLE;
    VkImageView m_NormalImageView = VK_NULL_HANDLE;
    VkImageView m_MetalRoughImageView = VK_NULL_HANDLE;

    VkDescriptorSet m_DrawImageDS = VK_NULL_HANDLE;
    VkDescriptorSet m_DepthImageDS = VK_NULL_HANDLE;
    VkDescriptorSet m_NormalImageDS = VK_NULL_HANDLE;
    VkDescriptorSet m_MetalRoughImageDS = VK_NULL_HANDLE;
};

} // namespace Yalaz::UI
