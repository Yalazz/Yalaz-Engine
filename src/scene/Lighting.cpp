#include "Lighting.h"
#include <fmt/core.h>
#include <algorithm>

namespace Yalaz::Scene {

void Lighting::OnInit() {
    fmt::print("[Lighting] Initialized\n");

    // Add a default point light
    PointLight defaultLight{};
    defaultLight.position = glm::vec3(0.0f, 2.0f, 0.0f);
    defaultLight.color = glm::vec3(1.0f, 1.0f, 1.0f);
    defaultLight.intensity = 1.0f;
    defaultLight.radius = 10.0f;

    // Don't add default light - let the scene define lights
    // m_PointLights.push_back(defaultLight);
}

void Lighting::OnShutdown() {
    fmt::print("[Lighting] Shutdown\n");
    ClearPointLights();
}

int Lighting::AddPointLight(const PointLight& light) {
    if (m_PointLights.size() >= MAX_POINT_LIGHTS) {
        fmt::print("[Lighting] Warning: Max point lights ({}) reached\n", MAX_POINT_LIGHTS);
        return -1;
    }

    m_PointLights.push_back(light);
    m_Dirty = true;
    return static_cast<int>(m_PointLights.size() - 1);
}

void Lighting::RemovePointLight(int index) {
    if (index < 0 || index >= static_cast<int>(m_PointLights.size())) {
        return;
    }

    m_PointLights.erase(m_PointLights.begin() + index);
    m_Dirty = true;
}

PointLight* Lighting::GetPointLight(int index) {
    if (index < 0 || index >= static_cast<int>(m_PointLights.size())) {
        return nullptr;
    }
    return &m_PointLights[index];
}

const PointLight* Lighting::GetPointLight(int index) const {
    if (index < 0 || index >= static_cast<int>(m_PointLights.size())) {
        return nullptr;
    }
    return &m_PointLights[index];
}

void Lighting::ClearPointLights() {
    m_PointLights.clear();
    m_Dirty = true;
}

void Lighting::SyncToGPU() {
    if (!m_Dirty || m_PointLightBuffer.buffer == VK_NULL_HANDLE) {
        return;
    }

    // Copy light data to GPU buffer
    if (m_PointLightBuffer.info.pMappedData && !m_PointLights.empty()) {
        size_t dataSize = std::min(
            m_PointLights.size() * sizeof(PointLight),
            m_PointLightBuffer.size
        );
        memcpy(m_PointLightBuffer.info.pMappedData, m_PointLights.data(), dataSize);
    }

    m_Dirty = false;
}

} // namespace Yalaz::Scene
