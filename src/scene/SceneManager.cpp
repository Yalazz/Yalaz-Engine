#include "SceneManager.h"
#include "vk_engine.h"  // For MeshNode, LoadedGLTF
#include "vk_loader.h"  // For loadGltf function
#include "ui/views/ConsoleView.h"
#include "ui/views/ViewManager.h"
#include "ui/EditorUI.h"
#include <fmt/core.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <array>
#include <cctype>

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace Yalaz::Scene {

namespace {
std::string toLowerCopy(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return s;
}

bool isImageFile(const fs::path& p) {
    const std::string ext = toLowerCopy(p.extension().string());
    return ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga" ||
           ext == ".bmp" || ext == ".hdr";
}

bool faceNameMatches(const std::string& n, int faceIdx) {
    static const std::array<std::vector<std::string>, 6> tokens = {{
        {"posx", "xpos", "right", "_px", ".px", "+x"},
        {"negx", "xneg", "left", "_nx", ".nx", "-x"},
        {"posy", "ypos", "up", "top", "_py", ".py", "+y"},
        {"negy", "yneg", "down", "bottom", "_ny", ".ny", "-y"},
        {"posz", "zpos", "front", "_pz", ".pz", "+z"},
        {"negz", "zneg", "back", "_nz", ".nz", "-z"},
    }};
    for (const auto& t : tokens[faceIdx]) {
        if (n.find(t) != std::string::npos) return true;
    }
    return false;
}

bool collectCubemapFaces(const fs::path& dir, std::array<std::string, 6>& outFaces) {
    if (!fs::exists(dir) || !fs::is_directory(dir)) return false;

    std::array<std::string, 6> found{};
    std::array<bool, 6> ok = { false, false, false, false, false, false };

    for (const auto& entry : fs::directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        const fs::path p = entry.path();
        if (!isImageFile(p)) continue;
        const std::string lowerName = toLowerCopy(p.filename().string());

        for (int i = 0; i < 6; ++i) {
            if (!ok[i] && faceNameMatches(lowerName, i)) {
                found[i] = p.string();
                ok[i] = true;
                break;
            }
        }
    }

    for (bool v : ok) {
        if (!v) return false;
    }
    outFaces = found;
    return true;
}

bool tryLoadCubemapFromAssetsFolder(Yalaz::Renderer::EnvironmentMap* envMap) {
    if (!envMap) return false;

    const std::array<std::string, 4> roots = {
        "../../assets/cubemaps", "../assets/cubemaps", "assets/cubemaps", "./assets/cubemaps"
    };

    for (const auto& root : roots) {
        fs::path base(root);
        if (!fs::exists(base) || !fs::is_directory(base)) continue;

        std::array<std::string, 6> faces{};
        if (collectCubemapFaces(base, faces)) {
            return envMap->loadCubemapFaces(faces.data());
        }

        for (const auto& child : fs::directory_iterator(base)) {
            if (!child.is_directory()) continue;
            if (collectCubemapFaces(child.path(), faces)) {
                return envMap->loadCubemapFaces(faces.data());
            }
        }
    }

    return false;
}
} // namespace

void SceneManager::OnInit() {
    fmt::print("[SceneManager] Initialized\n");
}

void SceneManager::OnShutdown() {
    fmt::print("[SceneManager] Shutdown\n");
    ClearAllScenes();
}

void SceneManager::OnUpdate(float deltaTime) {
    (void)deltaTime;
    // Scene update logic - transforms, animations, etc.
    // Will be populated when migrated from VulkanEngine::update_scene()
}

bool SceneManager::LoadGLTF(const std::string& filePath, const std::string& sceneName) {
    // During migration, VulkanEngine handles loading
    // This will be implemented when loadGltf is migrated
    fmt::print("[SceneManager] LoadGLTF: {} as '{}'\n", filePath, sceneName);
    m_SceneFilePaths[sceneName] = filePath;
    return true;
}

void SceneManager::UnloadScene(const std::string& sceneName) {
    auto it = m_LoadedScenes.find(sceneName);
    if (it != m_LoadedScenes.end()) {
        m_LoadedScenes.erase(it);
    }

    auto pathIt = m_SceneFilePaths.find(sceneName);
    if (pathIt != m_SceneFilePaths.end()) {
        m_SceneFilePaths.erase(pathIt);
    }

    fmt::print("[SceneManager] Unloaded scene: {}\n", sceneName);
}

void SceneManager::ClearAllScenes() {
    m_LoadedScenes.clear();
    m_SceneFilePaths.clear();
    m_LoadedNodes.clear();

    if (m_DrawContext) {
        m_DrawContext->OpaqueSurfaces.clear();
        m_DrawContext->TransparentSurfaces.clear();
    }
    if (m_DrawCommands) {
        m_DrawCommands->OpaqueSurfaces.clear();
        m_DrawCommands->TransparentSurfaces.clear();
    }
}

std::shared_ptr<LoadedGLTF> SceneManager::GetScene(const std::string& name) {
    auto it = m_LoadedScenes.find(name);
    if (it != m_LoadedScenes.end()) {
        return it->second;
    }
    return nullptr;
}

MeshNode* SceneManager::FindNodeByName(const std::string& name) {
    for (auto& [sceneName, scene] : m_LoadedScenes) {
        // Search through scene nodes
        // Implementation depends on LoadedGLTF structure
    }

    for (auto& [nodeName, node] : m_LoadedNodes) {
        auto* result = FindNodeRecursive(node, name);
        if (result) return result;
    }

    return nullptr;
}

MeshNode* SceneManager::FindNodeRecursive(std::shared_ptr<Node> node, const std::string& name) {
    if (!node) return nullptr;

    // Check if this node matches
    // Implementation depends on Node structure

    // Recursively search children
    for (auto& child : node->children) {
        auto* result = FindNodeRecursive(child, name);
        if (result) return result;
    }

    return nullptr;
}

// =============================================================================
// Scene Serialization
// =============================================================================

bool SceneManager::SaveScene(const std::string& filePath) {
    try {
        json sceneData;

        // Header
        sceneData["version"] = "1.3";
        sceneData["engine"] = "Yalaz Engine";

        // Save loaded model scenes (authoritative source is engine maps if available)
        const auto& scenePaths = (m_Engine ? m_Engine->sceneFilePaths : m_SceneFilePaths);
        const auto& loadedScenes = (m_Engine ? m_Engine->loadedScenes : m_LoadedScenes);

        json scenesArray = json::array();
        for (const auto& [name, path] : scenePaths) {
            json sceneEntry;
            sceneEntry["name"] = name;
            sceneEntry["file"] = path;

            // Save transforms for nodes in this scene
            auto sceneIt = loadedScenes.find(name);
            if (sceneIt != loadedScenes.end() && sceneIt->second) {
                json nodesArray = json::array();
                for (const auto& [nodeName, node] : sceneIt->second->nodes) {
                    if (!node) continue;

                    json nodeData;
                    nodeData["name"] = nodeName;

                    // Save local transform matrix (4x4)
                    json transform = json::array();
                    const float* m = &node->localTransform[0][0];
                    for (int i = 0; i < 16; i++) {
                        transform.push_back(m[i]);
                    }
                    nodeData["localTransform"] = transform;

                    nodesArray.push_back(nodeData);
                }
                sceneEntry["nodes"] = nodesArray;

                // Save material runtime overrides for loaded scene assets (GLTF/GLB/FBX/DAE via converted GLTF)
                auto& loadedScene = sceneIt->second;
                if (m_Engine &&
                    loadedScene->materialDataBuffer.buffer != VK_NULL_HANDLE &&
                    loadedScene->materialDataBuffer.allocation != VK_NULL_HANDLE &&
                    loadedScene->materialDataBuffer.size >= sizeof(GLTFMetallic_Roughness::MaterialConstants) &&
                    !loadedScene->materials.empty()) {

                    void* mappedData = nullptr;
                    VkResult mapResult = vmaMapMemory(m_Engine->_allocator, loadedScene->materialDataBuffer.allocation, &mappedData);
                    if (mapResult == VK_SUCCESS && mappedData) {
                        json materialsArray = json::array();
                        const auto* bytes = static_cast<const uint8_t*>(mappedData);

                        for (const auto& [matName, mat] : loadedScene->materials) {
                            if (!mat) continue;

                            const size_t offset = static_cast<size_t>(mat->bufferOffset);
                            if (offset + sizeof(GLTFMetallic_Roughness::MaterialConstants) > loadedScene->materialDataBuffer.size) {
                                continue;
                            }

                            const auto* c = reinterpret_cast<const GLTFMetallic_Roughness::MaterialConstants*>(bytes + offset);
                            json m;
                            m["name"] = matName;
                            m["bufferOffset"] = mat->bufferOffset;
                            m["colorFactors"] = {c->colorFactors.x, c->colorFactors.y, c->colorFactors.z, c->colorFactors.w};
                            m["metalRoughFactors"] = {c->metal_rough_factors.x, c->metal_rough_factors.y, c->metal_rough_factors.z, c->metal_rough_factors.w};
                            m["colorTexID"] = c->colorTexID;
                            m["metalRoughTexID"] = c->metalRoughTexID;
                            m["normalTexID"] = c->normalTexID;
                            m["emissiveTexID"] = c->emissiveTexID;

                            json extra = json::array();
                            for (int i = 0; i < 13; ++i) {
                                extra.push_back({c->extra[i].x, c->extra[i].y, c->extra[i].z, c->extra[i].w});
                            }
                            m["extra"] = extra;
                            materialsArray.push_back(m);
                        }

                        vmaUnmapMemory(m_Engine->_allocator, loadedScene->materialDataBuffer.allocation);

                        if (!materialsArray.empty()) {
                            sceneEntry["materials"] = materialsArray;
                        }
                    } else {
                        UI::Console::Warn("Failed to map material buffer for scene save: " + name);
                    }
                }
            }

            scenesArray.push_back(sceneEntry);
        }
        sceneData["scenes"] = scenesArray;

        // Save primitives
        if (m_Engine) {
            json primitivesArray = json::array();
            for (const auto& prim : m_Engine->static_shapes) {
                // Skip light visualization spheres
                if (prim.materialType == ShaderOnlyMaterial::POINTLIGHT_VIS ||
                    prim.materialType == ShaderOnlyMaterial::EMISSIVE) {
                    continue;
                }

                json primData;
                primData["name"] = prim.name;
                primData["type"] = static_cast<int>(prim.type);
                primData["position"] = {prim.position.x, prim.position.y, prim.position.z};
                primData["rotation"] = {prim.rotation.x, prim.rotation.y, prim.rotation.z};
                primData["scale"] = {prim.scale.x, prim.scale.y, prim.scale.z};
                primData["mainColor"] = {prim.mainColor.r, prim.mainColor.g, prim.mainColor.b, prim.mainColor.a};
                primData["metallic"] = prim.metallic;
                primData["roughness"] = prim.roughness;
                primData["reflectionIntensity"] = prim.reflectionIntensity;
                primData["emission"] = {prim.emission.r, prim.emission.g, prim.emission.b};
                primData["useFaceColors"] = prim.useFaceColors;
                primData["visible"] = prim.visible;
                primData["materialType"] = static_cast<int>(prim.materialType);
                primData["passType"] = static_cast<int>(prim.passType);
                primData["albedoTexturePath"] = prim.albedoTexturePath;
                primData["metalRoughTexturePath"] = prim.metalRoughTexturePath;
                primData["emissionTexturePath"] = prim.emissionTexturePath;

                // Save face colors
                json faceColorsArray = json::array();
                for (int i = 0; i < 6; i++) {
                    faceColorsArray.push_back({prim.faceColors[i].r, prim.faceColors[i].g,
                                               prim.faceColors[i].b, prim.faceColors[i].a});
                }
                primData["faceColors"] = faceColorsArray;

                primitivesArray.push_back(primData);
            }
            sceneData["primitives"] = primitivesArray;

            // Save point lights
            json lightsArray = json::array();
            for (const auto& light : m_Engine->scenePointLights) {
                json lightData;
                lightData["position"] = {light.position.x, light.position.y, light.position.z};
                lightData["color"] = {light.color.r, light.color.g, light.color.b};
                lightData["intensity"] = light.intensity;
                lightData["radius"] = light.radius;
                lightData["castsShadow"] = light.castsShadow;
                lightsArray.push_back(lightData);
            }
            sceneData["pointLights"] = lightsArray;

            // Save spot lights
            json spotLightsArray = json::array();
            for (const auto& light : m_Engine->sceneSpotLights) {
                json lightData;
                lightData["name"] = light.name;
                lightData["position"] = {light.position.x, light.position.y, light.position.z};
                lightData["direction"] = {light.direction.x, light.direction.y, light.direction.z};
                lightData["color"] = {light.color.r, light.color.g, light.color.b};
                lightData["intensity"] = light.intensity;
                lightData["range"] = light.range;
                lightData["innerConeAngle"] = light.innerConeAngle;
                lightData["outerConeAngle"] = light.outerConeAngle;
                lightData["castsShadow"] = light.castsShadow;
                spotLightsArray.push_back(lightData);
            }
            sceneData["spotLights"] = spotLightsArray;

            // Save camera
            json cameraData;
            cameraData["position"] = {m_Engine->mainCamera.position.x,
                                      m_Engine->mainCamera.position.y,
                                      m_Engine->mainCamera.position.z};
            cameraData["pitch"] = m_Engine->mainCamera.pitch;
            cameraData["yaw"] = m_Engine->mainCamera.yaw;
            sceneData["camera"] = cameraData;

            // Save animation runtime + graph state
            json animationData;
            animationData["activeAnimationIndex"] = m_Engine->activeAnimationIndex;
            animationData["activeSkeletonIndex"] = m_Engine->activeSkeletonIndex;

            json clipsArray = json::array();
            for (const auto& clip : m_Engine->animationClips) {
                json clipData;
                clipData["name"] = clip.name;
                clipData["sourceScene"] = clip.sourceScene;
                clipData["skeletonIndex"] = clip.skeletonIndex;
                clipData["duration"] = clip.duration;
                clipData["currentTime"] = clip.currentTime;
                clipData["isPlaying"] = clip.isPlaying;
                clipData["loop"] = clip.loop;
                clipData["pingPong"] = clip.pingPong;
                clipData["reverse"] = clip.reverse;
                clipData["speed"] = clip.speed;
                clipsArray.push_back(clipData);
            }
            animationData["clips"] = clipsArray;

            const auto& graph = m_Engine->animationGraph;
            json graphData;
            graphData["enabled"] = graph.enabled;
            graphData["activeState"] = graph.activeState;
            graphData["nextState"] = graph.nextState;
            graphData["blending"] = graph.blending;
            graphData["blendDuration"] = graph.blendDuration;
            graphData["blendElapsed"] = graph.blendElapsed;

            json graphStates = json::array();
            for (const auto& s : graph.states) {
                json stateData;
                stateData["name"] = s.name;
                stateData["clipIndex"] = s.clipIndex;
                stateData["isDefault"] = s.isDefault;
                stateData["positionX"] = s.positionX;
                stateData["positionY"] = s.positionY;
                if (s.clipIndex >= 0 && s.clipIndex < static_cast<int>(m_Engine->animationClips.size())) {
                    const auto& clip = m_Engine->animationClips[s.clipIndex];
                    stateData["clipName"] = clip.name;
                    stateData["clipSourceScene"] = clip.sourceScene;
                }
                graphStates.push_back(stateData);
            }
            graphData["states"] = graphStates;

            json graphTransitions = json::array();
            for (const auto& t : graph.transitions) {
                json trData;
                trData["fromState"] = t.fromState;
                trData["toState"] = t.toState;
                trData["parameter"] = t.parameter;
                trData["comparison"] = t.comparison;
                trData["threshold"] = t.threshold;
                trData["hasExitTime"] = t.hasExitTime;
                trData["exitTime"] = t.exitTime;
                trData["blendTime"] = t.blendTime;
                trData["enabled"] = t.enabled;
                graphTransitions.push_back(trData);
            }
            graphData["transitions"] = graphTransitions;

            json graphParameters = json::array();
            for (const auto& p : graph.parameters) {
                json paramData;
                paramData["name"] = p.name;
                paramData["value"] = p.value;
                paramData["isBool"] = p.isBool;
                graphParameters.push_back(paramData);
            }
            graphData["parameters"] = graphParameters;

            animationData["graph"] = graphData;
            sceneData["animation"] = animationData;

            // Save global editor/render settings (session persistence)
            json settings;
            settings["viewMode"] = static_cast<int>(m_Engine->_currentViewMode);
            settings["showGrid"] = m_Engine->_showGrid;
            settings["showOutline"] = m_Engine->_showOutline;
            settings["enableBackfaceCulling"] = m_Engine->enableBackfaceCulling;
            settings["renderScale"] = m_Engine->renderScale;
            settings["shadowsEnabled"] = m_Engine->shadowsEnabled;
            settings["pointLightShadowsEnabled"] = m_Engine->pointLightShadowsEnabled;
            settings["sunEnabled"] = m_Engine->sunEnabled;
            settings["savedSunIntensity"] = m_Engine->savedSunIntensity;
            settings["snapEnabled"] = m_Engine->snapEnabled;
            settings["snapPositionValue"] = m_Engine->snapPositionValue;
            settings["snapRotationEnabled"] = m_Engine->snapRotationEnabled;
            settings["snapRotationAngle"] = m_Engine->snapRotationAngle;
            settings["snapScaleEnabled"] = m_Engine->snapScaleEnabled;
            settings["snapScaleValue"] = m_Engine->snapScaleValue;

            json renderSettings;
            const auto& rs = m_Engine->_renderSettings;
            renderSettings["ssaoEnabled"] = rs.ssaoEnabled;
            renderSettings["ssaoSamples"] = rs.ssaoSamples;
            renderSettings["ssaoRadius"] = rs.ssaoRadius;
            renderSettings["ssaoIntensity"] = rs.ssaoIntensity;
            renderSettings["ssaoBias"] = rs.ssaoBias;
            renderSettings["ssaoBlurPasses"] = rs.ssaoBlurPasses;
            renderSettings["bloomEnabled"] = rs.bloomEnabled;
            renderSettings["bloomThreshold"] = rs.bloomThreshold;
            renderSettings["bloomIntensity"] = rs.bloomIntensity;
            renderSettings["bloomMipLevels"] = rs.bloomMipLevels;
            renderSettings["bloomRadius"] = rs.bloomRadius;
            renderSettings["tonemappingEnabled"] = rs.tonemappingEnabled;
            renderSettings["tonemapOperator"] = rs.tonemapOperator;
            renderSettings["exposure"] = rs.exposure;
            renderSettings["gamma"] = rs.gamma;
            renderSettings["contrast"] = rs.contrast;
            renderSettings["saturation"] = rs.saturation;
            renderSettings["sharpness"] = rs.sharpness;
            renderSettings["temperature"] = rs.temperature;
            renderSettings["tint"] = rs.tint;
            renderSettings["ssrEnabled"] = rs.ssrEnabled;
            renderSettings["ssrMaxSteps"] = rs.ssrMaxSteps;
            renderSettings["ssrMaxDistance"] = rs.ssrMaxDistance;
            renderSettings["ssrThickness"] = rs.ssrThickness;
            renderSettings["ssrRoughnessThreshold"] = rs.ssrRoughnessThreshold;
            renderSettings["pcssEnabled"] = rs.pcssEnabled;
            renderSettings["pcssBlockerSamples"] = rs.pcssBlockerSamples;
            renderSettings["pcssPCFSamples"] = rs.pcssPCFSamples;
            renderSettings["pcssLightSize"] = rs.pcssLightSize;
            renderSettings["pcssMinPenumbra"] = rs.pcssMinPenumbra;
            renderSettings["contactShadowsEnabled"] = rs.contactShadowsEnabled;
            renderSettings["contactShadowSteps"] = rs.contactShadowSteps;
            renderSettings["contactShadowLength"] = rs.contactShadowLength;
            renderSettings["contactShadowFadeStart"] = rs.contactShadowFadeStart;
            renderSettings["spotLightsEnabled"] = rs.spotLightsEnabled;
            renderSettings["maxSpotLights"] = rs.maxSpotLights;
            renderSettings["spotLightShadowsEnabled"] = rs.spotLightShadowsEnabled;
            renderSettings["reflectionProbesEnabled"] = rs.reflectionProbesEnabled;
            renderSettings["globalSkyBlend"] = rs.globalSkyBlend;
            settings["renderSettings"] = renderSettings;

            if (m_Engine->_environmentMap) {
                json env;
                env["loadedPath"] = m_Engine->_environmentMap->stats.loadedPath;
                env["skyColorTop"] = {m_Engine->_environmentMap->settings.skyColorTop.x, m_Engine->_environmentMap->settings.skyColorTop.y, m_Engine->_environmentMap->settings.skyColorTop.z};
                env["skyColorHorizon"] = {m_Engine->_environmentMap->settings.skyColorHorizon.x, m_Engine->_environmentMap->settings.skyColorHorizon.y, m_Engine->_environmentMap->settings.skyColorHorizon.z};
                env["groundColor"] = {m_Engine->_environmentMap->settings.groundColor.x, m_Engine->_environmentMap->settings.groundColor.y, m_Engine->_environmentMap->settings.groundColor.z};
                env["skyIntensity"] = m_Engine->_environmentMap->settings.skyIntensity;
                env["exposure"] = m_Engine->_environmentMap->settings.exposure;
                env["rotation"] = m_Engine->_environmentMap->settings.rotation;
                env["useIBL"] = m_Engine->_environmentMap->settings.useIBL;
                env["iblIntensity"] = m_Engine->_environmentMap->settings.iblIntensity;
                settings["environment"] = env;
            }

            sceneData["settings"] = settings;

            // Save physics world + runtime bodies/constraints
            json physics;
            physics["enabled"] = m_Engine->physicsEnabled;
            physics["gravity"] = {m_Engine->physicsSettings.gravity.x, m_Engine->physicsSettings.gravity.y, m_Engine->physicsSettings.gravity.z};
            physics["timeStep"] = m_Engine->physicsSettings.timeStep;
            physics["maxSubSteps"] = m_Engine->physicsSettings.maxSubSteps;
            physics["debugDraw"] = m_Engine->physicsSettings.debugDraw;
            physics["paused"] = m_Engine->physicsSettings.paused;

            json bodies = json::array();
            for (const auto& body : m_Engine->physicsBodies) {
                json b;
                b["name"] = body.name;
                b["type"] = body.type;
                b["position"] = {body.position.x, body.position.y, body.position.z};
                b["rotation"] = {body.rotation.x, body.rotation.y, body.rotation.z, body.rotation.w};
                b["velocity"] = {body.velocity.x, body.velocity.y, body.velocity.z};
                b["angularVelocity"] = {body.angularVelocity.x, body.angularVelocity.y, body.angularVelocity.z};
                b["mass"] = body.mass;
                b["friction"] = body.friction;
                b["restitution"] = body.restitution;
                b["isAwake"] = body.isAwake;
                b["colliderType"] = body.colliderType;
                b["colliderSize"] = {body.colliderSize.x, body.colliderSize.y, body.colliderSize.z};
                bodies.push_back(b);
            }
            physics["bodies"] = bodies;

            json constraints = json::array();
            for (const auto& c : m_Engine->physicsConstraints) {
                json jc;
                jc["name"] = c.name;
                jc["bodyA"] = c.bodyA;
                jc["bodyB"] = c.bodyB;
                jc["type"] = c.type;
                jc["pivotA"] = {c.pivotA.x, c.pivotA.y, c.pivotA.z};
                jc["pivotB"] = {c.pivotB.x, c.pivotB.y, c.pivotB.z};
                constraints.push_back(jc);
            }
            physics["constraints"] = constraints;
            sceneData["physics"] = physics;

            // Save UI layout state (open views + window positions/sizes)
            json uiLayout;
            uiLayout["layoutLocked"] = Yalaz::UI::EditorUI::Get().IsLayoutLocked();
            json viewsArray = json::array();
            for (const auto& viewPtr : Yalaz::UI::ViewManager::Get().GetViews()) {
                if (!viewPtr) continue;
                json v;
                v["name"] = viewPtr->GetName();
                v["isOpen"] = viewPtr->IsOpen();
                ImVec2 p = viewPtr->GetLastPos();
                ImVec2 s = viewPtr->GetLastSize();
                v["position"] = {p.x, p.y};
                v["size"] = {s.x, s.y};
                viewsArray.push_back(v);
            }
            uiLayout["views"] = viewsArray;
            sceneData["uiLayout"] = uiLayout;
        }

        // Create directory if needed
        fs::path savePath(filePath);
        if (savePath.has_parent_path()) {
            fs::create_directories(savePath.parent_path());
        }

        // Write to file
        std::ofstream file(filePath);
        if (!file.is_open()) {
            UI::Console::Error("Failed to open file for saving: " + filePath);
            return false;
        }

        file << sceneData.dump(2);  // Pretty print with 2-space indent
        file.close();

        UI::Console::Log("Scene saved: " + filePath);
        AddRecentScene(filePath);

        return true;

    } catch (const std::exception& e) {
        UI::Console::Error("Failed to save scene: " + std::string(e.what()));
        return false;
    }
}

bool SceneManager::LoadScene(const std::string& filePath) {
    try {
        // Check file exists
        if (!fs::exists(filePath)) {
            UI::Console::Error("Scene file not found: " + filePath);
            return false;
        }

        // Read file
        std::ifstream file(filePath);
        if (!file.is_open()) {
            UI::Console::Error("Failed to open scene file: " + filePath);
            return false;
        }

        json sceneData;
        file >> sceneData;
        file.close();

        // Validate version
        std::string version = sceneData.value("version", "unknown");
        UI::Console::Log("Loading scene (version " + version + "): " + filePath);

        if (!m_Engine) {
            UI::Console::Error("Engine not set - cannot load scene");
            return false;
        }

        // Replace current scene content: unload all currently loaded model scenes first.
        if (!m_Engine->loadedScenes.empty()) {
            for (const auto& [sceneName, _] : m_Engine->loadedScenes) {
                bool alreadyQueued = false;
                for (const auto& queued : m_Engine->_pendingSceneUnloads) {
                    if (queued == sceneName) {
                        alreadyQueued = true;
                        break;
                    }
                }
                if (!alreadyQueued) {
                    m_Engine->_pendingSceneUnloads.push_back(sceneName);
                }
            }
            m_Engine->processPendingSceneUnloads();
        }

        // Keep SceneManager and engine maps in sync
        m_LoadedScenes.clear();
        m_SceneFilePaths.clear();
        m_LoadedNodes.clear();
        m_Engine->loadedScenes.clear();
        m_Engine->sceneFilePaths.clear();

        // Clear existing data
        m_Engine->static_shapes.clear();
        m_Engine->scenePointLights.clear();
        m_Engine->sceneSpotLights.clear();

        // Load primitives
        if (sceneData.contains("primitives")) {
            int loadedCount = 0;
            for (const auto& primData : sceneData["primitives"]) {
                StaticMeshData prim;
                prim.name = primData.value("name", "Unnamed");
                prim.type = static_cast<PrimitiveType>(primData.value("type", 0));

                // Position, rotation, scale
                if (primData.contains("position")) {
                    prim.position = glm::vec3(primData["position"][0], primData["position"][1], primData["position"][2]);
                }
                if (primData.contains("rotation")) {
                    prim.rotation = glm::vec3(primData["rotation"][0], primData["rotation"][1], primData["rotation"][2]);
                }
                if (primData.contains("scale")) {
                    prim.scale = glm::vec3(primData["scale"][0], primData["scale"][1], primData["scale"][2]);
                }

                // Material properties
                if (primData.contains("mainColor")) {
                    prim.mainColor = glm::vec4(primData["mainColor"][0], primData["mainColor"][1],
                                               primData["mainColor"][2], primData["mainColor"][3]);
                }
                prim.metallic = primData.value("metallic", 0.0f);
                prim.roughness = primData.value("roughness", 0.5f);
                prim.reflectionIntensity = primData.value("reflectionIntensity", 0.0f);
                if (primData.contains("emission")) {
                    prim.emission = glm::vec3(primData["emission"][0], primData["emission"][1], primData["emission"][2]);
                }

                prim.useFaceColors = primData.value("useFaceColors", false);
                prim.visible = primData.value("visible", true);
                prim.materialType = static_cast<ShaderOnlyMaterial>(primData.value("materialType", 0));
                prim.passType = static_cast<MaterialPass>(primData.value("passType", 0));
                prim.albedoTexturePath = primData.value("albedoTexturePath", std::string{});
                prim.metalRoughTexturePath = primData.value("metalRoughTexturePath", std::string{});
                prim.emissionTexturePath = primData.value("emissionTexturePath", std::string{});

                // Face colors
                if (primData.contains("faceColors")) {
                    for (int i = 0; i < 6 && i < primData["faceColors"].size(); i++) {
                        prim.faceColors[i] = glm::vec4(primData["faceColors"][i][0], primData["faceColors"][i][1],
                                                       primData["faceColors"][i][2], primData["faceColors"][i][3]);
                    }
                }

                // Get mesh from default meshes
                auto it = m_Engine->defaultMeshes.find(prim.type);
                if (it != m_Engine->defaultMeshes.end()) {
                    prim.mesh = it->second;
                }

                m_Engine->static_shapes.push_back(prim);
                loadedCount++;
            }
            UI::Console::Log(fmt::format("  Loaded {} primitives", loadedCount));
        }

        // Load point lights
        if (sceneData.contains("pointLights")) {
            for (const auto& lightData : sceneData["pointLights"]) {
                PointLight light;
                if (lightData.contains("position")) {
                    light.position = glm::vec3(lightData["position"][0], lightData["position"][1], lightData["position"][2]);
                }
                if (lightData.contains("color")) {
                    light.color = glm::vec3(lightData["color"][0], lightData["color"][1], lightData["color"][2]);
                }
                light.intensity = lightData.value("intensity", 10.0f);
                light.radius = lightData.value("radius", 15.0f);
                light.castsShadow = lightData.value("castsShadow", false);
                m_Engine->scenePointLights.push_back(light);
            }
            UI::Console::Log(fmt::format("  Loaded {} point lights", m_Engine->scenePointLights.size()));

            // Sync light visualizations
            m_Engine->sync_point_light_billboards();
        }

        // Load spot lights
        if (sceneData.contains("spotLights")) {
            m_Engine->sceneSpotLights.clear();
            for (const auto& lightData : sceneData["spotLights"]) {
                SpotLight light;
                light.name = lightData.value("name", std::string{"SpotLight"});
                if (lightData.contains("position")) {
                    light.position = glm::vec3(lightData["position"][0], lightData["position"][1], lightData["position"][2]);
                }
                if (lightData.contains("direction")) {
                    light.direction = glm::vec3(lightData["direction"][0], lightData["direction"][1], lightData["direction"][2]);
                }
                if (lightData.contains("color")) {
                    light.color = glm::vec3(lightData["color"][0], lightData["color"][1], lightData["color"][2]);
                }
                light.intensity = lightData.value("intensity", light.intensity);
                light.range = lightData.value("range", light.range);
                light.innerConeAngle = lightData.value("innerConeAngle", light.innerConeAngle);
                light.outerConeAngle = lightData.value("outerConeAngle", light.outerConeAngle);
                light.castsShadow = lightData.value("castsShadow", light.castsShadow);
                m_Engine->sceneSpotLights.push_back(light);
            }
            UI::Console::Log(fmt::format("  Loaded {} spot lights", m_Engine->sceneSpotLights.size()));
        }

        // Load camera
        if (sceneData.contains("camera")) {
            const auto& cam = sceneData["camera"];
            if (cam.contains("position")) {
                m_Engine->mainCamera.position = glm::vec3(cam["position"][0], cam["position"][1], cam["position"][2]);
            }
            if (cam.contains("pitch")) {
                m_Engine->mainCamera.pitch = cam["pitch"];
            }
            if (cam.contains("yaw")) {
                m_Engine->mainCamera.yaw = cam["yaw"];
            }
            UI::Console::Log("  Loaded camera position");
        }

        // Load referenced model scenes
        if (sceneData.contains("scenes")) {
            for (const auto& sceneEntry : sceneData["scenes"]) {
                std::string name = sceneEntry.value("name", "");
                std::string gltfFile = sceneEntry.value("file", "");

                if (name.empty() || gltfFile.empty()) continue;

                // Check if scene file exists
                if (!fs::exists(gltfFile)) {
                    UI::Console::Warn("Scene file not found, skipping: " + gltfFile);
                    continue;
                }

                // Store the file path
                m_SceneFilePaths[name] = gltfFile;
                m_Engine->sceneFilePaths[name] = gltfFile;

                // Actually load the scene/model file
                UI::Console::Log("  Loading scene asset: " + gltfFile);
                auto gltfScene = loadSceneAsset(m_Engine, gltfFile);
                if (gltfScene.has_value()) {
                    m_Engine->loadedScenes[name] = *gltfScene;
                    m_LoadedScenes[name] = *gltfScene;
                    UI::Console::Log("    Loaded successfully");

                    // Restore per-node local transforms
                    if (sceneEntry.contains("nodes") && sceneEntry["nodes"].is_array()) {
                        auto loadedIt = m_Engine->loadedScenes.find(name);
                        if (loadedIt != m_Engine->loadedScenes.end() && loadedIt->second) {
                            auto& loadedScene = loadedIt->second;
                            for (const auto& nodeData : sceneEntry["nodes"]) {
                                if (!nodeData.contains("name") || !nodeData.contains("localTransform")) continue;
                                if (!nodeData["localTransform"].is_array() || nodeData["localTransform"].size() != 16) continue;

                                const std::string nodeName = nodeData["name"].get<std::string>();
                                auto nodeIt = loadedScene->nodes.find(nodeName);
                                if (nodeIt == loadedScene->nodes.end() || !nodeIt->second) continue;

                                glm::mat4 local(1.0f);
                                for (int i = 0; i < 16; ++i) {
                                    local[i / 4][i % 4] = nodeData["localTransform"][i].get<float>();
                                }
                                nodeIt->second->localTransform = local;
                            }

                            for (auto& top : loadedScene->topNodes) {
                                if (top) top->refreshTransform(glm::mat4(1.0f));
                            }
                        }
                    }

                    // Restore material runtime overrides
                    if (sceneEntry.contains("materials") && sceneEntry["materials"].is_array()) {
                        auto loadedIt = m_Engine->loadedScenes.find(name);
                        if (loadedIt != m_Engine->loadedScenes.end() && loadedIt->second) {
                            auto& loadedScene = loadedIt->second;
                            if (loadedScene->materialDataBuffer.buffer != VK_NULL_HANDLE &&
                                loadedScene->materialDataBuffer.allocation != VK_NULL_HANDLE &&
                                loadedScene->materialDataBuffer.size >= sizeof(GLTFMetallic_Roughness::MaterialConstants)) {

                                void* mappedData = nullptr;
                                VkResult mapResult = vmaMapMemory(m_Engine->_allocator, loadedScene->materialDataBuffer.allocation, &mappedData);
                                if (mapResult == VK_SUCCESS && mappedData) {
                                    auto* bytes = static_cast<uint8_t*>(mappedData);

                                    for (const auto& matData : sceneEntry["materials"]) {
                                        if (!matData.is_object()) continue;

                                        std::string matName = matData.value("name", "");
                                        uint32_t savedOffset = matData.value("bufferOffset", 0u);

                                        uint32_t targetOffset = savedOffset;
                                        if (!matName.empty()) {
                                            auto mit = loadedScene->materials.find(matName);
                                            if (mit != loadedScene->materials.end() && mit->second) {
                                                targetOffset = mit->second->bufferOffset;
                                            }
                                        }

                                        const size_t offset = static_cast<size_t>(targetOffset);
                                        if (offset + sizeof(GLTFMetallic_Roughness::MaterialConstants) > loadedScene->materialDataBuffer.size) {
                                            continue;
                                        }

                                        auto* dst = reinterpret_cast<GLTFMetallic_Roughness::MaterialConstants*>(bytes + offset);
                                        auto constants = *dst; // preserve unspecified fields for backward compatibility

                                        if (matData.contains("colorFactors") && matData["colorFactors"].is_array() && matData["colorFactors"].size() == 4) {
                                            constants.colorFactors = glm::vec4(
                                                matData["colorFactors"][0].get<float>(),
                                                matData["colorFactors"][1].get<float>(),
                                                matData["colorFactors"][2].get<float>(),
                                                matData["colorFactors"][3].get<float>());
                                        }

                                        if (matData.contains("metalRoughFactors") && matData["metalRoughFactors"].is_array() && matData["metalRoughFactors"].size() == 4) {
                                            constants.metal_rough_factors = glm::vec4(
                                                matData["metalRoughFactors"][0].get<float>(),
                                                matData["metalRoughFactors"][1].get<float>(),
                                                matData["metalRoughFactors"][2].get<float>(),
                                                matData["metalRoughFactors"][3].get<float>());
                                        }

                                        constants.colorTexID = matData.value("colorTexID", constants.colorTexID);
                                        constants.metalRoughTexID = matData.value("metalRoughTexID", constants.metalRoughTexID);
                                        constants.normalTexID = matData.value("normalTexID", constants.normalTexID);
                                        constants.emissiveTexID = matData.value("emissiveTexID", constants.emissiveTexID);

                                        if (matData.contains("extra") && matData["extra"].is_array()) {
                                            const auto& e = matData["extra"];
                                            const size_t count = std::min<size_t>(13, e.size());
                                            for (size_t i = 0; i < count; ++i) {
                                                if (!e[i].is_array() || e[i].size() != 4) continue;
                                                constants.extra[i] = glm::vec4(
                                                    e[i][0].get<float>(),
                                                    e[i][1].get<float>(),
                                                    e[i][2].get<float>(),
                                                    e[i][3].get<float>());
                                            }
                                        }

                                        *dst = constants;
                                    }

                                    vmaUnmapMemory(m_Engine->_allocator, loadedScene->materialDataBuffer.allocation);
                                    vmaFlushAllocation(m_Engine->_allocator, loadedScene->materialDataBuffer.allocation, 0, VK_WHOLE_SIZE);
                                } else {
                                    UI::Console::Warn("Failed to map material buffer for scene load: " + name);
                                }
                            }
                        }
                    }
                } else {
                    UI::Console::Error("    Failed to load scene asset");
                }
            }
        }

        // Restore animation runtime state + graph
        if (sceneData.contains("animation") && sceneData["animation"].is_object()) {
            const auto& anim = sceneData["animation"];

            auto findClipIndex = [&](const std::string& clipName, const std::string& clipSource) -> int {
                if (!clipName.empty() && !clipSource.empty()) {
                    for (int i = 0; i < static_cast<int>(m_Engine->animationClips.size()); ++i) {
                        const auto& c = m_Engine->animationClips[i];
                        if (c.name == clipName && c.sourceScene == clipSource) return i;
                    }
                }
                if (!clipName.empty()) {
                    for (int i = 0; i < static_cast<int>(m_Engine->animationClips.size()); ++i) {
                        const auto& c = m_Engine->animationClips[i];
                        if (c.name == clipName) return i;
                    }
                }
                return -1;
            };

            if (anim.contains("clips") && anim["clips"].is_array()) {
                for (auto& clip : m_Engine->animationClips) {
                    clip.isPlaying = false;
                }

                for (const auto& savedClip : anim["clips"]) {
                    std::string clipName = savedClip.value("name", "");
                    std::string clipSource = savedClip.value("sourceScene", "");
                    int clipIndex = findClipIndex(clipName, clipSource);
                    if (clipIndex < 0 || clipIndex >= static_cast<int>(m_Engine->animationClips.size())) continue;

                    auto& clip = m_Engine->animationClips[clipIndex];
                    clip.loop = savedClip.value("loop", clip.loop);
                    clip.pingPong = savedClip.value("pingPong", clip.pingPong);
                    clip.reverse = savedClip.value("reverse", clip.reverse);
                    clip.speed = savedClip.value("speed", clip.speed);
                    clip.currentTime = savedClip.value("currentTime", clip.currentTime);
                    if (clip.duration > 0.0f) {
                        clip.currentTime = glm::clamp(clip.currentTime, 0.0f, clip.duration);
                    } else {
                        clip.currentTime = 0.0f;
                    }
                    clip.isPlaying = savedClip.value("isPlaying", false);
                }
            }

            if (anim.contains("activeAnimationIndex")) {
                int active = anim.value("activeAnimationIndex", -1);
                m_Engine->activeAnimationIndex = (active >= 0 && active < static_cast<int>(m_Engine->animationClips.size())) ? active : -1;
            }
            if (anim.contains("activeSkeletonIndex")) {
                int activeSkel = anim.value("activeSkeletonIndex", -1);
                m_Engine->activeSkeletonIndex = (activeSkel >= 0 && activeSkel < static_cast<int>(m_Engine->skeletons.size())) ? activeSkel : -1;
            }

            if (anim.contains("graph") && anim["graph"].is_object()) {
                const auto& graphData = anim["graph"];
                auto& graph = m_Engine->animationGraph;
                graph.states.clear();
                graph.transitions.clear();
                graph.parameters.clear();

                if (graphData.contains("states") && graphData["states"].is_array()) {
                    for (const auto& stateData : graphData["states"]) {
                        AnimationGraphStateData state;
                        state.name = stateData.value("name", "State");
                        state.isDefault = stateData.value("isDefault", false);
                        state.positionX = stateData.value("positionX", 120.0f);
                        state.positionY = stateData.value("positionY", 100.0f);

                        std::string clipName = stateData.value("clipName", "");
                        std::string clipSource = stateData.value("clipSourceScene", "");
                        int clipIndex = findClipIndex(clipName, clipSource);
                        if (clipIndex < 0) {
                            int rawIndex = stateData.value("clipIndex", -1);
                            if (rawIndex >= 0 && rawIndex < static_cast<int>(m_Engine->animationClips.size())) {
                                clipIndex = rawIndex;
                            }
                        }
                        state.clipIndex = clipIndex;

                        if (state.clipIndex >= 0 && state.clipIndex < static_cast<int>(m_Engine->animationClips.size())) {
                            graph.states.push_back(state);
                        }
                    }
                }

                if (graphData.contains("transitions") && graphData["transitions"].is_array()) {
                    for (const auto& trData : graphData["transitions"]) {
                        AnimationGraphTransitionData tr;
                        tr.fromState = trData.value("fromState", -1);
                        tr.toState = trData.value("toState", -1);
                        tr.parameter = trData.value("parameter", "speed");
                        tr.comparison = trData.value("comparison", 0);
                        tr.threshold = trData.value("threshold", 0.5f);
                        tr.hasExitTime = trData.value("hasExitTime", true);
                        tr.exitTime = trData.value("exitTime", 0.9f);
                        tr.blendTime = trData.value("blendTime", 0.2f);
                        tr.enabled = trData.value("enabled", true);
                        if (tr.fromState >= 0 && tr.fromState < static_cast<int>(graph.states.size()) &&
                            tr.toState >= 0 && tr.toState < static_cast<int>(graph.states.size())) {
                            graph.transitions.push_back(tr);
                        }
                    }
                }

                if (graphData.contains("parameters") && graphData["parameters"].is_array()) {
                    for (const auto& pData : graphData["parameters"]) {
                        AnimationGraphParameter p;
                        p.name = pData.value("name", "");
                        p.value = pData.value("value", 0.0f);
                        p.isBool = pData.value("isBool", false);
                        if (!p.name.empty()) graph.parameters.push_back(p);
                    }
                }
                if (graph.parameters.empty()) {
                    graph.parameters.push_back({"speed", 0.0f, false});
                    graph.parameters.push_back({"isGrounded", 1.0f, true});
                }

                graph.enabled = graphData.value("enabled", false);
                graph.activeState = graphData.value("activeState", -1);
                graph.nextState = graphData.value("nextState", -1);
                graph.blending = graphData.value("blending", false);
                graph.blendDuration = graphData.value("blendDuration", 0.0f);
                graph.blendElapsed = graphData.value("blendElapsed", 0.0f);

                if (graph.activeState < 0 || graph.activeState >= static_cast<int>(graph.states.size())) {
                    graph.activeState = graph.states.empty() ? -1 : 0;
                }
                if (graph.nextState < 0 || graph.nextState >= static_cast<int>(graph.states.size())) {
                    graph.nextState = -1;
                    graph.blending = false;
                }
            }

            if (m_Engine->activeAnimationIndex < 0 || m_Engine->activeAnimationIndex >= static_cast<int>(m_Engine->animationClips.size())) {
                for (int i = 0; i < static_cast<int>(m_Engine->animationClips.size()); ++i) {
                    if (m_Engine->animationClips[i].isPlaying) {
                        m_Engine->activeAnimationIndex = i;
                        break;
                    }
                }
            }
            if (m_Engine->activeAnimationIndex >= 0 &&
                m_Engine->activeAnimationIndex < static_cast<int>(m_Engine->animationClips.size()) &&
                m_Engine->activeSkeletonIndex < 0) {
                int skel = m_Engine->animationClips[m_Engine->activeAnimationIndex].skeletonIndex;
                if (skel >= 0 && skel < static_cast<int>(m_Engine->skeletons.size())) {
                    m_Engine->activeSkeletonIndex = skel;
                }
            }

            m_Engine->updateAnimations(0.0f);
        }

        // Restore editor/render settings
        if (sceneData.contains("settings") && sceneData["settings"].is_object()) {
            const auto& settings = sceneData["settings"];

            m_Engine->_currentViewMode = static_cast<VulkanEngine::ViewMode>(
                settings.value("viewMode", static_cast<int>(m_Engine->_currentViewMode)));
            m_Engine->_showGrid = settings.value("showGrid", m_Engine->_showGrid);
            m_Engine->_showOutline = settings.value("showOutline", m_Engine->_showOutline);
            m_Engine->enableBackfaceCulling = settings.value("enableBackfaceCulling", m_Engine->enableBackfaceCulling);
            m_Engine->renderScale = settings.value("renderScale", m_Engine->renderScale);
            m_Engine->shadowsEnabled = settings.value("shadowsEnabled", m_Engine->shadowsEnabled);
            m_Engine->pointLightShadowsEnabled = settings.value("pointLightShadowsEnabled", m_Engine->pointLightShadowsEnabled);
            m_Engine->sunEnabled = settings.value("sunEnabled", m_Engine->sunEnabled);
            m_Engine->savedSunIntensity = settings.value("savedSunIntensity", m_Engine->savedSunIntensity);
            m_Engine->snapEnabled = settings.value("snapEnabled", m_Engine->snapEnabled);
            m_Engine->snapPositionValue = settings.value("snapPositionValue", m_Engine->snapPositionValue);
            m_Engine->snapRotationEnabled = settings.value("snapRotationEnabled", m_Engine->snapRotationEnabled);
            m_Engine->snapRotationAngle = settings.value("snapRotationAngle", m_Engine->snapRotationAngle);
            m_Engine->snapScaleEnabled = settings.value("snapScaleEnabled", m_Engine->snapScaleEnabled);
            m_Engine->snapScaleValue = settings.value("snapScaleValue", m_Engine->snapScaleValue);

            if (settings.contains("renderSettings") && settings["renderSettings"].is_object()) {
                const auto& rs = settings["renderSettings"];
                auto& r = m_Engine->_renderSettings;
                r.ssaoEnabled = rs.value("ssaoEnabled", r.ssaoEnabled);
                r.ssaoSamples = rs.value("ssaoSamples", r.ssaoSamples);
                r.ssaoRadius = rs.value("ssaoRadius", r.ssaoRadius);
                r.ssaoIntensity = rs.value("ssaoIntensity", r.ssaoIntensity);
                r.ssaoBias = rs.value("ssaoBias", r.ssaoBias);
                r.ssaoBlurPasses = rs.value("ssaoBlurPasses", r.ssaoBlurPasses);
                r.bloomEnabled = rs.value("bloomEnabled", r.bloomEnabled);
                r.bloomThreshold = rs.value("bloomThreshold", r.bloomThreshold);
                r.bloomIntensity = rs.value("bloomIntensity", r.bloomIntensity);
                r.bloomMipLevels = rs.value("bloomMipLevels", r.bloomMipLevels);
                r.bloomRadius = rs.value("bloomRadius", r.bloomRadius);
                r.tonemappingEnabled = rs.value("tonemappingEnabled", r.tonemappingEnabled);
                r.tonemapOperator = rs.value("tonemapOperator", r.tonemapOperator);
                r.exposure = rs.value("exposure", r.exposure);
                r.gamma = rs.value("gamma", r.gamma);
                r.contrast = rs.value("contrast", r.contrast);
                r.saturation = rs.value("saturation", r.saturation);
                r.sharpness = rs.value("sharpness", r.sharpness);
                r.temperature = rs.value("temperature", r.temperature);
                r.tint = rs.value("tint", r.tint);
                r.ssrEnabled = rs.value("ssrEnabled", r.ssrEnabled);
                r.ssrMaxSteps = rs.value("ssrMaxSteps", r.ssrMaxSteps);
                r.ssrMaxDistance = rs.value("ssrMaxDistance", r.ssrMaxDistance);
                r.ssrThickness = rs.value("ssrThickness", r.ssrThickness);
                r.ssrRoughnessThreshold = rs.value("ssrRoughnessThreshold", r.ssrRoughnessThreshold);
                r.pcssEnabled = rs.value("pcssEnabled", r.pcssEnabled);
                r.pcssBlockerSamples = rs.value("pcssBlockerSamples", r.pcssBlockerSamples);
                r.pcssPCFSamples = rs.value("pcssPCFSamples", r.pcssPCFSamples);
                r.pcssLightSize = rs.value("pcssLightSize", r.pcssLightSize);
                r.pcssMinPenumbra = rs.value("pcssMinPenumbra", r.pcssMinPenumbra);
                r.contactShadowsEnabled = rs.value("contactShadowsEnabled", r.contactShadowsEnabled);
                r.contactShadowSteps = rs.value("contactShadowSteps", r.contactShadowSteps);
                r.contactShadowLength = rs.value("contactShadowLength", r.contactShadowLength);
                r.contactShadowFadeStart = rs.value("contactShadowFadeStart", r.contactShadowFadeStart);
                r.spotLightsEnabled = rs.value("spotLightsEnabled", r.spotLightsEnabled);
                r.maxSpotLights = rs.value("maxSpotLights", r.maxSpotLights);
                r.spotLightShadowsEnabled = rs.value("spotLightShadowsEnabled", r.spotLightShadowsEnabled);
                r.reflectionProbesEnabled = rs.value("reflectionProbesEnabled", r.reflectionProbesEnabled);
                r.globalSkyBlend = rs.value("globalSkyBlend", r.globalSkyBlend);
            }

            if (settings.contains("environment") && settings["environment"].is_object() && m_Engine->_environmentMap) {
                const auto& env = settings["environment"];
                auto& es = m_Engine->_environmentMap->settings;
                if (env.contains("skyColorTop") && env["skyColorTop"].is_array() && env["skyColorTop"].size() == 3) {
                    es.skyColorTop = glm::vec3(env["skyColorTop"][0], env["skyColorTop"][1], env["skyColorTop"][2]);
                }
                if (env.contains("skyColorHorizon") && env["skyColorHorizon"].is_array() && env["skyColorHorizon"].size() == 3) {
                    es.skyColorHorizon = glm::vec3(env["skyColorHorizon"][0], env["skyColorHorizon"][1], env["skyColorHorizon"][2]);
                }
                if (env.contains("groundColor") && env["groundColor"].is_array() && env["groundColor"].size() == 3) {
                    es.groundColor = glm::vec3(env["groundColor"][0], env["groundColor"][1], env["groundColor"][2]);
                }
                es.skyIntensity = env.value("skyIntensity", es.skyIntensity);
                es.exposure = env.value("exposure", es.exposure);
                es.rotation = env.value("rotation", es.rotation);
                es.useIBL = env.value("useIBL", es.useIBL);
                es.iblIntensity = env.value("iblIntensity", es.iblIntensity);

                std::string path = env.value("loadedPath", std::string{});
                bool envLoaded = false;
                if (!path.empty() && path != "cubemap faces" && fs::exists(path)) {
                    if (fs::is_directory(path)) {
                        std::array<std::string, 6> faces{};
                        if (collectCubemapFaces(path, faces)) {
                            envLoaded = m_Engine->_environmentMap->loadCubemapFaces(faces.data());
                        } else {
                            // Try first child directory with valid faces
                            for (const auto& child : fs::directory_iterator(path)) {
                                if (!child.is_directory()) continue;
                                if (collectCubemapFaces(child.path(), faces)) {
                                    envLoaded = m_Engine->_environmentMap->loadCubemapFaces(faces.data());
                                    break;
                                }
                            }
                        }
                    } else {
                        envLoaded = m_Engine->_environmentMap->loadFromFile(path);
                    }
                } else {
                    envLoaded = tryLoadCubemapFromAssetsFolder(m_Engine->_environmentMap.get());
                }

                if (!envLoaded) {
                    m_Engine->_environmentMap->generateProceduralSky();
                }

                // Reconnect environment to all consumers after scene load.
                if (m_Engine->_pathTracer && m_Engine->_environmentMap->getEnvironmentCubemap() != VK_NULL_HANDLE) {
                    m_Engine->_pathTracer->setEnvironmentCubemap(
                        m_Engine->_environmentMap->getEnvironmentCubemap(),
                        m_Engine->_environmentMap->getSampler());
                }
                m_Engine->updateSkyboxBgDescriptor();
                for (auto& probe : m_Engine->_reflectionProbes) {
                    probe.needsUpdate = true;
                }
                m_Engine->_reflectionFrameCounter = VulkanEngine::REFLECTION_UPDATE_INTERVAL;
            } else if (m_Engine && m_Engine->_environmentMap) {
                // Even if scene file had no env block, try to keep cubemap/background valid.
                if (!tryLoadCubemapFromAssetsFolder(m_Engine->_environmentMap.get())) {
                    m_Engine->_environmentMap->generateProceduralSky();
                }
                if (m_Engine->_pathTracer && m_Engine->_environmentMap->getEnvironmentCubemap() != VK_NULL_HANDLE) {
                    m_Engine->_pathTracer->setEnvironmentCubemap(
                        m_Engine->_environmentMap->getEnvironmentCubemap(),
                        m_Engine->_environmentMap->getSampler());
                } else {
                    if (m_Engine->_pathTracer) {
                        m_Engine->_pathTracer->setEnvironmentCubemap(VK_NULL_HANDLE, VK_NULL_HANDLE);
                    }
                }
                m_Engine->updateSkyboxBgDescriptor();
                for (auto& probe : m_Engine->_reflectionProbes) {
                    probe.needsUpdate = true;
                }
                m_Engine->_reflectionFrameCounter = VulkanEngine::REFLECTION_UPDATE_INTERVAL;
            }
        }

        // Restore physics state
        if (sceneData.contains("physics") && sceneData["physics"].is_object()) {
            const auto& physics = sceneData["physics"];
            m_Engine->physicsEnabled = physics.value("enabled", m_Engine->physicsEnabled);

            if (physics.contains("gravity") && physics["gravity"].is_array() && physics["gravity"].size() == 3) {
                m_Engine->physicsSettings.gravity = glm::vec3(
                    physics["gravity"][0].get<float>(),
                    physics["gravity"][1].get<float>(),
                    physics["gravity"][2].get<float>());
            }
            m_Engine->physicsSettings.timeStep = physics.value("timeStep", m_Engine->physicsSettings.timeStep);
            m_Engine->physicsSettings.maxSubSteps = physics.value("maxSubSteps", m_Engine->physicsSettings.maxSubSteps);
            m_Engine->physicsSettings.debugDraw = physics.value("debugDraw", m_Engine->physicsSettings.debugDraw);
            m_Engine->physicsSettings.paused = physics.value("paused", m_Engine->physicsSettings.paused);

            m_Engine->physicsBodies.clear();
            if (physics.contains("bodies") && physics["bodies"].is_array()) {
                for (const auto& b : physics["bodies"]) {
                    if (!b.is_object()) continue;
                    PhysicsBodyData body;
                    body.name = b.value("name", std::string{});
                    body.type = b.value("type", body.type);
                    if (b.contains("position") && b["position"].is_array() && b["position"].size() == 3) {
                        body.position = glm::vec3(b["position"][0].get<float>(), b["position"][1].get<float>(), b["position"][2].get<float>());
                    }
                    if (b.contains("rotation") && b["rotation"].is_array() && b["rotation"].size() == 4) {
                        body.rotation = glm::quat(
                            b["rotation"][3].get<float>(),
                            b["rotation"][0].get<float>(),
                            b["rotation"][1].get<float>(),
                            b["rotation"][2].get<float>());
                    }
                    if (b.contains("velocity") && b["velocity"].is_array() && b["velocity"].size() == 3) {
                        body.velocity = glm::vec3(b["velocity"][0].get<float>(), b["velocity"][1].get<float>(), b["velocity"][2].get<float>());
                    }
                    if (b.contains("angularVelocity") && b["angularVelocity"].is_array() && b["angularVelocity"].size() == 3) {
                        body.angularVelocity = glm::vec3(b["angularVelocity"][0].get<float>(), b["angularVelocity"][1].get<float>(), b["angularVelocity"][2].get<float>());
                    }
                    body.mass = b.value("mass", body.mass);
                    body.friction = b.value("friction", body.friction);
                    body.restitution = b.value("restitution", body.restitution);
                    body.isAwake = b.value("isAwake", body.isAwake);
                    body.colliderType = b.value("colliderType", body.colliderType);
                    if (b.contains("colliderSize") && b["colliderSize"].is_array() && b["colliderSize"].size() == 3) {
                        body.colliderSize = glm::vec3(b["colliderSize"][0].get<float>(), b["colliderSize"][1].get<float>(), b["colliderSize"][2].get<float>());
                    }
                    m_Engine->physicsBodies.push_back(body);
                }
            }

            m_Engine->physicsConstraints.clear();
            if (physics.contains("constraints") && physics["constraints"].is_array()) {
                for (const auto& c : physics["constraints"]) {
                    if (!c.is_object()) continue;
                    PhysicsConstraintData pc;
                    pc.name = c.value("name", std::string{});
                    pc.bodyA = c.value("bodyA", pc.bodyA);
                    pc.bodyB = c.value("bodyB", pc.bodyB);
                    pc.type = c.value("type", pc.type);
                    if (c.contains("pivotA") && c["pivotA"].is_array() && c["pivotA"].size() == 3) {
                        pc.pivotA = glm::vec3(c["pivotA"][0].get<float>(), c["pivotA"][1].get<float>(), c["pivotA"][2].get<float>());
                    }
                    if (c.contains("pivotB") && c["pivotB"].is_array() && c["pivotB"].size() == 3) {
                        pc.pivotB = glm::vec3(c["pivotB"][0].get<float>(), c["pivotB"][1].get<float>(), c["pivotB"][2].get<float>());
                    }
                    m_Engine->physicsConstraints.push_back(pc);
                }
            }
        }

        // Restore UI layout state
        if (sceneData.contains("uiLayout") && sceneData["uiLayout"].is_object()) {
            const auto& uiLayout = sceneData["uiLayout"];
            const bool layoutLocked = uiLayout.value("layoutLocked", false);
            Yalaz::UI::EditorUI::Get().SetLayoutLocked(layoutLocked);

            if (uiLayout.contains("views") && uiLayout["views"].is_array()) {
                auto& views = Yalaz::UI::ViewManager::Get().GetViews();
                for (const auto& saved : uiLayout["views"]) {
                    if (!saved.is_object()) continue;
                    const std::string viewName = saved.value("name", std::string{});
                    if (viewName.empty()) continue;

                    for (const auto& viewPtr : views) {
                        if (!viewPtr || viewPtr->GetName() != viewName) continue;
                        viewPtr->SetOpen(saved.value("isOpen", viewPtr->IsOpen()));

                        if (saved.contains("position") && saved["position"].is_array() && saved["position"].size() == 2 &&
                            saved.contains("size") && saved["size"].is_array() && saved["size"].size() == 2) {
                            ImVec2 pos(saved["position"][0].get<float>(), saved["position"][1].get<float>());
                            ImVec2 size(saved["size"][0].get<float>(), saved["size"][1].get<float>());
                            viewPtr->SetInitialLayout(pos, size);
                            viewPtr->ClearDynamicLayout();
                            viewPtr->ResetLayout();
                        }
                        break;
                    }
                }
            }
        }

        AddRecentScene(filePath);
        UI::Console::Log("Scene loaded successfully: " + filePath);

        return true;

    } catch (const json::exception& e) {
        UI::Console::Error("JSON parse error: " + std::string(e.what()));
        return false;
    } catch (const std::exception& e) {
        UI::Console::Error("Failed to load scene: " + std::string(e.what()));
        return false;
    }
}

void SceneManager::AddRecentScene(const std::string& path) {
    // Remove if already exists
    auto it = std::find(m_RecentScenes.begin(), m_RecentScenes.end(), path);
    if (it != m_RecentScenes.end()) {
        m_RecentScenes.erase(it);
    }

    // Add to front
    m_RecentScenes.insert(m_RecentScenes.begin(), path);

    // Limit to 10 recent scenes
    if (m_RecentScenes.size() > 10) {
        m_RecentScenes.resize(10);
    }
}

void SceneManager::RemoveRecentScene(const std::string& path) {
    auto it = std::remove(m_RecentScenes.begin(), m_RecentScenes.end(), path);
    if (it != m_RecentScenes.end()) {
        m_RecentScenes.erase(it, m_RecentScenes.end());
    }
}

bool SceneManager::DeleteSceneFile(const std::string& filePath) {
    try {
        if (!fs::exists(filePath)) {
            RemoveRecentScene(filePath);
            return false;
        }
        if (!fs::is_regular_file(filePath)) return false;
        if (fs::path(filePath).extension() != ".yscene") return false;
        const bool removed = fs::remove(filePath);
        if (removed) {
            RemoveRecentScene(filePath);
            UI::Console::Log("Scene deleted: " + filePath);
        }
        return removed;
    } catch (const std::exception& e) {
        UI::Console::Error("Failed to delete scene: " + std::string(e.what()));
        return false;
    }
}

} // namespace Yalaz::Scene
