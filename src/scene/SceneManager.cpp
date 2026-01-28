#include "SceneManager.h"
#include "vk_engine.h"  // For MeshNode, LoadedGLTF
#include <fmt/core.h>

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

} // namespace Yalaz::Scene
