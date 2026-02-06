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
    MeshNode* meshNode = m_Engine->selectedNode;

    if (meshNode && meshNode->mesh && !meshNode->mesh->surfaces.empty()) {
        // GLTF model selected
        if (meshNode != m_LastSelectedMeshNode) {
            m_SelectionType = MaterialSelectionType::GLTFMaterial;
            m_SelectedMeshNode = meshNode;
            m_SelectedSurfaceIndex = 0;
            m_LastSelectedMeshNode = meshNode;
            m_LastSelectedPrimitiveIndex = -1;

            // Load material data
            LoadGLTFMaterialData();
        }
    } else if (m_Engine->selectedPrimitiveIndex >= 0 &&
               m_Engine->selectedPrimitiveIndex < static_cast<int>(m_Engine->static_shapes.size())) {
        // Primitive selected
        if (m_Engine->selectedPrimitiveIndex != m_LastSelectedPrimitiveIndex) {
            m_SelectionType = MaterialSelectionType::Primitive;
            m_SelectedMeshNode = nullptr;
            m_CurrentGLTFMaterial = nullptr;
            m_LastSelectedPrimitiveIndex = m_Engine->selectedPrimitiveIndex;
            m_LastSelectedMeshNode = nullptr;

            // Load all primitive material properties
            auto& shape = m_Engine->static_shapes[m_Engine->selectedPrimitiveIndex];
            m_BaseColor = shape.mainColor;
            m_Metallic = shape.metallic;
            m_Roughness = shape.roughness;

            // Load emission (if non-zero)
            float emissionLen = glm::length(shape.emission);
            if (emissionLen > 0.001f) {
                m_EmissionStrength = emissionLen;
                m_Emission = shape.emission / emissionLen;
            } else {
                m_Emission = glm::vec3(0.0f);
                m_EmissionStrength = 0.0f;
            }
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
    if (!m_SelectedMeshNode || !m_SelectedMeshNode->mesh) return;

    auto& surfaces = m_SelectedMeshNode->mesh->surfaces;
    if (m_SelectedSurfaceIndex >= static_cast<int>(surfaces.size())) {
        m_SelectedSurfaceIndex = 0;
    }

    if (surfaces.empty()) return;

    auto& surface = surfaces[m_SelectedSurfaceIndex];
    m_CurrentGLTFMaterial = surface.material;

    if (!m_CurrentGLTFMaterial) return;

    // Read material constants from the material data buffer
    // The material data is stored in the LoadedGLTF's materialDataBuffer
    // We need to find which LoadedGLTF owns this material

    // For now, try to read from the material's descriptor set data
    // This would require accessing the mapped buffer data

    // Default values if we can't read from buffer
    m_BaseColor = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);
    m_Metallic = 0.0f;
    m_Roughness = 0.5f;

    // Try to find the material in loaded scenes to get its data
    for (auto& [sceneName, scene] : m_Engine->loadedScenes) {
        if (!scene) continue;

        for (auto& [matName, mat] : scene->materials) {
            if (mat.get() == m_CurrentGLTFMaterial.get()) {
                // Found the material - now we need to read its constants
                // The constants are in scene->materialDataBuffer

                // For now, we'll use defaults and update them when the user changes values
                // A full implementation would memory-map the buffer to read current values
                break;
            }
        }
    }
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

    // Base Color with alpha
    SectionHeader("Base Color");
    float col[4] = { m_BaseColor.r, m_BaseColor.g, m_BaseColor.b, m_BaseColor.a };
    if (ImGui::ColorEdit4("##BaseColor", col, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_AlphaBar)) {
        m_BaseColor = glm::vec4(col[0], col[1], col[2], col[3]);
        changed = true;
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
    if (ImGui::ColorEdit3("##EmissionColor", emCol)) {
        m_Emission = glm::vec3(emCol[0], emCol[1], emCol[2]);
        changed = true;
    }
    if (ImGui::SliderFloat("Emission Strength", &m_EmissionStrength, 0.0f, 10.0f)) {
        changed = true;
    }

    // Apply button for GLTF materials
    if (m_SelectionType == MaterialSelectionType::GLTFMaterial) {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Apply to GLTF Material", ImVec2(-1, 30))) {
            ApplyToGLTFMaterial();
        }
        ImGui::TextDisabled("Updates the GPU buffer in real-time");
    }

    // Apply changes
    if (changed) {
        ApplyToSelection();
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
