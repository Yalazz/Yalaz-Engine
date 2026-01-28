#include "Engine.h"
#include <fmt/core.h>
#include <chrono>

namespace Yalaz::Core {

void Engine::Init() {
    if (m_Initialized) {
        fmt::print("[Engine] Warning: Already initialized\n");
        return;
    }

    fmt::print("[Engine] Initializing Yalaz Engine...\n");

    // Initialize all registered subsystems
    SubsystemRegistry::Get().InitAll();

    m_Initialized = true;
    m_FrameNumber = 0;
    m_ElapsedTime = 0.0f;

    fmt::print("[Engine] Yalaz Engine initialized successfully\n");
}

void Engine::Run() {
    if (!m_Initialized) {
        fmt::print("[Engine] Error: Engine not initialized\n");
        return;
    }

    m_Running = true;
    fmt::print("[Engine] Starting main loop...\n");

    auto lastTime = std::chrono::high_resolution_clock::now();

    while (m_Running) {
        // Calculate delta time
        auto currentTime = std::chrono::high_resolution_clock::now();
        m_DeltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;
        m_ElapsedTime += m_DeltaTime;

        // Update all subsystems
        SubsystemRegistry::Get().UpdateAll(m_DeltaTime);

        // Note: Rendering is handled by RenderSystem subsystem
        // which will be called via VulkanEngine facade during migration

        m_FrameNumber++;
    }

    fmt::print("[Engine] Main loop ended\n");
}

void Engine::Shutdown() {
    if (!m_Initialized) {
        return;
    }

    fmt::print("[Engine] Shutting down Yalaz Engine...\n");

    m_Running = false;

    // Shutdown all subsystems in reverse order
    SubsystemRegistry::Get().ShutdownAll();

    m_Initialized = false;

    fmt::print("[Engine] Yalaz Engine shut down successfully\n");
}

} // namespace Yalaz::Core
