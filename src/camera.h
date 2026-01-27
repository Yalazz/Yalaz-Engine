#pragma once

#include <glm/glm.hpp>
#include <SDL_events.h>

class Camera {
public:
    // --- Transform ---
    glm::vec3 position{ 0.0f, 0.0f, 5.0f };
    float pitch{ 0.0f };
    float yaw{ 0.0f };

    // --- Projection ---
    float fov{ 60.0f };
    float targetFov{ 60.0f };
    float aspectRatio{ 16.0f / 9.0f };
    float nearPlane{ 0.1f };
    float farPlane{ 1000.0f };
    glm::mat4 projectionMatrix{ 1.0f };

    // --- Movement tuning ---
    float moveSpeed{ 5.0f };
    float sprintMultiplier{ 3.0f };
    float mouseSensitivity{ 0.002f };
    float panSensitivity{ 0.005f };
    float scrollZoomSpeed{ 3.0f };
    float smoothing{ 10.0f };
    float fovSmoothSpeed{ 8.0f };

    // --- Focus / orbit ---
    glm::vec3 targetPosition{ 0.0f };
    float targetYaw{ 0.0f };
    float targetPitch{ 0.0f };
    bool focusActive{ false };
    float focusDistance{ 5.0f };
    float focusSmoothSpeed{ 6.0f };

    // --- Matrices ---
    glm::mat4 getViewMatrix() const;
    glm::mat4 getRotationMatrix() const;
    glm::mat4 getProjectionMatrix() const { return projectionMatrix; }
    glm::vec3 getLookDirection() const;
    glm::vec3 getRightDirection() const;
    glm::vec3 getUpDirection() const;

    // --- Focus ---
    void focusOnPoint(const glm::vec3& target, float distance = 5.0f);

    // --- Per-frame ---
    void processSDLEvent(SDL_Event& e);
    void update(float deltaTime);

    // --- Resize ---
    void updateFromMousePosition(int windowWidth, int windowHeight);
    void onWindowResized(int width, int height);
    void setAspectRatio(float aspect);
    float getAspectRatio() const { return aspectRatio; }
    void updateProjectionMatrix();

private:
    // Raw input state (binary, not smoothed)
    glm::vec3 inputDirection{ 0.0f };
    bool sprinting{ false };

    // Smoothed velocity (interpolated toward input)
    glm::vec3 smoothVelocity{ 0.0f };
};
