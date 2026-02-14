#include "MaterialFile.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <fmt/core.h>

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace MaterialFile {

// JSON serialization helpers
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(glm::vec3, x, y, z)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(glm::vec4, x, y, z, w)

// Custom serialization for AlphaMode enum
static std::string alphaModeToString(MaterialData::AlphaMode mode) {
    switch (mode) {
        case MaterialData::AlphaMode::Opaque: return "opaque";
        case MaterialData::AlphaMode::Mask: return "mask";
        case MaterialData::AlphaMode::Blend: return "blend";
        default: return "opaque";
    }
}

static MaterialData::AlphaMode stringToAlphaMode(const std::string& str) {
    if (str == "mask") return MaterialData::AlphaMode::Mask;
    if (str == "blend") return MaterialData::AlphaMode::Blend;
    return MaterialData::AlphaMode::Opaque;
}

bool save(const std::string& filepath, const MaterialData& material) {
    try {
        json j;

        // Header
        j["version"] = material.version;
        j["name"] = material.name;

        // Base Color
        j["baseColor"] = {
            {"r", material.baseColor.r},
            {"g", material.baseColor.g},
            {"b", material.baseColor.b},
            {"a", material.baseColor.a}
        };
        if (!material.albedoTexturePath.empty()) {
            j["albedoTexture"] = material.albedoTexturePath;
        }

        // PBR
        j["pbr"] = {
            {"metallic", material.metallic},
            {"roughness", material.roughness},
            {"ao", material.ao}
        };
        if (!material.metallicRoughnessTexturePath.empty()) {
            j["metallicRoughnessTexture"] = material.metallicRoughnessTexturePath;
        }

        // Normal
        if (!material.normalTexturePath.empty()) {
            j["normalTexture"] = material.normalTexturePath;
            j["normalScale"] = material.normalScale;
        }

        // Displacement
        if (!material.displacementTexturePath.empty()) {
            j["displacementTexture"] = material.displacementTexturePath;
            j["displacementScale"] = material.displacementScale;
            j["displacementBias"] = material.displacementBias;
        }

        // Emission
        if (material.emissionStrength > 0.0f || !material.emissionTexturePath.empty()) {
            j["emission"] = {
                {"color", {
                    {"r", material.emissionColor.r},
                    {"g", material.emissionColor.g},
                    {"b", material.emissionColor.b}
                }},
                {"strength", material.emissionStrength}
            };
            if (!material.emissionTexturePath.empty()) {
                j["emissionTexture"] = material.emissionTexturePath;
            }
        }

        // Alpha
        j["alpha"] = {
            {"mode", alphaModeToString(material.alphaMode)},
            {"cutoff", material.alphaCutoff}
        };

        // Rendering
        j["rendering"] = {
            {"doubleSided", material.doubleSided},
            {"castShadows", material.castShadows},
            {"receiveShadows", material.receiveShadows}
        };

        // Tags
        if (!material.category.empty()) {
            j["category"] = material.category;
        }
        if (!material.tags.empty()) {
            j["tags"] = material.tags;
        }

        // Ensure directory exists
        fs::path path(filepath);
        if (path.has_parent_path()) {
            fs::create_directories(path.parent_path());
        }

        // Write to file with pretty formatting
        std::ofstream file(filepath);
        if (!file.is_open()) {
            fmt::print("[MaterialFile] Error: Could not open file for writing: {}\n", filepath);
            return false;
        }

        file << j.dump(4);  // 4-space indentation
        file.close();

        fmt::print("[MaterialFile] Saved material: {} -> {}\n", material.name, filepath);
        return true;
    }
    catch (const std::exception& e) {
        fmt::print("[MaterialFile] Error saving material: {}\n", e.what());
        return false;
    }
}

std::optional<MaterialData> load(const std::string& filepath) {
    try {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            fmt::print("[MaterialFile] Error: Could not open file: {}\n", filepath);
            return std::nullopt;
        }

        json j;
        file >> j;
        file.close();

        MaterialData material;

        // Header
        material.version = j.value("version", "1.0");
        material.name = j.value("name", "Unnamed Material");

        // Base Color
        if (j.contains("baseColor")) {
            auto& bc = j["baseColor"];
            material.baseColor = glm::vec4(
                bc.value("r", 1.0f),
                bc.value("g", 1.0f),
                bc.value("b", 1.0f),
                bc.value("a", 1.0f)
            );
        }
        material.albedoTexturePath = j.value("albedoTexture", "");

        // PBR
        if (j.contains("pbr")) {
            auto& pbr = j["pbr"];
            material.metallic = pbr.value("metallic", 0.0f);
            material.roughness = pbr.value("roughness", 0.5f);
            material.ao = pbr.value("ao", 1.0f);
        }
        material.metallicRoughnessTexturePath = j.value("metallicRoughnessTexture", "");

        // Normal
        material.normalTexturePath = j.value("normalTexture", "");
        material.normalScale = j.value("normalScale", 1.0f);
        material.displacementTexturePath = j.value("displacementTexture", "");
        material.displacementScale = j.value("displacementScale", 0.0f);
        material.displacementBias = j.value("displacementBias", 0.0f);

        // Emission
        if (j.contains("emission")) {
            auto& em = j["emission"];
            if (em.contains("color")) {
                auto& ec = em["color"];
                material.emissionColor = glm::vec3(
                    ec.value("r", 0.0f),
                    ec.value("g", 0.0f),
                    ec.value("b", 0.0f)
                );
            }
            material.emissionStrength = em.value("strength", 0.0f);
        }
        material.emissionTexturePath = j.value("emissionTexture", "");

        // Alpha
        if (j.contains("alpha")) {
            auto& alpha = j["alpha"];
            material.alphaMode = stringToAlphaMode(alpha.value("mode", "opaque"));
            material.alphaCutoff = alpha.value("cutoff", 0.5f);
        }

        // Rendering
        if (j.contains("rendering")) {
            auto& render = j["rendering"];
            material.doubleSided = render.value("doubleSided", false);
            material.castShadows = render.value("castShadows", true);
            material.receiveShadows = render.value("receiveShadows", true);
        }

        // Tags
        material.category = j.value("category", "");
        material.tags = j.value("tags", "");

        fmt::print("[MaterialFile] Loaded material: {} from {}\n", material.name, filepath);
        return material;
    }
    catch (const std::exception& e) {
        fmt::print("[MaterialFile] Error loading material: {}\n", e.what());
        return std::nullopt;
    }
}

std::string getPreviewPath(const std::string& materialPath) {
    fs::path path(materialPath);
    fs::path previewDir = path.parent_path() / ".previews";
    fs::path previewFile = previewDir / (path.stem().string() + "_preview.png");
    return previewFile.string();
}

bool isValidMaterialFile(const std::string& filepath) {
    if (!fs::exists(filepath)) return false;

    fs::path path(filepath);
    if (path.extension() != ".mat") return false;

    // Try to parse as JSON
    try {
        std::ifstream file(filepath);
        if (!file.is_open()) return false;

        json j;
        file >> j;

        // Check for required fields
        return j.contains("name") || j.contains("baseColor") || j.contains("pbr");
    }
    catch (...) {
        return false;
    }
}

MaterialData createDefault(const std::string& name) {
    MaterialData material;
    material.name = name;
    material.version = "1.0";
    material.baseColor = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);  // Light gray
    material.metallic = 0.0f;
    material.roughness = 0.5f;
    material.ao = 1.0f;
    material.alphaMode = MaterialData::AlphaMode::Opaque;
    material.doubleSided = false;
    material.castShadows = true;
    material.receiveShadows = true;
    return material;
}

} // namespace MaterialFile
