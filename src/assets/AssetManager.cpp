#include "AssetManager.h"
#include <fmt/core.h>

namespace Yalaz::Assets {

void AssetManager::OnInit() {
    fmt::print("[AssetManager] Initialized\n");
}

void AssetManager::OnShutdown() {
    fmt::print("[AssetManager] Shutdown\n");
    ClearAll();
}

std::shared_ptr<LoadedGLTF> AssetManager::LoadGLTF(const std::string& filePath, const std::string& name) {
    // Check cache first
    auto it = m_LoadedGLTFs.find(name);
    if (it != m_LoadedGLTFs.end()) {
        return it->second;
    }

    // During migration, actual loading is done by VulkanEngine
    // This will be implemented when vk_loader is migrated
    fmt::print("[AssetManager] LoadGLTF: {} as '{}'\n", filePath, name);

    m_PathToName[filePath] = name;

    return nullptr;
}

GPUMeshBuffers AssetManager::LoadOBJ(const std::string& filePath) {
    // Check cache
    auto it = m_LoadedMeshes.find(filePath);
    if (it != m_LoadedMeshes.end()) {
        return it->second;
    }

    // During migration, actual loading is done by VulkanEngine
    fmt::print("[AssetManager] LoadOBJ: {}\n", filePath);

    return {};
}

AllocatedImage AssetManager::LoadTexture(const std::string& filePath) {
    // During migration, texture loading is done by VulkanEngine
    fmt::print("[AssetManager] LoadTexture: {}\n", filePath);
    return {};
}

TextureID AssetManager::CacheTexture(const std::string& name, VkImageView view, VkSampler sampler) {
    VkDescriptorImageInfo info{};
    info.sampler = sampler;
    info.imageView = view;
    info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    return m_TextureCache.AddTexture(info, name);
}

TextureID AssetManager::GetCachedTexture(const std::string& name) const {
    auto it = m_TextureCache.NameMap.find(name);
    if (it != m_TextureCache.NameMap.end()) {
        return it->second;
    }
    return INVALID_TEXTURE_ID;
}

void AssetManager::UnloadAsset(const std::string& name) {
    m_LoadedGLTFs.erase(name);
    m_LoadedMeshes.erase(name);
}

void AssetManager::ClearAll() {
    m_LoadedGLTFs.clear();
    m_LoadedMeshes.clear();
    m_TextureCache.Cache.clear();
    m_TextureCache.NameMap.clear();
    m_PathToName.clear();
}

bool AssetManager::IsLoaded(const std::string& name) const {
    return m_LoadedGLTFs.count(name) > 0 || m_LoadedMeshes.count(name) > 0;
}

} // namespace Yalaz::Assets
