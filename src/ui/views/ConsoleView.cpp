// =============================================================================
// YALAZ ENGINE - Console View Implementation
// =============================================================================

#include "ConsoleView.h"
#include "../EditorTheme.h"
#include "../../vk_engine.h"
#include <ctime>
#include <algorithm>
#include <cstdio>

namespace Yalaz::UI {

// =============================================================================
// Global Log Storage
// =============================================================================

void LogStorage::AddLog(LogLevel level, const std::string& msg) {
    std::lock_guard<std::mutex> lock(m_Mutex);

    LogEntry entry;
    entry.level = level;
    entry.message = msg;

    // Get timestamp
    time_t now = time(nullptr);
    tm* t = localtime(&now);
    char buf[32];
    strftime(buf, sizeof(buf), "[%H:%M:%S]", t);
    entry.timestamp = buf;

    m_Logs.push_back(entry);

    // Limit size
    while (m_Logs.size() > 1000) {
        m_Logs.erase(m_Logs.begin());
    }

    // Also print to stdout for debugging
    const char* prefix = (level == LogLevel::Info) ? "[INFO]" :
                         (level == LogLevel::Warning) ? "[WARN]" : "[ERROR]";
    printf("%s %s %s\n", buf, prefix, msg.c_str());
}

// =============================================================================
// Console Namespace Functions
// =============================================================================

namespace Console {
    void Log(const std::string& msg) {
        LogStorage::Get().AddLog(LogLevel::Info, msg);
    }

    void Warn(const std::string& msg) {
        LogStorage::Get().AddLog(LogLevel::Warning, msg);
    }

    void Error(const std::string& msg) {
        LogStorage::Get().AddLog(LogLevel::Error, msg);
    }
}

// =============================================================================
// ConsoleView Implementation
// =============================================================================

void ConsoleView::OnUpdate(float deltaTime) {
    if (deltaTime > 0.0f && deltaTime < 1.0f) {
        float instantFps = 1.0f / deltaTime;
        m_Fps = m_Fps * 0.95f + instantFps * 0.05f;
        m_FrameTime = deltaTime * 1000.0f;

        m_FpsHistory[m_FpsIndex] = m_Fps;
        m_FpsIndex = (m_FpsIndex + 1) % 120;
    }
}

void ConsoleView::OnRender() {
    if (!BeginView(ImGuiWindowFlags_MenuBar)) {
        EndView();
        return;
    }

    // Menu bar
    if (ImGui::BeginMenuBar()) {
        if (ImGui::Button("Clear All")) {
            LogStorage::Get().Clear();
        }

        ImGui::SameLine();
        ImGui::Checkbox("Info", &m_ShowInfo);
        ImGui::SameLine();
        ImGui::Checkbox("Warn", &m_ShowWarnings);
        ImGui::SameLine();
        ImGui::Checkbox("Error", &m_ShowErrors);
        ImGui::SameLine();
        ImGui::Checkbox("Auto-scroll", &m_AutoScroll);

        // FPS display
        ImGui::SameLine(ImGui::GetWindowWidth() - 120);
        ImGui::TextColored(
            m_Fps >= 60 ? ImVec4(0.3f, 1.0f, 0.3f, 1.0f) :
            m_Fps >= 30 ? ImVec4(1.0f, 0.8f, 0.2f, 1.0f) :
                          ImVec4(1.0f, 0.3f, 0.3f, 1.0f),
            "%.0f FPS", m_Fps);

        ImGui::EndMenuBar();
    }

    // Tabs for Stats and Logs
    if (ImGui::BeginTabBar("ConsoleTabs")) {
        if (ImGui::BeginTabItem("Stats")) {
            RenderStats();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Logs")) {
            RenderLogs();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    EndView();
}

void ConsoleView::RenderStats() {
    if (!m_Engine) return;

    // Performance stats
    SectionHeader("Performance");

    ImGui::Columns(3, nullptr, false);
    ImGui::Text("FPS: %.0f", m_Fps);
    ImGui::NextColumn();
    ImGui::Text("Frame: %.2f ms", m_FrameTime);
    ImGui::NextColumn();
    ImGui::Text("Delta: %.4f s", m_FrameTime / 1000.0f);
    ImGui::Columns(1);

    ImGui::Spacing();

    // FPS Graph
    ImGui::PlotLines("##FPS", m_FpsHistory, 120, m_FpsIndex, "FPS History", 0.0f, 144.0f, ImVec2(-1, 80));

    ImGui::Spacing();
    SectionHeader("Scene");

    ImGui::Columns(2, nullptr, false);
    ImGui::Text("Primitives: %zu", m_Engine->static_shapes.size());
    ImGui::NextColumn();
    ImGui::Text("Point Lights: %zu", m_Engine->scenePointLights.size());
    ImGui::Columns(1);

    ImGui::Text("Loaded Scenes: %zu", m_Engine->loadedScenes.size());

    // View mode - matches VulkanEngine::ViewMode enum order
    const char* viewModes[] = { "Solid", "Shaded", "Material Preview", "Rendered", "Wireframe", "Normals", "UV Checker", "Path Traced" };
    int mode = static_cast<int>(m_Engine->_currentViewMode);
    if (mode >= 0 && mode < 8) {
        ImGui::Text("View Mode: %s", viewModes[mode]);
    }

    ImGui::Spacing();
    SectionHeader("Camera");
    ImGui::Text("Position: (%.1f, %.1f, %.1f)",
        m_Engine->mainCamera.position.x,
        m_Engine->mainCamera.position.y,
        m_Engine->mainCamera.position.z);
    ImGui::Text("Pitch: %.1f  Yaw: %.1f",
        glm::degrees(m_Engine->mainCamera.pitch),
        glm::degrees(m_Engine->mainCamera.yaw));
}

void ConsoleView::RenderLogs() {
    // Filter
    ImGui::SetNextItemWidth(200);
    ImGui::InputTextWithHint("##Filter", "Filter...", m_FilterBuffer, sizeof(m_FilterBuffer));

    ImGui::SameLine();
    if (ImGui::Button("Clear")) {
        LogStorage::Get().Clear();
    }

    ImGui::SameLine();
    ImGui::TextDisabled("(%zu entries)", LogStorage::Get().GetLogs().size());

    ImGui::Separator();

    // Log list
    ImGui::BeginChild("LogList", ImVec2(0, 0), false);

    std::string lowerFilter = m_FilterBuffer;
    std::transform(lowerFilter.begin(), lowerFilter.end(), lowerFilter.begin(), ::tolower);

    // Read from global storage
    const auto& logs = LogStorage::Get().GetLogs();

    for (const auto& log : logs) {
        // Level filter
        if (log.level == LogLevel::Info && !m_ShowInfo) continue;
        if (log.level == LogLevel::Warning && !m_ShowWarnings) continue;
        if (log.level == LogLevel::Error && !m_ShowErrors) continue;

        // Text filter
        if (!lowerFilter.empty()) {
            std::string lowerMsg = log.message;
            std::transform(lowerMsg.begin(), lowerMsg.end(), lowerMsg.begin(), ::tolower);
            if (lowerMsg.find(lowerFilter) == std::string::npos) continue;
        }

        // Color based on level
        ImVec4 color;
        const char* prefix;
        switch (log.level) {
            case LogLevel::Info:
                color = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);
                prefix = "[INFO]";
                break;
            case LogLevel::Warning:
                color = ImVec4(1.0f, 0.8f, 0.2f, 1.0f);
                prefix = "[WARN]";
                break;
            case LogLevel::Error:
                color = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
                prefix = "[ERROR]";
                break;
        }

        ImGui::TextColored(color, "%s %s %s", log.timestamp.c_str(), prefix, log.message.c_str());
    }

    if (m_AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }

    ImGui::EndChild();
}

} // namespace Yalaz::UI
