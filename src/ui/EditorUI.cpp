// =============================================================================
// YALAZ ENGINE - Editor UI Implementation (Optimized)
// =============================================================================
// Includes layout preset system with save/load functionality
// =============================================================================

#include "EditorUI.h"
#include "panels/SceneHierarchyPanel.h"
#include "panels/InspectorPanel.h"
#include "panels/ViewportPanel.h"
#include "panels/LightingPanel.h"
#include "panels/ConsolePanel.h"
#include "../vk_engine.h"
#include <fstream>
#include <nlohmann/json.hpp>
#include <algorithm>

using json = nlohmann::json;

namespace Yalaz::UI {

// =============================================================================
// INITIALIZATION & SHUTDOWN
// =============================================================================

void EditorUI::Init(VulkanEngine* engine) {
    m_Engine = engine;

    // Apply professional theme
    EditorTheme::Get().Apply();

    // Register all panels and cache pointers
    m_SceneHierarchy = PanelManager::Get().AddPanel<SceneHierarchyPanel>();
    m_Inspector = PanelManager::Get().AddPanel<InspectorPanel>();
    m_Viewport = PanelManager::Get().AddPanel<ViewportPanel>();
    m_Lighting = PanelManager::Get().AddPanel<LightingPanel>();
    m_Console = PanelManager::Get().AddPanel<ConsolePanel>();

    // Initialize panels
    PanelManager::Get().Init(engine);

    // Initialize preset system
    InitBuiltInPresets();
    LoadPresetsFromFile();
}

void EditorUI::Shutdown() {
    SavePresetsToFile();
    PanelManager::Get().Shutdown();
}

// =============================================================================
// PRESET SYSTEM - BUILT-IN PRESETS
// =============================================================================

void EditorUI::InitBuiltInPresets() {
    m_Presets.clear();

    // Default Layout
    LayoutPreset defaultPreset;
    defaultPreset.name = "Default";
    defaultPreset.icon = "|||";
    defaultPreset.description = "Standard editor layout with all panels visible";
    defaultPreset.isBuiltIn = true;
    defaultPreset.leftPanelWidth = 280.0f;
    defaultPreset.rightPanelWidth = 320.0f;
    defaultPreset.bottomPanelHeight = 200.0f;
    defaultPreset.inspectorHeightRatio = 0.5f;
    defaultPreset.viewportHeightRatio = 0.25f;
    defaultPreset.lightingHeightRatio = 0.25f;
    m_Presets.push_back(defaultPreset);

    // Wide Viewport
    LayoutPreset widePreset;
    widePreset.name = "Wide Viewport";
    widePreset.icon = "[=]";
    widePreset.description = "Maximized 3D viewport with minimal panels";
    widePreset.isBuiltIn = true;
    widePreset.leftPanelWidth = 220.0f;
    widePreset.rightPanelWidth = 260.0f;
    widePreset.bottomPanelHeight = 150.0f;
    widePreset.inspectorHeightRatio = 0.6f;
    widePreset.viewportHeightRatio = 0.2f;
    widePreset.lightingHeightRatio = 0.2f;
    m_Presets.push_back(widePreset);

    // Compact
    LayoutPreset compactPreset;
    compactPreset.name = "Compact";
    compactPreset.icon = "[ ]";
    compactPreset.description = "Smaller panels for high-DPI displays";
    compactPreset.isBuiltIn = true;
    compactPreset.leftPanelWidth = 240.0f;
    compactPreset.rightPanelWidth = 280.0f;
    compactPreset.bottomPanelHeight = 160.0f;
    compactPreset.inspectorHeightRatio = 0.45f;
    compactPreset.viewportHeightRatio = 0.275f;
    compactPreset.lightingHeightRatio = 0.275f;
    m_Presets.push_back(compactPreset);

    // Focus Mode
    LayoutPreset focusPreset;
    focusPreset.name = "Focus Mode";
    focusPreset.icon = "<>";
    focusPreset.description = "Scene hierarchy and inspector only";
    focusPreset.isBuiltIn = true;
    focusPreset.leftPanelWidth = 300.0f;
    focusPreset.rightPanelWidth = 350.0f;
    focusPreset.bottomPanelHeight = 0.0f;
    focusPreset.sceneHierarchyOpen = true;
    focusPreset.inspectorOpen = true;
    focusPreset.viewportOpen = false;
    focusPreset.lightingOpen = false;
    focusPreset.consoleOpen = false;
    focusPreset.inspectorHeightRatio = 1.0f;
    focusPreset.viewportHeightRatio = 0.0f;
    focusPreset.lightingHeightRatio = 0.0f;
    m_Presets.push_back(focusPreset);

    // Lighting Artist
    LayoutPreset lightingPreset;
    lightingPreset.name = "Lighting Artist";
    lightingPreset.icon = "*";
    lightingPreset.description = "Expanded lighting panel for light setup";
    lightingPreset.isBuiltIn = true;
    lightingPreset.leftPanelWidth = 260.0f;
    lightingPreset.rightPanelWidth = 380.0f;
    lightingPreset.bottomPanelHeight = 180.0f;
    lightingPreset.inspectorHeightRatio = 0.35f;
    lightingPreset.viewportHeightRatio = 0.25f;
    lightingPreset.lightingHeightRatio = 0.4f;
    m_Presets.push_back(lightingPreset);

    // Debug Mode
    LayoutPreset debugPreset;
    debugPreset.name = "Debug Mode";
    debugPreset.icon = ">#";
    debugPreset.description = "Large console for debugging output";
    debugPreset.isBuiltIn = true;
    debugPreset.leftPanelWidth = 280.0f;
    debugPreset.rightPanelWidth = 300.0f;
    debugPreset.bottomPanelHeight = 300.0f;
    debugPreset.inspectorHeightRatio = 0.5f;
    debugPreset.viewportHeightRatio = 0.25f;
    debugPreset.lightingHeightRatio = 0.25f;
    m_Presets.push_back(debugPreset);
}

// =============================================================================
// PRESET SYSTEM - FILE I/O
// =============================================================================

void EditorUI::LoadPresetsFromFile() {
    try {
        std::ifstream file("layout_presets.json");
        if (!file.is_open()) return;

        json j;
        file >> j;

        for (const auto& item : j["presets"]) {
            LayoutPreset preset;
            preset.name = item.value("name", "Unnamed");
            preset.icon = item.value("icon", "?");
            preset.description = item.value("description", "");
            preset.isBuiltIn = false;  // Loaded presets are never built-in
            preset.leftPanelWidth = item.value("leftPanelWidth", Layout::LeftPanelWidth);
            preset.rightPanelWidth = item.value("rightPanelWidth", Layout::RightPanelWidth);
            preset.bottomPanelHeight = item.value("bottomPanelHeight", Layout::BottomPanelHeight);
            preset.sceneHierarchyOpen = item.value("sceneHierarchyOpen", true);
            preset.inspectorOpen = item.value("inspectorOpen", true);
            preset.viewportOpen = item.value("viewportOpen", true);
            preset.lightingOpen = item.value("lightingOpen", true);
            preset.consoleOpen = item.value("consoleOpen", true);
            preset.inspectorHeightRatio = item.value("inspectorHeightRatio", 0.5f);
            preset.viewportHeightRatio = item.value("viewportHeightRatio", 0.25f);
            preset.lightingHeightRatio = item.value("lightingHeightRatio", 0.25f);

            m_Presets.push_back(preset);
        }

        // Load last used preset
        if (j.contains("currentPreset")) {
            std::string lastPreset = j["currentPreset"];
            for (const auto& p : m_Presets) {
                if (p.name == lastPreset) {
                    LoadPreset(lastPreset);
                    break;
                }
            }
        }
    } catch (...) {
        // Silently fail - just use built-in presets
    }
}

void EditorUI::SavePresetsToFile() {
    try {
        json j;
        j["currentPreset"] = m_CurrentPresetName;

        json presets = json::array();
        for (const auto& p : m_Presets) {
            if (p.isBuiltIn) continue;  // Don't save built-in presets

            json preset;
            preset["name"] = p.name;
            preset["icon"] = p.icon;
            preset["description"] = p.description;
            preset["leftPanelWidth"] = p.leftPanelWidth;
            preset["rightPanelWidth"] = p.rightPanelWidth;
            preset["bottomPanelHeight"] = p.bottomPanelHeight;
            preset["sceneHierarchyOpen"] = p.sceneHierarchyOpen;
            preset["inspectorOpen"] = p.inspectorOpen;
            preset["viewportOpen"] = p.viewportOpen;
            preset["lightingOpen"] = p.lightingOpen;
            preset["consoleOpen"] = p.consoleOpen;
            preset["inspectorHeightRatio"] = p.inspectorHeightRatio;
            preset["viewportHeightRatio"] = p.viewportHeightRatio;
            preset["lightingHeightRatio"] = p.lightingHeightRatio;

            presets.push_back(preset);
        }
        j["presets"] = presets;

        std::ofstream file("layout_presets.json");
        file << j.dump(2);
    } catch (...) {
        // Silently fail
    }
}

// =============================================================================
// PRESET SYSTEM - MANAGEMENT
// =============================================================================

void EditorUI::SavePreset(const std::string& name) {
    LayoutPreset preset = CaptureCurrentLayout(name);

    // Check if preset exists
    for (auto& p : m_Presets) {
        if (p.name == name && !p.isBuiltIn) {
            p = preset;
            SavePresetsToFile();
            return;
        }
    }

    // Add new preset
    m_Presets.push_back(preset);
    m_CurrentPresetName = name;
    SavePresetsToFile();
}

void EditorUI::LoadPreset(const std::string& name) {
    for (const auto& p : m_Presets) {
        if (p.name == name) {
            ApplyPreset(p);
            m_CurrentPresetName = name;
            return;
        }
    }
}

void EditorUI::DeletePreset(const std::string& name) {
    auto it = std::remove_if(m_Presets.begin(), m_Presets.end(),
        [&name](const LayoutPreset& p) { return p.name == name && !p.isBuiltIn; });
    m_Presets.erase(it, m_Presets.end());
    SavePresetsToFile();
}

void EditorUI::ApplyPreset(const LayoutPreset& preset) {
    m_LeftPanelWidth = preset.leftPanelWidth;
    m_RightPanelWidth = preset.rightPanelWidth;
    m_BottomPanelHeight = preset.bottomPanelHeight;
    m_InspectorHeightRatio = preset.inspectorHeightRatio;
    m_ViewportHeightRatio = preset.viewportHeightRatio;
    m_LightingHeightRatio = preset.lightingHeightRatio;

    if (m_SceneHierarchy) m_SceneHierarchy->SetOpen(preset.sceneHierarchyOpen);
    if (m_Inspector) m_Inspector->SetOpen(preset.inspectorOpen);
    if (m_Viewport) m_Viewport->SetOpen(preset.viewportOpen);
    if (m_Lighting) m_Lighting->SetOpen(preset.lightingOpen);
    if (m_Console) m_Console->SetOpen(preset.consoleOpen);

    m_LayoutDirty = true;
}

LayoutPreset EditorUI::CaptureCurrentLayout(const std::string& name) {
    LayoutPreset preset;
    preset.name = name;
    preset.icon = "+";
    preset.description = "Custom layout";
    preset.isBuiltIn = false;
    preset.leftPanelWidth = m_LeftPanelWidth;
    preset.rightPanelWidth = m_RightPanelWidth;
    preset.bottomPanelHeight = m_BottomPanelHeight;
    preset.sceneHierarchyOpen = m_SceneHierarchy ? m_SceneHierarchy->IsOpen() : true;
    preset.inspectorOpen = m_Inspector ? m_Inspector->IsOpen() : true;
    preset.viewportOpen = m_Viewport ? m_Viewport->IsOpen() : true;
    preset.lightingOpen = m_Lighting ? m_Lighting->IsOpen() : true;
    preset.consoleOpen = m_Console ? m_Console->IsOpen() : true;
    preset.inspectorHeightRatio = m_InspectorHeightRatio;
    preset.viewportHeightRatio = m_ViewportHeightRatio;
    preset.lightingHeightRatio = m_LightingHeightRatio;

    return preset;
}

// =============================================================================
// RENDER - MAIN
// =============================================================================

void EditorUI::Render() {
    // Setup layout only when viewport changes
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    if (viewport->Size.x != m_LastViewportSize.x || viewport->Size.y != m_LastViewportSize.y) {
        m_LastViewportSize = viewport->Size;
        m_LayoutDirty = true;
    }

    if (m_LayoutDirty) {
        CalculateLayout();
    }

    // Render menu bar
    RenderMenuBar();

    // Apply cached positions and render panels directly
    RenderPanels();

    // Render preset management window
    if (m_ShowPresetWindow) {
        RenderLayoutPresetWindow();
    }

    // Clear dirty flag after panels are rendered
    m_LayoutDirty = false;
}

// =============================================================================
// RENDER - MENU BAR
// =============================================================================

void EditorUI::RenderMenuBar() {
    if (!m_Engine) return;

    if (ImGui::BeginMainMenuBar()) {
        // === FILE MENU ===
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Scene", "Ctrl+N")) {
                m_Engine->resetState();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Save Scene", "Ctrl+S")) {
                m_Engine->saveState("scene.json");
            }
            if (ImGui::MenuItem("Load Scene", "Ctrl+O")) {
                m_Engine->loadState("scene.json");
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "Alt+F4")) {
                // Will be handled by SDL
            }
            ImGui::EndMenu();
        }

        // === EDIT MENU ===
        if (ImGui::BeginMenu("Edit")) {
            bool hasSelection = (m_Engine->selectedNode != nullptr) ||
                               (m_Engine->selectedPrimitiveIndex >= 0 &&
                                m_Engine->selectedPrimitiveIndex < static_cast<int>(m_Engine->static_shapes.size()));

            if (ImGui::MenuItem("Delete Selected", "Delete", false, hasSelection)) {
                if (m_Engine->selectedPrimitiveIndex >= 0 &&
                    m_Engine->selectedPrimitiveIndex < static_cast<int>(m_Engine->static_shapes.size())) {
                    m_Engine->static_shapes.erase(m_Engine->static_shapes.begin() + m_Engine->selectedPrimitiveIndex);
                    m_Engine->selectedPrimitiveIndex = -1;
                }
                if (m_Engine->selectedNode != nullptr) {
                    m_Engine->selectedNode = nullptr;
                    m_Engine->selectedObjectName.clear();
                }
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Reset Camera", "Home")) {
                m_Engine->mainCamera.position = glm::vec3(0.f, 5.f, 10.f);
                m_Engine->mainCamera.pitch = -0.3f;
                m_Engine->mainCamera.yaw = 0.f;
                m_Engine->mainCamera.targetPosition = m_Engine->mainCamera.position;
                m_Engine->mainCamera.targetPitch = m_Engine->mainCamera.pitch;
                m_Engine->mainCamera.targetYaw = m_Engine->mainCamera.yaw;
                m_Engine->mainCamera.focusActive = false;
                m_Engine->mainCamera.updateProjectionMatrix();
            }
            ImGui::EndMenu();
        }

        // === VIEW MENU ===
        if (ImGui::BeginMenu("View")) {
            // Panel toggles
            ImGui::TextDisabled("Panels");
            if (m_SceneHierarchy) {
                bool open = m_SceneHierarchy->IsOpen();
                if (ImGui::MenuItem("Scene Hierarchy", nullptr, &open)) {
                    m_SceneHierarchy->SetOpen(open);
                }
            }
            if (m_Inspector) {
                bool open = m_Inspector->IsOpen();
                if (ImGui::MenuItem("Inspector", nullptr, &open)) {
                    m_Inspector->SetOpen(open);
                }
            }
            if (m_Viewport) {
                bool open = m_Viewport->IsOpen();
                if (ImGui::MenuItem("Viewport Settings", nullptr, &open)) {
                    m_Viewport->SetOpen(open);
                }
            }
            if (m_Lighting) {
                bool open = m_Lighting->IsOpen();
                if (ImGui::MenuItem("Lighting", nullptr, &open)) {
                    m_Lighting->SetOpen(open);
                }
            }
            if (m_Console) {
                bool open = m_Console->IsOpen();
                if (ImGui::MenuItem("Console", nullptr, &open)) {
                    m_Console->SetOpen(open);
                }
            }

            ImGui::Separator();
            ImGui::TextDisabled("Layout Presets");

            // Quick preset buttons
            for (const auto& preset : m_Presets) {
                bool isActive = (preset.name == m_CurrentPresetName);
                std::string label = preset.icon + "  " + preset.name;
                if (ImGui::MenuItem(label.c_str(), nullptr, isActive)) {
                    LoadPreset(preset.name);
                }
            }

            ImGui::Separator();
            if (ImGui::MenuItem("Manage Presets...")) {
                m_ShowPresetWindow = true;
            }
            if (ImGui::MenuItem("Save Current Layout...")) {
                m_ShowSavePresetPopup = true;
                memset(m_NewPresetName, 0, sizeof(m_NewPresetName));
                memset(m_NewPresetDescription, 0, sizeof(m_NewPresetDescription));
            }

            ImGui::EndMenu();
        }

        // === WINDOW MENU (Layout adjustments) ===
        if (ImGui::BeginMenu("Window")) {
            ImGui::TextDisabled("Panel Sizes");

            ImGui::SetNextItemWidth(150);
            if (ImGui::SliderFloat("Left Width", &m_LeftPanelWidth, 180.0f, 400.0f, "%.0f")) {
                m_LayoutDirty = true;
            }
            ImGui::SetNextItemWidth(150);
            if (ImGui::SliderFloat("Right Width", &m_RightPanelWidth, 200.0f, 500.0f, "%.0f")) {
                m_LayoutDirty = true;
            }
            ImGui::SetNextItemWidth(150);
            if (ImGui::SliderFloat("Bottom Height", &m_BottomPanelHeight, 100.0f, 400.0f, "%.0f")) {
                m_LayoutDirty = true;
            }

            ImGui::Separator();
            ImGui::TextDisabled("Right Panel Ratios");

            ImGui::SetNextItemWidth(150);
            if (ImGui::SliderFloat("Inspector", &m_InspectorHeightRatio, 0.2f, 0.8f, "%.2f")) {
                // Normalize ratios
                float remaining = 1.0f - m_InspectorHeightRatio;
                float ratio = m_ViewportHeightRatio / (m_ViewportHeightRatio + m_LightingHeightRatio);
                m_ViewportHeightRatio = remaining * ratio;
                m_LightingHeightRatio = remaining * (1.0f - ratio);
                m_LayoutDirty = true;
            }

            ImGui::EndMenu();
        }

        // === RIGHT-ALIGNED INFO ===
        float rightOffset = ImGui::GetWindowWidth() - 280.0f;
        ImGui::SetCursorPosX(rightOffset);

        // Current preset indicator
        ImGui::TextDisabled("[%s]", m_CurrentPresetName.c_str());
        ImGui::SameLine();
        ImGui::Text("FPS: %.0f | %.2f ms", 1000.0f / m_Engine->stats.frametime, m_Engine->stats.frametime);

        ImGui::EndMainMenuBar();
    }

    // Save preset popup
    if (m_ShowSavePresetPopup) {
        ImGui::OpenPopup("Save Layout Preset");
        m_ShowSavePresetPopup = false;
    }

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("Save Layout Preset", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Save the current layout as a preset");
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Text("Name:");
        ImGui::SetNextItemWidth(300);
        ImGui::InputText("##PresetName", m_NewPresetName, sizeof(m_NewPresetName));

        ImGui::Spacing();
        ImGui::Text("Description:");
        ImGui::SetNextItemWidth(300);
        ImGui::InputTextMultiline("##PresetDesc", m_NewPresetDescription, sizeof(m_NewPresetDescription),
            ImVec2(0, 60));

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        bool canSave = strlen(m_NewPresetName) > 0;

        if (!canSave) {
            ImGui::BeginDisabled();
        }

        if (ImGui::Button("Save", ImVec2(120, 0))) {
            LayoutPreset preset = CaptureCurrentLayout(m_NewPresetName);
            preset.description = m_NewPresetDescription;
            m_Presets.push_back(preset);
            m_CurrentPresetName = preset.name;
            SavePresetsToFile();
            ImGui::CloseCurrentPopup();
        }

        if (!canSave) {
            ImGui::EndDisabled();
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    // Keyboard shortcuts
    ImGuiIO& io = ImGui::GetIO();
    if (!io.WantTextInput) {
        if (io.KeyCtrl) {
            if (ImGui::IsKeyPressed(ImGuiKey_S, false)) {
                m_Engine->saveState("scene.json");
            }
            if (ImGui::IsKeyPressed(ImGuiKey_O, false)) {
                m_Engine->loadState("scene.json");
            }
            if (ImGui::IsKeyPressed(ImGuiKey_N, false)) {
                m_Engine->resetState();
            }
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Delete, false)) {
            if (m_Engine->selectedPrimitiveIndex >= 0 &&
                m_Engine->selectedPrimitiveIndex < static_cast<int>(m_Engine->static_shapes.size())) {
                m_Engine->static_shapes.erase(m_Engine->static_shapes.begin() + m_Engine->selectedPrimitiveIndex);
                m_Engine->selectedPrimitiveIndex = -1;
            }
            if (m_Engine->selectedNode != nullptr) {
                m_Engine->selectedNode = nullptr;
                m_Engine->selectedObjectName.clear();
            }
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Home, false)) {
            m_Engine->mainCamera.position = glm::vec3(0.f, 5.f, 10.f);
            m_Engine->mainCamera.pitch = -0.3f;
            m_Engine->mainCamera.yaw = 0.f;
            m_Engine->mainCamera.targetPosition = m_Engine->mainCamera.position;
            m_Engine->mainCamera.targetPitch = m_Engine->mainCamera.pitch;
            m_Engine->mainCamera.targetYaw = m_Engine->mainCamera.yaw;
            m_Engine->mainCamera.focusActive = false;
            m_Engine->mainCamera.updateProjectionMatrix();
        }
    }
}

// =============================================================================
// RENDER - LAYOUT PRESET WINDOW
// =============================================================================

void EditorUI::RenderLayoutPresetWindow() {
    ImGui::SetNextWindowSize(ImVec2(500, 450), ImGuiCond_FirstUseEver);
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));

    if (ImGui::Begin("Layout Presets", &m_ShowPresetWindow, ImGuiWindowFlags_NoCollapse)) {
        // Header with current preset
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Current Layout:");
        ImGui::SameLine();
        ImGui::Text("%s", m_CurrentPresetName.c_str());

        ImGui::Separator();
        ImGui::Spacing();

        // Preset list with cards
        ImGui::BeginChild("PresetList", ImVec2(0, -60), true);

        for (size_t i = 0; i < m_Presets.size(); ++i) {
            const auto& preset = m_Presets[i];
            bool isSelected = (m_SelectedPresetIndex == static_cast<int>(i));
            bool isActive = (preset.name == m_CurrentPresetName);

            // Card style
            ImGui::PushID(static_cast<int>(i));

            ImVec4 cardColor = isActive ?
                ImVec4(0.2f, 0.4f, 0.6f, 1.0f) :
                ImVec4(0.15f, 0.15f, 0.18f, 1.0f);

            if (isSelected) {
                cardColor = ImVec4(0.3f, 0.3f, 0.4f, 1.0f);
            }

            ImGui::PushStyleColor(ImGuiCol_ChildBg, cardColor);
            ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);

            if (ImGui::BeginChild("Card", ImVec2(-1, 70), true,
                ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {

                // Icon and name
                ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "%s", preset.icon.c_str());
                ImGui::SameLine();

                if (preset.isBuiltIn) {
                    ImGui::Text("%s", preset.name.c_str());
                    ImGui::SameLine();
                    ImGui::TextDisabled("(Built-in)");
                } else {
                    ImGui::Text("%s", preset.name.c_str());
                    ImGui::SameLine();
                    ImGui::TextDisabled("(Custom)");
                }

                // Description
                ImGui::TextDisabled("%s", preset.description.c_str());

                // Panel info
                ImGui::TextDisabled("Left: %.0f | Right: %.0f | Bottom: %.0f",
                    preset.leftPanelWidth, preset.rightPanelWidth, preset.bottomPanelHeight);

                // Click handling
                if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0)) {
                    m_SelectedPresetIndex = static_cast<int>(i);
                }
                if (ImGui::IsWindowHovered() && ImGui::IsMouseDoubleClicked(0)) {
                    LoadPreset(preset.name);
                }
            }
            ImGui::EndChild();

            ImGui::PopStyleVar();
            ImGui::PopStyleColor();
            ImGui::PopID();

            ImGui::Spacing();
        }

        ImGui::EndChild();

        // Action buttons
        ImGui::Separator();
        ImGui::Spacing();

        bool hasSelection = m_SelectedPresetIndex >= 0 && m_SelectedPresetIndex < static_cast<int>(m_Presets.size());
        bool canDelete = hasSelection && !m_Presets[m_SelectedPresetIndex].isBuiltIn;

        if (ImGui::Button("Apply", ImVec2(80, 0))) {
            if (hasSelection) {
                LoadPreset(m_Presets[m_SelectedPresetIndex].name);
            }
        }
        ImGui::SameLine();

        if (!canDelete) ImGui::BeginDisabled();
        if (ImGui::Button("Delete", ImVec2(80, 0))) {
            if (canDelete) {
                DeletePreset(m_Presets[m_SelectedPresetIndex].name);
                m_SelectedPresetIndex = -1;
            }
        }
        if (!canDelete) ImGui::EndDisabled();

        ImGui::SameLine();
        if (ImGui::Button("Save Current...", ImVec2(120, 0))) {
            m_ShowSavePresetPopup = true;
            memset(m_NewPresetName, 0, sizeof(m_NewPresetName));
            memset(m_NewPresetDescription, 0, sizeof(m_NewPresetDescription));
        }

        ImGui::SameLine();
        float closeButtonX = ImGui::GetWindowWidth() - 90;
        ImGui::SetCursorPosX(closeButtonX);
        if (ImGui::Button("Close", ImVec2(80, 0))) {
            m_ShowPresetWindow = false;
        }
    }
    ImGui::End();
}

// =============================================================================
// RENDER - LAYOUT CALCULATION
// =============================================================================

void EditorUI::CalculateLayout() {
    float w = m_LastViewportSize.x;
    float h = m_LastViewportSize.y;
    float top = Layout::MenuBarHeight;
    float mainH = h - top - m_BottomPanelHeight;

    // Calculate right panel heights based on ratios
    float inspectorH = mainH * m_InspectorHeightRatio;
    float viewportH = mainH * m_ViewportHeightRatio;
    float lightingH = mainH * m_LightingHeightRatio;

    // Cache all positions and sizes
    m_LayoutCache[0] = {ImVec2(0, top), ImVec2(m_LeftPanelWidth, mainH)};  // SceneHierarchy
    m_LayoutCache[1] = {ImVec2(w - m_RightPanelWidth, top), ImVec2(m_RightPanelWidth, inspectorH)};  // Inspector
    m_LayoutCache[2] = {ImVec2(w - m_RightPanelWidth, top + inspectorH), ImVec2(m_RightPanelWidth, viewportH)};  // Viewport
    m_LayoutCache[3] = {ImVec2(w - m_RightPanelWidth, top + inspectorH + viewportH), ImVec2(m_RightPanelWidth, lightingH)};  // Lighting
    m_LayoutCache[4] = {ImVec2(0, h - m_BottomPanelHeight), ImVec2(w, m_BottomPanelHeight)};  // Console

    m_ViewportPos = ImVec2(m_LeftPanelWidth, top);
    m_ViewportSize = ImVec2(w - m_LeftPanelWidth - m_RightPanelWidth, mainH);
}

// =============================================================================
// RENDER - PANELS
// =============================================================================

void EditorUI::RenderPanels() {
    ImGuiCond layoutCond = m_LayoutDirty ? ImGuiCond_Always : ImGuiCond_FirstUseEver;

    if (m_SceneHierarchy && m_SceneHierarchy->IsOpen()) {
        ImGui::SetNextWindowPos(m_LayoutCache[0].pos, layoutCond);
        ImGui::SetNextWindowSize(m_LayoutCache[0].size, layoutCond);
        m_SceneHierarchy->OnRender();
    }

    if (m_Inspector && m_Inspector->IsOpen()) {
        ImGui::SetNextWindowPos(m_LayoutCache[1].pos, layoutCond);
        ImGui::SetNextWindowSize(m_LayoutCache[1].size, layoutCond);
        m_Inspector->OnRender();
    }

    if (m_Viewport && m_Viewport->IsOpen()) {
        ImGui::SetNextWindowPos(m_LayoutCache[2].pos, layoutCond);
        ImGui::SetNextWindowSize(m_LayoutCache[2].size, layoutCond);
        m_Viewport->OnRender();
    }

    if (m_Lighting && m_Lighting->IsOpen()) {
        ImGui::SetNextWindowPos(m_LayoutCache[3].pos, layoutCond);
        ImGui::SetNextWindowSize(m_LayoutCache[3].size, layoutCond);
        m_Lighting->OnRender();
    }

    if (m_Console && m_Console->IsOpen()) {
        ImGui::SetNextWindowPos(m_LayoutCache[4].pos, layoutCond);
        ImGui::SetNextWindowSize(m_LayoutCache[4].size, layoutCond);
        m_Console->OnRender();
    }
}

} // namespace Yalaz::UI
