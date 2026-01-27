#include "camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <algorithm>
#include "imgui.h"

static constexpr float PITCH_LIMIT = glm::radians(89.0f);
static constexpr float MIN_FOV     = 10.0f;
static constexpr float MAX_FOV     = 120.0f;

// Framerate-independent exponential decay: lerp(a, b, 1 - exp(-speed * dt))
static float dampFactor(float speed, float dt) {
    return 1.0f - std::exp(-speed * dt);
}

// ================================================================
// Direction helpers
// ================================================================

glm::vec3 Camera::getLookDirection() const {
    glm::mat4 rot = getRotationMatrix();
    return glm::vec3(rot * glm::vec4(0, 0, -1, 0));
}

glm::vec3 Camera::getRightDirection() const {
    glm::mat4 rot = getRotationMatrix();
    return glm::vec3(rot * glm::vec4(1, 0, 0, 0));
}

glm::vec3 Camera::getUpDirection() const {
    glm::mat4 rot = getRotationMatrix();
    return glm::vec3(rot * glm::vec4(0, 1, 0, 0));
}

// ================================================================
// Matrices
// ================================================================

glm::mat4 Camera::getViewMatrix() const {
    glm::vec3 lookDir = getLookDirection();
    return glm::lookAt(position, position + lookDir, glm::vec3(0.f, 1.f, 0.f));
}

glm::mat4 Camera::getRotationMatrix() const {
    glm::quat pitchRot = glm::angleAxis(pitch, glm::vec3{ 1.f, 0.f, 0.f });
    glm::quat yawRot   = glm::angleAxis(yaw,   glm::vec3{ 0.f, -1.f, 0.f });
    return glm::toMat4(yawRot) * glm::toMat4(pitchRot);
}

// ================================================================
// Input
// ================================================================

void Camera::processSDLEvent(SDL_Event& e) {
    // Window resize
    if (e.type == SDL_WINDOWEVENT) {
        if (e.window.event == SDL_WINDOWEVENT_RESIZED ||
            e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
            onWindowResized(e.window.data1, e.window.data2);
        }
    }

    // Keyboard: WASD + QE + Shift
    if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
        if (ImGui::GetIO().WantTextInput) {
            inputDirection = glm::vec3(0.f);
            sprinting = false;
            return;
        }

        bool pressed = (e.type == SDL_KEYDOWN);
        switch (e.key.keysym.sym) {
        case SDLK_w: inputDirection.z = pressed ? -1.f : (inputDirection.z == -1.f ? 0.f : inputDirection.z); break;
        case SDLK_s: inputDirection.z = pressed ?  1.f : (inputDirection.z ==  1.f ? 0.f : inputDirection.z); break;
        case SDLK_a: inputDirection.x = pressed ? -1.f : (inputDirection.x == -1.f ? 0.f : inputDirection.x); break;
        case SDLK_d: inputDirection.x = pressed ?  1.f : (inputDirection.x ==  1.f ? 0.f : inputDirection.x); break;
        case SDLK_q: inputDirection.y = pressed ? -1.f : (inputDirection.y == -1.f ? 0.f : inputDirection.y); break;
        case SDLK_e: inputDirection.y = pressed ?  1.f : (inputDirection.y ==  1.f ? 0.f : inputDirection.y); break;
        case SDLK_LSHIFT:
        case SDLK_RSHIFT:
            sprinting = pressed;
            break;
        }
    }

    // Right-mouse-button look (only when RMB is held)
    if (e.type == SDL_MOUSEMOTION && !ImGui::GetIO().WantCaptureMouse) {
        Uint32 buttons = SDL_GetMouseState(nullptr, nullptr);
        if (buttons & SDL_BUTTON(SDL_BUTTON_RIGHT)) {
            yaw   += static_cast<float>(e.motion.xrel) * mouseSensitivity;
            pitch -= static_cast<float>(e.motion.yrel) * mouseSensitivity;
            pitch  = std::clamp(pitch, -PITCH_LIMIT, PITCH_LIMIT);
        }
        // Middle-mouse-button pan
        if (buttons & SDL_BUTTON(SDL_BUTTON_MIDDLE)) {
            glm::vec3 right = getRightDirection();
            glm::vec3 up    = getUpDirection();
            float dx = -static_cast<float>(e.motion.xrel) * panSensitivity;
            float dy =  static_cast<float>(e.motion.yrel) * panSensitivity;
            position += right * dx + up * dy;
        }
    }

    // Scroll wheel: zoom FOV, or move speed with Alt
    if (e.type == SDL_MOUSEWHEEL && !ImGui::GetIO().WantCaptureMouse) {
        const Uint8* keys = SDL_GetKeyboardState(nullptr);
        if (keys[SDL_SCANCODE_LALT] || keys[SDL_SCANCODE_RALT]) {
            // Alt+Scroll: adjust movement speed
            float factor = (e.wheel.y > 0) ? 1.2f : (1.0f / 1.2f);
            moveSpeed = std::clamp(moveSpeed * factor, 0.1f, 200.0f);
        } else {
            // Scroll: zoom via FOV
            targetFov -= static_cast<float>(e.wheel.y) * scrollZoomSpeed;
            targetFov = std::clamp(targetFov, MIN_FOV, MAX_FOV);
        }
    }
}

// ================================================================
// Focus on object
// ================================================================

void Camera::focusOnPoint(const glm::vec3& target, float distance) {
    glm::vec3 direction = glm::normalize(target - position);

    targetPosition = target - direction * distance;
    targetYaw   = std::atan2(-direction.x, -direction.z);
    targetPitch = std::asin(std::clamp(direction.y, -1.0f, 1.0f));
    targetPitch = std::clamp(targetPitch, -PITCH_LIMIT, PITCH_LIMIT);

    focusActive = true;
}

// ================================================================
// Update (delta-time based)
// ================================================================

void Camera::update(float deltaTime) {
    // Clamp deltaTime to avoid huge jumps (e.g. after breakpoint or stall)
    deltaTime = std::clamp(deltaTime, 0.0001f, 0.1f);

    // --- Smooth FOV ---
    float fovDamp = dampFactor(fovSmoothSpeed, deltaTime);
    fov += (targetFov - fov) * fovDamp;
    updateProjectionMatrix();

    if (focusActive) {
        // --- Focus / orbit mode ---
        float focusDamp = dampFactor(focusSmoothSpeed, deltaTime);

        position = glm::mix(position, targetPosition, focusDamp);
        yaw   += (targetYaw   - yaw)   * focusDamp;
        pitch += (targetPitch - pitch) * focusDamp;

        // Cancel focus on manual input
        if (glm::length(inputDirection) > 0.001f ||
            ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
            focusActive = false;
        }

        // Arrived at target
        if (glm::distance(position, targetPosition) < 0.01f &&
            std::abs(targetYaw - yaw) < 0.001f &&
            std::abs(targetPitch - pitch) < 0.001f) {
            focusActive = false;
        }
    } else {
        // --- Free movement ---
        float speed = moveSpeed * (sprinting ? sprintMultiplier : 1.0f);

        // Normalize input so diagonals aren't faster
        glm::vec3 desiredDir = inputDirection;
        float inputLen = glm::length(desiredDir);
        if (inputLen > 1.0f)
            desiredDir /= inputLen;

        glm::vec3 targetVelocity = desiredDir * speed;

        // Smooth velocity (framerate-independent)
        float moveDamp = dampFactor(smoothing, deltaTime);
        smoothVelocity = glm::mix(smoothVelocity, targetVelocity, moveDamp);

        // Apply movement in camera space
        glm::mat4 rotation = getRotationMatrix();
        glm::vec3 worldMove = glm::vec3(rotation * glm::vec4(smoothVelocity, 0.f));
        position += worldMove * deltaTime;
    }

    pitch = std::clamp(pitch, -PITCH_LIMIT, PITCH_LIMIT);
}

// ================================================================
// Resize
// ================================================================

void Camera::onWindowResized(int width, int height) {
    if (height > 0)
        setAspectRatio(static_cast<float>(width) / static_cast<float>(height));
    updateProjectionMatrix();
}

void Camera::setAspectRatio(float aspect) {
    aspectRatio = aspect;
}

void Camera::updateProjectionMatrix() {
    if (aspectRatio <= 0.0f) return;
    projectionMatrix = glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
}
