// =============================================================================
// YALAZ ENGINE - Editor UI Implementation
// =============================================================================
// Professional editor UI with dynamic layout that resizes with window
// =============================================================================

#include "EditorUI.h"
#include "../vk_engine.h"
#include "../scene/SceneManager.h"
#include <fstream>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cmath>
#include <fmt/core.h>

using json = nlohmann::json;

namespace Yalaz::UI {

// =============================================================================
// LAYOUT CONFIGURATION
// =============================================================================
struct LayoutConfig {
    float leftPanelWidth = 250.0f;
    float rightPanelWidth = 320.0f;
    float bottomPanelHeight = 200.0f;
    float menuBarHeight = 25.0f;
    float toolbarHeight = 35.0f;
};

static LayoutConfig s_Layout;

// =============================================================================
// INITIALIZATION & SHUTDOWN
// =============================================================================

void EditorUI::Init(VulkanEngine* engine) {
    m_Engine = engine;

    // Apply professional theme
    EditorTheme::Get().Apply();

    // Register all view types
    RegisterAllViewTypes();

    // Initialize view manager
    ViewManager::Get().Init(m_Engine);

    // Initialize scene manager with engine pointer for save/load
    Yalaz::Scene::SceneManager::Get().SetEngine(m_Engine);

    // Initialize workspace system
    InitBuiltInWorkspaces();
    LoadWorkspacesFromFile();

    fmt::print("[Editor] View System initialized with {} views\n",
               ViewManager::Get().GetViews().size());
}

void EditorUI::Shutdown() {
    SaveWorkspacesToFile();
    ViewManager::Get().Shutdown();
}

void EditorUI::Update(float deltaTime) {
    ViewManager::Get().Update(deltaTime);
}

// =============================================================================
// LAYOUT CALCULATION - Dynamic based on viewport size
// =============================================================================

void EditorUI::CalculateLayout() {
    // If layout is unlocked, let users move panels freely
    if (!m_LayoutLocked) {
        // Clear dynamic layout on all views so they can be moved
        auto& views = ViewManager::Get().GetViews();
        for (auto& view : views) {
            view->ClearDynamicLayout();
        }
        return;
    }

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    float vw = viewport->WorkSize.x;
    float vh = viewport->WorkSize.y;

    // Use minimum viable size if viewport not ready yet
    if (vw < 100) vw = 1280;
    if (vh < 100) vh = 720;

    float menuH = s_Layout.menuBarHeight;
    const bool relayoutAll = m_ForceLayoutRecalc;

    auto& views = ViewManager::Get().GetViews();

    // === STANDARD LAYOUT - Core views are dynamically resized, secondary views are only placed on open ===
    float leftW = std::max(200.0f, vw * 0.18f);   // ~18% of width, min 200
    float rightW = std::max(280.0f, vw * 0.22f);  // ~22% of width, min 280
    float bottomH = std::max(150.0f, vh * 0.25f); // ~25% of height, min 150
    float centerW = vw - leftW - rightW;
    float topH = vh - menuH - bottomH;

    int cascadeIdx = 0;
    const float cascadeStep = 30.0f;

    for (auto& view : views) {
        const std::string& name = view->GetName();
        const bool isOpen = view->IsOpen();
        const bool wasOpen = m_PreviousOpenStates[name];
        const bool justOpened = isOpen && !wasOpen;

        if (!isOpen) {
            view->ClearDynamicLayout();
            m_PreviousOpenStates[name] = false;
            continue;
        }

        // Core editor panes stay responsive to viewport resize.
        if (name == "Hierarchy") {
            view->SetDynamicLayout(ImVec2(0, menuH), ImVec2(leftW, topH));
        } else if (name == "Inspector") {
            view->SetDynamicLayout(ImVec2(vw - rightW, menuH), ImVec2(rightW, topH));
        } else if (name == "Console") {
            view->SetDynamicLayout(ImVec2(0, vh - bottomH), ImVec2(vw * 0.5f, bottomH));
        } else if (name == "Asset Browser") {
            view->SetDynamicLayout(ImVec2(vw * 0.5f, vh - bottomH), ImVec2(vw * 0.5f, bottomH));
        } else if (name == "Scene") {
            // Scene toolbar is handled in SceneView::OnRender.
            view->ClearDynamicLayout();
        } else {
            // Secondary/tool windows: place once when opened, then let user resize/move.
            float cascadeX = leftW + 10 + (cascadeIdx * cascadeStep);
            float cascadeY = menuH + 10 + (cascadeIdx * cascadeStep);
            float maxCascadeX = vw - rightW - 350;
            float maxCascadeY = vh - bottomH - 250;

            if (cascadeX > maxCascadeX) cascadeX = leftW + 10 + ((cascadeIdx % 3) * cascadeStep);
            if (cascadeY > maxCascadeY) cascadeY = menuH + 10 + ((cascadeIdx % 4) * cascadeStep);

            float viewW = std::min(450.0f, centerW * 0.7f);
            float viewH = std::min(350.0f, topH * 0.8f);

            if (justOpened || relayoutAll) {
                view->SetDynamicLayout(ImVec2(cascadeX, cascadeY), ImVec2(viewW, viewH));
            } else {
                view->ClearDynamicLayout();
            }
            cascadeIdx++;
        }

        m_PreviousOpenStates[name] = true;
    }

    m_LastViewportWidth = vw;
    m_LastViewportHeight = vh;
    m_ForceLayoutRecalc = false;
}

// =============================================================================
// MAIN RENDER
// =============================================================================

void EditorUI::Render() {
    // Calculate dynamic layout based on current viewport
    CalculateLayout();

    // Render menu bar
    RenderMenuBar();

    // Render all views
    ViewManager::Get().Render();

    // Render workspace management window
    if (m_ShowWorkspaceWindow) {
        RenderWorkspaceWindow();
    }

    // Save workspace popup
    if (m_ShowSaveWorkspacePopup) {
        ImGui::OpenPopup("Save Workspace");
        m_ShowSaveWorkspacePopup = false;
    }

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("Save Workspace", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Save the current workspace configuration");
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Text("Name:");
        ImGui::SetNextItemWidth(300);
        ImGui::InputText("##WorkspaceName", m_NewWorkspaceName, sizeof(m_NewWorkspaceName));

        ImGui::Spacing();
        ImGui::Text("Description:");
        ImGui::SetNextItemWidth(300);
        ImGui::InputTextMultiline("##WorkspaceDesc", m_NewWorkspaceDescription,
                                  sizeof(m_NewWorkspaceDescription), ImVec2(0, 60));

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        bool canSave = strlen(m_NewWorkspaceName) > 0;

        if (!canSave) ImGui::BeginDisabled();

        if (ImGui::Button("Save", ImVec2(120, 0))) {
            SaveWorkspace(m_NewWorkspaceName, m_NewWorkspaceDescription);
            ImGui::CloseCurrentPopup();
        }

        if (!canSave) ImGui::EndDisabled();

        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

// Keep SetupDockspace for compatibility but it does nothing now
void EditorUI::SetupDockspace() {}

// =============================================================================
// MENU BAR
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
            ImGui::TextDisabled("Scene Files:");

            // Save Scene submenu
            if (ImGui::BeginMenu("Save Scene...")) {
                static char saveNameBuffer[128] = "my_scene";

                ImGui::Text("Save Name:");
                ImGui::SetNextItemWidth(200);
                ImGui::InputText("##SaveName", saveNameBuffer, sizeof(saveNameBuffer));

                if (ImGui::Button("Save", ImVec2(100, 0))) {
                    std::string savePath = "saves/" + std::string(saveNameBuffer) + ".yscene";
                    Yalaz::Scene::SceneManager::Get().SaveScene(savePath);
                    ImGui::CloseCurrentPopup();
                }

                ImGui::Separator();
                ImGui::TextDisabled("Recent:");

                const auto& recentScenes = Yalaz::Scene::SceneManager::Get().GetRecentScenes();
                for (const auto& recent : recentScenes) {
                    std::filesystem::path p(recent);
                    if (ImGui::MenuItem(p.stem().string().c_str())) {
                        Yalaz::Scene::SceneManager::Get().SaveScene(recent);
                    }
                }

                ImGui::EndMenu();
            }

            // Load Scene submenu
            if (ImGui::BeginMenu("Load Scene...")) {
                ImGui::TextDisabled("Saved Scenes:");

                // List files in saves folder
                std::filesystem::path savesDir("saves");
                if (std::filesystem::exists(savesDir)) {
                    for (const auto& entry : std::filesystem::directory_iterator(savesDir)) {
                        if (entry.path().extension() == ".yscene") {
                            std::string name = entry.path().stem().string();
                            if (ImGui::MenuItem(name.c_str())) {
                                Yalaz::Scene::SceneManager::Get().LoadScene(entry.path().string());
                            }
                        }
                    }
                } else {
                    ImGui::TextDisabled("No saves folder found");
                }

                ImGui::Separator();
                ImGui::TextDisabled("Recent:");

                const auto& recentScenes = Yalaz::Scene::SceneManager::Get().GetRecentScenes();
                for (const auto& recent : recentScenes) {
                    std::filesystem::path p(recent);
                    if (ImGui::MenuItem(p.stem().string().c_str())) {
                        Yalaz::Scene::SceneManager::Get().LoadScene(recent);
                    }
                }

                ImGui::EndMenu();
            }

            // Delete Scene submenu
            if (ImGui::BeginMenu("Delete Scene...")) {
                ImGui::TextDisabled("Saved Scenes:");
                bool anyScene = false;

                std::filesystem::path savesDir("saves");
                if (std::filesystem::exists(savesDir)) {
                    for (const auto& entry : std::filesystem::directory_iterator(savesDir)) {
                        if (entry.path().extension() == ".yscene") {
                            anyScene = true;
                            std::string name = entry.path().stem().string();
                            std::string label = "Delete " + name;
                            if (ImGui::MenuItem(label.c_str())) {
                                Yalaz::Scene::SceneManager::Get().DeleteSceneFile(entry.path().string());
                            }
                        }
                    }
                }

                if (!anyScene) {
                    ImGui::TextDisabled("No saved scenes found");
                }

                ImGui::Separator();
                ImGui::TextDisabled("Recent:");
                const auto& recentScenes = Yalaz::Scene::SceneManager::Get().GetRecentScenes();
                if (recentScenes.empty()) {
                    ImGui::TextDisabled("No recent scenes");
                } else {
                    for (const auto& recent : recentScenes) {
                        std::filesystem::path p(recent);
                        std::string label = "Delete " + p.stem().string();
                        if (ImGui::MenuItem(label.c_str())) {
                            Yalaz::Scene::SceneManager::Get().DeleteSceneFile(recent);
                        }
                    }
                }

                ImGui::EndMenu();
            }

            ImGui::Separator();
            if (ImGui::MenuItem("Quick Save", "Ctrl+S")) {
                Yalaz::Scene::SceneManager::Get().SaveScene("saves/quicksave.yscene");
            }
            if (ImGui::MenuItem("Quick Load", "Ctrl+O")) {
                Yalaz::Scene::SceneManager::Get().LoadScene("saves/quicksave.yscene");
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
                m_Engine->mainCamera.updateProjectionMatrix();
            }
            ImGui::EndMenu();
        }

        // === VIEW MENU ===
        if (ImGui::BeginMenu("View")) {
            RenderViewMenu();
            ImGui::EndMenu();
        }

        // === WORKSPACE MENU ===
        if (ImGui::BeginMenu("Workspace")) {
            RenderWorkspaceMenu();
            ImGui::EndMenu();
        }

        // === RIGHT-ALIGNED INFO ===
        float rightOffset = ImGui::GetWindowWidth() - 280.0f;
        ImGui::SetCursorPosX(rightOffset);

        // Current workspace indicator
        ImGui::TextDisabled("[%s]", m_CurrentWorkspaceName.c_str());
        ImGui::SameLine();
        ImGui::Text("FPS: %.0f | %.2f ms", 1000.0f / m_Engine->stats.frametime, m_Engine->stats.frametime);

        ImGui::EndMainMenuBar();
    }

    // Keyboard shortcuts
    ImGuiIO& io = ImGui::GetIO();
    if (!io.WantTextInput) {
        if (io.KeyCtrl) {
            if (ImGui::IsKeyPressed(ImGuiKey_S, false)) {
                Yalaz::Scene::SceneManager::Get().SaveScene("saves/quicksave.yscene");
            }
            if (ImGui::IsKeyPressed(ImGuiKey_O, false)) {
                Yalaz::Scene::SceneManager::Get().LoadScene("saves/quicksave.yscene");
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
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Home, false)) {
            m_Engine->mainCamera.position = glm::vec3(0.f, 5.f, 10.f);
            m_Engine->mainCamera.pitch = -0.3f;
            m_Engine->mainCamera.yaw = 0.f;
            m_Engine->mainCamera.updateProjectionMatrix();
        }
    }
}

// =============================================================================
// VIEW MENU
// =============================================================================

void EditorUI::RenderViewMenu() {
    ViewManager::Get().RenderViewMenu();
}

// =============================================================================
// WORKSPACE MENU
// =============================================================================

void EditorUI::RenderWorkspaceMenu() {
    // Built-in and custom workspaces
    ImGui::TextDisabled("Workspaces");
    ImGui::Separator();

    for (const auto& workspace : m_Workspaces) {
        bool isActive = (workspace.name == m_CurrentWorkspaceName);
        std::string label = workspace.icon + "  " + workspace.name;

        if (workspace.isBuiltIn) {
            label += " (Built-in)";
        }

        if (ImGui::MenuItem(label.c_str(), nullptr, isActive)) {
            LoadWorkspace(workspace.name);
        }

        if (ImGui::IsItemHovered() && !workspace.description.empty()) {
            ImGui::SetTooltip("%s", workspace.description.c_str());
        }
    }

    ImGui::Separator();

    if (ImGui::MenuItem("Save Current Workspace...")) {
        m_ShowSaveWorkspacePopup = true;
        memset(m_NewWorkspaceName, 0, sizeof(m_NewWorkspaceName));
        memset(m_NewWorkspaceDescription, 0, sizeof(m_NewWorkspaceDescription));
    }

    if (ImGui::MenuItem("Manage Workspaces...")) {
        m_ShowWorkspaceWindow = true;
    }
}

// =============================================================================
// WORKSPACE WINDOW
// =============================================================================

void EditorUI::RenderWorkspaceWindow() {
    ImGui::SetNextWindowSize(ImVec2(500, 450), ImGuiCond_FirstUseEver);
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));

    if (ImGui::Begin("Workspace Manager", &m_ShowWorkspaceWindow, ImGuiWindowFlags_NoCollapse)) {
        // Header
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Current Workspace:");
        ImGui::SameLine();
        ImGui::Text("%s", m_CurrentWorkspaceName.c_str());

        ImGui::Separator();
        ImGui::Spacing();

        // Workspace list
        ImGui::BeginChild("WorkspaceList", ImVec2(0, -60), true);

        for (size_t i = 0; i < m_Workspaces.size(); ++i) {
            const auto& workspace = m_Workspaces[i];
            bool isSelected = (m_SelectedWorkspaceIndex == static_cast<int>(i));
            bool isActive = (workspace.name == m_CurrentWorkspaceName);

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
                ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "%s", workspace.icon.c_str());
                ImGui::SameLine();

                ImGui::Text("%s", workspace.name.c_str());
                ImGui::SameLine();
                ImGui::TextDisabled(workspace.isBuiltIn ? "(Built-in)" : "(Custom)");

                // Description
                if (!workspace.description.empty()) {
                    ImGui::TextDisabled("%s", workspace.description.c_str());
                }

                // View count
                ImGui::TextDisabled("%zu views configured", workspace.viewStates.size());

                // Click handling
                if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0)) {
                    m_SelectedWorkspaceIndex = static_cast<int>(i);
                }
                if (ImGui::IsWindowHovered() && ImGui::IsMouseDoubleClicked(0)) {
                    LoadWorkspace(workspace.name);
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

        bool hasSelection = m_SelectedWorkspaceIndex >= 0 &&
                           m_SelectedWorkspaceIndex < static_cast<int>(m_Workspaces.size());
        bool canDelete = hasSelection && !m_Workspaces[m_SelectedWorkspaceIndex].isBuiltIn;

        if (ImGui::Button("Apply", ImVec2(80, 0))) {
            if (hasSelection) {
                LoadWorkspace(m_Workspaces[m_SelectedWorkspaceIndex].name);
            }
        }
        ImGui::SameLine();

        if (!canDelete) ImGui::BeginDisabled();
        if (ImGui::Button("Delete", ImVec2(80, 0))) {
            if (canDelete) {
                DeleteWorkspace(m_Workspaces[m_SelectedWorkspaceIndex].name);
                m_SelectedWorkspaceIndex = -1;
            }
        }
        if (!canDelete) ImGui::EndDisabled();

        ImGui::SameLine();
        if (ImGui::Button("Save Current...", ImVec2(120, 0))) {
            m_ShowSaveWorkspacePopup = true;
            memset(m_NewWorkspaceName, 0, sizeof(m_NewWorkspaceName));
            memset(m_NewWorkspaceDescription, 0, sizeof(m_NewWorkspaceDescription));
        }

        ImGui::SameLine();
        float closeButtonX = ImGui::GetWindowWidth() - 90;
        ImGui::SetCursorPosX(closeButtonX);
        if (ImGui::Button("Close", ImVec2(80, 0))) {
            m_ShowWorkspaceWindow = false;
        }
    }
    ImGui::End();
}

// =============================================================================
// WORKSPACE MANAGEMENT - BUILT-IN WORKSPACES
// =============================================================================

void EditorUI::InitBuiltInWorkspaces() {
    m_Workspaces.clear();

    // Default Workspace - Core editor views
    WorkspaceLayout defaultWs;
    defaultWs.name = "Default";
    defaultWs.icon = "|||";
    defaultWs.description = "Standard editor layout with core views";
    defaultWs.isBuiltIn = true;
    defaultWs.viewStates = {
        {"Scene", true, {0, 0}, {0, 0}},
        {"Hierarchy", true, {0, 0}, {0, 0}},
        {"Inspector", true, {0, 0}, {0, 0}},
        {"Console", true, {0, 0}, {0, 0}},
        {"Asset Browser", true, {0, 0}, {0, 0}}
    };
    m_Workspaces.push_back(defaultWs);

    // Modeling Workspace - Focus on 3D editing
    WorkspaceLayout modelingWs;
    modelingWs.name = "Modeling";
    modelingWs.icon = "[M]";
    modelingWs.description = "3D modeling with hierarchy, inspector, UV editor";
    modelingWs.isBuiltIn = true;
    modelingWs.viewStates = {
        {"Scene", true, {0, 0}, {0, 0}},
        {"Hierarchy", true, {0, 0}, {0, 0}},
        {"Inspector", true, {0, 0}, {0, 0}},
        {"UV Editor", true, {0, 0}, {0, 0}}
    };
    m_Workspaces.push_back(modelingWs);

    // Debug Workspace - Performance analysis
    WorkspaceLayout debugWs;
    debugWs.name = "Debug";
    debugWs.icon = "[D]";
    debugWs.description = "Debugging and profiling tools";
    debugWs.isBuiltIn = true;
    debugWs.viewStates = {
        {"Scene", true, {0, 0}, {0, 0}},
        {"Console", true, {0, 0}, {0, 0}},
        {"Profiler", true, {0, 0}, {0, 0}},
        {"GPU Debug", true, {0, 0}, {0, 0}}
    };
    m_Workspaces.push_back(debugWs);

    // Graphics Workspace - Material and texture editing
    WorkspaceLayout graphicsWs;
    graphicsWs.name = "Graphics";
    graphicsWs.icon = "[G]";
    graphicsWs.description = "Material and texture editing tools";
    graphicsWs.isBuiltIn = true;
    graphicsWs.viewStates = {
        {"Scene", true, {0, 0}, {0, 0}},
        {"Hierarchy", true, {0, 0}, {0, 0}},
        {"Inspector", true, {0, 0}, {0, 0}},
        {"Material", true, {0, 0}, {0, 0}},
        {"Texture", true, {0, 0}, {0, 0}}
    };
    m_Workspaces.push_back(graphicsWs);

    // Minimal Workspace - Just scene view
    WorkspaceLayout minimalWs;
    minimalWs.name = "Minimal";
    minimalWs.icon = "[_]";
    minimalWs.description = "Scene view only - maximum viewport space";
    minimalWs.isBuiltIn = true;
    minimalWs.viewStates = {
        {"Scene", true, {0, 0}, {0, 0}}
    };
    m_Workspaces.push_back(minimalWs);
}

// =============================================================================
// WORKSPACE FILE I/O
// =============================================================================

void EditorUI::LoadWorkspacesFromFile() {
    try {
        std::ifstream file("workspaces.json");
        if (!file.is_open()) return;

        json j;
        file >> j;

        for (const auto& item : j["workspaces"]) {
            WorkspaceLayout ws;
            ws.name = item.value("name", "Unnamed");
            ws.icon = item.value("icon", "[?]");
            ws.description = item.value("description", "");
            ws.isBuiltIn = false;

            if (item.contains("views")) {
                for (const auto& view : item["views"]) {
                    ViewLayoutState state;
                    state.viewName = view.value("name", "");
                    state.isOpen = view.value("open", true);
                    state.position.x = view.value("x", 0.0f);
                    state.position.y = view.value("y", 0.0f);
                    state.size.x = view.value("width", 400.0f);
                    state.size.y = view.value("height", 300.0f);
                    ws.viewStates.push_back(state);
                }
            }

            m_Workspaces.push_back(ws);
        }

        // Load last used workspace
        if (j.contains("currentWorkspace")) {
            std::string lastWorkspace = j["currentWorkspace"];
            for (const auto& ws : m_Workspaces) {
                if (ws.name == lastWorkspace) {
                    LoadWorkspace(lastWorkspace);
                    break;
                }
            }
        }
    } catch (...) {
        // Silently fail - use built-in workspaces
    }
}

void EditorUI::SaveWorkspacesToFile() {
    try {
        json j;
        j["currentWorkspace"] = m_CurrentWorkspaceName;

        json workspaces = json::array();
        for (const auto& ws : m_Workspaces) {
            if (ws.isBuiltIn) continue;

            json workspace;
            workspace["name"] = ws.name;
            workspace["icon"] = ws.icon;
            workspace["description"] = ws.description;

            json views = json::array();
            for (const auto& state : ws.viewStates) {
                json view;
                view["name"] = state.viewName;
                view["open"] = state.isOpen;
                view["x"] = state.position.x;
                view["y"] = state.position.y;
                view["width"] = state.size.x;
                view["height"] = state.size.y;
                views.push_back(view);
            }
            workspace["views"] = views;

            workspaces.push_back(workspace);
        }
        j["workspaces"] = workspaces;

        std::ofstream file("workspaces.json");
        file << j.dump(2);
    } catch (...) {
        // Silently fail
    }
}

// =============================================================================
// WORKSPACE OPERATIONS
// =============================================================================

void EditorUI::SaveWorkspace(const std::string& name, const std::string& description) {
    WorkspaceLayout ws = CaptureCurrentWorkspace(name);
    ws.description = description;
    ws.icon = "[+]";

    // Check if workspace exists
    for (auto& existing : m_Workspaces) {
        if (existing.name == name && !existing.isBuiltIn) {
            existing = ws;
            SaveWorkspacesToFile();
            m_CurrentWorkspaceName = name;
            return;
        }
    }

    // Add new workspace
    m_Workspaces.push_back(ws);
    m_CurrentWorkspaceName = name;
    SaveWorkspacesToFile();
}

void EditorUI::LoadWorkspace(const std::string& name) {
    for (const auto& ws : m_Workspaces) {
        if (ws.name == name) {
            ApplyWorkspace(ws);
            m_CurrentWorkspaceName = name;
            return;
        }
    }
}

void EditorUI::DeleteWorkspace(const std::string& name) {
    auto it = std::remove_if(m_Workspaces.begin(), m_Workspaces.end(),
        [&name](const WorkspaceLayout& ws) { return ws.name == name && !ws.isBuiltIn; });
    m_Workspaces.erase(it, m_Workspaces.end());
    SaveWorkspacesToFile();
}

void EditorUI::ApplyWorkspace(const WorkspaceLayout& layout) {
    auto& views = ViewManager::Get().GetViews();

    // First, close all views and clear layouts
    for (auto& view : views) {
        view->SetOpen(false);
        view->ClearDynamicLayout();
        view->ResetLayout();
    }

    // Open views specified in workspace
    for (const auto& state : layout.viewStates) {
        for (auto& view : views) {
            if (view->GetName() == state.viewName) {
                view->SetOpen(state.isOpen);
                break;
            }
        }
    }

    // Enable locked layout so CalculateLayout positions views without overlap
    m_LayoutLocked = true;
    m_ForceLayoutRecalc = true;
}

WorkspaceLayout EditorUI::CaptureCurrentWorkspace(const std::string& name) {
    WorkspaceLayout ws;
    ws.name = name;
    ws.isBuiltIn = false;

    const auto& views = ViewManager::Get().GetViews();
    for (const auto& view : views) {
        ViewLayoutState state;
        state.viewName = view->GetName();
        state.isOpen = view->IsOpen();
        state.position = view->GetLastPos();
        state.size = view->GetLastSize();
        ws.viewStates.push_back(state);
    }

    return ws;
}

// =============================================================================
// VIEW SHORTCUTS
// =============================================================================

void EditorUI::ShowView(const std::string& viewName) {
    ViewManager::Get().ShowView(viewName);
}

void EditorUI::HideView(const std::string& viewName) {
    ViewManager::Get().HideView(viewName);
}

void EditorUI::ToggleView(const std::string& viewName) {
    ViewManager::Get().ToggleView(viewName);
}

} // namespace Yalaz::UI
