#pragma once
// =============================================================================
// YALAZ ENGINE - Plugin Manager View
// =============================================================================
// Plugin/module management interface for loading, enabling, and configuring
// engine extensions and third-party integrations
// =============================================================================

#include "EditorView.h"
#include <vector>
#include <functional>

namespace Yalaz::UI {

// =============================================================================
// Plugin State
// =============================================================================
enum class PluginState {
    Unloaded,
    Loading,
    Loaded,
    Enabled,
    Disabled,
    Error
};

// =============================================================================
// Plugin Type
// =============================================================================
enum class PluginType {
    Core,           // Core engine functionality
    Rendering,      // Rendering extensions
    Editor,         // Editor tools
    Physics,        // Physics integration
    Audio,          // Audio systems
    Networking,     // Network features
    Script,         // Scripting language support
    ThirdParty      // Third-party integrations
};

// =============================================================================
// Plugin Dependency
// =============================================================================
struct PluginDependency {
    std::string name;
    std::string minVersion;
    bool required = true;
    bool satisfied = false;
};

// =============================================================================
// Plugin Info
// =============================================================================
struct PluginInfo {
    std::string id;
    std::string name;
    std::string description;
    std::string author;
    std::string version;
    std::string path;

    PluginType type = PluginType::ThirdParty;
    PluginState state = PluginState::Unloaded;

    std::vector<PluginDependency> dependencies;

    // Metadata
    bool isBuiltIn = false;
    bool canDisable = true;
    bool hasSettings = false;

    // Statistics
    float loadTimeMs = 0.0f;
    size_t memoryUsage = 0;

    // Error info
    std::string errorMessage;
};

// =============================================================================
// Plugin Manager Settings
// =============================================================================
struct PluginManagerSettings {
    bool showBuiltIn = true;
    bool showDisabled = true;
    bool showErrored = true;

    std::string searchFilter;
    PluginType typeFilter = PluginType::ThirdParty;
    bool filterByType = false;

    bool autoLoadPlugins = true;
};

// =============================================================================
// Plugin Manager View
// =============================================================================
class PluginManagerView : public EditorView {
public:
    PluginManagerView();
    ~PluginManagerView() override = default;

    // View interface
    const char* GetDisplayName() const override { return "Plugin Manager"; }
    ViewCategory GetCategory() const override { return ViewCategory::System; }
    ViewFlags GetFlags() const override;

    // Lifecycle
    void OnInit(VulkanEngine* engine) override;
    void OnUpdate(float deltaTime) override;

    // Plugin management
    void RefreshPluginList();
    void LoadPlugin(const std::string& id);
    void UnloadPlugin(const std::string& id);
    void EnablePlugin(const std::string& id);
    void DisablePlugin(const std::string& id);

    // Settings
    PluginManagerSettings& GetSettings() { return m_Settings; }

protected:
    void OnRenderContent() override;
    void OnRenderToolbar() override;

private:
    void SyncWithEngine();
    void RenderPluginList();
    void RenderPluginDetails();
    void RenderPluginSettings();

    const char* GetStateName(PluginState state);
    ImVec4 GetStateColor(PluginState state);
    const char* GetTypeName(PluginType type);
    const char* GetTypeIcon(PluginType type);

    PluginManagerSettings m_Settings;

    std::vector<PluginInfo> m_Plugins;
    int m_SelectedPluginIndex = -1;

    float m_ListPanelWidth = 300.0f;
    char m_SearchBuffer[256] = "";
    bool m_ShowInstallDialog = false;
};

} // namespace Yalaz::UI
