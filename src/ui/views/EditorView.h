#pragma once
// =============================================================================
// YALAZ ENGINE - Editor View Base Class
// =============================================================================
// Professional AAA-style view base class with support for:
// - Dockable/tabbable/floatable windows
// - Toolbar and status bar support
// - Flexible rendering patterns (simple OnRender or Toolbar+Content+StatusBar)
// =============================================================================

#include <string>
#include <imgui.h>
#include <imgui_internal.h>

class VulkanEngine;

namespace Yalaz::UI {

// =============================================================================
// View Category - For menu organization
// =============================================================================
enum class ViewCategory {
    Core,       // Scene, Game, Hierarchy, Inspector
    Assets,     // Asset Browser
    Debug,      // Console, Profiler, GPU Debug, Shader Debug
    Graphics,   // Material, Texture, UV, Lighting Debug
    Animation,  // Animation, Camera Sequencer
    System,     // Settings, Plugin Manager
    Scene,      // Alias for Core (backward compat)
    Layout,     // Layout views
    Rendering   // Rendering views
};

// =============================================================================
// View Flags - Capabilities and behavior
// =============================================================================
enum class ViewFlags : uint32_t {
    None           = 0,
    CanDock        = 1 << 0,   // Can be docked
    CanTab         = 1 << 1,   // Can be tabbed with other views
    CanFloat       = 1 << 2,   // Can float as separate window
    HasToolbar     = 1 << 3,   // Has a toolbar at the top
    HasStatusBar   = 1 << 4,   // Has a status bar at the bottom
    Singleton      = 1 << 5,   // Only one instance allowed
    NoClose        = 1 << 6,   // Cannot be closed
    NoMove         = 1 << 7,   // Cannot be moved
    NoResize       = 1 << 8,   // Cannot be resized

    // Common combinations
    Default = CanDock | CanTab | CanFloat,
    ToolWindow = CanDock | CanTab | CanFloat | HasToolbar,
    FullWindow = CanDock | CanTab | CanFloat | HasToolbar | HasStatusBar
};

inline ViewFlags operator|(ViewFlags a, ViewFlags b) {
    return static_cast<ViewFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline ViewFlags operator&(ViewFlags a, ViewFlags b) {
    return static_cast<ViewFlags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline bool HasFlag(ViewFlags flags, ViewFlags flag) {
    return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(flag)) != 0;
}

// =============================================================================
// Editor View Base Class
// =============================================================================
class EditorView {
public:
    // Constructor with icon and category (original pattern)
    EditorView(const std::string& name, const std::string& icon, ViewCategory category)
        : m_Name(name), m_Icon(icon), m_Category(category) {}

    // Constructor with just icon (for views that override GetCategory)
    EditorView(const std::string& name, const std::string& icon)
        : m_Name(name), m_Icon(icon), m_Category(ViewCategory::Core) {}

    virtual ~EditorView() = default;

    // ==========================================================================
    // Lifecycle
    // ==========================================================================
    virtual void OnInit(VulkanEngine* engine) { m_Engine = engine; }
    virtual void OnShutdown() {}
    virtual void OnUpdate(float /*deltaTime*/) {}

    // Simple render pattern - override this for basic views
    virtual void OnRender() { RenderAdvanced(); }

    // ==========================================================================
    // Accessors / Virtual Getters (can be overridden)
    // ==========================================================================
    const std::string& GetName() const { return m_Name; }
    virtual const char* GetDisplayName() const { return m_Name.c_str(); }
    const std::string& GetIcon() const { return m_Icon; }
    virtual ViewCategory GetCategory() const { return m_Category; }
    virtual ViewFlags GetFlags() const { return ViewFlags::Default; }

    bool IsOpen() const { return m_IsOpen; }
    void SetOpen(bool open) { m_IsOpen = open; }
    void ToggleOpen() { m_IsOpen = !m_IsOpen; }

    // Window state
    ImVec2 GetLastPos() const { return m_LastPos; }
    ImVec2 GetLastSize() const { return m_LastSize; }
    bool IsFocused() const { return m_IsFocused; }
    bool IsHovered() const { return m_IsHovered; }

    // Layout control - dynamic positions updated every frame
    void SetDynamicLayout(ImVec2 pos, ImVec2 size) {
        m_DynamicPos = pos;
        m_DynamicSize = size;
        m_UseDynamicLayout = true;
    }

    void ClearDynamicLayout() {
        m_UseDynamicLayout = false;
    }

    bool UsesDynamicLayout() const { return m_UseDynamicLayout; }

    // Initial layout - applied once on first render
    void SetInitialLayout(ImVec2 pos, ImVec2 size) {
        m_InitialPos = pos;
        m_InitialSize = size;
        m_HasInitialLayout = true;
    }
    bool HasInitialLayout() const { return m_HasInitialLayout; }

    void SetFixedLayout(ImVec2 pos, ImVec2 size) {
        m_FixedPos = pos;
        m_FixedSize = size;
        m_IsFixedLayout = true;
    }
    bool IsFixedLayout() const { return m_IsFixedLayout; }

    void ResetLayout() {
        m_LayoutApplied = false;
        m_IsFixedLayout = false;
        m_UseDynamicLayout = false;
    }

protected:
    std::string m_Name;
    std::string m_Icon;
    ViewCategory m_Category;
    bool m_IsOpen = false;  // Views closed by default, ViewManager opens core views
    bool m_IsFocused = false;
    bool m_IsHovered = false;
    VulkanEngine* m_Engine = nullptr;

    ImVec2 m_LastPos = {0, 0};
    ImVec2 m_LastSize = {0, 0};

    // Dynamic layout - recalculated and applied every frame
    ImVec2 m_DynamicPos = {0, 0};
    ImVec2 m_DynamicSize = {400, 300};
    bool m_UseDynamicLayout = false;

    // Initial layout - applied once on first render
    ImVec2 m_InitialPos = {100, 100};
    ImVec2 m_InitialSize = {400, 300};
    bool m_HasInitialLayout = false;
    bool m_LayoutApplied = false;

    // Fixed layout (legacy)
    ImVec2 m_FixedPos = {0, 0};
    ImVec2 m_FixedSize = {400, 300};
    bool m_IsFixedLayout = false;

    // ==========================================================================
    // Advanced Rendering Pattern (Toolbar + Content + StatusBar)
    // Override these for views with toolbar/statusbar
    // ==========================================================================
    virtual void OnRenderToolbar() {}
    virtual void OnRenderContent() {}
    virtual void OnRenderStatusBar() {}

    // Called by default OnRender if not overridden
    void RenderAdvanced() {
        ViewFlags flags = GetFlags();
        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_None;

        if (HasFlag(flags, ViewFlags::HasToolbar) || HasFlag(flags, ViewFlags::HasStatusBar)) {
            windowFlags |= ImGuiWindowFlags_MenuBar;
        }
        if (HasFlag(flags, ViewFlags::NoMove)) {
            windowFlags |= ImGuiWindowFlags_NoMove;
        }
        if (HasFlag(flags, ViewFlags::NoResize)) {
            windowFlags |= ImGuiWindowFlags_NoResize;
        }

        if (!BeginView(windowFlags)) {
            EndView();
            return;
        }

        // Toolbar
        if (HasFlag(flags, ViewFlags::HasToolbar)) {
            if (ImGui::BeginMenuBar()) {
                OnRenderToolbar();
                ImGui::EndMenuBar();
            }
        }

        // Content
        OnRenderContent();

        // Status bar (rendered at bottom)
        if (HasFlag(flags, ViewFlags::HasStatusBar)) {
            ImGui::Separator();
            OnRenderStatusBar();
        }

        EndView();
    }

    // ==========================================================================
    // Helper: Begin/End view window with professional styling
    // ==========================================================================
    bool BeginView(ImGuiWindowFlags extraFlags = 0) {
        if (!m_IsOpen) return false;

        // Dynamic layout - applied EVERY frame (recalculates with window resize)
        if (m_UseDynamicLayout) {
            ImGui::SetNextWindowPos(m_DynamicPos, ImGuiCond_Always);
            ImGui::SetNextWindowSize(m_DynamicSize, ImGuiCond_Always);
        }
        // Initial layout - applied once on first render
        else if (m_HasInitialLayout && !m_LayoutApplied) {
            ImGui::SetNextWindowPos(m_InitialPos, ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(m_InitialSize, ImGuiCond_FirstUseEver);
            m_LayoutApplied = true;
        }

        std::string title = m_Icon + "  " + m_Name;
        bool result = ImGui::Begin(title.c_str(), &m_IsOpen, extraFlags);
        m_LastPos = ImGui::GetWindowPos();
        m_LastSize = ImGui::GetWindowSize();
        m_IsFocused = ImGui::IsWindowFocused();
        m_IsHovered = ImGui::IsWindowHovered();
        return result;
    }

    void EndView() {
        m_LastPos = ImGui::GetWindowPos();
        m_LastSize = ImGui::GetWindowSize();
        ImGui::End();
    }

    // ==========================================================================
    // UI Helpers
    // ==========================================================================

    // Section header with professional styling
    void SectionHeader(const char* label) {
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", label);
        ImGui::Separator();
        ImGui::Spacing();
    }

    // Subsection header
    void SubSection(const char* label) {
        ImGui::TextDisabled("%s", label);
    }

    // Property row with label and formatted value
    void PropertyRow(const char* label, const char* format, ...) {
        ImGui::Text("%s:", label);
        ImGui::SameLine(150);
        va_list args;
        va_start(args, format);
        ImGui::TextV(format, args);
        va_end(args);
    }

    // Colored status indicator
    void StatusIndicator(const char* label, bool active,
                         const char* activeText = "Active",
                         const char* inactiveText = "Inactive") {
        ImGui::Text("%s:", label);
        ImGui::SameLine(150);
        if (active) {
            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "%s", activeText);
        } else {
            ImGui::TextDisabled("%s", inactiveText);
        }
    }

    // Collapsing section with colored header
    bool CollapsingSection(const char* label, bool defaultOpen = true) {
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.3f, 0.4f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.3f, 0.4f, 0.5f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.35f, 0.45f, 0.55f, 1.0f));

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_CollapsingHeader;
        if (defaultOpen) flags |= ImGuiTreeNodeFlags_DefaultOpen;

        bool open = ImGui::CollapsingHeader(label, flags);
        ImGui::PopStyleColor(3);

        return open;
    }

    // Labeled value with consistent formatting
    void LabeledValue(const char* label, const char* value, float labelWidth = 120.0f) {
        ImGui::Text("%s", label);
        ImGui::SameLine(labelWidth);
        ImGui::TextDisabled("%s", value);
    }

    // Progress bar with label
    void LabeledProgress(const char* label, float fraction, const char* overlay = nullptr, float labelWidth = 120.0f) {
        ImGui::Text("%s", label);
        ImGui::SameLine(labelWidth);
        ImGui::ProgressBar(fraction, ImVec2(-1, 0), overlay);
    }
};

// =============================================================================
// Category Helpers
// =============================================================================
inline const char* GetCategoryName(ViewCategory cat) {
    switch (cat) {
        case ViewCategory::Core: return "Core";
        case ViewCategory::Assets: return "Assets";
        case ViewCategory::Debug: return "Debug";
        case ViewCategory::Graphics: return "Graphics";
        case ViewCategory::Animation: return "Animation";
        case ViewCategory::System: return "System";
        case ViewCategory::Scene: return "Scene";
        case ViewCategory::Layout: return "Layout";
        case ViewCategory::Rendering: return "Rendering";
        default: return "Other";
    }
}

// =============================================================================
// ViewState - For layout serialization and restoration
// =============================================================================
struct ViewState {
    std::string viewId;
    bool isOpen = true;
    ImVec2 position = {0, 0};
    ImVec2 size = {400, 300};

    ViewState() = default;
    ViewState(const std::string& id);
};

// =============================================================================
// View Utility Functions (implemented in EditorView.cpp)
// =============================================================================

// Drawing utilities
void DrawSeparator(float thickness = 1.0f, ImU32 color = IM_COL32(60, 60, 60, 255));
void DrawSectionHeader(const char* label, bool withSeparator = true);
bool DrawToolbarButton(const char* label, const char* tooltip = nullptr, bool active = false);
void DrawToolbarSeparator();
void DrawPropertyRow(const char* label, const char* value, float labelWidth = 120.0f);
void DrawProgressBarColored(float fraction, const ImVec2& size, ImU32 color, const char* overlay = nullptr);
void DrawStatsValue(const char* label, float value, const char* format = "%.2f", float labelWidth = 120.0f);
void DrawStatsValue(const char* label, int value, float labelWidth = 120.0f);

// Collapsing headers
bool CollapsingHeaderStyled(const char* label, bool defaultOpen = true);

// Property table helpers
bool BeginPropertyTable(const char* id, float labelColumnWidth = 120.0f);
void EndPropertyTable();
bool PropertyRowFloat(const char* label, float* value, float speed = 0.1f, float min = 0.0f, float max = 0.0f);
bool PropertyRowFloat3(const char* label, float* values, float speed = 0.1f);
bool PropertyRowColor3(const char* label, float* values);
bool PropertyRowColor4(const char* label, float* values);
bool PropertyRowBool(const char* label, bool* value);
bool PropertyRowString(const char* label, char* buffer, size_t bufferSize);
bool PropertyRowDropdown(const char* label, int* current, const char* const* items, int count);
void PropertyRowReadOnly(const char* label, const char* value);

// Debug/profiling utilities
void PlotFrameTimeHistory(const char* label, const float* values, int count, float scaleMin = 0.0f, float scaleMax = 33.33f, const ImVec2& size = ImVec2(-1, 60));
void PlotHistogramStyled(const char* label, const float* values, int count, ImU32 color, const ImVec2& size = ImVec2(-1, 60));
bool BeginStatsTable(const char* id, int columns = 2);
void StatsTableRow(const char* label, const char* value);
void StatsTableRow(const char* label, float value, const char* fmt = "%.2f");
void StatsTableRow(const char* label, int value);
void EndStatsTable();

// Background drawing
void DrawCheckerboard(ImDrawList* drawList, ImVec2 pos, ImVec2 size, int gridSize = 16, ImU32 color1 = IM_COL32(60, 60, 60, 255), ImU32 color2 = IM_COL32(40, 40, 40, 255));
void DrawGridBackground(ImDrawList* drawList, ImVec2 pos, ImVec2 size, float gridStep = 50.0f, ImU32 majorColor = IM_COL32(80, 80, 80, 255), ImU32 minorColor = IM_COL32(50, 50, 50, 255));

// =============================================================================
// View Color Constants
// =============================================================================
namespace ViewColors {
    extern const ImVec4 Header;
    extern const ImVec4 HeaderHovered;
    extern const ImVec4 HeaderActive;
    extern const ImVec4 Accent;
    extern const ImVec4 AccentOrange;
    extern const ImVec4 Success;
    extern const ImVec4 Warning;
    extern const ImVec4 Error;
    extern const ImVec4 TextSecondary;
}

} // namespace Yalaz::UI
