// =============================================================================
// YALAZ ENGINE - Material View Implementation
// =============================================================================
// Dynamic material editor supporting both primitives and GLTF materials
// - Real-time PBR property updates via GPU buffer mapping
// - Dynamic texture loading from Asset Browser drag-drop
// - Descriptor set rebuilding for texture changes
// - Save/Load .mat files (JSON format)
// =============================================================================

#include "MaterialView.h"
#include "../../vk_engine.h"
#include "../../vk_loader.h"
#include "../../vk_descriptors.h"
#include "../../assets/MaterialFile.h"
#include <glm/gtc/type_ptr.hpp>
#include <filesystem>
#include <algorithm>
#include <stb_image.h>

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

    // Ceramic & Fabric
    m_Presets.push_back({"Ceramic", glm::vec4(0.95f, 0.93f, 0.88f, 1.0f), 0.0f, 0.15f, 1.0f, glm::vec3(0)});
    m_Presets.push_back({"Clay", glm::vec4(0.76f, 0.55f, 0.38f, 1.0f), 0.0f, 0.85f, 1.0f, glm::vec3(0)});
    m_Presets.push_back({"Silk", glm::vec4(0.95f, 0.87f, 0.80f, 1.0f), 0.0f, 0.3f, 1.0f, glm::vec3(0)});
    m_Presets.push_back({"Velvet", glm::vec4(0.35f, 0.05f, 0.15f, 1.0f), 0.0f, 0.95f, 1.0f, glm::vec3(0)});
    m_Presets.push_back({"Leather", glm::vec4(0.45f, 0.25f, 0.10f, 1.0f), 0.0f, 0.65f, 1.0f, glm::vec3(0)});

    // Emissive variants
    m_Presets.push_back({"Neon Red", glm::vec4(1.0f, 0.1f, 0.1f, 1.0f), 0.0f, 0.5f, 1.0f, glm::vec3(1.0f, 0.1f, 0.1f)});
    m_Presets.push_back({"Neon Blue", glm::vec4(0.1f, 0.3f, 1.0f, 1.0f), 0.0f, 0.5f, 1.0f, glm::vec3(0.1f, 0.3f, 1.0f)});
    m_Presets.push_back({"Neon Green", glm::vec4(0.1f, 1.0f, 0.3f, 1.0f), 0.0f, 0.5f, 1.0f, glm::vec3(0.1f, 1.0f, 0.3f)});
    m_Presets.push_back({"Lava", glm::vec4(0.9f, 0.2f, 0.0f, 1.0f), 0.0f, 0.7f, 1.0f, glm::vec3(1.0f, 0.3f, 0.0f)});
}

void MaterialView::OnRender() {
    if (!BeginView(ImGuiWindowFlags_MenuBar)) {
        EndView();
        return;
    }

    // Sync with current selection
    SyncWithSelection();

    // Menu bar
    if (ImGui::BeginMenuBar()) {
        // File menu for save/load
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Save Material...", "Ctrl+S", false, m_SelectionType != MaterialSelectionType::None)) {
                m_IsLoadDialog = false;
                m_ShowSaveLoadDialog = true;
                strncpy(m_MaterialNameBuffer, m_MaterialName.c_str(), sizeof(m_MaterialNameBuffer) - 1);
            }
            if (ImGui::MenuItem("Load Material...", "Ctrl+O")) {
                m_IsLoadDialog = true;
                m_ShowSaveLoadDialog = true;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Export as .mat", nullptr, false, m_SelectionType != MaterialSelectionType::None)) {
                SaveMaterialToFile();
            }
            ImGui::EndMenu();
        }

        ImGui::Separator();

        // Show what's selected
        if (m_SelectionType == MaterialSelectionType::GLTFMaterial && m_SelectedMeshNode) {
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "GLTF: %s",
                m_SelectedMeshNode->mesh ? m_SelectedMeshNode->mesh->name.c_str() : "Unknown");
        } else if (m_SelectionType == MaterialSelectionType::Primitive && m_Engine &&
                   m_Engine->selectedPrimitiveIndex >= 0) {
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), "Primitive: %s",
                m_Engine->static_shapes[m_Engine->selectedPrimitiveIndex].name.c_str());
        } else {
            ImGui::TextDisabled("No selection");
        }

        ImGui::SameLine(ImGui::GetWindowWidth() - 200);
        ImGui::Checkbox("Materials", &m_ShowMaterialList);
        ImGui::SameLine();
        ImGui::Checkbox("Presets", &m_ShowPresets);
        ImGui::EndMenuBar();
    }

    // Render save/load dialog if open
    RenderSaveLoadUI();

    // No selection
    if (m_SelectionType == MaterialSelectionType::None) {
        ImGui::TextWrapped("Select a GLTF model or primitive in the Hierarchy to edit its material.");
        ImGui::Spacing();
        ImGui::TextDisabled("Supported:");
        ImGui::BulletText("GLTF/GLB model nodes");
        ImGui::BulletText("Primitive shapes (cube, sphere, etc.)");
        EndView();
        return;
    }

    // Main layout
    float leftPanelWidth = m_ShowMaterialList ? ImGui::GetWindowWidth() * 0.25f : 0;

    if (m_ShowMaterialList && m_SelectionType == MaterialSelectionType::GLTFMaterial) {
        ImGui::Columns(2, "MaterialViewColumns", true);
        ImGui::SetColumnWidth(0, leftPanelWidth);

        // Left: Material list for GLTF
        RenderGLTFMaterialList();

        ImGui::NextColumn();
    }

    // Right: Material editor
    if (ImGui::BeginTabBar("MaterialTabs")) {
        if (ImGui::BeginTabItem("Properties")) {
            RenderMaterialProperties();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Textures")) {
            RenderTextureSlots();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Preview")) {
            RenderPreview();
            ImGui::EndTabItem();
        }

        if (m_ShowPresets && ImGui::BeginTabItem("Presets")) {
            RenderPresets();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    if (m_ShowMaterialList && m_SelectionType == MaterialSelectionType::GLTFMaterial) {
        ImGui::Columns(1);
    }

    EndView();
}

void MaterialView::SyncWithSelection() {
    if (!m_Engine) return;

    // Check for GLTF MeshNode selection first
    // Use try/catch for dynamic_cast safety since selectedNode could be stale
    MeshNode* meshNode = nullptr;
    if (m_Engine->selectedNode) {
        meshNode = dynamic_cast<MeshNode*>(m_Engine->selectedNode);
    }

    if (meshNode && meshNode->mesh && !meshNode->mesh->surfaces.empty()) {
        // GLTF model selected
        if (meshNode != m_LastSelectedMeshNode) {
            m_SelectionType = MaterialSelectionType::GLTFMaterial;
            m_SelectedMeshNode = meshNode;
            m_SelectedSurfaceIndex = 0;
            m_LastSelectedMeshNode = meshNode;
            m_LastSelectedPrimitiveIndex = -1;
        }

        // Always re-read from GPU buffer to stay in sync with ObjectInspectorView
        LoadGLTFMaterialData();
    } else if (m_Engine->selectedPrimitiveIndex >= 0 &&
               m_Engine->selectedPrimitiveIndex < static_cast<int>(m_Engine->static_shapes.size())) {
        // Primitive selected - always read latest values from engine
        m_SelectionType = MaterialSelectionType::Primitive;

        if (m_Engine->selectedPrimitiveIndex != m_LastSelectedPrimitiveIndex) {
            m_SelectedMeshNode = nullptr;
            m_CurrentGLTFMaterial = nullptr;
            m_LastSelectedPrimitiveIndex = m_Engine->selectedPrimitiveIndex;
            m_LastSelectedMeshNode = nullptr;

            // Sync texture slots from the newly selected primitive's paths
            auto& newShape = m_Engine->static_shapes[m_Engine->selectedPrimitiveIndex];
            m_AlbedoSlot.path = newShape.albedoTexturePath;
            m_AlbedoSlot.isLoaded = !newShape.albedoTexturePath.empty();
            m_MetallicRoughnessSlot.path = newShape.metalRoughTexturePath;
            m_MetallicRoughnessSlot.isLoaded = !newShape.metalRoughTexturePath.empty();
            m_EmissionSlot.path = newShape.emissionTexturePath;
            m_EmissionSlot.isLoaded = !newShape.emissionTexturePath.empty();
            // Clear slots that don't have per-primitive paths
            m_NormalSlot = {"Normal", "", false, 0};
            m_AOSlot = {"Ambient Occlusion", "", false, 0};
        }

        // Always sync from the shape's current values (ObjectInspectorView may have changed them)
        auto& shape = m_Engine->static_shapes[m_Engine->selectedPrimitiveIndex];
        m_BaseColor = shape.mainColor;
        m_Metallic = shape.metallic;
        m_Roughness = shape.roughness;

        float emissionLen = glm::length(shape.emission);
        if (emissionLen > 0.001f) {
            m_EmissionStrength = emissionLen;
            m_Emission = shape.emission / emissionLen;
        } else {
            m_Emission = glm::vec3(0.0f);
            m_EmissionStrength = 0.0f;
        }
    } else {
        // Nothing selected
        m_SelectionType = MaterialSelectionType::None;
        m_SelectedMeshNode = nullptr;
        m_CurrentGLTFMaterial = nullptr;
        m_LastSelectedPrimitiveIndex = -1;
        m_LastSelectedMeshNode = nullptr;
    }
}

void MaterialView::LoadGLTFMaterialData() {
    if (!m_Engine || !m_SelectedMeshNode || !m_SelectedMeshNode->mesh) return;

    auto& surfaces = m_SelectedMeshNode->mesh->surfaces;
    if (surfaces.empty()) {
        m_SelectedSurfaceIndex = 0;
        return;
    }
    if (m_SelectedSurfaceIndex >= static_cast<int>(surfaces.size())) {
        m_SelectedSurfaceIndex = 0;
    }

    auto& surface = surfaces[m_SelectedSurfaceIndex];
    m_CurrentGLTFMaterial = surface.material;

    if (!m_CurrentGLTFMaterial) return;

    uint32_t bufferOffset = m_CurrentGLTFMaterial->bufferOffset;

    // Read actual material constants from the GPU buffer
    // Take a snapshot of loaded scenes to avoid issues with concurrent modification
    for (auto& [sceneName, scene] : m_Engine->loadedScenes) {
        if (!scene) continue;

        bool found = false;
        for (auto& [matName, mat] : scene->materials) {
            if (mat && mat == m_CurrentGLTFMaterial) { found = true; break; }
        }
        if (!found) continue;

        if (scene->materialDataBuffer.buffer != VK_NULL_HANDLE &&
            scene->materialDataBuffer.allocation != VK_NULL_HANDLE) {

            void* data = nullptr;
            VkResult result = vmaMapMemory(m_Engine->_allocator, scene->materialDataBuffer.allocation, &data);

            if (result == VK_SUCCESS && data) {
                auto* constants = reinterpret_cast<GLTFMetallic_Roughness::MaterialConstants*>(
                    static_cast<char*>(data) + bufferOffset);

                m_BaseColor = constants->colorFactors;
                m_Metallic = constants->metal_rough_factors.x;
                m_Roughness = constants->metal_rough_factors.y;
                m_AO = constants->metal_rough_factors.z;
                m_NormalStrength = constants->metal_rough_factors.w;
                m_Emission = glm::vec3(constants->extra[0]);
                m_EmissionStrength = constants->extra[0].w;

                vmaUnmapMemory(m_Engine->_allocator, scene->materialDataBuffer.allocation);
                return;
            }
        }
    }

    // Fallback defaults if buffer read fails
    m_BaseColor = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);
    m_Metallic = 0.0f;
    m_Roughness = 0.5f;
    m_AO = 1.0f;
    m_NormalStrength = 1.0f;
    m_Emission = glm::vec3(0.0f);
    m_EmissionStrength = 0.0f;
}

void MaterialView::RenderGLTFMaterialList() {
    SectionHeader("Materials");

    if (!m_SelectedMeshNode || !m_SelectedMeshNode->mesh) {
        ImGui::TextDisabled("No mesh selected");
        return;
    }

    auto& surfaces = m_SelectedMeshNode->mesh->surfaces;

    ImGui::BeginChild("MaterialList", ImVec2(0, 0), true);

    for (int i = 0; i < static_cast<int>(surfaces.size()); ++i) {
        auto& surface = surfaces[i];

        ImGui::PushID(i);

        bool isSelected = (i == m_SelectedSurfaceIndex);

        // Material card
        ImVec4 cardColor = isSelected ?
            ImVec4(0.25f, 0.4f, 0.55f, 1.0f) :
            ImVec4(0.18f, 0.18f, 0.22f, 1.0f);

        ImGui::PushStyleColor(ImGuiCol_ChildBg, cardColor);

        if (ImGui::BeginChild("MatCard", ImVec2(-1, 50), true)) {
            // Material icon
            ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "[MAT]");
            ImGui::SameLine();

            // Material name/index
            char label[64];
            snprintf(label, sizeof(label), "Material %d", i);
            ImGui::Text("%s", label);

            // Triangle count
            ImGui::TextDisabled("%u triangles", surface.count / 3);

            // Click to select
            if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0)) {
                m_SelectedSurfaceIndex = i;
                LoadGLTFMaterialData();
            }
        }
        ImGui::EndChild();

        ImGui::PopStyleColor();
        ImGui::PopID();

        ImGui::Spacing();
    }

    ImGui::EndChild();
}

void MaterialView::RenderMaterialProperties() {
    bool changed = false;

    // === BASE COLOR (Detailed Color Palette) ===
    SectionHeader("Base Color");

    float col[4] = { m_BaseColor.r, m_BaseColor.g, m_BaseColor.b, m_BaseColor.a };

    // Color display bar - shows current color prominently
    ImVec4 displayColor(col[0], col[1], col[2], col[3]);
    ImGui::PushStyleColor(ImGuiCol_Button, displayColor);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, displayColor);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, displayColor);
    ImGui::Button("##ColorDisplay", ImVec2(-1, 25));
    ImGui::PopStyleColor(3);

    // HSV display
    float h, s, v;
    ImGui::ColorConvertRGBtoHSV(col[0], col[1], col[2], h, s, v);
    ImGui::TextDisabled("RGB: %.0f, %.0f, %.0f  |  HSV: %.0f, %.0f%%, %.0f%%  |  A: %.0f%%",
        col[0] * 255, col[1] * 255, col[2] * 255,
        h * 360, s * 100, v * 100, col[3] * 100);

    // Picker mode toggle
    ImGui::AlignTextToFramePadding();
    ImGui::TextDisabled("Picker:");
    ImGui::SameLine();
    if (ImGui::RadioButton("Wheel", m_ColorPickerMode == 0)) m_ColorPickerMode = 0;
    ImGui::SameLine();
    if (ImGui::RadioButton("Bar", m_ColorPickerMode == 1)) m_ColorPickerMode = 1;
    ImGui::SameLine();
    ImGui::Checkbox("Expanded", &m_ShowFullColorPicker);

    // Full color picker widget
    if (m_ShowFullColorPicker) {
        ImGuiColorEditFlags pickerFlags =
            ImGuiColorEditFlags_AlphaBar |
            ImGuiColorEditFlags_AlphaPreview |
            ImGuiColorEditFlags_Float |
            ImGuiColorEditFlags_DisplayRGB |
            ImGuiColorEditFlags_DisplayHSV |
            ImGuiColorEditFlags_DisplayHex |
            ImGuiColorEditFlags_InputRGB;

        if (m_ColorPickerMode == 0) {
            pickerFlags |= ImGuiColorEditFlags_PickerHueWheel;
        } else {
            pickerFlags |= ImGuiColorEditFlags_PickerHueBar;
        }

        bool wasActive = m_ColorPickerActive;
        if (ImGui::ColorPicker4("##ColorPicker", col, pickerFlags)) {
            m_BaseColor = glm::vec4(col[0], col[1], col[2], col[3]);
            changed = true;
            m_ColorPickerActive = true;
        }

        // Save to color history when user releases the picker
        if (wasActive && !ImGui::IsItemActive()) {
            m_ColorPickerActive = false;
            // Add to history if different from last entry
            bool duplicate = false;
            if (m_ColorHistoryCount > 0) {
                auto& last = m_ColorHistory[(m_ColorHistoryCount - 1) % MAX_COLOR_HISTORY];
                if (glm::abs(last.r - m_BaseColor.r) < 0.01f &&
                    glm::abs(last.g - m_BaseColor.g) < 0.01f &&
                    glm::abs(last.b - m_BaseColor.b) < 0.01f) {
                    duplicate = true;
                }
            }
            if (!duplicate) {
                int idx = m_ColorHistoryCount % MAX_COLOR_HISTORY;
                m_ColorHistory[idx] = m_BaseColor;
                m_ColorHistoryCount++;
            }
        }
    } else {
        // Compact color edit
        if (ImGui::ColorEdit4("##BaseColor", col,
            ImGuiColorEditFlags_Float | ImGuiColorEditFlags_AlphaBar |
            ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_DisplayHex)) {
            m_BaseColor = glm::vec4(col[0], col[1], col[2], col[3]);
            changed = true;
        }
    }

    // === COLOR PALETTE SYSTEM ===
    ImGui::Spacing();

    // Hue spectrum bar - click anywhere for a hue
    {
        ImVec2 barPos = ImGui::GetCursorScreenPos();
        float barWidth = ImGui::GetContentRegionAvail().x;
        float barHeight = 16.0f;
        ImDrawList* dl = ImGui::GetWindowDrawList();

        // Draw rainbow gradient
        int segments = 32;
        float segW = barWidth / segments;
        for (int i = 0; i < segments; i++) {
            float h0 = (float)i / segments;
            float h1 = (float)(i + 1) / segments;
            float r0, g0, b0, r1, g1, b1;
            ImGui::ColorConvertHSVtoRGB(h0, 1.0f, 1.0f, r0, g0, b0);
            ImGui::ColorConvertHSVtoRGB(h1, 1.0f, 1.0f, r1, g1, b1);
            dl->AddRectFilledMultiColor(
                ImVec2(barPos.x + segW * i, barPos.y),
                ImVec2(barPos.x + segW * (i + 1), barPos.y + barHeight),
                IM_COL32(r0*255,g0*255,b0*255,255), IM_COL32(r1*255,g1*255,b1*255,255),
                IM_COL32(r1*255,g1*255,b1*255,255), IM_COL32(r0*255,g0*255,b0*255,255));
        }

        // Current hue indicator
        float curH, curS, curV;
        ImGui::ColorConvertRGBtoHSV(col[0], col[1], col[2], curH, curS, curV);
        float indicatorX = barPos.x + curH * barWidth;
        dl->AddTriangleFilled(
            ImVec2(indicatorX, barPos.y + barHeight),
            ImVec2(indicatorX - 4, barPos.y + barHeight + 5),
            ImVec2(indicatorX + 4, barPos.y + barHeight + 5),
            IM_COL32(255,255,255,255));

        // Invisible button for interaction
        ImGui::InvisibleButton("##HueBar", ImVec2(barWidth, barHeight + 6));
        if (ImGui::IsItemActive()) {
            float mouseX = ImGui::GetIO().MousePos.x - barPos.x;
            float newH = std::clamp(mouseX / barWidth, 0.0f, 1.0f);
            float r, g, b;
            ImGui::ColorConvertHSVtoRGB(newH, std::max(curS, 0.5f), std::max(curV, 0.5f), r, g, b);
            m_BaseColor = glm::vec4(r, g, b, m_BaseColor.a);
            col[0] = r; col[1] = g; col[2] = b;
            changed = true;
        }
    }

    ImGui::Spacing();

    // Category filter tabs
    const char* categories[] = { "All", "Basic", "Pastel", "Earth", "Neon", "Gray", "Skin", "Metal", "Nature" };
    for (int i = 0; i < 9; i++) {
        if (i > 0) ImGui::SameLine();
        bool selected = (m_PaletteCategory == i);
        if (selected) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.7f, 1.0f));
        if (ImGui::SmallButton(categories[i])) m_PaletteCategory = i;
        if (selected) ImGui::PopStyleColor();
    }

    ImGui::Spacing();

    // --- MEGA COLOR PALETTE ---
    // Basic (16)
    static const ImVec4 basicColors[] = {
        {1,1,1,1}, {0.85f,0.85f,0.85f,1}, {0.65f,0.65f,0.65f,1}, {0.45f,0.45f,0.45f,1},
        {0.25f,0.25f,0.25f,1}, {0.1f,0.1f,0.1f,1}, {0,0,0,1}, {1,0,0,1},
        {0,0.8f,0,1}, {0,0,1,1}, {1,1,0,1}, {1,0.5f,0,1},
        {0.5f,0,1,1}, {0,1,1,1}, {1,0,1,1}, {1,0.4f,0.6f,1},
    };
    // Pastel (16)
    static const ImVec4 pastelColors[] = {
        {1,0.82f,0.86f,1}, {1,0.87f,0.74f,1}, {1,0.97f,0.74f,1}, {0.82f,1,0.74f,1},
        {0.74f,1,0.89f,1}, {0.74f,0.92f,1,1}, {0.82f,0.74f,1,1}, {1,0.74f,1,1},
        {0.95f,0.77f,0.77f,1}, {0.95f,0.88f,0.77f,1}, {0.90f,0.95f,0.77f,1}, {0.77f,0.95f,0.82f,1},
        {0.77f,0.95f,0.95f,1}, {0.77f,0.82f,0.95f,1}, {0.88f,0.77f,0.95f,1}, {0.95f,0.77f,0.88f,1},
    };
    // Earth tones (16)
    static const ImVec4 earthColors[] = {
        {0.72f,0.53f,0.04f,1}, {0.55f,0.27f,0.07f,1}, {0.40f,0.26f,0.13f,1}, {0.82f,0.71f,0.55f,1},
        {0.76f,0.60f,0.42f,1}, {0.65f,0.50f,0.39f,1}, {0.87f,0.72f,0.53f,1}, {0.72f,0.60f,0.50f,1},
        {0.44f,0.37f,0.22f,1}, {0.33f,0.42f,0.18f,1}, {0.55f,0.47f,0.14f,1}, {0.42f,0.32f,0.28f,1},
        {0.86f,0.80f,0.65f,1}, {0.60f,0.46f,0.33f,1}, {0.70f,0.60f,0.33f,1}, {0.50f,0.40f,0.30f,1},
    };
    // Neon (12)
    static const ImVec4 neonColors[] = {
        {1,0,0.4f,1}, {1,0.2f,0,1}, {0.9f,1,0,1}, {0,1,0.2f,1},
        {0,1,0.8f,1}, {0,0.6f,1,1}, {0.4f,0,1,1}, {1,0,0.8f,1},
        {1,0.6f,0,1}, {0.5f,1,0,1}, {0,0.8f,1,1}, {0.8f,0,1,1},
    };
    // Grayscale (12)
    static const ImVec4 grayColors[] = {
        {1,1,1,1}, {0.93f,0.93f,0.93f,1}, {0.85f,0.85f,0.85f,1}, {0.75f,0.75f,0.75f,1},
        {0.65f,0.65f,0.65f,1}, {0.55f,0.55f,0.55f,1}, {0.45f,0.45f,0.45f,1}, {0.35f,0.35f,0.35f,1},
        {0.25f,0.25f,0.25f,1}, {0.15f,0.15f,0.15f,1}, {0.07f,0.07f,0.07f,1}, {0,0,0,1},
    };
    // Skin tones (12)
    static const ImVec4 skinColors[] = {
        {0.99f,0.93f,0.88f,1}, {0.97f,0.87f,0.78f,1}, {0.95f,0.80f,0.68f,1}, {0.92f,0.73f,0.58f,1},
        {0.87f,0.65f,0.47f,1}, {0.80f,0.58f,0.38f,1}, {0.72f,0.49f,0.30f,1}, {0.62f,0.40f,0.24f,1},
        {0.52f,0.33f,0.19f,1}, {0.43f,0.27f,0.15f,1}, {0.35f,0.22f,0.12f,1}, {0.26f,0.17f,0.10f,1},
    };
    // Metallic reference (12)
    static const ImVec4 metalColors[] = {
        {1.0f,0.766f,0.336f,1}, {0.972f,0.960f,0.915f,1}, {0.955f,0.637f,0.538f,1}, {0.56f,0.57f,0.58f,1},
        {0.913f,0.921f,0.925f,1}, {0.549f,0.556f,0.554f,1}, {0.66f,0.66f,0.63f,1}, {0.78f,0.57f,0.11f,1},
        {0.90f,0.86f,0.80f,1}, {0.68f,0.70f,0.72f,1}, {0.83f,0.69f,0.22f,1}, {0.34f,0.34f,0.34f,1},
    };
    // Nature (16)
    static const ImVec4 natureColors[] = {
        {0.13f,0.55f,0.13f,1}, {0.0f,0.39f,0.0f,1}, {0.18f,0.31f,0.18f,1}, {0.42f,0.56f,0.14f,1},
        {0.56f,0.74f,0.56f,1}, {0.53f,0.81f,0.92f,1}, {0.0f,0.75f,1.0f,1}, {0.12f,0.56f,1.0f,1},
        {0.0f,0.0f,0.55f,1}, {0.28f,0.24f,0.55f,1}, {0.94f,0.90f,0.55f,1}, {0.98f,0.50f,0.45f,1},
        {0.50f,0.0f,0.0f,1}, {0.85f,0.44f,0.84f,1}, {0.60f,0.80f,0.20f,1}, {0.30f,0.69f,0.31f,1},
    };

    // Collect pointers and sizes based on category filter
    struct PaletteGroup { const ImVec4* colors; int count; const char* name; };
    PaletteGroup allGroups[] = {
        { basicColors,  16, "Basic" },
        { pastelColors, 16, "Pastel" },
        { earthColors,  16, "Earth Tones" },
        { neonColors,   12, "Neon" },
        { grayColors,   12, "Grayscale" },
        { skinColors,   12, "Skin Tones" },
        { metalColors,  12, "Metallic" },
        { natureColors, 16, "Nature" },
    };

    float swatchSize = 22.0f;
    float availWidth = ImGui::GetContentRegionAvail().x;
    int colsPerRow = std::max(1, (int)(availWidth / (swatchSize + 3.0f)));

    // Helper lambda for drawing a swatch grid
    auto drawSwatches = [&](const ImVec4* colors, int count, int idBase) {
        for (int i = 0; i < count; i++) {
            if (i > 0 && (i % colsPerRow) != 0) ImGui::SameLine(0, 1);
            ImGui::PushID(idBase + i);
            ImGui::PushStyleColor(ImGuiCol_Button, colors[i]);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                ImVec4(std::min(colors[i].x*1.2f,1.0f), std::min(colors[i].y*1.2f,1.0f),
                       std::min(colors[i].z*1.2f,1.0f), 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, colors[i]);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);

            if (ImGui::Button("##s", ImVec2(swatchSize, swatchSize))) {
                m_BaseColor = glm::vec4(colors[i].x, colors[i].y, colors[i].z, m_BaseColor.a);
                changed = true;
            }

            // Right-click to add to favorites
            if (ImGui::IsItemClicked(ImGuiMouseButton_Right) && m_FavoriteCount < MAX_FAVORITES) {
                m_FavoriteColors[m_FavoriteCount++] = glm::vec4(colors[i].x, colors[i].y, colors[i].z, 1.0f);
            }

            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::ColorButton("##tt", colors[i], 0, ImVec2(40,40));
                ImGui::SameLine();
                ImGui::Text("R:%.0f G:%.0f B:%.0f\n#%02X%02X%02X\nRight-click: Add to favorites",
                    colors[i].x*255, colors[i].y*255, colors[i].z*255,
                    (int)(colors[i].x*255), (int)(colors[i].y*255), (int)(colors[i].z*255));
                ImGui::EndTooltip();
            }

            ImGui::PopStyleVar();
            ImGui::PopStyleColor(3);
            ImGui::PopID();
        }
    };

    // Draw palette based on selected category
    if (m_PaletteCategory == 0) {
        // Show all groups with headers
        int idBase = 1000;
        for (int g = 0; g < 8; g++) {
            ImGui::TextDisabled("%s", allGroups[g].name);
            drawSwatches(allGroups[g].colors, allGroups[g].count, idBase);
            idBase += allGroups[g].count;
            ImGui::Spacing();
        }
    } else {
        int groupIdx = m_PaletteCategory - 1;
        if (groupIdx >= 0 && groupIdx < 8) {
            drawSwatches(allGroups[groupIdx].colors, allGroups[groupIdx].count, 1000);
        }
    }

    // === COLOR HARMONY ===
    ImGui::Spacing();
    ImGui::TextDisabled("Harmony:");
    ImGui::SameLine();
    const char* harmonyModes[] = { "None", "Complement", "Analogous", "Triadic", "Split" };
    ImGui::SetNextItemWidth(100);
    ImGui::Combo("##Harmony", &m_HarmonyMode, harmonyModes, 5);

    if (m_HarmonyMode > 0) {
        float hh, ss, vv;
        ImGui::ColorConvertRGBtoHSV(col[0], col[1], col[2], hh, ss, vv);

        ImVec4 harmonies[4];
        int harmonyCount = 0;

        if (m_HarmonyMode == 1) { // Complementary
            float r,g,b;
            ImGui::ColorConvertHSVtoRGB(fmodf(hh+0.5f,1.0f), ss, vv, r, g, b);
            harmonies[harmonyCount++] = ImVec4(r,g,b,1);
        } else if (m_HarmonyMode == 2) { // Analogous
            for (int i = 0; i < 2; i++) {
                float offset = (i == 0) ? -0.083f : 0.083f;
                float r,g,b;
                ImGui::ColorConvertHSVtoRGB(fmodf(hh+offset+1.0f,1.0f), ss, vv, r, g, b);
                harmonies[harmonyCount++] = ImVec4(r,g,b,1);
            }
        } else if (m_HarmonyMode == 3) { // Triadic
            for (int i = 0; i < 2; i++) {
                float offset = (i == 0) ? 0.333f : 0.667f;
                float r,g,b;
                ImGui::ColorConvertHSVtoRGB(fmodf(hh+offset,1.0f), ss, vv, r, g, b);
                harmonies[harmonyCount++] = ImVec4(r,g,b,1);
            }
        } else if (m_HarmonyMode == 4) { // Split-Complementary
            for (int i = 0; i < 2; i++) {
                float offset = (i == 0) ? 0.417f : 0.583f;
                float r,g,b;
                ImGui::ColorConvertHSVtoRGB(fmodf(hh+offset,1.0f), ss, vv, r, g, b);
                harmonies[harmonyCount++] = ImVec4(r,g,b,1);
            }
        }

        ImGui::SameLine();
        for (int i = 0; i < harmonyCount; i++) {
            ImGui::PushID(5000 + i);
            ImGui::PushStyleColor(ImGuiCol_Button, harmonies[i]);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, harmonies[i]);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, harmonies[i]);
            if (ImGui::Button("##harm", ImVec2(28, 20))) {
                m_BaseColor = glm::vec4(harmonies[i].x, harmonies[i].y, harmonies[i].z, m_BaseColor.a);
                changed = true;
            }
            ImGui::PopStyleColor(3);
            ImGui::PopID();
            if (i < harmonyCount - 1) ImGui::SameLine(0, 2);
        }
    }

    // === FAVORITES ===
    if (m_FavoriteCount > 0) {
        ImGui::Spacing();
        ImGui::TextDisabled("Favorites (right-click swatch to add):");
        for (int i = 0; i < m_FavoriteCount; i++) {
            if (i > 0 && (i % colsPerRow) != 0) ImGui::SameLine(0, 1);
            ImGui::PushID(3000 + i);
            ImVec4 fc(m_FavoriteColors[i].r, m_FavoriteColors[i].g, m_FavoriteColors[i].b, 1.0f);
            ImGui::PushStyleColor(ImGuiCol_Button, fc);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                ImVec4(std::min(fc.x*1.2f,1.0f), std::min(fc.y*1.2f,1.0f), std::min(fc.z*1.2f,1.0f), 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, fc);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
            if (ImGui::Button("##fav", ImVec2(swatchSize, swatchSize))) {
                m_BaseColor = glm::vec4(fc.x, fc.y, fc.z, m_BaseColor.a);
                changed = true;
            }
            if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
                // Remove from favorites
                for (int j = i; j < m_FavoriteCount - 1; j++)
                    m_FavoriteColors[j] = m_FavoriteColors[j+1];
                m_FavoriteCount--;
            }
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("Click: apply | Right-click: remove");
                ImGui::EndTooltip();
            }
            ImGui::PopStyleVar();
            ImGui::PopStyleColor(3);
            ImGui::PopID();
        }
    }

    // === RECENT HISTORY ===
    if (m_ColorHistoryCount > 0) {
        ImGui::Spacing();
        ImGui::TextDisabled("Recent:");
        int histCount = std::min(m_ColorHistoryCount, (int)MAX_COLOR_HISTORY);
        for (int i = 0; i < histCount; i++) {
            int idx = (m_ColorHistoryCount - 1 - i) % MAX_COLOR_HISTORY;
            if (idx < 0) idx += MAX_COLOR_HISTORY;
            if (i > 0 && (i % colsPerRow) != 0) ImGui::SameLine(0, 1);
            ImGui::PushID(2000 + i);
            ImVec4 hc(m_ColorHistory[idx].r, m_ColorHistory[idx].g, m_ColorHistory[idx].b, m_ColorHistory[idx].a);
            ImGui::PushStyleColor(ImGuiCol_Button, hc);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                ImVec4(std::min(hc.x*1.2f,1.0f), std::min(hc.y*1.2f,1.0f), std::min(hc.z*1.2f,1.0f), 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, hc);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
            if (ImGui::Button("##hist", ImVec2(swatchSize, swatchSize))) {
                m_BaseColor = m_ColorHistory[idx];
                changed = true;
            }
            ImGui::PopStyleVar();
            ImGui::PopStyleColor(3);
            ImGui::PopID();
        }
    }

    ImGui::Spacing();
    SectionHeader("PBR Properties");

    // Metallic
    ImGui::Text("Metallic");
    ImGui::SameLine(ImGui::GetWindowWidth() - 80);
    ImGui::TextDisabled("%.0f%%", m_Metallic * 100);
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.8f, 0.7f, 0.3f, 1.0f));
    if (ImGui::SliderFloat("##Metallic", &m_Metallic, 0.0f, 1.0f, "")) {
        changed = true;
    }
    ImGui::PopStyleColor();

    // Roughness
    ImGui::Text("Roughness");
    ImGui::SameLine(ImGui::GetWindowWidth() - 80);
    ImGui::TextDisabled("%.0f%%", m_Roughness * 100);
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
    if (ImGui::SliderFloat("##Roughness", &m_Roughness, 0.0f, 1.0f, "")) {
        changed = true;
    }
    ImGui::PopStyleColor();

    // Quick presets
    ImGui::Spacing();
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
    if (ImGui::ColorEdit3("##EmissionColor", emCol,
        ImGuiColorEditFlags_Float | ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_DisplayHex)) {
        m_Emission = glm::vec3(emCol[0], emCol[1], emCol[2]);
        changed = true;
    }

    // Emission strength with logarithmic control
    ImGui::Text("Strength");
    ImGui::SameLine(ImGui::GetWindowWidth() - 80);
    ImGui::TextDisabled("%.2f", m_EmissionStrength);
    if (ImGui::SliderFloat("##EmissionStrength", &m_EmissionStrength, 0.0f, 10.0f, "%.2f", ImGuiSliderFlags_Logarithmic)) {
        changed = true;
    }

    // Emission quick presets
    ImGui::TextDisabled("Quick:");
    if (ImGui::Button("Off##em", ImVec2(40, 0))) { m_EmissionStrength = 0.0f; changed = true; }
    ImGui::SameLine();
    if (ImGui::Button("Dim##em", ImVec2(40, 0))) { m_EmissionStrength = 0.5f; changed = true; }
    ImGui::SameLine();
    if (ImGui::Button("Glow##em", ImVec2(40, 0))) { m_EmissionStrength = 2.0f; changed = true; }
    ImGui::SameLine();
    if (ImGui::Button("Bright##em", ImVec2(50, 0))) { m_EmissionStrength = 5.0f; changed = true; }
    ImGui::SameLine();
    if (ImGui::Button("Max##em", ImVec2(40, 0))) { m_EmissionStrength = 10.0f; changed = true; }

    // Apply changes
    if (changed) {
        ApplyToSelection();

        // Auto-apply to GLTF material buffer in real-time
        if (m_SelectionType == MaterialSelectionType::GLTFMaterial) {
            ApplyToGLTFMaterial();
        }
    }

    // Material File section
    ImGui::Spacing();
    ImGui::Separator();
    SectionHeader("Material File");

    // Show current material name
    ImGui::Text("Name:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-1);
    if (ImGui::InputText("##MaterialName", m_MaterialNameBuffer, sizeof(m_MaterialNameBuffer))) {
        m_MaterialName = m_MaterialNameBuffer;
    }

    // Show current file path if any
    if (!m_CurrentMaterialPath.empty()) {
        ImGui::TextDisabled("File: %s", fs::path(m_CurrentMaterialPath).filename().string().c_str());

        // Quick save button
        if (ImGui::Button("Quick Save", ImVec2(-1, 0))) {
            SaveMaterialToFile();
        }
    }

    // Save/Load buttons
    ImGui::Spacing();
    float buttonWidth = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) / 2;
    if (ImGui::Button("Save As...", ImVec2(buttonWidth, 0))) {
        m_CurrentMaterialPath.clear();  // Force dialog
        m_IsLoadDialog = false;
        m_ShowSaveLoadDialog = true;
        strncpy(m_MaterialNameBuffer, m_MaterialName.c_str(), sizeof(m_MaterialNameBuffer) - 1);
    }
    ImGui::SameLine();
    if (ImGui::Button("Load...", ImVec2(buttonWidth, 0))) {
        m_IsLoadDialog = true;
        m_ShowSaveLoadDialog = true;
    }
}

void MaterialView::ApplyToSelection() {
    if (!m_Engine) return;

    if (m_SelectionType == MaterialSelectionType::Primitive) {
        if (m_Engine->selectedPrimitiveIndex >= 0 &&
            m_Engine->selectedPrimitiveIndex < static_cast<int>(m_Engine->static_shapes.size())) {
            auto& shape = m_Engine->static_shapes[m_Engine->selectedPrimitiveIndex];

            // Update all material properties for primitives
            shape.mainColor = m_BaseColor;
            shape.metallic = m_Metallic;
            shape.roughness = m_Roughness;
            shape.emission = m_Emission * m_EmissionStrength;

            // Update pass type based on alpha
            if (m_BaseColor.a < 1.0f) {
                shape.passType = MaterialPass::Transparent;
            } else {
                shape.passType = MaterialPass::MainColor;
            }
        }
    }
    // For GLTF, we apply on button click via ApplyToGLTFMaterial()
}

void MaterialView::ApplyToGLTFMaterial() {
    if (!m_Engine || !m_SelectedMeshNode || !m_SelectedMeshNode->mesh) return;

    auto& surfaces = m_SelectedMeshNode->mesh->surfaces;
    if (m_SelectedSurfaceIndex >= static_cast<int>(surfaces.size())) return;

    // Find the LoadedGLTF that owns this mesh
    for (auto& [sceneName, scene] : m_Engine->loadedScenes) {
        if (!scene) continue;

        // Check if this scene contains our mesh
        bool found = false;
        for (auto& [nodeName, node] : scene->nodes) {
            if (node.get() == m_SelectedMeshNode) {
                found = true;
                break;
            }
        }

        if (found && scene->materialDataBuffer.buffer != VK_NULL_HANDLE) {
            // Update the material constants in the buffer
            UpdateGLTFMaterialBuffer();
            return;
        }
    }
}

void MaterialView::UpdateGLTFMaterialBuffer() {
    if (!m_Engine || !m_SelectedMeshNode || !m_SelectedMeshNode->mesh) return;

    auto& surfaces = m_SelectedMeshNode->mesh->surfaces;
    if (m_SelectedSurfaceIndex >= static_cast<int>(surfaces.size())) return;

    auto& surface = surfaces[m_SelectedSurfaceIndex];
    if (!surface.material) return;

    // Get the buffer offset stored in the material
    uint32_t bufferOffset = surface.material->bufferOffset;

    // Find the scene that owns this material to get the buffer
    for (auto& [sceneName, scene] : m_Engine->loadedScenes) {
        if (!scene) continue;

        // Check if this scene contains the material
        bool found = false;
        for (auto& [matName, mat] : scene->materials) {
            if (mat == surface.material) {
                found = true;
                break;
            }
        }
        if (!found) continue;

        // Map the buffer and update the material constants
        if (scene->materialDataBuffer.buffer != VK_NULL_HANDLE &&
            scene->materialDataBuffer.allocation != VK_NULL_HANDLE) {

            void* data = nullptr;
            VkResult result = vmaMapMemory(m_Engine->_allocator, scene->materialDataBuffer.allocation, &data);

            if (result == VK_SUCCESS && data) {
                // Use the stored buffer offset
                GLTFMetallic_Roughness::MaterialConstants* constants =
                    reinterpret_cast<GLTFMetallic_Roughness::MaterialConstants*>(
                        static_cast<char*>(data) + bufferOffset);

                // Update the constants
                constants->colorFactors = m_BaseColor;
                constants->metal_rough_factors = glm::vec4(m_Metallic, m_Roughness, m_AO, m_NormalStrength);

                // Store emission in extra[0] (x,y,z = color, w = strength)
                constants->extra[0] = glm::vec4(m_Emission, m_EmissionStrength);

                vmaUnmapMemory(m_Engine->_allocator, scene->materialDataBuffer.allocation);

                // Flush to ensure GPU sees the changes
                vmaFlushAllocation(m_Engine->_allocator, scene->materialDataBuffer.allocation, bufferOffset,
                    sizeof(GLTFMetallic_Roughness::MaterialConstants));

                fmt::print("[MaterialView] Updated GLTF material - Base: ({:.2f},{:.2f},{:.2f},{:.2f}), Metal: {:.2f}, Rough: {:.2f}, Emission: ({:.2f},{:.2f},{:.2f}) x {:.2f}\n",
                    m_BaseColor.r, m_BaseColor.g, m_BaseColor.b, m_BaseColor.a,
                    m_Metallic, m_Roughness,
                    m_Emission.r, m_Emission.g, m_Emission.b, m_EmissionStrength);
            }
            return;
        }
    }
}

void MaterialView::RenderTextureSlots() {
    if (m_SelectionType == MaterialSelectionType::GLTFMaterial) {
        ImGui::TextWrapped("Drag textures from Asset Browser to change GLTF material textures.");
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f), "Note: Texture changes require material rebuild.");
    } else {
        ImGui::TextWrapped("Drag textures from Asset Browser to assign them to slots.");
    }

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
        m_AlbedoSlot = {"Albedo", "", false, 0};
        m_NormalSlot = {"Normal", "", false, 0};
        m_MetallicRoughnessSlot = {"Metallic/Roughness", "", false, 0};
        m_AOSlot = {"Ambient Occlusion", "", false, 0};
        m_EmissionSlot = {"Emission", "", false, 0};
    }

    // Apply textures button for GLTF
    if (m_SelectionType == MaterialSelectionType::GLTFMaterial) {
        bool hasTextures = m_AlbedoSlot.isLoaded || m_MetallicRoughnessSlot.isLoaded ||
                           m_NormalSlot.isLoaded || m_EmissionSlot.isLoaded;

        if (hasTextures) {
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.3f, 1.0f));

            if (ImGui::Button("Apply Textures to Material", ImVec2(-1, 35))) {
                // Load albedo texture
                if (m_AlbedoSlot.isLoaded && !m_AlbedoSlot.path.empty()) {
                    uint32_t texID = 0;
                    if (LoadTextureFromFile(m_AlbedoSlot.path, texID)) {
                        m_AlbedoSlot.textureID = texID;
                    }
                }

                // Load metallic/roughness texture
                if (m_MetallicRoughnessSlot.isLoaded && !m_MetallicRoughnessSlot.path.empty()) {
                    uint32_t texID = 0;
                    if (LoadTextureFromFile(m_MetallicRoughnessSlot.path, texID)) {
                        m_MetallicRoughnessSlot.textureID = texID;
                    }
                }

                // Load normal texture
                if (m_NormalSlot.isLoaded && !m_NormalSlot.path.empty()) {
                    uint32_t texID = 0;
                    if (LoadTextureFromFile(m_NormalSlot.path, texID)) {
                        m_NormalSlot.textureID = texID;
                    }
                }

                // Update material with new texture IDs
                RebuildMaterialDescriptorSet();
            }

            ImGui::PopStyleColor();

            ImGui::TextDisabled("Updates GPU material buffer in real-time");
        }
    }

    // Apply textures button for PRIMITIVES
    if (m_SelectionType == MaterialSelectionType::Primitive) {
        bool hasTextures = m_AlbedoSlot.isLoaded || m_MetallicRoughnessSlot.isLoaded;

        if (hasTextures) {
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.2f, 1.0f));

            if (ImGui::Button("Apply Textures to Primitive", ImVec2(-1, 35))) {
                ApplyTexturesToPrimitive();
            }

            ImGui::PopStyleColor();

            ImGui::TextDisabled("Creates a new material for this primitive");
        }
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

    if (ImGui::Button(buttonLabel.c_str(), ImVec2(-30, 40))) {
        // Click to clear
        if (slot.isLoaded) {
            slot.path.clear();
            slot.isLoaded = false;
            slot.textureID = 0;
        }
    }

    // Drag-drop target
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(payloadType)) {
            const char* droppedPath = static_cast<const char*>(payload->Data);
            slot.path = droppedPath;

            // Immediately try to load the texture for preview
            uint32_t texID = 0;
            if (LoadTextureFromFile(droppedPath, texID)) {
                slot.textureID = texID;
                slot.isLoaded = true;
            } else {
                slot.isLoaded = true;  // Mark as loaded even if GPU load fails (path is valid)
            }
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

    // Clear button
    ImGui::SameLine();
    if (ImGui::Button("X", ImVec2(25, 40))) {
        slot.path.clear();
        slot.isLoaded = false;
        slot.textureID = 0;
    }

    ImGui::PopID();
    ImGui::Spacing();
}

bool MaterialView::LoadTextureFromFile(const std::string& texturePath, uint32_t& outTextureID) {
    if (!m_Engine) return false;

    // Check if texture already loaded in cache
    auto& cache = m_Engine->texCache;
    auto it = cache.NameMap.find(texturePath);
    if (it != cache.NameMap.end()) {
        outTextureID = it->second.Index;
        return true;
    }

    // Load image from file using stb_image
    int width, height, channels;
    unsigned char* data = stbi_load(texturePath.c_str(), &width, &height, &channels, 4);
    if (!data) {
        fmt::print("Failed to load texture: {}\n", texturePath);
        return false;
    }

    // Create Vulkan image
    VkExtent3D imageSize{};
    imageSize.width = static_cast<uint32_t>(width);
    imageSize.height = static_cast<uint32_t>(height);
    imageSize.depth = 1;

    AllocatedImage newImage = m_Engine->create_image(
        data, imageSize,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_USAGE_SAMPLED_BIT,
        false  // No mipmaps for now
    );

    stbi_image_free(data);

    if (newImage.image == VK_NULL_HANDLE) {
        fmt::print("Failed to create Vulkan image for: {}\n", texturePath);
        return false;
    }

    // Add to texture cache
    TextureID texID = cache.AddTexture(
        newImage.imageView,
        m_Engine->_defaultSamplerLinear,
        texturePath
    );

    outTextureID = texID.Index;
    fmt::print("Loaded texture: {} -> ID {}\n", texturePath, outTextureID);
    return true;
}

void MaterialView::UpdateMaterialTexture(uint32_t colorTexID, uint32_t metalRoughTexID) {
    if (!m_Engine || !m_SelectedMeshNode || !m_SelectedMeshNode->mesh) return;

    auto& surfaces = m_SelectedMeshNode->mesh->surfaces;
    if (m_SelectedSurfaceIndex >= static_cast<int>(surfaces.size())) return;

    auto& surface = surfaces[m_SelectedSurfaceIndex];
    if (!surface.material) return;

    uint32_t bufferOffset = surface.material->bufferOffset;

    // Find the scene and update texture IDs in the material buffer
    for (auto& [sceneName, scene] : m_Engine->loadedScenes) {
        if (!scene) continue;

        bool found = false;
        for (auto& [matName, mat] : scene->materials) {
            if (mat == surface.material) {
                found = true;
                break;
            }
        }
        if (!found) continue;

        if (scene->materialDataBuffer.buffer != VK_NULL_HANDLE &&
            scene->materialDataBuffer.allocation != VK_NULL_HANDLE) {

            void* data = nullptr;
            VkResult result = vmaMapMemory(m_Engine->_allocator, scene->materialDataBuffer.allocation, &data);

            if (result == VK_SUCCESS && data) {
                GLTFMetallic_Roughness::MaterialConstants* constants =
                    reinterpret_cast<GLTFMetallic_Roughness::MaterialConstants*>(
                        static_cast<char*>(data) + bufferOffset);

                // Update texture IDs
                constants->colorTexID = colorTexID;
                constants->metalRoughTexID = metalRoughTexID;

                vmaUnmapMemory(m_Engine->_allocator, scene->materialDataBuffer.allocation);

                vmaFlushAllocation(m_Engine->_allocator, scene->materialDataBuffer.allocation, bufferOffset,
                    sizeof(GLTFMetallic_Roughness::MaterialConstants));

                fmt::print("Updated material texture IDs: color={}, metalRough={}\n", colorTexID, metalRoughTexID);
            }
            return;
        }
    }
}

void MaterialView::RebuildMaterialDescriptorSet() {
    // This function would rebuild the material's descriptor set with new textures
    // For now, this requires the texture IDs to be set in the MaterialConstants buffer
    // which the shader reads via bindless texturing

    if (!m_Engine || !m_SelectedMeshNode || !m_SelectedMeshNode->mesh) return;

    auto& surfaces = m_SelectedMeshNode->mesh->surfaces;
    if (m_SelectedSurfaceIndex >= static_cast<int>(surfaces.size())) return;

    // Note: In a full implementation, you would:
    // 1. Wait for GPU idle on this material's descriptor set
    // 2. Destroy old descriptor set
    // 3. Allocate new descriptor set
    // 4. Write new texture bindings
    // 5. Update the material's data.materialSet

    // For bindless texture approach, we just update the texture IDs in the buffer
    // The shader uses these IDs to index into a global texture array

    uint32_t colorTexID = m_AlbedoSlot.isLoaded ? m_AlbedoSlot.textureID : 0;
    uint32_t metalRoughTexID = m_MetallicRoughnessSlot.isLoaded ? m_MetallicRoughnessSlot.textureID : 0;

    UpdateMaterialTexture(colorTexID, metalRoughTexID);
}

void MaterialView::ApplyTexturesToPrimitive() {
    if (!m_Engine) return;
    if (m_Engine->selectedPrimitiveIndex < 0 ||
        m_Engine->selectedPrimitiveIndex >= static_cast<int>(m_Engine->static_shapes.size())) {
        return;
    }

    auto& shape = m_Engine->static_shapes[m_Engine->selectedPrimitiveIndex];

    // Create a new material for this primitive using the engine's create_primitive_material
    std::string albedoPath = m_AlbedoSlot.isLoaded ? m_AlbedoSlot.path : "";
    std::string metalRoughPath = m_MetallicRoughnessSlot.isLoaded ? m_MetallicRoughnessSlot.path : "";
    std::string emissionPath = m_EmissionSlot.isLoaded ? m_EmissionSlot.path : "";

    MaterialInstance newMaterial = m_Engine->create_primitive_material(albedoPath, metalRoughPath, emissionPath);

    // Create a shared_ptr and assign to the shape
    shape.material = std::make_shared<MaterialInstance>(newMaterial);

    // Store texture paths for serialization
    shape.albedoTexturePath = albedoPath;
    shape.metalRoughTexturePath = metalRoughPath;
    shape.emissionTexturePath = emissionPath;

    fmt::print("[MaterialView] Applied textures to primitive '{}'\n", shape.name);
    fmt::print("  Albedo: {}\n", albedoPath.empty() ? "[default white]" : albedoPath);
    fmt::print("  MetalRough: {}\n", metalRoughPath.empty() ? "[default white]" : metalRoughPath);
}

void MaterialView::RenderPreview() {
    SectionHeader("Material Preview");

    // Preview mesh selector
    const char* meshes[] = { "Sphere", "Cube", "Plane", "Cylinder", "Torus" };
    ImGui::SetNextItemWidth(150);
    ImGui::Combo("Preview Mesh", &m_PreviewMesh, meshes, 5);

    ImGui::SameLine();
    ImGui::Checkbox("Auto-rotate", &m_AutoRotate);

    // Preview area
    ImVec2 size = ImGui::GetContentRegionAvail();
    float previewSize = std::min(size.x - 10, 300.0f);
    previewSize = std::max(previewSize, 150.0f);

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
                ImVec2(center.x, center.y - radius * 0.5f),
                ImVec2(center.x + radius, center.y),
                ImVec2(center.x, center.y + radius * 0.5f),
                ImVec2(center.x - radius, center.y),
                matColor);
            break;
        default:
            drawList->AddCircleFilled(center, radius, matColor);
            break;
    }

    // Specular highlight
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
    ImGui::Spacing();
    ImGui::TextDisabled("Metallic: %.0f%% | Roughness: %.0f%%",
        m_Metallic * 100, m_Roughness * 100);

    if (m_SelectionType == MaterialSelectionType::GLTFMaterial) {
        ImGui::TextDisabled("Type: GLTF Material");
    } else if (m_SelectionType == MaterialSelectionType::Primitive) {
        ImGui::TextDisabled("Type: Primitive");
    }
}

void MaterialView::RenderPresets() {
    ImGui::TextDisabled("Click a preset to apply it");
    ImGui::Spacing();

    // Group presets by category
    const char* categories[] = { "Metals", "Plastics", "Natural", "Special", "Ceramic & Fabric", "Emissive" };
    int categoryStarts[] = { 0, 6, 11, 15, 18, 23 };
    int categoryEnds[] = { 6, 11, 15, 18, 23, (int)m_Presets.size() };

    for (int cat = 0; cat < 6; ++cat) {
        if (ImGui::CollapsingHeader(categories[cat], ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Indent();

            for (int i = categoryStarts[cat]; i < categoryEnds[cat] && i < (int)m_Presets.size(); ++i) {
                const auto& preset = m_Presets[i];

                ImGui::PushID(i);

                // Color preview
                ImVec4 previewColor(preset.baseColor.r, preset.baseColor.g, preset.baseColor.b, 1.0f);
                ImGui::PushStyleColor(ImGuiCol_Button, previewColor);

                if (ImGui::Button("##Color", ImVec2(25, 25))) {
                    // Apply preset
                    m_BaseColor = preset.baseColor;
                    m_Metallic = preset.metallic;
                    m_Roughness = preset.roughness;
                    m_AO = preset.ao;
                    m_Emission = preset.emission;
                    m_EmissionStrength = glm::length(preset.emission) > 0 ? 2.0f : 0.0f;
                    m_SelectedPreset = i;

                    ApplyToSelection();

                    // For GLTF, also update the buffer
                    if (m_SelectionType == MaterialSelectionType::GLTFMaterial) {
                        ApplyToGLTFMaterial();
                    }
                }

                ImGui::PopStyleColor();

                ImGui::SameLine();
                bool isSelected = (m_SelectedPreset == i);
                if (isSelected) {
                    ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "%s", preset.name.c_str());
                } else {
                    ImGui::Text("%s", preset.name.c_str());
                }

                // Tooltip
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

void MaterialView::ClearGLTFSelection() {
    m_SelectionType = MaterialSelectionType::None;
    m_SelectedMeshNode = nullptr;
    m_SelectedSurfaceIndex = 0;
    m_CurrentGLTFMaterial = nullptr;
    m_LastSelectedMeshNode = nullptr;
}

void MaterialView::OnSceneUnloading(const std::string& sceneName) {
    if (!m_Engine || !m_SelectedMeshNode) return;

    // Check if our selected node belongs to the scene being unloaded
    auto it = m_Engine->loadedScenes.find(sceneName);
    if (it != m_Engine->loadedScenes.end() && it->second) {
        for (const auto& [name, node] : it->second->nodes) {
            if (node.get() == m_SelectedMeshNode) {
                // Our selection is being destroyed - clear it
                ClearGLTFSelection();
                return;
            }
        }
    }
}

// =============================================================================
// MATERIAL FILE SAVE/LOAD
// =============================================================================

void MaterialView::RenderSaveLoadUI() {
    if (!m_ShowSaveLoadDialog) return;

    const char* title = m_IsLoadDialog ? "Load Material###MaterialFileDialog" : "Save Material###MaterialFileDialog";

    ImGui::SetNextWindowSize(ImVec2(500, 220), ImGuiCond_FirstUseEver);
    if (ImGui::Begin(title, &m_ShowSaveLoadDialog, ImGuiWindowFlags_NoCollapse)) {

        if (m_IsLoadDialog) {
            // Load dialog
            ImGui::TextWrapped("Load a material file (.mat) to apply its properties to the current selection.");
            ImGui::Spacing();

            ImGui::Text("File Path:");
            ImGui::SetNextItemWidth(-1);
            ImGui::InputText("##FilePath", m_FilePathBuffer, sizeof(m_FilePathBuffer));

            ImGui::TextDisabled("Enter path to .mat file (e.g., assets/materials/metal.mat)");

            // Show validation
            fs::path filePath(m_FilePathBuffer);
            if (strlen(m_FilePathBuffer) > 0) {
                if (fs::exists(filePath)) {
                    ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.4f, 1.0f), "File exists");
                } else {
                    ImGui::TextColored(ImVec4(0.9f, 0.4f, 0.4f, 1.0f), "File not found");
                }
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            bool canLoad = strlen(m_FilePathBuffer) > 0 && fs::exists(filePath);
            if (!canLoad) ImGui::BeginDisabled();
            if (ImGui::Button("Load", ImVec2(100, 0))) {
                m_CurrentMaterialPath = m_FilePathBuffer;
                LoadMaterialFromFile();
                m_ShowSaveLoadDialog = false;
            }
            if (!canLoad) ImGui::EndDisabled();

            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(100, 0))) {
                m_ShowSaveLoadDialog = false;
            }
        } else {
            // Save dialog
            ImGui::TextWrapped("Save current material properties to a .mat file.");
            ImGui::Spacing();

            ImGui::Text("Material Name:");
            ImGui::SetNextItemWidth(-1);
            ImGui::InputText("##MatName", m_MaterialNameBuffer, sizeof(m_MaterialNameBuffer));

            ImGui::Text("File Path:");
            ImGui::SetNextItemWidth(-1);
            ImGui::InputText("##FilePath", m_FilePathBuffer, sizeof(m_FilePathBuffer));

            ImGui::TextDisabled("Tip: Use assets/materials/ folder for organization");

            // Show preview of the path
            if (strlen(m_FilePathBuffer) > 0) {
                fs::path filePath(m_FilePathBuffer);
                if (filePath.extension() != ".mat") {
                    ImGui::TextDisabled("Will save as: %s.mat", m_FilePathBuffer);
                }
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            bool canSave = strlen(m_FilePathBuffer) > 0 && strlen(m_MaterialNameBuffer) > 0;
            if (!canSave) ImGui::BeginDisabled();
            if (ImGui::Button("Save", ImVec2(100, 0))) {
                m_MaterialName = m_MaterialNameBuffer;
                m_CurrentMaterialPath = m_FilePathBuffer;
                SaveMaterialToFile();
                m_ShowSaveLoadDialog = false;
            }
            if (!canSave) ImGui::EndDisabled();

            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(100, 0))) {
                m_ShowSaveLoadDialog = false;
            }
        }
    }
    ImGui::End();
}

void MaterialView::SaveMaterialToFile() {
    if (m_CurrentMaterialPath.empty()) {
        // Open save dialog
        m_IsLoadDialog = false;
        m_ShowSaveLoadDialog = true;
        return;
    }

    // Ensure .mat extension
    fs::path path(m_CurrentMaterialPath);
    if (path.extension() != ".mat") {
        m_CurrentMaterialPath += ".mat";
    }

    MaterialFile::MaterialData data = GetCurrentMaterialData();

    if (MaterialFile::save(m_CurrentMaterialPath, data)) {
        fmt::print("[MaterialView] Saved material to: {}\n", m_CurrentMaterialPath);
    } else {
        fmt::print("[MaterialView] Failed to save material to: {}\n", m_CurrentMaterialPath);
    }
}

void MaterialView::LoadMaterialFromFile() {
    if (m_CurrentMaterialPath.empty()) {
        // Open load dialog
        m_IsLoadDialog = true;
        m_ShowSaveLoadDialog = true;
        return;
    }

    auto materialOpt = MaterialFile::load(m_CurrentMaterialPath);
    if (materialOpt.has_value()) {
        ApplyMaterialData(materialOpt.value());
        fmt::print("[MaterialView] Loaded material from: {}\n", m_CurrentMaterialPath);
    } else {
        fmt::print("[MaterialView] Failed to load material from: {}\n", m_CurrentMaterialPath);
    }
}

MaterialFile::MaterialData MaterialView::GetCurrentMaterialData() const {
    MaterialFile::MaterialData data;

    data.name = m_MaterialName;
    data.version = "1.0";

    // PBR properties
    data.baseColor = m_BaseColor;
    data.metallic = m_Metallic;
    data.roughness = m_Roughness;
    data.ao = m_AO;

    // Emission
    data.emissionColor = m_Emission;
    data.emissionStrength = m_EmissionStrength;

    // Texture paths
    if (m_AlbedoSlot.isLoaded) {
        data.albedoTexturePath = m_AlbedoSlot.path;
    }
    if (m_MetallicRoughnessSlot.isLoaded) {
        data.metallicRoughnessTexturePath = m_MetallicRoughnessSlot.path;
    }
    if (m_NormalSlot.isLoaded) {
        data.normalTexturePath = m_NormalSlot.path;
        data.normalScale = m_NormalStrength;
    }
    if (m_EmissionSlot.isLoaded) {
        data.emissionTexturePath = m_EmissionSlot.path;
    }

    // Metadata
    data.category = m_MaterialCategory;
    data.tags = m_MaterialTags;

    // Rendering options (default values for now)
    data.doubleSided = false;
    data.castShadows = true;
    data.receiveShadows = true;
    data.alphaMode = m_BaseColor.a < 1.0f ?
        MaterialFile::MaterialData::AlphaMode::Blend :
        MaterialFile::MaterialData::AlphaMode::Opaque;
    data.alphaCutoff = 0.5f;

    return data;
}

void MaterialView::ApplyMaterialData(const MaterialFile::MaterialData& data) {
    // Update material name
    m_MaterialName = data.name;
    strncpy(m_MaterialNameBuffer, data.name.c_str(), sizeof(m_MaterialNameBuffer) - 1);

    // PBR properties
    m_BaseColor = data.baseColor;
    m_Metallic = data.metallic;
    m_Roughness = data.roughness;
    m_AO = data.ao;

    // Emission
    m_Emission = data.emissionColor;
    m_EmissionStrength = data.emissionStrength;

    // Normal scale
    m_NormalStrength = data.normalScale;

    // Texture paths
    if (!data.albedoTexturePath.empty()) {
        m_AlbedoSlot.path = data.albedoTexturePath;
        m_AlbedoSlot.isLoaded = true;
        // Try to load the texture
        uint32_t texID = 0;
        if (LoadTextureFromFile(data.albedoTexturePath, texID)) {
            m_AlbedoSlot.textureID = texID;
        }
    }

    if (!data.metallicRoughnessTexturePath.empty()) {
        m_MetallicRoughnessSlot.path = data.metallicRoughnessTexturePath;
        m_MetallicRoughnessSlot.isLoaded = true;
        uint32_t texID = 0;
        if (LoadTextureFromFile(data.metallicRoughnessTexturePath, texID)) {
            m_MetallicRoughnessSlot.textureID = texID;
        }
    }

    if (!data.normalTexturePath.empty()) {
        m_NormalSlot.path = data.normalTexturePath;
        m_NormalSlot.isLoaded = true;
        uint32_t texID = 0;
        if (LoadTextureFromFile(data.normalTexturePath, texID)) {
            m_NormalSlot.textureID = texID;
        }
    }

    if (!data.emissionTexturePath.empty()) {
        m_EmissionSlot.path = data.emissionTexturePath;
        m_EmissionSlot.isLoaded = true;
        uint32_t texID = 0;
        if (LoadTextureFromFile(data.emissionTexturePath, texID)) {
            m_EmissionSlot.textureID = texID;
        }
    }

    // Metadata
    m_MaterialCategory = data.category;
    m_MaterialTags = data.tags;

    // Apply to current selection
    ApplyToSelection();

    // For GLTF materials, also update the GPU buffer
    if (m_SelectionType == MaterialSelectionType::GLTFMaterial) {
        ApplyToGLTFMaterial();
    }
}

} // namespace Yalaz::UI
