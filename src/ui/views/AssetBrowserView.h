#pragma once
// =============================================================================
// YALAZ ENGINE - Asset Browser View
// =============================================================================
// Professional asset browser with dynamic features:
// - Multiple scene loading/unloading
// - Texture drag-drop to material slots
// - Grid/list view modes with thumbnail previews
// - Search and filtering
// - Asset type color coding with visual previews
// - Loaded scenes management panel
// =============================================================================

#include "EditorView.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <vulkan/vulkan.h>

namespace Yalaz::UI {

enum class AssetType {
    Unknown,
    Folder,
    Model,
    Texture,
    Shader,
    Scene,
    Material,
    Audio,
    HDRI
};

struct AssetEntry {
    std::string name;
    std::string path;
    AssetType type = AssetType::Unknown;
    bool isDirectory = false;
    size_t fileSize = 0;  // File size in bytes
};

// Cached thumbnail for an asset
struct ThumbnailEntry {
    VkDescriptorSet imguiDescriptor = VK_NULL_HANDLE;  // ImTextureID for ImGui::Image
    bool loaded = false;
    bool failed = false;
};

class AssetBrowserView : public EditorView {
public:
    AssetBrowserView() : EditorView("Asset Browser", "[A]", ViewCategory::Assets) {}
    ~AssetBrowserView();

    void OnInit(VulkanEngine* engine) override;
    void OnRender() override;

    // Public API for other views
    const std::string& GetLastSelectedTexture() const { return m_LastSelectedTexture; }
    void ClearLastSelectedTexture() { m_LastSelectedTexture.clear(); }
    const std::string& GetSelectedScene() const { return m_SelectedScene; }

private:
    void RenderFileBrowser();
    void RenderLoadedScenes();
    void RenderPathBar();
    void RenderAssetGrid();
    void RenderAssetContextMenu(const AssetEntry& asset);
    void HandleAssetClick(const AssetEntry& asset);
    void RefreshDirectory();

    AssetType GetAssetType(const std::string& ext);
    const char* GetAssetTypeName(AssetType type);

    // Thumbnail system
    ThumbnailEntry* GetOrLoadThumbnail(const AssetEntry& asset);
    void RenderAssetCard(const AssetEntry& asset, float size);
    void RenderFolderIcon(ImDrawList* drawList, ImVec2 pos, ImVec2 size);
    void RenderModelIcon(ImDrawList* drawList, ImVec2 pos, ImVec2 size);
    void RenderShaderIcon(ImDrawList* drawList, ImVec2 pos, ImVec2 size);
    void RenderAudioIcon(ImDrawList* drawList, ImVec2 pos, ImVec2 size);
    void RenderSceneIcon(ImDrawList* drawList, ImVec2 pos, ImVec2 size);
    void RenderFileIcon(ImDrawList* drawList, ImVec2 pos, ImVec2 size, const char* ext);
    void ClearThumbnailCache();
    static std::string FormatFileSize(size_t bytes);

    std::string m_CurrentPath;
    std::string m_RootPath;      // Root assets folder path
    std::vector<AssetEntry> m_Assets;
    char m_SearchBuffer[256] = "";
    char m_PathBuffer[1024] = "";

    // View settings
    bool m_GridView = true;
    int m_ThumbnailSize = 90;
    bool m_ShowLoadedScenes = true;

    // Thumbnail cache: path -> thumbnail data
    std::unordered_map<std::string, ThumbnailEntry> m_ThumbnailCache;
    std::string m_CachedDirectory;  // Track which directory thumbnails belong to

    // Selection
    std::string m_SelectedScene;
    std::string m_LastSelectedTexture;
    std::string m_PendingNavigatePath;
};

} // namespace Yalaz::UI
