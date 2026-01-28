#include "VulkanContext.h"
#include <fmt/core.h>

namespace Yalaz::Graphics {

void VulkanContext::OnInit() {
    fmt::print("[VulkanContext] Initialized\n");

    // During migration phase, VulkanEngine will call Set* methods
    // to provide the Vulkan objects it creates.
    // Once migration is complete, this will create objects directly.
}

void VulkanContext::OnShutdown() {
    fmt::print("[VulkanContext] Shutdown\n");

    // During migration, VulkanEngine owns and destroys the resources.
    // Only destroy if we own the resources.
    if (m_OwnsResources) {
        if (m_Allocator) {
            vmaDestroyAllocator(m_Allocator);
            m_Allocator = VK_NULL_HANDLE;
        }

        if (m_Device) {
            vkDestroyDevice(m_Device, nullptr);
            m_Device = VK_NULL_HANDLE;
        }

        if (m_Surface) {
            vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
            m_Surface = VK_NULL_HANDLE;
        }

        if (m_DebugMessenger) {
            auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)
                vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugUtilsMessengerEXT");
            if (func) {
                func(m_Instance, m_DebugMessenger, nullptr);
            }
            m_DebugMessenger = VK_NULL_HANDLE;
        }

        if (m_Instance) {
            vkDestroyInstance(m_Instance, nullptr);
            m_Instance = VK_NULL_HANDLE;
        }
    }

    m_PhysicalDevice = VK_NULL_HANDLE;
    m_GraphicsQueue = VK_NULL_HANDLE;
}

VkPhysicalDeviceProperties VulkanContext::GetDeviceProperties() const {
    VkPhysicalDeviceProperties props{};
    if (m_PhysicalDevice) {
        vkGetPhysicalDeviceProperties(m_PhysicalDevice, &props);
    }
    return props;
}

void VulkanContext::WaitIdle() {
    if (m_Device) {
        vkDeviceWaitIdle(m_Device);
    }
}

} // namespace Yalaz::Graphics
