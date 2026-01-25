#pragma once
// =============================================================================
// YALAZ ENGINE - Editor Theme System
// =============================================================================
// Professional dark theme inspired by Blender 4.0
// Provides consistent styling across all UI panels
// =============================================================================

#include <imgui.h>

namespace Yalaz::UI {

// Color palette - Blender 4.0 Dark style
namespace Colors {
    // Backgrounds
    constexpr ImVec4 WindowBg        = ImVec4(0.114f, 0.114f, 0.114f, 1.0f);  // #1D1D1D
    constexpr ImVec4 ChildBg         = ImVec4(0.141f, 0.141f, 0.141f, 1.0f);  // #242424
    constexpr ImVec4 PopupBg         = ImVec4(0.169f, 0.169f, 0.169f, 1.0f);  // #2B2B2B
    constexpr ImVec4 FrameBg         = ImVec4(0.157f, 0.157f, 0.157f, 1.0f);  // #282828
    constexpr ImVec4 FrameBgHovered  = ImVec4(0.196f, 0.196f, 0.196f, 1.0f);  // #323232
    constexpr ImVec4 FrameBgActive   = ImVec4(0.235f, 0.235f, 0.235f, 1.0f);  // #3C3C3C

    // Headers
    constexpr ImVec4 Header          = ImVec4(0.184f, 0.184f, 0.184f, 1.0f);  // #2F2F2F
    constexpr ImVec4 HeaderHovered   = ImVec4(0.235f, 0.235f, 0.235f, 1.0f);  // #3C3C3C
    constexpr ImVec4 HeaderActive    = ImVec4(0.275f, 0.275f, 0.275f, 1.0f);  // #464646

    // Buttons
    constexpr ImVec4 Button          = ImVec4(0.216f, 0.216f, 0.216f, 1.0f);  // #373737
    constexpr ImVec4 ButtonHovered   = ImVec4(0.275f, 0.275f, 0.275f, 1.0f);  // #464646
    constexpr ImVec4 ButtonActive    = ImVec4(0.314f, 0.314f, 0.314f, 1.0f);  // #505050

    // Accent Orange (Primary actions - Blender style)
    constexpr ImVec4 AccentOrange       = ImVec4(0.929f, 0.529f, 0.231f, 1.0f);  // #ED873B
    constexpr ImVec4 AccentOrangeHover  = ImVec4(1.000f, 0.588f, 0.294f, 1.0f);  // #FF964B
    constexpr ImVec4 AccentOrangeActive = ImVec4(0.784f, 0.431f, 0.176f, 1.0f);  // #C86E2D

    // Accent Blue (Selection)
    constexpr ImVec4 SelectionBlue      = ImVec4(0.259f, 0.588f, 0.980f, 1.0f);  // #4296FA
    constexpr ImVec4 SelectionBlueHover = ImVec4(0.337f, 0.667f, 1.000f, 1.0f);  // #56AAFF
    constexpr ImVec4 SelectionBlueBg    = ImVec4(0.259f, 0.588f, 0.980f, 0.3f);  // #4296FA 30%

    // Text
    constexpr ImVec4 TextPrimary     = ImVec4(0.902f, 0.902f, 0.902f, 1.0f);  // #E6E6E6
    constexpr ImVec4 TextSecondary   = ImVec4(0.627f, 0.627f, 0.627f, 1.0f);  // #A0A0A0
    constexpr ImVec4 TextDisabled    = ImVec4(0.392f, 0.392f, 0.392f, 1.0f);  // #646464

    // Tabs
    constexpr ImVec4 Tab             = ImVec4(0.141f, 0.141f, 0.141f, 1.0f);  // #242424
    constexpr ImVec4 TabHovered      = ImVec4(0.275f, 0.275f, 0.275f, 1.0f);  // #464646
    constexpr ImVec4 TabActive       = ImVec4(0.196f, 0.196f, 0.196f, 1.0f);  // #323232
    constexpr ImVec4 TabUnfocused    = ImVec4(0.118f, 0.118f, 0.118f, 1.0f);  // #1E1E1E

    // Borders & Separators
    constexpr ImVec4 Border          = ImVec4(0.196f, 0.196f, 0.196f, 1.0f);  // #323232
    constexpr ImVec4 Separator       = ImVec4(0.235f, 0.235f, 0.235f, 1.0f);  // #3C3C3C

    // Scrollbar
    constexpr ImVec4 ScrollbarBg     = ImVec4(0.114f, 0.114f, 0.114f, 1.0f);  // #1D1D1D
    constexpr ImVec4 ScrollbarGrab   = ImVec4(0.275f, 0.275f, 0.275f, 1.0f);  // #464646
    constexpr ImVec4 ScrollbarHover  = ImVec4(0.353f, 0.353f, 0.353f, 1.0f);  // #5A5A5A

    // Title bar
    constexpr ImVec4 TitleBg         = ImVec4(0.114f, 0.114f, 0.114f, 1.0f);  // #1D1D1D
    constexpr ImVec4 TitleBgActive   = ImVec4(0.157f, 0.157f, 0.157f, 1.0f);  // #282828
    constexpr ImVec4 TitleBgCollapsed= ImVec4(0.114f, 0.114f, 0.114f, 0.5f);

    // Checkmark & Slider
    constexpr ImVec4 CheckMark       = ImVec4(0.929f, 0.529f, 0.231f, 1.0f);  // #ED873B
    constexpr ImVec4 SliderGrab      = ImVec4(0.929f, 0.529f, 0.231f, 1.0f);  // #ED873B
    constexpr ImVec4 SliderGrabActive= ImVec4(1.000f, 0.588f, 0.294f, 1.0f);  // #FF964B
}

class EditorTheme {
public:
    static EditorTheme& Get() {
        static EditorTheme instance;
        return instance;
    }

    // Apply the current theme to ImGui
    void Apply() {
        ImGuiStyle& style = ImGui::GetStyle();
        ImVec4* colors = style.Colors;

        // ========== COLORS ==========

        // Main backgrounds
        colors[ImGuiCol_WindowBg]           = Colors::WindowBg;
        colors[ImGuiCol_ChildBg]            = Colors::ChildBg;
        colors[ImGuiCol_PopupBg]            = Colors::PopupBg;

        // Borders
        colors[ImGuiCol_Border]             = Colors::Border;
        colors[ImGuiCol_BorderShadow]       = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

        // Frame backgrounds
        colors[ImGuiCol_FrameBg]            = Colors::FrameBg;
        colors[ImGuiCol_FrameBgHovered]     = Colors::FrameBgHovered;
        colors[ImGuiCol_FrameBgActive]      = Colors::FrameBgActive;

        // Title bar
        colors[ImGuiCol_TitleBg]            = Colors::TitleBg;
        colors[ImGuiCol_TitleBgActive]      = Colors::TitleBgActive;
        colors[ImGuiCol_TitleBgCollapsed]   = Colors::TitleBgCollapsed;

        // Menu bar
        colors[ImGuiCol_MenuBarBg]          = Colors::WindowBg;

        // Scrollbar
        colors[ImGuiCol_ScrollbarBg]        = Colors::ScrollbarBg;
        colors[ImGuiCol_ScrollbarGrab]      = Colors::ScrollbarGrab;
        colors[ImGuiCol_ScrollbarGrabHovered] = Colors::ScrollbarHover;
        colors[ImGuiCol_ScrollbarGrabActive]  = Colors::AccentOrange;

        // Checkmark
        colors[ImGuiCol_CheckMark]          = Colors::CheckMark;

        // Slider
        colors[ImGuiCol_SliderGrab]         = Colors::SliderGrab;
        colors[ImGuiCol_SliderGrabActive]   = Colors::SliderGrabActive;

        // Buttons
        colors[ImGuiCol_Button]             = Colors::Button;
        colors[ImGuiCol_ButtonHovered]      = Colors::ButtonHovered;
        colors[ImGuiCol_ButtonActive]       = Colors::ButtonActive;

        // Headers (collapsing headers, tree nodes)
        colors[ImGuiCol_Header]             = Colors::Header;
        colors[ImGuiCol_HeaderHovered]      = Colors::HeaderHovered;
        colors[ImGuiCol_HeaderActive]       = Colors::HeaderActive;

        // Separator
        colors[ImGuiCol_Separator]          = Colors::Separator;
        colors[ImGuiCol_SeparatorHovered]   = Colors::AccentOrange;
        colors[ImGuiCol_SeparatorActive]    = Colors::AccentOrangeActive;

        // Resize grip
        colors[ImGuiCol_ResizeGrip]         = Colors::Button;
        colors[ImGuiCol_ResizeGripHovered]  = Colors::AccentOrangeHover;
        colors[ImGuiCol_ResizeGripActive]   = Colors::AccentOrange;

        // Tabs
        colors[ImGuiCol_Tab]                = Colors::Tab;
        colors[ImGuiCol_TabHovered]         = Colors::TabHovered;
        colors[ImGuiCol_TabActive]          = Colors::TabActive;
        colors[ImGuiCol_TabUnfocused]       = Colors::TabUnfocused;
        colors[ImGuiCol_TabUnfocusedActive] = Colors::Tab;

        // Plot
        colors[ImGuiCol_PlotLines]          = Colors::AccentOrange;
        colors[ImGuiCol_PlotLinesHovered]   = Colors::AccentOrangeHover;
        colors[ImGuiCol_PlotHistogram]      = Colors::AccentOrange;
        colors[ImGuiCol_PlotHistogramHovered] = Colors::AccentOrangeHover;

        // Table
        colors[ImGuiCol_TableHeaderBg]      = Colors::Header;
        colors[ImGuiCol_TableBorderStrong]  = Colors::Border;
        colors[ImGuiCol_TableBorderLight]   = Colors::Separator;
        colors[ImGuiCol_TableRowBg]         = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
        colors[ImGuiCol_TableRowBgAlt]      = ImVec4(1.0f, 1.0f, 1.0f, 0.02f);

        // Text
        colors[ImGuiCol_Text]               = Colors::TextPrimary;
        colors[ImGuiCol_TextDisabled]       = Colors::TextDisabled;
        colors[ImGuiCol_TextSelectedBg]     = Colors::SelectionBlueBg;

        // Drag & Drop
        colors[ImGuiCol_DragDropTarget]     = Colors::AccentOrange;

        // Nav
        colors[ImGuiCol_NavHighlight]       = Colors::AccentOrange;
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.7f);
        colors[ImGuiCol_NavWindowingDimBg]  = ImVec4(0.8f, 0.8f, 0.8f, 0.2f);

        // Modal
        colors[ImGuiCol_ModalWindowDimBg]   = ImVec4(0.0f, 0.0f, 0.0f, 0.5f);

        // ========== STYLE ==========

        // Rounding
        style.WindowRounding    = 4.0f;
        style.ChildRounding     = 4.0f;
        style.FrameRounding     = 3.0f;
        style.PopupRounding     = 4.0f;
        style.ScrollbarRounding = 6.0f;
        style.GrabRounding      = 3.0f;
        style.TabRounding       = 4.0f;

        // Sizing
        style.WindowPadding     = ImVec2(8.0f, 8.0f);
        style.FramePadding      = ImVec2(6.0f, 4.0f);
        style.CellPadding       = ImVec2(4.0f, 2.0f);
        style.ItemSpacing       = ImVec2(8.0f, 4.0f);
        style.ItemInnerSpacing  = ImVec2(4.0f, 4.0f);
        style.IndentSpacing     = 12.0f;
        style.ScrollbarSize     = 14.0f;
        style.GrabMinSize       = 10.0f;

        // Borders
        style.WindowBorderSize  = 1.0f;
        style.ChildBorderSize   = 1.0f;
        style.PopupBorderSize   = 1.0f;
        style.FrameBorderSize   = 0.0f;
        style.TabBorderSize     = 0.0f;

        // Alignment
        style.WindowTitleAlign  = ImVec2(0.0f, 0.5f);
        style.ButtonTextAlign   = ImVec2(0.5f, 0.5f);
        style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

        // Anti-aliasing
        style.AntiAliasedLines  = true;
        style.AntiAliasedFill   = true;
    }

private:
    EditorTheme() = default;
    ~EditorTheme() = default;
    EditorTheme(const EditorTheme&) = delete;
    EditorTheme& operator=(const EditorTheme&) = delete;
};

} // namespace Yalaz::UI
