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
// Color & Style Definitions
// =============================================================================

static ImVec4 GetLevelColor(LogLevel level) {
    switch (level) {
        case LogLevel::Debug:   return ImVec4(0.5f, 0.5f, 0.6f, 1.0f);  // Grey-blue
        case LogLevel::Info:    return ImVec4(0.8f, 0.9f, 1.0f, 1.0f);  // Light blue-white
        case LogLevel::Success: return ImVec4(0.3f, 1.0f, 0.5f, 1.0f);  // Green
        case LogLevel::Warning: return ImVec4(1.0f, 0.85f, 0.2f, 1.0f); // Yellow
        case LogLevel::Error:   return ImVec4(1.0f, 0.3f, 0.3f, 1.0f);  // Red
    }
    return ImVec4(1, 1, 1, 1);
}

static ImVec4 GetLevelBgColor(LogLevel level) {
    switch (level) {
        case LogLevel::Debug:   return ImVec4(0.15f, 0.15f, 0.2f, 0.3f);
        case LogLevel::Info:    return ImVec4(0.1f, 0.12f, 0.18f, 0.0f);
        case LogLevel::Success: return ImVec4(0.05f, 0.2f, 0.08f, 0.3f);
        case LogLevel::Warning: return ImVec4(0.25f, 0.2f, 0.05f, 0.3f);
        case LogLevel::Error:   return ImVec4(0.3f, 0.05f, 0.05f, 0.4f);
    }
    return ImVec4(0, 0, 0, 0);
}

static const char* GetLevelTag(LogLevel level) {
    switch (level) {
        case LogLevel::Debug:   return "DBG";
        case LogLevel::Info:    return "INF";
        case LogLevel::Success: return "OK ";
        case LogLevel::Warning: return "WRN";
        case LogLevel::Error:   return "ERR";
    }
    return "???";
}

static const char* GetLevelIcon(LogLevel level) {
    switch (level) {
        case LogLevel::Debug:   return "[~]";
        case LogLevel::Info:    return "[i]";
        case LogLevel::Success: return "[+]";
        case LogLevel::Warning: return "[!]";
        case LogLevel::Error:   return "[X]";
    }
    return "[?]";
}

static const char* GetStdoutPrefix(LogLevel level) {
    switch (level) {
        case LogLevel::Debug:   return "[DEBUG]";
        case LogLevel::Info:    return "[INFO]";
        case LogLevel::Success: return "[OK]";
        case LogLevel::Warning: return "[WARN]";
        case LogLevel::Error:   return "[ERROR]";
    }
    return "[???]";
}

// Auto-detect source from message prefix like "[Engine]", "[Loader]", etc.
static std::string DetectSource(const std::string& msg) {
    if (msg.size() > 2 && msg[0] == '[') {
        size_t end = msg.find(']');
        if (end != std::string::npos && end < 30) {
            return msg.substr(1, end - 1);
        }
    }
    return "";
}

// Strip source prefix from message if it matches detected source
static std::string StripSourcePrefix(const std::string& msg, const std::string& source) {
    if (!source.empty() && msg.size() > source.size() + 2) {
        size_t skip = source.size() + 2; // "[Source]"
        if (skip < msg.size() && msg[skip] == ' ') skip++;
        return msg.substr(skip);
    }
    return msg;
}

// =============================================================================
// Global Log Storage
// =============================================================================

void LogStorage::AddLog(LogLevel level, const std::string& msg, const std::string& source) {
    std::lock_guard<std::mutex> lock(m_Mutex);

    // Auto-detect source from message if not provided
    std::string src = source;
    std::string cleanMsg = msg;
    if (src.empty()) {
        src = DetectSource(msg);
        if (!src.empty()) {
            cleanMsg = StripSourcePrefix(msg, src);
        }
    }

    // Strip trailing newline
    while (!cleanMsg.empty() && (cleanMsg.back() == '\n' || cleanMsg.back() == '\r')) {
        cleanMsg.pop_back();
    }

    // Collapse identical consecutive messages
    if (!m_Logs.empty()) {
        auto& last = m_Logs.back();
        if (last.level == level && last.message == cleanMsg && last.source == src) {
            last.count++;
            return;
        }
    }

    LogEntry entry;
    entry.level = level;
    entry.message = cleanMsg;
    entry.source = src;
    entry.count = 1;

    // Timestamp
    time_t now = time(nullptr);
    tm* t = localtime(&now);
    char buf[32];
    strftime(buf, sizeof(buf), "%H:%M:%S", t);
    entry.timestamp = buf;

    m_Logs.push_back(entry);

    // Limit size
    while (m_Logs.size() > MAX_LOGS) {
        m_Logs.pop_front();
    }

    // Also print to stdout
    printf("%s %s %s%s%s\n", buf, GetStdoutPrefix(level),
           src.empty() ? "" : "[", src.empty() ? "" : src.c_str(),
           src.empty() ? "" : "] ");
    printf("  %s\n", cleanMsg.c_str());
}

std::vector<LogEntry> LogStorage::GetLogsSnapshot() const {
    std::lock_guard<std::mutex> lock(m_Mutex);
    return std::vector<LogEntry>(m_Logs.begin(), m_Logs.end());
}

int LogStorage::GetCount(LogLevel level) const {
    std::lock_guard<std::mutex> lock(m_Mutex);
    int count = 0;
    for (const auto& entry : m_Logs) {
        if (entry.level == level) count++;
    }
    return count;
}

int LogStorage::GetTotalCount() const {
    std::lock_guard<std::mutex> lock(m_Mutex);
    return static_cast<int>(m_Logs.size());
}

// =============================================================================
// Console Namespace Functions
// =============================================================================

namespace Console {
    void Log(const std::string& msg)     { LogStorage::Get().AddLog(LogLevel::Info, msg); }
    void Warn(const std::string& msg)    { LogStorage::Get().AddLog(LogLevel::Warning, msg); }
    void Error(const std::string& msg)   { LogStorage::Get().AddLog(LogLevel::Error, msg); }
    void Debug(const std::string& msg)   { LogStorage::Get().AddLog(LogLevel::Debug, msg); }
    void Success(const std::string& msg) { LogStorage::Get().AddLog(LogLevel::Success, msg); }

    void Log(const std::string& source, const std::string& msg)     { LogStorage::Get().AddLog(LogLevel::Info, msg, source); }
    void Warn(const std::string& source, const std::string& msg)    { LogStorage::Get().AddLog(LogLevel::Warning, msg, source); }
    void Error(const std::string& source, const std::string& msg)   { LogStorage::Get().AddLog(LogLevel::Error, msg, source); }
    void Debug(const std::string& source, const std::string& msg)   { LogStorage::Get().AddLog(LogLevel::Debug, msg, source); }
    void Success(const std::string& source, const std::string& msg) { LogStorage::Get().AddLog(LogLevel::Success, msg, source); }
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

    // Menu bar with level filter toggles and counts
    if (ImGui::BeginMenuBar()) {
        if (ImGui::Button("Clear")) {
            LogStorage::Get().Clear();
        }

        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();

        // Level filter toggles with colored count badges
        auto& storage = LogStorage::Get();

        // Debug toggle
        int debugCount = storage.GetCount(LogLevel::Debug);
        ImGui::PushStyleColor(ImGuiCol_Text, m_ShowDebug ? GetLevelColor(LogLevel::Debug) : ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
        if (ImGui::SmallButton(fmt::format("DBG ({})", debugCount).c_str())) m_ShowDebug = !m_ShowDebug;
        ImGui::PopStyleColor();
        ImGui::SameLine();

        // Info toggle
        int infoCount = storage.GetCount(LogLevel::Info);
        ImGui::PushStyleColor(ImGuiCol_Text, m_ShowInfo ? GetLevelColor(LogLevel::Info) : ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
        if (ImGui::SmallButton(fmt::format("INF ({})", infoCount).c_str())) m_ShowInfo = !m_ShowInfo;
        ImGui::PopStyleColor();
        ImGui::SameLine();

        // Success toggle
        int successCount = storage.GetCount(LogLevel::Success);
        ImGui::PushStyleColor(ImGuiCol_Text, m_ShowSuccess ? GetLevelColor(LogLevel::Success) : ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
        if (ImGui::SmallButton(fmt::format("OK ({})", successCount).c_str())) m_ShowSuccess = !m_ShowSuccess;
        ImGui::PopStyleColor();
        ImGui::SameLine();

        // Warning toggle
        int warnCount = storage.GetCount(LogLevel::Warning);
        ImGui::PushStyleColor(ImGuiCol_Text, m_ShowWarnings ? GetLevelColor(LogLevel::Warning) : ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
        if (ImGui::SmallButton(fmt::format("WRN ({})", warnCount).c_str())) m_ShowWarnings = !m_ShowWarnings;
        ImGui::PopStyleColor();
        ImGui::SameLine();

        // Error toggle
        int errCount = storage.GetCount(LogLevel::Error);
        ImGui::PushStyleColor(ImGuiCol_Text, m_ShowErrors ? GetLevelColor(LogLevel::Error) : ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
        if (ImGui::SmallButton(fmt::format("ERR ({})", errCount).c_str())) m_ShowErrors = !m_ShowErrors;
        ImGui::PopStyleColor();

        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();

        ImGui::Checkbox("Collapse", &m_CollapseIdentical);
        ImGui::SameLine();
        ImGui::Checkbox("Scroll", &m_AutoScroll);

        // FPS display on the right
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
        // Show error count badge on Logs tab
        int totalErrors = LogStorage::Get().GetCount(LogLevel::Error);
        std::string logsLabel = totalErrors > 0
            ? fmt::format("Logs ({} errors)###Logs", totalErrors)
            : "Logs###Logs";

        if (ImGui::BeginTabItem("Stats")) {
            RenderStats();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem(logsLabel.c_str())) {
            RenderLogs();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    EndView();
}

void ConsoleView::RenderStats() {
    if (!m_Engine) return;

    SectionHeader("Performance");

    ImGui::Columns(3, nullptr, false);
    ImGui::Text("FPS: %.0f", m_Fps);
    ImGui::NextColumn();
    ImGui::Text("Frame: %.2f ms", m_FrameTime);
    ImGui::NextColumn();
    ImGui::Text("Delta: %.4f s", m_FrameTime / 1000.0f);
    ImGui::Columns(1);

    ImGui::Spacing();
    ImGui::PlotLines("##FPS", m_FpsHistory, 120, m_FpsIndex, "FPS History", 0.0f, 144.0f, ImVec2(-1, 80));

    ImGui::Spacing();
    SectionHeader("Scene");

    ImGui::Columns(2, nullptr, false);
    ImGui::Text("Primitives: %zu", m_Engine->static_shapes.size());
    ImGui::NextColumn();
    ImGui::Text("Point Lights: %zu", m_Engine->scenePointLights.size());
    ImGui::Columns(1);

    ImGui::Text("Loaded Scenes: %zu", m_Engine->loadedScenes.size());

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
    // Filter bar
    ImGui::SetNextItemWidth(250);
    ImGui::InputTextWithHint("##Filter", "Filter logs...", m_FilterBuffer, sizeof(m_FilterBuffer));

    ImGui::SameLine();
    int total = LogStorage::Get().GetTotalCount();
    ImGui::TextDisabled("(%d entries)", total);

    ImGui::Separator();

    // Log list with clipping for performance
    ImGui::BeginChild("LogList", ImVec2(0, 0), false);

    std::string lowerFilter = m_FilterBuffer;
    std::transform(lowerFilter.begin(), lowerFilter.end(), lowerFilter.begin(), ::tolower);

    const auto logs = LogStorage::Get().GetLogsSnapshot();

    ImGuiListClipper clipper;
    // We need to filter first, then clip - so collect visible indices
    std::vector<int> visibleIndices;
    visibleIndices.reserve(logs.size());

    for (int i = 0; i < static_cast<int>(logs.size()); i++) {
        const auto& log = logs[i];

        // Level filter
        switch (log.level) {
            case LogLevel::Debug:   if (!m_ShowDebug) continue; break;
            case LogLevel::Info:    if (!m_ShowInfo) continue; break;
            case LogLevel::Success: if (!m_ShowSuccess) continue; break;
            case LogLevel::Warning: if (!m_ShowWarnings) continue; break;
            case LogLevel::Error:   if (!m_ShowErrors) continue; break;
        }

        // Text filter
        if (!lowerFilter.empty()) {
            std::string lowerMsg = log.message;
            std::transform(lowerMsg.begin(), lowerMsg.end(), lowerMsg.begin(), ::tolower);
            std::string lowerSrc = log.source;
            std::transform(lowerSrc.begin(), lowerSrc.end(), lowerSrc.begin(), ::tolower);
            if (lowerMsg.find(lowerFilter) == std::string::npos &&
                lowerSrc.find(lowerFilter) == std::string::npos) continue;
        }

        visibleIndices.push_back(i);
    }

    clipper.Begin(static_cast<int>(visibleIndices.size()), ImGui::GetTextLineHeightWithSpacing() + 2.0f);
    while (clipper.Step()) {
        for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++) {
            int idx = visibleIndices[row];
            RenderLogEntry(logs[idx], row);
        }
    }
    clipper.End();

    // Auto-scroll
    if (m_AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 20.0f) {
        ImGui::SetScrollHereY(1.0f);
    }

    ImGui::EndChild();
}

void ConsoleView::RenderLogEntry(const LogEntry& entry, int index) {
    ImVec4 color = GetLevelColor(entry.level);
    ImVec4 bgColor = GetLevelBgColor(entry.level);
    const char* icon = GetLevelIcon(entry.level);
    const char* tag = GetLevelTag(entry.level);

    // Alternating row background
    ImVec2 cursorPos = ImGui::GetCursorScreenPos();
    float rowHeight = ImGui::GetTextLineHeightWithSpacing() + 2.0f;
    float rowWidth = ImGui::GetContentRegionAvail().x;

    // Background color based on level (errors/warnings get tinted bg)
    ImVec4 rowBg = (index % 2 == 0)
        ? ImVec4(0.12f, 0.12f, 0.14f, 1.0f)
        : ImVec4(0.10f, 0.10f, 0.12f, 1.0f);

    // Blend with level color for warnings/errors
    if (entry.level == LogLevel::Error || entry.level == LogLevel::Warning) {
        rowBg.x += bgColor.x;
        rowBg.y += bgColor.y;
        rowBg.z += bgColor.z;
    }

    ImGui::GetWindowDrawList()->AddRectFilled(
        cursorPos,
        ImVec2(cursorPos.x + rowWidth, cursorPos.y + rowHeight),
        ImGui::ColorConvertFloat4ToU32(rowBg));

    // Level color bar on the left edge
    ImGui::GetWindowDrawList()->AddRectFilled(
        cursorPos,
        ImVec2(cursorPos.x + 3.0f, cursorPos.y + rowHeight),
        ImGui::ColorConvertFloat4ToU32(color));

    ImGui::SetCursorScreenPos(ImVec2(cursorPos.x + 6.0f, cursorPos.y + 1.0f));

    // Timestamp (dimmed)
    ImGui::TextColored(ImVec4(0.45f, 0.45f, 0.5f, 1.0f), "%s", entry.timestamp.c_str());
    ImGui::SameLine();

    // Level badge with colored background
    {
        ImVec2 badgePos = ImGui::GetCursorScreenPos();
        ImVec2 badgeSize = ImGui::CalcTextSize(tag);
        float padX = 4.0f, padY = 1.0f;

        ImVec4 badgeBg = color;
        badgeBg.w = 0.25f; // Semi-transparent

        ImGui::GetWindowDrawList()->AddRectFilled(
            ImVec2(badgePos.x - padX, badgePos.y - padY),
            ImVec2(badgePos.x + badgeSize.x + padX, badgePos.y + badgeSize.y + padY),
            ImGui::ColorConvertFloat4ToU32(badgeBg),
            3.0f);

        ImGui::TextColored(color, "%s", tag);
    }
    ImGui::SameLine();

    // Source badge (if present)
    if (!entry.source.empty()) {
        ImVec2 srcPos = ImGui::GetCursorScreenPos();
        ImVec2 srcSize = ImGui::CalcTextSize(entry.source.c_str());
        float padX = 3.0f, padY = 1.0f;

        ImGui::GetWindowDrawList()->AddRectFilled(
            ImVec2(srcPos.x - padX, srcPos.y - padY),
            ImVec2(srcPos.x + srcSize.x + padX, srcPos.y + srcSize.y + padY),
            ImGui::ColorConvertFloat4ToU32(ImVec4(0.3f, 0.3f, 0.4f, 0.4f)),
            3.0f);

        ImGui::TextColored(ImVec4(0.6f, 0.7f, 0.9f, 1.0f), "%s", entry.source.c_str());
        ImGui::SameLine();
    }

    // Message text
    ImGui::TextColored(color, "%s", entry.message.c_str());

    // Repeat count badge
    if (entry.count > 1) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.8f, 1.0f), "(x%u)", entry.count);
    }
}

} // namespace Yalaz::UI
