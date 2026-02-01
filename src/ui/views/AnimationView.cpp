// =============================================================================
// YALAZ ENGINE - Animation View Implementation
// =============================================================================

#include "AnimationView.h"
#include "../../vk_engine.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <algorithm>
#include <cmath>

namespace Yalaz::UI {

AnimationView::AnimationView()
    : EditorView("Animation", "[A]", ViewCategory::Animation) {
}

ViewFlags AnimationView::GetFlags() const {
    return ViewFlags::CanDock | ViewFlags::CanTab | ViewFlags::CanFloat |
           ViewFlags::HasToolbar | ViewFlags::HasStatusBar;
}

void AnimationView::OnInit(VulkanEngine* engine) {
    EditorView::OnInit(engine);
    SyncWithEngine();
}

void AnimationView::SyncWithEngine() {
    if (!m_Engine) return;

    // Sync animation clips from engine
    if (!m_Engine->animationClips.empty()) {
        const auto& engineClip = m_Engine->animationClips[0];
        m_Clip.name = engineClip.name;
        m_Clip.duration = engineClip.duration;
        m_Clip.loop = engineClip.loop;

        // Convert engine tracks to view tracks
        m_Clip.tracks.clear();
        for (const auto& engineTrack : engineClip.tracks) {
            AnimationTrack track;
            track.name = engineTrack.targetNode + "/" + engineTrack.property;
            track.property = engineTrack.property;
            track.component = 0;

            for (const auto& kf : engineTrack.keyframes) {
                AnimationKeyframe keyframe;
                keyframe.time = kf.time;
                keyframe.value = kf.value.x;
                keyframe.interpolation = static_cast<AnimationKeyframe::Interpolation>(kf.interpolation);
                track.keyframes.push_back(keyframe);
            }

            m_Clip.tracks.push_back(track);
        }
    }

    // Sync skeleton from engine
    m_Skeleton.clear();
    if (!m_Engine->skeletons.empty()) {
        const auto& engineSkeleton = m_Engine->skeletons[0];
        for (const auto& bone : engineSkeleton.bones) {
            SkeletonBone viewBone;
            viewBone.name = bone.name;
            viewBone.parentIndex = bone.parentIndex;
            viewBone.localPosition = {bone.localPosition.x, bone.localPosition.y, bone.localPosition.z};
            viewBone.localRotation = {bone.localRotation.x, bone.localRotation.y, bone.localRotation.z, bone.localRotation.w};
            viewBone.localScale = {bone.localScale.x, bone.localScale.y, bone.localScale.z};
            m_Skeleton.push_back(viewBone);
        }
    }

    // If no skeleton loaded, create default
    if (m_Skeleton.empty()) {
        m_Skeleton.push_back({"Root", -1, {0, 0, 0}, {0, 0, 0, 1}, {1, 1, 1}});
        m_Skeleton.push_back({"Hips", 0, {0, 1, 0}, {0, 0, 0, 1}, {1, 1, 1}});
        m_Skeleton.push_back({"Spine", 1, {0, 0.3f, 0}, {0, 0, 0, 1}, {1, 1, 1}});
        m_Skeleton.push_back({"Chest", 2, {0, 0.3f, 0}, {0, 0, 0, 1}, {1, 1, 1}});
        m_Skeleton.push_back({"Head", 3, {0, 0.3f, 0}, {0, 0, 0, 1}, {1, 1, 1}});
        m_Skeleton.push_back({"LeftArm", 3, {0.2f, 0, 0}, {0, 0, 0, 1}, {1, 1, 1}});
        m_Skeleton.push_back({"RightArm", 3, {-0.2f, 0, 0}, {0, 0, 0, 1}, {1, 1, 1}});
        m_Skeleton.push_back({"LeftLeg", 1, {0.1f, 0, 0}, {0, 0, 0, 1}, {1, 1, 1}});
        m_Skeleton.push_back({"RightLeg", 1, {-0.1f, 0, 0}, {0, 0, 0, 1}, {1, 1, 1}});
    }

    // Animation states for state machine view
    m_States.clear();
    m_States.push_back({"Idle", "Idle", 1.0f, 100, 100, true, false});
    m_States.push_back({"Walk", "Walk", 1.0f, 300, 100, false, false});
    m_States.push_back({"Run", "Run", 1.0f, 300, 200, false, false});
    m_States.push_back({"Jump", "Jump", 1.0f, 500, 150, false, false});

    m_Transitions.clear();
    m_Transitions.push_back({0, 1, 0.25f, "speed > 0.1", true, 0.9f});
    m_Transitions.push_back({1, 0, 0.25f, "speed < 0.1", true, 0.9f});
    m_Transitions.push_back({1, 2, 0.2f, "speed > 0.5", true, 0.8f});
    m_Transitions.push_back({2, 1, 0.2f, "speed < 0.5", true, 0.8f});
}

void AnimationView::OnUpdate(float deltaTime) {
    if (m_IsPlaying) {
        m_CurrentTime += deltaTime * m_Settings.previewSpeed;

        if (m_CurrentTime >= m_Clip.duration) {
            if (m_Clip.loop) {
                m_CurrentTime = std::fmod(m_CurrentTime, m_Clip.duration);
            } else {
                m_CurrentTime = m_Clip.duration;
                m_IsPlaying = false;
            }
        }
    }
}

void AnimationView::Play() {
    m_IsPlaying = true;
    if (m_Engine && m_Engine->activeAnimationIndex >= 0) {
        m_Engine->playAnimation(m_Engine->activeAnimationIndex);
    }
}

void AnimationView::Pause() {
    m_IsPlaying = false;
    if (m_Engine && m_Engine->activeAnimationIndex >= 0 &&
        m_Engine->activeAnimationIndex < static_cast<int>(m_Engine->animationClips.size())) {
        m_Engine->animationClips[m_Engine->activeAnimationIndex].isPlaying = false;
    }
}

void AnimationView::Stop() {
    m_IsPlaying = false;
    m_CurrentTime = 0.0f;
    if (m_Engine && m_Engine->activeAnimationIndex >= 0) {
        m_Engine->stopAnimation(m_Engine->activeAnimationIndex);
    }
}

void AnimationView::SetTime(float time) {
    m_CurrentTime = std::clamp(time, 0.0f, m_Clip.duration);
}

void AnimationView::SetClip(const AnimationClip& clip) {
    m_Clip = clip;
    m_CurrentTime = 0.0f;
    m_IsPlaying = false;
}

void AnimationView::SetSkeleton(const std::vector<SkeletonBone>& bones) {
    m_Skeleton = bones;
}

void AnimationView::OnRenderToolbar() {
    // Playback controls
    if (ImGui::Button(m_IsPlaying ? "||" : ">")) {
        if (m_IsPlaying) Pause();
        else Play();
    }
    ImGui::SameLine();

    if (ImGui::Button("[]")) {
        Stop();
    }

    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine();

    // Time display
    int frame = static_cast<int>(m_CurrentTime * m_Clip.frameRate);
    int totalFrames = static_cast<int>(m_Clip.duration * m_Clip.frameRate);
    ImGui::Text("%d / %d", frame, totalFrames);

    ImGui::SameLine();
    ImGui::SetNextItemWidth(100);
    float time = m_CurrentTime;
    if (ImGui::SliderFloat("##Time", &time, 0.0f, m_Clip.duration, "%.2f s")) {
        SetTime(time);
    }

    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine();

    // Speed
    ImGui::Text("Speed:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(60);
    ImGui::SliderFloat("##Speed", &m_Settings.previewSpeed, 0.1f, 2.0f, "%.1fx");

    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine();

    // Zoom
    ImGui::Text("Zoom:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(80);
    ImGui::SliderFloat("##Zoom", &m_Settings.zoom, 0.5f, 4.0f, "%.1fx");

    ImGui::SameLine();
    ImGui::Checkbox("Snap", &m_Settings.snapToFrame);
}

void AnimationView::OnRenderContent() {
    ImGui::BeginTabBar("AnimationTabs");

    if (ImGui::BeginTabItem("Timeline")) {
        RenderTimeline();
        ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("State Machine")) {
        RenderStateMachine();
        ImGui::EndTabItem();
    }

    ImGui::EndTabBar();
}

void AnimationView::RenderTimeline() {
    ImVec2 avail = ImGui::GetContentRegionAvail();

    // Track list (left)
    float trackListWidth = 180.0f;

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

    // Preview panel (right)
    ImGui::BeginChild("AnimPreview", ImVec2(0, 0), true);
    RenderPreviewPanel();
    ImGui::EndChild();
}

void AnimationView::RenderTrackList() {
    ImGui::Text("Tracks");
    ImGui::Separator();

    for (size_t i = 0; i < m_Clip.tracks.size(); ++i) {
        auto& track = m_Clip.tracks[i];

        ImGui::PushID(static_cast<int>(i));

        // Mute/Lock buttons
        if (track.isMuted) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1));
        }

        bool selected = (m_SelectedTrackIndex == static_cast<int>(i));
        if (ImGui::Selectable(track.name.c_str(), selected, 0, ImVec2(0, m_TrackHeight))) {
            m_SelectedTrackIndex = static_cast<int>(i);
        }

        if (track.isMuted) {
            ImGui::PopStyleColor();
        }

        // Context menu
        if (ImGui::BeginPopupContextItem()) {
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

    // Events section
    ImGui::Text("Events");

    for (const auto& event : m_Clip.events) {
        ImGui::TextDisabled("%.2fs: %s", event.time, event.name.c_str());
    }
}

void AnimationView::RenderTimeRuler() {
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    float rulerHeight = 20.0f;
    ImVec2 rulerStart = m_TimelinePos;
    ImVec2 rulerEnd(m_TimelinePos.x + m_TimelineSize.x, m_TimelinePos.y + rulerHeight);

    // Background
    drawList->AddRectFilled(rulerStart, rulerEnd, IM_COL32(40, 40, 45, 255));

    // Time marks
    float pixelsPerSecond = 100.0f * m_Settings.zoom;
    float step = 0.1f;  // 0.1 second marks

    if (m_Settings.zoom < 0.5f) step = 0.5f;
    if (m_Settings.zoom > 2.0f) step = 0.05f;

    for (float t = 0.0f; t <= m_Clip.duration; t += step) {
        float x = TimeToPixel(t);

        bool isMajor = std::abs(std::fmod(t, 1.0f)) < 0.001f;

        if (isMajor) {
            drawList->AddLine(ImVec2(x, rulerStart.y), ImVec2(x, rulerEnd.y),
                              IM_COL32(150, 150, 150, 255));

            char label[16];
            snprintf(label, sizeof(label), "%.1fs", t);
            drawList->AddText(ImVec2(x + 2, rulerStart.y + 2), IM_COL32(200, 200, 200, 255), label);
        } else {
            drawList->AddLine(ImVec2(x, rulerStart.y + rulerHeight * 0.6f),
                              ImVec2(x, rulerEnd.y), IM_COL32(80, 80, 80, 255));
        }
    }

    // Current time indicator
    float currentX = TimeToPixel(m_CurrentTime);
    drawList->AddLine(ImVec2(currentX, rulerStart.y), ImVec2(currentX, rulerEnd.y),
                      IM_COL32(255, 100, 100, 255), 2.0f);

    // Advance cursor past ruler
    ImGui::Dummy(ImVec2(m_TimelineSize.x, rulerHeight));
}

void AnimationView::RenderKeyframeEditor() {
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    float startY = m_TimelinePos.y + 25;  // After ruler

    // Draw track backgrounds and keyframes
    for (size_t i = 0; i < m_Clip.tracks.size(); ++i) {
        auto& track = m_Clip.tracks[i];
        float trackY = startY + i * m_TrackHeight;

        // Track background
        ImU32 bgColor = (i % 2 == 0) ? IM_COL32(35, 35, 40, 255) : IM_COL32(40, 40, 45, 255);
        if (m_SelectedTrackIndex == static_cast<int>(i)) {
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
                ImVec2(x, y - 6),
                ImVec2(x + 6, y),
                ImVec2(x, y + 6),
                ImVec2(x - 6, y)
            };

            ImU32 keyColor = IM_COL32(200, 200, 100, 255);
            if (track.isMuted) keyColor = IM_COL32(100, 100, 100, 255);

            drawList->AddConvexPolyFilled(points, 4, keyColor);
            drawList->AddPolyline(points, 4, IM_COL32(255, 255, 200, 255), true, 1.0f);
        }
    }

    // Draw current time line
    float currentX = TimeToPixel(m_CurrentTime);
    drawList->AddLine(
        ImVec2(currentX, startY),
        ImVec2(currentX, startY + m_Clip.tracks.size() * m_TrackHeight),
        IM_COL32(255, 100, 100, 255), 2.0f);

    // Draw events
    if (m_Settings.showEvents) {
        for (const auto& event : m_Clip.events) {
            float x = TimeToPixel(event.time);
            drawList->AddTriangleFilled(
                ImVec2(x, m_TimelinePos.y + 22),
                ImVec2(x - 5, m_TimelinePos.y + 15),
                ImVec2(x + 5, m_TimelinePos.y + 15),
                IM_COL32(100, 200, 255, 255));
        }
    }

    // Reserve space
    ImGui::Dummy(ImVec2(m_Clip.duration * 100.0f * m_Settings.zoom,
                        m_Clip.tracks.size() * m_TrackHeight));
}

void AnimationView::RenderPreviewPanel() {
    ImGui::Text("Preview");
    ImGui::Separator();

    ImGui::Checkbox("Show Skeleton", &m_Settings.showSkeleton);
    ImGui::Checkbox("Show Mesh", &m_Settings.showMesh);
    ImGui::Checkbox("Bone Names", &m_Settings.showBoneNames);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Skeleton visualization
    ImVec2 avail = ImGui::GetContentRegionAvail();
    float previewSize = std::min(avail.x - 10, avail.y - 80);

    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // Background
    drawList->AddRectFilled(pos, ImVec2(pos.x + previewSize, pos.y + previewSize),
                            IM_COL32(30, 30, 35, 255));

    if (m_Settings.showSkeleton) {
        // Draw simplified skeleton
        ImVec2 center(pos.x + previewSize * 0.5f, pos.y + previewSize * 0.5f);
        float scale = previewSize * 0.15f;

        // Draw bones as lines
        for (size_t i = 0; i < m_Skeleton.size(); ++i) {
            const auto& bone = m_Skeleton[i];

            if (bone.parentIndex >= 0) {
                const auto& parent = m_Skeleton[bone.parentIndex];

                ImVec2 p1(center.x + parent.localPosition[0] * scale,
                          center.y - parent.localPosition[1] * scale);
                ImVec2 p2(center.x + bone.localPosition[0] * scale,
                          center.y - bone.localPosition[1] * scale);

                drawList->AddLine(p1, p2, IM_COL32(200, 200, 200, 255), 2.0f);
            }

            // Joint
            ImVec2 jointPos(center.x + bone.localPosition[0] * scale,
                            center.y - bone.localPosition[1] * scale);

            ImU32 jointColor = bone.isSelected ?
                IM_COL32(255, 200, 100, 255) : IM_COL32(100, 150, 200, 255);
            drawList->AddCircleFilled(jointPos, 4.0f, jointColor);

            if (m_Settings.showBoneNames) {
                drawList->AddText(ImVec2(jointPos.x + 5, jointPos.y - 5),
                                  IM_COL32(150, 150, 150, 255), bone.name.c_str());
            }
        }
    }

    drawList->AddRect(pos, ImVec2(pos.x + previewSize, pos.y + previewSize),
                      IM_COL32(60, 60, 60, 255));

    ImGui::Dummy(ImVec2(previewSize, previewSize));

    // Bone list
    ImGui::Spacing();
    ImGui::Text("Bones");
    ImGui::Separator();

    ImGui::BeginChild("BoneList", ImVec2(0, 0), false);
    for (size_t i = 0; i < m_Skeleton.size(); ++i) {
        auto& bone = m_Skeleton[i];

        bool selected = (m_SelectedBoneIndex == static_cast<int>(i));
        if (ImGui::Selectable(bone.name.c_str(), selected)) {
            m_SelectedBoneIndex = static_cast<int>(i);
            bone.isSelected = true;

            // Deselect others
            for (size_t j = 0; j < m_Skeleton.size(); ++j) {
                if (j != i) m_Skeleton[j].isSelected = false;
            }
        }
    }
    ImGui::EndChild();
}

void AnimationView::RenderStateMachine() {
    ImGui::Text("Animation State Machine");
    ImGui::Separator();

    ImVec2 avail = ImGui::GetContentRegionAvail();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // Background
    drawList->AddRectFilled(pos, ImVec2(pos.x + avail.x, pos.y + avail.y),
                            IM_COL32(30, 30, 35, 255));

    // Draw grid
    for (float x = 0; x < avail.x; x += 50) {
        drawList->AddLine(ImVec2(pos.x + x, pos.y),
                          ImVec2(pos.x + x, pos.y + avail.y),
                          IM_COL32(40, 40, 45, 255));
    }
    for (float y = 0; y < avail.y; y += 50) {
        drawList->AddLine(ImVec2(pos.x, pos.y + y),
                          ImVec2(pos.x + avail.x, pos.y + y),
                          IM_COL32(40, 40, 45, 255));
    }

    // Draw transitions
    for (const auto& trans : m_Transitions) {
        if (trans.fromState < 0 || trans.fromState >= static_cast<int>(m_States.size())) continue;
        if (trans.toState < 0 || trans.toState >= static_cast<int>(m_States.size())) continue;

        const auto& from = m_States[trans.fromState];
        const auto& to = m_States[trans.toState];

        ImVec2 start(pos.x + from.positionX, pos.y + from.positionY);
        ImVec2 end(pos.x + to.positionX, pos.y + to.positionY);

        drawList->AddLine(start, end, IM_COL32(150, 150, 150, 255), 2.0f);

        // Arrow head
        ImVec2 dir(end.x - start.x, end.y - start.y);
        float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
        dir.x /= len;
        dir.y /= len;

        ImVec2 arrowPos(end.x - dir.x * 35, end.y - dir.y * 35);
        ImVec2 perp(-dir.y * 5, dir.x * 5);

        drawList->AddTriangleFilled(
            ImVec2(arrowPos.x + dir.x * 10, arrowPos.y + dir.y * 10),
            ImVec2(arrowPos.x + perp.x, arrowPos.y + perp.y),
            ImVec2(arrowPos.x - perp.x, arrowPos.y - perp.y),
            IM_COL32(150, 150, 150, 255));
    }

    // Draw states
    for (size_t i = 0; i < m_States.size(); ++i) {
        auto& state = m_States[i];

        ImVec2 statePos(pos.x + state.positionX, pos.y + state.positionY);
        ImVec2 stateSize(100, 40);

        ImU32 bgColor = state.isDefault ?
            IM_COL32(80, 120, 80, 255) : IM_COL32(60, 60, 80, 255);

        if (state.isSelected) {
            bgColor = IM_COL32(80, 80, 120, 255);
        }

        drawList->AddRectFilled(
            ImVec2(statePos.x - stateSize.x * 0.5f, statePos.y - stateSize.y * 0.5f),
            ImVec2(statePos.x + stateSize.x * 0.5f, statePos.y + stateSize.y * 0.5f),
            bgColor, 5.0f);

        drawList->AddRect(
            ImVec2(statePos.x - stateSize.x * 0.5f, statePos.y - stateSize.y * 0.5f),
            ImVec2(statePos.x + stateSize.x * 0.5f, statePos.y + stateSize.y * 0.5f),
            IM_COL32(100, 100, 120, 255), 5.0f, 0, 2.0f);

        // State name
        ImVec2 textSize = ImGui::CalcTextSize(state.name.c_str());
        drawList->AddText(
            ImVec2(statePos.x - textSize.x * 0.5f, statePos.y - textSize.y * 0.5f),
            IM_COL32(220, 220, 220, 255), state.name.c_str());
    }

    ImGui::Dummy(avail);
}

void AnimationView::HandleTimelineInput() {
    ImGuiIO& io = ImGui::GetIO();

    if (ImGui::IsWindowHovered()) {
        // Scrub timeline with click
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            ImVec2 mousePos = io.MousePos;
            if (mousePos.y < m_TimelinePos.y + 25) {
                // Click in ruler area - set time
                float time = PixelToTime(mousePos.x);
                SetTime(time);
            }
        }

        // Drag to scrub
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            ImVec2 mousePos = io.MousePos;
            float time = PixelToTime(mousePos.x);

            if (m_Settings.snapToFrame) {
                time = std::round(time * m_Clip.frameRate) / m_Clip.frameRate;
            }

            SetTime(time);
        }

        // Zoom with scroll
        if (io.MouseWheel != 0.0f) {
            m_Settings.zoom *= (1.0f + io.MouseWheel * 0.1f);
            m_Settings.zoom = std::clamp(m_Settings.zoom, 0.5f, 4.0f);
        }
    }
}

float AnimationView::TimeToPixel(float time) {
    float pixelsPerSecond = 100.0f * m_Settings.zoom;
    return m_TimelinePos.x + (time - m_TimelineStart) * pixelsPerSecond;
}

float AnimationView::PixelToTime(float pixel) {
    float pixelsPerSecond = 100.0f * m_Settings.zoom;
    return m_TimelineStart + (pixel - m_TimelinePos.x) / pixelsPerSecond;
}

void AnimationView::OnRenderStatusBar() {
    ImGui::Text("%s | %.2fs / %.2fs | %d tracks | %s",
                m_Clip.name.c_str(),
                m_CurrentTime,
                m_Clip.duration,
                static_cast<int>(m_Clip.tracks.size()),
                m_IsPlaying ? "Playing" : "Stopped");
}

} // namespace Yalaz::UI
