#pragma once
// =============================================================================
// YALAZ ENGINE - Object Inspector View
// =============================================================================
// Professional property inspector with ALL features:
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

#include "EditorView.h"
#include "HierarchyView.h"  // For PrimitiveFaceConfig

namespace Yalaz::UI {

class ObjectInspectorView : public EditorView {
public:
    ObjectInspectorView() : EditorView("Inspector", "[I]", ViewCategory::Core) {}
    void OnRender() override;

private:
    void RenderNoSelection();
    void RenderPrimitiveInspector(int index);
    void RenderLightInspector(int index);
    void RenderSceneNodeInspector();

    // Editor helpers
    void RenderTransformEditor(glm::vec3& position, glm::vec3& rotation, glm::vec3& scale);
    void RenderColorEditor(const char* label, glm::vec3& color);
    void RenderColorEditor4(const char* label, glm::vec4& color);

    // Dynamic face color editor for ANY primitive type
    void RenderDynamicFaceColorEditor(int primitiveType, glm::vec4* faceColors, bool& useFaceColors);

    // Statistics helper
    void RenderPrimitiveStatistics(int primitiveType);

    // Name editing buffer
    char m_NameBuffer[256] = "";
    int m_LastEditedIndex = -1;
};

} // namespace Yalaz::UI
