#include "Input.h"
#include <SDL.h>
#include <fmt/core.h>

namespace Yalaz::Platform {

void Input::OnInit() {
    fmt::print("[Input] Initialized\n");
}

void Input::OnShutdown() {
    fmt::print("[Input] Shutdown\n");
    if (m_MouseCaptured) {
        SetMouseCapture(false);
    }
}

void Input::OnUpdate(float deltaTime) {
    (void)deltaTime;
    // Input state is updated via ProcessEvent
}

void Input::EndFrame() {
    // Clear per-frame state
    m_KeyPressed.fill(false);
    m_KeyReleased.fill(false);
    m_MousePressed.fill(false);
    m_MouseReleased.fill(false);
    m_MouseDelta = glm::vec2(0.0f);
    m_ScrollDelta = 0.0f;
}

bool Input::ProcessEvent(const SDL_Event& event) {
    switch (event.type) {
        case SDL_KEYDOWN: {
            Key key = SDLScancodeToKey(event.key.keysym.scancode);
            if (key != Key::Unknown && !m_KeyState[static_cast<size_t>(key)]) {
                m_KeyState[static_cast<size_t>(key)] = true;
                m_KeyPressed[static_cast<size_t>(key)] = true;
                if (m_KeyCallback) m_KeyCallback(key, true);
            }
            return true;
        }

        case SDL_KEYUP: {
            Key key = SDLScancodeToKey(event.key.keysym.scancode);
            if (key != Key::Unknown) {
                m_KeyState[static_cast<size_t>(key)] = false;
                m_KeyReleased[static_cast<size_t>(key)] = true;
                if (m_KeyCallback) m_KeyCallback(key, false);
            }
            return true;
        }

        case SDL_MOUSEMOTION: {
            glm::vec2 newPos(static_cast<float>(event.motion.x), static_cast<float>(event.motion.y));
            m_MouseDelta = glm::vec2(static_cast<float>(event.motion.xrel), static_cast<float>(event.motion.yrel));
            m_MousePosition = newPos;
            return true;
        }

        case SDL_MOUSEBUTTONDOWN: {
            MouseButton button = static_cast<MouseButton>(event.button.button - 1);
            if (button < MouseButton::Count) {
                m_MouseState[static_cast<size_t>(button)] = true;
                m_MousePressed[static_cast<size_t>(button)] = true;
                if (m_MouseCallback) m_MouseCallback(button, true);
            }
            return true;
        }

        case SDL_MOUSEBUTTONUP: {
            MouseButton button = static_cast<MouseButton>(event.button.button - 1);
            if (button < MouseButton::Count) {
                m_MouseState[static_cast<size_t>(button)] = false;
                m_MouseReleased[static_cast<size_t>(button)] = true;
                if (m_MouseCallback) m_MouseCallback(button, false);
            }
            return true;
        }

        case SDL_MOUSEWHEEL: {
            m_ScrollDelta = static_cast<float>(event.wheel.y);
            if (m_ScrollCallback) m_ScrollCallback(m_ScrollDelta);
            return true;
        }

        default:
            return false;
    }
}

bool Input::IsKeyDown(Key key) const {
    return m_KeyState[static_cast<size_t>(key)];
}

bool Input::IsKeyPressed(Key key) const {
    return m_KeyPressed[static_cast<size_t>(key)];
}

bool Input::IsKeyReleased(Key key) const {
    return m_KeyReleased[static_cast<size_t>(key)];
}

bool Input::IsShiftDown() const {
    return IsKeyDown(Key::LShift) || IsKeyDown(Key::RShift);
}

bool Input::IsCtrlDown() const {
    return IsKeyDown(Key::LCtrl) || IsKeyDown(Key::RCtrl);
}

bool Input::IsAltDown() const {
    return IsKeyDown(Key::LAlt) || IsKeyDown(Key::RAlt);
}

bool Input::IsMouseButtonDown(MouseButton button) const {
    return m_MouseState[static_cast<size_t>(button)];
}

bool Input::IsMouseButtonPressed(MouseButton button) const {
    return m_MousePressed[static_cast<size_t>(button)];
}

bool Input::IsMouseButtonReleased(MouseButton button) const {
    return m_MouseReleased[static_cast<size_t>(button)];
}

void Input::SetMouseCapture(bool capture) {
    m_MouseCaptured = capture;
    SDL_SetRelativeMouseMode(capture ? SDL_TRUE : SDL_FALSE);
}

Key Input::SDLScancodeToKey(int scancode) {
    switch (scancode) {
        case SDL_SCANCODE_A: return Key::A;
        case SDL_SCANCODE_B: return Key::B;
        case SDL_SCANCODE_C: return Key::C;
        case SDL_SCANCODE_D: return Key::D;
        case SDL_SCANCODE_E: return Key::E;
        case SDL_SCANCODE_F: return Key::F;
        case SDL_SCANCODE_G: return Key::G;
        case SDL_SCANCODE_H: return Key::H;
        case SDL_SCANCODE_I: return Key::I;
        case SDL_SCANCODE_J: return Key::J;
        case SDL_SCANCODE_K: return Key::K;
        case SDL_SCANCODE_L: return Key::L;
        case SDL_SCANCODE_M: return Key::M;
        case SDL_SCANCODE_N: return Key::N;
        case SDL_SCANCODE_O: return Key::O;
        case SDL_SCANCODE_P: return Key::P;
        case SDL_SCANCODE_Q: return Key::Q;
        case SDL_SCANCODE_R: return Key::R;
        case SDL_SCANCODE_S: return Key::S;
        case SDL_SCANCODE_T: return Key::T;
        case SDL_SCANCODE_U: return Key::U;
        case SDL_SCANCODE_V: return Key::V;
        case SDL_SCANCODE_W: return Key::W;
        case SDL_SCANCODE_X: return Key::X;
        case SDL_SCANCODE_Y: return Key::Y;
        case SDL_SCANCODE_Z: return Key::Z;
        case SDL_SCANCODE_0: return Key::Num0;
        case SDL_SCANCODE_1: return Key::Num1;
        case SDL_SCANCODE_2: return Key::Num2;
        case SDL_SCANCODE_3: return Key::Num3;
        case SDL_SCANCODE_4: return Key::Num4;
        case SDL_SCANCODE_5: return Key::Num5;
        case SDL_SCANCODE_6: return Key::Num6;
        case SDL_SCANCODE_7: return Key::Num7;
        case SDL_SCANCODE_8: return Key::Num8;
        case SDL_SCANCODE_9: return Key::Num9;
        case SDL_SCANCODE_SPACE: return Key::Space;
        case SDL_SCANCODE_RETURN: return Key::Enter;
        case SDL_SCANCODE_ESCAPE: return Key::Escape;
        case SDL_SCANCODE_TAB: return Key::Tab;
        case SDL_SCANCODE_BACKSPACE: return Key::Backspace;
        case SDL_SCANCODE_LEFT: return Key::Left;
        case SDL_SCANCODE_RIGHT: return Key::Right;
        case SDL_SCANCODE_UP: return Key::Up;
        case SDL_SCANCODE_DOWN: return Key::Down;
        case SDL_SCANCODE_LSHIFT: return Key::LShift;
        case SDL_SCANCODE_RSHIFT: return Key::RShift;
        case SDL_SCANCODE_LCTRL: return Key::LCtrl;
        case SDL_SCANCODE_RCTRL: return Key::RCtrl;
        case SDL_SCANCODE_LALT: return Key::LAlt;
        case SDL_SCANCODE_RALT: return Key::RAlt;
        case SDL_SCANCODE_F1: return Key::F1;
        case SDL_SCANCODE_F2: return Key::F2;
        case SDL_SCANCODE_F3: return Key::F3;
        case SDL_SCANCODE_F4: return Key::F4;
        case SDL_SCANCODE_F5: return Key::F5;
        case SDL_SCANCODE_F6: return Key::F6;
        case SDL_SCANCODE_F7: return Key::F7;
        case SDL_SCANCODE_F8: return Key::F8;
        case SDL_SCANCODE_F9: return Key::F9;
        case SDL_SCANCODE_F10: return Key::F10;
        case SDL_SCANCODE_F11: return Key::F11;
        case SDL_SCANCODE_F12: return Key::F12;
        case SDL_SCANCODE_DELETE: return Key::Delete;
        case SDL_SCANCODE_HOME: return Key::Home;
        case SDL_SCANCODE_END: return Key::End;
        case SDL_SCANCODE_PAGEUP: return Key::PageUp;
        case SDL_SCANCODE_PAGEDOWN: return Key::PageDown;
        default: return Key::Unknown;
    }
}

} // namespace Yalaz::Platform
