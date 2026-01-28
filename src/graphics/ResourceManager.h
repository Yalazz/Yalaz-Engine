#pragma once

#include "core/ISubsystem.h"
#include "vk_types.h"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <span>
#include <functional>

namespace Yalaz::Graphics {

/**
 * @brief Manages GPU resource creation and destruction
 *
 * Design Patterns:
 * - Singleton: Central resource management
 * - Factory: Creates GPU resources
 *
 * SOLID:
 * - Single Responsibility: Only manages resource lifecycle
 */
class ResourceManager : public Core::ISubsystem {
public:
    static ResourceManager& Get() {
        static ResourceManager instance;
        return instance;
    }

    // ISubsystem interface
    void OnInit() override;
    void OnShutdown() override;
    const char* GetName() const override { return "ResourceManager"; }

    // =========================================================================
    // Buffer Operations
    // =========================================================================

    /**
     * @brief Create a GPU buffer
     * @param allocSize Size in bytes
     * @param usage Vulkan buffer usage flags
     * @param memoryUsage VMA memory usage hint
     * @return Allocated buffer with mapping info
     */
    AllocatedBuffer CreateBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);

    /**
     * @brief Destroy a GPU buffer
     */
    void DestroyBuffer(const AllocatedBuffer& buffer);

    // =========================================================================
    // Image Operations
    // =========================================================================

    /**
     * @brief Create a GPU image (no initial data)
     * @param size Image dimensions
     * @param format Vulkan image format
     * @param usage Image usage flags
     * @param mipmapped Generate mipmaps
     * @return Allocated image with view
     */
    AllocatedImage CreateImage(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);

    /**
     * @brief Create a GPU image with initial data
     * @param data Pixel data to upload
     * @param size Image dimensions
     * @param format Vulkan image format
     * @param usage Image usage flags
     * @param mipmapped Generate mipmaps
     * @return Allocated image with view
     */
    AllocatedImage CreateImage(void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);

    /**
     * @brief Destroy a GPU image
     */
    void DestroyImage(const AllocatedImage& image);

    // =========================================================================
    // Mesh Operations
    // =========================================================================

    /**
     * @brief Upload mesh data to GPU
     * @param indices Index data
     * @param vertices Vertex data
     * @return GPU mesh buffers
     */
    GPUMeshBuffers UploadMesh(std::span<uint32_t> indices, std::span<Vertex> vertices);

    // =========================================================================
    // Shader Operations
    // =========================================================================

    /**
     * @brief Load a compiled SPIR-V shader module
     * @param filePath Path to .spv file
     * @return Shader module (VK_NULL_HANDLE on failure)
     */
    VkShaderModule LoadShaderModule(const char* filePath);

    // =========================================================================
    // Immediate Submit (for one-time commands)
    // =========================================================================

    /**
     * @brief Execute commands immediately and wait for completion
     * @param function Lambda receiving command buffer
     */
    void ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function);

    // =========================================================================
    // Sampler Access
    // =========================================================================

    VkSampler GetDefaultLinearSampler() const { return m_DefaultSamplerLinear; }
    VkSampler GetDefaultNearestSampler() const { return m_DefaultSamplerNearest; }

    // =========================================================================
    // Default Textures
    // =========================================================================

    const AllocatedImage& GetWhiteImage() const { return m_WhiteImage; }
    const AllocatedImage& GetBlackImage() const { return m_BlackImage; }
    const AllocatedImage& GetGreyImage() const { return m_GreyImage; }
    const AllocatedImage& GetErrorImage() const { return m_ErrorCheckerboardImage; }

    // =========================================================================
    // Migration Helpers (called by VulkanEngine during migration)
    // =========================================================================

    void SetDevice(VkDevice device) { m_Device = device; }
    void SetAllocator(VmaAllocator allocator) { m_Allocator = allocator; }
    void SetGraphicsQueue(VkQueue queue, uint32_t family) {
        m_GraphicsQueue = queue;
        m_GraphicsQueueFamily = family;
    }
    void SetImmediateResources(VkCommandPool pool, VkCommandBuffer cmd, VkFence fence) {
        m_ImmCommandPool = pool;
        m_ImmCommandBuffer = cmd;
        m_ImmFence = fence;
    }
    void SetDefaultSamplers(VkSampler linear, VkSampler nearest) {
        m_DefaultSamplerLinear = linear;
        m_DefaultSamplerNearest = nearest;
    }
    void SetDefaultImages(const AllocatedImage& white, const AllocatedImage& black,
                         const AllocatedImage& grey, const AllocatedImage& error) {
        m_WhiteImage = white;
        m_BlackImage = black;
        m_GreyImage = grey;
        m_ErrorCheckerboardImage = error;
    }

private:
    ResourceManager() = default;
    ~ResourceManager() = default;

    // Vulkan handles (set during migration, owned later)
    VkDevice m_Device = VK_NULL_HANDLE;
    VmaAllocator m_Allocator = VK_NULL_HANDLE;
    VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
    uint32_t m_GraphicsQueueFamily = 0;

    // Immediate submit resources
    VkCommandPool m_ImmCommandPool = VK_NULL_HANDLE;
    VkCommandBuffer m_ImmCommandBuffer = VK_NULL_HANDLE;
    VkFence m_ImmFence = VK_NULL_HANDLE;

    // Default samplers
    VkSampler m_DefaultSamplerLinear = VK_NULL_HANDLE;
    VkSampler m_DefaultSamplerNearest = VK_NULL_HANDLE;

    // Default images
    AllocatedImage m_WhiteImage{};
    AllocatedImage m_BlackImage{};
    AllocatedImage m_GreyImage{};
    AllocatedImage m_ErrorCheckerboardImage{};

    bool m_OwnsResources = false;
};

} // namespace Yalaz::Graphics
