#pragma once
// =============================================================================
// YALAZ ENGINE - Console View
// =============================================================================

#include "EditorView.h"
#include <vector>
#include <string>

namespace Yalaz::UI {

enum class LogLevel { Info, Warning, Error };

struct LogEntry {
    LogLevel level;
    std::string message;
    std::string timestamp;
};

class ConsoleView : public EditorView {
public:
    ConsoleView() : EditorView("Console", "[C]", ViewCategory::Debug) {}
    void OnUpdate(float deltaTime) override;
    void OnRender() override;

    // Static logging
    static void Log(const std::string& msg) { Get().AddLog(LogLevel::Info, msg); }
    static void LogWarning(const std::string& msg) { Get().AddLog(LogLevel::Warning, msg); }
    static void LogError(const std::string& msg) { Get().AddLog(LogLevel::Error, msg); }
    static ConsoleView& Get() { static ConsoleView instance; return instance; }

private:
    void AddLog(LogLevel level, const std::string& msg);
    void RenderStats();
    void RenderLogs();

    std::vector<LogEntry> m_Logs;
    bool m_AutoScroll = true;
    bool m_ShowInfo = true;
    bool m_ShowWarnings = true;
    bool m_ShowErrors = true;
    char m_FilterBuffer[256] = "";

    float m_Fps = 0.0f;
    float m_FrameTime = 0.0f;
    float m_FpsHistory[120] = {0};
    int m_FpsIndex = 0;
};

} // namespace Yalaz::UI
