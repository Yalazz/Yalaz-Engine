#pragma once

#include <string>

namespace Yalaz::Core {

/**
 * @brief Base interface for all engine subsystems
 *
 * Follows the Template Method pattern - defines the algorithm skeleton
 * for subsystem lifecycle while allowing subclasses to override specific steps.
 *
 * SOLID Principles:
 * - Single Responsibility: Each subsystem has one job
 * - Open/Closed: New subsystems can be added without modifying registry
 * - Liskov Substitution: All subsystems can be used polymorphically
 * - Interface Segregation: Minimal interface, specific capabilities via IRenderable, IUpdatable
 * - Dependency Inversion: High-level code depends on this abstraction
 */
class ISubsystem {
public:
    virtual ~ISubsystem() = default;

    /**
     * @brief Initialize the subsystem
     * Called once when the subsystem is registered and ready to start.
     * Dependencies should be resolved here via Get() singletons.
     */
    virtual void OnInit() = 0;

    /**
     * @brief Shutdown the subsystem
     * Called once when the engine is shutting down.
     * Release all resources in reverse order of acquisition.
     */
    virtual void OnShutdown() = 0;

    /**
     * @brief Update the subsystem (optional)
     * Called every frame for subsystems that need per-frame updates.
     * @param deltaTime Time elapsed since last frame in seconds
     */
    virtual void OnUpdate(float deltaTime) { (void)deltaTime; }

    /**
     * @brief Get the subsystem name for debugging and registry lookup
     * @return Human-readable name of the subsystem
     */
    virtual const char* GetName() const = 0;

    /**
     * @brief Check if subsystem is initialized
     * @return true if OnInit() has been called successfully
     */
    bool IsInitialized() const { return m_Initialized; }

protected:
    bool m_Initialized = false;

    // Allow SubsystemRegistry to manage m_Initialized
    friend class SubsystemRegistry;
};

/**
 * @brief Interface for subsystems that can render
 * Implements Interface Segregation - not all subsystems need rendering
 */
class IRenderable {
public:
    virtual ~IRenderable() = default;

    /**
     * @brief Render the subsystem's content
     * @param cmd Active Vulkan command buffer
     */
    virtual void OnRender(struct VkCommandBuffer_T* cmd) = 0;
};

/**
 * @brief Interface for subsystems that need per-frame updates
 * Implements Interface Segregation - not all subsystems need updates
 */
class IUpdatable {
public:
    virtual ~IUpdatable() = default;

    /**
     * @brief Update subsystem state
     * @param deltaTime Time elapsed since last frame in seconds
     */
    virtual void OnUpdate(float deltaTime) = 0;
};

} // namespace Yalaz::Core
