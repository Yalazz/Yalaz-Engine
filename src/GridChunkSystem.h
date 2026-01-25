#pragma once
// =============================================================================
// YALAZ ENGINE - Chunked Grid System
// =============================================================================
// High-performance infinite grid using chunk-based rendering with:
// - Frustum culling (only render visible chunks)
// - Distance-based LOD (far chunks = simpler)
// - GPU instancing (single draw call for all chunks)
// =============================================================================

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <array>

// Forward declaration
class VulkanEngine;

namespace Yalaz {

// =============================================================================
// CONSTANTS
// =============================================================================
constexpr int MAX_VISIBLE_CHUNKS = 256;      // Max chunks per frame
constexpr float CHUNK_SIZE = 50.0f;          // World units per chunk
constexpr int CHUNKS_PER_AXIS = 16;          // 16x16 = 256 potential chunks
constexpr float MAX_RENDER_DISTANCE = 500.0f; // Don't render beyond this

// =============================================================================
// CHUNK DATA - Sent to GPU via instance buffer
// =============================================================================
struct ChunkInstanceData {
    glm::vec4 positionScale;  // xyz = position, w = scale
    glm::vec4 lodParams;      // x = lod level (0-3), y = opacity, zw = reserved
};

// =============================================================================
// FRUSTUM PLANES for culling
// =============================================================================
struct Frustum {
    glm::vec4 planes[6];  // left, right, bottom, top, near, far

    void extractFromMatrix(const glm::mat4& viewProj);
    bool isBoxVisible(const glm::vec3& min, const glm::vec3& max) const;
};

// =============================================================================
// GRID CHUNK SYSTEM
// =============================================================================
class GridChunkSystem {
public:
    GridChunkSystem() = default;
    ~GridChunkSystem() = default;

    // Lifecycle
    void init(VulkanEngine* engine);
    void shutdown();

    // Update visible chunks based on camera position
    void update(const glm::vec3& cameraPos, const glm::mat4& viewProj);

    // Draw all visible chunks (single instanced draw call)
    void draw(VkCommandBuffer cmd, VkDescriptorSet globalDescriptor);

    // Settings
    void setEnabled(bool enabled) { m_Enabled = enabled; }
    bool isEnabled() const { return m_Enabled; }
    void setChunkSize(float size) { m_ChunkSize = size; }
    void setRenderDistance(float dist) { m_RenderDistance = dist; }

    // Stats
    int getVisibleChunkCount() const { return m_VisibleChunkCount; }
    int getCulledChunkCount() const { return m_CulledChunkCount; }

private:
    void createChunkMesh();
    void createInstanceBuffer();
    void updateInstanceBuffer();
    void collectVisibleChunks(const glm::vec3& cameraPos, const Frustum& frustum);

    VulkanEngine* m_Engine = nullptr;
    bool m_Enabled = true;

    // Chunk settings
    float m_ChunkSize = CHUNK_SIZE;
    float m_RenderDistance = MAX_RENDER_DISTANCE;

    // GPU resources
    VkBuffer m_ChunkVertexBuffer = VK_NULL_HANDLE;
    VkBuffer m_ChunkIndexBuffer = VK_NULL_HANDLE;
    VkBuffer m_InstanceBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_VertexMemory = VK_NULL_HANDLE;
    VkDeviceMemory m_IndexMemory = VK_NULL_HANDLE;
    VkDeviceMemory m_InstanceMemory = VK_NULL_HANDLE;

    uint32_t m_IndexCount = 0;

    // Instance data
    std::vector<ChunkInstanceData> m_VisibleChunks;
    int m_VisibleChunkCount = 0;
    int m_CulledChunkCount = 0;

    // Last camera chunk position (to avoid recalculating when stationary)
    glm::ivec2 m_LastCameraChunk = glm::ivec2(INT_MAX);
};

} // namespace Yalaz
