#pragma once

#include "core/ISubsystem.h"
#include "vk_types.h"
#include <vector>

namespace Yalaz::Scene {

/**
 * @brief Manages scene lighting (point lights, directional lights)
 *
 * Design Patterns:
 * - Singleton: Central lighting management
 *
 * SOLID:
 * - Single Responsibility: Only manages lights
 */
class Lighting : public Core::ISubsystem {
public:
    static Lighting& Get() {
        static Lighting instance;
        return instance;
    }

    // ISubsystem interface
    void OnInit() override;
    void OnShutdown() override;
    const char* GetName() const override { return "Lighting"; }

    // =========================================================================
    // Point Lights
    // =========================================================================

    /**
     * @brief Add a point light to the scene
     * @return Index of the new light
     */
    int AddPointLight(const PointLight& light);

    /**
     * @brief Remove a point light by index
     */
    void RemovePointLight(int index);

    /**
     * @brief Get point light at index
     */
    PointLight* GetPointLight(int index);
    const PointLight* GetPointLight(int index) const;

    /**
     * @brief Get all point lights
     */
    std::vector<PointLight>& GetPointLights() { return m_PointLights; }
    const std::vector<PointLight>& GetPointLights() const { return m_PointLights; }

    /**
     * @brief Get point light count
     */
    size_t GetPointLightCount() const { return m_PointLights.size(); }

    /**
     * @brief Clear all point lights
     */
    void ClearPointLights();

    // =========================================================================
    // Ambient Lighting
    // =========================================================================

    void SetAmbientColor(const glm::vec4& color) { m_AmbientColor = color; }
    const glm::vec4& GetAmbientColor() const { return m_AmbientColor; }

    void SetAmbientIntensity(float intensity) { m_AmbientIntensity = intensity; }
    float GetAmbientIntensity() const { return m_AmbientIntensity; }

    // =========================================================================
    // Directional Light (Sun)
    // =========================================================================

    void SetSunDirection(const glm::vec4& dir) { m_SunDirection = dir; }
    const glm::vec4& GetSunDirection() const { return m_SunDirection; }

    void SetSunColor(const glm::vec4& color) { m_SunColor = color; }
    const glm::vec4& GetSunColor() const { return m_SunColor; }

    // =========================================================================
    // GPU Buffer Sync
    // =========================================================================

    /**
     * @brief Get GPU buffer for point lights
     */
    const AllocatedBuffer& GetPointLightBuffer() const { return m_PointLightBuffer; }

    /**
     * @brief Sync point lights to GPU buffer
     * Must be called after modifying lights and before rendering
     */
    void SyncToGPU();

    /**
     * @brief Check if lights have been modified since last sync
     */
    bool IsDirty() const { return m_Dirty; }

    // =========================================================================
    // Migration Helpers
    // =========================================================================

    void SetPointLights(std::vector<PointLight>& lights) {
        m_PointLights = lights;
        m_Dirty = true;
    }
    void SetPointLightBuffer(const AllocatedBuffer& buffer) {
        m_PointLightBuffer = buffer;
    }

private:
    Lighting() = default;
    ~Lighting() = default;

    // Point lights
    std::vector<PointLight> m_PointLights;
    AllocatedBuffer m_PointLightBuffer{};
    bool m_Dirty = true;

    // Ambient lighting
    glm::vec4 m_AmbientColor = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f);
    float m_AmbientIntensity = 0.2f;

    // Directional light (sun)
    glm::vec4 m_SunDirection = glm::vec4(0.0f, -1.0f, 0.0f, 0.0f);
    glm::vec4 m_SunColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
};

} // namespace Yalaz::Scene
