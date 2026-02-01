// =============================================================================
// YALAZ ENGINE - Material View Implementation
// =============================================================================
// Dynamic material editor connected to selected primitive:
// - Edits material of currently selected primitive
// - PBR properties with real-time preview
// - Material presets apply to selection
// - Syncs with ObjectInspectorView
// =============================================================================

#include "MaterialView.h"
#include "../../vk_engine.h"
#include <glm/gtc/type_ptr.hpp>
#include <filesystem>

namespace fs = std::filesystem;

namespace Yalaz::UI {

void MaterialView::OnInit(VulkanEngine* engine) {
    EditorView::OnInit(engine);
    InitPresets();
}

void MaterialView::InitPresets() {
    m_Presets.clear();

    // Metals
    m_Presets.push_back({"Gold", glm::vec4(1.0f, 0.766f, 0.336f, 1.0f), 1.0f, 0.3f, 1.0f, glm::vec3(0)});
    m_Presets.push_back({"Silver", glm::vec4(0.972f, 0.960f, 0.915f, 1.0f), 1.0f, 0.2f, 1.0f, glm::vec3(0)});
    m_Presets.push_back({"Copper", glm::vec4(0.955f, 0.637f, 0.538f, 1.0f), 1.0f, 0.25f, 1.0f, glm::vec3(0)});
    m_Presets.push_back({"Iron", glm::vec4(0.56f, 0.57f, 0.58f, 1.0f), 1.0f, 0.4f, 1.0f, glm::vec3(0)});
    m_Presets.push_back({"Aluminum", glm::vec4(0.913f, 0.921f, 0.925f, 1.0f), 1.0f, 0.35f, 1.0f, glm::vec3(0)});
    m_Presets.push_back({"Chrome", glm::vec4(0.549f, 0.556f, 0.554f, 1.0f), 1.0f, 0.1f, 1.0f, glm::vec3(0)});

    // Plastics
    m_Presets.push_back({"Red Plastic", glm::vec4(0.9f, 0.1f, 0.1f, 1.0f), 0.0f, 0.4f, 1.0f, glm::vec3(0)});
    m_Presets.push_back({"Blue Plastic", glm::vec4(0.1f, 0.3f, 0.9f, 1.0f), 0.0f, 0.4f, 1.0f, glm::vec3(0)});
    m_Presets.push_back({"Green Plastic", glm::vec4(0.1f, 0.8f, 0.2f, 1.0f), 0.0f, 0.4f, 1.0f, glm::vec3(0)});
    m_Presets.push_back({"White Plastic", glm::vec4(0.95f, 0.95f, 0.95f, 1.0f), 0.0f, 0.35f, 1.0f, glm::vec3(0)});
    m_Presets.push_back({"Black Plastic", glm::vec4(0.05f, 0.05f, 0.05f, 1.0f), 0.0f, 0.5f, 1.0f, glm::vec3(0)});

    // Natural materials
    m_Presets.push_back({"Wood", glm::vec4(0.6f, 0.4f, 0.2f, 1.0f), 0.0f, 0.7f, 0.8f, glm::vec3(0)});
    m_Presets.push_back({"Stone", glm::vec4(0.5f, 0.5f, 0.5f, 1.0f), 0.0f, 0.8f, 0.9f, glm::vec3(0)});
    m_Presets.push_back({"Marble", glm::vec4(0.9f, 0.9f, 0.85f, 1.0f), 0.0f, 0.3f, 1.0f, glm::vec3(0)});
    m_Presets.push_back({"Skin", glm::vec4(0.9f, 0.7f, 0.6f, 1.0f), 0.0f, 0.6f, 1.0f, glm::vec3(0)});

    // Special
    m_Presets.push_back({"Glass", glm::vec4(0.95f, 0.95f, 0.95f, 0.2f), 0.0f, 0.05f, 1.0f, glm::vec3(0)});
    m_Presets.push_back({"Rubber", glm::vec4(0.1f, 0.1f, 0.1f, 1.0f), 0.0f, 0.9f, 1.0f, glm::vec3(0)});
    m_Presets.push_back({"Emissive", glm::vec4(1.0f, 0.5f, 0.2f, 1.0f), 0.0f, 0.5f, 1.0f, glm::vec3(1.0f, 0.5f, 0.2f)});
}

void MaterialView::OnRender() {
    if (!BeginView(ImGuiWindowFlags_MenuBar)) {
        EndView();
        return;
    }

    // Sync with selected primitive
    SyncWithSelection();

    if (ImGui::BeginMenuBar()) {
        // Show what's selected
        if (m_Engine && m_Engine->selectedPrimitiveIndex >= 0) {
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), "Editing: %s",
                m_Engine->static_shapes[m_Engine->selectedPrimitiveIndex].name.c_str());
        } else {
            ImGui::TextDisabled("No primitive selected");
        }

        ImGui::SameLine(ImGui::GetWindowWidth() - 180);
        ImGui::Checkbox("Auto-rotate", &m_AutoRotate);
        ImGui::SameLine();
        ImGui::Checkbox("Presets", &m_ShowPresets);
        ImGui::EndMenuBar();
    }

    // Check if we have a valid selection
    bool hasSelection = m_Engine && m_Engine->selectedPrimitiveIndex >= 0 &&
                        m_Engine->selectedPrimitiveIndex < static_cast<int>(m_Engine->static_shapes.size());

    if (!hasSelection) {
        ImGui::TextWrapped("Select a primitive in the Hierarchy to edit its material.");
        EndView();
        return;
    }

    // Main layout
    ImGui::Columns(2, "MaterialColumns", true);
    ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * 0.4f);

    // Left: Preview
    RenderPreview();

    ImGui::NextColumn();

    // Right: Properties and textures in tabs
    if (ImGui::BeginTabBar("MaterialTabs")) {
        if (ImGui::BeginTabItem("Properties")) {
            RenderMaterialProperties();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Textures")) {
            RenderTextureSlots();
            ImGui::EndTabItem();
        }

        if (m_ShowPresets && ImGui::BeginTabItem("Presets")) {
            RenderPresets();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::Columns(1);

    EndView();
}

void MaterialView::SyncWithSelection() {
    if (!m_Engine) return;

    int currentSelection = m_Engine->selectedPrimitiveIndex;

    // If selection changed, load the primitive's material
    if (currentSelection != m_LastSelectedIndex && currentSelection >= 0 &&
        currentSelection < static_cast<int>(m_Engine->static_shapes.size())) {

        auto& shape = m_Engine->static_shapes[currentSelection];
        m_BaseColor = shape.mainColor;
        // Note: metallic/roughness would need to be stored in StaticMeshData
        m_LastSelectedIndex = currentSelection;
    }
}

void MaterialView::ApplyToSelection() {
    if (!m_Engine) return;

    if (m_Engine->selectedPrimitiveIndex >= 0 &&
        m_Engine->selectedPrimitiveIndex < static_cast<int>(m_Engine->static_shapes.size())) {

        auto& shape = m_Engine->static_shapes[m_Engine->selectedPrimitiveIndex];
        shape.mainColor = m_BaseColor;
        // Apply other properties as engine supports them
    }
}

void MaterialView::RenderPreview() {
    SectionHeader("Preview");

    // Preview mesh selector
    const char* meshes[] = { "Sphere", "Cube", "Plane", "Cylinder", "Torus" };
    ImGui::SetNextItemWidth(-1);
    ImGui::Combo("##PreviewMesh", &m_PreviewMesh, meshes, 5);

    // Preview area
    ImVec2 size = ImGui::GetContentRegionAvail();
    float previewSize = std::min(size.x - 10, size.y - 30);
    previewSize = std::max(previewSize, 100.0f);

    ImGui::BeginChild("PreviewArea", ImVec2(previewSize, previewSize), true);
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // Gradient background
    drawList->AddRectFilledMultiColor(
        pos, ImVec2(pos.x + previewSize, pos.y + previewSize),
        IM_COL32(40, 40, 50, 255), IM_COL32(40, 40, 50, 255),
        IM_COL32(25, 25, 35, 255), IM_COL32(25, 25, 35, 255));

    // Material preview sphere
    ImVec2 center(pos.x + previewSize / 2, pos.y + previewSize / 2);
    float radius = previewSize * 0.35f;

    // Base color with lighting simulation
    float lightIntensity = 0.8f + 0.2f * m_AO;
    ImU32 matColor = IM_COL32(
        (int)(m_BaseColor.r * 255 * lightIntensity),
        (int)(m_BaseColor.g * 255 * lightIntensity),
        (int)(m_BaseColor.b * 255 * lightIntensity),
        (int)(m_BaseColor.a * 255));

    // Draw based on mesh type
    switch (m_PreviewMesh) {
        case 0: // Sphere
            drawList->AddCircleFilled(center, radius, matColor);
            break;
        case 1: // Cube
            drawList->AddRectFilled(
                ImVec2(center.x - radius * 0.7f, center.y - radius * 0.7f),
                ImVec2(center.x + radius * 0.7f, center.y + radius * 0.7f),
                matColor);
            break;
        case 2: // Plane
            drawList->AddQuadFilled(
                ImVec2(center.x, center.y - radius * 0.8f),
                ImVec2(center.x + radius, center.y),
                ImVec2(center.x, center.y + radius * 0.8f),
                ImVec2(center.x - radius, center.y),
                matColor);
            break;
        default:
            drawList->AddCircleFilled(center, radius, matColor);
            break;
    }

    // Specular highlight (based on roughness and metallic)
    float highlightIntensity = (1.0f - m_Roughness) * (m_Metallic * 0.5f + 0.5f);
    float highlightSize = radius * (1.0f - m_Roughness * 0.8f) * 0.3f;
    ImVec2 highlightPos(center.x - radius * 0.25f, center.y - radius * 0.25f);

    if (highlightIntensity > 0.1f) {
        drawList->AddCircleFilled(highlightPos, highlightSize,
            IM_COL32(255, 255, 255, (int)(120 * highlightIntensity)));
    }

    // Emission glow
    if (m_EmissionStrength > 0.01f) {
        ImU32 emitColor = IM_COL32(
            (int)(m_Emission.r * 255 * m_EmissionStrength * 0.3f),
            (int)(m_Emission.g * 255 * m_EmissionStrength * 0.3f),
            (int)(m_Emission.b * 255 * m_EmissionStrength * 0.3f),
            100);
        drawList->AddCircle(center, radius + 5, emitColor, 32, 3.0f);
    }

    ImGui::EndChild();

    if (m_AutoRotate) {
        m_PreviewRotation += ImGui::GetIO().DeltaTime * 30.0f;
    }

    // Material info
    ImGui::TextDisabled("Metallic: %.0f%% | Roughness: %.0f%%",
        m_Metallic * 100, m_Roughness * 100);
}

void MaterialView::RenderMaterialProperties() {
    bool changed = false;

    // Base Color with alpha
    SectionHeader("Base Color");
    float col[4] = { m_BaseColor.r, m_BaseColor.g, m_BaseColor.b, m_BaseColor.a };
    if (ImGui::ColorEdit4("##BaseColor", col, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_AlphaBar)) {
        m_BaseColor = glm::vec4(col[0], col[1], col[2], col[3]);
        changed = true;
    }

    ImGui::Spacing();
    SectionHeader("PBR Properties");

    // Metallic with visual indicator
    ImGui::Text("Metallic");
    ImGui::SameLine(ImGui::GetWindowWidth() - 60);
    ImGui::TextDisabled("%.0f%%", m_Metallic * 100);
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.8f, 0.7f, 0.3f, 1.0f));
    if (ImGui::SliderFloat("##Metallic", &m_Metallic, 0.0f, 1.0f, "")) {
        changed = true;
    }
    ImGui::PopStyleColor();

    // Roughness with visual indicator
    ImGui::Text("Roughness");
    ImGui::SameLine(ImGui::GetWindowWidth() - 60);
    ImGui::TextDisabled("%.0f%%", m_Roughness * 100);
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
    if (ImGui::SliderFloat("##Roughness", &m_Roughness, 0.0f, 1.0f, "")) {
        changed = true;
    }
    ImGui::PopStyleColor();

    // Quick presets for metallic/roughness
    ImGui::TextDisabled("Quick:");
    if (ImGui::Button("Shiny", ImVec2(50, 0))) { m_Roughness = 0.1f; changed = true; }
    ImGui::SameLine();
    if (ImGui::Button("Matte", ImVec2(50, 0))) { m_Roughness = 0.8f; changed = true; }
    ImGui::SameLine();
    if (ImGui::Button("Metal", ImVec2(50, 0))) { m_Metallic = 1.0f; m_Roughness = 0.3f; changed = true; }
    ImGui::SameLine();
    if (ImGui::Button("Plastic", ImVec2(50, 0))) { m_Metallic = 0.0f; m_Roughness = 0.4f; changed = true; }

    ImGui::Spacing();
    SectionHeader("Additional");

    // AO
    if (ImGui::SliderFloat("Ambient Occlusion", &m_AO, 0.0f, 1.0f)) {
        changed = true;
    }
    if (ImGui::SliderFloat("Normal Strength", &m_NormalStrength, 0.0f, 2.0f)) {
        changed = true;
    }

    ImGui::Spacing();
    SectionHeader("Emission");

    float emCol[3] = { m_Emission.r, m_Emission.g, m_Emission.b };
    if (ImGui::ColorEdit3("##EmissionColor", emCol)) {
        m_Emission = glm::vec3(emCol[0], emCol[1], emCol[2]);
        changed = true;
    }
    if (ImGui::SliderFloat("Emission Strength", &m_EmissionStrength, 0.0f, 10.0f)) {
        changed = true;
    }

    // Emission presets
    if (m_EmissionStrength > 0) {
        ImGui::TextDisabled("Presets:");
        if (ImGui::Button("Fire", ImVec2(40, 0))) { m_Emission = glm::vec3(1.0f, 0.3f, 0.1f); m_EmissionStrength = 3.0f; changed = true; }
        ImGui::SameLine();
        if (ImGui::Button("Neon", ImVec2(40, 0))) { m_Emission = glm::vec3(0.2f, 1.0f, 0.8f); m_EmissionStrength = 5.0f; changed = true; }
        ImGui::SameLine();
        if (ImGui::Button("Glow", ImVec2(40, 0))) { m_Emission = glm::vec3(0.5f, 0.5f, 1.0f); m_EmissionStrength = 2.0f; changed = true; }
    }

    // Apply changes to selected primitive
    if (changed) {
        ApplyToSelection();
    }
}

void MaterialView::RenderTextureSlots() {
    ImGui::TextWrapped("Drag textures from Asset Browser to assign them to slots.");
    ImGui::Spacing();

    RenderTextureSlot("Albedo (Base Color)", m_AlbedoSlot, "TEXTURE_PATH");
    RenderTextureSlot("Normal Map", m_NormalSlot, "TEXTURE_PATH");
    RenderTextureSlot("Metallic/Roughness", m_MetallicRoughnessSlot, "TEXTURE_PATH");
    RenderTextureSlot("Ambient Occlusion", m_AOSlot, "TEXTURE_PATH");
    RenderTextureSlot("Emission", m_EmissionSlot, "TEXTURE_PATH");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Clear all button
    if (ImGui::Button("Clear All Textures", ImVec2(-1, 0))) {
        m_AlbedoSlot = {"Albedo", "", false};
        m_NormalSlot = {"Normal", "", false};
        m_MetallicRoughnessSlot = {"Metallic/Roughness", "", false};
        m_AOSlot = {"Ambient Occlusion", "", false};
        m_EmissionSlot = {"Emission", "", false};
    }
}

void MaterialView::RenderTextureSlot(const char* label, TextureSlot& slot, const char* payloadType) {
    ImGui::PushID(label);

    // Slot header
    ImGui::TextDisabled("%s", label);

    // Slot button/drop target
    ImVec4 slotColor = slot.isLoaded ?
        ImVec4(0.2f, 0.4f, 0.3f, 1.0f) :
        ImVec4(0.2f, 0.2f, 0.25f, 1.0f);

    ImGui::PushStyleColor(ImGuiCol_Button, slotColor);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(slotColor.x + 0.1f, slotColor.y + 0.1f, slotColor.z + 0.1f, 1.0f));

    std::string buttonLabel = slot.isLoaded ?
        fs::path(slot.path).filename().string() :
        "[Drop Texture Here]";

    if (ImGui::Button(buttonLabel.c_str(), ImVec2(-1, 40))) {
        // Click to clear
        if (slot.isLoaded) {
            slot.path.clear();
            slot.isLoaded = false;
        }
    }

    // Drag-drop target
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(payloadType)) {
            const char* droppedPath = static_cast<const char*>(payload->Data);
            slot.path = droppedPath;
            slot.isLoaded = true;
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::PopStyleColor(2);

    // Tooltip
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        if (slot.isLoaded) {
            ImGui::Text("Texture: %s", slot.path.c_str());
            ImGui::TextDisabled("Click to remove");
        } else {
            ImGui::Text("Drop a texture from Asset Browser");
        }
        ImGui::EndTooltip();
    }

    // Clear button for loaded textures
    if (slot.isLoaded) {
        ImGui::SameLine();
        if (ImGui::Button("X", ImVec2(20, 40))) {
            slot.path.clear();
            slot.isLoaded = false;
        }
    }

    ImGui::PopID();
    ImGui::Spacing();
}

void MaterialView::RenderPresets() {
    ImGui::TextDisabled("Click a preset to apply it to selected primitive");
    ImGui::Spacing();

    // Group presets by category
    const char* categories[] = { "Metals", "Plastics", "Natural", "Special" };
    int categoryStarts[] = { 0, 6, 11, 15 };
    int categoryEnds[] = { 6, 11, 15, (int)m_Presets.size() };

    for (int cat = 0; cat < 4; ++cat) {
        if (ImGui::CollapsingHeader(categories[cat], ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Indent();

            for (int i = categoryStarts[cat]; i < categoryEnds[cat] && i < (int)m_Presets.size(); ++i) {
                const auto& preset = m_Presets[i];

                ImGui::PushID(i);

                // Color preview
                ImVec4 previewColor(preset.baseColor.r, preset.baseColor.g, preset.baseColor.b, 1.0f);
                ImGui::PushStyleColor(ImGuiCol_Button, previewColor);

                if (ImGui::Button("##Color", ImVec2(25, 25))) {
                    // Apply preset to material view
                    m_BaseColor = preset.baseColor;
                    m_Metallic = preset.metallic;
                    m_Roughness = preset.roughness;
                    m_AO = preset.ao;
                    m_Emission = preset.emission;
                    m_EmissionStrength = glm::length(preset.emission) > 0 ? 2.0f : 0.0f;
                    m_SelectedPreset = i;

                    // Apply to selected primitive
                    ApplyToSelection();
                }

                ImGui::PopStyleColor();

                ImGui::SameLine();
                bool isSelected = (m_SelectedPreset == i);
                if (isSelected) {
                    ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "%s", preset.name.c_str());
                } else {
                    ImGui::Text("%s", preset.name.c_str());
                }

                // Tooltip with details
                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::Text("%s", preset.name.c_str());
                    ImGui::TextDisabled("Metallic: %.0f%% | Roughness: %.0f%%",
                        preset.metallic * 100, preset.roughness * 100);
                    ImGui::EndTooltip();
                }

                ImGui::PopID();
            }

            ImGui::Unindent();
        }
    }
}

// Public API implementations
void MaterialView::SetAlbedoTexture(const std::string& path) {
    m_AlbedoSlot.path = path;
    m_AlbedoSlot.isLoaded = !path.empty();
}

void MaterialView::SetNormalTexture(const std::string& path) {
    m_NormalSlot.path = path;
    m_NormalSlot.isLoaded = !path.empty();
}

void MaterialView::SetMetallicRoughnessTexture(const std::string& path) {
    m_MetallicRoughnessSlot.path = path;
    m_MetallicRoughnessSlot.isLoaded = !path.empty();
}

void MaterialView::SetAOTexture(const std::string& path) {
    m_AOSlot.path = path;
    m_AOSlot.isLoaded = !path.empty();
}

void MaterialView::SetEmissionTexture(const std::string& path) {
    m_EmissionSlot.path = path;
    m_EmissionSlot.isLoaded = !path.empty();
}

} // namespace Yalaz::UI
