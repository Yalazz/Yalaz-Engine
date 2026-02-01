// =============================================================================
// YALAZ ENGINE - Hierarchy View Implementation
// =============================================================================
// Professional scene hierarchy with ALL features:
// - Dynamic face colors per primitive type (cube=6, sphere=2, cylinder=3, etc.)
// - Spawn settings (transform, colors)
// - Quick create in front of camera
// - Tree view with search and type filtering
// - Context menus with Focus Camera
// - Stats bar
// - Visibility toggles
// =============================================================================

#include "HierarchyView.h"
#include "../EditorTheme.h"
#include "../../vk_engine.h"
#include <algorithm>
#include <cmath>

namespace Yalaz::UI {

// =============================================================================
// DYNAMIC FACE CONFIGURATIONS PER PRIMITIVE TYPE
// =============================================================================
// Each primitive has different face structure:
// - Cube: 6 faces (Front, Back, Right, Left, Top, Bottom)
// - Sphere: 2 hemispheres (Upper, Lower) or bands
// - Cylinder: 3 parts (Top Cap, Bottom Cap, Side)
// - Cone: 2 parts (Base, Side)
// - Capsule: 3 parts (Top Dome, Middle, Bottom Dome)
// - Torus: 2 parts (Outer Ring, Inner Ring)
// - Plane: 2 sides (Front, Back)
// - Triangle: 2 sides (Front, Back)
// =============================================================================

static const PrimitiveFaceConfig s_FaceConfigs[] = {
    // Cube (type 0) - 6 faces
    {
        6,
        { "Front (+Z)", "Back (-Z)", "Right (+X)", "Left (-X)", "Top (+Y)", "Bottom (-Y)", "", "" },
        {
            glm::vec4(1.0f, 0.3f, 0.3f, 1.0f),  // Front - Red
            glm::vec4(0.3f, 1.0f, 0.3f, 1.0f),  // Back - Green
            glm::vec4(0.3f, 0.3f, 1.0f, 1.0f),  // Right - Blue
            glm::vec4(1.0f, 1.0f, 0.3f, 1.0f),  // Left - Yellow
            glm::vec4(1.0f, 0.3f, 1.0f, 1.0f),  // Top - Magenta
            glm::vec4(0.3f, 1.0f, 1.0f, 1.0f),  // Bottom - Cyan
            glm::vec4(1.0f), glm::vec4(1.0f)
        }
    },
    // Sphere (type 1) - 2 hemispheres + optional bands
    {
        4,
        { "Upper Hemisphere", "Lower Hemisphere", "Equator Band", "Poles", "", "", "", "" },
        {
            glm::vec4(0.9f, 0.6f, 0.3f, 1.0f),  // Upper - Orange
            glm::vec4(0.3f, 0.6f, 0.9f, 1.0f),  // Lower - Sky Blue
            glm::vec4(0.9f, 0.9f, 0.3f, 1.0f),  // Equator - Yellow
            glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),  // Poles - White
            glm::vec4(1.0f), glm::vec4(1.0f), glm::vec4(1.0f), glm::vec4(1.0f)
        }
    },
    // Cylinder (type 2) - 3 parts
    {
        3,
        { "Top Cap", "Bottom Cap", "Side Surface", "", "", "", "", "" },
        {
            glm::vec4(0.8f, 0.2f, 0.2f, 1.0f),  // Top - Red
            glm::vec4(0.2f, 0.8f, 0.2f, 1.0f),  // Bottom - Green
            glm::vec4(0.6f, 0.6f, 0.8f, 1.0f),  // Side - Light Blue
            glm::vec4(1.0f), glm::vec4(1.0f), glm::vec4(1.0f), glm::vec4(1.0f), glm::vec4(1.0f)
        }
    },
    // Cone (type 3) - 2 parts
    {
        2,
        { "Base Circle", "Cone Surface", "", "", "", "", "", "" },
        {
            glm::vec4(0.2f, 0.6f, 0.2f, 1.0f),  // Base - Green
            glm::vec4(0.9f, 0.5f, 0.2f, 1.0f),  // Surface - Orange
            glm::vec4(1.0f), glm::vec4(1.0f), glm::vec4(1.0f), glm::vec4(1.0f), glm::vec4(1.0f), glm::vec4(1.0f)
        }
    },
    // Capsule (type 4) - 3 parts
    {
        3,
        { "Top Dome", "Middle Cylinder", "Bottom Dome", "", "", "", "", "" },
        {
            glm::vec4(1.0f, 0.4f, 0.4f, 1.0f),  // Top - Light Red
            glm::vec4(0.4f, 1.0f, 0.4f, 1.0f),  // Middle - Light Green
            glm::vec4(0.4f, 0.4f, 1.0f, 1.0f),  // Bottom - Light Blue
            glm::vec4(1.0f), glm::vec4(1.0f), glm::vec4(1.0f), glm::vec4(1.0f), glm::vec4(1.0f)
        }
    },
    // Torus (type 5) - 2 parts
    {
        2,
        { "Outer Ring", "Inner Ring", "", "", "", "", "", "" },
        {
            glm::vec4(0.8f, 0.3f, 0.8f, 1.0f),  // Outer - Purple
            glm::vec4(0.3f, 0.8f, 0.8f, 1.0f),  // Inner - Teal
            glm::vec4(1.0f), glm::vec4(1.0f), glm::vec4(1.0f), glm::vec4(1.0f), glm::vec4(1.0f), glm::vec4(1.0f)
        }
    },
    // Plane (type 6) - 2 sides
    {
        2,
        { "Front Face", "Back Face", "", "", "", "", "", "" },
        {
            glm::vec4(0.7f, 0.7f, 0.9f, 1.0f),  // Front - Light Purple
            glm::vec4(0.5f, 0.5f, 0.7f, 1.0f),  // Back - Darker Purple
            glm::vec4(1.0f), glm::vec4(1.0f), glm::vec4(1.0f), glm::vec4(1.0f), glm::vec4(1.0f), glm::vec4(1.0f)
        }
    },
    // Triangle (type 7) - 2 sides
    {
        2,
        { "Front Face", "Back Face", "", "", "", "", "", "" },
        {
            glm::vec4(1.0f, 0.8f, 0.2f, 1.0f),  // Front - Gold
            glm::vec4(0.8f, 0.6f, 0.1f, 1.0f),  // Back - Darker Gold
            glm::vec4(1.0f), glm::vec4(1.0f), glm::vec4(1.0f), glm::vec4(1.0f), glm::vec4(1.0f), glm::vec4(1.0f)
        }
    }
};

const PrimitiveFaceConfig& HierarchyView::GetFaceConfig(int primitiveType) {
    if (primitiveType >= 0 && primitiveType < 8) {
        return s_FaceConfigs[primitiveType];
    }
    return s_FaceConfigs[0]; // Default to cube
}

// =============================================================================
// CONSTRUCTOR
// =============================================================================

HierarchyView::HierarchyView()
    : EditorView("Hierarchy", "[H]", ViewCategory::Core)
{
    // Initialize face colors with cube defaults
    const auto& config = GetFaceConfig(0);
    for (int i = 0; i < 8; ++i) {
        m_FaceColors[i] = config.defaultColors[i];
    }
}

// =============================================================================
// MAIN RENDER
// =============================================================================

void HierarchyView::OnRender() {
    if (!BeginView(ImGuiWindowFlags_MenuBar)) {
        EndView();
        return;
    }

    // Menu bar with create options
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("Create")) {
            RenderCreateMenu();
            ImGui::EndMenu();
        }

        // Filter toggles
        ImGui::SameLine(ImGui::GetWindowWidth() - 150);
        ImGui::Checkbox("P", &m_ShowPrimitives);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Show Primitives");
        ImGui::SameLine();
        ImGui::Checkbox("L", &m_ShowLights);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Show Lights");
        ImGui::SameLine();
        ImGui::Checkbox("N", &m_ShowNodes);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Show Scene Nodes");

        ImGui::EndMenuBar();
    }

    // Create Section (collapsible)
    RenderCreateSection();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Search bar
    ImGui::SetNextItemWidth(-80);
    ImGui::InputTextWithHint("##Search", "Search...", m_SearchBuffer, sizeof(m_SearchBuffer));
    ImGui::SameLine();

    // Type filter
    const char* filters[] = { "All", "Cube", "Sphere", "Cyl", "Cone", "Cap", "Tor", "Plane", "Tri" };
    ImGui::SetNextItemWidth(70);
    ImGui::Combo("##Filter", &m_FilterType, filters, IM_ARRAYSIZE(filters));

    ImGui::Spacing();

    // Scrollable list
    float listHeight = ImGui::GetContentRegionAvail().y - 25;
    ImGui::BeginChild("HierarchyList", ImVec2(0, listHeight), true);

    if (m_Engine) {
        if (m_ShowNodes) RenderSceneNodes();
        if (m_ShowPrimitives) RenderPrimitives();
        if (m_ShowLights) RenderLights();
    }

    ImGui::EndChild();

    // Stats bar
    if (m_Engine) {
        ImGui::TextDisabled("%zu primitives | %zu lights | %zu scenes",
            m_Engine->static_shapes.size(),
            m_Engine->scenePointLights.size(),
            m_Engine->loadedScenes.size());
    }

    EndView();
}

// =============================================================================
// CREATE SECTION
// =============================================================================

void HierarchyView::RenderCreateSection() {
    // Green header
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.15f, 0.45f, 0.35f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.2f, 0.55f, 0.4f, 0.9f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.25f, 0.6f, 0.45f, 1.0f));

    if (ImGui::CollapsingHeader("+ Create Primitive", m_CreateSectionOpen ? ImGuiTreeNodeFlags_DefaultOpen : 0)) {
        m_CreateSectionOpen = true;
        ImGui::Indent(8.0f);

        // Shape tabs
        if (ImGui::BeginTabBar("ShapeTabs")) {
            if (ImGui::BeginTabItem("3D")) {
                m_ShapeTab = 1;
                const char* shapes[] = { "Cube", "Sphere", "Cylinder", "Cone", "Capsule", "Torus" };
                int oldShape = m_Selected3DShape;
                ImGui::SetNextItemWidth(-1);
                if (ImGui::Combo("##3DShape", &m_Selected3DShape, shapes, IM_ARRAYSIZE(shapes))) {
                    // Reset face colors when shape changes
                    if (oldShape != m_Selected3DShape) {
                        const auto& config = GetFaceConfig(m_Selected3DShape);
                        for (int i = 0; i < 8; ++i) {
                            m_FaceColors[i] = config.defaultColors[i];
                        }
                    }
                }
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("2D")) {
                m_ShapeTab = 0;
                const char* shapes[] = { "Triangle", "Plane" };
                int oldShape = m_Selected2DShape;
                ImGui::SetNextItemWidth(-1);
                if (ImGui::Combo("##2DShape", &m_Selected2DShape, shapes, IM_ARRAYSIZE(shapes))) {
                    // Reset face colors when shape changes
                    if (oldShape != m_Selected2DShape) {
                        int typeIndex = (m_Selected2DShape == 0) ? 7 : 6; // Triangle=7, Plane=6
                        const auto& config = GetFaceConfig(typeIndex);
                        for (int i = 0; i < 8; ++i) {
                            m_FaceColors[i] = config.defaultColors[i];
                        }
                    }
                }
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }

        ImGui::Spacing();

        // Transform settings
        if (ImGui::TreeNode("Transform")) {
            ImGui::DragFloat3("Position", &m_SpawnPosition.x, 0.1f);

            glm::vec3 rotDeg = glm::degrees(m_SpawnRotation);
            if (ImGui::DragFloat3("Rotation", &rotDeg.x, 1.0f)) {
                m_SpawnRotation = glm::radians(rotDeg);
            }

            ImGui::DragFloat3("Scale", &m_SpawnScale.x, 0.05f, 0.01f, 100.0f);

            if (ImGui::Button("Reset Transform", ImVec2(-1, 0))) {
                m_SpawnPosition = glm::vec3(0.0f);
                m_SpawnRotation = glm::vec3(0.0f);
                m_SpawnScale = glm::vec3(1.0f);
            }

            ImGui::TreePop();
        }

        // Color settings with DYNAMIC face colors
        if (ImGui::TreeNode("Color")) {
            ImGui::ColorEdit4("Main Color", &m_MainColor.x, ImGuiColorEditFlags_AlphaBar);
            ImGui::Checkbox("Use Face Colors", &m_UseFaceColors);

            if (m_UseFaceColors) {
                int typeIndex = (m_ShapeTab == 1) ? m_Selected3DShape : ((m_Selected2DShape == 0) ? 7 : 6);
                RenderFaceColorEditor(typeIndex);
            }

            ImGui::TreePop();
        }

        ImGui::Spacing();

        // Add button
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.9f, 0.5f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.6f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.4f, 0.1f, 1.0f));

        if (ImGui::Button("+ Add Primitive", ImVec2(-1, 32))) {
            SpawnPrimitive();
        }

        ImGui::PopStyleColor(3);
        ImGui::Unindent(8.0f);
    } else {
        m_CreateSectionOpen = false;
    }

    ImGui::PopStyleColor(3);
}

// =============================================================================
// DYNAMIC FACE COLOR EDITOR
// =============================================================================

void HierarchyView::RenderFaceColorEditor(int primitiveType) {
    const auto& config = GetFaceConfig(primitiveType);

    ImGui::Spacing();
    ImGui::TextDisabled("Face Colors (%d faces)", config.faceCount);
    ImGui::Separator();

    for (int i = 0; i < config.faceCount; ++i) {
        ImGui::PushID(i);
        ImGui::ColorEdit4(config.faceNames[i], &m_FaceColors[i].x,
            ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
        ImGui::PopID();
    }

    // Color presets based on type
    ImGui::Spacing();
    ImGui::TextDisabled("Presets:");

    if (ImGui::Button("Default", ImVec2(60, 0))) {
        for (int i = 0; i < 8; ++i) {
            m_FaceColors[i] = config.defaultColors[i];
        }
    }
    ImGui::SameLine();

    if (ImGui::Button("Rainbow", ImVec2(60, 0))) {
        const glm::vec4 rainbow[] = {
            glm::vec4(1, 0, 0, 1), glm::vec4(1, 0.5f, 0, 1),
            glm::vec4(1, 1, 0, 1), glm::vec4(0, 1, 0, 1),
            glm::vec4(0, 1, 1, 1), glm::vec4(0, 0, 1, 1),
            glm::vec4(0.5f, 0, 1, 1), glm::vec4(1, 0, 0.5f, 1)
        };
        for (int i = 0; i < config.faceCount && i < 8; ++i) {
            m_FaceColors[i] = rainbow[i];
        }
    }
    ImGui::SameLine();

    if (ImGui::Button("White", ImVec2(60, 0))) {
        for (int i = 0; i < 8; ++i) {
            m_FaceColors[i] = glm::vec4(1, 1, 1, 1);
        }
    }
    ImGui::SameLine();

    if (ImGui::Button("Random", ImVec2(60, 0))) {
        for (int i = 0; i < config.faceCount; ++i) {
            m_FaceColors[i] = glm::vec4(
                static_cast<float>(rand()) / RAND_MAX,
                static_cast<float>(rand()) / RAND_MAX,
                static_cast<float>(rand()) / RAND_MAX,
                1.0f
            );
        }
    }
}

// =============================================================================
// SPAWN PRIMITIVE - WITH SPAWN SETTINGS
// =============================================================================

void HierarchyView::SpawnPrimitive() {
    if (!m_Engine) return;

    StaticMeshData newMesh;

    const char* names2D[] = { "Triangle", "Plane" };
    const char* names3D[] = { "Cube", "Sphere", "Cylinder", "Cone", "Capsule", "Torus" };

    // Set name
    if (m_ShapeTab == 0) {
        newMesh.name = std::string(names2D[m_Selected2DShape]) + "_" + std::to_string(++m_PrimitiveCounter);
    } else {
        newMesh.name = std::string(names3D[m_Selected3DShape]) + "_" + std::to_string(++m_PrimitiveCounter);
    }

    // Set transform from spawn settings
    newMesh.position = m_SpawnPosition;
    newMesh.rotation = m_SpawnRotation;
    newMesh.scale = m_SpawnScale;

    // Set colors
    newMesh.mainColor = m_MainColor;
    newMesh.useFaceColors = m_UseFaceColors;

    // Get face count for this type
    int typeIndex;
    if (m_ShapeTab == 0) {
        typeIndex = (m_Selected2DShape == 0) ? 7 : 6;
        newMesh.type = (m_Selected2DShape == 0) ? PrimitiveType::Triangle : PrimitiveType::Plane;
    } else {
        typeIndex = m_Selected3DShape;
        switch (m_Selected3DShape) {
            case 0: newMesh.type = PrimitiveType::Cube; break;
            case 1: newMesh.type = PrimitiveType::Sphere; break;
            case 2: newMesh.type = PrimitiveType::Cylinder; break;
            case 3: newMesh.type = PrimitiveType::Cone; break;
            case 4: newMesh.type = PrimitiveType::Capsule; break;
            case 5: newMesh.type = PrimitiveType::Torus; break;
        }
    }

    // Copy face colors (up to 6 for StaticMeshData compatibility)
    for (int i = 0; i < 6; i++) {
        newMesh.faceColors[i] = m_FaceColors[i];
    }

    // Get mesh from engine's default meshes
    auto it = m_Engine->defaultMeshes.find(newMesh.type);
    if (it != m_Engine->defaultMeshes.end()) {
        newMesh.mesh = it->second;
    }

    newMesh.materialType = ShaderOnlyMaterial::DEFAULT;
    newMesh.passType = MaterialPass::MainColor;
    newMesh.visible = true;
    newMesh.selected = false;

    m_Engine->static_shapes.push_back(newMesh);
    m_Engine->selectedPrimitiveIndex = static_cast<int>(m_Engine->static_shapes.size()) - 1;
    m_Engine->selectedNode = nullptr;
    m_Engine->selectedObjectName.clear();

    // Auto-advance spawn position
    m_SpawnPosition.x += 2.5f;
}

// =============================================================================
// SPAWN PRIMITIVE QUICK - IN FRONT OF CAMERA
// =============================================================================

void HierarchyView::SpawnPrimitiveQuick(int type, const char* name) {
    if (!m_Engine) return;

    StaticMeshData newMesh;
    newMesh.type = static_cast<PrimitiveType>(type);
    newMesh.name = std::string(name) + "_" + std::to_string(++m_PrimitiveCounter);

    // Position in front of camera
    float yaw = m_Engine->mainCamera.yaw;
    float pitch = m_Engine->mainCamera.pitch;
    glm::vec3 forward = glm::normalize(glm::vec3(
        cos(glm::radians(yaw)) * cos(glm::radians(pitch)),
        sin(glm::radians(pitch)),
        sin(glm::radians(yaw)) * cos(glm::radians(pitch))
    ));
    newMesh.position = m_Engine->mainCamera.position + forward * 5.0f;
    newMesh.rotation = glm::vec3(0.0f);
    newMesh.scale = glm::vec3(1.0f);

    // Default color
    newMesh.mainColor = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);
    newMesh.useFaceColors = false;
    newMesh.visible = true;
    newMesh.selected = false;

    // Default material
    newMesh.materialType = ShaderOnlyMaterial::DEFAULT;
    newMesh.passType = MaterialPass::MainColor;

    // Get mesh from engine's default meshes
    auto it = m_Engine->defaultMeshes.find(newMesh.type);
    if (it != m_Engine->defaultMeshes.end()) {
        newMesh.mesh = it->second;
    }

    m_Engine->static_shapes.push_back(newMesh);
    m_Engine->selectedPrimitiveIndex = static_cast<int>(m_Engine->static_shapes.size()) - 1;
    m_Engine->selectedNode = nullptr;
    m_Engine->selectedObjectName.clear();
}

// =============================================================================
// SCENE NODES
// =============================================================================

void HierarchyView::RenderSceneNodes() {
    if (m_Engine->loadedScenes.empty()) return;

    bool nodeOpen = ImGui::TreeNodeEx("Scene Nodes", ImGuiTreeNodeFlags_DefaultOpen);
    if (nodeOpen) {
        for (auto& [sceneName, scene] : m_Engine->loadedScenes) {
            // Search filter
            std::string lowerName = sceneName;
            std::string lowerSearch = m_SearchBuffer;
            std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
            std::transform(lowerSearch.begin(), lowerSearch.end(), lowerSearch.begin(), ::tolower);

            if (strlen(m_SearchBuffer) > 0 && lowerName.find(lowerSearch) == std::string::npos) {
                continue;
            }

            ImGuiTreeNodeFlags sceneFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
            bool isSceneSelected = (m_Engine->selectedObjectName == sceneName);
            if (isSceneSelected) sceneFlags |= ImGuiTreeNodeFlags_Selected;

            bool sceneOpen = ImGui::TreeNodeEx(sceneName.c_str(), sceneFlags);

            if (ImGui::IsItemClicked()) {
                m_Engine->selectedObjectName = sceneName;
                m_Engine->selectedPrimitiveIndex = -1;
            }

            if (sceneOpen && scene) {
                for (auto& [nodeName, node] : scene->nodes) {
                    ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanAvailWidth;

                    bool isNodeSelected = m_Engine->selectedNode &&
                        (static_cast<Node*>(m_Engine->selectedNode) == node.get());
                    if (isNodeSelected) nodeFlags |= ImGuiTreeNodeFlags_Selected;

                    bool nodeItemOpen = ImGui::TreeNodeEx(nodeName.c_str(), nodeFlags);

                    if (ImGui::IsItemClicked()) {
                        MeshNode* meshNode = dynamic_cast<MeshNode*>(node.get());
                        m_Engine->selectedNode = meshNode;
                        m_Engine->selectedObjectName = nodeName;
                        m_Engine->selectedPrimitiveIndex = -1;
                        m_Engine->selectedLightIndex = -1;  // Clear light selection
                    }

                    // Context menu
                    if (ImGui::BeginPopupContextItem()) {
                        if (ImGui::MenuItem("Focus Camera")) {
                            glm::vec3 pos = glm::vec3(node->worldTransform[3]);
                            m_Engine->mainCamera.position = pos + glm::vec3(0, 2, 5);
                        }
                        ImGui::EndPopup();
                    }

                    if (nodeItemOpen) ImGui::TreePop();
                }
                ImGui::TreePop();
            }
        }
        ImGui::TreePop();
    }
}

// =============================================================================
// PRIMITIVES
// =============================================================================

void HierarchyView::RenderPrimitives() {
    if (m_Engine->static_shapes.empty()) return;

    bool primOpen = ImGui::TreeNodeEx("Primitives", ImGuiTreeNodeFlags_DefaultOpen);
    if (primOpen) {
        for (size_t i = 0; i < m_Engine->static_shapes.size(); ++i) {
            auto& shape = m_Engine->static_shapes[i];

            const char* typeNames[] = { "Cube", "Sphere", "Cylinder", "Cone", "Capsule", "Torus", "Plane", "Triangle" };
            int typeIndex = static_cast<int>(shape.type);
            const char* typeName = (typeIndex >= 0 && typeIndex < 8) ? typeNames[typeIndex] : "Unknown";

            std::string label = shape.name.empty() ? std::string(typeName) + " " + std::to_string(i) : shape.name;

            // Search filter
            std::string lowerLabel = label;
            std::string lowerSearch = m_SearchBuffer;
            std::transform(lowerLabel.begin(), lowerLabel.end(), lowerLabel.begin(), ::tolower);
            std::transform(lowerSearch.begin(), lowerSearch.end(), lowerSearch.begin(), ::tolower);

            if (strlen(m_SearchBuffer) > 0 && lowerLabel.find(lowerSearch) == std::string::npos) continue;

            // Type filter
            if (m_FilterType > 0 && static_cast<int>(shape.type) != (m_FilterType - 1)) continue;

            ImGui::PushID(static_cast<int>(i));

            // Visibility toggle
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
            if (ImGui::Button(shape.visible ? "[V]" : "[H]", ImVec2(24, 0))) {
                shape.visible = !shape.visible;
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip(shape.visible ? "Hide" : "Show");
            ImGui::PopStyleColor();
            ImGui::SameLine();

            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanAvailWidth;
            bool isSelected = (m_Engine->selectedPrimitiveIndex == static_cast<int>(i));
            if (isSelected) flags |= ImGuiTreeNodeFlags_Selected;

            ImGui::PushStyleColor(ImGuiCol_Text,
                ImVec4(shape.mainColor.x, shape.mainColor.y, shape.mainColor.z, shape.visible ? 1.0f : 0.5f));

            bool open = ImGui::TreeNodeEx(label.c_str(), flags);

            ImGui::PopStyleColor();

            if (ImGui::IsItemClicked()) {
                if (m_Engine->selectedPrimitiveIndex >= 0 &&
                    m_Engine->selectedPrimitiveIndex < static_cast<int>(m_Engine->static_shapes.size())) {
                    m_Engine->static_shapes[m_Engine->selectedPrimitiveIndex].selected = false;
                }
                m_Engine->selectedPrimitiveIndex = static_cast<int>(i);
                m_Engine->selectedLightIndex = -1;  // Clear light selection
                m_Engine->selectedNode = nullptr;
                m_Engine->selectedObjectName.clear();
                shape.selected = true;
            }

            // Context menu
            if (ImGui::BeginPopupContextItem()) {
                if (ImGui::MenuItem("Focus Camera")) {
                    m_Engine->mainCamera.position = shape.position + glm::vec3(0, 2, 5);
                }
                ImGui::Separator();
                if (ImGui::MenuItem(shape.visible ? "Hide" : "Show")) {
                    shape.visible = !shape.visible;
                }
                if (ImGui::MenuItem("Duplicate")) {
                    StaticMeshData copy = shape;
                    copy.position += glm::vec3(1, 0, 1);
                    copy.name = shape.name + "_copy";
                    copy.selected = false;
                    m_Engine->static_shapes.push_back(copy);
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Delete")) {
                    m_Engine->static_shapes.erase(m_Engine->static_shapes.begin() + i);
                    if (m_Engine->selectedPrimitiveIndex == static_cast<int>(i)) {
                        m_Engine->selectedPrimitiveIndex = -1;
                    } else if (m_Engine->selectedPrimitiveIndex > static_cast<int>(i)) {
                        m_Engine->selectedPrimitiveIndex--;
                    }
                    ImGui::EndPopup();
                    ImGui::PopID();
                    if (open) ImGui::TreePop();
                    break;
                }
                ImGui::EndPopup();
            }

            if (open) ImGui::TreePop();
            ImGui::PopID();
        }
        ImGui::TreePop();
    }
}

// =============================================================================
// LIGHTS
// =============================================================================

void HierarchyView::RenderLights() {
    bool lightsOpen = ImGui::TreeNodeEx("Lights", ImGuiTreeNodeFlags_DefaultOpen);
    if (lightsOpen) {
        for (size_t i = 0; i < m_Engine->scenePointLights.size(); ++i) {
            auto& light = m_Engine->scenePointLights[i];
            std::string label = "Point Light " + std::to_string(i);

            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanAvailWidth;

            // Highlight selected light
            bool isSelected = (m_Engine->selectedLightIndex == static_cast<int>(i));
            if (isSelected) flags |= ImGuiTreeNodeFlags_Selected;

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(light.color.x, light.color.y, light.color.z, 1.0f));
            bool open = ImGui::TreeNodeEx(label.c_str(), flags);
            ImGui::PopStyleColor();

            // Handle selection
            if (ImGui::IsItemClicked()) {
                m_Engine->selectedLightIndex = static_cast<int>(i);
                m_Engine->selectedPrimitiveIndex = -1;
                m_Engine->selectedNode = nullptr;
                m_Engine->selectedObjectName.clear();
            }

            if (ImGui::BeginPopupContextItem()) {
                if (ImGui::MenuItem("Focus Camera")) {
                    m_Engine->mainCamera.position = light.position + glm::vec3(0, 2, 5);
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Duplicate")) {
                    PointLight newLight = light;
                    newLight.position += glm::vec3(2, 0, 0);
                    m_Engine->scenePointLights.push_back(newLight);
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Delete")) {
                    m_Engine->scenePointLights.erase(m_Engine->scenePointLights.begin() + i);
                    // Update selection
                    if (m_Engine->selectedLightIndex == static_cast<int>(i)) {
                        m_Engine->selectedLightIndex = -1;
                    } else if (m_Engine->selectedLightIndex > static_cast<int>(i)) {
                        m_Engine->selectedLightIndex--;
                    }
                    ImGui::EndPopup();
                    if (open) ImGui::TreePop();
                    break;
                }
                ImGui::EndPopup();
            }

            if (open) ImGui::TreePop();
        }

        if (ImGui::Button("+ Add Light", ImVec2(-1, 0))) {
            PointLight newLight;
            newLight.position = m_Engine->mainCamera.position + glm::vec3(0, 2, 0);
            newLight.color = glm::vec3(1.0f);
            newLight.intensity = 10.0f;
            newLight.radius = 15.0f;
            m_Engine->scenePointLights.push_back(newLight);
        }

        ImGui::TreePop();
    }
}

// =============================================================================
// CREATE MENU - QUICK CREATE OPTIONS
// =============================================================================

void HierarchyView::RenderCreateMenu() {
    // Quick create - spawns in front of camera
    if (ImGui::BeginMenu("Quick Create (In Front)")) {
        if (ImGui::MenuItem("Cube")) SpawnPrimitiveQuick(0, "Cube");
        if (ImGui::MenuItem("Sphere")) SpawnPrimitiveQuick(1, "Sphere");
        if (ImGui::MenuItem("Cylinder")) SpawnPrimitiveQuick(2, "Cylinder");
        if (ImGui::MenuItem("Cone")) SpawnPrimitiveQuick(3, "Cone");
        if (ImGui::MenuItem("Capsule")) SpawnPrimitiveQuick(4, "Capsule");
        if (ImGui::MenuItem("Torus")) SpawnPrimitiveQuick(5, "Torus");
        ImGui::Separator();
        if (ImGui::MenuItem("Plane")) SpawnPrimitiveQuick(6, "Plane");
        if (ImGui::MenuItem("Triangle")) SpawnPrimitiveQuick(7, "Triangle");
        ImGui::EndMenu();
    }

    ImGui::Separator();

    // Create with settings - uses spawn settings panel
    if (ImGui::BeginMenu("3D Primitives")) {
        if (ImGui::MenuItem("Cube")) { m_ShapeTab = 1; m_Selected3DShape = 0; SpawnPrimitive(); }
        if (ImGui::MenuItem("Sphere")) { m_ShapeTab = 1; m_Selected3DShape = 1; SpawnPrimitive(); }
        if (ImGui::MenuItem("Cylinder")) { m_ShapeTab = 1; m_Selected3DShape = 2; SpawnPrimitive(); }
        if (ImGui::MenuItem("Cone")) { m_ShapeTab = 1; m_Selected3DShape = 3; SpawnPrimitive(); }
        if (ImGui::MenuItem("Capsule")) { m_ShapeTab = 1; m_Selected3DShape = 4; SpawnPrimitive(); }
        if (ImGui::MenuItem("Torus")) { m_ShapeTab = 1; m_Selected3DShape = 5; SpawnPrimitive(); }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("2D Primitives")) {
        if (ImGui::MenuItem("Triangle")) { m_ShapeTab = 0; m_Selected2DShape = 0; SpawnPrimitive(); }
        if (ImGui::MenuItem("Plane")) { m_ShapeTab = 0; m_Selected2DShape = 1; SpawnPrimitive(); }
        ImGui::EndMenu();
    }

    ImGui::Separator();

    if (ImGui::BeginMenu("Lights")) {
        if (ImGui::MenuItem("Point Light")) {
            PointLight newLight;
            newLight.position = m_Engine->mainCamera.position + glm::vec3(0, 2, 0);
            newLight.color = glm::vec3(1.0f);
            newLight.intensity = 10.0f;
            newLight.radius = 15.0f;
            m_Engine->scenePointLights.push_back(newLight);
        }
        ImGui::EndMenu();
    }
}

} // namespace Yalaz::UI
