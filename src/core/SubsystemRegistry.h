#pragma once

#include "ISubsystem.h"
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include <typeindex>
#include <functional>

namespace Yalaz::Core {

/**
 * @brief Central registry for managing engine subsystems
 *
 * Design Patterns:
 * - Singleton: Single point of access via Get()
 * - Registry: Manages subsystem lifecycle and lookup
 * - Dependency Injection: Subsystems receive dependencies via OnInit()
 *
 * Based on existing PanelManager pattern from Yalaz::UI
 */
class SubsystemRegistry {
public:
    /**
     * @brief Get singleton instance
     * @return Reference to the global registry
     */
    static SubsystemRegistry& Get() {
        static SubsystemRegistry instance;
        return instance;
    }

    // Delete copy/move to enforce singleton
    SubsystemRegistry(const SubsystemRegistry&) = delete;
    SubsystemRegistry& operator=(const SubsystemRegistry&) = delete;
    SubsystemRegistry(SubsystemRegistry&&) = delete;
    SubsystemRegistry& operator=(SubsystemRegistry&&) = delete;

    /**
     * @brief Register and create a new subsystem
     *
     * Template factory pattern - creates subsystem and registers it.
     * Subsystem is NOT initialized until InitAll() is called.
     *
     * @tparam T Subsystem type (must inherit from ISubsystem)
     * @tparam Args Constructor argument types
     * @param args Arguments forwarded to subsystem constructor
     * @return Pointer to created subsystem (owned by registry)
     */
    template<typename T, typename... Args>
    T* Register(Args&&... args) {
        static_assert(std::is_base_of_v<ISubsystem, T>,
            "T must inherit from ISubsystem");

        auto subsystem = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = subsystem.get();

        // Store by name for string lookup
        m_NameMap[ptr->GetName()] = ptr;

        // Store by type for type-safe lookup
        m_TypeMap[std::type_index(typeid(T))] = ptr;

        // Add to ordered list (maintains registration order)
        m_Subsystems.push_back(std::move(subsystem));

        return ptr;
    }

    /**
     * @brief Initialize all registered subsystems in registration order
     *
     * Call this after all subsystems are registered.
     * Subsystems should resolve dependencies in their OnInit().
     */
    void InitAll();

    /**
     * @brief Shutdown all subsystems in reverse registration order
     *
     * LIFO order ensures dependencies are released correctly.
     */
    void ShutdownAll();

    /**
     * @brief Update all updatable subsystems
     * @param deltaTime Time elapsed since last frame
     */
    void UpdateAll(float deltaTime);

    /**
     * @brief Get subsystem by name
     * @param name Subsystem name (from GetName())
     * @return Pointer to subsystem or nullptr if not found
     */
    ISubsystem* GetByName(const std::string& name);

    /**
     * @brief Get subsystem by type (type-safe)
     * @tparam T Subsystem type
     * @return Pointer to subsystem or nullptr if not registered
     */
    template<typename T>
    T* Get() {
        static_assert(std::is_base_of_v<ISubsystem, T>,
            "T must inherit from ISubsystem");

        auto it = m_TypeMap.find(std::type_index(typeid(T)));
        if (it != m_TypeMap.end()) {
            return static_cast<T*>(it->second);
        }
        return nullptr;
    }

    /**
     * @brief Check if a subsystem is registered
     * @tparam T Subsystem type
     * @return true if registered
     */
    template<typename T>
    bool Has() const {
        return m_TypeMap.find(std::type_index(typeid(T))) != m_TypeMap.end();
    }

    /**
     * @brief Get all registered subsystems
     * @return Const reference to subsystem list
     */
    const std::vector<std::unique_ptr<ISubsystem>>& GetAll() const {
        return m_Subsystems;
    }

    /**
     * @brief Get count of registered subsystems
     * @return Number of subsystems
     */
    size_t Count() const { return m_Subsystems.size(); }

    /**
     * @brief Clear all subsystems (calls ShutdownAll first)
     */
    void Clear();

private:
    SubsystemRegistry() = default;
    ~SubsystemRegistry();

    // Ordered list of subsystems (registration order)
    std::vector<std::unique_ptr<ISubsystem>> m_Subsystems;

    // Fast lookup by name
    std::unordered_map<std::string, ISubsystem*> m_NameMap;

    // Fast lookup by type
    std::unordered_map<std::type_index, ISubsystem*> m_TypeMap;

    bool m_Initialized = false;
};

} // namespace Yalaz::Core
