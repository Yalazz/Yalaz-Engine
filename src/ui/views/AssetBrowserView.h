#pragma once
// =============================================================================
// YALAZ ENGINE - Asset Browser View
// =============================================================================
// Professional asset browser with dynamic features:
// - Multiple scene loading/unloading
// - Texture drag-drop to material slots
// - Grid/list view modes
// - Search and filtering
// - Asset type color coding
// - Loaded scenes management panel
// =============================================================================

#include "EditorView.h"
#include <vector>
#include <string>

namespace Yalaz::UI {

enum class AssetType {
    Unknown,
    Folder,
    Model,
    Texture,
    Shader,
    Scene,
    Material
};

struct AssetEntry {
    std::string name;
    std::string path;
    AssetType type = AssetType::Unknown;
    bool isDirectory = false;
};

class AssetBrowserView : public EditorView {
public:
    AssetBrowserView() : EditorView("Asset Browser", "[A]", ViewCategory::Assets) {}

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
    const char* GetAssetIcon(AssetType type);
    const char* GetAssetTypeName(AssetType type);

    std::string m_CurrentPath;
    std::vector<AssetEntry> m_Assets;
    char m_SearchBuffer[256] = "";

    // View settings
    bool m_GridView = true;
    int m_ThumbnailSize = 80;
    bool m_ShowLoadedScenes = true;

    // Selection
    std::string m_SelectedScene;
    std::string m_LastSelectedTexture;
};

} // namespace Yalaz::UI
