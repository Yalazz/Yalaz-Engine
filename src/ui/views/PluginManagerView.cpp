// =============================================================================
// YALAZ ENGINE - Plugin Manager View Implementation
// =============================================================================

#include "PluginManagerView.h"
#include "../../vk_engine.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <algorithm>

namespace Yalaz::UI {

PluginManagerView::PluginManagerView()
    : EditorView("Plugin Manager", "[K]", ViewCategory::System) {
}

ViewFlags PluginManagerView::GetFlags() const {
    return ViewFlags::CanDock | ViewFlags::CanTab | ViewFlags::CanFloat |
           ViewFlags::HasToolbar | ViewFlags::Singleton;
}

void PluginManagerView::OnInit(VulkanEngine* engine) {
    EditorView::OnInit(engine);
    SyncWithEngine();
}

void PluginManagerView::SyncWithEngine() {
    if (!m_Engine) return;

    m_Plugins.clear();

    // Sync with engine subsystems
    for (const auto& subsystem : m_Engine->subsystems) {
        PluginInfo plugin;
        plugin.id = subsystem.id;
        plugin.name = subsystem.name;
        plugin.version = subsystem.version;
        plugin.author = "Yalaz Team";
        plugin.description = "Engine subsystem: " + subsystem.name;
        plugin.isBuiltIn = subsystem.isCore;
        plugin.canDisable = !subsystem.isCore;
        plugin.loadTimeMs = subsystem.loadTimeMs;
        plugin.memoryUsage = subsystem.memoryUsage;

        // Map subsystem state to plugin state
        switch (subsystem.state) {
            case SubsystemState::Active:
                plugin.state = PluginState::Enabled;
                break;
            case SubsystemState::Loaded:
                plugin.state = PluginState::Loaded;
                break;
            case SubsystemState::Loading:
                plugin.state = PluginState::Loading;
                break;
            case SubsystemState::Error:
                plugin.state = PluginState::Error;
                break;
            default:
                plugin.state = PluginState::Unloaded;
                break;
        }

        // Determine type based on name
        if (subsystem.name.find("Render") != std::string::npos ||
            subsystem.name.find("Material") != std::string::npos) {
            plugin.type = PluginType::Rendering;
        } else if (subsystem.name.find("Physics") != std::string::npos) {
            plugin.type = PluginType::Physics;
        } else if (subsystem.name.find("Audio") != std::string::npos) {
            plugin.type = PluginType::Audio;
        } else if (subsystem.name.find("UI") != std::string::npos ||
                   subsystem.name.find("Editor") != std::string::npos) {
            plugin.type = PluginType::Editor;
        } else if (subsystem.name.find("Script") != std::string::npos ||
                   subsystem.name.find("Lua") != std::string::npos) {
            plugin.type = PluginType::Script;
        } else if (subsystem.isCore) {
            plugin.type = PluginType::Core;
        } else {
            plugin.type = PluginType::ThirdParty;
        }

        m_Plugins.push_back(plugin);
    }

    // If no subsystems loaded, show default plugins
    if (m_Plugins.empty()) {
        PluginInfo vulkanRenderer;
        vulkanRenderer.id = "vulkan-renderer";
        vulkanRenderer.name = "Vulkan Renderer";
        vulkanRenderer.description = "Core Vulkan rendering backend";
        vulkanRenderer.author = "Yalaz Team";
        vulkanRenderer.version = "1.0.0";
        vulkanRenderer.type = PluginType::Core;
        vulkanRenderer.state = PluginState::Enabled;
        vulkanRenderer.isBuiltIn = true;
        vulkanRenderer.canDisable = false;
        vulkanRenderer.loadTimeMs = 125.5f;
        vulkanRenderer.memoryUsage = 256 * 1024 * 1024;
        m_Plugins.push_back(vulkanRenderer);
    }
}

void PluginManagerView::OnUpdate(float deltaTime) {
    // Check for plugin state changes
}

void PluginManagerView::RefreshPluginList() {
    SyncWithEngine();
}

void PluginManagerView::LoadPlugin(const std::string& id) {
    for (auto& plugin : m_Plugins) {
        if (plugin.id == id && plugin.state == PluginState::Unloaded) {
            plugin.state = PluginState::Loading;
            // Would actually load plugin here
            plugin.state = PluginState::Loaded;
            break;
        }
    }
}

void PluginManagerView::UnloadPlugin(const std::string& id) {
    for (auto& plugin : m_Plugins) {
        if (plugin.id == id && (plugin.state == PluginState::Loaded ||
                                 plugin.state == PluginState::Disabled)) {
            // Would actually unload plugin here
            plugin.state = PluginState::Unloaded;
            break;
        }
    }
}

void PluginManagerView::EnablePlugin(const std::string& id) {
    for (auto& plugin : m_Plugins) {
        if (plugin.id == id && (plugin.state == PluginState::Loaded ||
                                 plugin.state == PluginState::Disabled)) {
            plugin.state = PluginState::Enabled;
            break;
        }
    }
}

void PluginManagerView::DisablePlugin(const std::string& id) {
    for (auto& plugin : m_Plugins) {
        if (plugin.id == id && plugin.state == PluginState::Enabled && plugin.canDisable) {
            plugin.state = PluginState::Disabled;
            break;
        }
    }
}

void PluginManagerView::OnRenderToolbar() {
    // Search
    ImGui::SetNextItemWidth(200);
    if (ImGui::InputTextWithHint("##Search", "Search plugins...", m_SearchBuffer, sizeof(m_SearchBuffer))) {
        m_Settings.searchFilter = m_SearchBuffer;
    }

    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine();

    // Type filter
    ImGui::Checkbox("Filter Type", &m_Settings.filterByType);

    if (m_Settings.filterByType) {
        ImGui::SameLine();
        const char* types[] = {"Core", "Rendering", "Editor", "Physics", "Audio", "Network", "Script", "Third-Party"};
        int type = static_cast<int>(m_Settings.typeFilter);
        ImGui::SetNextItemWidth(100);
        if (ImGui::Combo("##TypeFilter", &type, types, 8)) {
            m_Settings.typeFilter = static_cast<PluginType>(type);
        }
    }

    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine();

    // View options
    ImGui::Checkbox("Built-in", &m_Settings.showBuiltIn);
    ImGui::SameLine();
    ImGui::Checkbox("Disabled", &m_Settings.showDisabled);
    ImGui::SameLine();
    ImGui::Checkbox("Errors", &m_Settings.showErrored);

    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine();

    if (ImGui::Button("Refresh")) {
        RefreshPluginList();
    }

    ImGui::SameLine();
    if (ImGui::Button("Install...")) {
        m_ShowInstallDialog = true;
    }
}

void PluginManagerView::OnRenderContent() {
    // Plugin list
    ImGui::BeginChild("PluginList", ImVec2(m_ListPanelWidth, 0), true);
    RenderPluginList();
    ImGui::EndChild();

    ImGui::SameLine();

    // Splitter
    ImGui::Button("##Splitter", ImVec2(4, -1));
    if (ImGui::IsItemActive()) {
        m_ListPanelWidth += ImGui::GetIO().MouseDelta.x;
        m_ListPanelWidth = std::clamp(m_ListPanelWidth, 200.0f, 500.0f);
    }

    ImGui::SameLine();

    // Plugin details
    ImGui::BeginChild("PluginDetails", ImVec2(0, 0), true);
    RenderPluginDetails();
    ImGui::EndChild();

    // Install dialog
    if (m_ShowInstallDialog) {
        ImGui::OpenPopup("Install Plugin");
        m_ShowInstallDialog = false;
    }

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("Install Plugin", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Install a plugin from file or URL");
        ImGui::Separator();
        ImGui::Spacing();

        static char pathBuffer[512] = "";
        ImGui::Text("Path/URL:");
        ImGui::SetNextItemWidth(400);
        ImGui::InputText("##Path", pathBuffer, sizeof(pathBuffer));

        ImGui::Spacing();

        if (ImGui::Button("Browse...")) {
            // Would open file dialog
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Install", ImVec2(100, 0))) {
            // Would install plugin
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(100, 0))) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void PluginManagerView::RenderPluginList() {
    ImGui::Text("Plugins (%zu)", m_Plugins.size());
    ImGui::Separator();

    for (size_t i = 0; i < m_Plugins.size(); ++i) {
        const auto& plugin = m_Plugins[i];

        // Apply filters
        if (!m_Settings.showBuiltIn && plugin.isBuiltIn) continue;
        if (!m_Settings.showDisabled && plugin.state == PluginState::Disabled) continue;
        if (!m_Settings.showErrored && plugin.state == PluginState::Error) continue;

        if (!m_Settings.searchFilter.empty()) {
            if (plugin.name.find(m_Settings.searchFilter) == std::string::npos &&
                plugin.id.find(m_Settings.searchFilter) == std::string::npos) {
                continue;
            }
        }

        if (m_Settings.filterByType && plugin.type != m_Settings.typeFilter) {
            continue;
        }

        ImGui::PushID(static_cast<int>(i));

        // State indicator
        ImGui::TextColored(GetStateColor(plugin.state), "%s", GetStateName(plugin.state));
        ImGui::SameLine();

        // Plugin name
        bool selected = (m_SelectedPluginIndex == static_cast<int>(i));
        if (ImGui::Selectable(plugin.name.c_str(), selected)) {
            m_SelectedPluginIndex = static_cast<int>(i);
        }

        // Tooltip
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("%s", plugin.name.c_str());
            ImGui::TextDisabled("v%s by %s", plugin.version.c_str(), plugin.author.c_str());
            ImGui::TextDisabled("%s", GetTypeName(plugin.type));
            ImGui::EndTooltip();
        }

        ImGui::PopID();
    }
}

void PluginManagerView::RenderPluginDetails() {
    if (m_SelectedPluginIndex < 0 || m_SelectedPluginIndex >= static_cast<int>(m_Plugins.size())) {
        ImVec2 avail = ImGui::GetContentRegionAvail();
        ImVec2 textSize = ImGui::CalcTextSize("Select a plugin");
        ImGui::SetCursorPos(ImVec2((avail.x - textSize.x) * 0.5f, avail.y * 0.5f));
        ImGui::TextDisabled("Select a plugin from the list");
        return;
    }

    auto& plugin = m_Plugins[m_SelectedPluginIndex];

    // Header
    ImGui::Text("%s %s", GetTypeIcon(plugin.type), plugin.name.c_str());

    if (plugin.isBuiltIn) {
        ImGui::SameLine();
        ImGui::TextDisabled("(Built-in)");
    }

    ImGui::TextDisabled("ID: %s", plugin.id.c_str());
    ImGui::TextDisabled("Version: %s | Author: %s", plugin.version.c_str(), plugin.author.c_str());

    // State
    ImGui::Text("Status:");
    ImGui::SameLine();
    ImGui::TextColored(GetStateColor(plugin.state), "%s", GetStateName(plugin.state));

    if (plugin.state == PluginState::Error && !plugin.errorMessage.empty()) {
        ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "Error: %s", plugin.errorMessage.c_str());
    }

    ImGui::Separator();
    ImGui::Spacing();

    // Description
    ImGui::TextWrapped("%s", plugin.description.c_str());

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Action buttons
    switch (plugin.state) {
        case PluginState::Unloaded:
            if (ImGui::Button("Load", ImVec2(100, 0))) {
                LoadPlugin(plugin.id);
            }
            break;

        case PluginState::Loaded:
            if (ImGui::Button("Enable", ImVec2(100, 0))) {
                EnablePlugin(plugin.id);
            }
            ImGui::SameLine();
            if (ImGui::Button("Unload", ImVec2(100, 0))) {
                UnloadPlugin(plugin.id);
            }
            break;

        case PluginState::Enabled:
            if (plugin.canDisable) {
                if (ImGui::Button("Disable", ImVec2(100, 0))) {
                    DisablePlugin(plugin.id);
                }
            } else {
                ImGui::BeginDisabled();
                ImGui::Button("Disable", ImVec2(100, 0));
                ImGui::EndDisabled();
                ImGui::SameLine();
                ImGui::TextDisabled("(Required)");
            }
            break;

        case PluginState::Disabled:
            if (ImGui::Button("Enable", ImVec2(100, 0))) {
                EnablePlugin(plugin.id);
            }
            ImGui::SameLine();
            if (ImGui::Button("Unload", ImVec2(100, 0))) {
                UnloadPlugin(plugin.id);
            }
            break;

        case PluginState::Error:
            if (ImGui::Button("Retry Load", ImVec2(100, 0))) {
                plugin.state = PluginState::Unloaded;
                LoadPlugin(plugin.id);
            }
            break;

        default:
            break;
    }

    if (plugin.hasSettings && (plugin.state == PluginState::Enabled ||
                                plugin.state == PluginState::Loaded)) {
        ImGui::SameLine();
        if (ImGui::Button("Settings...", ImVec2(100, 0))) {
            // Would open settings dialog
        }
    }

    // Dependencies
    if (!plugin.dependencies.empty()) {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("Dependencies");

        for (const auto& dep : plugin.dependencies) {
            ImVec4 color = dep.satisfied ? ImVec4(0.3f, 1, 0.3f, 1) : ImVec4(1, 0.3f, 0.3f, 1);
            const char* status = dep.satisfied ? "OK" : "Missing";

            ImGui::TextColored(color, "[%s]", status);
            ImGui::SameLine();
            ImGui::Text("%s >= %s", dep.name.c_str(), dep.minVersion.c_str());

            if (!dep.required) {
                ImGui::SameLine();
                ImGui::TextDisabled("(optional)");
            }
        }
    }

    // Statistics
    if (plugin.state == PluginState::Enabled || plugin.state == PluginState::Loaded) {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("Statistics");

        ImGui::TextDisabled("Load Time: %.1f ms", plugin.loadTimeMs);

        if (plugin.memoryUsage > 1024 * 1024) {
            ImGui::TextDisabled("Memory: %.1f MB", plugin.memoryUsage / (1024.0f * 1024.0f));
        } else if (plugin.memoryUsage > 1024) {
            ImGui::TextDisabled("Memory: %.1f KB", plugin.memoryUsage / 1024.0f);
        } else {
            ImGui::TextDisabled("Memory: %zu B", plugin.memoryUsage);
        }
    }
}

void PluginManagerView::RenderPluginSettings() {
    // Would render plugin-specific settings
}

const char* PluginManagerView::GetStateName(PluginState state) {
    switch (state) {
        case PluginState::Unloaded: return "[-]";
        case PluginState::Loading: return "[~]";
        case PluginState::Loaded: return "[L]";
        case PluginState::Enabled: return "[+]";
        case PluginState::Disabled: return "[x]";
        case PluginState::Error: return "[!]";
        default: return "[?]";
    }
}

ImVec4 PluginManagerView::GetStateColor(PluginState state) {
    switch (state) {
        case PluginState::Unloaded: return ImVec4(0.5f, 0.5f, 0.5f, 1);
        case PluginState::Loading: return ImVec4(1, 0.8f, 0.2f, 1);
        case PluginState::Loaded: return ImVec4(0.4f, 0.8f, 1, 1);
        case PluginState::Enabled: return ImVec4(0.3f, 1, 0.3f, 1);
        case PluginState::Disabled: return ImVec4(0.8f, 0.8f, 0.2f, 1);
        case PluginState::Error: return ImVec4(1, 0.3f, 0.3f, 1);
        default: return ImVec4(1, 1, 1, 1);
    }
}

const char* PluginManagerView::GetTypeName(PluginType type) {
    switch (type) {
        case PluginType::Core: return "Core";
        case PluginType::Rendering: return "Rendering";
        case PluginType::Editor: return "Editor";
        case PluginType::Physics: return "Physics";
        case PluginType::Audio: return "Audio";
        case PluginType::Networking: return "Networking";
        case PluginType::Script: return "Scripting";
        case PluginType::ThirdParty: return "Third-Party";
        default: return "Unknown";
    }
}

const char* PluginManagerView::GetTypeIcon(PluginType type) {
    switch (type) {
        case PluginType::Core: return "[C]";
        case PluginType::Rendering: return "[R]";
        case PluginType::Editor: return "[E]";
        case PluginType::Physics: return "[P]";
        case PluginType::Audio: return "[A]";
        case PluginType::Networking: return "[N]";
        case PluginType::Script: return "[S]";
        case PluginType::ThirdParty: return "[3]";
        default: return "[?]";
    }
}

} // namespace Yalaz::UI
