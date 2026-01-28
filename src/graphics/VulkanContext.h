#pragma once

#include "core/ISubsystem.h"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

struct SDL_Window;

namespace Yalaz::Graphics {

/**
 * @brief Core Vulkan context - manages instance, device, queues
 *
 * Design Patterns:
 * - Singleton: Single Vulkan context for the application
 * - Facade: Simplifies access to core Vulkan objects
 *
 * SOLID:
 * - Single Responsibility: Only manages core Vulkan objects
 * - Interface Segregation: IResourceProvider for resource creation
 */
class VulkanContext : public Core::ISubsystem {
public:
    static VulkanContext& Get() {
        static VulkanContext instance;
        return instance;
    }

    // ISubsystem interface
    void OnInit() override;
    void OnShutdown() override;
    const char* GetName() const override { return "VulkanContext"; }

    // =========================================================================
    // Core Vulkan Objects Accessors
    // =========================================================================

    VkInstance GetInstance() const { return m_Instance; }
    VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }
    VkDevice GetDevice() const { return m_Device; }
    VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }
    uint32_t GetGraphicsQueueFamily() const { return m_GraphicsQueueFamily; }
    VkSurfaceKHR GetSurface() const { return m_Surface; }
    VmaAllocator GetAllocator() const { return m_Allocator; }

    // =========================================================================
    // Initialization (called by VulkanEngine during migration)
    // =========================================================================

    /**
     * @brief Set Vulkan objects from VulkanEngine (migration helper)
     * During migration, VulkanEngine still creates these objects.
     * This allows VulkanContext to provide access without owning.
     */
    void SetInstance(VkInstance instance) { m_Instance = instance; }
    void SetPhysicalDevice(VkPhysicalDevice gpu) { m_PhysicalDevice = gpu; }
    void SetDevice(VkDevice device) { m_Device = device; }
    void SetGraphicsQueue(VkQueue queue, uint32_t family) {
        m_GraphicsQueue = queue;
        m_GraphicsQueueFamily = family;
    }
    void SetSurface(VkSurfaceKHR surface) { m_Surface = surface; }
    void SetAllocator(VmaAllocator allocator) { m_Allocator = allocator; }
    void SetDebugMessenger(VkDebugUtilsMessengerEXT messenger) { m_DebugMessenger = messenger; }

    // =========================================================================
    // Utility Methods
    // =========================================================================

    /**
     * @brief Check if validation layers are enabled
     */
    bool IsValidationEnabled() const { return m_ValidationEnabled; }

    /**
     * @brief Get physical device properties
     */
    VkPhysicalDeviceProperties GetDeviceProperties() const;

    /**
     * @brief Wait for device to be idle
     */
    void WaitIdle();

private:
    VulkanContext() = default;
    ~VulkanContext() = default;

    // Core Vulkan objects
    VkInstance m_Instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;
    VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
    VkDevice m_Device = VK_NULL_HANDLE;
    VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
    uint32_t m_GraphicsQueueFamily = 0;
    VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
    VmaAllocator m_Allocator = VK_NULL_HANDLE;

    bool m_ValidationEnabled = false;
    bool m_OwnsResources = false;  // false during migration
};

} // namespace Yalaz::Graphics
