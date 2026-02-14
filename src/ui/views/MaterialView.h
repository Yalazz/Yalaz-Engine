#pragma once
// =============================================================================
// YALAZ ENGINE - Material View
// =============================================================================
// Dynamic material editor supporting both primitives and GLTF materials:
// - PBR properties editing (color, metallic, roughness)
// - Dynamic texture slot loading (drag-drop from Asset Browser)
// - Real-time GPU buffer updates for GLTF materials
// - Material presets
// - Live preview
// - Save/Load .mat files (JSON format)
// =============================================================================

#include "EditorView.h"
#include "../../assets/MaterialFile.h"
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <memory>

// Forward declarations
struct MeshNode;
struct MeshAsset;
struct GLTFMaterial;
struct GeoSurface;

namespace Yalaz::UI {

// Texture slot for material
struct TextureSlot {
    std::string name;
    std::string path;
    bool isLoaded = false;
    uint32_t textureID = 0;  // TextureCache ID
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

// Selection type
enum class MaterialSelectionType {
    None,
    Primitive,      // static_shapes primitive
    GLTFMaterial    // GLTF model material
};

class MaterialView : public EditorView {
public:
    MaterialView() : EditorView("Material Editor", "[M]", ViewCategory::Graphics) {}

    void OnInit(VulkanEngine* engine) override;
    void OnRender() override;

    // Public API for texture assignment
    void SetAlbedoTexture(const std::string& path);
    void SetNormalTexture(const std::string& path);
    void SetMetallicRoughnessTexture(const std::string& path);
    void SetAOTexture(const std::string& path);
    void SetEmissionTexture(const std::string& path);
    void SetDisplacementTexture(const std::string& path);

    // Scene lifecycle - call when a scene is being unloaded
    void OnSceneUnloading(const std::string& sceneName);
    void ClearGLTFSelection();

private:
    // Rendering sections
    void RenderMaterialProperties();
    void RenderTextureSlots();
    void RenderPreview();
    void RenderPresets();
    void RenderTextureSlot(const char* label, TextureSlot& slot, const char* payloadType);
    void RenderGLTFMaterialList();

    // Initialization
    void InitPresets();

    // Selection handling
    void SyncWithSelection();
    void ApplyToSelection();
    void ApplyToGLTFMaterial();

    // GLTF material helpers
    void LoadGLTFMaterialData();
    void UpdateGLTFMaterialBuffer();

    // Dynamic texture loading system
    bool LoadTextureFromFile(const std::string& texturePath, uint32_t& outTextureID);
    void UpdateMaterialTexture(uint32_t colorTexID, uint32_t metalRoughTexID, uint32_t normalTexID, uint32_t emissiveTexID, uint32_t displacementTexID, uint32_t aoTexID);
    void RebuildMaterialDescriptorSet();
    void ApplyTexturesToPrimitive();  // Apply textures to selected primitive

    // Material file save/load
    void RenderSaveLoadUI();
    void SaveMaterialToFile();
    void LoadMaterialFromFile();
    MaterialFile::MaterialData GetCurrentMaterialData() const;
    void ApplyMaterialData(const MaterialFile::MaterialData& data);

    // Current selection type
    MaterialSelectionType m_SelectionType = MaterialSelectionType::None;

    // GLTF material tracking
    MeshNode* m_SelectedMeshNode = nullptr;
    int m_SelectedSurfaceIndex = 0;  // Which surface/material in the mesh
    std::shared_ptr<GLTFMaterial> m_CurrentGLTFMaterial;

    // PBR Material properties (works for both primitive and GLTF)
    glm::vec4 m_BaseColor = glm::vec4(1.0f);
    float m_Metallic = 0.0f;
    float m_Roughness = 0.5f;
    float m_AO = 1.0f;
    glm::vec3 m_Emission = glm::vec3(0.0f);
    float m_EmissionStrength = 0.0f;
    float m_NormalStrength = 1.0f;
    float m_DisplacementScale = 0.0f;
    float m_DisplacementBias = 0.0f;

    // Texture slots (dynamic loading)
    TextureSlot m_AlbedoSlot{"Albedo", "", false, 0};
    TextureSlot m_NormalSlot{"Normal", "", false, 0};
    TextureSlot m_MetallicRoughnessSlot{"Metallic/Roughness", "", false, 0};
    TextureSlot m_AOSlot{"Ambient Occlusion", "", false, 0};
    TextureSlot m_EmissionSlot{"Emission", "", false, 0};
    TextureSlot m_DisplacementSlot{"Displacement", "", false, 0};

    // Material presets
    std::vector<MaterialPreset> m_Presets;
    int m_SelectedPreset = -1;

    // Preview
    bool m_AutoRotate = true;
    float m_PreviewRotation = 0.0f;
    int m_PreviewMesh = 0;  // 0=Sphere, 1=Cube, 2=Plane

    // UI state
    bool m_ShowPresets = true;
    bool m_ShowMaterialList = true;
    int m_LastSelectedPrimitiveIndex = -1;
    MeshNode* m_LastSelectedMeshNode = nullptr;

    // Color picker state
    bool m_ShowFullColorPicker = true;
    int m_ColorPickerMode = 0; // 0=Wheel, 1=Bar
    static constexpr int MAX_COLOR_HISTORY = 24;
    glm::vec4 m_ColorHistory[MAX_COLOR_HISTORY] = {};
    int m_ColorHistoryCount = 0;
    bool m_ColorPickerActive = false; // track if user is currently picking

    // Favorites
    static constexpr int MAX_FAVORITES = 32;
    glm::vec4 m_FavoriteColors[MAX_FAVORITES] = {};
    int m_FavoriteCount = 0;

    // Palette category filter
    int m_PaletteCategory = 0; // 0=All, 1=Basic, 2=Pastel, 3=Earth, 4=Neon, 5=Gray, 6=Skin, 7=Metal, 8=Nature

    // Color harmony
    int m_HarmonyMode = 0; // 0=None, 1=Complementary, 2=Analogous, 3=Triadic, 4=Split-Comp

    // Material file state
    std::string m_CurrentMaterialPath;
    std::string m_MaterialName = "Unnamed Material";
    std::string m_MaterialCategory;
    std::string m_MaterialTags;
    bool m_ShowSaveLoadDialog = false;
    bool m_IsLoadDialog = false;
    char m_FilePathBuffer[512] = "";
    char m_MaterialNameBuffer[128] = "";
};

} // namespace Yalaz::UI
