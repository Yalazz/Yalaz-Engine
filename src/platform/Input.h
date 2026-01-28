#pragma once

#include "core/ISubsystem.h"
#include <glm/glm.hpp>
#include <array>
#include <functional>

union SDL_Event;

namespace Yalaz::Platform {

/**
 * @brief Keyboard key codes (subset of SDL scancodes)
 */
enum class Key {
    Unknown = 0,
    A, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
    Space, Enter, Escape, Tab, Backspace,
    Left, Right, Up, Down,
    LShift, RShift, LCtrl, RCtrl, LAlt, RAlt,
    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    Delete, Home, End, PageUp, PageDown,
    Count
};

/**
 * @brief Mouse button codes
 */
enum class MouseButton {
    Left = 0,
    Middle,
    Right,
    X1,
    X2,
    Count
};

/**
 * @brief Input management (keyboard, mouse)
 *
 * Design Patterns:
 * - Singleton: Central input state
 * - Observer: Event callbacks
 *
 * SOLID:
 * - Single Responsibility: Only manages input
 */
class Input : public Core::ISubsystem, public Core::IUpdatable {
public:
    static Input& Get() {
        static Input instance;
        return instance;
    }

    // ISubsystem interface
    void OnInit() override;
    void OnShutdown() override;
    void OnUpdate(float deltaTime) override;
    const char* GetName() const override { return "Input"; }

    // =========================================================================
    // Event Processing
    // =========================================================================

    /**
     * @brief Process an SDL event
     * @return true if event was consumed
     */
    bool ProcessEvent(const SDL_Event& event);

    /**
     * @brief Update state at end of frame (clear deltas)
     */
    void EndFrame();

    // =========================================================================
    // Keyboard State
    // =========================================================================

    bool IsKeyDown(Key key) const;
    bool IsKeyPressed(Key key) const;  // Just pressed this frame
    bool IsKeyReleased(Key key) const; // Just released this frame

    bool IsShiftDown() const;
    bool IsCtrlDown() const;
    bool IsAltDown() const;

    // =========================================================================
    // Mouse State
    // =========================================================================

    glm::vec2 GetMousePosition() const { return m_MousePosition; }
    glm::vec2 GetMouseDelta() const { return m_MouseDelta; }
    float GetScrollDelta() const { return m_ScrollDelta; }

    bool IsMouseButtonDown(MouseButton button) const;
    bool IsMouseButtonPressed(MouseButton button) const;
    bool IsMouseButtonReleased(MouseButton button) const;

    /**
     * @brief Check if mouse is captured (relative mode)
     */
    bool IsMouseCaptured() const { return m_MouseCaptured; }

    /**
     * @brief Capture/release mouse
     */
    void SetMouseCapture(bool capture);

    // =========================================================================
    // Callbacks
    // =========================================================================

    using KeyCallback = std::function<void(Key, bool)>;
    using MouseCallback = std::function<void(MouseButton, bool)>;
    using ScrollCallback = std::function<void(float)>;

    void SetKeyCallback(KeyCallback callback) { m_KeyCallback = callback; }
    void SetMouseCallback(MouseCallback callback) { m_MouseCallback = callback; }
    void SetScrollCallback(ScrollCallback callback) { m_ScrollCallback = callback; }

private:
    Input() = default;
    ~Input() = default;

    // Keyboard state
    std::array<bool, static_cast<size_t>(Key::Count)> m_KeyState{};
    std::array<bool, static_cast<size_t>(Key::Count)> m_KeyPressed{};
    std::array<bool, static_cast<size_t>(Key::Count)> m_KeyReleased{};

    // Mouse state
    glm::vec2 m_MousePosition{0.0f};
    glm::vec2 m_MouseDelta{0.0f};
    float m_ScrollDelta = 0.0f;
    std::array<bool, static_cast<size_t>(MouseButton::Count)> m_MouseState{};
    std::array<bool, static_cast<size_t>(MouseButton::Count)> m_MousePressed{};
    std::array<bool, static_cast<size_t>(MouseButton::Count)> m_MouseReleased{};
    bool m_MouseCaptured = false;

    // Callbacks
    KeyCallback m_KeyCallback;
    MouseCallback m_MouseCallback;
    ScrollCallback m_ScrollCallback;

    // Helper to convert SDL scancode to Key
    static Key SDLScancodeToKey(int scancode);
};

} // namespace Yalaz::Platform
