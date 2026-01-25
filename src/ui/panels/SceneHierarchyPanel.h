#pragma once
// =============================================================================
// YALAZ ENGINE - Scene Hierarchy Panel
// =============================================================================
// Unified panel combining primitive spawner and scene object list
// =============================================================================

#include "../Panel.h"
#include <glm/glm.hpp>
#include <string>
#include <memory>

class VulkanEngine;
struct Node;

namespace Yalaz::UI {

class SceneHierarchyPanel : public Panel {
public:
    SceneHierarchyPanel();

    void OnInit(VulkanEngine* engine) override;
    void OnRender() override;

private:
    // UI State
    int m_ActiveTab = 0;            // 0 = Primitives, 1 = Scene Objects
    char m_SearchBuffer[128] = "";
    int m_FilterType = 0;           // 0 = All, 1 = Cube, 2 = Sphere, etc.
    bool m_CreateSectionOpen = true;

    // Spawn settings
    int m_ShapeTab = 1;             // 0 = 2D, 1 = 3D
    int m_Selected2DShape = 0;
    int m_Selected3DShape = 0;
    int m_PrimitiveCounter = 0;

    glm::vec3 m_SpawnPosition = glm::vec3(0.0f, 0.0f, -5.0f);
    glm::vec3 m_SpawnRotation = glm::vec3(0.0f);
    glm::vec3 m_SpawnScale = glm::vec3(1.0f);
    glm::vec4 m_MainColor = glm::vec4(1.0f);
    bool m_UseFaceColors = false;
    glm::vec4 m_FaceColors[6];

    // Render sections
    void RenderTabs();
    void RenderPrimitivesTab();
    void RenderCreateSection();
    void RenderPrimitivesList();
    void RenderSceneObjectsTab();
    void RenderNodeRecursive(std::shared_ptr<Node> node, int& nodeIndex);

    // Actions
    void SpawnPrimitive();
    void DeletePrimitive(int index);
    void DuplicatePrimitive(int index);

    // Helpers
    const char* GetPrimitiveTypeName(int type);
    const char* GetPrimitiveIcon(int type);
};

} // namespace Yalaz::UI
