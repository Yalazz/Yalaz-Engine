// =============================================================================
// YALAZ ENGINE - Chunked Grid System Implementation
// =============================================================================

#include "GridChunkSystem.h"
#include "vk_engine.h"
#include <algorithm>
#include <cmath>

namespace Yalaz {

// =============================================================================
// FRUSTUM IMPLEMENTATION
// =============================================================================

void Frustum::extractFromMatrix(const glm::mat4& vp) {
    // Left plane
    planes[0] = glm::vec4(
        vp[0][3] + vp[0][0],
        vp[1][3] + vp[1][0],
        vp[2][3] + vp[2][0],
        vp[3][3] + vp[3][0]
    );

    // Right plane
    planes[1] = glm::vec4(
        vp[0][3] - vp[0][0],
        vp[1][3] - vp[1][0],
        vp[2][3] - vp[2][0],
        vp[3][3] - vp[3][0]
    );

    // Bottom plane
    planes[2] = glm::vec4(
        vp[0][3] + vp[0][1],
        vp[1][3] + vp[1][1],
        vp[2][3] + vp[2][1],
        vp[3][3] + vp[3][1]
    );

    // Top plane
    planes[3] = glm::vec4(
        vp[0][3] - vp[0][1],
        vp[1][3] - vp[1][1],
        vp[2][3] - vp[2][1],
        vp[3][3] - vp[3][1]
    );

    // Near plane
    planes[4] = glm::vec4(
        vp[0][3] + vp[0][2],
        vp[1][3] + vp[1][2],
        vp[2][3] + vp[2][2],
        vp[3][3] + vp[3][2]
    );

    // Far plane
    planes[5] = glm::vec4(
        vp[0][3] - vp[0][2],
        vp[1][3] - vp[1][2],
        vp[2][3] - vp[2][2],
        vp[3][3] - vp[3][2]
    );

    // Normalize planes
    for (int i = 0; i < 6; i++) {
        float len = glm::length(glm::vec3(planes[i]));
        if (len > 0.0f) {
            planes[i] /= len;
        }
    }
}

bool Frustum::isBoxVisible(const glm::vec3& min, const glm::vec3& max) const {
    for (int i = 0; i < 6; i++) {
        glm::vec3 p = min;
        if (planes[i].x >= 0) p.x = max.x;
        if (planes[i].y >= 0) p.y = max.y;
        if (planes[i].z >= 0) p.z = max.z;

        float dist = glm::dot(glm::vec3(planes[i]), p) + planes[i].w;
        if (dist < 0) return false;
    }
    return true;
}

// =============================================================================
// GRID CHUNK SYSTEM IMPLEMENTATION
// =============================================================================

void GridChunkSystem::init(VulkanEngine* engine) {
    m_Engine = engine;
    m_VisibleChunks.reserve(MAX_VISIBLE_CHUNKS);

    // Note: We reuse the engine's existing grid mesh and pipeline
    // This system just manages which chunks to draw and provides instancing data
}

void GridChunkSystem::shutdown() {
    // Cleanup handled by engine's deletion queue
    m_VisibleChunks.clear();
}

void GridChunkSystem::update(const glm::vec3& cameraPos, const glm::mat4& viewProj) {
    if (!m_Enabled) return;

    // Calculate camera chunk position
    glm::ivec2 cameraChunk(
        static_cast<int>(std::floor(cameraPos.x / m_ChunkSize)),
        static_cast<int>(std::floor(cameraPos.z / m_ChunkSize))
    );

    // Extract frustum planes
    Frustum frustum;
    frustum.extractFromMatrix(viewProj);

    // Collect visible chunks
    collectVisibleChunks(cameraPos, frustum);

    m_LastCameraChunk = cameraChunk;
}

void GridChunkSystem::collectVisibleChunks(const glm::vec3& cameraPos, const Frustum& frustum) {
    m_VisibleChunks.clear();
    m_VisibleChunkCount = 0;
    m_CulledChunkCount = 0;

    // Calculate chunk range to check
    int chunkRadius = static_cast<int>(std::ceil(m_RenderDistance / m_ChunkSize));
    chunkRadius = std::min(chunkRadius, CHUNKS_PER_AXIS / 2);

    glm::ivec2 centerChunk(
        static_cast<int>(std::floor(cameraPos.x / m_ChunkSize)),
        static_cast<int>(std::floor(cameraPos.z / m_ChunkSize))
    );

    float cameraHeight = std::abs(cameraPos.y);

    // Iterate through potential chunks in a grid pattern
    for (int z = -chunkRadius; z <= chunkRadius; z++) {
        for (int x = -chunkRadius; x <= chunkRadius; x++) {
            if (m_VisibleChunkCount >= MAX_VISIBLE_CHUNKS) break;

            int chunkX = centerChunk.x + x;
            int chunkZ = centerChunk.y + z;

            // Calculate chunk world position (center)
            float worldX = chunkX * m_ChunkSize + m_ChunkSize * 0.5f;
            float worldZ = chunkZ * m_ChunkSize + m_ChunkSize * 0.5f;

            // Distance from camera to chunk center
            float distX = worldX - cameraPos.x;
            float distZ = worldZ - cameraPos.z;
            float distSq = distX * distX + distZ * distZ;
            float dist = std::sqrt(distSq);

            // Distance culling
            if (dist > m_RenderDistance) {
                m_CulledChunkCount++;
                continue;
            }

            // Frustum culling (check chunk AABB)
            glm::vec3 chunkMin(
                chunkX * m_ChunkSize,
                -0.1f,  // Grid is at Y=0
                chunkZ * m_ChunkSize
            );
            glm::vec3 chunkMax(
                (chunkX + 1) * m_ChunkSize,
                0.1f,
                (chunkZ + 1) * m_ChunkSize
            );

            if (!frustum.isBoxVisible(chunkMin, chunkMax)) {
                m_CulledChunkCount++;
                continue;
            }

            // Calculate LOD level based on distance
            float lodLevel = 0.0f;
            if (dist > m_RenderDistance * 0.7f) {
                lodLevel = 2.0f;  // Lowest detail
            } else if (dist > m_RenderDistance * 0.4f) {
                lodLevel = 1.0f;  // Medium detail
            }

            // Calculate opacity (fade out at distance)
            float opacity = 1.0f - std::pow(dist / m_RenderDistance, 2.0f);
            opacity = std::max(opacity, 0.1f);

            // Add to visible chunks
            ChunkInstanceData chunk;
            chunk.positionScale = glm::vec4(worldX, 0.0f, worldZ, m_ChunkSize);
            chunk.lodParams = glm::vec4(lodLevel, opacity, 0.0f, 0.0f);

            m_VisibleChunks.push_back(chunk);
            m_VisibleChunkCount++;
        }
    }

    // Sort by distance for better rendering (closest first)
    std::sort(m_VisibleChunks.begin(), m_VisibleChunks.end(),
        [&cameraPos](const ChunkInstanceData& a, const ChunkInstanceData& b) {
            float distA = glm::length(glm::vec2(a.positionScale.x, a.positionScale.z) -
                                      glm::vec2(cameraPos.x, cameraPos.z));
            float distB = glm::length(glm::vec2(b.positionScale.x, b.positionScale.z) -
                                      glm::vec2(cameraPos.x, cameraPos.z));
            return distA < distB;
        });
}

void GridChunkSystem::draw(VkCommandBuffer cmd, VkDescriptorSet globalDescriptor) {
    if (!m_Enabled || m_VisibleChunkCount == 0) return;

    // The actual drawing is done by the engine using the chunk data
    // This system provides the visible chunks list
}

} // namespace Yalaz
