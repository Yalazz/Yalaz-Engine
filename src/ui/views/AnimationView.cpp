// =============================================================================
// YALAZ ENGINE - Animation View Implementation
// =============================================================================

#include "AnimationView.h"
#include "../../vk_engine.h"
#include "../../vk_loader.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstring>
#include <fstream>
#include <filesystem>

namespace Yalaz::UI {

using json = nlohmann::json;

namespace {

std::string fileNameOnly(const std::string& p) {
    std::filesystem::path path(p);
    return path.filename().string();
}

bool sceneMatchesSource(VulkanEngine* engine, const std::string& sceneName, const std::string& sourceScene) {
    if (sourceScene.empty()) return true;
    if (sceneName == sourceScene) return true;

    if (engine) {
        auto it = engine->sceneFilePaths.find(sceneName);
        if (it != engine->sceneFilePaths.end() && it->second == sourceScene) {
            return true;
        }
    }

    std::error_code ec;
    const std::string scenePath = std::filesystem::weakly_canonical(sceneName, ec).string();
    if (!scenePath.empty() && scenePath == sourceScene) return true;

    const bool sourceIsFileOnly =
        (sourceScene.find('/') == std::string::npos && sourceScene.find('\\') == std::string::npos);
    if (!sourceIsFileOnly) return false;

    return fileNameOnly(sceneName) == sourceScene;
}

std::vector<int> collectVisibleClipIndices(VulkanEngine* engine) {
    std::vector<int> visible;
    if (!engine || engine->animationClips.empty()) return visible;
    if (engine->loadedScenes.empty()) return visible;

    visible.reserve(engine->animationClips.size());
    for (int i = 0; i < static_cast<int>(engine->animationClips.size()); ++i) {
        const auto& clip = engine->animationClips[i];
        if (clip.sourceScene.empty()) {
            visible.push_back(i);
            continue;
        }

        bool clipSceneLoaded = false;
        for (const auto& [sceneName, scene] : engine->loadedScenes) {
            if (!scene) continue;
            if (sceneMatchesSource(engine, sceneName, clip.sourceScene)) {
                clipSceneLoaded = true;
                break;
            }
        }
        if (!clipSceneLoaded) continue;

        visible.push_back(i);
    }

    // Fallback: if we have loaded scenes and clips but strict source matching found none,
    // don't hide animation panel content completely.
    if (visible.empty() && !engine->animationClips.empty()) {
        visible.reserve(engine->animationClips.size());
        for (int i = 0; i < static_cast<int>(engine->animationClips.size()); ++i) {
            visible.push_back(i);
        }
    }

    return visible;
}

int normalizeActiveVisibleClip(VulkanEngine* engine, const std::vector<int>& visibleIndices) {
    if (!engine) return -1;
    if (visibleIndices.empty()) {
        engine->activeAnimationIndex = -1;
        return -1;
    }

    const int active = engine->activeAnimationIndex;
    if (std::find(visibleIndices.begin(), visibleIndices.end(), active) != visibleIndices.end()) {
        return active;
    }

    engine->activeAnimationIndex = visibleIndices.front();
    return engine->activeAnimationIndex;
}

} // namespace

AnimationView::AnimationView()
    : EditorView("Animation", "[A]", ViewCategory::Animation) {
}

ViewFlags AnimationView::GetFlags() const {
    return ViewFlags::CanDock | ViewFlags::CanTab | ViewFlags::CanFloat |
           ViewFlags::HasToolbar | ViewFlags::HasStatusBar;
}

void AnimationView::EnsureGraphInitialized() {
    if (!m_Engine) return;
    const std::vector<int> visibleClips = collectVisibleClipIndices(m_Engine);
    if (visibleClips.empty()) {
        auto& graph = m_Engine->animationGraph;
        graph.states.clear();
        graph.transitions.clear();
        graph.activeState = -1;
        graph.nextState = -1;
        graph.blending = false;
        graph.blendDuration = 0.0f;
        graph.blendElapsed = 0.0f;
        return;
    }

    auto& graph = m_Engine->animationGraph;
    bool graphValid = !graph.states.empty();
    if (graphValid) {
        for (const auto& s : graph.states) {
            if (std::find(visibleClips.begin(), visibleClips.end(), s.clipIndex) == visibleClips.end()) {
                graphValid = false;
                break;
            }
        }
    }
    if (graphValid) return;

    graph.states.clear();
    graph.transitions.clear();
    graph.parameters.clear();
    graph.parameters.push_back({"speed", 0.0f, false});
    graph.parameters.push_back({"isGrounded", 1.0f, true});

    for (int i = 0; i < static_cast<int>(visibleClips.size()); ++i) {
        const int clipIndex = visibleClips[i];
        AnimationGraphStateData state;
        state.name = m_Engine->animationClips[clipIndex].name;
        state.clipIndex = clipIndex;
        state.isDefault = (i == 0);
        state.positionX = 120.0f + static_cast<float>(i % 4) * 180.0f;
        state.positionY = 100.0f + static_cast<float>(i / 4) * 120.0f;
        graph.states.push_back(state);
    }
    for (int i = 0; i + 1 < static_cast<int>(graph.states.size()); ++i) {
        AnimationGraphTransitionData tr;
        tr.fromState = i;
        tr.toState = i + 1;
        tr.parameter = "speed";
        tr.comparison = 0;
        tr.threshold = 0.2f;
        tr.hasExitTime = true;
        tr.exitTime = 0.9f;
        tr.blendTime = 0.2f;
        tr.enabled = true;
        graph.transitions.push_back(tr);
    }
    graph.activeState = 0;
    graph.enabled = false;
}

void AnimationView::UpdateAutoGraphPath() {
    if (!m_Engine) return;
    std::string sceneKey = "default_scene";
    const std::vector<int> visibleClips = collectVisibleClipIndices(m_Engine);
    const int activeVisibleIndex = normalizeActiveVisibleClip(m_Engine, visibleClips);

    if (activeVisibleIndex >= 0 &&
        activeVisibleIndex < static_cast<int>(m_Engine->animationClips.size())) {
        const std::string& src = m_Engine->animationClips[activeVisibleIndex].sourceScene;
        if (!src.empty()) {
            sceneKey = src;
        }
    } else if (!visibleClips.empty() &&
        !m_Engine->animationClips[visibleClips.front()].sourceScene.empty()) {
        sceneKey = m_Engine->animationClips[visibleClips.front()].sourceScene;
    }

    std::string sanitized;
    sanitized.reserve(sceneKey.size());
    for (char c : sceneKey) {
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-' || c == '.') {
            sanitized.push_back(c);
        } else {
            sanitized.push_back('_');
        }
    }
    if (sanitized.empty()) sanitized = "default_scene";

    std::string autoPath = "animation_graphs/" + sanitized + ".animgraph.json";
    if (m_LastAutoGraphScene != autoPath) {
        m_LastAutoGraphScene = autoPath;
        std::snprintf(m_GraphPath, IM_ARRAYSIZE(m_GraphPath), "%s", autoPath.c_str());
        if (std::filesystem::exists(autoPath)) {
            LoadStateMachineGraph(autoPath);
        }
    }
}

void AnimationView::SaveStateMachineGraph(const std::string& path) {
    if (!m_Engine) return;
    const auto& graph = m_Engine->animationGraph;

    json root = json::object();
    root["enabled"] = graph.enabled;
    root["activeState"] = graph.activeState;
    root["states"] = json::array();
    root["transitions"] = json::array();
    root["parameters"] = json::array();

    for (const auto& s : graph.states) {
        root["states"].push_back({
            {"name", s.name},
            {"clipIndex", s.clipIndex},
            {"isDefault", s.isDefault},
            {"positionX", s.positionX},
            {"positionY", s.positionY}
        });
    }
    for (const auto& t : graph.transitions) {
        root["transitions"].push_back({
            {"fromState", t.fromState},
            {"toState", t.toState},
            {"parameter", t.parameter},
            {"comparison", t.comparison},
            {"threshold", t.threshold},
            {"hasExitTime", t.hasExitTime},
            {"exitTime", t.exitTime},
            {"blendTime", t.blendTime},
            {"enabled", t.enabled}
        });
    }
    for (const auto& p : graph.parameters) {
        root["parameters"].push_back({
            {"name", p.name},
            {"value", p.value},
            {"isBool", p.isBool}
        });
    }

    std::filesystem::path outPath(path);
    if (!outPath.parent_path().empty()) {
        std::error_code ec;
        std::filesystem::create_directories(outPath.parent_path(), ec);
    }
    std::ofstream out(path);
    if (!out.is_open()) return;
    out << root.dump(2);
}

void AnimationView::LoadStateMachineGraph(const std::string& path) {
    if (!m_Engine) return;
    std::ifstream in(path);
    if (!in.is_open()) return;

    json root;
    try {
        in >> root;
    } catch (...) {
        return;
    }

    auto& graph = m_Engine->animationGraph;
    graph.states.clear();
    graph.transitions.clear();
    graph.parameters.clear();

    graph.enabled = root.value("enabled", false);
    graph.activeState = root.value("activeState", -1);

    if (root.contains("states") && root["states"].is_array()) {
        for (const auto& s : root["states"]) {
            AnimationGraphStateData state;
            state.name = s.value("name", "State");
            state.clipIndex = s.value("clipIndex", -1);
            state.isDefault = s.value("isDefault", false);
            state.positionX = s.value("positionX", 120.0f);
            state.positionY = s.value("positionY", 100.0f);
            if (state.clipIndex >= 0 && state.clipIndex < static_cast<int>(m_Engine->animationClips.size())) {
                graph.states.push_back(state);
            }
        }
    }
    if (root.contains("transitions") && root["transitions"].is_array()) {
        for (const auto& t : root["transitions"]) {
            AnimationGraphTransitionData tr;
            tr.fromState = t.value("fromState", -1);
            tr.toState = t.value("toState", -1);
            tr.parameter = t.value("parameter", "speed");
            tr.comparison = t.value("comparison", 0);
            tr.threshold = t.value("threshold", 0.5f);
            tr.hasExitTime = t.value("hasExitTime", true);
            tr.exitTime = t.value("exitTime", 0.9f);
            tr.blendTime = t.value("blendTime", 0.2f);
            tr.enabled = t.value("enabled", true);
            if (tr.fromState >= 0 && tr.fromState < static_cast<int>(graph.states.size()) &&
                tr.toState >= 0 && tr.toState < static_cast<int>(graph.states.size())) {
                graph.transitions.push_back(tr);
            }
        }
    }
    if (root.contains("parameters") && root["parameters"].is_array()) {
        for (const auto& p : root["parameters"]) {
            AnimationGraphParameter param;
            param.name = p.value("name", "");
            param.value = p.value("value", 0.0f);
            param.isBool = p.value("isBool", false);
            if (!param.name.empty()) {
                graph.parameters.push_back(param);
            }
        }
    }

    if (graph.parameters.empty()) {
        graph.parameters.push_back({"speed", 0.0f, false});
        graph.parameters.push_back({"isGrounded", 1.0f, true});
    }
    if (graph.activeState < 0 || graph.activeState >= static_cast<int>(graph.states.size())) {
        graph.activeState = graph.states.empty() ? -1 : 0;
    }
    graph.nextState = -1;
    graph.blending = false;
    graph.blendDuration = 0.0f;
    graph.blendElapsed = 0.0f;
}

void AnimationView::OnInit(VulkanEngine* engine) {
    EditorView::OnInit(engine);
    UpdateAutoGraphPath();
    EnsureGraphInitialized();
    SyncWithEngine();
}

void AnimationView::SyncWithEngine() {
    if (!m_Engine) return;
    UpdateAutoGraphPath();

    const std::vector<int> visibleClips = collectVisibleClipIndices(m_Engine);

    // Sync animation clips from engine
    int clipIndex = normalizeActiveVisibleClip(m_Engine, visibleClips);
    if (clipIndex >= 0) {
        const auto& engineClip = m_Engine->animationClips[clipIndex];
        m_Clip.name = engineClip.name;
        m_Clip.duration = std::max(engineClip.duration, 0.0f);
        m_Clip.loop = engineClip.loop;
        m_CurrentTime = std::clamp(engineClip.currentTime, 0.0f, m_Clip.duration);
        m_IsPlaying = engineClip.isPlaying;

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
    } else {
        m_Clip.name = "No Clip";
        m_Clip.duration = 0.0f;
        m_Clip.loop = true;
        m_Clip.tracks.clear();
        m_CurrentTime = 0.0f;
        m_IsPlaying = false;
    }

    // Sync skeleton from engine
    m_Skeleton.clear();
    int skeletonIndex = -1;
    if (clipIndex >= 0) {
        const auto& clip = m_Engine->animationClips[clipIndex];
        if (clip.skeletonIndex >= 0 &&
            clip.skeletonIndex < static_cast<int>(m_Engine->skeletons.size())) {
            skeletonIndex = clip.skeletonIndex;
            m_Engine->activeSkeletonIndex = skeletonIndex;
        } else {
            m_Engine->activeSkeletonIndex = -1;
        }
    }
    if (skeletonIndex >= 0) {
        const auto& engineSkeleton = m_Engine->skeletons[skeletonIndex];
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

    if (clipIndex >= 0) {
        const auto& engineClip = m_Engine->animationClips[clipIndex];
        for (const auto& track : engineClip.tracks) {
            if (track.targetBoneIndex >= 0 && track.targetBoneIndex < static_cast<int>(m_Skeleton.size())) {
                m_Skeleton[track.targetBoneIndex].isSelected = engineClip.isPlaying;
            }
        }
    }

    if (m_SelectedTrackIndex >= static_cast<int>(m_Clip.tracks.size())) {
        m_SelectedTrackIndex = m_Clip.tracks.empty() ? -1 : 0;
    }
    if (m_SelectedBoneIndex >= static_cast<int>(m_Skeleton.size())) {
        m_SelectedBoneIndex = m_Skeleton.empty() ? -1 : 0;
    }

    EnsureGraphInitialized();

    // State machine view mirrors runtime graph data.
    m_States.clear();
    m_Transitions.clear();
    if (m_Engine && !m_Engine->animationGraph.states.empty()) {
        const int stateCount = static_cast<int>(m_Engine->animationGraph.states.size());
        const int cols = std::max(1, static_cast<int>(std::ceil(std::sqrt(static_cast<float>(stateCount)))));
        const float xStart = 120.0f;
        const float yStart = 100.0f;
        const float xStep = 180.0f;
        const float yStep = 120.0f;

        for (int i = 0; i < stateCount; ++i) {
            const auto& gState = m_Engine->animationGraph.states[i];

            AnimationState state;
            state.name = gState.name;
            state.clipName = gState.name;
            if (gState.clipIndex >= 0 && gState.clipIndex < static_cast<int>(m_Engine->animationClips.size())) {
                state.speed = m_Engine->animationClips[gState.clipIndex].speed;
            } else {
                state.speed = 1.0f;
            }
            state.positionX = (gState.positionX == 0.0f && gState.positionY == 0.0f)
                ? (xStart + static_cast<float>(i % cols) * xStep) : gState.positionX;
            state.positionY = (gState.positionX == 0.0f && gState.positionY == 0.0f)
                ? (yStart + static_cast<float>(i / cols) * yStep) : gState.positionY;
            state.isDefault = gState.isDefault;
            state.isSelected = (i == m_Engine->animationGraph.activeState);
            m_States.push_back(state);
        }

        for (const auto& tr : m_Engine->animationGraph.transitions) {
            AnimationTransition vt;
            vt.fromState = tr.fromState;
            vt.toState = tr.toState;
            vt.duration = tr.blendTime;
            vt.condition = tr.parameter;
            vt.hasExitTime = tr.hasExitTime;
            vt.exitTime = tr.exitTime;
            m_Transitions.push_back(vt);
        }
    }

    if (m_SelectedStateIndex >= static_cast<int>(m_States.size())) {
        m_SelectedStateIndex = m_States.empty() ? -1 : 0;
    }
    if (m_SelectedTransitionIndex >= static_cast<int>(m_Engine->animationGraph.transitions.size())) {
        m_SelectedTransitionIndex = m_Engine->animationGraph.transitions.empty() ? -1 : 0;
    }
}

void AnimationView::OnUpdate(float deltaTime) {
    SyncWithEngine();
    const std::vector<int> visibleClips = collectVisibleClipIndices(m_Engine);
    const int activeVisible = normalizeActiveVisibleClip(m_Engine, visibleClips);
    if (m_Engine && activeVisible >= 0 &&
        activeVisible < static_cast<int>(m_Engine->animationClips.size())) {
        return; // Engine drives runtime playback time
    }
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
    if (!m_Engine) return;
    const std::vector<int> visibleClips = collectVisibleClipIndices(m_Engine);
    if (visibleClips.empty()) return;

    m_IsPlaying = true;
    int activeClip = normalizeActiveVisibleClip(m_Engine, visibleClips);
    if (activeClip < 0) return;

    if (m_Engine->animationClips[activeClip].skeletonIndex >= 0) {
        m_Engine->activeSkeletonIndex = m_Engine->animationClips[activeClip].skeletonIndex;
    } else {
        m_Engine->activeSkeletonIndex = -1;
    }
    auto& clip = m_Engine->animationClips[activeClip];
    if (std::abs(clip.speed) < 0.0001f) {
        clip.speed = 1.0f;
    }
    m_Engine->playAnimation(activeClip);
}

void AnimationView::Pause() {
    m_IsPlaying = false;
    if (!m_Engine) return;
    const std::vector<int> visibleClips = collectVisibleClipIndices(m_Engine);
    const int activeVisible = normalizeActiveVisibleClip(m_Engine, visibleClips);
    if (activeVisible >= 0 &&
        activeVisible < static_cast<int>(m_Engine->animationClips.size())) {
        m_Engine->animationClips[activeVisible].isPlaying = false;
    }
}

void AnimationView::Stop() {
    m_IsPlaying = false;
    m_CurrentTime = 0.0f;
    if (!m_Engine) return;
    const std::vector<int> visibleClips = collectVisibleClipIndices(m_Engine);
    const int activeVisible = normalizeActiveVisibleClip(m_Engine, visibleClips);
    if (activeVisible >= 0) {
        m_Engine->stopAnimation(activeVisible);
    }
}

void AnimationView::SetTime(float time) {
    const float maxTime = std::max(0.0f, m_Clip.duration);
    m_CurrentTime = std::clamp(time, 0.0f, maxTime);
    if (!m_Engine) return;
    const std::vector<int> visibleClips = collectVisibleClipIndices(m_Engine);
    const int activeVisible = normalizeActiveVisibleClip(m_Engine, visibleClips);
    if (activeVisible >= 0 &&
        activeVisible < static_cast<int>(m_Engine->animationClips.size())) {
        auto& activeClip = m_Engine->animationClips[activeVisible];
        activeClip.currentTime = m_CurrentTime;
        m_Engine->updateAnimations(0.0f);
    }
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
    const std::vector<int> visibleClips = collectVisibleClipIndices(m_Engine);
    const int activeVisible = normalizeActiveVisibleClip(m_Engine, visibleClips);

    if (m_Engine && activeVisible >= 0 &&
        activeVisible < static_cast<int>(m_Engine->animationClips.size())) {
        auto& activeClip = m_Engine->animationClips[activeVisible];
        m_Clip.loop = activeClip.loop;
        m_CurrentTime = activeClip.currentTime;
        m_IsPlaying = activeClip.isPlaying;
    }

    const bool hasEngineClips = (m_Engine && !visibleClips.empty());
    if (hasEngineClips) {
        int activeIdx = activeVisible;
        if (activeIdx < 0) {
            activeIdx = visibleClips.front();
            m_Engine->activeAnimationIndex = activeIdx;
            if (m_Engine->animationClips[activeIdx].skeletonIndex >= 0) {
                m_Engine->activeSkeletonIndex = m_Engine->animationClips[activeIdx].skeletonIndex;
            } else {
                m_Engine->activeSkeletonIndex = -1;
            }
        }

        ImGui::SetNextItemWidth(170.0f);
        const char* currentName = m_Engine->animationClips[activeIdx].name.c_str();
        if (ImGui::BeginCombo("##AnimClipSelect", currentName)) {
            for (const int i : visibleClips) {
                bool selected = (i == activeIdx);
                if (ImGui::Selectable(m_Engine->animationClips[i].name.c_str(), selected)) {
                    m_Engine->activeAnimationIndex = i;
                    if (m_Engine->animationClips[i].skeletonIndex >= 0) {
                        m_Engine->activeSkeletonIndex = m_Engine->animationClips[i].skeletonIndex;
                    } else {
                        m_Engine->activeSkeletonIndex = -1;
                    }
                    SyncWithEngine();
                }
                if (selected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        ImGui::SameLine();
    }

    if (!hasEngineClips) {
        ImGui::TextDisabled("No animation clips loaded");
        return;
    }

    // Playback controls
    if (ImGui::Button(m_IsPlaying ? "||" : ">")) {
        if (m_IsPlaying) Pause();
        else Play();
    }
    ImGui::SameLine();

    if (ImGui::Button("[]")) {
        Stop();
    }

    if (m_Clip.duration <= 0.0f) {
        ImGui::SameLine();
        ImGui::TextDisabled("Static clip (single keyframe)");
    }

    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine();

    // Time display
    const float fps = std::max(1.0f, m_Clip.frameRate);
    int frame = static_cast<int>(m_CurrentTime * fps);
    int totalFrames = static_cast<int>(m_Clip.duration * fps);
    ImGui::Text("%d / %d", frame, totalFrames);

    ImGui::SameLine();
    ImGui::SetNextItemWidth(100);
    float time = m_CurrentTime;
    if (m_Clip.duration > 0.0f && ImGui::SliderFloat("##Time", &time, 0.0f, m_Clip.duration, "%.2f s")) {
        SetTime(time);
    }

    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine();

    // Speed
    ImGui::Text("Speed:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(60);
    float speed = m_Settings.previewSpeed;
    if (m_Engine && activeVisible >= 0 &&
        activeVisible < static_cast<int>(m_Engine->animationClips.size())) {
        speed = m_Engine->animationClips[activeVisible].speed;
    }
    if (ImGui::SliderFloat("##Speed", &speed, 0.1f, 3.0f, "%.1fx")) {
        m_Settings.previewSpeed = speed;
        if (m_Engine && activeVisible >= 0 &&
            activeVisible < static_cast<int>(m_Engine->animationClips.size())) {
            m_Engine->animationClips[activeVisible].speed = speed;
        }
    }

    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine();

    // Zoom
    ImGui::Text("Zoom:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(80);
    ImGui::SliderFloat("##Zoom", &m_Settings.zoom, 0.5f, 4.0f, "%.1fx");

    ImGui::SameLine();
    if (ImGui::Checkbox("Loop", &m_Clip.loop)) {
        if (m_Engine && activeVisible >= 0 &&
            activeVisible < static_cast<int>(m_Engine->animationClips.size())) {
            m_Engine->animationClips[activeVisible].loop = m_Clip.loop;
        }
    }
    if (m_Engine && activeVisible >= 0 &&
        activeVisible < static_cast<int>(m_Engine->animationClips.size())) {
        ImGui::SameLine();
        ImGui::Checkbox("PingPong", &m_Engine->animationClips[activeVisible].pingPong);
    }
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
        const ImVec2 clipMin = pos;
        const ImVec2 clipMax(pos.x + previewSize, pos.y + previewSize);
        drawList->PushClipRect(clipMin, clipMax, true);

        std::vector<glm::vec2> worldXY(m_Skeleton.size(), glm::vec2(0.0f));
        float minX = 0.0f, minY = 0.0f, maxX = 0.0f, maxY = 0.0f;
        bool hasBounds = false;

        for (size_t i = 0; i < m_Skeleton.size(); ++i) {
            glm::vec2 p(m_Skeleton[i].localPosition[0], m_Skeleton[i].localPosition[1]);
            int parent = m_Skeleton[i].parentIndex;
            int guard = 0;
            while (parent >= 0 && parent < static_cast<int>(m_Skeleton.size()) && guard++ < 512) {
                p += glm::vec2(m_Skeleton[parent].localPosition[0], m_Skeleton[parent].localPosition[1]);
                parent = m_Skeleton[parent].parentIndex;
            }
            worldXY[i] = p;

            if (!hasBounds) {
                minX = maxX = p.x;
                minY = maxY = p.y;
                hasBounds = true;
            } else {
                minX = std::min(minX, p.x);
                minY = std::min(minY, p.y);
                maxX = std::max(maxX, p.x);
                maxY = std::max(maxY, p.y);
            }
        }

        const float spanX = std::max(0.001f, maxX - minX);
        const float spanY = std::max(0.001f, maxY - minY);
        const float fitPadding = 20.0f;
        const float fitW = std::max(1.0f, previewSize - fitPadding * 2.0f);
        const float fitH = std::max(1.0f, previewSize - fitPadding * 2.0f);
        const float scale = std::min(fitW / spanX, fitH / spanY);
        const float centerX = 0.5f * (minX + maxX);
        const float centerY = 0.5f * (minY + maxY);
        const ImVec2 screenCenter(pos.x + previewSize * 0.5f, pos.y + previewSize * 0.5f);

        auto toScreen = [&](const glm::vec2& p) {
            return ImVec2(
                screenCenter.x + (p.x - centerX) * scale,
                screenCenter.y - (p.y - centerY) * scale
            );
        };

        for (size_t i = 0; i < m_Skeleton.size(); ++i) {
            const auto& bone = m_Skeleton[i];
            if (bone.parentIndex >= 0 && bone.parentIndex < static_cast<int>(worldXY.size())) {
                ImVec2 p1 = toScreen(worldXY[bone.parentIndex]);
                ImVec2 p2 = toScreen(worldXY[i]);
                drawList->AddLine(p1, p2, IM_COL32(200, 200, 200, 255), 2.0f);
            }
        }

        for (size_t i = 0; i < m_Skeleton.size(); ++i) {
            const auto& bone = m_Skeleton[i];
            ImVec2 jointPos = toScreen(worldXY[i]);
            ImU32 jointColor = bone.isSelected
                ? IM_COL32(255, 200, 100, 255)
                : IM_COL32(100, 150, 200, 255);
            drawList->AddCircleFilled(jointPos, 4.0f, jointColor);

            if (m_Settings.showBoneNames) {
                drawList->AddText(
                    ImVec2(jointPos.x + 5, jointPos.y - 5),
                    IM_COL32(150, 150, 150, 255),
                    bone.name.c_str());
            }
        }

        drawList->PopClipRect();
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

            if (m_Engine && m_Engine->activeSkeletonIndex >= 0 &&
                m_Engine->activeSkeletonIndex < static_cast<int>(m_Engine->skeletons.size())) {
                const auto& engineSkeleton = m_Engine->skeletons[m_Engine->activeSkeletonIndex];
                if (i < engineSkeleton.bones.size()) {
                    int nodeIndex = engineSkeleton.bones[i].nodeIndex;
                    if (nodeIndex >= 0) {
                        for (auto& [sceneName, scene] : m_Engine->loadedScenes) {
                            if (!scene) continue;
                            if (nodeIndex < static_cast<int>(scene->indexedNodes.size()) && scene->indexedNodes[nodeIndex]) {
                                m_Engine->selectedNode = scene->indexedNodes[nodeIndex].get();
                                m_Engine->selectedPrimitiveIndex = -1;
                                m_Engine->selectedLightIndex = -1;
                                m_Engine->selectedObjectName = bone.name;
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
    ImGui::EndChild();
}

void AnimationView::RenderStateMachine() {
    ImGui::Text("Animation State Machine");
    ImGui::Separator();

    if (!m_Engine) {
        ImGui::TextDisabled("Engine not available.");
        return;
    }

    EnsureGraphInitialized();
    auto& graph = m_Engine->animationGraph;

    ImGui::Checkbox("Enable Runtime State Machine", &graph.enabled);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(320.0f);
    ImGui::InputText("Graph File", m_GraphPath, IM_ARRAYSIZE(m_GraphPath));
    ImGui::SameLine();
    if (ImGui::Button("Save Graph")) {
        SaveStateMachineGraph(m_GraphPath);
    }
    ImGui::SameLine();
    if (ImGui::Button("Load Graph")) {
        LoadStateMachineGraph(m_GraphPath);
        SyncWithEngine();
    }

    if (graph.blending) {
        float blendAlpha = (graph.blendDuration > 0.0001f)
            ? glm::clamp(graph.blendElapsed / graph.blendDuration, 0.0f, 1.0f)
            : 1.0f;
        ImGui::Text("Cross-fade: %.2f / %.2f s", graph.blendElapsed, graph.blendDuration);
        ImGui::ProgressBar(blendAlpha, ImVec2(280.0f, 0.0f), "Blend");
    }

    ImGui::Spacing();

    ImVec2 avail = ImGui::GetContentRegionAvail();
    float canvasHeight = std::max(220.0f, avail.y * 0.58f);
    ImVec2 canvasSize(avail.x, canvasHeight);
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // Background
    drawList->AddRectFilled(pos, ImVec2(pos.x + canvasSize.x, pos.y + canvasSize.y),
                            IM_COL32(30, 30, 35, 255));

    // Draw grid
    for (float x = 0; x < canvasSize.x; x += 50) {
        drawList->AddLine(ImVec2(pos.x + x, pos.y),
                          ImVec2(pos.x + x, pos.y + canvasSize.y),
                          IM_COL32(40, 40, 45, 255));
    }
    for (float y = 0; y < canvasSize.y; y += 50) {
        drawList->AddLine(ImVec2(pos.x, pos.y + y),
                          ImVec2(pos.x + canvasSize.x, pos.y + y),
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
        if (len <= 0.0001f) {
            continue;
        }
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
        ImVec2 stateMin(statePos.x - stateSize.x * 0.5f, statePos.y - stateSize.y * 0.5f);
        ImVec2 stateMax(statePos.x + stateSize.x * 0.5f, statePos.y + stateSize.y * 0.5f);

        ImU32 bgColor = state.isDefault ?
            IM_COL32(80, 120, 80, 255) : IM_COL32(60, 60, 80, 255);

        if (state.isSelected) {
            bgColor = IM_COL32(80, 80, 120, 255);
        }

        drawList->AddRectFilled(
            stateMin,
            stateMax,
            bgColor, 5.0f);

        drawList->AddRect(
            stateMin,
            stateMax,
            IM_COL32(100, 100, 120, 255), 5.0f, 0, 2.0f);

        // State name
        ImVec2 textSize = ImGui::CalcTextSize(state.name.c_str());
        drawList->AddText(
            ImVec2(statePos.x - textSize.x * 0.5f, statePos.y - textSize.y * 0.5f),
            IM_COL32(220, 220, 220, 255), state.name.c_str());

        // Interaction: click a state to switch active clip.
        const bool stateHovered = ImGui::IsMouseHoveringRect(stateMin, stateMax);
        if (stateHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            m_SelectedStateIndex = static_cast<int>(i);
            for (size_t si = 0; si < m_States.size(); ++si) {
                m_States[si].isSelected = (si == i);
            }

            if (static_cast<int>(i) < static_cast<int>(graph.states.size())) {
                const bool wasPlaying = m_IsPlaying;
                graph.activeState = static_cast<int>(i);
                graph.nextState = -1;
                graph.blending = false;
                graph.blendDuration = 0.0f;
                graph.blendElapsed = 0.0f;

                int clipIndex = graph.states[graph.activeState].clipIndex;
                if (clipIndex >= 0 && clipIndex < static_cast<int>(m_Engine->animationClips.size())) {
                    m_Engine->activeAnimationIndex = clipIndex;
                    auto& clip = m_Engine->animationClips[clipIndex];
                    if (clip.skeletonIndex >= 0) {
                        m_Engine->activeSkeletonIndex = clip.skeletonIndex;
                    }
                    clip.currentTime = 0.0f;
                    if (wasPlaying) {
                        m_Engine->playAnimation(clipIndex);
                    } else {
                        m_Engine->updateAnimations(0.0f);
                    }
                }
                SyncWithEngine();
            }
        }
        if (stateHovered &&
            m_SelectedStateIndex == static_cast<int>(i) &&
            ImGui::IsMouseDown(ImGuiMouseButton_Left) &&
            ImGui::IsMouseDragging(ImGuiMouseButton_Left, 2.0f)) {
            m_DraggingStateIndex = static_cast<int>(i);
        }
    }

    ImGuiIO& io = ImGui::GetIO();
    if (m_DraggingStateIndex >= 0 && m_DraggingStateIndex < static_cast<int>(graph.states.size()) &&
        ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        graph.states[m_DraggingStateIndex].positionX += io.MouseDelta.x;
        graph.states[m_DraggingStateIndex].positionY += io.MouseDelta.y;
        if (m_DraggingStateIndex < static_cast<int>(m_States.size())) {
            m_States[m_DraggingStateIndex].positionX = graph.states[m_DraggingStateIndex].positionX;
            m_States[m_DraggingStateIndex].positionY = graph.states[m_DraggingStateIndex].positionY;
        }
    }
    if (m_DraggingStateIndex >= 0 && !ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        SaveStateMachineGraph(m_GraphPath);
        m_DraggingStateIndex = -1;
    }

    ImGui::Dummy(canvasSize);

    ImGui::Spacing();
    ImGui::SeparatorText("Parameters");
    for (size_t pi = 0; pi < graph.parameters.size(); ++pi) {
        auto& p = graph.parameters[pi];
        ImGui::PushID(static_cast<int>(pi));
        ImGui::SetNextItemWidth(220.0f);
        ImGui::Text("%s", p.name.c_str());
        ImGui::SameLine();
        ImGui::SetNextItemWidth(160.0f);
        bool paramChanged = false;
        if (p.isBool) {
            bool b = (p.value >= 0.5f);
            if (ImGui::Checkbox("##ParamBool", &b)) {
                p.value = b ? 1.0f : 0.0f;
                paramChanged = true;
            }
        } else {
            paramChanged = ImGui::DragFloat("##ParamValue", &p.value, 0.01f);
        }
        ImGui::SameLine();
        if (ImGui::Checkbox("Bool", &p.isBool)) {
            if (p.isBool) p.value = (p.value >= 0.5f) ? 1.0f : 0.0f;
            paramChanged = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("X")) {
            graph.parameters.erase(graph.parameters.begin() + static_cast<int>(pi));
            SaveStateMachineGraph(m_GraphPath);
            ImGui::PopID();
            break;
        }
        if (paramChanged) {
            SaveStateMachineGraph(m_GraphPath);
        }
        ImGui::PopID();
    }
    ImGui::SetNextItemWidth(160.0f);
    ImGui::InputText("New Param", m_NewParamName, IM_ARRAYSIZE(m_NewParamName));
    ImGui::SameLine();
    if (ImGui::Button("Add Param") && std::strlen(m_NewParamName) > 0) {
        graph.parameters.push_back({m_NewParamName, 0.0f, false});
        SaveStateMachineGraph(m_GraphPath);
    }

    ImGui::Spacing();
    ImGui::SeparatorText("Transitions");
    if (m_SelectedStateIndex >= 0 && m_SelectedStateIndex < static_cast<int>(graph.states.size())) {
        ImGui::Text("Selected State: %s", graph.states[m_SelectedStateIndex].name.c_str());

        std::vector<int> filteredTransitionIndices;
        for (int i = 0; i < static_cast<int>(graph.transitions.size()); ++i) {
            if (graph.transitions[i].fromState == m_SelectedStateIndex) {
                filteredTransitionIndices.push_back(i);
            }
        }

        if (ImGui::Button("+ Add Transition")) {
            AnimationGraphTransitionData tr;
            tr.fromState = m_SelectedStateIndex;
            tr.toState = m_SelectedStateIndex;
            tr.parameter = graph.parameters.empty() ? "speed" : graph.parameters.front().name;
            graph.transitions.push_back(tr);
            m_SelectedTransitionIndex = static_cast<int>(graph.transitions.size()) - 1;
            SaveStateMachineGraph(m_GraphPath);
            SyncWithEngine();
        }

        if (!filteredTransitionIndices.empty()) {
            if (m_SelectedTransitionIndex < 0 ||
                std::find(filteredTransitionIndices.begin(), filteredTransitionIndices.end(), m_SelectedTransitionIndex) == filteredTransitionIndices.end()) {
                m_SelectedTransitionIndex = filteredTransitionIndices.front();
            }

            const auto& selectedTr = graph.transitions[m_SelectedTransitionIndex];
            auto stateLabel = [&](int idx) -> const char* {
                if (idx >= 0 && idx < static_cast<int>(graph.states.size())) {
                    return graph.states[idx].name.c_str();
                }
                return "Invalid";
            };
            std::string label = std::string(stateLabel(selectedTr.fromState)) + " -> " + stateLabel(selectedTr.toState);
            if (ImGui::BeginCombo("Transition", label.c_str())) {
                for (int ti : filteredTransitionIndices) {
                    const auto& optTr = graph.transitions[ti];
                    std::string entry = std::string(stateLabel(optTr.fromState)) + " -> " + stateLabel(optTr.toState) +
                        "##tr_" + std::to_string(ti);
                    bool selected = (ti == m_SelectedTransitionIndex);
                    if (ImGui::Selectable(entry.c_str(), selected)) {
                        m_SelectedTransitionIndex = ti;
                    }
                }
                ImGui::EndCombo();
            }

            if (m_SelectedTransitionIndex >= 0 && m_SelectedTransitionIndex < static_cast<int>(graph.transitions.size())) {
                auto& tr = graph.transitions[m_SelectedTransitionIndex];
                bool transitionChanged = false;
                transitionChanged |= ImGui::Checkbox("Enabled", &tr.enabled);

                std::vector<const char*> stateNames;
                stateNames.reserve(graph.states.size());
                for (auto& s : graph.states) stateNames.push_back(s.name.c_str());
                transitionChanged |= ImGui::Combo("To State", &tr.toState, stateNames.data(), static_cast<int>(stateNames.size()));

                if (!graph.parameters.empty()) {
                    int paramIndex = 0;
                    for (int i = 0; i < static_cast<int>(graph.parameters.size()); ++i) {
                        if (graph.parameters[i].name == tr.parameter) {
                            paramIndex = i;
                            break;
                        }
                    }
                    std::vector<const char*> paramNames;
                    paramNames.reserve(graph.parameters.size());
                    for (auto& p : graph.parameters) paramNames.push_back(p.name.c_str());
                    if (ImGui::Combo("Parameter", &paramIndex, paramNames.data(), static_cast<int>(paramNames.size()))) {
                        tr.parameter = graph.parameters[paramIndex].name;
                        transitionChanged = true;
                    }
                } else {
                    ImGui::TextDisabled("No parameters defined");
                }

                const char* cmpNames[] = { ">", "<", ">=", "<=", "==", "!=" };
                tr.comparison = std::clamp(tr.comparison, 0, 5);
                transitionChanged |= ImGui::Combo("Condition", &tr.comparison, cmpNames, IM_ARRAYSIZE(cmpNames));
                transitionChanged |= ImGui::DragFloat("Threshold", &tr.threshold, 0.01f);
                transitionChanged |= ImGui::Checkbox("Has Exit Time", &tr.hasExitTime);
                if (tr.hasExitTime) {
                    transitionChanged |= ImGui::SliderFloat("Exit Time", &tr.exitTime, 0.0f, 1.0f, "%.2f");
                }
                transitionChanged |= ImGui::SliderFloat("Blend Time", &tr.blendTime, 0.0f, 2.0f, "%.2fs");

                if (ImGui::Button("Delete Transition")) {
                    graph.transitions.erase(graph.transitions.begin() + m_SelectedTransitionIndex);
                    m_SelectedTransitionIndex = -1;
                    SaveStateMachineGraph(m_GraphPath);
                } else if (transitionChanged) {
                    SaveStateMachineGraph(m_GraphPath);
                }
            }
        }
    } else {
        ImGui::TextDisabled("Select a state node to edit transitions.");
    }
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
                float fps = std::max(1.0f, m_Clip.frameRate);
                time = std::round(time * fps) / fps;
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
    ImGui::Text("%s | %.2fs / %.2fs | %d tracks | %d bones | %s",
                m_Clip.name.c_str(),
                m_CurrentTime,
                m_Clip.duration,
                static_cast<int>(m_Clip.tracks.size()),
                static_cast<int>(m_Skeleton.size()),
                m_IsPlaying ? "Playing" : "Stopped");
}

} // namespace Yalaz::UI
