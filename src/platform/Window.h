#pragma once

#include "core/ISubsystem.h"
#include <vulkan/vulkan.h>
#include <string>

struct SDL_Window;

namespace Yalaz::Platform {

/**
 * @brief Window management (SDL wrapper)
 *
 * Design Patterns:
 * - Singleton: Single window for the application
 * - Facade: Simplifies SDL window operations
 *
 * SOLID:
 * - Single Responsibility: Only manages window
 */
class Window : public Core::ISubsystem {
public:
    static Window& Get() {
        static Window instance;
        return instance;
    }

    // ISubsystem interface
    void OnInit() override;
    void OnShutdown() override;
    const char* GetName() const override { return "Window"; }

    // =========================================================================
    // Window Properties
    // =========================================================================

    SDL_Window* GetHandle() const { return m_Window; }

    uint32_t GetWidth() const { return m_Width; }
    uint32_t GetHeight() const { return m_Height; }

    float GetAspectRatio() const {
        return m_Height > 0 ? static_cast<float>(m_Width) / m_Height : 1.0f;
    }

    const std::string& GetTitle() const { return m_Title; }
    void SetTitle(const std::string& title);

    bool IsMinimized() const { return m_Minimized; }
    bool ShouldClose() const { return m_ShouldClose; }

    // =========================================================================
    // Window Operations
    // =========================================================================

    void SetSize(uint32_t width, uint32_t height);
    void SetMinimumSize(uint32_t minWidth, uint32_t minHeight);

    void RequestClose() { m_ShouldClose = true; }
    void CancelClose() { m_ShouldClose = false; }

    /**
     * @brief Handle window resize event
     */
    void OnResize(uint32_t width, uint32_t height);

    /**
     * @brief Handle minimize/restore events
     */
    void OnMinimize(bool minimized) { m_Minimized = minimized; }

    // =========================================================================
    // Vulkan Surface
    // =========================================================================

    /**
     * @brief Create Vulkan surface for this window
     */
    VkSurfaceKHR CreateVulkanSurface(VkInstance instance);

    // =========================================================================
    // Migration Helpers
    // =========================================================================

    void SetWindow(SDL_Window* window, uint32_t width, uint32_t height) {
        m_Window = window;
        m_Width = width;
        m_Height = height;
    }

private:
    Window() = default;
    ~Window() = default;

    SDL_Window* m_Window = nullptr;
    uint32_t m_Width = 1700;
    uint32_t m_Height = 900;
    std::string m_Title = "Yalaz Engine";
    bool m_Minimized = false;
    bool m_ShouldClose = false;
    bool m_OwnsWindow = false;
};

} // namespace Yalaz::Platform
