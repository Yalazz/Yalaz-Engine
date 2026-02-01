// =============================================================================
// YALAZ ENGINE - Object Inspector View Implementation
// =============================================================================
// Professional property inspector for scene objects with ALL features:
// - Editable name field
// - Transform editing (position, rotation, scale) with colored axes
// - Material properties with type selectors
// - Dynamic face colors per primitive type (cube=6, sphere=4, cylinder=3, etc.)
// - Focus Camera button
// - Statistics (triangles, surfaces)
// - Light presets (Warm White, Cool White, Sun, Candle, Neon, Fire)
// - Duplicate, Reset, Delete actions
// - Debug info with transform matrix preview
// =============================================================================

#include "ObjectInspectorView.h"
#include "../EditorTheme.h"
#include "../../vk_engine.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/quaternion.hpp>
#include <cstring>
#include <cstdlib>

namespace Yalaz::UI {

// =============================================================================
// PRIMITIVE STATISTICS - Triangle counts per primitive type
// =============================================================================

static const struct PrimitiveStats {
    int triangleCount;
    int vertexCount;
    const char* description;
} s_PrimitiveStats[] = {
    { 12, 24, "6 faces, 2 triangles each" },      // Cube
    { 960, 482, "32x15 subdivisions" },           // Sphere (approximation)
    { 192, 98, "32 segments" },                   // Cylinder
    { 96, 49, "32 segments" },                    // Cone
    { 640, 322, "16 segments, 8 rings" },         // Capsule
    { 1152, 578, "32 segments, 16 rings" },       // Torus
    { 2, 4, "Single quad" },                      // Plane
    { 1, 3, "Single triangle" }                   // Triangle
};

// =============================================================================
// MAIN RENDER
// =============================================================================

void ObjectInspectorView::OnRender() {
    if (!BeginView()) {
        EndView();
        return;
    }

    if (!m_Engine) {
        RenderNoSelection();
        EndView();
        return;
    }

    // Check what's selected - Priority: Primitive > Light > Node
    if (m_Engine->selectedPrimitiveIndex >= 0 &&
        m_Engine->selectedPrimitiveIndex < static_cast<int>(m_Engine->static_shapes.size())) {
        RenderPrimitiveInspector(m_Engine->selectedPrimitiveIndex);
    }
    else if (m_Engine->selectedLightIndex >= 0 &&
             m_Engine->selectedLightIndex < static_cast<int>(m_Engine->scenePointLights.size())) {
        RenderLightInspector(m_Engine->selectedLightIndex);
    }
    else if (m_Engine->selectedNode != nullptr) {
        RenderSceneNodeInspector();
    }
    else {
        RenderNoSelection();
    }

    EndView();
}

// =============================================================================
// NO SELECTION
// =============================================================================

void ObjectInspectorView::RenderNoSelection() {
    ImGui::TextDisabled("No object selected");
    ImGui::Spacing();
    ImGui::TextWrapped("Select an object in the Hierarchy or Scene view to inspect its properties.");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Quick stats
    ImGui::TextDisabled("Scene Statistics:");
    if (m_Engine) {
        ImGui::Text("Primitives: %zu", m_Engine->static_shapes.size());
        ImGui::Text("Lights: %zu", m_Engine->scenePointLights.size());
        ImGui::Text("Scenes: %zu", m_Engine->loadedScenes.size());

        // Total triangle count
        int totalTris = 0;
        for (const auto& shape : m_Engine->static_shapes) {
            int typeIdx = static_cast<int>(shape.type);
            if (typeIdx >= 0 && typeIdx < 8) {
                totalTris += s_PrimitiveStats[typeIdx].triangleCount;
            }
        }
        ImGui::Text("Total Triangles: ~%d", totalTris);
    }
}

// =============================================================================
// PRIMITIVE INSPECTOR - FULL FEATURED
// =============================================================================

void ObjectInspectorView::RenderPrimitiveInspector(int index) {
    auto& shape = m_Engine->static_shapes[index];

    // Header - Primitive type name
    const char* typeNames[] = { "Cube", "Sphere", "Cylinder", "Cone", "Capsule", "Torus", "Plane", "Triangle" };
    int typeIndex = static_cast<int>(shape.type);
    const char* typeName = (typeIndex >= 0 && typeIndex < 8) ? typeNames[typeIndex] : "Primitive";

    // Editable name field
    if (m_LastEditedIndex != index) {
        // Load name into buffer when selection changes
        strncpy(m_NameBuffer, shape.name.c_str(), sizeof(m_NameBuffer) - 1);
        m_NameBuffer[sizeof(m_NameBuffer) - 1] = '\0';
        m_LastEditedIndex = index;
    }

    ImGui::Text("Name:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-1);
    if (ImGui::InputText("##Name", m_NameBuffer, sizeof(m_NameBuffer))) {
        shape.name = m_NameBuffer;
    }

    ImGui::TextDisabled("Type: %s", typeName);

    ImGui::Spacing();

    // Visibility and selection toggles on same line
    ImGui::Checkbox("Visible", &shape.visible);
    ImGui::SameLine();
    ImGui::Checkbox("Selected", &shape.selected);
    ImGui::SameLine(ImGui::GetWindowWidth() - 110);

    // Focus Camera button (in header area)
    if (ImGui::Button("Focus Camera", ImVec2(100, 0))) {
        m_Engine->mainCamera.position = shape.position + glm::vec3(0, 2, 5);
    }

    ImGui::Spacing();
    ImGui::Separator();

    // ==========================================================================
    // TRANSFORM SECTION
    // ==========================================================================
    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();

        // Work with copies, then apply back
        glm::vec3 pos = shape.position;
        glm::vec3 rot = glm::degrees(shape.rotation);
        glm::vec3 scl = shape.scale;

        RenderTransformEditor(pos, rot, scl);

        shape.position = pos;
        shape.rotation = glm::radians(rot);
        shape.scale = scl;

        // Quick transform buttons
        ImGui::Spacing();
        if (ImGui::Button("Reset Position")) {
            shape.position = glm::vec3(0.0f);
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset Rotation")) {
            shape.rotation = glm::vec3(0.0f);
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset Scale")) {
            shape.scale = glm::vec3(1.0f);
        }

        // Snap buttons
        ImGui::Spacing();
        ImGui::TextDisabled("Snap:");
        ImGui::SameLine();
        if (ImGui::Button("Grid 1")) {
            shape.position = glm::round(shape.position);
        }
        ImGui::SameLine();
        if (ImGui::Button("Grid 0.5")) {
            shape.position = glm::round(shape.position * 2.0f) / 2.0f;
        }
        ImGui::SameLine();
        if (ImGui::Button("Grid 0.25")) {
            shape.position = glm::round(shape.position * 4.0f) / 4.0f;
        }

        ImGui::Unindent();
    }

    // ==========================================================================
    // MATERIAL SECTION
    // ==========================================================================
    if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();

        // Main color
        RenderColorEditor4("Main Color", shape.mainColor);

        // Material type selector
        const char* materialNames[] = { "Default", "Unlit", "PBR", "Normal Debug", "Wireframe" };
        int matType = static_cast<int>(shape.materialType);
        if (ImGui::Combo("Material Type", &matType, materialNames, 5)) {
            shape.materialType = static_cast<ShaderOnlyMaterial>(matType);
        }

        // Pass type selector
        const char* passNames[] = { "Main Color", "Shadow", "Wireframe", "Selection" };
        int passType = static_cast<int>(shape.passType);
        if (ImGui::Combo("Pass Type", &passType, passNames, 4)) {
            shape.passType = static_cast<MaterialPass>(passType);
        }

        // Color presets
        ImGui::Spacing();
        ImGui::TextDisabled("Color Presets:");
        if (ImGui::Button("Red", ImVec2(50, 0))) shape.mainColor = glm::vec4(1, 0.2f, 0.2f, 1);
        ImGui::SameLine();
        if (ImGui::Button("Green", ImVec2(50, 0))) shape.mainColor = glm::vec4(0.2f, 1, 0.2f, 1);
        ImGui::SameLine();
        if (ImGui::Button("Blue", ImVec2(50, 0))) shape.mainColor = glm::vec4(0.2f, 0.2f, 1, 1);
        ImGui::SameLine();
        if (ImGui::Button("White", ImVec2(50, 0))) shape.mainColor = glm::vec4(1, 1, 1, 1);
        if (ImGui::Button("Gray", ImVec2(50, 0))) shape.mainColor = glm::vec4(0.5f, 0.5f, 0.5f, 1);
        ImGui::SameLine();
        if (ImGui::Button("Gold", ImVec2(50, 0))) shape.mainColor = glm::vec4(1, 0.84f, 0, 1);
        ImGui::SameLine();
        if (ImGui::Button("Silver", ImVec2(50, 0))) shape.mainColor = glm::vec4(0.75f, 0.75f, 0.75f, 1);
        ImGui::SameLine();
        if (ImGui::Button("Bronze", ImVec2(50, 0))) shape.mainColor = glm::vec4(0.8f, 0.5f, 0.2f, 1);

        ImGui::Unindent();
    }

    // ==========================================================================
    // DYNAMIC FACE COLORS SECTION - Per primitive type
    // ==========================================================================
    if (ImGui::CollapsingHeader("Face Colors")) {
        ImGui::Indent();
        RenderDynamicFaceColorEditor(typeIndex, shape.faceColors, shape.useFaceColors);
        ImGui::Unindent();
    }

    // ==========================================================================
    // STATISTICS SECTION
    // ==========================================================================
    if (ImGui::CollapsingHeader("Statistics")) {
        ImGui::Indent();
        RenderPrimitiveStatistics(typeIndex);

        // Bounding box info
        ImGui::Spacing();
        ImGui::TextDisabled("Bounding Box:");
        glm::vec3 minB = shape.position - shape.scale * 0.5f;
        glm::vec3 maxB = shape.position + shape.scale * 0.5f;
        ImGui::Text("Min: (%.2f, %.2f, %.2f)", minB.x, minB.y, minB.z);
        ImGui::Text("Max: (%.2f, %.2f, %.2f)", maxB.x, maxB.y, maxB.z);
        glm::vec3 size = maxB - minB;
        ImGui::Text("Size: (%.2f, %.2f, %.2f)", size.x, size.y, size.z);

        ImGui::Unindent();
    }

    // ==========================================================================
    // ACTIONS SECTION
    // ==========================================================================
    if (ImGui::CollapsingHeader("Actions")) {
        ImGui::Indent();

        // Focus camera
        if (ImGui::Button("Focus Camera", ImVec2(-1, 0))) {
            m_Engine->mainCamera.position = shape.position + glm::vec3(0, 2, 5);
        }

        // Duplicate
        if (ImGui::Button("Duplicate", ImVec2(-1, 0))) {
            auto copy = shape;
            copy.position += glm::vec3(1, 0, 1);
            copy.name = shape.name + " (Copy)";
            copy.selected = false;
            m_Engine->static_shapes.push_back(copy);
        }

        // Reset all
        if (ImGui::Button("Reset Transform", ImVec2(-1, 0))) {
            shape.position = glm::vec3(0);
            shape.rotation = glm::vec3(0);
            shape.scale = glm::vec3(1);
        }

        // Center at origin
        if (ImGui::Button("Move to Origin", ImVec2(-1, 0))) {
            shape.position = glm::vec3(0.0f);
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Delete (with confirmation)
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.3f, 0.3f, 1.0f));
        if (ImGui::Button("Delete", ImVec2(-1, 0))) {
            m_Engine->static_shapes.erase(m_Engine->static_shapes.begin() + index);
            m_Engine->selectedPrimitiveIndex = -1;
            m_LastEditedIndex = -1;
        }
        ImGui::PopStyleColor(2);

        ImGui::Unindent();
    }

    // ==========================================================================
    // DEBUG INFO SECTION
    // ==========================================================================
    if (ImGui::CollapsingHeader("Debug Info")) {
        ImGui::Indent();
        ImGui::TextDisabled("Index: %d", index);
        ImGui::TextDisabled("Type ID: %d", static_cast<int>(shape.type));
        ImGui::TextDisabled("Material ID: %d", static_cast<int>(shape.materialType));
        bool hasMesh = (shape.mesh.indexBuffer.buffer != VK_NULL_HANDLE);
        ImGui::TextDisabled("Has Mesh: %s", hasMesh ? "Yes" : "No");

        // Transform matrix preview
        auto transform = shape.get_transform();
        ImGui::Spacing();
        ImGui::TextDisabled("Transform Matrix:");
        ImGui::Text("[%.2f, %.2f, %.2f, %.2f]", transform[0][0], transform[1][0], transform[2][0], transform[3][0]);
        ImGui::Text("[%.2f, %.2f, %.2f, %.2f]", transform[0][1], transform[1][1], transform[2][1], transform[3][1]);
        ImGui::Text("[%.2f, %.2f, %.2f, %.2f]", transform[0][2], transform[1][2], transform[2][2], transform[3][2]);
        ImGui::Text("[%.2f, %.2f, %.2f, %.2f]", transform[0][3], transform[1][3], transform[2][3], transform[3][3]);

        // Memory address (for debugging)
        ImGui::Spacing();
        ImGui::TextDisabled("Memory: %p", static_cast<void*>(&shape));

        ImGui::Unindent();
    }
}

// =============================================================================
// DYNAMIC FACE COLOR EDITOR - Per Primitive Type
// =============================================================================

void ObjectInspectorView::RenderDynamicFaceColorEditor(int primitiveType, glm::vec4* faceColors, bool& useFaceColors) {
    // Get the face configuration for this primitive type
    const auto& config = HierarchyView::GetFaceConfig(primitiveType);

    ImGui::Checkbox("Use Face Colors", &useFaceColors);

    if (!useFaceColors) {
        ImGui::TextDisabled("Enable to edit individual face colors");
        return;
    }

    ImGui::Spacing();
    ImGui::TextDisabled("%d faces for this primitive type", config.faceCount);
    ImGui::Separator();
    ImGui::Spacing();

    // Render color editor for each face
    for (int i = 0; i < config.faceCount && i < 6; ++i) {
        ImGui::PushID(i);
        float col[4] = { faceColors[i].x, faceColors[i].y, faceColors[i].z, faceColors[i].w };
        if (ImGui::ColorEdit4(config.faceNames[i], col, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar)) {
            faceColors[i] = glm::vec4(col[0], col[1], col[2], col[3]);
        }
        ImGui::PopID();
    }

    // Preset buttons
    ImGui::Spacing();
    ImGui::TextDisabled("Presets:");

    if (ImGui::Button("Default", ImVec2(60, 0))) {
        for (int i = 0; i < config.faceCount && i < 6; ++i) {
            faceColors[i] = config.defaultColors[i];
        }
    }
    ImGui::SameLine();

    if (ImGui::Button("Rainbow", ImVec2(60, 0))) {
        const glm::vec4 rainbow[] = {
            glm::vec4(1, 0, 0, 1), glm::vec4(1, 0.5f, 0, 1),
            glm::vec4(1, 1, 0, 1), glm::vec4(0, 1, 0, 1),
            glm::vec4(0, 1, 1, 1), glm::vec4(0, 0, 1, 1)
        };
        for (int i = 0; i < config.faceCount && i < 6; ++i) {
            faceColors[i] = rainbow[i];
        }
    }
    ImGui::SameLine();

    if (ImGui::Button("White", ImVec2(60, 0))) {
        for (int i = 0; i < 6; ++i) {
            faceColors[i] = glm::vec4(1, 1, 1, 1);
        }
    }
    ImGui::SameLine();

    if (ImGui::Button("Random", ImVec2(60, 0))) {
        for (int i = 0; i < config.faceCount && i < 6; ++i) {
            faceColors[i] = glm::vec4(
                static_cast<float>(rand()) / RAND_MAX,
                static_cast<float>(rand()) / RAND_MAX,
                static_cast<float>(rand()) / RAND_MAX,
                1.0f
            );
        }
    }

    // Gradient buttons
    ImGui::Spacing();
    if (ImGui::Button("Gradient V", ImVec2(70, 0))) {
        for (int i = 0; i < config.faceCount && i < 6; ++i) {
            float t = static_cast<float>(i) / static_cast<float>(config.faceCount - 1);
            faceColors[i] = glm::vec4(t, t, t, 1.0f);
        }
    }
    ImGui::SameLine();

    if (ImGui::Button("Warm Grad", ImVec2(70, 0))) {
        for (int i = 0; i < config.faceCount && i < 6; ++i) {
            float t = static_cast<float>(i) / static_cast<float>(config.faceCount - 1);
            faceColors[i] = glm::vec4(1.0f, 0.5f + t * 0.5f, t * 0.5f, 1.0f);
        }
    }
    ImGui::SameLine();

    if (ImGui::Button("Cool Grad", ImVec2(70, 0))) {
        for (int i = 0; i < config.faceCount && i < 6; ++i) {
            float t = static_cast<float>(i) / static_cast<float>(config.faceCount - 1);
            faceColors[i] = glm::vec4(t * 0.5f, 0.5f + t * 0.5f, 1.0f, 1.0f);
        }
    }
}

// =============================================================================
// PRIMITIVE STATISTICS
// =============================================================================

void ObjectInspectorView::RenderPrimitiveStatistics(int primitiveType) {
    if (primitiveType < 0 || primitiveType >= 8) {
        ImGui::TextDisabled("Unknown primitive type");
        return;
    }

    const auto& stats = s_PrimitiveStats[primitiveType];
    const auto& faceConfig = HierarchyView::GetFaceConfig(primitiveType);

    ImGui::Text("Triangles: ~%d", stats.triangleCount);
    ImGui::Text("Vertices: ~%d", stats.vertexCount);
    ImGui::Text("Faces: %d", faceConfig.faceCount);
    ImGui::TextDisabled("(%s)", stats.description);
}

// =============================================================================
// LIGHT INSPECTOR - FULL FEATURED
// =============================================================================

void ObjectInspectorView::RenderLightInspector(int index) {
    if (index < 0 || index >= static_cast<int>(m_Engine->scenePointLights.size())) return;

    auto& light = m_Engine->scenePointLights[index];

    SectionHeader("Point Light");

    // Focus button in header
    ImGui::SameLine(ImGui::GetWindowWidth() - 110);
    if (ImGui::Button("Focus Camera", ImVec2(100, 0))) {
        m_Engine->mainCamera.position = light.position + glm::vec3(0, 2, 5);
    }

    ImGui::Spacing();
    ImGui::Separator();

    // Position
    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();
        ImGui::DragFloat3("Position", glm::value_ptr(light.position), 0.1f);

        // Quick position buttons
        ImGui::Spacing();
        if (ImGui::Button("Move to Origin")) {
            light.position = glm::vec3(0.0f, 2.0f, 0.0f);
        }
        ImGui::SameLine();
        if (ImGui::Button("Above Camera")) {
            light.position = m_Engine->mainCamera.position + glm::vec3(0, 3, 0);
        }

        ImGui::Unindent();
    }

    // Light properties
    if (ImGui::CollapsingHeader("Light Properties", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();

        RenderColorEditor("Color", light.color);
        ImGui::SliderFloat("Intensity", &light.intensity, 0.0f, 100.0f, "%.1f");
        ImGui::SliderFloat("Radius", &light.radius, 0.1f, 100.0f, "%.1f");

        // Attenuation preview
        ImGui::Spacing();
        ImGui::TextDisabled("Attenuation at distances:");
        for (float d : {1.0f, 5.0f, 10.0f, 20.0f}) {
            float atten = 1.0f / (d * d);
            float effectiveIntensity = light.intensity * atten;
            ImGui::Text("  %.0fm: %.2f", d, effectiveIntensity);
        }

        ImGui::Unindent();
    }

    // Light presets - EXPANDED
    if (ImGui::CollapsingHeader("Presets", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();

        ImGui::TextDisabled("Standard:");
        if (ImGui::Button("Warm White", ImVec2(85, 0))) {
            light.color = glm::vec3(1.0f, 0.9f, 0.8f);
            light.intensity = 10.0f;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cool White", ImVec2(85, 0))) {
            light.color = glm::vec3(0.9f, 0.95f, 1.0f);
            light.intensity = 10.0f;
        }
        ImGui::SameLine();
        if (ImGui::Button("Neutral", ImVec2(85, 0))) {
            light.color = glm::vec3(1.0f, 1.0f, 1.0f);
            light.intensity = 10.0f;
        }

        ImGui::Spacing();
        ImGui::TextDisabled("Natural:");
        if (ImGui::Button("Sun", ImVec2(70, 0))) {
            light.color = glm::vec3(1.0f, 0.95f, 0.85f);
            light.intensity = 50.0f;
            light.radius = 50.0f;
        }
        ImGui::SameLine();
        if (ImGui::Button("Moonlight", ImVec2(70, 0))) {
            light.color = glm::vec3(0.7f, 0.8f, 1.0f);
            light.intensity = 2.0f;
            light.radius = 30.0f;
        }
        ImGui::SameLine();
        if (ImGui::Button("Candle", ImVec2(70, 0))) {
            light.color = glm::vec3(1.0f, 0.6f, 0.2f);
            light.intensity = 3.0f;
            light.radius = 5.0f;
        }
        ImGui::SameLine();
        if (ImGui::Button("Fire", ImVec2(70, 0))) {
            light.color = glm::vec3(1.0f, 0.4f, 0.1f);
            light.intensity = 15.0f;
            light.radius = 12.0f;
        }

        ImGui::Spacing();
        ImGui::TextDisabled("Artificial:");
        if (ImGui::Button("Neon Pink", ImVec2(85, 0))) {
            light.color = glm::vec3(1.0f, 0.1f, 0.6f);
            light.intensity = 8.0f;
            light.radius = 8.0f;
        }
        ImGui::SameLine();
        if (ImGui::Button("Neon Blue", ImVec2(85, 0))) {
            light.color = glm::vec3(0.1f, 0.5f, 1.0f);
            light.intensity = 8.0f;
            light.radius = 8.0f;
        }
        ImGui::SameLine();
        if (ImGui::Button("Neon Green", ImVec2(85, 0))) {
            light.color = glm::vec3(0.2f, 1.0f, 0.3f);
            light.intensity = 8.0f;
            light.radius = 8.0f;
        }

        if (ImGui::Button("LED Warm", ImVec2(85, 0))) {
            light.color = glm::vec3(1.0f, 0.85f, 0.7f);
            light.intensity = 12.0f;
            light.radius = 10.0f;
        }
        ImGui::SameLine();
        if (ImGui::Button("LED Cool", ImVec2(85, 0))) {
            light.color = glm::vec3(0.8f, 0.9f, 1.0f);
            light.intensity = 12.0f;
            light.radius = 10.0f;
        }
        ImGui::SameLine();
        if (ImGui::Button("Halogen", ImVec2(85, 0))) {
            light.color = glm::vec3(1.0f, 0.95f, 0.9f);
            light.intensity = 20.0f;
            light.radius = 15.0f;
        }

        ImGui::Spacing();
        ImGui::TextDisabled("Dramatic:");
        if (ImGui::Button("Blood Red", ImVec2(85, 0))) {
            light.color = glm::vec3(0.8f, 0.1f, 0.1f);
            light.intensity = 5.0f;
            light.radius = 8.0f;
        }
        ImGui::SameLine();
        if (ImGui::Button("Toxic", ImVec2(85, 0))) {
            light.color = glm::vec3(0.3f, 1.0f, 0.1f);
            light.intensity = 6.0f;
            light.radius = 10.0f;
        }
        ImGui::SameLine();
        if (ImGui::Button("Purple Haze", ImVec2(85, 0))) {
            light.color = glm::vec3(0.6f, 0.2f, 1.0f);
            light.intensity = 7.0f;
            light.radius = 12.0f;
        }

        ImGui::Unindent();
    }

    // Actions
    if (ImGui::CollapsingHeader("Actions")) {
        ImGui::Indent();

        if (ImGui::Button("Focus Camera", ImVec2(-1, 0))) {
            m_Engine->mainCamera.position = light.position + glm::vec3(0, 2, 5);
        }

        if (ImGui::Button("Duplicate Light", ImVec2(-1, 0))) {
            PointLight newLight = light;
            newLight.position += glm::vec3(2, 0, 0);
            m_Engine->scenePointLights.push_back(newLight);
        }

        if (ImGui::Button("Move to Camera", ImVec2(-1, 0))) {
            light.position = m_Engine->mainCamera.position;
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));
        if (ImGui::Button("Delete Light", ImVec2(-1, 0))) {
            m_Engine->scenePointLights.erase(m_Engine->scenePointLights.begin() + index);
            m_Engine->selectedLightIndex = -1;
        }
        ImGui::PopStyleColor();

        ImGui::Unindent();
    }
}

// =============================================================================
// SCENE NODE INSPECTOR
// =============================================================================

void ObjectInspectorView::RenderSceneNodeInspector() {
    auto* node = m_Engine->selectedNode;
    if (!node) return;

    SectionHeader("Scene Node");

    ImGui::Text("Name: %s", m_Engine->selectedObjectName.c_str());

    // Focus button
    ImGui::SameLine(ImGui::GetWindowWidth() - 110);
    if (ImGui::Button("Focus Camera", ImVec2(100, 0))) {
        glm::vec3 pos = glm::vec3(node->worldTransform[3]);
        m_Engine->mainCamera.position = pos + glm::vec3(0, 2, 5);
    }

    ImGui::Spacing();
    ImGui::Separator();

    // Transform
    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();

        // Extract transform components from matrix
        glm::vec3 scale, translation, skew;
        glm::vec4 perspective;
        glm::quat rotation;

        if (glm::decompose(node->worldTransform, scale, rotation, translation, skew, perspective)) {
            glm::vec3 eulerRot = glm::degrees(glm::eulerAngles(rotation));

            RenderTransformEditor(translation, eulerRot, scale);

            // Note: Display only - would need matrix reconstruction for editing
            ImGui::TextDisabled("(Read-only - node transforms are hierarchical)");
        } else {
            ImGui::TextDisabled("Unable to decompose transform matrix");
        }

        ImGui::Unindent();
    }

    // Mesh info
    if (ImGui::CollapsingHeader("Mesh Info")) {
        ImGui::Indent();
        ImGui::Text("Has Mesh: %s", node->mesh ? "Yes" : "No");
        if (node->mesh) {
            ImGui::Text("Surfaces: %zu", node->mesh->surfaces.size());

            // Calculate total triangles
            uint32_t totalIndices = 0;
            for (const auto& surface : node->mesh->surfaces) {
                totalIndices += surface.count;
            }
            ImGui::Text("Total Indices: %u", totalIndices);
            ImGui::Text("Triangles: ~%u", totalIndices / 3);

            // Surface details
            if (ImGui::TreeNode("Surfaces")) {
                for (size_t i = 0; i < node->mesh->surfaces.size(); ++i) {
                    auto& surface = node->mesh->surfaces[i];
                    if (ImGui::TreeNode(("Surface " + std::to_string(i)).c_str())) {
                        ImGui::Text("Start Index: %u", surface.startIndex);
                        ImGui::Text("Index Count: %u", surface.count);
                        ImGui::Text("Triangles: %u", surface.count / 3);
                        ImGui::TreePop();
                    }
                }
                ImGui::TreePop();
            }
        }

        // Children info
        ImGui::Text("Children: %zu", node->children.size());

        ImGui::Unindent();
    }

    // Node hierarchy
    if (!node->children.empty() && ImGui::CollapsingHeader("Children")) {
        ImGui::Indent();
        for (size_t i = 0; i < node->children.size(); ++i) {
            ImGui::BulletText("Child %zu", i);
        }
        ImGui::Unindent();
    }

    // Debug info
    if (ImGui::CollapsingHeader("Debug Info")) {
        ImGui::Indent();

        ImGui::TextDisabled("World Transform Matrix:");
        ImGui::Text("[%.2f, %.2f, %.2f, %.2f]", node->worldTransform[0][0], node->worldTransform[1][0], node->worldTransform[2][0], node->worldTransform[3][0]);
        ImGui::Text("[%.2f, %.2f, %.2f, %.2f]", node->worldTransform[0][1], node->worldTransform[1][1], node->worldTransform[2][1], node->worldTransform[3][1]);
        ImGui::Text("[%.2f, %.2f, %.2f, %.2f]", node->worldTransform[0][2], node->worldTransform[1][2], node->worldTransform[2][2], node->worldTransform[3][2]);
        ImGui::Text("[%.2f, %.2f, %.2f, %.2f]", node->worldTransform[0][3], node->worldTransform[1][3], node->worldTransform[2][3], node->worldTransform[3][3]);

        ImGui::Spacing();
        ImGui::TextDisabled("Memory: %p", static_cast<void*>(node));

        ImGui::Unindent();
    }
}

// =============================================================================
// TRANSFORM EDITOR - Colored XYZ axes
// =============================================================================

void ObjectInspectorView::RenderTransformEditor(glm::vec3& position, glm::vec3& rotation, glm::vec3& scale) {
    // Position with colored XYZ indicators
    ImGui::Text("Position");
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.5f, 0.15f, 0.15f, 1.0f));
    ImGui::SetNextItemWidth(70);
    ImGui::DragFloat("##PosX", &position.x, 0.1f, 0, 0, "X: %.2f");
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.5f, 0.15f, 1.0f));
    ImGui::SetNextItemWidth(70);
    ImGui::DragFloat("##PosY", &position.y, 0.1f, 0, 0, "Y: %.2f");
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.5f, 1.0f));
    ImGui::SetNextItemWidth(70);
    ImGui::DragFloat("##PosZ", &position.z, 0.1f, 0, 0, "Z: %.2f");
    ImGui::PopStyleColor();

    // Rotation
    ImGui::Text("Rotation");
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.5f, 0.15f, 0.15f, 1.0f));
    ImGui::SetNextItemWidth(70);
    ImGui::DragFloat("##RotX", &rotation.x, 1.0f, 0, 0, "X: %.1f");
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.5f, 0.15f, 1.0f));
    ImGui::SetNextItemWidth(70);
    ImGui::DragFloat("##RotY", &rotation.y, 1.0f, 0, 0, "Y: %.1f");
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.5f, 1.0f));
    ImGui::SetNextItemWidth(70);
    ImGui::DragFloat("##RotZ", &rotation.z, 1.0f, 0, 0, "Z: %.1f");
    ImGui::PopStyleColor();

    // Scale
    ImGui::Text("Scale");
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.5f, 0.15f, 0.15f, 1.0f));
    ImGui::SetNextItemWidth(70);
    ImGui::DragFloat("##SclX", &scale.x, 0.01f, 0.01f, 100.0f, "X: %.2f");
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.5f, 0.15f, 1.0f));
    ImGui::SetNextItemWidth(70);
    ImGui::DragFloat("##SclY", &scale.y, 0.01f, 0.01f, 100.0f, "Y: %.2f");
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.5f, 1.0f));
    ImGui::SetNextItemWidth(70);
    ImGui::DragFloat("##SclZ", &scale.z, 0.01f, 0.01f, 100.0f, "Z: %.2f");
    ImGui::PopStyleColor();
}

// =============================================================================
// COLOR EDITORS
// =============================================================================

void ObjectInspectorView::RenderColorEditor(const char* label, glm::vec3& color) {
    float col[3] = { color.x, color.y, color.z };
    if (ImGui::ColorEdit3(label, col, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR)) {
        color = glm::vec3(col[0], col[1], col[2]);
    }
}

void ObjectInspectorView::RenderColorEditor4(const char* label, glm::vec4& color) {
    float col[4] = { color.x, color.y, color.z, color.w };
    if (ImGui::ColorEdit4(label, col, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_AlphaBar)) {
        color = glm::vec4(col[0], col[1], col[2], col[3]);
    }
}

} // namespace Yalaz::UI
