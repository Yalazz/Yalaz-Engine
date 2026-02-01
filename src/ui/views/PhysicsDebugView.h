#pragma once
// =============================================================================
// YALAZ ENGINE - Physics Debug View
// =============================================================================
// Physics and collision visualization tools:
// - Collider shape display (Box, Sphere, Capsule, Mesh)
// - Broadphase debug visualization
// - Raycast visualization
// - Contact point display
// - Physics simulation controls
// - Rigid body statistics
// =============================================================================

#include "EditorView.h"
#include <glm/glm.hpp>
#include <vector>
#include <string>

namespace Yalaz::UI {

// Demo collider data
struct ColliderInfo {
    std::string name;
    int type;  // 0=Box, 1=Sphere, 2=Capsule, 3=Mesh
    glm::vec3 position;
    glm::vec3 size;
    bool isStatic;
    bool isTrigger;
    bool isColliding;
};

// Demo raycast result
struct RaycastResult {
    glm::vec3 origin;
    glm::vec3 direction;
    float distance;
    bool hit;
    glm::vec3 hitPoint;
    glm::vec3 hitNormal;
    std::string hitObject;
};

class PhysicsDebugView : public EditorView {
public:
    PhysicsDebugView();
    ViewFlags GetFlags() const override;
    void OnInit(VulkanEngine* engine) override;
    void OnUpdate(float deltaTime) override;

protected:
    void OnRenderToolbar() override;
    void OnRenderContent() override;

private:
    void SyncWithEngine();
    void RenderColliderVisualization();
    void RenderBroadphaseDebug();
    void RenderRaycastDebug();
    void RenderContactPoints();
    void RenderSimulationControls();
    void RenderStatistics();
    void RenderColliderList();
    void RenderColliderDetails();

    // Debug modes
    int m_DebugMode = 0;  // 0=Colliders, 1=Broadphase, 2=Raycasts, 3=Contacts
    bool m_ShowColliders = true;
    bool m_ShowContacts = true;
    bool m_ShowRaycasts = true;
    bool m_ShowBroadphase = false;
    bool m_WireframeMode = true;

    // Visualization settings
    float m_ColliderAlpha = 0.5f;
    bool m_ShowSleeping = true;
    bool m_ShowStatic = true;
    bool m_ShowDynamic = true;
    bool m_ShowTriggers = true;

    // Simulation state
    bool m_SimulationPaused = false;
    float m_TimeScale = 1.0f;
    float m_FixedTimestep = 1.0f / 60.0f;
    int m_SubSteps = 4;

    // Demo data
    std::vector<ColliderInfo> m_Colliders;
    std::vector<RaycastResult> m_Raycasts;
    int m_SelectedCollider = -1;

    // Stats
    int m_ActiveBodies = 0;
    int m_SleepingBodies = 0;
    int m_ContactPairs = 0;
    float m_SimulationTime = 0.0f;
};

} // namespace Yalaz::UI
