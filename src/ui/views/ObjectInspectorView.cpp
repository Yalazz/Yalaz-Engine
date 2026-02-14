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
#include "../../vk_loader.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/quaternion.hpp>
#include <algorithm>
#include <functional>
#include <filesystem>
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

static void CollectEditableMeshNodes(Node* node, std::vector<MeshNode*>& outNodes) {
    if (!node) return;

    if (auto* meshNode = dynamic_cast<MeshNode*>(node)) {
        if (meshNode->mesh && !meshNode->mesh->surfaces.empty()) {
            outNodes.push_back(meshNode);
        }
    }

    for (const auto& child : node->children) {
        if (child) {
            CollectEditableMeshNodes(child.get(), outNodes);
        }
    }
}

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
        FocusCameraOnPosition(shape.position);
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
    // MATERIAL SECTION - Full PBR Properties
    // ==========================================================================
    if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();

        // Main color (Base Color / Albedo)
        ImGui::TextDisabled("Base Color (Albedo)");
        RenderColorEditor4("##MainColor", shape.mainColor);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // === PBR PROPERTIES ===
        ImGui::TextDisabled("PBR Properties");

        // Metallic with percentage display
        ImGui::Text("Metallic");
        ImGui::SameLine(ImGui::GetWindowWidth() - 60);
        ImGui::TextDisabled("%.0f%%", shape.metallic * 100);
        ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.8f, 0.7f, 0.3f, 1.0f));
        ImGui::SliderFloat("##Metallic", &shape.metallic, 0.0f, 1.0f, "");
        ImGui::PopStyleColor();

        // Roughness with percentage display
        ImGui::Text("Roughness");
        ImGui::SameLine(ImGui::GetWindowWidth() - 60);
        ImGui::TextDisabled("%.0f%%", shape.roughness * 100);
        ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
        ImGui::SliderFloat("##Roughness", &shape.roughness, 0.0f, 1.0f, "");
        ImGui::PopStyleColor();

        // Reflection Intensity (cubemap)
        ImGui::Spacing();
        ImGui::Text("Reflection");
        ImGui::SameLine(ImGui::GetWindowWidth() - 60);
        ImGui::TextDisabled("%.0f%%", shape.reflectionIntensity * 100);
        ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.3f, 0.6f, 0.9f, 1.0f));
        ImGui::SliderFloat("##Reflection", &shape.reflectionIntensity, 0.0f, 1.0f, "");
        ImGui::PopStyleColor();

        // Quick PBR presets
        ImGui::Spacing();
        ImGui::TextDisabled("Quick:");
        if (ImGui::Button("Shiny", ImVec2(50, 0))) { shape.roughness = 0.1f; }
        ImGui::SameLine();
        if (ImGui::Button("Matte", ImVec2(50, 0))) { shape.roughness = 0.8f; }
        ImGui::SameLine();
        if (ImGui::Button("Metal", ImVec2(50, 0))) { shape.metallic = 1.0f; shape.roughness = 0.3f; }
        ImGui::SameLine();
        if (ImGui::Button("Plastic", ImVec2(60, 0))) { shape.metallic = 0.0f; shape.roughness = 0.4f; }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // === EMISSION ===
        ImGui::TextDisabled("Emission");

        // Calculate emission strength from length
        float emissionStrength = glm::length(shape.emission);
        glm::vec3 emissionColor = emissionStrength > 0.001f ? shape.emission / emissionStrength : glm::vec3(1.0f);

        float emCol[3] = { emissionColor.r, emissionColor.g, emissionColor.b };
        if (ImGui::ColorEdit3("##EmissionColor", emCol, ImGuiColorEditFlags_Float)) {
            emissionColor = glm::vec3(emCol[0], emCol[1], emCol[2]);
            shape.emission = emissionColor * emissionStrength;
        }

        if (ImGui::SliderFloat("Strength##Emission", &emissionStrength, 0.0f, 10.0f, "%.2f")) {
            shape.emission = emissionColor * emissionStrength;
        }

        // Emission presets
        if (ImGui::Button("Off##Emission", ImVec2(40, 0))) { shape.emission = glm::vec3(0.0f); }
        ImGui::SameLine();
        if (ImGui::Button("Glow", ImVec2(40, 0))) { shape.emission = glm::vec3(1.0f, 0.5f, 0.2f) * 2.0f; }
        ImGui::SameLine();
        if (ImGui::Button("Neon", ImVec2(40, 0))) { shape.emission = glm::vec3(0.2f, 1.0f, 0.5f) * 3.0f; }
        ImGui::SameLine();
        if (ImGui::Button("Hot", ImVec2(40, 0))) { shape.emission = glm::vec3(1.0f, 0.2f, 0.1f) * 5.0f; }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // === MATERIAL PRESETS ===
        ImGui::TextDisabled("Material Presets");

        // Metals
        if (ImGui::Button("Gold", ImVec2(55, 0))) {
            shape.mainColor = glm::vec4(1.0f, 0.766f, 0.336f, 1.0f);
            shape.metallic = 1.0f; shape.roughness = 0.3f;
        }
        ImGui::SameLine();
        if (ImGui::Button("Silver", ImVec2(55, 0))) {
            shape.mainColor = glm::vec4(0.972f, 0.960f, 0.915f, 1.0f);
            shape.metallic = 1.0f; shape.roughness = 0.2f;
        }
        ImGui::SameLine();
        if (ImGui::Button("Copper", ImVec2(55, 0))) {
            shape.mainColor = glm::vec4(0.955f, 0.637f, 0.538f, 1.0f);
            shape.metallic = 1.0f; shape.roughness = 0.25f;
        }
        ImGui::SameLine();
        if (ImGui::Button("Chrome", ImVec2(55, 0))) {
            shape.mainColor = glm::vec4(0.549f, 0.556f, 0.554f, 1.0f);
            shape.metallic = 1.0f; shape.roughness = 0.1f;
        }

        // Dielectrics
        if (ImGui::Button("Plastic R", ImVec2(55, 0))) {
            shape.mainColor = glm::vec4(0.9f, 0.1f, 0.1f, 1.0f);
            shape.metallic = 0.0f; shape.roughness = 0.4f;
        }
        ImGui::SameLine();
        if (ImGui::Button("Wood", ImVec2(55, 0))) {
            shape.mainColor = glm::vec4(0.6f, 0.4f, 0.2f, 1.0f);
            shape.metallic = 0.0f; shape.roughness = 0.7f;
        }
        ImGui::SameLine();
        if (ImGui::Button("Rubber", ImVec2(55, 0))) {
            shape.mainColor = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f);
            shape.metallic = 0.0f; shape.roughness = 0.9f;
        }
        ImGui::SameLine();
        if (ImGui::Button("Glass", ImVec2(55, 0))) {
            shape.mainColor = glm::vec4(0.95f, 0.95f, 0.95f, 0.3f);
            shape.metallic = 0.0f; shape.roughness = 0.05f;
            shape.passType = MaterialPass::Transparent;
        }

        // Reflective materials
        if (ImGui::Button("Mirror", ImVec2(55, 0))) {
            shape.mainColor = glm::vec4(0.95f, 0.95f, 0.95f, 1.0f);
            shape.metallic = 1.0f; shape.roughness = 0.05f;
            shape.reflectionIntensity = 1.0f;
        }
        ImGui::SameLine();
        if (ImGui::Button("Reflect", ImVec2(55, 0))) {
            shape.reflectionIntensity = 0.5f;
        }
        ImGui::SameLine();
        if (ImGui::Button("No Refl", ImVec2(55, 0))) {
            shape.reflectionIntensity = 0.0f;
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Material type selector (advanced)
        const char* materialNames[] = { "Default", "Unlit", "PBR", "Normal Debug", "Wireframe" };
        int matType = static_cast<int>(shape.materialType);
        if (ImGui::Combo("Shader Type", &matType, materialNames, 5)) {
            shape.materialType = static_cast<ShaderOnlyMaterial>(matType);
        }

        // Pass type selector
        const char* passNames[] = { "Opaque", "Transparent", "Other" };
        int passType = static_cast<int>(shape.passType);
        if (ImGui::Combo("Render Pass", &passType, passNames, 3)) {
            shape.passType = static_cast<MaterialPass>(passType);
        }

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
    // TEXTURES SECTION - Drag-and-drop texture assignment per primitive
    // ==========================================================================
    if (ImGui::CollapsingHeader("Textures")) {
        ImGui::Indent();

        ImGui::TextDisabled("Drag textures from Asset Browser onto each slot");
        ImGui::Spacing();

        bool texturesChanged = false;

        // Drag-and-drop texture slot lambda
        auto renderTextureDropSlot = [&](const char* label, std::string& texPath) -> bool {
            bool changed = false;
            ImGui::PushID(label);

            ImGui::TextDisabled("%s", label);

            // Determine button color based on whether texture is assigned
            ImVec4 slotColor = !texPath.empty() ?
                ImVec4(0.2f, 0.4f, 0.3f, 1.0f) :
                ImVec4(0.2f, 0.2f, 0.25f, 1.0f);

            ImGui::PushStyleColor(ImGuiCol_Button, slotColor);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                ImVec4(slotColor.x + 0.1f, slotColor.y + 0.1f, slotColor.z + 0.1f, 1.0f));

            // Show filename or placeholder
            std::string buttonLabel;
            if (!texPath.empty()) {
                size_t lastSlash = texPath.find_last_of("/\\");
                buttonLabel = (lastSlash != std::string::npos) ? texPath.substr(lastSlash + 1) : texPath;
            } else {
                buttonLabel = "[Drop Texture Here]";
            }

            ImGui::Button(buttonLabel.c_str(), ImVec2(-30, 35));

            // Drag-drop target
            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("TEXTURE_PATH")) {
                    const char* droppedPath = static_cast<const char*>(payload->Data);
                    texPath = droppedPath;
                    changed = true;
                }
                ImGui::EndDragDropTarget();
            }

            // Tooltip
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                if (!texPath.empty()) {
                    ImGui::Text("%s", texPath.c_str());
                    ImGui::TextDisabled("Click X to remove");
                } else {
                    ImGui::Text("Drop a texture from Asset Browser");
                }
                ImGui::EndTooltip();
            }

            ImGui::PopStyleColor(2);

            // Clear button
            ImGui::SameLine();
            if (ImGui::Button("X", ImVec2(25, 35)) && !texPath.empty()) {
                texPath.clear();
                changed = true;
            }

            ImGui::PopID();
            ImGui::Spacing();
            return changed;
        };

        // Render the three texture slots
        if (renderTextureDropSlot("Albedo (Base Color)", shape.albedoTexturePath)) texturesChanged = true;
        if (renderTextureDropSlot("Metallic/Roughness", shape.metalRoughTexturePath)) texturesChanged = true;
        if (renderTextureDropSlot("Emission", shape.emissionTexturePath)) texturesChanged = true;
        if (renderTextureDropSlot("Displacement (Height)", shape.displacementTexturePath)) texturesChanged = true;
        if (ImGui::DragFloat("Displacement Scale", &shape.displacementScale, 0.005f, -1.0f, 1.0f, "%.3f")) texturesChanged = true;
        if (ImGui::DragFloat("Displacement Bias", &shape.displacementBias, 0.005f, -1.0f, 1.0f, "%.3f")) texturesChanged = true;

        // Auto-rebuild material when textures change
        if (texturesChanged) {
            MaterialInstance newMaterial = m_Engine->create_primitive_material(
                shape.albedoTexturePath, shape.metalRoughTexturePath, shape.emissionTexturePath,
                shape.displacementTexturePath, shape.displacementScale, shape.displacementBias);
            shape.material = std::make_shared<MaterialInstance>(newMaterial);
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Clear all textures button
        bool hasTextures = !shape.albedoTexturePath.empty() ||
                          !shape.metalRoughTexturePath.empty() ||
                          !shape.emissionTexturePath.empty() ||
                          !shape.displacementTexturePath.empty();
        if (hasTextures) {
            if (ImGui::Button("Clear All Textures", ImVec2(-1, 0))) {
                shape.material = nullptr;  // Reverts to default material
                shape.albedoTexturePath.clear();
                shape.metalRoughTexturePath.clear();
                shape.emissionTexturePath.clear();
                shape.displacementTexturePath.clear();
                shape.displacementScale = 0.0f;
                shape.displacementBias = 0.0f;
            }
        }

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
        if (ImGui::Button("Focus Camera##actions", ImVec2(-1, 0))) {
            FocusCameraOnPosition(shape.position);
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
    if (ImGui::Button("Focus Camera##lightHeader", ImVec2(100, 0))) {
        FocusCameraOnPosition(light.position);
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

        if (ImGui::Button("Focus Camera##lightActions", ImVec2(-1, 0))) {
            FocusCameraOnPosition(light.position);
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

    // Validate node still exists in a loaded scene (prevent dangling pointer access)
    bool nodeValid = false;
    for (const auto& [sceneName, scene] : m_Engine->loadedScenes) {
        if (!scene) continue;
        for (const auto& [name, n] : scene->nodes) {
            if (n && n.get() == node) {
                nodeValid = true;
                break;
            }
        }
        if (nodeValid) break;
    }
    if (!nodeValid) {
        m_Engine->selectedNode = nullptr;
        m_Engine->selectedObjectName.clear();
        return;
    }

    if (m_LastInspectedNode != node) {
        m_LastInspectedNode = node;
        m_SelectedSkeletonIndex = -1;
        m_SelectedBoneIndex = -1;
        m_SelectedMaterialMeshIndex = 0;
    }

    SectionHeader("Scene Node");

    ImGui::Text("Name: %s", m_Engine->selectedObjectName.c_str());

    LoadedGLTF* ownerScene = nullptr;
    std::string ownerSceneName;
    int selectedNodeIndex = -1;
    for (auto& [sceneName, scene] : m_Engine->loadedScenes) {
        if (!scene) continue;
        for (int ni = 0; ni < static_cast<int>(scene->indexedNodes.size()); ++ni) {
            if (scene->indexedNodes[ni] && scene->indexedNodes[ni].get() == node) {
                ownerScene = scene.get();
                ownerSceneName = sceneName;
                selectedNodeIndex = ni;
                break;
            }
        }
        if (ownerScene) break;
    }

    // Show scene name and source file path
    if (!ownerSceneName.empty()) {
        ImGui::TextDisabled("Scene: %s", ownerSceneName.c_str());
        auto pathIt = m_Engine->sceneFilePaths.find(ownerSceneName);
        if (pathIt != m_Engine->sceneFilePaths.end()) {
            // Show file type badge
            std::string ext = std::filesystem::path(pathIt->second).extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::toupper);
            if (!ext.empty() && ext[0] == '.') ext = ext.substr(1);
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "[%s]", ext.c_str());
            ImGui::TextDisabled("Source: %s", pathIt->second.c_str());
        }
    }

    // Focus button
    ImGui::SameLine(ImGui::GetWindowWidth() - 110);
    if (ImGui::Button("Focus Camera##nodeHeader", ImVec2(100, 0))) {
        glm::vec3 pos = glm::vec3(node->worldTransform[3]);
        FocusCameraOnPosition(pos);
    }

    ImGui::Spacing();
    ImGui::Separator();

    auto refreshNodeWorldFromLocal = [&]() {
        glm::mat4 parentMatrix = glm::mat4(1.0f);
        if (auto parent = node->parent.lock()) {
            parentMatrix = parent->worldTransform;
        }
        node->refreshTransform(parentMatrix);
    };

    auto applyLocalTRS = [&](const glm::vec3& translation, const glm::vec3& eulerDeg, const glm::vec3& scale) {
        glm::mat4 translationMat = glm::translate(glm::mat4(1.0f), translation);
        glm::mat4 rotationMat = glm::mat4(glm::quat(glm::radians(eulerDeg)));
        glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), scale);
        glm::mat4 newLocal = translationMat * rotationMat * scaleMat;

        if (m_ApplyTransformToWholeAsset && ownerScene && !ownerScene->topNodes.empty()) {
            glm::mat4 delta = newLocal * glm::inverse(node->localTransform);
            for (auto& top : ownerScene->topNodes) {
                if (!top) continue;
                top->localTransform = delta * top->localTransform;
            }
            for (auto& top : ownerScene->topNodes) {
                if (top) top->refreshTransform(glm::mat4(1.0f));
            }
        } else {
            node->localTransform = newLocal;
            refreshNodeWorldFromLocal();
        }
    };

    // Transform - NOW EDITABLE
    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();

        if (ownerScene) {
            ImGui::Checkbox("Apply To Whole Asset", &m_ApplyTransformToWholeAsset);
            ImGui::TextDisabled("Edits affect all nodes in this imported GLTF scene.");
            ImGui::Spacing();
        }

        // Extract transform components from matrix
        glm::vec3 scale, translation, skew;
        glm::vec4 perspective;
        glm::quat rotation;

        if (glm::decompose(node->localTransform, scale, rotation, translation, skew, perspective)) {
            glm::vec3 eulerRot = glm::degrees(glm::eulerAngles(rotation));

            // Store original values to detect changes
            glm::vec3 origTranslation = translation;
            glm::vec3 origRotation = eulerRot;
            glm::vec3 origScale = scale;

            RenderTransformEditor(translation, eulerRot, scale);

            // Apply changes back to worldTransform if any value changed
            bool changed = (translation != origTranslation ||
                           eulerRot != origRotation ||
                           scale != origScale);

            if (changed) {
                applyLocalTRS(translation, eulerRot, scale);
            }

            // Quick transform buttons
            ImGui::Spacing();
            if (ImGui::Button("Reset Position")) {
                translation = glm::vec3(0.0f);
                applyLocalTRS(translation, eulerRot, scale);
            }
            ImGui::SameLine();
            if (ImGui::Button("Reset Rotation")) {
                eulerRot = glm::vec3(0.0f);
                applyLocalTRS(translation, eulerRot, scale);
            }
            ImGui::SameLine();
            if (ImGui::Button("Reset Scale")) {
                scale = glm::vec3(1.0f);
                applyLocalTRS(translation, eulerRot, scale);
            }

            // Snap buttons
            ImGui::Spacing();
            ImGui::TextDisabled("Snap:");
            ImGui::SameLine();
            if (ImGui::Button("Grid 1##node")) {
                translation = glm::round(translation);
                applyLocalTRS(translation, eulerRot, scale);
            }
            ImGui::SameLine();
            if (ImGui::Button("Grid 0.5##node")) {
                translation = glm::round(translation * 2.0f) / 2.0f;
                applyLocalTRS(translation, eulerRot, scale);
            }

        } else {
            ImGui::TextDisabled("Unable to decompose transform matrix");
        }

        ImGui::Unindent();
    }

    // Material editing for imported scene nodes.
    // If selected node is a root/group node (common in FBX/DAE), expose child mesh materials too.
    MeshNode* meshNode = dynamic_cast<MeshNode*>(node);
    std::vector<MeshNode*> editableMeshNodes;
    CollectEditableMeshNodes(node, editableMeshNodes);

    if (!editableMeshNodes.empty() && ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();

        if (m_SelectedMaterialMeshIndex < 0 ||
            m_SelectedMaterialMeshIndex >= static_cast<int>(editableMeshNodes.size())) {
            m_SelectedMaterialMeshIndex = 0;
        }

        if (editableMeshNodes.size() > 1) {
            ImGui::Text("Mesh: %zu", editableMeshNodes.size());
            ImGui::SetNextItemWidth(-1);
            if (ImGui::BeginCombo("##MeshMaterialSelect",
                ("Mesh " + std::to_string(m_SelectedMaterialMeshIndex)).c_str())) {
                for (int i = 0; i < static_cast<int>(editableMeshNodes.size()); ++i) {
                    const MeshNode* mn = editableMeshNodes[i];
                    std::string meshLabel = "Mesh " + std::to_string(i);
                    if (mn && mn->mesh && !mn->mesh->name.empty()) {
                        meshLabel += " - " + mn->mesh->name;
                    }
                    bool isSelected = (i == m_SelectedMaterialMeshIndex);
                    if (ImGui::Selectable(meshLabel.c_str(), isSelected)) {
                        m_SelectedMaterialMeshIndex = i;
                    }
                    if (isSelected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
            ImGui::Spacing();
        }

        RenderGLTFMaterialEditor(editableMeshNodes[m_SelectedMaterialMeshIndex]);
        ImGui::Unindent();
    }

    // GLTF scene data panels (camera/animation) for this node
    {
        if (ownerScene) {
            // Camera panel
            if (!ownerScene->cameras.empty()) {
                std::vector<int> nodeCameraIndices;
                nodeCameraIndices.reserve(ownerScene->cameras.size());
                for (int camIdx = 0; camIdx < static_cast<int>(ownerScene->cameras.size()); ++camIdx) {
                    if (ownerScene->cameras[camIdx].sourceNode == node) {
                        nodeCameraIndices.push_back(camIdx);
                    }
                }

                if (!nodeCameraIndices.empty() && ImGui::CollapsingHeader("GLTF Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::Indent();

                    for (int camIdx : nodeCameraIndices) {
                        GLTFCamera& cam = ownerScene->cameras[camIdx];
                        ImGui::PushID(camIdx);
                        ImGui::SeparatorText(cam.name.c_str());

                        bool isActive = m_Engine->useGLTFCamera &&
                            m_Engine->currentGLTFCameraScene == ownerSceneName &&
                            m_Engine->currentGLTFCameraIndex == camIdx;
                        ImGui::Text("Active: %s", isActive ? "Yes" : "No");
                        ImGui::Text("Type: %s", cam.isPerspective ? "Perspective" : "Orthographic");

                        bool cameraChanged = false;
                        if (cam.isPerspective) {
                            cameraChanged |= ImGui::DragFloat("FOV", &cam.fov, 0.1f, 1.0f, 179.0f, "%.1f deg");
                        } else {
                            cameraChanged |= ImGui::DragFloat("Ortho Width", &cam.orthoWidth, 0.05f, 0.01f, 10000.0f, "%.2f");
                            cameraChanged |= ImGui::DragFloat("Ortho Height", &cam.orthoHeight, 0.05f, 0.01f, 10000.0f, "%.2f");
                        }
                        cameraChanged |= ImGui::DragFloat("Near", &cam.nearPlane, 0.001f, 0.001f, cam.farPlane - 0.001f, "%.4f");
                        cameraChanged |= ImGui::DragFloat("Far", &cam.farPlane, 0.1f, cam.nearPlane + 0.001f, 1000000.0f, "%.2f");
                        cam.fov = glm::clamp(cam.fov, 1.0f, 179.0f);
                        cam.nearPlane = glm::max(cam.nearPlane, 0.001f);
                        cam.farPlane = glm::max(cam.farPlane, cam.nearPlane + 0.001f);
                        cam.orthoWidth = glm::max(cam.orthoWidth, 0.01f);
                        cam.orthoHeight = glm::max(cam.orthoHeight, 0.01f);

                        if (cameraChanged && isActive) {
                            m_Engine->applyGLTFCamera(ownerSceneName, camIdx);
                        }

                        if (ImGui::Button("Use This Camera", ImVec2(-1, 0))) {
                            m_Engine->applyGLTFCamera(ownerSceneName, camIdx);
                        }
                        if (isActive && ImGui::Button("Back To Free Camera", ImVec2(-1, 0))) {
                            m_Engine->resetToFreeCamera();
                        }

                        ImGui::PopID();
                    }

                    ImGui::Unindent();
                }
            }

            // Light panel
            if (!ownerScene->lights.empty()) {
                std::vector<GLTFLight*> nodeLights;
                for (auto& l : ownerScene->lights) {
                    if (l.sourceNode == node) nodeLights.push_back(&l);
                }

                if (!nodeLights.empty() && ImGui::CollapsingHeader("GLTF Light", ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::Indent();
                    for (size_t li = 0; li < nodeLights.size(); ++li) {
                        GLTFLight* l = nodeLights[li];
                        ImGui::PushID(static_cast<int>(li));
                        ImGui::SeparatorText(l->name.c_str());

                        static const char* kTypes[] = { "Directional", "Point", "Spot" };
                        int type = std::clamp(l->type, 0, 2);
                        ImGui::Combo("Type", &type, kTypes, IM_ARRAYSIZE(kTypes));
                        l->type = type;

                        bool changed = false;
                        changed |= ImGui::DragFloat3("Position", &l->position.x, 0.01f);
                        changed |= ImGui::ColorEdit3("Color", &l->color.x, ImGuiColorEditFlags_Float);
                        changed |= ImGui::DragFloat("Intensity", &l->intensity, 0.05f, 0.0f, 1000.0f, "%.2f");
                        changed |= ImGui::DragFloat("Range", &l->range, 0.1f, 0.01f, 100000.0f, "%.2f");

                        if (l->runtimePointLightIndex >= 0 &&
                            l->runtimePointLightIndex < static_cast<int>(m_Engine->scenePointLights.size())) {
                            PointLight& pl = m_Engine->scenePointLights[l->runtimePointLightIndex];
                            if (changed) {
                                pl.position = l->position;
                                pl.color = l->color;
                                pl.intensity = l->intensity;
                                pl.radius = l->range;
                            }
                        } else if (l->type == 0 && changed) {
                            glm::vec3 dir = glm::normalize(l->direction);
                            if (glm::length(dir) < 0.0001f) dir = glm::vec3(0.0f, -1.0f, 0.0f);
                            m_Engine->sceneData.sunlightDirection = glm::vec4(-dir, l->intensity);
                            m_Engine->sceneData.sunlightColor = glm::vec4(l->color, 1.0f);
                        }

                        ImGui::PopID();
                    }
                    ImGui::Unindent();
                }
            }

            // Animation panel for selected node
            if (selectedNodeIndex >= 0) {
                std::vector<int> nodeClipIndices;
                for (int ci = 0; ci < static_cast<int>(m_Engine->animationClips.size()); ++ci) {
                    const auto& clip = m_Engine->animationClips[ci];
                    bool affectsNode = false;
                    for (const auto& track : clip.tracks) {
                        if (track.targetNodeIndex == selectedNodeIndex || track.targetNode == m_Engine->selectedObjectName) {
                            affectsNode = true;
                            break;
                        }
                    }
                    if (affectsNode) nodeClipIndices.push_back(ci);
                }

                if (!nodeClipIndices.empty() && ImGui::CollapsingHeader("Animation", ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::Indent();

                    int activeClip = m_Engine->activeAnimationIndex;
                    if (std::find(nodeClipIndices.begin(), nodeClipIndices.end(), activeClip) == nodeClipIndices.end()) {
                        activeClip = nodeClipIndices.front();
                        m_Engine->activeAnimationIndex = activeClip;
                    }

                    const char* currentName = m_Engine->animationClips[activeClip].name.c_str();
                    ImGui::SetNextItemWidth(-1);
                    if (ImGui::BeginCombo("Clip", currentName)) {
                        for (int ci : nodeClipIndices) {
                            bool selected = (ci == activeClip);
                            if (ImGui::Selectable(m_Engine->animationClips[ci].name.c_str(), selected)) {
                                m_Engine->activeAnimationIndex = ci;
                                if (m_Engine->animationClips[ci].skeletonIndex >= 0) {
                                    m_Engine->activeSkeletonIndex = m_Engine->animationClips[ci].skeletonIndex;
                                }
                                activeClip = ci;
                            }
                            if (selected) ImGui::SetItemDefaultFocus();
                        }
                        ImGui::EndCombo();
                    }

                    auto& clip = m_Engine->animationClips[activeClip];
                    if (ImGui::Button("Play", ImVec2(56, 0))) m_Engine->playAnimation(activeClip);
                    ImGui::SameLine();
                    if (ImGui::Button("Pause", ImVec2(56, 0))) clip.isPlaying = false;
                    ImGui::SameLine();
                    if (ImGui::Button("Stop", ImVec2(56, 0))) m_Engine->stopAnimation(activeClip);

                    if (clip.duration > 0.0f) {
                        float t = clip.currentTime;
                        if (ImGui::SliderFloat("Time", &t, 0.0f, clip.duration, "%.3fs")) {
                            clip.currentTime = t;
                            m_Engine->updateAnimations(0.0f);
                        }
                    }
                    ImGui::Checkbox("Loop", &clip.loop);
                    ImGui::SameLine();
                    ImGui::Checkbox("PingPong", &clip.pingPong);
                    ImGui::SliderFloat("Speed", &clip.speed, 0.1f, 3.0f, "%.2fx");

                    ImGui::Text("Tracks: %d", static_cast<int>(clip.tracks.size()));
                    if (clip.skeletonIndex >= 0 && clip.skeletonIndex < static_cast<int>(m_Engine->skeletons.size())) {
                        const auto& skel = m_Engine->skeletons[clip.skeletonIndex];
                        ImGui::Text("Skeleton: %s (%d bones)", skel.name.c_str(), static_cast<int>(skel.bones.size()));
                    }

                    ImGui::Unindent();
                }
            }

            if (selectedNodeIndex >= 0 && !m_Engine->skeletons.empty() &&
                ImGui::CollapsingHeader("Skeleton / Bones", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Indent();

                std::vector<int> candidateSkeletons;
                candidateSkeletons.reserve(m_Engine->skeletons.size());
                for (int si = 0; si < static_cast<int>(m_Engine->skeletons.size()); ++si) {
                    const auto& skel = m_Engine->skeletons[si];
                    bool referencesNode = false;
                    for (const auto& bone : skel.bones) {
                        if (bone.nodeIndex == selectedNodeIndex) {
                            referencesNode = true;
                            break;
                        }
                    }
                    if (referencesNode) {
                        candidateSkeletons.push_back(si);
                    }
                }

                if (candidateSkeletons.empty() && m_Engine->activeSkeletonIndex >= 0 &&
                    m_Engine->activeSkeletonIndex < static_cast<int>(m_Engine->skeletons.size())) {
                    candidateSkeletons.push_back(m_Engine->activeSkeletonIndex);
                }

                if (candidateSkeletons.empty()) {
                    ImGui::TextDisabled("No skeleton mapped to this node.");
                } else {
                    auto skeletonInCandidates = [&candidateSkeletons](int skeletonIndex) {
                        return std::find(candidateSkeletons.begin(), candidateSkeletons.end(), skeletonIndex) != candidateSkeletons.end();
                    };

                    if (!skeletonInCandidates(m_SelectedSkeletonIndex)) {
                        if (skeletonInCandidates(m_Engine->activeSkeletonIndex)) {
                            m_SelectedSkeletonIndex = m_Engine->activeSkeletonIndex;
                        } else {
                            m_SelectedSkeletonIndex = candidateSkeletons.front();
                        }
                        m_SelectedBoneIndex = -1;
                    }

                    const auto& activeSkeleton = m_Engine->skeletons[m_SelectedSkeletonIndex];
                    ImGui::Text("Bones: %d", static_cast<int>(activeSkeleton.bones.size()));

                    std::string selectedSkeletonName = activeSkeleton.name.empty() ?
                        ("Skeleton " + std::to_string(m_SelectedSkeletonIndex)) : activeSkeleton.name;
                    if (ImGui::BeginCombo("Skeleton", selectedSkeletonName.c_str())) {
                        for (int si : candidateSkeletons) {
                            const auto& sk = m_Engine->skeletons[si];
                            std::string label = sk.name.empty() ? ("Skeleton " + std::to_string(si)) : sk.name;
                            bool selected = (si == m_SelectedSkeletonIndex);
                            if (ImGui::Selectable(label.c_str(), selected)) {
                                m_SelectedSkeletonIndex = si;
                                m_SelectedBoneIndex = -1;
                            }
                            if (selected) ImGui::SetItemDefaultFocus();
                        }
                        ImGui::EndCombo();
                    }

                    std::vector<std::vector<int>> boneChildren(activeSkeleton.bones.size());
                    for (int bi = 0; bi < static_cast<int>(activeSkeleton.bones.size()); ++bi) {
                        int parentIndex = activeSkeleton.bones[bi].parentIndex;
                        if (parentIndex >= 0 && parentIndex < static_cast<int>(activeSkeleton.bones.size())) {
                            boneChildren[parentIndex].push_back(bi);
                        }
                    }

                    std::function<void(int)> renderBoneTree = [&](int boneIndex) {
                        const auto& bone = activeSkeleton.bones[boneIndex];
                        bool hasChildren = !boneChildren[boneIndex].empty();
                        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth;
                        if (!hasChildren) flags |= ImGuiTreeNodeFlags_Leaf;
                        if (m_SelectedBoneIndex == boneIndex) flags |= ImGuiTreeNodeFlags_Selected;

                        std::string boneLabel = bone.name.empty() ?
                            ("Bone " + std::to_string(boneIndex)) : bone.name;
                        bool open = ImGui::TreeNodeEx((boneLabel + "##bone_" + std::to_string(boneIndex)).c_str(), flags);
                        if (ImGui::IsItemClicked()) {
                            m_SelectedBoneIndex = boneIndex;
                        }
                        if (open) {
                            for (int childIndex : boneChildren[boneIndex]) {
                                renderBoneTree(childIndex);
                            }
                            ImGui::TreePop();
                        }
                    };

                    if (ImGui::TreeNode("Bone Hierarchy")) {
                        for (int bi = 0; bi < static_cast<int>(activeSkeleton.bones.size()); ++bi) {
                            if (activeSkeleton.bones[bi].parentIndex < 0) {
                                renderBoneTree(bi);
                            }
                        }
                        ImGui::TreePop();
                    }

                    if (m_SelectedBoneIndex >= 0 && m_SelectedBoneIndex < static_cast<int>(activeSkeleton.bones.size())) {
                        const auto& selectedBone = activeSkeleton.bones[m_SelectedBoneIndex];
                        ImGui::Separator();
                        ImGui::Text("Selected Bone: %s",
                            selectedBone.name.empty() ? "(unnamed)" : selectedBone.name.c_str());
                        ImGui::Text("Bone Index: %d  Node Index: %d", m_SelectedBoneIndex, selectedBone.nodeIndex);

                        Node* boneNode = nullptr;
                        if (selectedBone.nodeIndex >= 0 &&
                            selectedBone.nodeIndex < static_cast<int>(ownerScene->indexedNodes.size()) &&
                            ownerScene->indexedNodes[selectedBone.nodeIndex]) {
                            boneNode = ownerScene->indexedNodes[selectedBone.nodeIndex].get();
                        }

                        if (!boneNode) {
                            ImGui::TextDisabled("Bone node not available in scene.");
                        } else {
                            glm::vec3 scale, translation, skew;
                            glm::vec4 perspective;
                            glm::quat rotation;
                            if (glm::decompose(boneNode->localTransform, scale, rotation, translation, skew, perspective)) {
                                glm::vec3 eulerRot = glm::degrees(glm::eulerAngles(rotation));
                                glm::vec3 oldTranslation = translation;
                                glm::vec3 oldEuler = eulerRot;
                                glm::vec3 oldScale = scale;

                                ImGui::DragFloat3("Bone Position", &translation.x, 0.01f);
                                ImGui::DragFloat3("Bone Rotation", &eulerRot.x, 0.25f);
                                ImGui::DragFloat3("Bone Scale", &scale.x, 0.01f, 0.001f, 1000.0f);

                                bool changed = (translation != oldTranslation) || (eulerRot != oldEuler) || (scale != oldScale);
                                if (changed) {
                                    glm::mat4 t = glm::translate(glm::mat4(1.0f), translation);
                                    glm::mat4 r = glm::mat4(glm::quat(glm::radians(eulerRot)));
                                    glm::mat4 s = glm::scale(glm::mat4(1.0f), scale);
                                    boneNode->localTransform = t * r * s;
                                    for (auto& top : ownerScene->topNodes) {
                                        if (top) top->refreshTransform(glm::mat4(1.0f));
                                    }
                                    m_Engine->updateAnimations(0.0f);
                                }
                            } else {
                                ImGui::TextDisabled("Unable to decompose selected bone transform.");
                            }
                        }
                    }
                }

                ImGui::Unindent();
            }
        }
    }

    // Mesh info
    if (ImGui::CollapsingHeader("Mesh Info")) {
        ImGui::Indent();
        ImGui::Text("Has Mesh: %s", meshNode && meshNode->mesh ? "Yes" : "No");
        if (meshNode && meshNode->mesh) {
            ImGui::Text("Surfaces: %zu", meshNode->mesh->surfaces.size());

            // Calculate total triangles
            uint32_t totalIndices = 0;
            for (const auto& surface : meshNode->mesh->surfaces) {
                totalIndices += surface.count;
            }
            ImGui::Text("Total Indices: %u", totalIndices);
            ImGui::Text("Triangles: ~%u", totalIndices / 3);

            // Surface details
            if (ImGui::TreeNode("Surfaces")) {
                for (size_t i = 0; i < meshNode->mesh->surfaces.size(); ++i) {
                    auto& surface = meshNode->mesh->surfaces[i];
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

        if (meshNode && meshNode->mesh && ImGui::TreeNode("Skinning Debug")) {
            ImGui::Text("Skinned Mesh: %s", meshNode->mesh->hasSkinData ? "Yes" : "No");
            ImGui::Text("Skin Buffer Address: 0x%llX", static_cast<unsigned long long>(meshNode->mesh->skinBufferAddress));
            ImGui::Text("Bone Buffer Address: 0x%llX", static_cast<unsigned long long>(m_Engine->skinningMatrixBufferAddress));
            ImGui::Text("Active Clip: %d", m_Engine->activeAnimationIndex);
            if (m_Engine->activeAnimationIndex >= 0 &&
                m_Engine->activeAnimationIndex < static_cast<int>(m_Engine->animationClips.size())) {
                const auto& clip = m_Engine->animationClips[m_Engine->activeAnimationIndex];
                ImGui::Text("Clip Name: %s", clip.name.c_str());
                ImGui::Text("Clip Playing: %s", clip.isPlaying ? "Yes" : "No");
            }
            ImGui::Text("Active Skeleton: %d", m_Engine->activeSkeletonIndex);
            if (m_Engine->activeSkeletonIndex >= 0 &&
                m_Engine->activeSkeletonIndex < static_cast<int>(m_Engine->skeletons.size())) {
                const auto& skel = m_Engine->skeletons[m_Engine->activeSkeletonIndex];
                ImGui::Text("Bone Count: %d", static_cast<int>(skel.bones.size()));
            } else {
                ImGui::Text("Bone Count: 0");
            }
            ImGui::TreePop();
        }

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

    // Actions section for GLTF nodes
    if (ImGui::CollapsingHeader("Actions")) {
        ImGui::Indent();

        // Focus camera
        if (ImGui::Button("Focus Camera##nodeActions", ImVec2(-1, 0))) {
            glm::vec3 pos = glm::vec3(node->worldTransform[3]);
            FocusCameraOnPosition(pos);
        }

        // Move to origin
        if (ImGui::Button("Move to Origin", ImVec2(-1, 0))) {
            // Extract current scale and rotation, reset translation
            glm::vec3 scale, translation, skew;
            glm::vec4 perspective;
            glm::quat rotation;
            if (glm::decompose(node->localTransform, scale, rotation, translation, skew, perspective)) {
                glm::vec3 eulerRot = glm::degrees(glm::eulerAngles(rotation));
                applyLocalTRS(glm::vec3(0.0f), eulerRot, scale);
            }
        }

        // Reset all transforms
        if (ImGui::Button("Reset Transform", ImVec2(-1, 0))) {
            node->localTransform = glm::mat4(1.0f);
            refreshNodeWorldFromLocal();
        }

        // Scale to unit size
        if (ImGui::Button("Reset Scale to 1", ImVec2(-1, 0))) {
            glm::vec3 scale, translation, skew;
            glm::vec4 perspective;
            glm::quat rotation;
            if (glm::decompose(node->localTransform, scale, rotation, translation, skew, perspective)) {
                glm::vec3 eulerRot = glm::degrees(glm::eulerAngles(rotation));
                applyLocalTRS(translation, eulerRot, glm::vec3(1.0f));
            }
        }

        // Uniform scale
        ImGui::Spacing();
        ImGui::TextDisabled("Uniform Scale:");
        if (ImGui::Button("0.5x", ImVec2(50, 0))) {
            glm::vec3 scale, translation, skew;
            glm::vec4 perspective;
            glm::quat rotation;
            if (glm::decompose(node->localTransform, scale, rotation, translation, skew, perspective)) {
                glm::vec3 eulerRot = glm::degrees(glm::eulerAngles(rotation));
                applyLocalTRS(translation, eulerRot, scale * 0.5f);
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("2x", ImVec2(50, 0))) {
            glm::vec3 scale, translation, skew;
            glm::vec4 perspective;
            glm::quat rotation;
            if (glm::decompose(node->localTransform, scale, rotation, translation, skew, perspective)) {
                glm::vec3 eulerRot = glm::degrees(glm::eulerAngles(rotation));
                applyLocalTRS(translation, eulerRot, scale * 2.0f);
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("0.1x", ImVec2(50, 0))) {
            glm::vec3 scale, translation, skew;
            glm::vec4 perspective;
            glm::quat rotation;
            if (glm::decompose(node->localTransform, scale, rotation, translation, skew, perspective)) {
                glm::vec3 eulerRot = glm::degrees(glm::eulerAngles(rotation));
                applyLocalTRS(translation, eulerRot, scale * 0.1f);
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("10x", ImVec2(50, 0))) {
            glm::vec3 scale, translation, skew;
            glm::vec4 perspective;
            glm::quat rotation;
            if (glm::decompose(node->localTransform, scale, rotation, translation, skew, perspective)) {
                glm::vec3 eulerRot = glm::degrees(glm::eulerAngles(rotation));
                applyLocalTRS(translation, eulerRot, scale * 10.0f);
            }
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
// GLTF MATERIAL EDITOR - Inline PBR editing for GLTF nodes
// =============================================================================

void ObjectInspectorView::RenderGLTFMaterialEditor(MeshNode* node) {
    if (!node || !node->mesh || node->mesh->surfaces.empty()) return;

    auto& surfaces = node->mesh->surfaces;

    // Surface selector is shared with MaterialView to keep both panels synchronized.
    int& selectedSurface = m_Engine->selectedGLTFSurfaceIndex;
    if (selectedSurface < 0 || selectedSurface >= static_cast<int>(surfaces.size())) {
        selectedSurface = 0;
    }

    if (surfaces.size() > 1) {
        ImGui::Text("Surfaces: %zu", surfaces.size());
        ImGui::SetNextItemWidth(-1);
        if (ImGui::BeginCombo("##SurfaceSelect", ("Surface " + std::to_string(selectedSurface)).c_str())) {
            for (int i = 0; i < static_cast<int>(surfaces.size()); ++i) {
                bool isSelected = (i == selectedSurface);
                char label[64];
                snprintf(label, sizeof(label), "Surface %d (%u tris)", i, surfaces[i].count / 3);
                if (ImGui::Selectable(label, isSelected)) {
                    selectedSurface = i;
                }
            }
            ImGui::EndCombo();
        }
        ImGui::Spacing();
    }

    auto& surface = surfaces[selectedSurface];
    if (!surface.material) {
        ImGui::TextDisabled("No material assigned");
        return;
    }

    // Find the scene that owns this material and read/write its buffer
    uint32_t bufferOffset = surface.material->bufferOffset;
    LoadedGLTF* ownerScene = nullptr;

    for (auto& [sceneName, scene] : m_Engine->loadedScenes) {
        if (!scene) continue;
        for (auto& [matName, mat] : scene->materials) {
            if (mat == surface.material) {
                ownerScene = scene.get();
                break;
            }
        }
        if (ownerScene) break;
    }

    if (!ownerScene || ownerScene->materialDataBuffer.buffer == VK_NULL_HANDLE ||
        ownerScene->materialDataBuffer.allocation == VK_NULL_HANDLE) {
        ImGui::TextDisabled("Material buffer not available");
        return;
    }

    // Map buffer and read current values
    void* data = nullptr;
    VkResult result = vmaMapMemory(m_Engine->_allocator, ownerScene->materialDataBuffer.allocation, &data);
    if (result != VK_SUCCESS || !data) {
        ImGui::TextDisabled("Failed to read material data");
        return;
    }

    auto* constants = reinterpret_cast<GLTFMetallic_Roughness::MaterialConstants*>(
        static_cast<char*>(data) + bufferOffset);

    // Read current values
    glm::vec4 baseColor = constants->colorFactors;
    float metallic = constants->metal_rough_factors.x;
    float roughness = constants->metal_rough_factors.y;
    float ao = constants->metal_rough_factors.z;
    float normalStrength = constants->metal_rough_factors.w;
    glm::vec3 emission = glm::vec3(constants->extra[0]);
    float emissionStrength = constants->extra[0].w;
    float reflectionIntensity = constants->extra[1].x;
    float displacementScale = constants->extra[2].x;
    float displacementBias = constants->extra[2].y;
    uint32_t displacementTexID = static_cast<uint32_t>(std::max(constants->extra[11].x, 0.0f));
    bool useDisplacementTexture = (displacementTexID > 0);
    uint32_t aoTexID = static_cast<uint32_t>(std::max(constants->extra[11].y, 0.0f));
    bool useAOTexture = (aoTexID > 0);
    uint32_t emissiveTexID = constants->emissiveTexID;
    uint32_t emissiveTexBackup = static_cast<uint32_t>(std::max(constants->extra[12].x, 0.0f));
    uint32_t aoTexBackup = static_cast<uint32_t>(std::max(constants->extra[12].y, 0.0f));
    bool useEmissiveTexture = (emissiveTexID > 0);

    vmaUnmapMemory(m_Engine->_allocator, ownerScene->materialDataBuffer.allocation);

    bool changed = false;

    // === BASE COLOR ===
    ImGui::TextDisabled("Base Color (Albedo)");
    float col[4] = { baseColor.r, baseColor.g, baseColor.b, baseColor.a };
    if (ImGui::ColorEdit4("##GLTFBaseColor", col, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_AlphaBar)) {
        baseColor = glm::vec4(col[0], col[1], col[2], col[3]);
        changed = true;
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // === PBR PROPERTIES ===
    ImGui::TextDisabled("PBR Properties");

    ImGui::Text("Metallic");
    ImGui::SameLine(ImGui::GetWindowWidth() - 60);
    ImGui::TextDisabled("%.0f%%", metallic * 100);
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.8f, 0.7f, 0.3f, 1.0f));
    if (ImGui::SliderFloat("##GLTFMetallic", &metallic, 0.0f, 1.0f, "")) changed = true;
    ImGui::PopStyleColor();

    ImGui::Text("Roughness");
    ImGui::SameLine(ImGui::GetWindowWidth() - 60);
    ImGui::TextDisabled("%.0f%%", roughness * 100);
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
    if (ImGui::SliderFloat("##GLTFRoughness", &roughness, 0.0f, 1.0f, "")) changed = true;
    ImGui::PopStyleColor();

    ImGui::Text("Normal Strength");
    ImGui::SameLine(ImGui::GetWindowWidth() - 60);
    ImGui::TextDisabled("%.0f%%", normalStrength * 100);
    if (ImGui::SliderFloat("##GLTFNormalStrength", &normalStrength, 0.0f, 2.0f, "")) changed = true;

    ImGui::Text("Occlusion");
    ImGui::SameLine(ImGui::GetWindowWidth() - 60);
    ImGui::TextDisabled("%.0f%%", ao * 100);
    if (ImGui::SliderFloat("##GLTFAO", &ao, 0.0f, 1.0f, "")) changed = true;
    const std::string aoTexName = (m_Engine && aoTexID > 0) ? m_Engine->texCache.GetTextureName(aoTexID) : std::string();
    ImGui::TextDisabled("AO Texture ID: %u", aoTexID);
    ImGui::TextDisabled("AO Texture Name: %s", aoTexName.empty() ? "<unnamed>" : aoTexName.c_str());
    if (ImGui::Checkbox("Use AO Texture", &useAOTexture)) {
        changed = true;
        if (!useAOTexture) {
            aoTexBackup = aoTexID;
            aoTexID = 0;
        } else if (aoTexID == 0 && aoTexBackup > 0) {
            aoTexID = aoTexBackup;
        }
    }

    // Reflection
    ImGui::Spacing();
    ImGui::Text("Reflection");
    ImGui::SameLine(ImGui::GetWindowWidth() - 60);
    ImGui::TextDisabled("%.0f%%", reflectionIntensity * 100);
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.3f, 0.6f, 0.9f, 1.0f));
    if (ImGui::SliderFloat("##GLTFReflection", &reflectionIntensity, 0.0f, 1.0f, "")) changed = true;
    ImGui::PopStyleColor();

    ImGui::Text("Displacement Scale");
    ImGui::SameLine(ImGui::GetWindowWidth() - 60);
    ImGui::TextDisabled("%.3f", displacementScale);
    if (ImGui::SliderFloat("##GLTFDispScale", &displacementScale, -1.0f, 1.0f, "")) changed = true;
    if (ImGui::SliderFloat("Displacement Bias##GLTFDispBias", &displacementBias, -1.0f, 1.0f, "%.3f")) changed = true;
    if (ImGui::Checkbox("Use Displacement Texture", &useDisplacementTexture)) changed = true;

    // Quick PBR presets
    ImGui::Spacing();
    ImGui::TextDisabled("Quick:");
    if (ImGui::Button("Shiny##gltf", ImVec2(50, 0))) { roughness = 0.1f; changed = true; }
    ImGui::SameLine();
    if (ImGui::Button("Matte##gltf", ImVec2(50, 0))) { roughness = 0.8f; changed = true; }
    ImGui::SameLine();
    if (ImGui::Button("Metal##gltf", ImVec2(50, 0))) { metallic = 1.0f; roughness = 0.3f; changed = true; }
    ImGui::SameLine();
    if (ImGui::Button("Plastic##gltf", ImVec2(60, 0))) { metallic = 0.0f; roughness = 0.4f; changed = true; }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // === EMISSION ===
    ImGui::TextDisabled("Emission");

    float emCol[3] = { emission.r, emission.g, emission.b };
    if (ImGui::ColorEdit3("##GLTFEmColor", emCol, ImGuiColorEditFlags_Float)) {
        emission = glm::vec3(emCol[0], emCol[1], emCol[2]);
        changed = true;
    }
    if (ImGui::SliderFloat("Strength##GLTFEm", &emissionStrength, 0.0f, 10.0f, "%.2f")) {
        changed = true;
    }
    if (ImGui::Checkbox("Use Emissive Texture", &useEmissiveTexture)) {
        changed = true;
        if (!useEmissiveTexture) {
            emissiveTexBackup = emissiveTexID;
            emissiveTexID = 0;
        } else if (emissiveTexID == 0 && emissiveTexBackup > 0) {
            emissiveTexID = emissiveTexBackup;
        }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // === MATERIAL PRESETS ===
    ImGui::TextDisabled("Material Presets");
    if (ImGui::Button("Gold##gltf", ImVec2(55, 0))) {
        baseColor = glm::vec4(1.0f, 0.766f, 0.336f, 1.0f);
        metallic = 1.0f; roughness = 0.3f; changed = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Silver##gltf", ImVec2(55, 0))) {
        baseColor = glm::vec4(0.972f, 0.960f, 0.915f, 1.0f);
        metallic = 1.0f; roughness = 0.2f; changed = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Copper##gltf", ImVec2(55, 0))) {
        baseColor = glm::vec4(0.955f, 0.637f, 0.538f, 1.0f);
        metallic = 1.0f; roughness = 0.25f; changed = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Chrome##gltf", ImVec2(55, 0))) {
        baseColor = glm::vec4(0.549f, 0.556f, 0.554f, 1.0f);
        metallic = 1.0f; roughness = 0.1f; changed = true;
    }

    if (ImGui::Button("Mirror##gltf", ImVec2(55, 0))) {
        baseColor = glm::vec4(0.95f, 0.95f, 0.95f, 1.0f);
        metallic = 1.0f; roughness = 0.05f; reflectionIntensity = 1.0f; changed = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Reflect##gltf", ImVec2(55, 0))) { reflectionIntensity = 0.5f; changed = true; }
    ImGui::SameLine();
    if (ImGui::Button("No Refl##gltf", ImVec2(55, 0))) { reflectionIntensity = 0.0f; changed = true; }

    // === WRITE BACK TO GPU BUFFER ===
    if (changed) {
        void* writeData = nullptr;
        VkResult writeResult = vmaMapMemory(m_Engine->_allocator, ownerScene->materialDataBuffer.allocation, &writeData);
        if (writeResult == VK_SUCCESS && writeData) {
            auto* writeConstants = reinterpret_cast<GLTFMetallic_Roughness::MaterialConstants*>(
                static_cast<char*>(writeData) + bufferOffset);

            writeConstants->colorFactors = baseColor;
            writeConstants->metal_rough_factors = glm::vec4(metallic, roughness, ao, normalStrength);
            writeConstants->extra[0] = glm::vec4(emission, emissionStrength);
            writeConstants->extra[1].x = reflectionIntensity;
            writeConstants->extra[2].x = displacementScale;
            writeConstants->extra[2].y = displacementBias;
            writeConstants->emissiveTexID = useEmissiveTexture ? emissiveTexID : 0;
            writeConstants->extra[11].x = static_cast<float>(useDisplacementTexture ? displacementTexID : 0u);
            writeConstants->extra[11].y = static_cast<float>(useAOTexture ? aoTexID : 0u);
            writeConstants->extra[12].x = static_cast<float>(emissiveTexBackup);
            writeConstants->extra[12].y = static_cast<float>(aoTexBackup);

            vmaUnmapMemory(m_Engine->_allocator, ownerScene->materialDataBuffer.allocation);

            vmaFlushAllocation(m_Engine->_allocator, ownerScene->materialDataBuffer.allocation,
                bufferOffset, sizeof(GLTFMetallic_Roughness::MaterialConstants));
        }
    }
}

// =============================================================================
// TRANSFORM EDITOR - Colored XYZ axes
// =============================================================================

void ObjectInspectorView::RenderTransformEditor(glm::vec3& position, glm::vec3& rotation, glm::vec3& scale) {
    // Get snap settings
    bool posSnap = m_Engine && m_Engine->snapEnabled;
    bool rotSnap = m_Engine && m_Engine->snapRotationEnabled;
    bool sclSnap = m_Engine && m_Engine->snapScaleEnabled;
    float posSnapVal = m_Engine ? m_Engine->snapPositionValue : 1.0f;
    float rotSnapVal = m_Engine ? m_Engine->snapRotationAngle : 15.0f;
    float sclSnapVal = m_Engine ? m_Engine->snapScaleValue : 0.1f;

    // Position with colored XYZ indicators
    ImGui::Text("Position");
    if (posSnap) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "[%.2f]", posSnapVal);
    }

    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.5f, 0.15f, 0.15f, 1.0f));
    ImGui::SetNextItemWidth(70);
    ImGui::DragFloat("##PosX", &position.x, posSnap ? posSnapVal : 0.1f, 0, 0, "X: %.2f");
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.5f, 0.15f, 1.0f));
    ImGui::SetNextItemWidth(70);
    ImGui::DragFloat("##PosY", &position.y, posSnap ? posSnapVal : 0.1f, 0, 0, "Y: %.2f");
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.5f, 1.0f));
    ImGui::SetNextItemWidth(70);
    ImGui::DragFloat("##PosZ", &position.z, posSnap ? posSnapVal : 0.1f, 0, 0, "Z: %.2f");
    ImGui::PopStyleColor();

    // Apply position snap
    if (posSnap) {
        position.x = glm::round(position.x / posSnapVal) * posSnapVal;
        position.y = glm::round(position.y / posSnapVal) * posSnapVal;
        position.z = glm::round(position.z / posSnapVal) * posSnapVal;
    }

    // Rotation
    ImGui::Text("Rotation");
    if (rotSnap) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "[%.0f°]", rotSnapVal);
    }

    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.5f, 0.15f, 0.15f, 1.0f));
    ImGui::SetNextItemWidth(70);
    ImGui::DragFloat("##RotX", &rotation.x, rotSnap ? rotSnapVal : 1.0f, 0, 0, "X: %.1f");
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.5f, 0.15f, 1.0f));
    ImGui::SetNextItemWidth(70);
    ImGui::DragFloat("##RotY", &rotation.y, rotSnap ? rotSnapVal : 1.0f, 0, 0, "Y: %.1f");
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.5f, 1.0f));
    ImGui::SetNextItemWidth(70);
    ImGui::DragFloat("##RotZ", &rotation.z, rotSnap ? rotSnapVal : 1.0f, 0, 0, "Z: %.1f");
    ImGui::PopStyleColor();

    // Apply rotation snap
    if (rotSnap) {
        rotation.x = glm::round(rotation.x / rotSnapVal) * rotSnapVal;
        rotation.y = glm::round(rotation.y / rotSnapVal) * rotSnapVal;
        rotation.z = glm::round(rotation.z / rotSnapVal) * rotSnapVal;
    }

    // Scale
    ImGui::Text("Scale");
    if (sclSnap) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "[%.2f]", sclSnapVal);
    }

    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.5f, 0.15f, 0.15f, 1.0f));
    ImGui::SetNextItemWidth(70);
    ImGui::DragFloat("##SclX", &scale.x, sclSnap ? sclSnapVal : 0.01f, 0.01f, 100.0f, "X: %.2f");
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.5f, 0.15f, 1.0f));
    ImGui::SetNextItemWidth(70);
    ImGui::DragFloat("##SclY", &scale.y, sclSnap ? sclSnapVal : 0.01f, 0.01f, 100.0f, "Y: %.2f");
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.5f, 1.0f));
    ImGui::SetNextItemWidth(70);
    ImGui::DragFloat("##SclZ", &scale.z, sclSnap ? sclSnapVal : 0.01f, 0.01f, 100.0f, "Z: %.2f");
    ImGui::PopStyleColor();

    // Apply scale snap
    if (sclSnap) {
        scale.x = glm::round(scale.x / sclSnapVal) * sclSnapVal;
        scale.y = glm::round(scale.y / sclSnapVal) * sclSnapVal;
        scale.z = glm::round(scale.z / sclSnapVal) * sclSnapVal;
        // Ensure scale doesn't go below minimum
        scale.x = glm::max(scale.x, sclSnapVal);
        scale.y = glm::max(scale.y, sclSnapVal);
        scale.z = glm::max(scale.z, sclSnapVal);
    }
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

// =============================================================================
// HELPER: Focus Camera on Target Position
// =============================================================================
void ObjectInspectorView::FocusCameraOnPosition(const glm::vec3& targetPos) {
    if (!m_Engine) return;

    // Position camera behind and above the target
    glm::vec3 offset(0.0f, 2.0f, 5.0f);
    m_Engine->mainCamera.position = targetPos + offset;

    // Calculate direction from camera to target
    glm::vec3 direction = glm::normalize(targetPos - m_Engine->mainCamera.position);

    // Calculate yaw (horizontal angle) - atan2 gives angle in XZ plane
    m_Engine->mainCamera.yaw = glm::degrees(atan2(direction.x, direction.z));

    // Calculate pitch (vertical angle) - asin gives angle from horizontal
    m_Engine->mainCamera.pitch = glm::degrees(asin(-direction.y));

    // Clamp pitch to avoid gimbal lock
    m_Engine->mainCamera.pitch = glm::clamp(m_Engine->mainCamera.pitch, -89.0f, 89.0f);
}

} // namespace Yalaz::UI
