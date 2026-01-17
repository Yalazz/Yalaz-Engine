#pragma once

#include <glm/glm.hpp>
#include <SDL_events.h>

class Camera {
public:
    glm::vec3 position{ 0.0f };
    glm::vec3 velocity{ 0.0f };
    float pitch{ 0.0f };
    float yaw{ 0.0f };

    float currentSpeed{ 0.05f };
    bool sprinting{ false };
    float fov{ 60.0f };
    float targetFov{ 60.0f };

    float aspectRatio{ 16.0f / 9.0f };
    float nearPlane{ 0.1f };
    float farPlane{ 100.0f };

    glm::mat4 projectionMatrix{ 1.0f };


    glm::vec3 targetPosition{ 0.0f };
    float targetYaw{ 0.0f };
    float targetPitch{ 0.0f };
    bool focusActive = false;
    float focusDistance = 5.0f;  // Hedefe olan ideal uzakl�k



    glm::mat4 getViewMatrix() const;
    glm::mat4 getRotationMatrix() const;
    glm::mat4 getProjectionMatrix() const { return projectionMatrix; }
    glm::vec3 getLookDirection() const;

    void focusOnPoint(const glm::vec3& target, float distance = 5.0f);

    void processSDLEvent(SDL_Event& e);
    void update();
    void updateFromMousePosition(int windowWidth, int windowHeight);
    void onWindowResized(int width, int height);
    void setAspectRatio(float aspect);
    float getAspectRatio() const { return aspectRatio; }
    void updateProjectionMatrix();
};
