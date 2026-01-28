#include "Window.h"
#include <SDL.h>
#include <SDL_vulkan.h>
#include <fmt/core.h>

namespace Yalaz::Platform {

void Window::OnInit() {
    fmt::print("[Window] Initialized\n");

    // During migration, VulkanEngine creates the window
    // and calls SetWindow() to register it
}

void Window::OnShutdown() {
    fmt::print("[Window] Shutdown\n");

    if (m_OwnsWindow && m_Window) {
        SDL_DestroyWindow(m_Window);
        m_Window = nullptr;
    }
}

void Window::SetTitle(const std::string& title) {
    m_Title = title;
    if (m_Window) {
        SDL_SetWindowTitle(m_Window, title.c_str());
    }
}

void Window::SetSize(uint32_t width, uint32_t height) {
    m_Width = width;
    m_Height = height;
    if (m_Window) {
        SDL_SetWindowSize(m_Window, static_cast<int>(width), static_cast<int>(height));
    }
}

void Window::SetMinimumSize(uint32_t minWidth, uint32_t minHeight) {
    if (m_Window) {
        SDL_SetWindowMinimumSize(m_Window, static_cast<int>(minWidth), static_cast<int>(minHeight));
    }
}

void Window::OnResize(uint32_t width, uint32_t height) {
    m_Width = width;
    m_Height = height;
    m_Minimized = (width == 0 || height == 0);
}

VkSurfaceKHR Window::CreateVulkanSurface(VkInstance instance) {
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    if (m_Window && instance) {
        if (!SDL_Vulkan_CreateSurface(m_Window, instance, &surface)) {
            fmt::print("[Window] Failed to create Vulkan surface: {}\n", SDL_GetError());
        }
    }
    return surface;
}

} // namespace Yalaz::Platform
