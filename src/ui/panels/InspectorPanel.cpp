// =============================================================================
// YALAZ ENGINE - Inspector Panel Implementation
// =============================================================================

#include "InspectorPanel.h"
#include "../EditorSelection.h"
#include "../EditorTheme.h"
#include "../../vk_engine.h"
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

namespace Yalaz::UI {

InspectorPanel::InspectorPanel()
    : Panel("Inspector")
{
}

void InspectorPanel::OnInit(VulkanEngine* engine) {
    Panel::OnInit(engine);
}

void InspectorPanel::OnRender() {
    if (BeginPanel(ImGuiWindowFlags_NoCollapse)) {
        auto& selection = EditorSelection::Get();

        if (!selection.HasSelection()) {
            RenderNoSelection();
        } else {
            switch (selection.GetType()) {
                case SelectionType::Primitive:
                    RenderPrimitiveInspector(selection.GetIndex());
                    break;
                case SelectionType::SceneNode:
                    RenderSceneNodeInspector();
                    break;
                case SelectionType::Light:
                    RenderLightInspector(selection.GetIndex());
                    break;
                default:
                    RenderNoSelection();
                    break;
            }
        }
    }
    EndPanel();
}

void InspectorPanel::RenderNoSelection() {
    ImGui::TextDisabled("No object selected");
    ImGui::Spacing();
    ImGui::TextWrapped("Select an object from the Scene Hierarchy to edit its properties.");
}

void InspectorPanel::RenderPrimitiveInspector(int index) {
    if (!m_Engine || index < 0 || index >= static_cast<int>(m_Engine->static_shapes.size())) {
        RenderNoSelection();
        return;
    }

    StaticMeshData& mesh = m_Engine->static_shapes[index];

    // Header with name
    ImGui::PushStyleColor(ImGuiCol_Text, Colors::AccentOrange);
    ImGui::Text("Primitive: %s", mesh.name.c_str());
    ImGui::PopStyleColor();

    ImGui::Separator();
    ImGui::Spacing();

    // Editable name
    char nameBuffer[128];
    strncpy(nameBuffer, mesh.name.c_str(), sizeof(nameBuffer) - 1);
    nameBuffer[sizeof(nameBuffer) - 1] = '\0';

    if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer))) {
        mesh.name = nameBuffer;
    }

    ImGui::Spacing();

    // Transform section
    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent(8.0f);
        RenderTransformSection(mesh.position, mesh.rotation, mesh.scale);
        ImGui::Unindent(8.0f);
    }

    // Colors section
    if (ImGui::CollapsingHeader("Colors", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent(8.0f);
        RenderColorSection(mesh.mainColor, mesh.useFaceColors, mesh.faceColors);
        ImGui::Unindent(8.0f);
    }

    // Visibility section
    if (ImGui::CollapsingHeader("Visibility", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent(8.0f);
        ImGui::Checkbox("Visible", &mesh.visible);
        ImGui::Unindent(8.0f);
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Actions
    ImGui::Text("Actions");
    ImGui::Spacing();

    if (ImGui::Button("Focus Camera", ImVec2(-1, 0))) {
        m_Engine->mainCamera.position = mesh.position + glm::vec3(0, 2, 5);
    }

    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.3f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.9f, 0.4f, 0.4f, 1.0f));

    if (ImGui::Button("Delete", ImVec2(-1, 0))) {
        m_Engine->static_shapes.erase(m_Engine->static_shapes.begin() + index);
        EditorSelection::Get().ClearSelection();
    }

    ImGui::PopStyleColor(3);
}

void InspectorPanel::RenderSceneNodeInspector() {
    if (!m_Engine || !m_Engine->selectedNode) {
        RenderNoSelection();
        return;
    }

    MeshNode* node = m_Engine->selectedNode;

    ImGui::PushStyleColor(ImGuiCol_Text, Colors::SelectionBlue);
    ImGui::Text("Scene Node");
    ImGui::PopStyleColor();

    ImGui::Separator();
    ImGui::Spacing();

    if (node->mesh) {
        ImGui::Text("Mesh: %s", node->mesh->name.c_str());
    }

    ImGui::Spacing();

    // Transform display (read-only for now)
    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
        glm::mat4& transform = node->localTransform;

        glm::vec3 position = glm::vec3(transform[3]);
        ImGui::Text("Position: %.2f, %.2f, %.2f", position.x, position.y, position.z);
    }
}

void InspectorPanel::RenderLightInspector(int index) {
    if (!m_Engine || index < 0 || index >= static_cast<int>(m_Engine->scenePointLights.size())) {
        RenderNoSelection();
        return;
    }

    PointLight& light = m_Engine->scenePointLights[index];

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.9f, 0.5f, 1.0f));
    ImGui::Text("Point Light %d", index);
    ImGui::PopStyleColor();

    ImGui::Separator();
    ImGui::Spacing();

    ImGui::DragFloat3("Position", &light.position.x, 0.1f);
    ImGui::ColorEdit3("Color", &light.color.x);
    ImGui::DragFloat("Intensity", &light.intensity, 0.1f, 0.0f, 100.0f);
    ImGui::DragFloat("Radius", &light.radius, 0.1f, 0.1f, 100.0f);
}

void InspectorPanel::RenderTransformSection(glm::vec3& position, glm::vec3& rotation, glm::vec3& scale) {
    RenderVec3Control("Position", position, 0.0f, 0.1f);

    glm::vec3 rotationDeg = glm::degrees(rotation);
    if (RenderVec3Control("Rotation", rotationDeg, 0.0f, 1.0f)) {
        rotation = glm::radians(rotationDeg);
    }

    RenderVec3Control("Scale", scale, 1.0f, 0.05f);

    ImGui::Spacing();

    if (ImGui::Button("Reset Transform", ImVec2(-1, 0))) {
        position = glm::vec3(0.0f);
        rotation = glm::vec3(0.0f);
        scale = glm::vec3(1.0f);
    }
}

void InspectorPanel::RenderColorSection(glm::vec4& mainColor, bool& useFaceColors, glm::vec4* faceColors) {
    ImGui::ColorEdit4("Main Color", &mainColor.x, ImGuiColorEditFlags_AlphaBar);

    ImGui::Spacing();
    ImGui::Checkbox("Use Face Colors", &useFaceColors);

    if (useFaceColors) {
        ImGui::Spacing();
        const char* faceNames[] = { "Front (+Z)", "Back (-Z)", "Right (+X)", "Left (-X)", "Top (+Y)", "Bottom (-Y)" };

        for (int i = 0; i < 6; i++) {
            ImGui::PushID(i);
            ImGui::ColorEdit4(faceNames[i], &faceColors[i].x, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
            ImGui::PopID();
        }
    }
}

bool InspectorPanel::RenderVec3Control(const char* label, glm::vec3& values, float resetValue, float speed) {
    bool changed = false;

    ImGui::PushID(label);

    ImGui::Columns(2, nullptr, false);
    ImGui::SetColumnWidth(0, 80.0f);

    ImGui::Text("%s", label);
    ImGui::NextColumn();

    // Calculate width for each component (total width / 3 items - spacing)
    float totalWidth = ImGui::CalcItemWidth();
    float itemWidth = (totalWidth - 24.0f * 3 - 8.0f * 2) / 3.0f;  // 24 for button, 8 for spacing

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));

    // X
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.1f, 0.15f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.2f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.1f, 0.15f, 1.0f));
    if (ImGui::Button("X", ImVec2(20, 0))) {
        values.x = resetValue;
        changed = true;
    }
    ImGui::PopStyleColor(3);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(itemWidth);
    if (ImGui::DragFloat("##X", &values.x, speed, 0.0f, 0.0f, "%.2f")) changed = true;
    ImGui::SameLine();

    // Y
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.8f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
    if (ImGui::Button("Y", ImVec2(20, 0))) {
        values.y = resetValue;
        changed = true;
    }
    ImGui::PopStyleColor(3);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(itemWidth);
    if (ImGui::DragFloat("##Y", &values.y, speed, 0.0f, 0.0f, "%.2f")) changed = true;
    ImGui::SameLine();

    // Z
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.25f, 0.8f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.35f, 0.9f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.25f, 0.8f, 1.0f));
    if (ImGui::Button("Z", ImVec2(20, 0))) {
        values.z = resetValue;
        changed = true;
    }
    ImGui::PopStyleColor(3);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(itemWidth);
    if (ImGui::DragFloat("##Z", &values.z, speed, 0.0f, 0.0f, "%.2f")) changed = true;

    ImGui::PopStyleVar();
    ImGui::Columns(1);
    ImGui::PopID();

    return changed;
}

} // namespace Yalaz::UI
