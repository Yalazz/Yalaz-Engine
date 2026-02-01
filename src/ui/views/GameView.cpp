// =============================================================================
// YALAZ ENGINE - Game View Implementation
// =============================================================================
// Connected to engine - shows game viewport with scene info
// =============================================================================

#include "GameView.h"
#include "../../vk_engine.h"

namespace Yalaz::UI {

void GameView::OnRender() {
    if (!BeginView(ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_MenuBar)) {
        EndView();
        return;
    }

    if (ImGui::BeginMenuBar()) {
        // Play mode toggle
        if (ImGui::Button(m_IsPlaying ? "Pause" : "Play")) {
            m_IsPlaying = !m_IsPlaying;
        }

        ImGui::SameLine();
        ImGui::Checkbox("Stats", &m_ShowStats);
        ImGui::SameLine();
        ImGui::Checkbox("Gizmos", &m_ShowGizmos);

        if (m_Engine) {
            ImGui::SameLine(ImGui::GetWindowWidth() - 120);
            float fps = m_Engine->stats.frametime > 0 ? 1000.0f / m_Engine->stats.frametime : 0;
            ImVec4 fpsColor = fps >= 60 ? ImVec4(0.3f, 1.0f, 0.3f, 1.0f) :
                              fps >= 30 ? ImVec4(1.0f, 0.8f, 0.3f, 1.0f) :
                                          ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
            ImGui::TextColored(fpsColor, "%.0f FPS", fps);
        }
        ImGui::EndMenuBar();
    }

    ImVec2 size = ImGui::GetContentRegionAvail();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // Background
    drawList->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                            IM_COL32(20, 20, 25, 255));

    // Draw scene representation
    if (m_Engine) {
        RenderScenePreview(pos, size, drawList);
    }

    // Stats overlay
    if (m_ShowStats && m_Engine) {
        RenderStatsOverlay(pos);
    }

    // Play mode indicator
    if (m_IsPlaying) {
        drawList->AddRectFilled(pos, ImVec2(pos.x + 80, pos.y + 25),
                                IM_COL32(50, 150, 50, 200));
        drawList->AddText(ImVec2(pos.x + 10, pos.y + 5),
                          IM_COL32(255, 255, 255, 255), "PLAYING");
    }

    EndView();
}

void GameView::RenderScenePreview(ImVec2 pos, ImVec2 size, ImDrawList* drawList) {
    // Draw a simple representation of the scene

    // Ground plane
    float groundY = pos.y + size.y * 0.7f;
    drawList->AddRectFilled(
        ImVec2(pos.x, groundY),
        ImVec2(pos.x + size.x, pos.y + size.y),
        IM_COL32(30, 35, 30, 255));

    // Grid lines on ground
    for (int i = 0; i < 20; ++i) {
        float x = pos.x + (i / 19.0f) * size.x;
        float alpha = 1.0f - std::abs(i - 10) / 10.0f;
        drawList->AddLine(
            ImVec2(x, groundY),
            ImVec2(x, pos.y + size.y),
            IM_COL32(60, 80, 60, static_cast<int>(100 * alpha)));
    }

    // Draw primitives as simple shapes
    float centerX = pos.x + size.x * 0.5f;
    float baseY = groundY - 20;

    size_t primitiveCount = m_Engine->static_shapes.size();
    if (primitiveCount > 0) {
        // Draw up to 10 primitives
        size_t maxDraw = std::min(primitiveCount, (size_t)10);
        for (size_t i = 0; i < maxDraw; ++i) {
            const auto& shape = m_Engine->static_shapes[i];

            // Simple position mapping
            float px = centerX + shape.position.x * 10;
            float py = baseY - shape.position.y * 10 - 30;
            float sz = 20.0f + shape.scale.x * 5;

            // Color from shape
            ImU32 col = IM_COL32(
                static_cast<int>(shape.mainColor.r * 255),
                static_cast<int>(shape.mainColor.g * 255),
                static_cast<int>(shape.mainColor.b * 255),
                200);

            // Draw based on type
            switch (shape.type) {
                case PrimitiveType::Cube:
                    drawList->AddRectFilled(
                        ImVec2(px - sz/2, py - sz/2),
                        ImVec2(px + sz/2, py + sz/2), col);
                    break;
                case PrimitiveType::Sphere:
                    drawList->AddCircleFilled(ImVec2(px, py), sz/2, col);
                    break;
                case PrimitiveType::Cylinder:
                    drawList->AddRectFilled(
                        ImVec2(px - sz/3, py - sz/2),
                        ImVec2(px + sz/3, py + sz/2), col);
                    break;
                default:
                    drawList->AddTriangleFilled(
                        ImVec2(px, py - sz/2),
                        ImVec2(px - sz/2, py + sz/2),
                        ImVec2(px + sz/2, py + sz/2), col);
                    break;
            }

            // Selection highlight
            if (m_ShowGizmos && m_Engine->selectedPrimitiveIndex == static_cast<int>(i)) {
                drawList->AddCircle(ImVec2(px, py), sz/2 + 5,
                                    IM_COL32(255, 200, 0, 255), 0, 2.0f);
            }
        }

        if (primitiveCount > 10) {
            drawList->AddText(ImVec2(pos.x + 10, pos.y + size.y - 30),
                              IM_COL32(200, 200, 200, 255),
                              ("+" + std::to_string(primitiveCount - 10) + " more objects").c_str());
        }
    }

    // Draw lights as glowing points
    for (size_t i = 0; i < m_Engine->scenePointLights.size(); ++i) {
        const auto& light = m_Engine->scenePointLights[i];

        float lx = centerX + light.position.x * 10;
        float ly = baseY - light.position.y * 10 - 30;

        ImU32 lightCol = IM_COL32(
            static_cast<int>(light.color.r * 255),
            static_cast<int>(light.color.g * 255),
            static_cast<int>(light.color.b * 255),
            255);

        // Glow
        float glowSize = 10.0f + light.intensity * 0.5f;
        ImU32 glowCol = IM_COL32(
            static_cast<int>(light.color.r * 100),
            static_cast<int>(light.color.g * 100),
            static_cast<int>(light.color.b * 100),
            100);
        drawList->AddCircleFilled(ImVec2(lx, ly), glowSize, glowCol);
        drawList->AddCircleFilled(ImVec2(lx, ly), 5, lightCol);

        // Selection
        if (m_ShowGizmos && m_Engine->selectedLightIndex == static_cast<int>(i)) {
            drawList->AddCircle(ImVec2(lx, ly), glowSize + 5,
                                IM_COL32(255, 200, 0, 255), 0, 2.0f);
        }
    }

    // Scene info text
    if (m_Engine->loadedScenes.empty() && m_Engine->static_shapes.empty()) {
        ImVec2 textPos(pos.x + size.x / 2 - 80, pos.y + size.y / 2 - 10);
        drawList->AddText(textPos, IM_COL32(150, 150, 150, 255),
                          "Scene is empty");
    }
}

void GameView::RenderStatsOverlay(ImVec2 pos) {
    ImGui::SetCursorScreenPos(ImVec2(pos.x + 10, pos.y + 30));

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0.5f));
    ImGui::BeginChild("GameStats", ImVec2(150, 120), true, ImGuiWindowFlags_NoScrollbar);

    ImGui::Text("Frame: %.2f ms", m_Engine->stats.frametime);
    ImGui::Text("Primitives: %zu", m_Engine->static_shapes.size());
    ImGui::Text("Lights: %zu", m_Engine->scenePointLights.size());
    ImGui::Text("Scenes: %zu", m_Engine->loadedScenes.size());

    // Camera info
    ImGui::Separator();
    ImGui::Text("Cam: %.1f, %.1f, %.1f",
                m_Engine->mainCamera.position.x,
                m_Engine->mainCamera.position.y,
                m_Engine->mainCamera.position.z);

    ImGui::EndChild();
    ImGui::PopStyleColor();
}

} // namespace Yalaz::UI
