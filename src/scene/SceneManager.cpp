#include "SceneManager.h"
#include "vk_engine.h"  // For MeshNode, LoadedGLTF
#include "vk_loader.h"  // For loadGltf function
#include "ui/views/ConsoleView.h"
#include <fmt/core.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace Yalaz::Scene {

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
        sceneData["version"] = "1.2";
        sceneData["engine"] = "Yalaz Engine";

        // Save loaded GLTF scenes
        json scenesArray = json::array();
        for (const auto& [name, path] : m_SceneFilePaths) {
            json sceneEntry;
            sceneEntry["name"] = name;
            sceneEntry["file"] = path;

            // Save transforms for nodes in this scene
            auto sceneIt = m_LoadedScenes.find(name);
            if (sceneIt != m_LoadedScenes.end() && sceneIt->second) {
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
                primData["emission"] = {prim.emission.r, prim.emission.g, prim.emission.b};
                primData["useFaceColors"] = prim.useFaceColors;
                primData["visible"] = prim.visible;
                primData["materialType"] = static_cast<int>(prim.materialType);
                primData["passType"] = static_cast<int>(prim.passType);

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
                lightsArray.push_back(lightData);
            }
            sceneData["pointLights"] = lightsArray;

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

        // Clear existing data
        m_Engine->static_shapes.clear();
        m_Engine->scenePointLights.clear();

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
                if (primData.contains("emission")) {
                    prim.emission = glm::vec3(primData["emission"][0], primData["emission"][1], primData["emission"][2]);
                }

                prim.useFaceColors = primData.value("useFaceColors", false);
                prim.visible = primData.value("visible", true);
                prim.materialType = static_cast<ShaderOnlyMaterial>(primData.value("materialType", 0));
                prim.passType = static_cast<MaterialPass>(primData.value("passType", 0));

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
                m_Engine->scenePointLights.push_back(light);
            }
            UI::Console::Log(fmt::format("  Loaded {} point lights", m_Engine->scenePointLights.size()));

            // Sync light visualizations
            m_Engine->sync_point_light_billboards();
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

                // Actually load the scene/model file
                UI::Console::Log("  Loading scene asset: " + gltfFile);
                auto gltfScene = loadSceneAsset(m_Engine, gltfFile);
                if (gltfScene.has_value()) {
                    m_Engine->loadedScenes[name] = *gltfScene;
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

} // namespace Yalaz::Scene
