#pragma once
// =============================================================================
// YALAZ ENGINE - Hierarchy View
// =============================================================================
// Professional scene hierarchy with ALL features:
// - Spawn settings (transform, colors, dynamic face colors per type)
// - Quick create (in front of camera)
// - Tree view of scenes, primitives, and lights
// - Search and type filtering
// - Context menus with Focus Camera
// - Stats bar
// =============================================================================

#include "EditorView.h"
#include <vector>
#include <string>
#include <glm/glm.hpp>

namespace Yalaz::UI {

// Dynamic face configuration per primitive type
struct PrimitiveFaceConfig {
    int faceCount;
    const char* faceNames[8];  // Max 8 faces
    glm::vec4 defaultColors[8];
};

class HierarchyView : public EditorView {
public:
    HierarchyView();
    void OnRender() override;

    // Get face configuration for a primitive type
    static const PrimitiveFaceConfig& GetFaceConfig(int primitiveType);

private:
    // Render sections
    void RenderCreateSection();
    void RenderSceneNodes();
    void RenderPrimitives();
    void RenderLights();
    void RenderCreateMenu();

    // Spawn methods
    void SpawnPrimitive();                              // Uses spawn settings
    void SpawnPrimitiveQuick(int type, const char* name); // In front of camera

    // Dynamic face color UI
    void RenderFaceColorEditor(int primitiveType);

    // Filters
    char m_SearchBuffer[256] = "";
    int m_FilterType = 0;           // 0 = All, 1 = Cube, etc.
    bool m_ShowPrimitives = true;
    bool m_ShowLights = true;
    bool m_ShowNodes = true;

    // Spawn settings
    int m_ShapeTab = 1;             // 0=2D, 1=3D
    int m_Selected3DShape = 0;
    int m_Selected2DShape = 0;
    bool m_CreateSectionOpen = true;

    glm::vec3 m_SpawnPosition = glm::vec3(0.0f);
    glm::vec3 m_SpawnRotation = glm::vec3(0.0f);
    glm::vec3 m_SpawnScale = glm::vec3(1.0f);
    glm::vec4 m_MainColor = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);
    bool m_UseFaceColors = false;
    glm::vec4 m_FaceColors[8];      // Dynamic based on type

    int m_PrimitiveCounter = 0;
};

} // namespace Yalaz::UI
