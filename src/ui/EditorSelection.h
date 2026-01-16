#pragma once
// =============================================================================
// YALAZ ENGINE - Editor Selection System
// =============================================================================
// Centralized selection state management
// Allows panels to communicate selection changes
// =============================================================================

#include <string>
#include <vector>
#include <functional>

namespace Yalaz::UI {

// Selection types
enum class SelectionType {
    None,
    Primitive,      // Static mesh primitive (cube, sphere, etc.)
    SceneNode,      // GLTF scene node
    Light           // Point light
};

class EditorSelection {
public:
    static EditorSelection& Get() {
        static EditorSelection instance;
        return instance;
    }

    // Selection callback type
    using SelectionCallback = std::function<void(SelectionType, int)>;

    // Set selection
    void Select(SelectionType type, int index, const std::string& name = "") {
        if (m_Type != type || m_Index != index) {
            m_Type = type;
            m_Index = index;
            m_Name = name;
            NotifyCallbacks();
        }
    }

    // Clear selection
    void ClearSelection() {
        if (m_Type != SelectionType::None) {
            m_Type = SelectionType::None;
            m_Index = -1;
            m_Name.clear();
            NotifyCallbacks();
        }
    }

    // Getters
    SelectionType GetType() const { return m_Type; }
    int GetIndex() const { return m_Index; }
    const std::string& GetName() const { return m_Name; }
    bool HasSelection() const { return m_Type != SelectionType::None && m_Index >= 0; }

    // Check specific selection types
    bool IsPrimitiveSelected() const { return m_Type == SelectionType::Primitive && m_Index >= 0; }
    bool IsSceneNodeSelected() const { return m_Type == SelectionType::SceneNode && m_Index >= 0; }
    bool IsLightSelected() const { return m_Type == SelectionType::Light && m_Index >= 0; }

    // Register callback for selection changes
    void OnSelectionChanged(SelectionCallback callback) {
        m_Callbacks.push_back(callback);
    }

    // Clear all callbacks
    void ClearCallbacks() {
        m_Callbacks.clear();
    }

private:
    EditorSelection() = default;
    ~EditorSelection() = default;
    EditorSelection(const EditorSelection&) = delete;
    EditorSelection& operator=(const EditorSelection&) = delete;

    void NotifyCallbacks() {
        for (auto& callback : m_Callbacks) {
            callback(m_Type, m_Index);
        }
    }

    SelectionType m_Type = SelectionType::None;
    int m_Index = -1;
    std::string m_Name;
    std::vector<SelectionCallback> m_Callbacks;
};

} // namespace Yalaz::UI
