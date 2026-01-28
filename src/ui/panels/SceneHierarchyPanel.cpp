// =============================================================================
// YALAZ ENGINE - Scene Hierarchy Panel Implementation
// =============================================================================

#include "SceneHierarchyPanel.h"
#include "../EditorSelection.h"
#include "../EditorTheme.h"
#include "../../vk_engine.h"
#include <imgui.h>

namespace Yalaz::UI {

SceneHierarchyPanel::SceneHierarchyPanel()
    : Panel("Scene Hierarchy")
{
    // Initialize face colors
    m_FaceColors[0] = glm::vec4(1.0f, 0.3f, 0.3f, 1.0f);  // Front
    m_FaceColors[1] = glm::vec4(0.3f, 1.0f, 0.3f, 1.0f);  // Back
    m_FaceColors[2] = glm::vec4(0.3f, 0.3f, 1.0f, 1.0f);  // Right
    m_FaceColors[3] = glm::vec4(1.0f, 1.0f, 0.3f, 1.0f);  // Left
    m_FaceColors[4] = glm::vec4(1.0f, 0.3f, 1.0f, 1.0f);  // Top
    m_FaceColors[5] = glm::vec4(0.3f, 1.0f, 1.0f, 1.0f);  // Bottom
}

void SceneHierarchyPanel::OnInit(VulkanEngine* engine) {
    Panel::OnInit(engine);
}

void SceneHierarchyPanel::OnRender() {
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;

    if (BeginPanel(flags)) {
        RenderTabs();
    }
    EndPanel();
}

void SceneHierarchyPanel::RenderTabs() {
    if (ImGui::BeginTabBar("HierarchyTabs")) {
        if (ImGui::BeginTabItem("Primitives")) {
            m_ActiveTab = 0;
            RenderPrimitivesTab();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Scene Objects")) {
            m_ActiveTab = 1;
            RenderSceneObjectsTab();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}

void SceneHierarchyPanel::RenderPrimitivesTab() {
    RenderCreateSection();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Search & Filter
    ImGui::SetNextItemWidth(-80);
    ImGui::InputTextWithHint("##Search", "Search...", m_SearchBuffer, sizeof(m_SearchBuffer));
    ImGui::SameLine();

    const char* filters[] = { "All", "Cube", "Sphere", "Cyl", "Cone", "Cap", "Tor" };
    ImGui::SetNextItemWidth(70);
    ImGui::Combo("##Filter", &m_FilterType, filters, IM_ARRAYSIZE(filters));

    ImGui::Spacing();

    RenderPrimitivesList();
}

void SceneHierarchyPanel::RenderCreateSection() {
    // Green header for create section
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
                ImGui::SetNextItemWidth(-1);
                ImGui::Combo("##3DShape", &m_Selected3DShape, shapes, IM_ARRAYSIZE(shapes));
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("2D")) {
                m_ShapeTab = 0;
                const char* shapes[] = { "Triangle", "Plane" };
                ImGui::SetNextItemWidth(-1);
                ImGui::Combo("##2DShape", &m_Selected2DShape, shapes, IM_ARRAYSIZE(shapes));
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }

        ImGui::Spacing();

        // Transform
        if (ImGui::TreeNode("Transform")) {
            ImGui::DragFloat3("Position", &m_SpawnPosition.x, 0.1f);

            glm::vec3 rotDeg = glm::degrees(m_SpawnRotation);
            if (ImGui::DragFloat3("Rotation", &rotDeg.x, 1.0f)) {
                m_SpawnRotation = glm::radians(rotDeg);
            }

            ImGui::DragFloat3("Scale", &m_SpawnScale.x, 0.05f, 0.01f, 100.0f);
            ImGui::TreePop();
        }

        // Color
        if (ImGui::TreeNode("Color")) {
            ImGui::ColorEdit4("Main Color", &m_MainColor.x, ImGuiColorEditFlags_AlphaBar);
            ImGui::Checkbox("Use Face Colors", &m_UseFaceColors);

            if (m_UseFaceColors) {
                const char* faces[] = { "Front", "Back", "Right", "Left", "Top", "Bottom" };
                for (int i = 0; i < 6; i++) {
                    ImGui::PushID(i);
                    ImGui::ColorEdit4(faces[i], &m_FaceColors[i].x, ImGuiColorEditFlags_NoInputs);
                    if (i % 2 == 0 && i < 5) ImGui::SameLine(150);
                    ImGui::PopID();
                }
            }
            ImGui::TreePop();
        }

        ImGui::Spacing();

        // Add button with orange accent
        ImGui::PushStyleColor(ImGuiCol_Button, Colors::AccentOrange);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Colors::AccentOrangeHover);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, Colors::AccentOrangeActive);

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

void SceneHierarchyPanel::RenderPrimitivesList() {
    if (!m_Engine) return;

    float listHeight = ImGui::GetContentRegionAvail().y - 30;
    ImGui::BeginChild("PrimitivesList", ImVec2(0, listHeight), true);

    auto& shapes = m_Engine->static_shapes;

    for (size_t i = 0; i < shapes.size(); ++i) {
        StaticMeshData& mesh = shapes[i];

        // Apply search filter
        if (strlen(m_SearchBuffer) > 0 && mesh.name.find(m_SearchBuffer) == std::string::npos) {
            continue;
        }

        // Apply type filter
        if (m_FilterType > 0) {
            PrimitiveType targetType = static_cast<PrimitiveType>(m_FilterType - 1);
            if (mesh.type != targetType) continue;
        }

        ImGui::PushID(static_cast<int>(i));

        // Visibility toggle
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        if (ImGui::Button(mesh.visible ? "[V]" : "[H]", ImVec2(24, 0))) {
            mesh.visible = !mesh.visible;
        }
        ImGui::PopStyleColor();
        ImGui::SameLine();

        // Selection
        bool isSelected = (m_Engine->selectedPrimitiveIndex == static_cast<int>(i));

        if (ImGui::Selectable(mesh.name.c_str(), isSelected)) {
            // Clear previous selection
            if (m_Engine->selectedPrimitiveIndex >= 0 &&
                m_Engine->selectedPrimitiveIndex < static_cast<int>(shapes.size())) {
                shapes[m_Engine->selectedPrimitiveIndex].selected = false;
            }

            // Set new selection
            m_Engine->selectedPrimitiveIndex = static_cast<int>(i);
            m_Engine->selectedNode = nullptr;  // Clear node selection
            EditorSelection::Get().Select(SelectionType::Primitive, static_cast<int>(i), mesh.name);
            mesh.selected = true;
        }

        // Context menu
        if (ImGui::BeginPopupContextItem()) {
            if (ImGui::MenuItem("Delete")) {
                DeletePrimitive(static_cast<int>(i));
            }
            if (ImGui::MenuItem("Duplicate")) {
                DuplicatePrimitive(static_cast<int>(i));
            }
            if (ImGui::MenuItem("Focus Camera")) {
                m_Engine->mainCamera.position = mesh.position + glm::vec3(0, 2, 5);
            }
            ImGui::EndPopup();
        }

        ImGui::PopID();
    }

    ImGui::EndChild();

    // Stats bar
    ImGui::Text("%zu primitives", shapes.size());
}

void SceneHierarchyPanel::RenderSceneObjectsTab() {
    if (!m_Engine) return;

    ImGui::BeginChild("SceneObjects", ImVec2(0, 0), true);

    for (auto& [name, scene] : m_Engine->loadedScenes) {
        if (ImGui::TreeNode(name.c_str())) {
            int nodeIndex = 0;
            for (auto& node : scene->topNodes) {
                RenderNodeRecursive(node, nodeIndex);
            }
            ImGui::TreePop();
        }
    }

    ImGui::EndChild();
}

void SceneHierarchyPanel::RenderNodeRecursive(std::shared_ptr<Node> node, int& nodeIndex) {
    if (!node) return;

    MeshNode* meshNode = dynamic_cast<MeshNode*>(node.get());
    std::string nodeName = meshNode && meshNode->mesh ? meshNode->mesh->name : "Node_" + std::to_string(nodeIndex++);

    bool hasChildren = !node->children.empty();
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;
    if (!hasChildren) flags |= ImGuiTreeNodeFlags_Leaf;

    bool isSelected = (m_Engine->selectedNode == meshNode);
    if (isSelected) flags |= ImGuiTreeNodeFlags_Selected;

    bool opened = ImGui::TreeNodeEx(nodeName.c_str(), flags);

    if (ImGui::IsItemClicked()) {
        m_Engine->selectedNode = meshNode;
        EditorSelection::Get().Select(SelectionType::SceneNode, nodeIndex, nodeName);
    }

    if (opened) {
        for (auto& child : node->children) {
            RenderNodeRecursive(child, nodeIndex);
        }
        ImGui::TreePop();
    }
}

// =============================================================================
// STATIC MESH CACHE - Generated once, reused for all primitives (huge FPS gain)
// =============================================================================
namespace {
    // Mesh type indices
    enum MeshCacheIndex {
        MESH_CUBE = 0,
        MESH_SPHERE = 1,
        MESH_CYLINDER = 2,
        MESH_CONE = 3,
        MESH_PLANE = 4,
        MESH_TRIANGLE = 5,
        MESH_CAPSULE = 6,
        MESH_TORUS = 7,
        MESH_COUNT = 8
    };

    // Static cache arrays
    static GPUMeshBuffers s_MeshCache[MESH_COUNT];
    static bool s_MeshCacheValid[MESH_COUNT] = { false };
}

static GPUMeshBuffers GetCachedMesh(VulkanEngine* engine, int primitiveType) {
    // Return cached mesh if already generated (instant!)
    if (s_MeshCacheValid[primitiveType]) {
        return s_MeshCache[primitiveType];
    }

    // Generate mesh only on first use
    GPUMeshBuffers mesh;
    switch (primitiveType) {
        case MESH_CUBE:     mesh = engine->generate_cube_mesh(); break;
        case MESH_SPHERE:   mesh = engine->generate_sphere_mesh(); break;
        case MESH_CYLINDER: mesh = engine->generate_cylinder_mesh(); break;
        case MESH_CONE:     mesh = engine->generate_cone_mesh(); break;
        case MESH_PLANE:    mesh = engine->generate_plane_mesh(); break;
        case MESH_TRIANGLE: mesh = engine->generate_triangle_mesh(); break;
        case MESH_CAPSULE:  mesh = engine->generate_capsule_mesh(); break;
        case MESH_TORUS:    mesh = engine->generate_torus_mesh(); break;
        default:            mesh = engine->generate_cube_mesh(); break;
    }

    // Cache for future use
    s_MeshCache[primitiveType] = mesh;
    s_MeshCacheValid[primitiveType] = true;
    return mesh;
}

void SceneHierarchyPanel::SpawnPrimitive() {
    if (!m_Engine) return;

    StaticMeshData newMesh;

    const char* names2D[] = { "Triangle", "Plane" };
    const char* names3D[] = { "Cube", "Sphere", "Cylinder", "Cone", "Capsule", "Torus" };

    if (m_ShapeTab == 0) {
        newMesh.name = std::string(names2D[m_Selected2DShape]) + "_" + std::to_string(++m_PrimitiveCounter);
    } else {
        newMesh.name = std::string(names3D[m_Selected3DShape]) + "_" + std::to_string(++m_PrimitiveCounter);
    }

    newMesh.position = m_SpawnPosition;
    newMesh.rotation = m_SpawnRotation;
    newMesh.scale = m_SpawnScale;
    newMesh.mainColor = m_MainColor;
    newMesh.useFaceColors = m_UseFaceColors;
    for (int i = 0; i < 6; i++) {
        newMesh.faceColors[i] = m_FaceColors[i];
    }

    // Use cached meshes - no GPU buffer creation after first use!
    if (m_ShapeTab == 0) {
        switch (m_Selected2DShape) {
            case 0: newMesh.type = PrimitiveType::Triangle; newMesh.mesh = GetCachedMesh(m_Engine, MESH_TRIANGLE); break;
            case 1: newMesh.type = PrimitiveType::Plane; newMesh.mesh = GetCachedMesh(m_Engine, MESH_PLANE); break;
        }
    } else {
        switch (m_Selected3DShape) {
            case 0: newMesh.type = PrimitiveType::Cube; newMesh.mesh = GetCachedMesh(m_Engine, MESH_CUBE); break;
            case 1: newMesh.type = PrimitiveType::Sphere; newMesh.mesh = GetCachedMesh(m_Engine, MESH_SPHERE); break;
            case 2: newMesh.type = PrimitiveType::Cylinder; newMesh.mesh = GetCachedMesh(m_Engine, MESH_CYLINDER); break;
            case 3: newMesh.type = PrimitiveType::Cone; newMesh.mesh = GetCachedMesh(m_Engine, MESH_CONE); break;
            case 4: newMesh.type = PrimitiveType::Capsule; newMesh.mesh = GetCachedMesh(m_Engine, MESH_CAPSULE); break;
            case 5: newMesh.type = PrimitiveType::Torus; newMesh.mesh = GetCachedMesh(m_Engine, MESH_TORUS); break;
        }
    }

    newMesh.materialType = ShaderOnlyMaterial::DEFAULT;
    newMesh.passType = MaterialPass::MainColor;
    newMesh.visible = true;
    newMesh.selected = false;

    m_Engine->static_shapes.push_back(newMesh);
    m_SpawnPosition.x += 2.5f;  // Offset next spawn
}

void SceneHierarchyPanel::DeletePrimitive(int index) {
    if (!m_Engine || index < 0 || index >= static_cast<int>(m_Engine->static_shapes.size())) return;

    m_Engine->static_shapes.erase(m_Engine->static_shapes.begin() + index);

    // Update selection
    if (m_Engine->selectedPrimitiveIndex == index) {
        m_Engine->selectedPrimitiveIndex = -1;
        EditorSelection::Get().ClearSelection();
    } else if (m_Engine->selectedPrimitiveIndex > index) {
        // Adjust index if deleted item was before selected
        m_Engine->selectedPrimitiveIndex--;
    }
}

void SceneHierarchyPanel::DuplicatePrimitive(int index) {
    if (!m_Engine || index < 0 || index >= static_cast<int>(m_Engine->static_shapes.size())) return;

    StaticMeshData& original = m_Engine->static_shapes[index];
    StaticMeshData copy = original;
    copy.name = original.name + "_copy";
    copy.position += glm::vec3(1.0f, 0, 0);
    copy.selected = false;

    m_Engine->static_shapes.push_back(copy);
}

const char* SceneHierarchyPanel::GetPrimitiveTypeName(int type) {
    const char* names[] = { "Cube", "Sphere", "Cylinder", "Cone", "Plane", "Triangle", "Capsule", "Torus" };
    if (type >= 0 && type < 8) return names[type];
    return "Unknown";
}

const char* SceneHierarchyPanel::GetPrimitiveIcon(int type) {
    return "[*]";  // Simple placeholder
}

} // namespace Yalaz::UI
