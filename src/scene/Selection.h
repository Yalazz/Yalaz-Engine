#pragma once

#include "core/ISubsystem.h"
#include "vk_types.h"
#include <string>
#include <vector>
#include <functional>

namespace Yalaz::Scene {

/**
 * @brief Selection types for the editor
 */
enum class SelectionType {
    None,
    MeshNode,
    Primitive,
    Light
};

/**
 * @brief Manages object selection and raycasting
 *
 * Design Patterns:
 * - Singleton: Central selection state
 * - Observer: Notifies listeners of selection changes
 *
 * SOLID:
 * - Single Responsibility: Only manages selection
 */
class Selection : public Core::ISubsystem {
public:
    static Selection& Get() {
        static Selection instance;
        return instance;
    }

    // ISubsystem interface
    void OnInit() override;
    void OnShutdown() override;
    const char* GetName() const override { return "Selection"; }

    // =========================================================================
    // Selection State
    // =========================================================================

    /**
     * @brief Select a mesh node
     */
    void SelectNode(MeshNode* node, const std::string& name = "");

    /**
     * @brief Select a primitive by index
     */
    void SelectPrimitive(int index);

    /**
     * @brief Select a light by index
     */
    void SelectLight(int index);

    /**
     * @brief Clear selection
     */
    void ClearSelection();

    /**
     * @brief Check if anything is selected
     */
    bool HasSelection() const { return m_SelectionType != SelectionType::None; }

    /**
     * @brief Get selection type
     */
    SelectionType GetSelectionType() const { return m_SelectionType; }

    /**
     * @brief Get selected node (if type is MeshNode)
     */
    MeshNode* GetSelectedNode() const { return m_SelectedNode; }

    /**
     * @brief Get selected object name
     */
    const std::string& GetSelectedName() const { return m_SelectedName; }

    /**
     * @brief Get selected index (for primitives/lights)
     */
    int GetSelectedIndex() const { return m_SelectedIndex; }

    // =========================================================================
    // Raycasting
    // =========================================================================

    /**
     * @brief Perform mouse pick selection
     * @param mouseX Mouse X position in screen space
     * @param mouseY Mouse Y position in screen space
     * @param viewMatrix Camera view matrix
     * @param projMatrix Camera projection matrix
     * @param screenWidth Viewport width
     * @param screenHeight Viewport height
     */
    void SelectObjectUnderMouse(float mouseX, float mouseY,
                                const glm::mat4& viewMatrix,
                                const glm::mat4& projMatrix,
                                float screenWidth, float screenHeight);

    /**
     * @brief Compute ray from mouse position
     */
    void ComputeRayFromMouse(float mouseX, float mouseY,
                             const glm::mat4& viewMatrix,
                             const glm::mat4& projMatrix,
                             float screenWidth, float screenHeight,
                             glm::vec3& outOrigin, glm::vec3& outDirection);

    /**
     * @brief Raycast against scene objects
     * @return Hit node or nullptr
     */
    MeshNode* RaycastSceneObjects(const glm::vec3& rayOrigin,
                                  const glm::vec3& rayDir);

    // =========================================================================
    // Pickable Objects
    // =========================================================================

    /**
     * @brief Set pickable render objects for raycasting
     */
    void SetPickableObjects(std::vector<RenderObject>& objects) {
        m_PickableObjects = &objects;
    }

    // =========================================================================
    // Observer Pattern
    // =========================================================================

    using SelectionCallback = std::function<void(SelectionType, int)>;

    /**
     * @brief Register callback for selection changes
     */
    void OnSelectionChanged(SelectionCallback callback) {
        m_Callbacks.push_back(callback);
    }

private:
    Selection() = default;
    ~Selection() = default;

    void NotifyCallbacks();

    // Selection state
    SelectionType m_SelectionType = SelectionType::None;
    MeshNode* m_SelectedNode = nullptr;
    std::string m_SelectedName;
    int m_SelectedIndex = -1;

    // Pickable objects reference
    std::vector<RenderObject>* m_PickableObjects = nullptr;

    // Observer callbacks
    std::vector<SelectionCallback> m_Callbacks;
};

} // namespace Yalaz::Scene
