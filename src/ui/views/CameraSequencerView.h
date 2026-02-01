#pragma once
// =============================================================================
// YALAZ ENGINE - Camera Sequencer View
// =============================================================================
// Cinematics and camera track editing:
// - Camera timeline with keyframes
// - Multiple camera tracks
// - FOV and focus distance animation
// - Camera shake effects
// - Preview playback
// - Export/import sequences
// =============================================================================

#include "EditorView.h"
#include <glm/glm.hpp>
#include <vector>
#include <string>

namespace Yalaz::UI {

// Camera keyframe
struct CameraKeyframe {
    float time;
    glm::vec3 position;
    glm::vec3 target;
    float fov;
    float roll;
    int interpolation;  // 0=Linear, 1=Smooth, 2=Bezier
};

// Camera track
struct CameraTrack {
    std::string name;
    std::vector<CameraKeyframe> keyframes;
    bool isActive;
    bool isMuted;
    bool isLocked;
};

// Camera sequence
struct CameraSequence {
    std::string name;
    float duration;
    float frameRate;
    bool loop;
    std::vector<CameraTrack> tracks;
};

// Shake preset
struct ShakePreset {
    std::string name;
    float intensity;
    float frequency;
    float duration;
    bool decay;
};

class CameraSequencerView : public EditorView {
public:
    CameraSequencerView();
    ViewFlags GetFlags() const override;
    void OnInit(VulkanEngine* engine) override;
    void OnUpdate(float deltaTime) override;

    // Playback controls
    void Play();
    void Pause();
    void Stop();
    void SetTime(float time);

protected:
    void OnRenderToolbar() override;
    void OnRenderContent() override;
    void OnRenderStatusBar() override;

private:
    void ApplyCameraFromSequence();
    void RenderTimeline();
    void RenderTrackList();
    void RenderTimeRuler();
    void RenderKeyframeEditor();
    void RenderPreviewPanel();
    void RenderCameraProperties();
    void RenderShakePanel();
    void HandleTimelineInput();

    // Timeline helpers
    float TimeToPixel(float time);
    float PixelToTime(float pixel);

    // Sequence data
    CameraSequence m_Sequence;
    int m_SelectedTrack = -1;
    int m_SelectedKeyframe = -1;

    // Playback
    bool m_IsPlaying = false;
    float m_CurrentTime = 0.0f;
    float m_PlaybackSpeed = 1.0f;

    // Timeline settings
    float m_TimelineZoom = 1.0f;
    float m_TimelineScroll = 0.0f;
    float m_TrackHeight = 30.0f;
    ImVec2 m_TimelinePos;
    ImVec2 m_TimelineSize;

    // View settings
    bool m_ShowPreview = true;
    bool m_SnapToFrame = true;
    bool m_AutoKey = false;
    float m_PreviewPanelWidth = 200.0f;

    // Shake presets
    std::vector<ShakePreset> m_ShakePresets;
    int m_SelectedShake = 0;
    bool m_ShakeActive = false;
};

} // namespace Yalaz::UI
