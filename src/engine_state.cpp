#include "engine_state.h"
#include "vk_engine.h"
#include "vk_loader.h"
#include "ui/EditorSelection.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <fmt/core.h>

using json = nlohmann::json;

// ================================================================
// JSON helpers for glm types
// ================================================================

static json vec3_to_json(const glm::vec3& v) {
    return json::array({v.x, v.y, v.z});
}

static json vec4_to_json(const glm::vec4& v) {
    return json::array({v.x, v.y, v.z, v.w});
}

static glm::vec3 json_to_vec3(const json& j) {
    return glm::vec3(j[0].get<float>(), j[1].get<float>(), j[2].get<float>());
}

static glm::vec4 json_to_vec4(const json& j) {
    return glm::vec4(j[0].get<float>(), j[1].get<float>(), j[2].get<float>(), j[3].get<float>());
}

// ================================================================
// Generate GPU mesh for a given primitive type
// ================================================================

static GPUMeshBuffers generateMeshForType(VulkanEngine& engine, PrimitiveType type) {
    switch (type) {
        case PrimitiveType::Cube:     return engine.generate_cube_mesh();
        case PrimitiveType::Sphere:   return engine.generate_sphere_mesh();
        case PrimitiveType::Capsule:  return engine.generate_capsule_mesh();
        case PrimitiveType::Cylinder: return engine.generate_cylinder_mesh();
        case PrimitiveType::Plane:    return engine.generate_plane_mesh();
        case PrimitiveType::Cone:     return engine.generate_cone_mesh();
        case PrimitiveType::Torus:    return engine.generate_torus_mesh();
        case PrimitiveType::Triangle: return engine.generate_triangle_mesh();
        default:                      return engine.generate_cube_mesh();
    }
}

// ================================================================
// Serialize camera
// ================================================================

static json serializeCamera(const Camera& cam) {
    json j;
    j["position"]         = vec3_to_json(cam.position);
    j["pitch"]            = cam.pitch;
    j["yaw"]              = cam.yaw;
    j["fov"]              = cam.fov;
    j["targetFov"]        = cam.targetFov;
    j["nearPlane"]        = cam.nearPlane;
    j["farPlane"]         = cam.farPlane;
    j["moveSpeed"]        = cam.moveSpeed;
    j["sprintMultiplier"] = cam.sprintMultiplier;
    j["mouseSensitivity"] = cam.mouseSensitivity;
    j["panSensitivity"]   = cam.panSensitivity;
    j["scrollZoomSpeed"]  = cam.scrollZoomSpeed;
    j["smoothing"]        = cam.smoothing;
    j["fovSmoothSpeed"]   = cam.fovSmoothSpeed;
    j["focusDistance"]     = cam.focusDistance;
    j["focusSmoothSpeed"] = cam.focusSmoothSpeed;
    return j;
}

static void deserializeCamera(Camera& cam, const json& j) {
    if (j.contains("position"))         cam.position         = json_to_vec3(j["position"]);
    if (j.contains("pitch"))            cam.pitch            = j["pitch"].get<float>();
    if (j.contains("yaw"))              cam.yaw              = j["yaw"].get<float>();
    if (j.contains("fov"))              cam.fov              = j["fov"].get<float>();
    if (j.contains("targetFov"))        cam.targetFov        = j["targetFov"].get<float>();
    if (j.contains("nearPlane"))        cam.nearPlane        = j["nearPlane"].get<float>();
    if (j.contains("farPlane"))         cam.farPlane         = j["farPlane"].get<float>();
    if (j.contains("moveSpeed"))        cam.moveSpeed        = j["moveSpeed"].get<float>();
    if (j.contains("sprintMultiplier")) cam.sprintMultiplier = j["sprintMultiplier"].get<float>();
    if (j.contains("mouseSensitivity")) cam.mouseSensitivity = j["mouseSensitivity"].get<float>();
    if (j.contains("panSensitivity"))   cam.panSensitivity   = j["panSensitivity"].get<float>();
    if (j.contains("scrollZoomSpeed"))  cam.scrollZoomSpeed  = j["scrollZoomSpeed"].get<float>();
    if (j.contains("smoothing"))        cam.smoothing        = j["smoothing"].get<float>();
    if (j.contains("fovSmoothSpeed"))   cam.fovSmoothSpeed   = j["fovSmoothSpeed"].get<float>();
    if (j.contains("focusDistance"))     cam.focusDistance     = j["focusDistance"].get<float>();
    if (j.contains("focusSmoothSpeed")) cam.focusSmoothSpeed = j["focusSmoothSpeed"].get<float>();

    cam.focusActive = false;
    cam.updateProjectionMatrix();
}

// ================================================================
// Serialize lighting
// ================================================================

static json serializeLighting(const GPUSceneData& sd) {
    json j;
    j["ambient"]      = vec4_to_json(sd.ambientColor);
    j["sunDirection"]  = vec4_to_json(sd.sunlightDirection);
    j["sunColor"]      = vec4_to_json(sd.sunlightColor);
    return j;
}

static void deserializeLighting(GPUSceneData& sd, const json& j) {
    if (j.contains("ambient"))     sd.ambientColor      = json_to_vec4(j["ambient"]);
    if (j.contains("sunDirection")) sd.sunlightDirection = json_to_vec4(j["sunDirection"]);
    if (j.contains("sunColor"))    sd.sunlightColor     = json_to_vec4(j["sunColor"]);
}

// ================================================================
// Serialize point lights
// ================================================================

static json serializePointLights(const std::vector<PointLight>& lights) {
    json arr = json::array();
    for (const auto& l : lights) {
        json j;
        j["position"]  = vec3_to_json(l.position);
        j["radius"]    = l.radius;
        j["color"]     = vec3_to_json(l.color);
        j["intensity"] = l.intensity;
        arr.push_back(j);
    }
    return arr;
}

static void deserializePointLights(std::vector<PointLight>& lights, const json& arr) {
    lights.clear();
    for (const auto& j : arr) {
        PointLight l{};
        if (j.contains("position"))  l.position  = json_to_vec3(j["position"]);
        if (j.contains("radius"))    l.radius    = j["radius"].get<float>();
        if (j.contains("color"))     l.color     = json_to_vec3(j["color"]);
        if (j.contains("intensity")) l.intensity = j["intensity"].get<float>();
        lights.push_back(l);
    }
}

// ================================================================
// Serialize primitives (static_shapes)
// ================================================================

static json serializePrimitives(const std::vector<StaticMeshData>& shapes) {
    json arr = json::array();
    for (const auto& s : shapes) {
        json j;
        j["name"]          = s.name;
        j["type"]          = static_cast<int>(s.type);
        j["position"]      = vec3_to_json(s.position);
        j["rotation"]      = vec3_to_json(s.rotation);
        j["scale"]         = vec3_to_json(s.scale);
        j["mainColor"]     = vec4_to_json(s.mainColor);
        j["useFaceColors"] = s.useFaceColors;
        j["visible"]       = s.visible;
        j["materialType"]  = static_cast<int>(s.materialType);
        j["passType"]      = static_cast<int>(s.passType);

        // PBR properties
        j["metallic"]            = s.metallic;
        j["roughness"]           = s.roughness;
        j["emission"]            = vec3_to_json(s.emission);
        j["reflectionIntensity"] = s.reflectionIntensity;

        // Texture paths
        if (!s.albedoTexturePath.empty())    j["albedoTexturePath"]    = s.albedoTexturePath;
        if (!s.metalRoughTexturePath.empty()) j["metalRoughTexturePath"] = s.metalRoughTexturePath;
        if (!s.emissionTexturePath.empty())  j["emissionTexturePath"]  = s.emissionTexturePath;

        json fc = json::array();
        for (int i = 0; i < 6; i++) {
            fc.push_back(vec4_to_json(s.faceColors[i]));
        }
        j["faceColors"] = fc;

        arr.push_back(j);
    }
    return arr;
}

static void deserializePrimitives(VulkanEngine& engine, std::vector<StaticMeshData>& shapes, const json& arr) {
    shapes.clear();

    // Cache generated meshes per type to avoid redundant GPU uploads
    std::unordered_map<int, GPUMeshBuffers> meshCache;

    for (const auto& j : arr) {
        StaticMeshData s{};
        if (j.contains("name"))          s.name          = j["name"].get<std::string>();
        if (j.contains("type"))          s.type          = static_cast<PrimitiveType>(j["type"].get<int>());
        if (j.contains("position"))      s.position      = json_to_vec3(j["position"]);
        if (j.contains("rotation"))      s.rotation      = json_to_vec3(j["rotation"]);
        if (j.contains("scale"))         s.scale         = json_to_vec3(j["scale"]);
        if (j.contains("mainColor"))     s.mainColor     = json_to_vec4(j["mainColor"]);
        if (j.contains("useFaceColors")) s.useFaceColors = j["useFaceColors"].get<bool>();
        if (j.contains("visible"))       s.visible       = j["visible"].get<bool>();
        if (j.contains("materialType"))  s.materialType  = static_cast<ShaderOnlyMaterial>(j["materialType"].get<int>());
        if (j.contains("passType"))      s.passType      = static_cast<MaterialPass>(j["passType"].get<int>());

        // PBR properties
        if (j.contains("metallic"))            s.metallic            = j["metallic"].get<float>();
        if (j.contains("roughness"))           s.roughness           = j["roughness"].get<float>();
        if (j.contains("emission"))            s.emission            = json_to_vec3(j["emission"]);
        if (j.contains("reflectionIntensity")) s.reflectionIntensity = j["reflectionIntensity"].get<float>();

        // Texture paths
        if (j.contains("albedoTexturePath"))    s.albedoTexturePath    = j["albedoTexturePath"].get<std::string>();
        if (j.contains("metalRoughTexturePath")) s.metalRoughTexturePath = j["metalRoughTexturePath"].get<std::string>();
        if (j.contains("emissionTexturePath"))  s.emissionTexturePath  = j["emissionTexturePath"].get<std::string>();

        if (j.contains("faceColors")) {
            const auto& fc = j["faceColors"];
            for (int i = 0; i < 6 && i < static_cast<int>(fc.size()); i++) {
                s.faceColors[i] = json_to_vec4(fc[i]);
            }
        }

        s.selected = false;

        // Generate or reuse cached GPU mesh
        int typeIdx = static_cast<int>(s.type);
        if (meshCache.find(typeIdx) == meshCache.end()) {
            meshCache[typeIdx] = generateMeshForType(engine, s.type);
        }
        s.mesh = meshCache[typeIdx];

        shapes.push_back(s);
    }
}

// ================================================================
// Serialize spot lights
// ================================================================

static json serializeSpotLights(const std::vector<SpotLight>& lights) {
    json arr = json::array();
    for (const auto& l : lights) {
        json j;
        j["name"]           = l.name;
        j["position"]       = vec3_to_json(l.position);
        j["direction"]      = vec3_to_json(l.direction);
        j["color"]          = vec3_to_json(l.color);
        j["intensity"]      = l.intensity;
        j["range"]          = l.range;
        j["innerConeAngle"] = l.innerConeAngle;
        j["outerConeAngle"] = l.outerConeAngle;
        j["castsShadow"]    = l.castsShadow;
        arr.push_back(j);
    }
    return arr;
}

static void deserializeSpotLights(std::vector<SpotLight>& lights, const json& arr) {
    lights.clear();
    for (const auto& j : arr) {
        SpotLight l{};
        if (j.contains("name"))           l.name           = j["name"].get<std::string>();
        if (j.contains("position"))       l.position       = json_to_vec3(j["position"]);
        if (j.contains("direction"))      l.direction      = json_to_vec3(j["direction"]);
        if (j.contains("color"))          l.color          = json_to_vec3(j["color"]);
        if (j.contains("intensity"))      l.intensity      = j["intensity"].get<float>();
        if (j.contains("range"))          l.range          = j["range"].get<float>();
        if (j.contains("innerConeAngle")) l.innerConeAngle = j["innerConeAngle"].get<float>();
        if (j.contains("outerConeAngle")) l.outerConeAngle = j["outerConeAngle"].get<float>();
        if (j.contains("castsShadow"))    l.castsShadow    = j["castsShadow"].get<bool>();
        lights.push_back(l);
    }
}

// ================================================================
// Serialize render settings
// ================================================================

static json serializeRenderSettings(const Yalaz::Renderer::RenderSettings& rs) {
    json j;
    // SSAO
    j["ssaoEnabled"]    = rs.ssaoEnabled;
    j["ssaoSamples"]    = rs.ssaoSamples;
    j["ssaoRadius"]     = rs.ssaoRadius;
    j["ssaoIntensity"]  = rs.ssaoIntensity;
    j["ssaoBias"]       = rs.ssaoBias;
    j["ssaoBlurPasses"] = rs.ssaoBlurPasses;
    // Bloom
    j["bloomEnabled"]   = rs.bloomEnabled;
    j["bloomThreshold"] = rs.bloomThreshold;
    j["bloomIntensity"] = rs.bloomIntensity;
    j["bloomMipLevels"] = rs.bloomMipLevels;
    j["bloomRadius"]    = rs.bloomRadius;
    // Tone mapping
    j["tonemappingEnabled"] = rs.tonemappingEnabled;
    j["tonemapOperator"]    = rs.tonemapOperator;
    j["exposure"]           = rs.exposure;
    j["gamma"]              = rs.gamma;
    // Color grading
    j["contrast"]    = rs.contrast;
    j["saturation"]  = rs.saturation;
    j["temperature"] = rs.temperature;
    j["tint"]        = rs.tint;
    // SSR
    j["ssrEnabled"]            = rs.ssrEnabled;
    j["ssrMaxSteps"]           = rs.ssrMaxSteps;
    j["ssrMaxDistance"]        = rs.ssrMaxDistance;
    j["ssrThickness"]          = rs.ssrThickness;
    j["ssrRoughnessThreshold"] = rs.ssrRoughnessThreshold;
    // Shadows
    j["pcssEnabled"]              = rs.pcssEnabled;
    j["pcssBlockerSamples"]       = rs.pcssBlockerSamples;
    j["pcssPCFSamples"]           = rs.pcssPCFSamples;
    j["pcssLightSize"]            = rs.pcssLightSize;
    j["pcssMinPenumbra"]          = rs.pcssMinPenumbra;
    j["contactShadowsEnabled"]    = rs.contactShadowsEnabled;
    j["contactShadowSteps"]       = rs.contactShadowSteps;
    j["contactShadowLength"]      = rs.contactShadowLength;
    j["contactShadowFadeStart"]   = rs.contactShadowFadeStart;
    // Spot lights
    j["spotLightsEnabled"]        = rs.spotLightsEnabled;
    j["maxSpotLights"]            = rs.maxSpotLights;
    j["spotLightShadowsEnabled"]  = rs.spotLightShadowsEnabled;
    return j;
}

static void deserializeRenderSettings(Yalaz::Renderer::RenderSettings& rs, const json& j) {
    // SSAO
    if (j.contains("ssaoEnabled"))    rs.ssaoEnabled    = j["ssaoEnabled"].get<bool>();
    if (j.contains("ssaoSamples"))    rs.ssaoSamples    = j["ssaoSamples"].get<int>();
    if (j.contains("ssaoRadius"))     rs.ssaoRadius     = j["ssaoRadius"].get<float>();
    if (j.contains("ssaoIntensity"))  rs.ssaoIntensity  = j["ssaoIntensity"].get<float>();
    if (j.contains("ssaoBias"))       rs.ssaoBias       = j["ssaoBias"].get<float>();
    if (j.contains("ssaoBlurPasses")) rs.ssaoBlurPasses = j["ssaoBlurPasses"].get<int>();
    // Bloom
    if (j.contains("bloomEnabled"))   rs.bloomEnabled   = j["bloomEnabled"].get<bool>();
    if (j.contains("bloomThreshold")) rs.bloomThreshold = j["bloomThreshold"].get<float>();
    if (j.contains("bloomIntensity")) rs.bloomIntensity = j["bloomIntensity"].get<float>();
    if (j.contains("bloomMipLevels")) rs.bloomMipLevels = j["bloomMipLevels"].get<int>();
    if (j.contains("bloomRadius"))    rs.bloomRadius    = j["bloomRadius"].get<float>();
    // Tone mapping
    if (j.contains("tonemappingEnabled")) rs.tonemappingEnabled = j["tonemappingEnabled"].get<bool>();
    if (j.contains("tonemapOperator"))    rs.tonemapOperator    = j["tonemapOperator"].get<int>();
    if (j.contains("exposure"))           rs.exposure           = j["exposure"].get<float>();
    if (j.contains("gamma"))              rs.gamma              = j["gamma"].get<float>();
    // Color grading
    if (j.contains("contrast"))    rs.contrast    = j["contrast"].get<float>();
    if (j.contains("saturation"))  rs.saturation  = j["saturation"].get<float>();
    if (j.contains("temperature")) rs.temperature = j["temperature"].get<float>();
    if (j.contains("tint"))        rs.tint        = j["tint"].get<float>();
    // SSR
    if (j.contains("ssrEnabled"))            rs.ssrEnabled            = j["ssrEnabled"].get<bool>();
    if (j.contains("ssrMaxSteps"))           rs.ssrMaxSteps           = j["ssrMaxSteps"].get<int>();
    if (j.contains("ssrMaxDistance"))        rs.ssrMaxDistance        = j["ssrMaxDistance"].get<float>();
    if (j.contains("ssrThickness"))          rs.ssrThickness          = j["ssrThickness"].get<float>();
    if (j.contains("ssrRoughnessThreshold")) rs.ssrRoughnessThreshold = j["ssrRoughnessThreshold"].get<float>();
    // Shadows
    if (j.contains("pcssEnabled"))            rs.pcssEnabled            = j["pcssEnabled"].get<bool>();
    if (j.contains("pcssBlockerSamples"))     rs.pcssBlockerSamples     = j["pcssBlockerSamples"].get<int>();
    if (j.contains("pcssPCFSamples"))         rs.pcssPCFSamples         = j["pcssPCFSamples"].get<int>();
    if (j.contains("pcssLightSize"))          rs.pcssLightSize          = j["pcssLightSize"].get<float>();
    if (j.contains("pcssMinPenumbra"))        rs.pcssMinPenumbra        = j["pcssMinPenumbra"].get<float>();
    if (j.contains("contactShadowsEnabled"))  rs.contactShadowsEnabled  = j["contactShadowsEnabled"].get<bool>();
    if (j.contains("contactShadowSteps"))     rs.contactShadowSteps     = j["contactShadowSteps"].get<int>();
    if (j.contains("contactShadowLength"))    rs.contactShadowLength    = j["contactShadowLength"].get<float>();
    if (j.contains("contactShadowFadeStart")) rs.contactShadowFadeStart = j["contactShadowFadeStart"].get<float>();
    // Spot lights
    if (j.contains("spotLightsEnabled"))       rs.spotLightsEnabled       = j["spotLightsEnabled"].get<bool>();
    if (j.contains("maxSpotLights"))           rs.maxSpotLights           = j["maxSpotLights"].get<int>();
    if (j.contains("spotLightShadowsEnabled")) rs.spotLightShadowsEnabled = j["spotLightShadowsEnabled"].get<bool>();
}

// ================================================================
// Serialize grid settings
// ================================================================

static json serializeGrid(const GridSettings& g) {
    json j;
    j["baseGridSize"]        = g.baseGridSize;
    j["majorGridMultiplier"] = g.majorGridMultiplier;
    j["lineWidth"]           = g.lineWidth;
    j["fadeDistance"]         = g.fadeDistance;
    j["gridOpacity"]         = g.gridOpacity;
    j["dynamicLOD"]          = g.dynamicLOD;
    j["lodBias"]             = g.lodBias;
    j["showAxisColors"]      = g.showAxisColors;
    j["axisLineWidth"]       = g.axisLineWidth;
    j["xAxisColor"]          = vec3_to_json(g.xAxisColor);
    j["zAxisColor"]          = vec3_to_json(g.zAxisColor);
    j["originColor"]         = vec3_to_json(g.originColor);
    j["minorLineColor"]      = vec3_to_json(g.minorLineColor);
    j["majorLineColor"]      = vec3_to_json(g.majorLineColor);
    j["infiniteGrid"]        = g.infiniteGrid;
    j["fadeFromCamera"]      = g.fadeFromCamera;
    j["showSubdivisions"]    = g.showSubdivisions;
    j["antiAliasing"]        = g.antiAliasing;
    j["gridHeight"]          = g.gridHeight;
    j["minFadeAlpha"]        = g.minFadeAlpha;
    j["useChunkedGrid"]      = g.useChunkedGrid;
    j["chunkSize"]           = g.chunkSize;
    j["chunkRenderDistance"]  = g.chunkRenderDistance;
    j["currentPreset"]       = g.currentPreset;
    return j;
}

static void deserializeGrid(GridSettings& g, const json& j) {
    if (j.contains("baseGridSize"))        g.baseGridSize        = j["baseGridSize"].get<float>();
    if (j.contains("majorGridMultiplier")) g.majorGridMultiplier = j["majorGridMultiplier"].get<float>();
    if (j.contains("lineWidth"))           g.lineWidth           = j["lineWidth"].get<float>();
    if (j.contains("fadeDistance"))         g.fadeDistance         = j["fadeDistance"].get<float>();
    if (j.contains("gridOpacity"))         g.gridOpacity         = j["gridOpacity"].get<float>();
    if (j.contains("dynamicLOD"))          g.dynamicLOD          = j["dynamicLOD"].get<bool>();
    if (j.contains("lodBias"))             g.lodBias             = j["lodBias"].get<float>();
    if (j.contains("showAxisColors"))      g.showAxisColors      = j["showAxisColors"].get<bool>();
    if (j.contains("axisLineWidth"))       g.axisLineWidth       = j["axisLineWidth"].get<float>();
    if (j.contains("xAxisColor"))          g.xAxisColor          = json_to_vec3(j["xAxisColor"]);
    if (j.contains("zAxisColor"))          g.zAxisColor          = json_to_vec3(j["zAxisColor"]);
    if (j.contains("originColor"))         g.originColor         = json_to_vec3(j["originColor"]);
    if (j.contains("minorLineColor"))      g.minorLineColor      = json_to_vec3(j["minorLineColor"]);
    if (j.contains("majorLineColor"))      g.majorLineColor      = json_to_vec3(j["majorLineColor"]);
    if (j.contains("infiniteGrid"))        g.infiniteGrid        = j["infiniteGrid"].get<bool>();
    if (j.contains("fadeFromCamera"))      g.fadeFromCamera       = j["fadeFromCamera"].get<bool>();
    if (j.contains("showSubdivisions"))    g.showSubdivisions    = j["showSubdivisions"].get<bool>();
    if (j.contains("antiAliasing"))        g.antiAliasing        = j["antiAliasing"].get<bool>();
    if (j.contains("gridHeight"))          g.gridHeight          = j["gridHeight"].get<float>();
    if (j.contains("minFadeAlpha"))        g.minFadeAlpha        = j["minFadeAlpha"].get<float>();
    if (j.contains("useChunkedGrid"))      g.useChunkedGrid      = j["useChunkedGrid"].get<bool>();
    if (j.contains("chunkSize"))           g.chunkSize           = j["chunkSize"].get<float>();
    if (j.contains("chunkRenderDistance")) g.chunkRenderDistance  = j["chunkRenderDistance"].get<float>();
    if (j.contains("currentPreset"))       g.currentPreset       = j["currentPreset"].get<int>();
}

// ================================================================
// Public API: Save
// ================================================================

void saveEngineState(VulkanEngine& engine, const std::string& filepath) {
    json root;

    root["camera"]         = serializeCamera(engine.mainCamera);
    root["lighting"]       = serializeLighting(engine.sceneData);
    root["pointLights"]    = serializePointLights(engine.scenePointLights);
    root["spotLights"]     = serializeSpotLights(engine.sceneSpotLights);
    root["primitives"]     = serializePrimitives(engine.static_shapes);
    root["grid"]           = serializeGrid(engine._gridSettings);
    root["renderSettings"] = serializeRenderSettings(engine._renderSettings);
    root["viewMode"]       = static_cast<int>(engine._currentViewMode);
    root["showGrid"]       = engine._showGrid;
    root["showOutline"]    = engine._showOutline;

    // Save loaded scene file paths
    json scenes = json::object();
    for (const auto& [name, path] : engine.sceneFilePaths) {
        scenes[name] = path;
    }
    root["scenes"] = scenes;

    std::ofstream file(filepath);
    if (!file.is_open()) {
        fmt::print("[ERROR] Failed to save engine state to: {}\n", filepath);
        return;
    }
    file << root.dump(2);
    file.close();
    fmt::print("[INFO] Engine state saved to: {}\n", filepath);
}

// ================================================================
// Public API: Load
// ================================================================

void loadEngineState(VulkanEngine& engine, const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        fmt::print("[ERROR] Failed to open state file: {}\n", filepath);
        return;
    }

    json root;
    try {
        root = json::parse(file);
    } catch (const json::parse_error& e) {
        fmt::print("[ERROR] JSON parse error: {}\n", e.what());
        return;
    }
    file.close();

    // Clear current state
    Yalaz::UI::EditorSelection::Get().ClearSelection();
    engine.selectedNode = nullptr;
    engine.selectedObjectName.clear();

    // Load camera
    if (root.contains("camera")) {
        deserializeCamera(engine.mainCamera, root["camera"]);
    }

    // Load lighting
    if (root.contains("lighting")) {
        deserializeLighting(engine.sceneData, root["lighting"]);
    }

    // Load point lights
    if (root.contains("pointLights")) {
        deserializePointLights(engine.scenePointLights, root["pointLights"]);
    }

    // Load spot lights
    if (root.contains("spotLights")) {
        deserializeSpotLights(engine.sceneSpotLights, root["spotLights"]);
    }

    // Load primitives
    if (root.contains("primitives")) {
        deserializePrimitives(engine, engine.static_shapes, root["primitives"]);

        // Rebuild materials from texture paths
        for (auto& shape : engine.static_shapes) {
            bool hasTextures = !shape.albedoTexturePath.empty() ||
                              !shape.metalRoughTexturePath.empty() ||
                              !shape.emissionTexturePath.empty();
            if (hasTextures) {
                MaterialInstance mat = engine.create_primitive_material(
                    shape.albedoTexturePath, shape.metalRoughTexturePath, shape.emissionTexturePath);
                shape.material = std::make_shared<MaterialInstance>(mat);
            }
        }
    }

    // Load grid
    if (root.contains("grid")) {
        deserializeGrid(engine._gridSettings, root["grid"]);
    }

    // Load render settings
    if (root.contains("renderSettings")) {
        deserializeRenderSettings(engine._renderSettings, root["renderSettings"]);
    }

    // Load view state
    if (root.contains("viewMode")) {
        engine._currentViewMode = static_cast<VulkanEngine::ViewMode>(root["viewMode"].get<int>());
    }
    if (root.contains("showGrid")) {
        engine._showGrid = root["showGrid"].get<bool>();
    }
    if (root.contains("showOutline")) {
        engine._showOutline = root["showOutline"].get<bool>();
    }

    // Load scenes
    if (root.contains("scenes")) {
        // Clear existing loaded scenes (shared_ptr destructor handles GPU cleanup)
        engine.loadedScenes.clear();
        engine.sceneFilePaths.clear();

        for (auto& [name, pathVal] : root["scenes"].items()) {
            std::string path = pathVal.get<std::string>();
            auto gltfScene = loadGltf(&engine, path);
            if (gltfScene.has_value()) {
                engine.loadedScenes[name] = *gltfScene;
                engine.sceneFilePaths[name] = path;
                fmt::print("[INFO] Loaded scene: {} from {}\n", name, path);
            } else {
                auto objScene = loadObj(&engine, path);
                if (objScene.has_value()) {
                    engine.loadedScenes[name] = *objScene;
                    engine.sceneFilePaths[name] = path;
                    fmt::print("[INFO] Loaded OBJ scene: {} from {}\n", name, path);
                } else {
                    fmt::print("[WARN] Failed to load scene: {} from {}\n", name, path);
                }
            }
        }
    }

    fmt::print("[INFO] Engine state loaded from: {}\n", filepath);
}

// ================================================================
// Public API: Reset
// ================================================================

void resetEngineState(VulkanEngine& engine) {
    // Clear selection
    Yalaz::UI::EditorSelection::Get().ClearSelection();
    engine.selectedNode = nullptr;
    engine.selectedObjectName.clear();

    // Reset camera to defaults
    engine.mainCamera.position         = glm::vec3(0.0f, 0.0f, 5.0f);
    engine.mainCamera.pitch            = 0.0f;
    engine.mainCamera.yaw              = 0.0f;
    engine.mainCamera.fov              = 60.0f;
    engine.mainCamera.targetFov        = 60.0f;
    engine.mainCamera.nearPlane        = 0.1f;
    engine.mainCamera.farPlane         = 1000.0f;
    engine.mainCamera.moveSpeed        = 5.0f;
    engine.mainCamera.sprintMultiplier = 3.0f;
    engine.mainCamera.mouseSensitivity = 0.002f;
    engine.mainCamera.panSensitivity   = 0.005f;
    engine.mainCamera.scrollZoomSpeed  = 3.0f;
    engine.mainCamera.smoothing        = 10.0f;
    engine.mainCamera.fovSmoothSpeed   = 8.0f;
    engine.mainCamera.focusActive      = false;
    engine.mainCamera.focusDistance     = 5.0f;
    engine.mainCamera.focusSmoothSpeed = 6.0f;
    engine.mainCamera.updateProjectionMatrix();

    // Reset lighting to init_scene_data defaults
    engine.sceneData.ambientColor = glm::vec4(0.1f, 0.1f, 0.1f, 0.5f);
    glm::vec3 sunDir = glm::normalize(glm::vec3(-1.0f, -3.0f, -1.0f));
    engine.sceneData.sunlightDirection = glm::vec4(sunDir, 3.0f);
    engine.sceneData.sunlightColor = glm::vec4(1.0f, 0.95f, 0.8f, 1.0f);
    engine.sceneData.cameraPosition = glm::vec4(0.0f, 0.0f, 5.0f, 1.0f);
    engine.sceneData.pointLightCount = 0;

    // Clear point lights
    engine.scenePointLights.clear();

    // Clear primitives
    engine.static_shapes.clear();

    // Reset grid to defaults
    engine._gridSettings = GridSettings{};

    // Reset view state
    engine._currentViewMode = VulkanEngine::ViewMode::Shaded;
    engine._showGrid = true;
    engine._showOutline = true;

    // Reload default scene
    engine.loadedScenes.clear();
    engine.sceneFilePaths.clear();

    auto gltfScene = loadGltf(&engine, "../../assets/monkeyHD.glb");
    if (gltfScene.has_value()) {
        engine.loadedScenes["monkey"] = *gltfScene;
        engine.sceneFilePaths["monkey"] = "../../assets/monkeyHD.glb";
        fmt::print("[INFO] Default scene reloaded\n");
    }

    fmt::print("[INFO] Engine state reset to defaults\n");
}
