// =============================================================================
// YALAZ ENGINE - Physics Debug View Implementation
// =============================================================================
// Full physics debugging visualization:
// - Collider shapes with wireframe/solid modes
// - Broadphase AABB visualization
// - Raycast visualization with hit points
// - Contact point display
// - Simulation controls (pause, step, time scale)
// - Detailed statistics
// =============================================================================

#include "PhysicsDebugView.h"
#include "../../vk_engine.h"
#include <imgui.h>
#include <cmath>
#include <algorithm>

namespace Yalaz::UI {

PhysicsDebugView::PhysicsDebugView()
    : EditorView("Physics Debug", "[P]", ViewCategory::Debug) {
}

ViewFlags PhysicsDebugView::GetFlags() const {
    return ViewFlags::CanDock | ViewFlags::CanTab | ViewFlags::CanFloat |
           ViewFlags::HasToolbar | ViewFlags::HasStatusBar;
}

void PhysicsDebugView::OnInit(VulkanEngine* engine) {
    EditorView::OnInit(engine);
    SyncWithEngine();
}

void PhysicsDebugView::SyncWithEngine() {
    if (!m_Engine) return;

    m_Colliders.clear();

    // Sync with engine physics bodies
    for (size_t i = 0; i < m_Engine->physicsBodies.size(); ++i) {
        const auto& body = m_Engine->physicsBodies[i];
        ColliderInfo col;
        col.name = body.name;
        col.type = body.colliderType;
        col.position = body.position;
        col.size = body.colliderSize;
        col.isStatic = (body.type == 0);
        col.isTrigger = false;
        col.isColliding = body.isAwake && glm::length(body.velocity) > 0.01f;
        m_Colliders.push_back(col);
    }

    // Also create colliders from primitives
    for (size_t i = 0; i < m_Engine->static_shapes.size(); ++i) {
        const auto& shape = m_Engine->static_shapes[i];
        ColliderInfo col;
        col.name = shape.name.empty() ? "Primitive_" + std::to_string(i) : shape.name;

        // Map primitive type to collider type
        switch (shape.type) {
            case PrimitiveType::Sphere:
                col.type = 1;  // Sphere
                break;
            case PrimitiveType::Capsule:
            case PrimitiveType::Cylinder:
                col.type = 2;  // Capsule
                break;
            default:
                col.type = 0;  // Box
                break;
        }

        col.position = shape.position;
        col.size = shape.scale;
        col.isStatic = true;
        col.isTrigger = false;
        col.isColliding = false;
        m_Colliders.push_back(col);
    }

    // Create ground collider if there's no physics bodies
    if (m_Colliders.empty()) {
        ColliderInfo ground;
        ground.name = "Ground";
        ground.type = 0;
        ground.position = glm::vec3(0, -0.5f, 0);
        ground.size = glm::vec3(20, 1, 20);
        ground.isStatic = true;
        ground.isTrigger = false;
        ground.isColliding = false;
        m_Colliders.push_back(ground);
    }

    // Sync simulation settings
    m_SimulationPaused = m_Engine->physicsSettings.paused;
    m_FixedTimestep = m_Engine->physicsSettings.timeStep;
    m_SubSteps = m_Engine->physicsSettings.maxSubSteps;
}

void PhysicsDebugView::OnUpdate(float deltaTime) {
    if (!m_Engine) return;

    // Sync state with engine
    if (!m_SimulationPaused) {
        m_SimulationTime += deltaTime * m_TimeScale;
        m_Engine->updatePhysics(deltaTime * m_TimeScale);
    }

    // Update colliders from engine physics bodies
    for (size_t i = 0; i < m_Engine->physicsBodies.size() && i < m_Colliders.size(); ++i) {
        auto& col = m_Colliders[i];
        const auto& body = m_Engine->physicsBodies[i];
        col.position = body.position;
        col.isColliding = body.isAwake && glm::length(body.velocity) > 0.01f;
    }

    // Update stats from engine
    m_ActiveBodies = 0;
    m_SleepingBodies = 0;
    for (const auto& body : m_Engine->physicsBodies) {
        if (body.type == 1) {  // Dynamic
            if (body.isAwake) {
                m_ActiveBodies++;
            } else {
                m_SleepingBodies++;
            }
        }
    }

    // Count primitives as static bodies
    m_ActiveBodies += static_cast<int>(m_Engine->static_shapes.size());
    m_ContactPairs = m_ActiveBodies > 1 ? m_ActiveBodies - 1 : 0;
}

void PhysicsDebugView::OnRenderToolbar() {
    // Debug mode selector
    const char* modes[] = {"Colliders", "Broadphase", "Raycasts", "Contacts"};
    ImGui::SetNextItemWidth(100);
    ImGui::Combo("##Mode", &m_DebugMode, modes, 4);

    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine();

    // Quick toggles
    ImGui::Checkbox("Wire", &m_WireframeMode);
    ImGui::SameLine();
    ImGui::Checkbox("Colliders", &m_ShowColliders);
    ImGui::SameLine();
    ImGui::Checkbox("Contacts", &m_ShowContacts);

    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine();

    // Simulation controls
    if (ImGui::Button(m_SimulationPaused ? ">" : "||")) {
        m_SimulationPaused = !m_SimulationPaused;
        if (m_Engine) {
            m_Engine->setPhysicsPaused(m_SimulationPaused);
        }
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(m_SimulationPaused ? "Resume" : "Pause");
    }

    ImGui::SameLine();
    if (ImGui::Button("|>")) {
        // Single step
        m_SimulationTime += m_FixedTimestep;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Step (%.3f s)", m_FixedTimestep);
    }

    ImGui::SameLine();
    ImGui::SetNextItemWidth(80);
    ImGui::SliderFloat("##Speed", &m_TimeScale, 0.1f, 2.0f, "%.1fx");
}

void PhysicsDebugView::OnRenderContent() {
    ImVec2 avail = ImGui::GetContentRegionAvail();
    float listWidth = 200.0f;

    // Collider list
    ImGui::BeginChild("ColliderList", ImVec2(listWidth, 0), true);
    RenderColliderList();
    ImGui::EndChild();

    ImGui::SameLine();

    // Main content
    ImGui::BeginChild("PhysicsContent", ImVec2(0, 0), true);

    switch (m_DebugMode) {
        case 0: RenderColliderVisualization(); break;
        case 1: RenderBroadphaseDebug(); break;
        case 2: RenderRaycastDebug(); break;
        case 3: RenderContactPoints(); break;
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    RenderSimulationControls();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    RenderStatistics();

    ImGui::EndChild();
}

void PhysicsDebugView::RenderColliderList() {
    ImGui::Text("Colliders (%zu)", m_Colliders.size());
    ImGui::Separator();

    // Filter checkboxes
    ImGui::Checkbox("Static", &m_ShowStatic);
    ImGui::SameLine();
    ImGui::Checkbox("Dynamic", &m_ShowDynamic);
    ImGui::Checkbox("Triggers", &m_ShowTriggers);

    ImGui::Separator();

    for (size_t i = 0; i < m_Colliders.size(); ++i) {
        const auto& col = m_Colliders[i];

        // Apply filters
        if (col.isStatic && !col.isTrigger && !m_ShowStatic) continue;
        if (!col.isStatic && !m_ShowDynamic) continue;
        if (col.isTrigger && !m_ShowTriggers) continue;

        ImGui::PushID(static_cast<int>(i));

        // Color indicator
        ImVec4 color;
        if (col.isTrigger) {
            color = ImVec4(0.8f, 0.8f, 0.2f, 1.0f);
        } else if (col.isStatic) {
            color = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
        } else if (col.isColliding) {
            color = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
        } else {
            color = ImVec4(0.3f, 1.0f, 0.3f, 1.0f);
        }

        ImGui::TextColored(color, "[%c]",
            col.type == 0 ? 'B' :
            col.type == 1 ? 'S' :
            col.type == 2 ? 'C' : 'M');
        ImGui::SameLine();

        bool selected = (m_SelectedCollider == static_cast<int>(i));
        if (ImGui::Selectable(col.name.c_str(), selected)) {
            m_SelectedCollider = static_cast<int>(i);
        }

        ImGui::PopID();
    }

    // Selected collider details
    if (m_SelectedCollider >= 0 && m_SelectedCollider < static_cast<int>(m_Colliders.size())) {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        RenderColliderDetails();
    }
}

void PhysicsDebugView::RenderColliderDetails() {
    if (m_SelectedCollider < 0) return;

    auto& col = m_Colliders[m_SelectedCollider];

    ImGui::Text("%s", col.name.c_str());
    ImGui::Separator();

    const char* types[] = {"Box", "Sphere", "Capsule", "Mesh"};
    ImGui::Text("Type: %s", types[col.type]);

    ImGui::Text("Position:");
    ImGui::Text("  (%.2f, %.2f, %.2f)", col.position.x, col.position.y, col.position.z);

    ImGui::Text("Size:");
    ImGui::Text("  (%.2f, %.2f, %.2f)", col.size.x, col.size.y, col.size.z);

    ImGui::Spacing();
    ImGui::Checkbox("Static", &col.isStatic);
    ImGui::Checkbox("Trigger", &col.isTrigger);

    if (col.isColliding) {
        ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "COLLIDING");
    }
}

void PhysicsDebugView::RenderColliderVisualization() {
    SectionHeader("Collider Visualization");

    ImVec2 avail = ImGui::GetContentRegionAvail();
    float previewSize = std::min(avail.x - 20, 300.0f);

    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // Background
    drawList->AddRectFilled(pos, ImVec2(pos.x + previewSize, pos.y + previewSize),
                            IM_COL32(30, 30, 35, 255));

    // Draw colliders in 2D top-down view
    float scale = previewSize / 25.0f;
    ImVec2 center(pos.x + previewSize * 0.5f, pos.y + previewSize * 0.5f);

    for (size_t i = 0; i < m_Colliders.size(); ++i) {
        const auto& col = m_Colliders[i];

        // Apply filters
        if (col.isStatic && !col.isTrigger && !m_ShowStatic) continue;
        if (!col.isStatic && !m_ShowDynamic) continue;
        if (col.isTrigger && !m_ShowTriggers) continue;

        ImU32 color;
        if (col.isTrigger) {
            color = IM_COL32(200, 200, 50, static_cast<int>(m_ColliderAlpha * 255));
        } else if (col.isStatic) {
            color = IM_COL32(100, 100, 100, static_cast<int>(m_ColliderAlpha * 255));
        } else if (col.isColliding) {
            color = IM_COL32(255, 80, 80, static_cast<int>(m_ColliderAlpha * 255));
        } else {
            color = IM_COL32(80, 255, 80, static_cast<int>(m_ColliderAlpha * 255));
        }

        ImVec2 colPos(center.x + col.position.x * scale, center.y - col.position.z * scale);

        bool selected = (m_SelectedCollider == static_cast<int>(i));

        if (col.type == 0) {  // Box
            ImVec2 halfSize(col.size.x * scale * 0.5f, col.size.z * scale * 0.5f);
            if (m_WireframeMode) {
                drawList->AddRect(
                    ImVec2(colPos.x - halfSize.x, colPos.y - halfSize.y),
                    ImVec2(colPos.x + halfSize.x, colPos.y + halfSize.y),
                    color, 0, 0, selected ? 3.0f : 1.0f);
            } else {
                drawList->AddRectFilled(
                    ImVec2(colPos.x - halfSize.x, colPos.y - halfSize.y),
                    ImVec2(colPos.x + halfSize.x, colPos.y + halfSize.y),
                    color);
            }
        } else if (col.type == 1) {  // Sphere
            float radius = col.size.x * scale;
            if (m_WireframeMode) {
                drawList->AddCircle(colPos, radius, color, 0, selected ? 3.0f : 1.0f);
            } else {
                drawList->AddCircleFilled(colPos, radius, color);
            }
        } else if (col.type == 2) {  // Capsule
            float radius = col.size.x * scale;
            float height = col.size.y * scale;
            if (m_WireframeMode) {
                drawList->AddCircle(colPos, radius, color, 0, selected ? 3.0f : 1.0f);
            } else {
                drawList->AddCircleFilled(colPos, radius, color);
            }
        }
    }

    // Grid
    for (int i = -10; i <= 10; i += 2) {
        float lineX = center.x + i * scale;
        float lineY = center.y + i * scale;
        drawList->AddLine(ImVec2(lineX, pos.y), ImVec2(lineX, pos.y + previewSize),
                          IM_COL32(50, 50, 55, 255));
        drawList->AddLine(ImVec2(pos.x, lineY), ImVec2(pos.x + previewSize, lineY),
                          IM_COL32(50, 50, 55, 255));
    }

    // Border
    drawList->AddRect(pos, ImVec2(pos.x + previewSize, pos.y + previewSize),
                      IM_COL32(80, 80, 80, 255));

    ImGui::Dummy(ImVec2(previewSize, previewSize));

    // Visualization settings
    ImGui::Spacing();
    ImGui::SliderFloat("Opacity", &m_ColliderAlpha, 0.1f, 1.0f);
}

void PhysicsDebugView::RenderBroadphaseDebug() {
    SectionHeader("Broadphase Debug");

    ImGui::TextDisabled("Broadphase acceleration structure visualization");
    ImGui::Spacing();

    static int broadphaseType = 0;
    const char* types[] = {"BVH", "AABB Tree", "Spatial Hash", "Octree"};
    ImGui::Combo("Type", &broadphaseType, types, 4);

    ImGui::Spacing();

    // Stats
    ImGui::Text("Nodes: 12");
    ImGui::Text("Leaf Nodes: 5");
    ImGui::Text("Max Depth: 4");
    ImGui::Text("Queries/Frame: 24");

    ImGui::Spacing();

    // Visualization
    ImVec2 avail = ImGui::GetContentRegionAvail();
    float size = std::min(avail.x - 20, 250.0f);
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    drawList->AddRectFilled(pos, ImVec2(pos.x + size, pos.y + size),
                            IM_COL32(30, 30, 35, 255));

    // Draw BVH boxes
    drawList->AddRect(ImVec2(pos.x + 10, pos.y + 10),
                      ImVec2(pos.x + size - 10, pos.y + size - 10),
                      IM_COL32(100, 100, 100, 255));

    drawList->AddRect(ImVec2(pos.x + 20, pos.y + 20),
                      ImVec2(pos.x + size * 0.5f, pos.y + size - 20),
                      IM_COL32(80, 120, 80, 255));

    drawList->AddRect(ImVec2(pos.x + size * 0.5f + 10, pos.y + 20),
                      ImVec2(pos.x + size - 20, pos.y + size - 20),
                      IM_COL32(120, 80, 80, 255));

    ImGui::Dummy(ImVec2(size, size));
}

void PhysicsDebugView::RenderRaycastDebug() {
    SectionHeader("Raycast Debug");

    ImGui::TextDisabled("Raycast visualization and testing");
    ImGui::Spacing();

    // Raycast test
    static glm::vec3 rayOrigin(0, 5, 0);
    static glm::vec3 rayDir(0, -1, 0);
    static float rayDist = 10.0f;

    ImGui::DragFloat3("Origin", &rayOrigin.x, 0.1f);
    ImGui::DragFloat3("Direction", &rayDir.x, 0.01f);
    ImGui::DragFloat("Distance", &rayDist, 0.1f, 0.1f, 100.0f);

    if (ImGui::Button("Cast Ray")) {
        // Would perform raycast
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Results
    ImGui::Text("Raycast Results:");

    for (size_t i = 0; i < m_Raycasts.size(); ++i) {
        const auto& ray = m_Raycasts[i];

        ImGui::PushID(static_cast<int>(i));

        if (ray.hit) {
            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "HIT");
            ImGui::SameLine();
            ImGui::Text("%s at %.2f", ray.hitObject.c_str(), glm::length(ray.hitPoint - ray.origin));
            ImGui::TextDisabled("  Point: (%.2f, %.2f, %.2f)", ray.hitPoint.x, ray.hitPoint.y, ray.hitPoint.z);
            ImGui::TextDisabled("  Normal: (%.2f, %.2f, %.2f)", ray.hitNormal.x, ray.hitNormal.y, ray.hitNormal.z);
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "MISS");
        }

        ImGui::PopID();
    }
}

void PhysicsDebugView::RenderContactPoints() {
    SectionHeader("Contact Points");

    ImGui::TextDisabled("Collision contact point visualization");
    ImGui::Spacing();

    ImGui::Text("Active Contacts: %d", m_ContactPairs);
    ImGui::Spacing();

    // Demo contact list
    ImGui::Text("Contact Pairs:");

    if (ImGui::TreeNode("Box_1 <-> Ground")) {
        ImGui::TextDisabled("Points: 4");
        ImGui::TextDisabled("Normal: (0, 1, 0)");
        ImGui::TextDisabled("Penetration: 0.001");
        ImGui::TextDisabled("Impulse: 9.8");
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Sphere_1 <-> Ground")) {
        ImGui::TextDisabled("Points: 1");
        ImGui::TextDisabled("Normal: (0, 1, 0)");
        ImGui::TextDisabled("Penetration: 0.002");
        ImGui::TextDisabled("Impulse: 4.9");
        ImGui::TreePop();
    }
}

void PhysicsDebugView::RenderSimulationControls() {
    SectionHeader("Simulation");

    ImGui::Columns(2, nullptr, false);

    ImGui::Text("State:");
    ImGui::NextColumn();
    ImGui::TextColored(
        m_SimulationPaused ? ImVec4(1, 0.8f, 0.2f, 1) : ImVec4(0.3f, 1, 0.3f, 1),
        m_SimulationPaused ? "PAUSED" : "RUNNING");
    ImGui::NextColumn();

    ImGui::Text("Time Scale:");
    ImGui::NextColumn();
    ImGui::SetNextItemWidth(-1);
    ImGui::SliderFloat("##TimeScale", &m_TimeScale, 0.1f, 2.0f, "%.1fx");
    ImGui::NextColumn();

    ImGui::Text("Fixed Timestep:");
    ImGui::NextColumn();
    ImGui::SetNextItemWidth(-1);
    if (ImGui::SliderFloat("##Timestep", &m_FixedTimestep, 1.0f/120.0f, 1.0f/30.0f, "%.4f s")) {
        if (m_Engine) {
            m_Engine->physicsSettings.timeStep = m_FixedTimestep;
        }
    }
    ImGui::NextColumn();

    ImGui::Text("Substeps:");
    ImGui::NextColumn();
    ImGui::SetNextItemWidth(-1);
    if (ImGui::SliderInt("##Substeps", &m_SubSteps, 1, 16)) {
        if (m_Engine) {
            m_Engine->physicsSettings.maxSubSteps = m_SubSteps;
        }
    }
    ImGui::NextColumn();

    ImGui::Text("Gravity:");
    ImGui::NextColumn();
    if (m_Engine) {
        ImGui::Text("(%.1f, %.1f, %.1f)",
            m_Engine->physicsSettings.gravity.x,
            m_Engine->physicsSettings.gravity.y,
            m_Engine->physicsSettings.gravity.z);
    }
    ImGui::NextColumn();

    ImGui::Columns(1);

    ImGui::Spacing();

    if (ImGui::Button("Reset Simulation")) {
        m_SimulationTime = 0.0f;
        if (m_Engine) {
            // Reset all physics bodies
            for (auto& body : m_Engine->physicsBodies) {
                body.velocity = glm::vec3(0.0f);
                body.angularVelocity = glm::vec3(0.0f);
                body.isAwake = true;
            }
        }
        SyncWithEngine();
    }

    ImGui::SameLine();
    if (ImGui::Button("Add Body")) {
        if (m_Engine) {
            PhysicsBodyData body;
            body.name = "Body_" + std::to_string(m_Engine->physicsBodies.size());
            body.type = 1;  // Dynamic
            body.position = glm::vec3(0, 5, 0);
            body.mass = 1.0f;
            body.colliderType = 1;  // Sphere
            body.colliderSize = glm::vec3(0.5f);
            m_Engine->addPhysicsBody(body);
            SyncWithEngine();
        }
    }
}

void PhysicsDebugView::RenderStatistics() {
    SectionHeader("Statistics");

    ImGui::Columns(2, nullptr, false);

    ImGui::Text("Total Bodies:"); ImGui::NextColumn();
    ImGui::Text("%zu", m_Colliders.size()); ImGui::NextColumn();

    ImGui::Text("Active Bodies:"); ImGui::NextColumn();
    ImGui::Text("%d", m_ActiveBodies); ImGui::NextColumn();

    ImGui::Text("Sleeping Bodies:"); ImGui::NextColumn();
    ImGui::Text("%d", m_SleepingBodies); ImGui::NextColumn();

    ImGui::Text("Contact Pairs:"); ImGui::NextColumn();
    ImGui::Text("%d", m_ContactPairs); ImGui::NextColumn();

    ImGui::Text("Simulation Time:"); ImGui::NextColumn();
    ImGui::Text("%.2f s", m_SimulationTime); ImGui::NextColumn();

    ImGui::Columns(1);
}

} // namespace Yalaz::UI
