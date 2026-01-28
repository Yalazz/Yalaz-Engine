#pragma once

#include "core/ISubsystem.h"
#include "vk_types.h"
#include <string>
#include <memory>
#include <unordered_map>

// Forward declarations
struct LoadedGLTF;

namespace Yalaz::Assets {

/**
 * @brief Asset types
 */
enum class AssetType {
    Unknown,
    GLTF,
    OBJ,
    Texture,
    Shader
};

/**
 * @brief Central asset management
 *
 * Design Patterns:
 * - Singleton: Central asset registry
 * - Factory: Creates assets on demand
 * - Cache: Stores loaded assets
 *
 * SOLID:
 * - Single Responsibility: Only manages asset loading/caching
 */
class AssetManager : public Core::ISubsystem {
public:
    static AssetManager& Get() {
        static AssetManager instance;
        return instance;
    }

    // ISubsystem interface
    void OnInit() override;
    void OnShutdown() override;
    const char* GetName() const override { return "AssetManager"; }

    // =========================================================================
    // Model Loading
    // =========================================================================

    /**
     * @brief Load a GLTF/GLB model
     * @param filePath Path to the file
     * @param name Asset name for caching
     * @return Loaded GLTF or nullptr on failure
     */
    std::shared_ptr<LoadedGLTF> LoadGLTF(const std::string& filePath, const std::string& name);

    /**
     * @brief Load an OBJ model
     */
    GPUMeshBuffers LoadOBJ(const std::string& filePath);

    // =========================================================================
    // Texture Loading
    // =========================================================================

    /**
     * @brief Load a texture from file
     * @param filePath Path to image file
     * @return Loaded image or empty on failure
     */
    AllocatedImage LoadTexture(const std::string& filePath);

    // =========================================================================
    // Texture Cache
    // =========================================================================

    /**
     * @brief Get the texture cache
     */
    TextureCache& GetTextureCache() { return m_TextureCache; }
    const TextureCache& GetTextureCache() const { return m_TextureCache; }

    /**
     * @brief Add texture to cache
     */
    TextureID CacheTexture(const std::string& name, VkImageView view, VkSampler sampler);

    /**
     * @brief Get cached texture by name
     */
    TextureID GetCachedTexture(const std::string& name) const;

    // =========================================================================
    // Asset Management
    // =========================================================================

    /**
     * @brief Unload an asset by name
     */
    void UnloadAsset(const std::string& name);

    /**
     * @brief Clear all cached assets
     */
    void ClearAll();

    /**
     * @brief Check if asset is loaded
     */
    bool IsLoaded(const std::string& name) const;

    // =========================================================================
    // Migration Helpers
    // =========================================================================

    void SetTextureCache(const TextureCache& cache) { m_TextureCache = cache; }

private:
    AssetManager() = default;
    ~AssetManager() = default;

    // Loaded models
    std::unordered_map<std::string, std::shared_ptr<LoadedGLTF>> m_LoadedGLTFs;
    std::unordered_map<std::string, GPUMeshBuffers> m_LoadedMeshes;

    // Texture cache
    TextureCache m_TextureCache;

    // File path to asset name mapping
    std::unordered_map<std::string, std::string> m_PathToName;
};

} // namespace Yalaz::Assets
