#pragma once

#include "SubsystemRegistry.h"

namespace Yalaz::Core {

/**
 * @brief Main engine orchestrator (slim coordinator)
 *
 * Design Patterns:
 * - Singleton: Single engine instance via Get()
 * - Facade: Simple interface to complex subsystem interactions
 * - Composition: Owns SubsystemRegistry, delegates to subsystems
 *
 * SOLID: Single Responsibility - only orchestrates, doesn't implement
 */
class Engine {
public:
    /**
     * @brief Get singleton instance
     * @return Reference to the global engine
     */
    static Engine& Get() {
        static Engine instance;
        return instance;
    }

    // Delete copy/move to enforce singleton
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;
    Engine(Engine&&) = delete;
    Engine& operator=(Engine&&) = delete;

    /**
     * @brief Initialize the engine and all subsystems
     *
     * Registration order determines initialization order.
     * Subsystems should be registered before calling this.
     */
    void Init();

    /**
     * @brief Run the main engine loop
     *
     * Handles frame timing, input, updates, and rendering.
     * Returns when the engine should exit.
     */
    void Run();

    /**
     * @brief Shutdown the engine and all subsystems
     *
     * Subsystems are shut down in reverse registration order.
     */
    void Shutdown();

    /**
     * @brief Request engine to stop running
     */
    void RequestExit() { m_Running = false; }

    /**
     * @brief Check if engine is running
     * @return true if Run() is active
     */
    bool IsRunning() const { return m_Running; }

    /**
     * @brief Check if engine is initialized
     * @return true if Init() has been called
     */
    bool IsInitialized() const { return m_Initialized; }

    /**
     * @brief Get the subsystem registry
     * @return Reference to the registry for subsystem access
     */
    SubsystemRegistry& GetRegistry() { return SubsystemRegistry::Get(); }

    /**
     * @brief Convenience method to get a subsystem by type
     * @tparam T Subsystem type
     * @return Pointer to subsystem or nullptr
     */
    template<typename T>
    T* GetSubsystem() {
        return SubsystemRegistry::Get().Get<T>();
    }

    /**
     * @brief Get current frame delta time
     * @return Time since last frame in seconds
     */
    float GetDeltaTime() const { return m_DeltaTime; }

    /**
     * @brief Get total elapsed time since engine start
     * @return Elapsed time in seconds
     */
    float GetElapsedTime() const { return m_ElapsedTime; }

    /**
     * @brief Get current frame number
     * @return Frame count since engine start
     */
    uint64_t GetFrameNumber() const { return m_FrameNumber; }

private:
    Engine() = default;
    ~Engine() = default;

    bool m_Initialized = false;
    bool m_Running = false;

    // Frame timing
    float m_DeltaTime = 0.0f;
    float m_ElapsedTime = 0.0f;
    uint64_t m_FrameNumber = 0;
};

} // namespace Yalaz::Core
