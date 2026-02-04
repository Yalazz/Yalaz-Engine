#pragma once

#include <string>
#include <glm/glm.hpp>
#include <optional>

// =============================================================================
// MATERIAL FILE SYSTEM - Persistent .mat files with JSON serialization
// =============================================================================
// Provides save/load functionality for materials, including:
// - PBR parameters (base color, metallic, roughness)
// - Texture paths (albedo, metallic-roughness, normal, emission)
// - Additional properties (emission color, alpha mode, etc.)
// =============================================================================

namespace MaterialFile {

// Material data structure for serialization
struct MaterialData {
    // === Identification ===
    std::string name = "Unnamed Material";
    std::string version = "1.0";

    // === Base Color ===
    glm::vec4 baseColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);  // RGBA
    std::string albedoTexturePath;  // Empty = use baseColor only

    // === PBR Parameters ===
    float metallic = 0.0f;    // 0.0 = dielectric, 1.0 = metal
    float roughness = 0.5f;   // 0.0 = smooth, 1.0 = rough
    float ao = 1.0f;          // Ambient occlusion multiplier
    std::string metallicRoughnessTexturePath;  // G=roughness, B=metallic (GLTF spec)

    // === Normal Mapping ===
    std::string normalTexturePath;
    float normalScale = 1.0f;

    // === Emission ===
    glm::vec3 emissionColor = glm::vec3(0.0f);
    float emissionStrength = 0.0f;
    std::string emissionTexturePath;

    // === Alpha Mode ===
    enum class AlphaMode { Opaque, Mask, Blend };
    AlphaMode alphaMode = AlphaMode::Opaque;
    float alphaCutoff = 0.5f;

    // === Rendering ===
    bool doubleSided = false;
    bool castShadows = true;
    bool receiveShadows = true;

    // === Tags/Categories ===
    std::string category;  // e.g., "Metal", "Wood", "Fabric"
    std::string tags;      // Comma-separated tags for search
};

// Save material to .mat file (JSON format)
bool save(const std::string& filepath, const MaterialData& material);

// Load material from .mat file
std::optional<MaterialData> load(const std::string& filepath);

// Get material preview thumbnail path (auto-generated)
std::string getPreviewPath(const std::string& materialPath);

// Check if a file is a valid material file
bool isValidMaterialFile(const std::string& filepath);

// Create a default material file
MaterialData createDefault(const std::string& name = "New Material");

// Convert MaterialData to engine's MaterialConstants
// (Defined in vk_engine.cpp to avoid circular dependencies)

} // namespace MaterialFile
