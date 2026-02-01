// =============================================================================
// YALAZ ENGINE - Profiler View Implementation
// =============================================================================

#include "ProfilerView.h"
#include "../EditorTheme.h"
#include "../../vk_engine.h"

namespace Yalaz::UI {

void ProfilerView::OnUpdate(float deltaTime) {
    if (m_Paused) return;

    if (deltaTime > 0.0f && deltaTime < 1.0f) {
        m_CurrentFrameTime = deltaTime * 1000.0f;
        m_CurrentFps = 1.0f / deltaTime;

        // Update history
        m_FpsHistory[m_HistoryIndex] = m_CurrentFps;
        m_FrameTimeHistory[m_HistoryIndex] = m_CurrentFrameTime;
        m_HistoryIndex = (m_HistoryIndex + 1) % 300;

        // Update stats
        m_MinFps = std::min(m_MinFps, m_CurrentFps);
        m_MaxFps = std::max(m_MaxFps, m_CurrentFps);

        // Calculate average
        float sum = 0.0f;
        for (int i = 0; i < 300; ++i) {
            sum += m_FpsHistory[i];
        }
        m_AverageFps = sum / 300.0f;
    }
}

void ProfilerView::OnRender() {
    if (!BeginView(ImGuiWindowFlags_MenuBar)) {
        EndView();
        return;
    }

    if (ImGui::BeginMenuBar()) {
        if (ImGui::Button(m_Paused ? "Resume" : "Pause")) {
            m_Paused = !m_Paused;
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset")) {
            m_MinFps = 9999.0f;
            m_MaxFps = 0.0f;
            for (int i = 0; i < 300; ++i) {
                m_FpsHistory[i] = 0.0f;
                m_FrameTimeHistory[i] = 0.0f;
            }
        }
        ImGui::SameLine();
        ImGui::Checkbox("CPU", &m_ShowCpu);
        ImGui::SameLine();
        ImGui::Checkbox("GPU", &m_ShowGpu);

        ImGui::SameLine(ImGui::GetWindowWidth() - 150);
        ImGui::TextColored(
            m_CurrentFps >= 60 ? ImVec4(0.3f, 1.0f, 0.3f, 1.0f) :
            m_CurrentFps >= 30 ? ImVec4(1.0f, 0.8f, 0.2f, 1.0f) :
                                  ImVec4(1.0f, 0.3f, 0.3f, 1.0f),
            "%.0f FPS", m_CurrentFps);

        ImGui::EndMenuBar();
    }

    // Tabs
    if (ImGui::BeginTabBar("ProfilerTabs")) {
        if (ImGui::BeginTabItem("Overview")) {
            RenderOverview();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Timeline")) {
            RenderTimeline();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Statistics")) {
            RenderStatistics();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Memory")) {
            RenderMemory();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    EndView();
}

void ProfilerView::RenderOverview() {
    SectionHeader("Frame Performance");

    // Current stats
    ImGui::Columns(4, nullptr, false);
    ImGui::Text("Current"); ImGui::NextColumn();
    ImGui::Text("Average"); ImGui::NextColumn();
    ImGui::Text("Min"); ImGui::NextColumn();
    ImGui::Text("Max"); ImGui::NextColumn();

    ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "%.1f FPS", m_CurrentFps);
    ImGui::NextColumn();
    ImGui::Text("%.1f FPS", m_AverageFps);
    ImGui::NextColumn();
    ImGui::Text("%.1f FPS", m_MinFps == 9999.0f ? 0.0f : m_MinFps);
    ImGui::NextColumn();
    ImGui::Text("%.1f FPS", m_MaxFps);
    ImGui::Columns(1);

    ImGui::Spacing();
    ImGui::Text("Frame Time: %.2f ms", m_CurrentFrameTime);

    ImGui::Spacing();
    SectionHeader("FPS History");

    // FPS graph
    ImGui::PlotLines("##FPS", m_FpsHistory, 300, m_HistoryIndex,
                    "FPS", 0.0f, 144.0f, ImVec2(-1, 100));

    ImGui::Spacing();
    SectionHeader("Frame Time History");

    // Frame time graph
    ImGui::PlotLines("##FrameTime", m_FrameTimeHistory, 300, m_HistoryIndex,
                    "ms", 0.0f, 33.3f, ImVec2(-1, 100));
}

void ProfilerView::RenderTimeline() {
    SectionHeader("Frame Timeline");

    ImGui::TextDisabled("Frame breakdown visualization");

    // Simplified timeline
    ImVec2 size = ImGui::GetContentRegionAvail();
    ImGui::BeginChild("Timeline", ImVec2(0, 150), true);

    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // Background
    drawList->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + 140), IM_COL32(30, 30, 35, 255));

    // Example timeline bars
    float barHeight = 20.0f;
    float y = pos.y + 10;

    // CPU bar
    if (m_ShowCpu) {
        float cpuWidth = std::min(size.x * 0.6f, size.x * (m_CurrentFrameTime / 16.67f));
        drawList->AddRectFilled(ImVec2(pos.x + 10, y), ImVec2(pos.x + 10 + cpuWidth, y + barHeight),
                               IM_COL32(50, 150, 255, 255));
        drawList->AddText(ImVec2(pos.x + 15, y + 3), IM_COL32(255, 255, 255, 255), "CPU");
        y += barHeight + 5;
    }

    // GPU bar
    if (m_ShowGpu) {
        float gpuWidth = std::min(size.x * 0.5f, size.x * (m_CurrentFrameTime / 16.67f) * 0.8f);
        drawList->AddRectFilled(ImVec2(pos.x + 10, y), ImVec2(pos.x + 10 + gpuWidth, y + barHeight),
                               IM_COL32(50, 255, 100, 255));
        drawList->AddText(ImVec2(pos.x + 15, y + 3), IM_COL32(255, 255, 255, 255), "GPU");
    }

    // 16.67ms line (60 FPS target)
    float targetX = pos.x + 10 + size.x * 0.6f;
    drawList->AddLine(ImVec2(targetX, pos.y), ImVec2(targetX, pos.y + 140), IM_COL32(255, 100, 100, 200), 2.0f);
    drawList->AddText(ImVec2(targetX + 5, pos.y + 5), IM_COL32(255, 100, 100, 255), "16.67ms");

    ImGui::EndChild();
}

void ProfilerView::RenderStatistics() {
    SectionHeader("Render Statistics");

    if (m_Engine) {
        ImGui::Columns(2, nullptr, false);
        ImGui::Text("Draw Calls:"); ImGui::NextColumn();
        ImGui::Text("%d", m_Engine->stats.drawcall_count); ImGui::NextColumn();

        ImGui::Text("Triangles:"); ImGui::NextColumn();
        ImGui::Text("%d", m_Engine->stats.triangle_count); ImGui::NextColumn();

        ImGui::Text("Primitives:"); ImGui::NextColumn();
        ImGui::Text("%zu", m_Engine->static_shapes.size()); ImGui::NextColumn();

        ImGui::Text("Lights:"); ImGui::NextColumn();
        ImGui::Text("%zu", m_Engine->scenePointLights.size()); ImGui::NextColumn();

        ImGui::Text("Scenes:"); ImGui::NextColumn();
        ImGui::Text("%zu", m_Engine->loadedScenes.size()); ImGui::NextColumn();
        ImGui::Columns(1);
    } else {
        ImGui::TextDisabled("No engine data available");
    }
}

void ProfilerView::RenderMemory() {
    SectionHeader("Memory Usage");

    // Placeholder memory stats
    ImGui::Text("VRAM Usage:");
    ImGui::ProgressBar(0.4f, ImVec2(-1, 20), "4.2 GB / 10 GB");

    ImGui::Spacing();
    ImGui::Text("Texture Memory:");
    ImGui::ProgressBar(0.3f, ImVec2(-1, 20), "256 MB");

    ImGui::Spacing();
    ImGui::Text("Buffer Memory:");
    ImGui::ProgressBar(0.2f, ImVec2(-1, 20), "128 MB");

    ImGui::Spacing();
    SectionHeader("Allocations");
    ImGui::TextDisabled("Buffer allocations: -");
    ImGui::TextDisabled("Image allocations: -");
    ImGui::TextDisabled("Descriptor sets: -");
}

} // namespace Yalaz::UI
