#pragma once

#include "core/ISubsystem.h"
#include "vk_types.h"
#include <string>
#include <memory>
#include <unordered_map>
#include <vector>

// Forward declarations
struct LoadedGLTF;
struct Node;
struct MeshNode;

namespace Yalaz::Scene {

/**
 * @brief Manages the scene graph, loaded models, and transforms
 *
 * Design Patterns:
 * - Singleton: Central scene management
 * - Composite: Scene graph with hierarchical nodes
 *
 * SOLID:
 * - Single Responsibility: Only manages scene data
 */
class SceneManager : public Core::ISubsystem, public Core::IUpdatable {
public:
    static SceneManager& Get() {
        static SceneManager instance;
        return instance;
    }

    // ISubsystem interface
    void OnInit() override;
    void OnShutdown() override;
    void OnUpdate(float deltaTime) override;
    const char* GetName() const override { return "SceneManager"; }

    // =========================================================================
    // Scene Loading
    // =========================================================================

    /**
     * @brief Load a GLTF scene
     * @param filePath Path to .gltf or .glb file
     * @param sceneName Identifier for the scene
     * @return true if loaded successfully
     */
    bool LoadGLTF(const std::string& filePath, const std::string& sceneName);

    /**
     * @brief Unload a scene by name
     */
    void UnloadScene(const std::string& sceneName);

    /**
     * @brief Clear all loaded scenes
     */
    void ClearAllScenes();

    // =========================================================================
    // Scene Access
    // =========================================================================

    /**
     * @brief Get a loaded scene by name
     */
    std::shared_ptr<LoadedGLTF> GetScene(const std::string& name);

    /**
     * @brief Get all loaded scenes
     */
    const std::unordered_map<std::string, std::shared_ptr<LoadedGLTF>>& GetAllScenes() const {
        return m_LoadedScenes;
    }

    /**
     * @brief Get scene file paths for persistence
     */
    const std::unordered_map<std::string, std::string>& GetSceneFilePaths() const {
        return m_SceneFilePaths;
    }

    // =========================================================================
    // Node Management
    // =========================================================================

    /**
     * @brief Find a node by name
     */
    MeshNode* FindNodeByName(const std::string& name);

    /**
     * @brief Get all loaded nodes
     */
    const std::unordered_map<std::string, std::shared_ptr<Node>>& GetAllNodes() const {
        return m_LoadedNodes;
    }

    // =========================================================================
    // Draw Context (pointers to avoid circular dependency)
    // =========================================================================

    /**
     * @brief Get the main draw context for rendering
     */
    DrawContext* GetDrawContext() { return m_DrawContext; }
    const DrawContext* GetDrawContext() const { return m_DrawContext; }

    /**
     * @brief Get draw commands (opaque + transparent surfaces)
     */
    DrawContext* GetDrawCommands() { return m_DrawCommands; }

    /**
     * @brief Set draw context pointers (migration helper)
     */
    void SetDrawContexts(DrawContext* main, DrawContext* commands) {
        m_DrawContext = main;
        m_DrawCommands = commands;
    }

    // =========================================================================
    // Migration Helpers
    // =========================================================================

    void SetLoadedScenes(std::unordered_map<std::string, std::shared_ptr<LoadedGLTF>>& scenes) {
        m_LoadedScenes = scenes;
    }
    void SetSceneFilePaths(std::unordered_map<std::string, std::string>& paths) {
        m_SceneFilePaths = paths;
    }
    void SetLoadedNodes(std::unordered_map<std::string, std::shared_ptr<Node>>& nodes) {
        m_LoadedNodes = nodes;
    }

private:
    SceneManager() = default;
    ~SceneManager() = default;

    // Loaded scenes
    std::unordered_map<std::string, std::shared_ptr<LoadedGLTF>> m_LoadedScenes;
    std::unordered_map<std::string, std::string> m_SceneFilePaths;
    std::unordered_map<std::string, std::shared_ptr<Node>> m_LoadedNodes;

    // Draw context pointers (owned by VulkanEngine during migration)
    DrawContext* m_DrawContext = nullptr;
    DrawContext* m_DrawCommands = nullptr;

    // Helper for recursive node search
    MeshNode* FindNodeRecursive(std::shared_ptr<Node> node, const std::string& name);
};

} // namespace Yalaz::Scene
