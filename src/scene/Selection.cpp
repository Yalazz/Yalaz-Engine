#include "Selection.h"
#include <fmt/core.h>
#include <glm/gtc/matrix_transform.hpp>

namespace Yalaz::Scene {

void Selection::OnInit() {
    fmt::print("[Selection] Initialized\n");
}

void Selection::OnShutdown() {
    fmt::print("[Selection] Shutdown\n");
    ClearSelection();
    m_Callbacks.clear();
}

void Selection::SelectNode(MeshNode* node, const std::string& name) {
    m_SelectionType = SelectionType::MeshNode;
    m_SelectedNode = node;
    m_SelectedName = name;
    m_SelectedIndex = -1;
    NotifyCallbacks();
}

void Selection::SelectPrimitive(int index) {
    m_SelectionType = SelectionType::Primitive;
    m_SelectedNode = nullptr;
    m_SelectedName = "Primitive_" + std::to_string(index);
    m_SelectedIndex = index;
    NotifyCallbacks();
}

void Selection::SelectLight(int index) {
    m_SelectionType = SelectionType::Light;
    m_SelectedNode = nullptr;
    m_SelectedName = "Light_" + std::to_string(index);
    m_SelectedIndex = index;
    NotifyCallbacks();
}

void Selection::ClearSelection() {
    m_SelectionType = SelectionType::None;
    m_SelectedNode = nullptr;
    m_SelectedName.clear();
    m_SelectedIndex = -1;
    NotifyCallbacks();
}

void Selection::NotifyCallbacks() {
    for (auto& callback : m_Callbacks) {
        callback(m_SelectionType, m_SelectedIndex);
    }
}

void Selection::SelectObjectUnderMouse(float mouseX, float mouseY,
                                       const glm::mat4& viewMatrix,
                                       const glm::mat4& projMatrix,
                                       float screenWidth, float screenHeight) {
    glm::vec3 rayOrigin, rayDir;
    ComputeRayFromMouse(mouseX, mouseY, viewMatrix, projMatrix, screenWidth, screenHeight, rayOrigin, rayDir);

    MeshNode* hit = RaycastSceneObjects(rayOrigin, rayDir);
    if (hit) {
        SelectNode(hit, "");  // Name will be set by caller if known
    } else {
        ClearSelection();
    }
}

void Selection::ComputeRayFromMouse(float mouseX, float mouseY,
                                    const glm::mat4& viewMatrix,
                                    const glm::mat4& projMatrix,
                                    float screenWidth, float screenHeight,
                                    glm::vec3& outOrigin, glm::vec3& outDirection) {
    // Convert screen coordinates to NDC
    float x = (2.0f * mouseX) / screenWidth - 1.0f;
    float y = 1.0f - (2.0f * mouseY) / screenHeight;

    // Create ray in clip space
    glm::vec4 rayClip(x, y, -1.0f, 1.0f);

    // Transform to eye space
    glm::mat4 invProj = glm::inverse(projMatrix);
    glm::vec4 rayEye = invProj * rayClip;
    rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);

    // Transform to world space
    glm::mat4 invView = glm::inverse(viewMatrix);
    glm::vec4 rayWorld = invView * rayEye;
    outDirection = glm::normalize(glm::vec3(rayWorld));

    // Camera position is the ray origin
    outOrigin = glm::vec3(invView[3]);
}

MeshNode* Selection::RaycastSceneObjects(const glm::vec3& rayOrigin, const glm::vec3& rayDir) {
    // During migration, raycasting is handled by VulkanEngine
    // This will be populated when raycast_scene_objects is migrated
    (void)rayOrigin;
    (void)rayDir;
    return nullptr;
}

} // namespace Yalaz::Scene
