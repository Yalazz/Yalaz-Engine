#include "camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <algorithm>
#include <iostream>
#include "imgui.h"

constexpr float BASE_SPEED = 2.0f;
constexpr float SPRINT_SPEED = 4.0f;
constexpr float PITCH_LIMIT = glm::radians(89.0f);
constexpr float MIN_FOV = 20.0f;
constexpr float MAX_FOV = 90.0f;

static void clamp_pitch(float& pitch) {
    pitch = std::clamp(pitch, -PITCH_LIMIT, PITCH_LIMIT);
}

glm::vec3 Camera::getLookDirection() const {
    glm::mat4 rot = getRotationMatrix();
    return glm::vec3(rot * glm::vec4(0, 0, -1, 0)); // -Z ileri yön
}

glm::mat4 Camera::getViewMatrix() const {
    glm::vec3 lookDir = getLookDirection();
    glm::vec3 up = glm::vec3(0.f, 1.f, 0.f);
    return glm::lookAt(position, position + lookDir, up);
}

glm::mat4 Camera::getRotationMatrix() const {
    glm::quat pitchRotation = glm::angleAxis(pitch, glm::vec3{ 1.f, 0.f, 0.f });
    glm::quat yawRotation = glm::angleAxis(yaw, glm::vec3{ 0.f, -1.f, 0.f });
    return glm::toMat4(yawRotation) * glm::toMat4(pitchRotation);
}

void Camera::processSDLEvent(SDL_Event& e) {
    if (e.type == SDL_WINDOWEVENT) {
        if (e.window.event == SDL_WINDOWEVENT_RESIZED || e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
            onWindowResized(e.window.data1, e.window.data2);
        }
    }

    if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
        bool pressed = (e.type == SDL_KEYDOWN);
        switch (e.key.keysym.sym) {
        case SDLK_w: velocity.z = pressed ? -1.f : (velocity.z == -1.f ? 0.f : velocity.z); break;
        case SDLK_s: velocity.z = pressed ? 1.f : (velocity.z == 1.f ? 0.f : velocity.z); break;
        case SDLK_a: velocity.x = pressed ? -1.f : (velocity.x == -1.f ? 0.f : velocity.x); break;
        case SDLK_d: velocity.x = pressed ? 1.f : (velocity.x == 1.f ? 0.f : velocity.x); break;
        case SDLK_LSHIFT:
        case SDLK_RSHIFT: sprinting = pressed; break;
        }
    }

    if (e.type == SDL_MOUSEMOTION && !ImGui::GetIO().WantCaptureMouse) {
        yaw += static_cast<float>(e.motion.xrel) / 200.f;
        pitch -= static_cast<float>(e.motion.yrel) / 200.f;
        clamp_pitch(pitch);
    }

    if (e.type == SDL_MOUSEWHEEL && !ImGui::GetIO().WantCaptureMouse) {
        targetFov -= static_cast<float>(e.wheel.y) * 2.0f;
        targetFov = std::clamp(targetFov, MIN_FOV, MAX_FOV);
        std::cout << "[Zoom] targetFov: " << targetFov << "\n";
    }
}
void Camera::focusOnPoint(const glm::vec3& target, float distance)
{
    // Objeye bakış yönünü hesapla
    glm::vec3 direction = glm::normalize(target - position);

    // Kamerayı objeden geri çekilmiş pozisyona yerleştir
    targetPosition = target - direction * distance;

    // Kameranın bakış yönünü objeye çevir
    targetYaw = atan2(-direction.x, -direction.z);
    targetPitch = asin(direction.y);
    clamp_pitch(targetPitch);

    focusActive = true;
}








//void Camera::update() {
//    clamp_pitch(pitch);
//
//    constexpr float zoomSmooth = 0.15f;
//    fov += (targetFov - fov) * zoomSmooth;
//    updateProjectionMatrix();
//
//    float targetSpeed = sprinting ? SPRINT_SPEED : BASE_SPEED;
//    constexpr float smoothFactor = 0.15f;
//    currentSpeed += (targetSpeed - currentSpeed) * smoothFactor;
//
//    glm::mat4 rotation = getRotationMatrix();
//    glm::vec3 move = velocity * currentSpeed;
//    position += glm::vec3(rotation * glm::vec4(move, 0.f));
//
//
//
//
//    if (focusActive)
//    {
//        const float smoothFactor = 0.1f;
//        position = glm::mix(position, targetPosition, smoothFactor);
//
//        if (glm::distance(position, targetPosition) < 0.01f)
//        {
//            focusActive = false;  // Hedefe ulaştıktan sonra dur
//        }
//    }
//    else
//    {
//        glm::mat4 rotation = getRotationMatrix();
//        glm::vec3 move = velocity * currentSpeed;
//        position += glm::vec3(rotation * glm::vec4(move, 0.f));
//    }
//
//
//
//
//
//
//}

void Camera::update()
{
    clamp_pitch(pitch);

    // 🎯 Zoom kontrolü (her zaman bağımsız çalışır)
    constexpr float zoomSmooth = 0.15f;
    fov += (targetFov - fov) * zoomSmooth;
    updateProjectionMatrix();

    // 🎯 Hız kontrolü
    float targetSpeed = sprinting ? SPRINT_SPEED : BASE_SPEED;
    constexpr float speedSmooth = 0.15f;
    currentSpeed += (targetSpeed - currentSpeed) * speedSmooth;

    if (focusActive)
    {
        constexpr float posSmooth = 0.1f;
        constexpr float rotSmooth = 0.1f;

        // Pozisyonu hedefe doğru yumuşak ilerlet:
        position = glm::mix(position, targetPosition, posSmooth);

        // Bakış yönünü hedefe doğru smooth döndür:
        yaw += (targetYaw - yaw) * rotSmooth;
        pitch += (targetPitch - pitch) * rotSmooth;

        // 🚨 Manuel kontrol algılanırsa fokus iptal et:
        if (glm::length(velocity) > 0.001f || ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
            focusActive = false;
        }

        // 🚩 Hedefe ulaşıldıysa fokus iptal et:
        if (glm::distance(position, targetPosition) < 0.01f &&
            glm::abs(targetYaw - yaw) < 0.001f &&
            glm::abs(targetPitch - pitch) < 0.001f)
        {
            focusActive = false;
        }
    }
    else
    {
        // 🎮 Klasik FPS hareketi (fokus yokken aktif)
        glm::mat4 rotation = getRotationMatrix();
        glm::vec3 move = velocity * currentSpeed;
        position += glm::vec3(rotation * glm::vec4(move, 0.f));
    }
}









void Camera::onWindowResized(int width, int height) {
    setAspectRatio(static_cast<float>(width) / static_cast<float>(height));
    updateProjectionMatrix();
}

void Camera::setAspectRatio(float aspect) {
    aspectRatio = aspect;
}

void Camera::updateProjectionMatrix() {
    if (aspectRatio <= 0.0f)
        return;

    projectionMatrix = glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
}
