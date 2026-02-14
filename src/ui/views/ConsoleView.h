#pragma once
// =============================================================================
// YALAZ ENGINE - Console View
// =============================================================================
// Centralized logging system with UI display
// Usage:
//   Yalaz::UI::Console::Log("Info message");
//   Yalaz::UI::Console::Warn("Warning message");
//   Yalaz::UI::Console::Error("Error message");
//   Yalaz::UI::Console::Debug("Debug message");
//   Yalaz::UI::Console::Success("Success message");
// =============================================================================

#include "EditorView.h"
#include <vector>
#include <string>
#include <mutex>
#include <deque>

namespace Yalaz::UI {

enum class LogLevel { Debug, Info, Success, Warning, Error };

struct LogEntry {
    LogLevel level;
    std::string message;
    std::string timestamp;
    std::string source;   // e.g. "Engine", "Loader", "Shader", "PathTracer"
    uint32_t count = 1;   // for collapsing identical messages
};

// Global log storage (shared between all instances)
class LogStorage {
public:
    static LogStorage& Get() {
        static LogStorage instance;
        return instance;
    }

    void AddLog(LogLevel level, const std::string& msg, const std::string& source = "");
    std::vector<LogEntry> GetLogsSnapshot() const;
    void Clear() { std::lock_guard<std::mutex> lock(m_Mutex); m_Logs.clear(); }

    // Counts per level
    int GetCount(LogLevel level) const;
    int GetTotalCount() const;

private:
    std::deque<LogEntry> m_Logs;
    mutable std::mutex m_Mutex;
    static constexpr size_t MAX_LOGS = 2000;
};

// Console namespace for easy logging from anywhere
namespace Console {
    void Log(const std::string& msg);
    void Warn(const std::string& msg);
    void Error(const std::string& msg);
    void Debug(const std::string& msg);
    void Success(const std::string& msg);

    // With source tag
    void Log(const std::string& source, const std::string& msg);
    void Warn(const std::string& source, const std::string& msg);
    void Error(const std::string& source, const std::string& msg);
    void Debug(const std::string& source, const std::string& msg);
    void Success(const std::string& source, const std::string& msg);

    // Printf-style logging
    template<typename... Args>
    void Log(const char* fmt, Args&&... args) {
        char buf[2048];
        snprintf(buf, sizeof(buf), fmt, std::forward<Args>(args)...);
        Log(std::string(buf));
    }

    template<typename... Args>
    void Warn(const char* fmt, Args&&... args) {
        char buf[2048];
        snprintf(buf, sizeof(buf), fmt, std::forward<Args>(args)...);
        Warn(std::string(buf));
    }

    template<typename... Args>
    void Error(const char* fmt, Args&&... args) {
        char buf[2048];
        snprintf(buf, sizeof(buf), fmt, std::forward<Args>(args)...);
        Error(std::string(buf));
    }
}

class ConsoleView : public EditorView {
public:
    ConsoleView() : EditorView("Console", "[C]", ViewCategory::Debug) {}
    void OnUpdate(float deltaTime) override;
    void OnRender() override;

    // Legacy static methods (redirect to Console namespace)
    static void Log(const std::string& msg) { Console::Log(msg); }
    static void LogWarning(const std::string& msg) { Console::Warn(msg); }
    static void LogError(const std::string& msg) { Console::Error(msg); }

private:
    void RenderStats();
    void RenderLogs();
    void RenderLogEntry(const LogEntry& entry, int index);

    bool m_AutoScroll = true;
    bool m_ShowDebug = false;
    bool m_ShowInfo = true;
    bool m_ShowSuccess = true;
    bool m_ShowWarnings = true;
    bool m_ShowErrors = true;
    bool m_CollapseIdentical = true;
    char m_FilterBuffer[256] = "";
    int m_SelectedLog = -1;

    float m_Fps = 0.0f;
    float m_FrameTime = 0.0f;
    float m_FpsHistory[120] = {0};
    int m_FpsIndex = 0;
};

} // namespace Yalaz::UI
