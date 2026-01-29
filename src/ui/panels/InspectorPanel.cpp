// =============================================================================
// YALAZ ENGINE - Inspector Panel Implementation
// =============================================================================

#include "InspectorPanel.h"
#include "../EditorSelection.h"
#include "../EditorTheme.h"
#include "../../vk_engine.h"
#include "../../vk_loader.h"
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/euler_angles.hpp>

namespace Yalaz::UI {

// Helper function to decompose a transformation matrix
static void DecomposeTransform(const glm::mat4& transform, glm::vec3& position, glm::vec3& rotation, glm::vec3& scale) {
    // Extract position from the last column
    position = glm::vec3(transform[3]);

    // Extract scale from column magnitudes
    scale.x = glm::length(glm::vec3(transform[0]));
    scale.y = glm::length(glm::vec3(transform[1]));
    scale.z = glm::length(glm::vec3(transform[2]));

    // Avoid division by zero
    if (scale.x < 0.0001f) scale.x = 0.0001f;
    if (scale.y < 0.0001f) scale.y = 0.0001f;
    if (scale.z < 0.0001f) scale.z = 0.0001f;

    // Extract rotation matrix by removing scale
    glm::mat3 rotMat(
        glm::vec3(transform[0]) / scale.x,
        glm::vec3(transform[1]) / scale.y,
        glm::vec3(transform[2]) / scale.z
    );

    // Convert rotation matrix to euler angles (YXZ order for intuitive editing)
    rotation.x = asin(-rotMat[2][1]);
    if (cos(rotation.x) > 0.0001f) {
        rotation.y = atan2(rotMat[2][0], rotMat[2][2]);
        rotation.z = atan2(rotMat[0][1], rotMat[1][1]);
    } else {
        rotation.y = atan2(-rotMat[0][2], rotMat[0][0]);
        rotation.z = 0.0f;
    }
}

// Helper function to compose a transformation matrix from components
static glm::mat4 ComposeTransform(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale) {
    glm::mat4 transform = glm::mat4(1.0f);

    // Apply transformations in order: Scale -> Rotate -> Translate
    transform = glm::translate(transform, position);
    transform = glm::rotate(transform, rotation.y, glm::vec3(0, 1, 0));  // Yaw
    transform = glm::rotate(transform, rotation.x, glm::vec3(1, 0, 0));  // Pitch
    transform = glm::rotate(transform, rotation.z, glm::vec3(0, 0, 1));  // Roll
    transform = glm::scale(transform, scale);

    return transform;
}

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

    // Header
    ImGui::PushStyleColor(ImGuiCol_Text, Colors::SelectionBlue);
    ImGui::Text("Scene Node");
    ImGui::PopStyleColor();

    ImGui::Separator();
    ImGui::Spacing();

    // Mesh info
    if (node->mesh) {
        ImGui::Text("Mesh: %s", node->mesh->name.c_str());
        ImGui::TextDisabled("Surfaces: %zu", node->mesh->surfaces.size());
    } else {
        ImGui::TextDisabled("(No mesh attached)");
    }

    ImGui::Spacing();

    // Transform section - EDITABLE
    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent(8.0f);

        // Decompose the current transform
        glm::vec3 position, rotation, scale;
        DecomposeTransform(node->localTransform, position, rotation, scale);

        // Convert rotation to degrees for display
        glm::vec3 rotationDeg = glm::degrees(rotation);

        bool changed = false;

        // Position
        if (RenderVec3Control("Position", position, 0.0f, 0.1f)) {
            changed = true;
        }

        // Rotation (in degrees)
        if (RenderVec3Control("Rotation", rotationDeg, 0.0f, 1.0f)) {
            rotation = glm::radians(rotationDeg);
            changed = true;
        }

        // Scale
        if (RenderVec3Control("Scale", scale, 1.0f, 0.05f)) {
            changed = true;
        }

        // Apply changes
        if (changed) {
            node->localTransform = ComposeTransform(position, rotation, scale);
            // Refresh world transform - get parent matrix
            if (auto parent = node->parent.lock()) {
                node->refreshTransform(parent->worldTransform);
            } else {
                node->refreshTransform(glm::mat4(1.0f));
            }
        }

        ImGui::Spacing();

        // Reset transform button
        if (ImGui::Button("Reset Transform", ImVec2(-1, 0))) {
            node->localTransform = glm::mat4(1.0f);
            if (auto parent = node->parent.lock()) {
                node->refreshTransform(parent->worldTransform);
            } else {
                node->refreshTransform(glm::mat4(1.0f));
            }
        }

        ImGui::Unindent(8.0f);
    }

    // Material info (read-only for now)
    if (node->mesh && ImGui::CollapsingHeader("Materials")) {
        ImGui::Indent(8.0f);

        for (size_t i = 0; i < node->mesh->surfaces.size(); ++i) {
            const auto& surface = node->mesh->surfaces[i];
            ImGui::PushID(static_cast<int>(i));

            ImGui::Text("Surface %zu", i);
            if (surface.material) {
                ImGui::TextDisabled("  Material bound");
            } else {
                ImGui::TextDisabled("  No material");
            }

            ImGui::PopID();
        }

        ImGui::Unindent(8.0f);
    }

    // Statistics
    if (ImGui::CollapsingHeader("Statistics")) {
        ImGui::Indent(8.0f);

        if (node->mesh) {
            size_t totalIndices = 0;
            for (const auto& surface : node->mesh->surfaces) {
                totalIndices += surface.count;
            }
            ImGui::Text("Triangles: %zu", totalIndices / 3);
            ImGui::Text("Surfaces: %zu", node->mesh->surfaces.size());
        }

        ImGui::Text("Children: %zu", node->children.size());

        ImGui::Unindent(8.0f);
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Actions
    ImGui::Text("Actions");
    ImGui::Spacing();

    // Focus camera on object
    if (ImGui::Button("Focus Camera", ImVec2(-1, 0))) {
        glm::vec3 position = glm::vec3(node->worldTransform[3]);
        m_Engine->mainCamera.focusOnPoint(position, 5.0f);
    }

    ImGui::Spacing();

    // Duplicate (create copy)
    if (ImGui::Button("Duplicate", ImVec2(-1, 0))) {
        // TODO: Implement node duplication
        ImGui::TextDisabled("(Not yet implemented)");
    }

    ImGui::Spacing();

    // Deselect button
    if (ImGui::Button("Deselect", ImVec2(-1, 0))) {
        m_Engine->selectedNode = nullptr;
        m_Engine->selectedObjectName.clear();
        EditorSelection::Get().ClearSelection();
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

    // Transform
    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent(8.0f);
        ImGui::DragFloat3("Position", &light.position.x, 0.1f);
        ImGui::Unindent(8.0f);
    }

    // Light properties
    if (ImGui::CollapsingHeader("Light Properties", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent(8.0f);

        ImGui::ColorEdit3("Color", &light.color.x);

        ImGui::Spacing();

        ImGui::DragFloat("Intensity", &light.intensity, 0.1f, 0.0f, 1000.0f);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Light brightness");
        }

        ImGui::DragFloat("Radius", &light.radius, 0.1f, 0.1f, 100.0f);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Light falloff distance");
        }

        ImGui::Unindent(8.0f);
    }

    // Presets
    if (ImGui::CollapsingHeader("Light Presets")) {
        ImGui::Indent(8.0f);

        if (ImGui::Button("Warm", ImVec2(70, 0))) {
            light.color = glm::vec3(1.0f, 0.8f, 0.6f);
            light.intensity = 10.0f;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cool", ImVec2(70, 0))) {
            light.color = glm::vec3(0.6f, 0.8f, 1.0f);
            light.intensity = 10.0f;
        }
        ImGui::SameLine();
        if (ImGui::Button("Sun", ImVec2(70, 0))) {
            light.color = glm::vec3(1.0f, 0.95f, 0.9f);
            light.intensity = 50.0f;
        }

        if (ImGui::Button("Candle", ImVec2(70, 0))) {
            light.color = glm::vec3(1.0f, 0.6f, 0.2f);
            light.intensity = 5.0f;
            light.radius = 3.0f;
        }
        ImGui::SameLine();
        if (ImGui::Button("Neon", ImVec2(70, 0))) {
            light.color = glm::vec3(0.2f, 1.0f, 0.8f);
            light.intensity = 15.0f;
        }
        ImGui::SameLine();
        if (ImGui::Button("Fire", ImVec2(70, 0))) {
            light.color = glm::vec3(1.0f, 0.4f, 0.1f);
            light.intensity = 20.0f;
            light.radius = 8.0f;
        }

        ImGui::Unindent(8.0f);
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Actions
    ImGui::Text("Actions");
    ImGui::Spacing();

    if (ImGui::Button("Focus Camera", ImVec2(-1, 0))) {
        m_Engine->mainCamera.focusOnPoint(light.position, 5.0f);
    }

    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.3f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.9f, 0.4f, 0.4f, 1.0f));

    if (ImGui::Button("Delete Light", ImVec2(-1, 0))) {
        m_Engine->scenePointLights.erase(m_Engine->scenePointLights.begin() + index);
        EditorSelection::Get().ClearSelection();
    }

    ImGui::PopStyleColor(3);
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
