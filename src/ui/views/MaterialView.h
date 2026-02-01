#pragma once
// =============================================================================
// YALAZ ENGINE - Material View
// =============================================================================
// Dynamic material editor with:
// - PBR properties editing
// - Dynamic texture slot loading (drag-drop)
// - Multiple texture channels
// - Material presets
// - Live preview
// =============================================================================

#include "EditorView.h"
#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace Yalaz::UI {

// Texture slot for material
struct TextureSlot {
    std::string name;
    std::string path;
    bool isLoaded = false;
    // Would contain VkImageView in real implementation
};

// Material preset
struct MaterialPreset {
    std::string name;
    glm::vec4 baseColor;
    float metallic;
    float roughness;
    float ao;
    glm::vec3 emission;
};

class MaterialView : public EditorView {
public:
    MaterialView() : EditorView("Material Editor", "[M]", ViewCategory::Graphics) {}

    void OnInit(VulkanEngine* engine) override;
    void OnRender() override;

    // Public API
    void SetAlbedoTexture(const std::string& path);
    void SetNormalTexture(const std::string& path);
    void SetMetallicRoughnessTexture(const std::string& path);
    void SetAOTexture(const std::string& path);
    void SetEmissionTexture(const std::string& path);

private:
    void RenderMaterialProperties();
    void RenderTextureSlots();
    void RenderPreview();
    void RenderPresets();
    void RenderTextureSlot(const char* label, TextureSlot& slot, const char* payloadType);
    void InitPresets();
    void SyncWithSelection();
    void ApplyToSelection();

    // PBR Material properties
    glm::vec4 m_BaseColor = glm::vec4(1.0f);
    float m_Metallic = 0.0f;
    float m_Roughness = 0.5f;
    float m_AO = 1.0f;
    glm::vec3 m_Emission = glm::vec3(0.0f);
    float m_EmissionStrength = 0.0f;
    float m_NormalStrength = 1.0f;

    // Texture slots (dynamic loading)
    TextureSlot m_AlbedoSlot{"Albedo", "", false};
    TextureSlot m_NormalSlot{"Normal", "", false};
    TextureSlot m_MetallicRoughnessSlot{"Metallic/Roughness", "", false};
    TextureSlot m_AOSlot{"Ambient Occlusion", "", false};
    TextureSlot m_EmissionSlot{"Emission", "", false};

    // Material presets
    std::vector<MaterialPreset> m_Presets;
    int m_SelectedPreset = -1;

    // Preview
    bool m_AutoRotate = true;
    float m_PreviewRotation = 0.0f;
    int m_PreviewMesh = 0;  // 0=Sphere, 1=Cube, 2=Plane

    // UI state
    bool m_ShowPresets = true;
    int m_LastSelectedIndex = -1;
};

} // namespace Yalaz::UI
