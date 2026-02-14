#pragma once
// =============================================================================
// YALAZ ENGINE - Console View
// =============================================================================
// Centralized logging system with UI display
// Usage:
//   Yalaz::UI::Console::Log("Info message");
//   Yalaz::UI::Console::Warn("Warning message");
//   Yalaz::UI::Console::Error("Error message");
// =============================================================================

#include "EditorView.h"
#include <vector>
#include <string>
#include <mutex>

namespace Yalaz::UI {

enum class LogLevel { Info, Warning, Error };

struct LogEntry {
    LogLevel level;
    std::string message;
    std::string timestamp;
};

// Global log storage (shared between all instances)
class LogStorage {
public:
    static LogStorage& Get() {
        static LogStorage instance;
        return instance;
    }

    void AddLog(LogLevel level, const std::string& msg);
    const std::vector<LogEntry>& GetLogs() const { return m_Logs; }
    std::vector<LogEntry> GetLogsSnapshot() const;
    void Clear() { std::lock_guard<std::mutex> lock(m_Mutex); m_Logs.clear(); }

private:
    std::vector<LogEntry> m_Logs;
    mutable std::mutex m_Mutex;
};

// Console namespace for easy logging from anywhere
namespace Console {
    void Log(const std::string& msg);
    void Warn(const std::string& msg);
    void Error(const std::string& msg);

    // Printf-style logging
    template<typename... Args>
    void Log(const char* fmt, Args&&... args) {
        char buf[1024];
        snprintf(buf, sizeof(buf), fmt, std::forward<Args>(args)...);
        Log(std::string(buf));
    }

    template<typename... Args>
    void Warn(const char* fmt, Args&&... args) {
        char buf[1024];
        snprintf(buf, sizeof(buf), fmt, std::forward<Args>(args)...);
        Warn(std::string(buf));
    }

    template<typename... Args>
    void Error(const char* fmt, Args&&... args) {
        char buf[1024];
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
