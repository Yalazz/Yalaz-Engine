// =============================================================================
// YALAZ ENGINE - Camera Sequencer View Implementation
// =============================================================================
// Full cinematics camera system:
// - Multi-track timeline with keyframes
// - Position, target, FOV, roll animation
// - Interpolation modes (Linear, Smooth, Bezier)
// - Camera shake effects with presets
// - Real-time preview
// - Playback controls
// =============================================================================

#include "CameraSequencerView.h"
#include "../../vk_engine.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <algorithm>
#include <cmath>

namespace Yalaz::UI {

CameraSequencerView::CameraSequencerView()
    : EditorView("Camera Sequencer", "[Q]", ViewCategory::Animation) {
}

ViewFlags CameraSequencerView::GetFlags() const {
    return ViewFlags::CanDock | ViewFlags::CanTab | ViewFlags::CanFloat |
           ViewFlags::HasToolbar | ViewFlags::HasStatusBar;
}

void CameraSequencerView::OnInit(VulkanEngine* engine) {
    EditorView::OnInit(engine);

    // Initialize sequence with current camera position
    m_Sequence.name = "Intro Sequence";
    m_Sequence.duration = 10.0f;
    m_Sequence.frameRate = 30.0f;
    m_Sequence.loop = false;

    // Get current camera position from engine
    glm::vec3 currentCamPos = glm::vec3(0, 2, 10);
    if (m_Engine) {
        currentCamPos = m_Engine->mainCamera.position;
    }

    // Main camera track
    CameraTrack mainTrack;
    mainTrack.name = "Main Camera";
    mainTrack.isActive = true;
    mainTrack.isMuted = false;
    mainTrack.isLocked = false;

    // First keyframe uses current camera position
    CameraKeyframe k1;
    k1.time = 0.0f;
    k1.position = currentCamPos;
    k1.target = glm::vec3(0, 0, 0);
    k1.fov = 60.0f;
    k1.roll = 0.0f;
    k1.interpolation = 1;
    mainTrack.keyframes.push_back(k1);

    CameraKeyframe k2;
    k2.time = 3.0f;
    k2.position = glm::vec3(5, 3, 8);
    k2.target = glm::vec3(0, 1, 0);
    k2.fov = 50.0f;
    k2.roll = 5.0f;
    k2.interpolation = 1;
    mainTrack.keyframes.push_back(k2);

    CameraKeyframe k3;
    k3.time = 6.0f;
    k3.position = glm::vec3(-5, 5, 5);
    k3.target = glm::vec3(0, 0, 0);
    k3.fov = 70.0f;
    k3.roll = -5.0f;
    k3.interpolation = 1;
    mainTrack.keyframes.push_back(k3);

    CameraKeyframe k4;
    k4.time = 10.0f;
    k4.position = glm::vec3(0, 2, 10);
    k4.target = glm::vec3(0, 0, 0);
    k4.fov = 60.0f;
    k4.roll = 0.0f;
    k4.interpolation = 1;
    mainTrack.keyframes.push_back(k4);

    m_Sequence.tracks.push_back(mainTrack);

    // Secondary track (disabled)
    CameraTrack closeupTrack;
    closeupTrack.name = "Closeup Cam";
    closeupTrack.isActive = false;
    closeupTrack.isMuted = true;
    closeupTrack.isLocked = false;

    CameraKeyframe ck1;
    ck1.time = 4.0f;
    ck1.position = glm::vec3(1, 1, 2);
    ck1.target = glm::vec3(0, 1, 0);
    ck1.fov = 40.0f;
    ck1.roll = 0.0f;
    ck1.interpolation = 2;
    closeupTrack.keyframes.push_back(ck1);

    CameraKeyframe ck2;
    ck2.time = 7.0f;
    ck2.position = glm::vec3(-1, 1, 2);
    ck2.target = glm::vec3(0, 1, 0);
    ck2.fov = 40.0f;
    ck2.roll = 0.0f;
    ck2.interpolation = 2;
    closeupTrack.keyframes.push_back(ck2);

    m_Sequence.tracks.push_back(closeupTrack);

    // Shake presets
    m_ShakePresets.clear();
    m_ShakePresets.push_back({"None", 0.0f, 0.0f, 0.0f, false});
    m_ShakePresets.push_back({"Subtle", 0.1f, 5.0f, 1.0f, true});
    m_ShakePresets.push_back({"Handheld", 0.3f, 3.0f, 0.0f, false});
    m_ShakePresets.push_back({"Impact", 0.8f, 15.0f, 0.5f, true});
    m_ShakePresets.push_back({"Explosion", 1.5f, 20.0f, 1.0f, true});
    m_ShakePresets.push_back({"Earthquake", 1.0f, 8.0f, 3.0f, true});
}

void CameraSequencerView::OnUpdate(float deltaTime) {
    if (m_IsPlaying) {
        m_CurrentTime += deltaTime * m_PlaybackSpeed;

        if (m_CurrentTime >= m_Sequence.duration) {
            if (m_Sequence.loop) {
                m_CurrentTime = std::fmod(m_CurrentTime, m_Sequence.duration);
            } else {
                m_CurrentTime = m_Sequence.duration;
                m_IsPlaying = false;
            }
        }

        // Apply camera position from active track
        ApplyCameraFromSequence();
    }
}

void CameraSequencerView::ApplyCameraFromSequence() {
    if (!m_Engine) return;

    // Find active track
    for (const auto& track : m_Sequence.tracks) {
        if (track.isActive && !track.isMuted && !track.keyframes.empty()) {
            // Find surrounding keyframes
            const CameraKeyframe* k1 = nullptr;
            const CameraKeyframe* k2 = nullptr;

            for (size_t i = 0; i < track.keyframes.size(); ++i) {
                if (track.keyframes[i].time <= m_CurrentTime) {
                    k1 = &track.keyframes[i];
                    if (i + 1 < track.keyframes.size()) {
                        k2 = &track.keyframes[i + 1];
                    }
                }
            }

            if (k1) {
                glm::vec3 position = k1->position;
                glm::vec3 target = k1->target;

                // Interpolate if we have two keyframes
                if (k2 && k2->time > k1->time) {
                    float t = (m_CurrentTime - k1->time) / (k2->time - k1->time);

                    // Apply interpolation based on type
                    if (k1->interpolation == 1) {
                        // Smooth (smoothstep)
                        t = t * t * (3.0f - 2.0f * t);
                    } else if (k1->interpolation == 2) {
                        // Bezier (smoother step)
                        t = t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
                    }

                    position = glm::mix(k1->position, k2->position, t);
                    target = glm::mix(k1->target, k2->target, t);
                }

                // Apply to engine camera
                m_Engine->mainCamera.position = position;

                // Calculate yaw and pitch from target
                glm::vec3 dir = glm::normalize(target - position);
                m_Engine->mainCamera.yaw = glm::degrees(atan2(dir.x, dir.z));
                m_Engine->mainCamera.pitch = glm::degrees(asin(-dir.y));
            }

            break;  // Only process first active track
        }
    }
}

void CameraSequencerView::Play() {
    m_IsPlaying = true;
}

void CameraSequencerView::Pause() {
    m_IsPlaying = false;
}

void CameraSequencerView::Stop() {
    m_IsPlaying = false;
    m_CurrentTime = 0.0f;
}

void CameraSequencerView::SetTime(float time) {
    m_CurrentTime = std::clamp(time, 0.0f, m_Sequence.duration);
}

void CameraSequencerView::OnRenderToolbar() {
    // Playback controls
    if (ImGui::Button(m_IsPlaying ? "||" : ">", ImVec2(30, 0))) {
        if (m_IsPlaying) Pause();
        else Play();
    }
    ImGui::SameLine();

    if (ImGui::Button("[]", ImVec2(30, 0))) {
        Stop();
    }
    ImGui::SameLine();

    if (ImGui::Button("|<", ImVec2(30, 0))) {
        SetTime(0.0f);
    }
    ImGui::SameLine();

    if (ImGui::Button(">|", ImVec2(30, 0))) {
        SetTime(m_Sequence.duration);
    }

    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine();

    // Time display
    int frame = static_cast<int>(m_CurrentTime * m_Sequence.frameRate);
    int totalFrames = static_cast<int>(m_Sequence.duration * m_Sequence.frameRate);
    ImGui::Text("%d / %d", frame, totalFrames);

    ImGui::SameLine();
    ImGui::SetNextItemWidth(120);
    float time = m_CurrentTime;
    if (ImGui::SliderFloat("##Time", &time, 0.0f, m_Sequence.duration, "%.2f s")) {
        SetTime(time);
    }

    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine();

    // Speed
    ImGui::Text("Speed:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(60);
    ImGui::SliderFloat("##Speed", &m_PlaybackSpeed, 0.1f, 2.0f, "%.1fx");

    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine();

    // Options
    ImGui::Checkbox("Snap", &m_SnapToFrame);
    ImGui::SameLine();
    ImGui::Checkbox("Auto Key", &m_AutoKey);
    ImGui::SameLine();
    ImGui::Checkbox("Loop", &m_Sequence.loop);
}

void CameraSequencerView::OnRenderContent() {
    ImGui::BeginTabBar("SequencerTabs");

    if (ImGui::BeginTabItem("Timeline")) {
        RenderTimeline();
        ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("Camera Properties")) {
        RenderCameraProperties();
        ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("Shake Effects")) {
        RenderShakePanel();
        ImGui::EndTabItem();
    }

    ImGui::EndTabBar();
}

void CameraSequencerView::RenderTimeline() {
    ImVec2 avail = ImGui::GetContentRegionAvail();

    // Track list (left)
    float trackListWidth = 150.0f;

    ImGui::BeginChild("TrackList", ImVec2(trackListWidth, 0), true);
    RenderTrackList();
    ImGui::EndChild();

    ImGui::SameLine();

    // Keyframe editor (center)
    ImGui::BeginChild("KeyframeEditor", ImVec2(avail.x - trackListWidth - m_PreviewPanelWidth - 20, 0), true,
                      ImGuiWindowFlags_HorizontalScrollbar);

    m_TimelinePos = ImGui::GetCursorScreenPos();
    m_TimelineSize = ImGui::GetContentRegionAvail();

    RenderTimeRuler();
    RenderKeyframeEditor();
    HandleTimelineInput();

    ImGui::EndChild();

    ImGui::SameLine();

    // Preview (right)
    if (m_ShowPreview) {
        ImGui::BeginChild("Preview", ImVec2(0, 0), true);
        RenderPreviewPanel();
        ImGui::EndChild();
    }
}

void CameraSequencerView::RenderTrackList() {
    ImGui::Text("Tracks");
    ImGui::Separator();

    for (size_t i = 0; i < m_Sequence.tracks.size(); ++i) {
        auto& track = m_Sequence.tracks[i];

        ImGui::PushID(static_cast<int>(i));

        // Active indicator
        ImVec4 color = track.isActive ?
            ImVec4(0.3f, 1.0f, 0.3f, 1.0f) : ImVec4(0.5f, 0.5f, 0.5f, 1.0f);

        if (track.isMuted) {
            color = ImVec4(0.5f, 0.5f, 0.5f, 0.5f);
        }

        ImGui::PushStyleColor(ImGuiCol_Text, color);

        bool selected = (m_SelectedTrack == static_cast<int>(i));
        if (ImGui::Selectable(track.name.c_str(), selected, 0, ImVec2(0, m_TrackHeight))) {
            m_SelectedTrack = static_cast<int>(i);
        }

        ImGui::PopStyleColor();

        // Context menu
        if (ImGui::BeginPopupContextItem()) {
            if (ImGui::MenuItem("Set Active", nullptr, track.isActive)) {
                // Deactivate others
                for (auto& t : m_Sequence.tracks) t.isActive = false;
                track.isActive = true;
            }
            ImGui::Checkbox("Mute", &track.isMuted);
            ImGui::Checkbox("Lock", &track.isLocked);
            ImGui::Separator();
            if (ImGui::MenuItem("Delete Track")) {
                // Would delete track
            }
            ImGui::EndPopup();
        }

        ImGui::PopID();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::Button("+ Add Track", ImVec2(-1, 0))) {
        CameraTrack newTrack;
        newTrack.name = "Camera " + std::to_string(m_Sequence.tracks.size() + 1);
        newTrack.isActive = false;
        newTrack.isMuted = false;
        newTrack.isLocked = false;
        m_Sequence.tracks.push_back(newTrack);
    }
}

void CameraSequencerView::RenderTimeRuler() {
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    float rulerHeight = 25.0f;
    ImVec2 rulerStart = m_TimelinePos;
    ImVec2 rulerEnd(m_TimelinePos.x + m_TimelineSize.x, m_TimelinePos.y + rulerHeight);

    // Background
    drawList->AddRectFilled(rulerStart, rulerEnd, IM_COL32(50, 50, 55, 255));

    // Time marks
    float pixelsPerSecond = 80.0f * m_TimelineZoom;
    float step = 1.0f;
    if (m_TimelineZoom < 0.5f) step = 2.0f;
    if (m_TimelineZoom > 1.5f) step = 0.5f;

    for (float t = 0.0f; t <= m_Sequence.duration; t += step) {
        float x = TimeToPixel(t);

        bool isMajor = std::fmod(t, 5.0f) < 0.01f;

        if (isMajor) {
            drawList->AddLine(ImVec2(x, rulerStart.y), ImVec2(x, rulerEnd.y),
                              IM_COL32(150, 150, 150, 255));

            char label[16];
            snprintf(label, sizeof(label), "%.0fs", t);
            drawList->AddText(ImVec2(x + 2, rulerStart.y + 3), IM_COL32(200, 200, 200, 255), label);
        } else {
            drawList->AddLine(ImVec2(x, rulerStart.y + rulerHeight * 0.5f),
                              ImVec2(x, rulerEnd.y), IM_COL32(80, 80, 80, 255));
        }
    }

    // Current time indicator
    float currentX = TimeToPixel(m_CurrentTime);
    drawList->AddTriangleFilled(
        ImVec2(currentX - 6, rulerStart.y),
        ImVec2(currentX + 6, rulerStart.y),
        ImVec2(currentX, rulerStart.y + 10),
        IM_COL32(255, 100, 100, 255));

    drawList->AddLine(ImVec2(currentX, rulerStart.y), ImVec2(currentX, rulerEnd.y),
                      IM_COL32(255, 100, 100, 255), 2.0f);

    ImGui::Dummy(ImVec2(m_TimelineSize.x, rulerHeight));
}

void CameraSequencerView::RenderKeyframeEditor() {
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    float startY = m_TimelinePos.y + 30;  // After ruler

    // Draw tracks and keyframes
    for (size_t i = 0; i < m_Sequence.tracks.size(); ++i) {
        auto& track = m_Sequence.tracks[i];
        float trackY = startY + i * m_TrackHeight;

        // Track background
        ImU32 bgColor = (i % 2 == 0) ? IM_COL32(35, 35, 40, 255) : IM_COL32(40, 40, 45, 255);
        if (m_SelectedTrack == static_cast<int>(i)) {
            bgColor = IM_COL32(50, 60, 80, 255);
        }

        drawList->AddRectFilled(
            ImVec2(m_TimelinePos.x, trackY),
            ImVec2(m_TimelinePos.x + m_TimelineSize.x, trackY + m_TrackHeight),
            bgColor);

        // Draw keyframes
        for (size_t k = 0; k < track.keyframes.size(); ++k) {
            auto& keyframe = track.keyframes[k];

            float x = TimeToPixel(keyframe.time);
            float y = trackY + m_TrackHeight * 0.5f;

            // Keyframe diamond
            ImVec2 points[4] = {
                ImVec2(x, y - 8),
                ImVec2(x + 8, y),
                ImVec2(x, y + 8),
                ImVec2(x - 8, y)
            };

            ImU32 keyColor = IM_COL32(100, 200, 255, 255);
            if (track.isMuted) keyColor = IM_COL32(80, 80, 80, 255);
            if (track.isActive) keyColor = IM_COL32(100, 255, 150, 255);

            drawList->AddConvexPolyFilled(points, 4, keyColor);
            drawList->AddPolyline(points, 4, IM_COL32(255, 255, 255, 200), true, 1.0f);
        }

        // Draw curve between keyframes
        if (track.keyframes.size() >= 2 && !track.isMuted) {
            for (size_t k = 0; k < track.keyframes.size() - 1; ++k) {
                float x1 = TimeToPixel(track.keyframes[k].time);
                float x2 = TimeToPixel(track.keyframes[k + 1].time);
                float y = trackY + m_TrackHeight * 0.5f;

                drawList->AddLine(ImVec2(x1, y), ImVec2(x2, y),
                    track.isActive ? IM_COL32(100, 255, 150, 100) : IM_COL32(100, 150, 200, 100),
                    2.0f);
            }
        }
    }

    // Draw current time line
    float currentX = TimeToPixel(m_CurrentTime);
    drawList->AddLine(
        ImVec2(currentX, startY),
        ImVec2(currentX, startY + m_Sequence.tracks.size() * m_TrackHeight),
        IM_COL32(255, 100, 100, 255), 2.0f);

    // Reserve space
    ImGui::Dummy(ImVec2(m_Sequence.duration * 80.0f * m_TimelineZoom,
                        m_Sequence.tracks.size() * m_TrackHeight + 30));
}

void CameraSequencerView::RenderPreviewPanel() {
    ImGui::Text("Preview");
    ImGui::Separator();

    // Preview settings
    ImGui::Checkbox("Show Preview", &m_ShowPreview);

    ImGui::Spacing();

    // Camera visualization
    ImVec2 avail = ImGui::GetContentRegionAvail();
    float previewSize = std::min(avail.x - 10, avail.y - 150);
    previewSize = std::max(100.0f, previewSize);

    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // Background
    drawList->AddRectFilled(pos, ImVec2(pos.x + previewSize, pos.y + previewSize),
                            IM_COL32(30, 30, 35, 255));

    // Grid
    int gridSize = static_cast<int>(previewSize / 10);
    for (int i = 0; i <= 10; ++i) {
        float offset = i * (previewSize / 10.0f);
        drawList->AddLine(ImVec2(pos.x + offset, pos.y),
                          ImVec2(pos.x + offset, pos.y + previewSize),
                          IM_COL32(50, 50, 55, 255));
        drawList->AddLine(ImVec2(pos.x, pos.y + offset),
                          ImVec2(pos.x + previewSize, pos.y + offset),
                          IM_COL32(50, 50, 55, 255));
    }

    // Draw camera path
    if (!m_Sequence.tracks.empty()) {
        auto& track = m_Sequence.tracks[0];
        if (track.keyframes.size() >= 2) {
            float scale = previewSize / 20.0f;
            ImVec2 center(pos.x + previewSize * 0.5f, pos.y + previewSize * 0.5f);

            // Draw path
            for (size_t i = 0; i < track.keyframes.size() - 1; ++i) {
                const auto& k1 = track.keyframes[i];
                const auto& k2 = track.keyframes[i + 1];

                ImVec2 p1(center.x + k1.position.x * scale, center.y - k1.position.z * scale);
                ImVec2 p2(center.x + k2.position.x * scale, center.y - k2.position.z * scale);

                drawList->AddLine(p1, p2, IM_COL32(100, 200, 255, 200), 2.0f);
            }

            // Draw keyframe positions
            for (const auto& k : track.keyframes) {
                ImVec2 p(center.x + k.position.x * scale, center.y - k.position.z * scale);
                drawList->AddCircleFilled(p, 5.0f, IM_COL32(100, 200, 255, 255));
            }

            // Draw current camera position (interpolated)
            // For now, just show first keyframe position
            ImVec2 camPos(center.x + track.keyframes[0].position.x * scale,
                          center.y - track.keyframes[0].position.z * scale);
            drawList->AddCircleFilled(camPos, 8.0f, IM_COL32(255, 100, 100, 255));

            // Draw view direction
            drawList->AddLine(camPos, ImVec2(camPos.x, camPos.y - 20),
                              IM_COL32(255, 100, 100, 255), 2.0f);
        }
    }

    drawList->AddRect(pos, ImVec2(pos.x + previewSize, pos.y + previewSize),
                      IM_COL32(80, 80, 80, 255));

    ImGui::Dummy(ImVec2(previewSize, previewSize));

    // Current keyframe info
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("Current Time: %.2f s", m_CurrentTime);
    ImGui::Text("Frame: %d", static_cast<int>(m_CurrentTime * m_Sequence.frameRate));
}

void CameraSequencerView::RenderCameraProperties() {
    if (m_SelectedTrack < 0 || m_SelectedTrack >= static_cast<int>(m_Sequence.tracks.size())) {
        ImGui::TextDisabled("Select a track to edit properties");
        return;
    }

    auto& track = m_Sequence.tracks[m_SelectedTrack];

    SectionHeader("Track Properties");

    char nameBuf[128];
    strncpy(nameBuf, track.name.c_str(), sizeof(nameBuf) - 1);
    if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf))) {
        track.name = nameBuf;
    }

    ImGui::Checkbox("Active", &track.isActive);
    ImGui::Checkbox("Muted", &track.isMuted);
    ImGui::Checkbox("Locked", &track.isLocked);

    ImGui::Spacing();
    SectionHeader("Keyframes");

    ImGui::Text("Keyframes: %zu", track.keyframes.size());

    for (size_t i = 0; i < track.keyframes.size(); ++i) {
        auto& kf = track.keyframes[i];

        ImGui::PushID(static_cast<int>(i));

        if (ImGui::TreeNode(("Keyframe " + std::to_string(i)).c_str())) {
            ImGui::DragFloat("Time", &kf.time, 0.1f, 0.0f, m_Sequence.duration);
            ImGui::DragFloat3("Position", &kf.position.x, 0.1f);
            ImGui::DragFloat3("Target", &kf.target.x, 0.1f);
            ImGui::DragFloat("FOV", &kf.fov, 1.0f, 10.0f, 120.0f);
            ImGui::DragFloat("Roll", &kf.roll, 1.0f, -180.0f, 180.0f);

            const char* interps[] = {"Linear", "Smooth", "Bezier"};
            ImGui::Combo("Interpolation", &kf.interpolation, interps, 3);

            if (ImGui::Button("Delete")) {
                track.keyframes.erase(track.keyframes.begin() + i);
            }

            ImGui::TreePop();
        }

        ImGui::PopID();
    }

    ImGui::Spacing();
    if (ImGui::Button("Add Keyframe")) {
        CameraKeyframe newKf;
        newKf.time = m_CurrentTime;
        newKf.position = glm::vec3(0, 2, 5);
        newKf.target = glm::vec3(0, 0, 0);
        newKf.fov = 60.0f;
        newKf.roll = 0.0f;
        newKf.interpolation = 1;
        track.keyframes.push_back(newKf);

        // Sort by time
        std::sort(track.keyframes.begin(), track.keyframes.end(),
            [](const CameraKeyframe& a, const CameraKeyframe& b) {
                return a.time < b.time;
            });
    }
}

void CameraSequencerView::RenderShakePanel() {
    SectionHeader("Camera Shake");

    ImGui::Checkbox("Enable Shake", &m_ShakeActive);

    ImGui::Spacing();

    // Preset selector
    ImGui::Text("Preset:");
    for (size_t i = 0; i < m_ShakePresets.size(); ++i) {
        if (i > 0) ImGui::SameLine();
        if (ImGui::RadioButton(m_ShakePresets[i].name.c_str(), m_SelectedShake == static_cast<int>(i))) {
            m_SelectedShake = static_cast<int>(i);
        }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Shake parameters
    if (m_SelectedShake > 0) {
        auto& shake = m_ShakePresets[m_SelectedShake];

        ImGui::SliderFloat("Intensity", &shake.intensity, 0.0f, 2.0f);
        ImGui::SliderFloat("Frequency", &shake.frequency, 1.0f, 30.0f);
        ImGui::SliderFloat("Duration", &shake.duration, 0.0f, 5.0f);
        ImGui::Checkbox("Decay", &shake.decay);

        ImGui::Spacing();

        if (ImGui::Button("Preview Shake")) {
            // Would trigger shake preview
        }
    }

    ImGui::Spacing();
    SectionHeader("Custom Shake");

    static float customIntensity = 0.5f;
    static float customFrequency = 10.0f;
    static float customDuration = 1.0f;
    static bool customDecay = true;

    ImGui::SliderFloat("Intensity##custom", &customIntensity, 0.0f, 2.0f);
    ImGui::SliderFloat("Frequency##custom", &customFrequency, 1.0f, 30.0f);
    ImGui::SliderFloat("Duration##custom", &customDuration, 0.0f, 5.0f);
    ImGui::Checkbox("Decay##custom", &customDecay);

    if (ImGui::Button("Save as Preset")) {
        ShakePreset newPreset;
        newPreset.name = "Custom " + std::to_string(m_ShakePresets.size());
        newPreset.intensity = customIntensity;
        newPreset.frequency = customFrequency;
        newPreset.duration = customDuration;
        newPreset.decay = customDecay;
        m_ShakePresets.push_back(newPreset);
    }
}

void CameraSequencerView::HandleTimelineInput() {
    ImGuiIO& io = ImGui::GetIO();

    if (ImGui::IsWindowHovered()) {
        // Scrub timeline with click
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            ImVec2 mousePos = io.MousePos;
            if (mousePos.y < m_TimelinePos.y + 30) {
                float time = PixelToTime(mousePos.x);
                SetTime(time);
            }
        }

        // Drag to scrub
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            ImVec2 mousePos = io.MousePos;
            float time = PixelToTime(mousePos.x);

            if (m_SnapToFrame) {
                time = std::round(time * m_Sequence.frameRate) / m_Sequence.frameRate;
            }

            SetTime(time);
        }

        // Zoom with scroll
        if (io.MouseWheel != 0.0f) {
            m_TimelineZoom *= (1.0f + io.MouseWheel * 0.1f);
            m_TimelineZoom = std::clamp(m_TimelineZoom, 0.25f, 4.0f);
        }
    }
}

float CameraSequencerView::TimeToPixel(float time) {
    float pixelsPerSecond = 80.0f * m_TimelineZoom;
    return m_TimelinePos.x + time * pixelsPerSecond;
}

float CameraSequencerView::PixelToTime(float pixel) {
    float pixelsPerSecond = 80.0f * m_TimelineZoom;
    return (pixel - m_TimelinePos.x) / pixelsPerSecond;
}

void CameraSequencerView::OnRenderStatusBar() {
    ImGui::Text("%s | %.2fs / %.2fs | %zu tracks | %s",
                m_Sequence.name.c_str(),
                m_CurrentTime,
                m_Sequence.duration,
                m_Sequence.tracks.size(),
                m_IsPlaying ? "Playing" : "Stopped");
}

} // namespace Yalaz::UI
