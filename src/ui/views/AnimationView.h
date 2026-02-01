#pragma once
// =============================================================================
// YALAZ ENGINE - Animation View
// =============================================================================
// Professional animation editor with:
// - Timeline with keyframe editing
// - State machine visualization
// - Skeleton preview and bone selection
// - Animation playback controls
// =============================================================================

#include "EditorView.h"
#include <vector>
#include <string>
#include <array>

namespace Yalaz::UI {

// =============================================================================
// Animation Keyframe
// =============================================================================
struct AnimationKeyframe {
    float time = 0.0f;
    float value = 0.0f;

    enum class Interpolation {
        Step,
        Linear,
        Cubic
    };
    Interpolation interpolation = Interpolation::Linear;
};

// =============================================================================
// Animation Track
// =============================================================================
struct AnimationTrack {
    std::string name;
    std::string property;   // Position, Rotation, Scale
    int component = 0;      // x=0, y=1, z=2, w=3

    std::vector<AnimationKeyframe> keyframes;

    bool isMuted = false;
    bool isLocked = false;
};

// =============================================================================
// Animation Event
// =============================================================================
struct AnimationEvent {
    float time = 0.0f;
    std::string name;
    std::string parameter;
};

// =============================================================================
// Animation Clip
// =============================================================================
struct AnimationClip {
    std::string name;
    float duration = 1.0f;
    float frameRate = 30.0f;
    bool loop = true;

    std::vector<AnimationTrack> tracks;
    std::vector<AnimationEvent> events;
};

// =============================================================================
// Skeleton Bone
// =============================================================================
struct SkeletonBone {
    std::string name;
    int parentIndex = -1;

    std::array<float, 3> localPosition = {0, 0, 0};
    std::array<float, 4> localRotation = {0, 0, 0, 1};  // Quaternion
    std::array<float, 3> localScale = {1, 1, 1};

    bool isSelected = false;
};

// =============================================================================
// Animation State (for State Machine)
// =============================================================================
struct AnimationState {
    std::string name;
    std::string clipName;
    float speed = 1.0f;

    float positionX = 0.0f;
    float positionY = 0.0f;

    bool isDefault = false;
    bool isSelected = false;
};

// =============================================================================
// Animation Transition
// =============================================================================
struct AnimationTransition {
    int fromState = -1;
    int toState = -1;
    float duration = 0.25f;
    std::string condition;

    bool hasExitTime = true;
    float exitTime = 0.9f;
};

// =============================================================================
// Animation View Settings
// =============================================================================
struct AnimationViewSettings {
    float zoom = 1.0f;
    float previewSpeed = 1.0f;

    bool snapToFrame = true;
    bool showEvents = true;
    bool showSkeleton = true;
    bool showMesh = false;
    bool showBoneNames = false;
};

// =============================================================================
// Animation View
// =============================================================================
class AnimationView : public EditorView {
public:
    AnimationView();
    ~AnimationView() override = default;

    // View interface
    const char* GetDisplayName() const override { return "Animation"; }
    ViewCategory GetCategory() const override { return ViewCategory::Animation; }
    ViewFlags GetFlags() const override;

    // Lifecycle
    void OnInit(VulkanEngine* engine) override;
    void OnUpdate(float deltaTime) override;

    // Playback controls
    void Play();
    void Pause();
    void Stop();
    void SetTime(float time);

    // Data setters
    void SetClip(const AnimationClip& clip);
    void SetSkeleton(const std::vector<SkeletonBone>& bones);

    // Accessors
    float GetCurrentTime() const { return m_CurrentTime; }
    bool IsPlaying() const { return m_IsPlaying; }
    AnimationViewSettings& GetSettings() { return m_Settings; }

protected:
    void OnRenderToolbar() override;
    void OnRenderContent() override;
    void OnRenderStatusBar() override;

private:
    void SyncWithEngine();
    void RenderTimeline();
    void RenderTrackList();
    void RenderTimeRuler();
    void RenderKeyframeEditor();
    void RenderPreviewPanel();
    void RenderStateMachine();
    void HandleTimelineInput();

    float TimeToPixel(float time);
    float PixelToTime(float pixel);

    // Animation data
    AnimationClip m_Clip;
    std::vector<SkeletonBone> m_Skeleton;
    std::vector<AnimationState> m_States;
    std::vector<AnimationTransition> m_Transitions;

    // Playback state
    bool m_IsPlaying = false;
    float m_CurrentTime = 0.0f;

    // UI state
    AnimationViewSettings m_Settings;

    int m_SelectedTrackIndex = -1;
    int m_SelectedBoneIndex = -1;
    int m_SelectedStateIndex = -1;

    float m_TrackHeight = 24.0f;
    float m_PreviewPanelWidth = 250.0f;
    float m_TimelineStart = 0.0f;

    ImVec2 m_TimelinePos = {0, 0};
    ImVec2 m_TimelineSize = {0, 0};
};

} // namespace Yalaz::UI
