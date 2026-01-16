// =============================================================================
// YALAZ ENGINE - Lighting Panel Implementation
// =============================================================================

#include "LightingPanel.h"
#include "../EditorSelection.h"
#include "../EditorTheme.h"
#include "../../vk_engine.h"
#include <imgui.h>

namespace Yalaz::UI {

LightingPanel::LightingPanel()
    : Panel("Lighting")
{
}

void LightingPanel::OnRender() {
    if (BeginPanel(ImGuiWindowFlags_NoCollapse)) {
        RenderAmbientSection();
        ImGui::Spacing();
        RenderDirectionalSection();
        ImGui::Spacing();
        RenderPointLightsSection();
    }
    EndPanel();
}

void LightingPanel::RenderAmbientSection() {
    if (!m_Engine) return;

    if (ImGui::CollapsingHeader("Ambient Light", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent(8.0f);

        // Get current values from scene data
        glm::vec4& ambient = m_Engine->sceneData.ambientColor;

        ImGui::ColorEdit3("Color", &ambient.x);
        ImGui::DragFloat("Intensity", &ambient.w, 0.01f, 0.0f, 2.0f);

        ImGui::Unindent(8.0f);
    }
}

void LightingPanel::RenderDirectionalSection() {
    if (!m_Engine) return;

    if (ImGui::CollapsingHeader("Directional Light (Sun)", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent(8.0f);

        glm::vec4& sunDir = m_Engine->sceneData.sunlightDirection;
        glm::vec4& sunColor = m_Engine->sceneData.sunlightColor;

        glm::vec3 direction = glm::vec3(sunDir);
        if (ImGui::DragFloat3("Direction", &direction.x, 0.01f, -1.0f, 1.0f)) {
            sunDir = glm::vec4(glm::normalize(direction), sunDir.w);
        }

        ImGui::ColorEdit3("Color", &sunColor.x);
        ImGui::DragFloat("Intensity", &sunDir.w, 0.01f, 0.0f, 5.0f);

        ImGui::Unindent(8.0f);
    }
}

void LightingPanel::RenderPointLightsSection() {
    if (!m_Engine) return;

    if (ImGui::CollapsingHeader("Point Lights", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent(8.0f);

        auto& lights = m_Engine->scenePointLights;

        // Add light button
        ImGui::PushStyleColor(ImGuiCol_Button, Colors::AccentOrange);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Colors::AccentOrangeHover);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, Colors::AccentOrangeActive);

        if (ImGui::Button("+ Add Point Light", ImVec2(-1, 0))) {
            PointLight newLight;
            newLight.position = m_Engine->mainCamera.position + glm::vec3(0, 2, -3);
            newLight.color = glm::vec3(1.0f);
            newLight.intensity = 1.0f;
            newLight.radius = 10.0f;
            lights.push_back(newLight);
        }

        ImGui::PopStyleColor(3);

        ImGui::Spacing();
        ImGui::Text("Lights: %zu", lights.size());
        ImGui::Spacing();

        // List of lights
        int toDelete = -1;

        for (size_t i = 0; i < lights.size(); ++i) {
            PointLight& light = lights[i];

            ImGui::PushID(static_cast<int>(i));

            char header[64];
            snprintf(header, sizeof(header), "Light %zu", i);

            bool isSelected = EditorSelection::Get().IsLightSelected() &&
                              EditorSelection::Get().GetIndex() == static_cast<int>(i);

            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;
            if (isSelected) flags |= ImGuiTreeNodeFlags_Selected;

            bool opened = ImGui::TreeNodeEx(header, flags);

            if (ImGui::IsItemClicked()) {
                EditorSelection::Get().Select(SelectionType::Light, static_cast<int>(i), header);
            }

            if (opened) {
                ImGui::DragFloat3("Position", &light.position.x, 0.1f);
                ImGui::ColorEdit3("Color", &light.color.x);
                ImGui::DragFloat("Intensity", &light.intensity, 0.1f, 0.0f, 100.0f);
                ImGui::DragFloat("Radius", &light.radius, 0.1f, 0.1f, 100.0f);

                ImGui::Spacing();

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
                if (ImGui::Button("Delete", ImVec2(-1, 0))) {
                    toDelete = static_cast<int>(i);
                }
                ImGui::PopStyleColor();

                ImGui::TreePop();
            }

            ImGui::PopID();
        }

        // Delete marked light
        if (toDelete >= 0 && toDelete < static_cast<int>(lights.size())) {
            lights.erase(lights.begin() + toDelete);
            if (EditorSelection::Get().IsLightSelected() && EditorSelection::Get().GetIndex() == toDelete) {
                EditorSelection::Get().ClearSelection();
            }
        }

        ImGui::Unindent(8.0f);
    }
}

} // namespace Yalaz::UI
