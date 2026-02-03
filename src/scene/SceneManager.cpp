#include "SceneManager.h"
#include "vk_engine.h"  // For MeshNode, LoadedGLTF
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
        sceneData["version"] = "1.0";
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

        // Clear existing scenes (optional - could be configurable)
        // ClearAllScenes();

        // Load each GLTF scene
        if (sceneData.contains("scenes")) {
            for (const auto& sceneEntry : sceneData["scenes"]) {
                std::string name = sceneEntry.value("name", "");
                std::string gltfFile = sceneEntry.value("file", "");

                if (name.empty() || gltfFile.empty()) continue;

                // Check if GLTF file exists
                if (!fs::exists(gltfFile)) {
                    UI::Console::Warn("GLTF file not found, skipping: " + gltfFile);
                    continue;
                }

                // Store the file path for loading
                m_SceneFilePaths[name] = gltfFile;
                UI::Console::Log("  Scene: " + name + " -> " + gltfFile);

                // Note: Actual GLTF loading is handled by VulkanEngine
                // The transforms will be applied after loading

                // Store node transforms for later application
                if (sceneEntry.contains("nodes")) {
                    for (const auto& nodeData : sceneEntry["nodes"]) {
                        // Transforms will be applied when scene is loaded
                        // This requires integration with VulkanEngine
                    }
                }
            }
        }

        AddRecentScene(filePath);
        UI::Console::Log("Scene configuration loaded: " + filePath);

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
