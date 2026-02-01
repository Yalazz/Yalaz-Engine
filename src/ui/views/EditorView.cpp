// =============================================================================
// YALAZ ENGINE - Editor View System Implementation
// =============================================================================
// This file provides utility implementations and specialized view base classes
// =============================================================================

#include "EditorView.h"
#include "../EditorTheme.h"
#include <imgui_internal.h>
#include <cstdarg>
#include <algorithm>
#include <cmath>

namespace Yalaz::UI {

// =============================================================================
// Namespace-level Color Constants (from EditorTheme when available)
// =============================================================================
namespace ViewColors {
    const ImVec4 Header = ImVec4(0.15f, 0.20f, 0.25f, 1.0f);
    const ImVec4 HeaderHovered = ImVec4(0.20f, 0.25f, 0.30f, 1.0f);
    const ImVec4 HeaderActive = ImVec4(0.25f, 0.30f, 0.35f, 1.0f);
    const ImVec4 Accent = ImVec4(0.26f, 0.59f, 0.98f, 1.0f);
    const ImVec4 AccentOrange = ImVec4(0.9f, 0.5f, 0.2f, 1.0f);
    const ImVec4 Success = ImVec4(0.3f, 0.8f, 0.3f, 1.0f);
    const ImVec4 Warning = ImVec4(0.9f, 0.7f, 0.2f, 1.0f);
    const ImVec4 Error = ImVec4(0.9f, 0.3f, 0.3f, 1.0f);
    const ImVec4 TextSecondary = ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
}

// =============================================================================
// Utility Functions for View Implementation
// =============================================================================

void DrawSeparator(float thickness, ImU32 color) {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();
    float width = ImGui::GetContentRegionAvail().x;
    drawList->AddLine(p, ImVec2(p.x + width, p.y), color, thickness);
    ImGui::Dummy(ImVec2(0, thickness + 2));
}

void DrawSectionHeader(const char* label, bool withSeparator) {
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.8f, 1.0f, 1.0f));
    ImGui::Text("%s", label);
    ImGui::PopStyleColor();
    if (withSeparator) {
        ImGui::Separator();
    }
    ImGui::Spacing();
}

bool DrawToolbarButton(const char* label, const char* tooltip, bool active) {
    if (active) {
        ImGui::PushStyleColor(ImGuiCol_Button, ViewColors::AccentOrange);
    }
    bool clicked = ImGui::Button(label);
    if (active) {
        ImGui::PopStyleColor();
    }
    if (tooltip && ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", tooltip);
    }
    ImGui::SameLine();
    return clicked;
}

void DrawToolbarSeparator() {
    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine();
}

void DrawPropertyRow(const char* label, const char* value, float labelWidth) {
    ImGui::Text("%s", label);
    ImGui::SameLine(labelWidth);
    ImGui::TextDisabled("%s", value);
}

void DrawProgressBarColored(float fraction, const ImVec2& size, ImU32 color, const char* overlay) {
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, color);
    ImGui::ProgressBar(fraction, size, overlay);
    ImGui::PopStyleColor();
}

void DrawStatsValue(const char* label, float value, const char* format, float labelWidth) {
    ImGui::Text("%s", label);
    ImGui::SameLine(labelWidth);
    ImGui::Text(format, value);
}

void DrawStatsValue(const char* label, int value, float labelWidth) {
    ImGui::Text("%s", label);
    ImGui::SameLine(labelWidth);
    ImGui::Text("%d", value);
}

// =============================================================================
// ViewState Serialization (optional, for layout save/restore)
// =============================================================================

ViewState::ViewState(const std::string& id)
    : viewId(id), isOpen(true), position(0, 0), size(400, 300) {
}

// =============================================================================
// View Helper: Collapsing Header with Custom Style
// =============================================================================

bool CollapsingHeaderStyled(const char* label, bool defaultOpen) {
    ImGui::PushStyleColor(ImGuiCol_Header, ViewColors::Header);
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ViewColors::HeaderHovered);
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ViewColors::HeaderActive);

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_CollapsingHeader;
    if (defaultOpen) {
        flags |= ImGuiTreeNodeFlags_DefaultOpen;
    }

    bool open = ImGui::CollapsingHeader(label, flags);
    ImGui::PopStyleColor(3);

    return open;
}

// =============================================================================
// View Helper: Begin Property Table
// =============================================================================

bool BeginPropertyTable(const char* id, float labelColumnWidth) {
    if (ImGui::BeginTable(id, 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV)) {
        ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed, labelColumnWidth);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
        return true;
    }
    return false;
}

void EndPropertyTable() {
    ImGui::EndTable();
}

// =============================================================================
// Property Row Helpers
// =============================================================================

bool PropertyRowFloat(const char* label, float* value, float speed, float min, float max) {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::AlignTextToFramePadding();
    ImGui::Text("%s", label);
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1);
    return ImGui::DragFloat(("##" + std::string(label)).c_str(), value, speed, min, max);
}

bool PropertyRowFloat3(const char* label, float* values, float speed) {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::AlignTextToFramePadding();
    ImGui::Text("%s", label);
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1);
    return ImGui::DragFloat3(("##" + std::string(label)).c_str(), values, speed);
}

bool PropertyRowColor3(const char* label, float* values) {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::AlignTextToFramePadding();
    ImGui::Text("%s", label);
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1);
    return ImGui::ColorEdit3(("##" + std::string(label)).c_str(), values);
}

bool PropertyRowColor4(const char* label, float* values) {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::AlignTextToFramePadding();
    ImGui::Text("%s", label);
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1);
    return ImGui::ColorEdit4(("##" + std::string(label)).c_str(), values);
}

bool PropertyRowBool(const char* label, bool* value) {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::AlignTextToFramePadding();
    ImGui::Text("%s", label);
    ImGui::TableSetColumnIndex(1);
    return ImGui::Checkbox(("##" + std::string(label)).c_str(), value);
}

bool PropertyRowString(const char* label, char* buffer, size_t bufferSize) {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::AlignTextToFramePadding();
    ImGui::Text("%s", label);
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1);
    return ImGui::InputText(("##" + std::string(label)).c_str(), buffer, bufferSize);
}

bool PropertyRowDropdown(const char* label, int* current, const char* const* items, int count) {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::AlignTextToFramePadding();
    ImGui::Text("%s", label);
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1);
    return ImGui::Combo(("##" + std::string(label)).c_str(), current, items, count);
}

void PropertyRowReadOnly(const char* label, const char* value) {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::AlignTextToFramePadding();
    ImGui::TextDisabled("%s", label);
    ImGui::TableSetColumnIndex(1);
    ImGui::TextDisabled("%s", value);
}

// =============================================================================
// Debug/Profiling Helpers
// =============================================================================

void PlotFrameTimeHistory(const char* label, const float* values, int count, float scaleMin, float scaleMax, const ImVec2& size) {
    ImGui::PlotLines(label, values, count, 0, nullptr, scaleMin, scaleMax, size);
}

void PlotHistogramStyled(const char* label, const float* values, int count, ImU32 color, const ImVec2& size) {
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, color);
    ImGui::PlotHistogram(label, values, count, 0, nullptr, 0.0f, 1.0f, size);
    ImGui::PopStyleColor();
}

bool BeginStatsTable(const char* id, int columns) {
    return ImGui::BeginTable(id, columns, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);
}

void StatsTableRow(const char* label, const char* value) {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Text("%s", label);
    ImGui::TableSetColumnIndex(1);
    ImGui::Text("%s", value);
}

void StatsTableRow(const char* label, float value, const char* fmt) {
    char buf[64];
    snprintf(buf, sizeof(buf), fmt, value);
    StatsTableRow(label, buf);
}

void StatsTableRow(const char* label, int value) {
    char buf[64];
    snprintf(buf, sizeof(buf), "%d", value);
    StatsTableRow(label, buf);
}

void EndStatsTable() {
    ImGui::EndTable();
}

// =============================================================================
// Checkerboard Background (for texture/UV views)
// =============================================================================

void DrawCheckerboard(ImDrawList* drawList, ImVec2 pos, ImVec2 size, int gridSize, ImU32 color1, ImU32 color2) {
    for (int y = 0; y < static_cast<int>(size.y); y += gridSize) {
        for (int x = 0; x < static_cast<int>(size.x); x += gridSize) {
            ImU32 col = ((x / gridSize + y / gridSize) % 2) ? color1 : color2;
            drawList->AddRectFilled(
                ImVec2(pos.x + x, pos.y + y),
                ImVec2(pos.x + std::min(x + gridSize, static_cast<int>(size.x)),
                       pos.y + std::min(y + gridSize, static_cast<int>(size.y))),
                col);
        }
    }
}

// =============================================================================
// Grid Background (for editors)
// =============================================================================

void DrawGridBackground(ImDrawList* drawList, ImVec2 pos, ImVec2 size, float gridStep, ImU32 majorColor, ImU32 minorColor) {
    // Minor grid lines
    for (float x = 0; x < size.x; x += gridStep) {
        drawList->AddLine(ImVec2(pos.x + x, pos.y), ImVec2(pos.x + x, pos.y + size.y), minorColor);
    }
    for (float y = 0; y < size.y; y += gridStep) {
        drawList->AddLine(ImVec2(pos.x, pos.y + y), ImVec2(pos.x + size.x, pos.y + y), minorColor);
    }

    // Major grid lines (every 5 cells)
    float majorStep = gridStep * 5;
    for (float x = 0; x < size.x; x += majorStep) {
        drawList->AddLine(ImVec2(pos.x + x, pos.y), ImVec2(pos.x + x, pos.y + size.y), majorColor);
    }
    for (float y = 0; y < size.y; y += majorStep) {
        drawList->AddLine(ImVec2(pos.x, pos.y + y), ImVec2(pos.x + size.x, pos.y + y), majorColor);
    }
}

} // namespace Yalaz::UI
