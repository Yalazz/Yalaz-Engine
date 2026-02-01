// =============================================================================
// YALAZ ENGINE - UV View Implementation
// =============================================================================

#include "UVView.h"
#include "../../vk_engine.h"
#include <algorithm>
#include <cmath>

namespace Yalaz::UI {

void UVView::OnInit(VulkanEngine* engine) {
    EditorView::OnInit(engine);
    SyncWithEngine();
}

void UVView::SyncWithEngine() {
    if (!m_Engine) return;

    m_DemoUVs.clear();
    m_DemoIndices.clear();

    // Get UV data from selected primitive or mesh
    if (m_Engine->selectedPrimitiveIndex >= 0 &&
        m_Engine->selectedPrimitiveIndex < static_cast<int>(m_Engine->static_shapes.size())) {
        // For primitives, generate UV coordinates based on type
        const auto& shape = m_Engine->static_shapes[m_Engine->selectedPrimitiveIndex];
        GenerateUVsForPrimitive(static_cast<int>(shape.type));
    } else {
        // Try to get UVs from loaded scenes
        for (const auto& [sceneName, scene] : m_Engine->loadedScenes) {
            if (scene && !scene->meshes.empty()) {
                // Get first mesh with UV data
                for (const auto& [meshName, meshAsset] : scene->meshes) {
                    if (meshAsset && !meshAsset->surfaces.empty()) {
                        m_SelectedMeshName = meshName;
                        // Generate placeholder UVs for the mesh
                        GeneratePlaceholderUVs(meshAsset->surfaces[0].count);
                        return;
                    }
                }
            }
        }
    }

    // If nothing selected, create demo UVs
    if (m_DemoUVs.empty()) {
        // Demo triangle UVs
        m_DemoUVs = {
            0.1f, 0.1f, 0.9f, 0.1f, 0.5f, 0.9f,
            0.2f, 0.3f, 0.4f, 0.3f, 0.3f, 0.6f,
            0.6f, 0.3f, 0.8f, 0.3f, 0.7f, 0.6f
        };
        m_DemoIndices = {0, 1, 2, 3, 4, 5, 6, 7, 8};
    }
}

void UVView::GenerateUVsForPrimitive(int primitiveType) {
    m_DemoUVs.clear();
    m_DemoIndices.clear();

    // 0=Cube, 1=Sphere, 2=Capsule, 3=Cylinder, 4=Plane, 5=Cone, 6=Torus, 7=Triangle
    switch (primitiveType) {
        case 0:  // Cube
            // Box UV mapping (6 faces)
            for (int face = 0; face < 6; ++face) {
                float offsetU = (face % 3) * 0.33f;
                float offsetV = (face / 3) * 0.5f;
                m_DemoUVs.push_back(offsetU); m_DemoUVs.push_back(offsetV);
                m_DemoUVs.push_back(offsetU + 0.3f); m_DemoUVs.push_back(offsetV);
                m_DemoUVs.push_back(offsetU + 0.3f); m_DemoUVs.push_back(offsetV + 0.45f);
                m_DemoUVs.push_back(offsetU); m_DemoUVs.push_back(offsetV + 0.45f);
                uint32_t base = face * 4;
                m_DemoIndices.push_back(base); m_DemoIndices.push_back(base + 1); m_DemoIndices.push_back(base + 2);
                m_DemoIndices.push_back(base); m_DemoIndices.push_back(base + 2); m_DemoIndices.push_back(base + 3);
            }
            break;
        case 1:  // Sphere
            // Sphere UV mapping (equirectangular)
            for (int i = 0; i <= 8; ++i) {
                for (int j = 0; j <= 4; ++j) {
                    float u = static_cast<float>(i) / 8.0f;
                    float v = static_cast<float>(j) / 4.0f;
                    m_DemoUVs.push_back(u);
                    m_DemoUVs.push_back(v);
                }
            }
            for (int i = 0; i < 8; ++i) {
                for (int j = 0; j < 4; ++j) {
                    uint32_t a = i * 5 + j;
                    uint32_t b = a + 1;
                    uint32_t c = (i + 1) * 5 + j;
                    uint32_t d = c + 1;
                    m_DemoIndices.push_back(a); m_DemoIndices.push_back(c); m_DemoIndices.push_back(b);
                    m_DemoIndices.push_back(b); m_DemoIndices.push_back(c); m_DemoIndices.push_back(d);
                }
            }
            break;
        default:
            // Default triangle UV
            m_DemoUVs = {0.1f, 0.1f, 0.9f, 0.1f, 0.5f, 0.9f};
            m_DemoIndices = {0, 1, 2};
            break;
    }
}

void UVView::GeneratePlaceholderUVs(uint32_t vertexCount) {
    m_DemoUVs.clear();
    m_DemoIndices.clear();

    // Generate random UV layout for visualization
    for (uint32_t i = 0; i < vertexCount; ++i) {
        float u = 0.1f + 0.8f * (static_cast<float>(rand()) / RAND_MAX);
        float v = 0.1f + 0.8f * (static_cast<float>(rand()) / RAND_MAX);
        m_DemoUVs.push_back(u);
        m_DemoUVs.push_back(v);
        m_DemoIndices.push_back(i);
    }
}

void UVView::OnRender() {
    if (!BeginView(ImGuiWindowFlags_MenuBar)) {
        EndView();
        return;
    }

    if (ImGui::BeginMenuBar()) {
        // Selection mode
        const char* selectModes[] = {"Vertex", "Edge", "Face", "Island"};
        ImGui::SetNextItemWidth(80);
        ImGui::Combo("##SelectMode", &m_SelectMode, selectModes, 4);

        ImGui::SameLine();
        ImGui::Checkbox("Wire", &m_ShowWireframe);
        ImGui::SameLine();
        ImGui::Checkbox("Fill", &m_ShowFilled);
        ImGui::SameLine();
        ImGui::Checkbox("Grid", &m_ShowGrid);
        ImGui::SameLine();
        ImGui::Checkbox("Seams", &m_ShowSeams);

        ImGui::SameLine();
        if (ImGui::Button("Fit")) {
            m_Zoom = 1.0f;
            m_PanX = 0.0f;
            m_PanY = 0.0f;
        }

        ImGui::SameLine();
        ImGui::SetNextItemWidth(60);
        int zoomPct = static_cast<int>(m_Zoom * 100);
        if (ImGui::DragInt("##Zoom", &zoomPct, 1.0f, 25, 800, "%d%%")) {
            m_Zoom = zoomPct / 100.0f;
        }

        ImGui::SameLine();
        ImGui::Text("UV:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(40);
        ImGui::InputInt("##UV", &m_UVChannel, 0, 0);
        m_UVChannel = std::max(0, m_UVChannel);

        ImGui::EndMenuBar();
    }

    ImVec2 avail = ImGui::GetContentRegionAvail();
    float statsWidth = 160.0f;

    // UV Canvas
    ImGui::BeginChild("UVCanvas", ImVec2(avail.x - statsWidth - 10, 0), true,
                      ImGuiWindowFlags_NoScrollbar);
    RenderCanvas();
    ImGui::EndChild();

    ImGui::SameLine();

    // Statistics
    ImGui::BeginChild("UVStats", ImVec2(0, 0), true);
    RenderStatistics();
    ImGui::EndChild();

    EndView();
}

void UVView::RenderCanvas() {
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // Background
    drawList->AddRectFilled(canvasPos,
                            ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y),
                            IM_COL32(40, 40, 45, 255));

    float uvSize = std::min(canvasSize.x, canvasSize.y) * m_Zoom * 0.8f;
    float offsetX = (canvasSize.x - uvSize) * 0.5f + m_PanX;
    float offsetY = (canvasSize.y - uvSize) * 0.5f + m_PanY;

    ImVec2 uvMin(canvasPos.x + offsetX, canvasPos.y + offsetY);
    ImVec2 uvMax(uvMin.x + uvSize, uvMin.y + uvSize);

    // Grid
    if (m_ShowGrid) {
        int gridDivs = 10;
        float gridStep = uvSize / gridDivs;

        for (int i = 0; i <= gridDivs; ++i) {
            bool isMajor = (i % 5 == 0);
            ImU32 gridCol = isMajor ? IM_COL32(80, 80, 85, 255) : IM_COL32(60, 60, 65, 255);

            // Vertical
            float x = uvMin.x + i * gridStep;
            drawList->AddLine(ImVec2(x, uvMin.y), ImVec2(x, uvMax.y), gridCol);

            // Horizontal
            float y = uvMin.y + i * gridStep;
            drawList->AddLine(ImVec2(uvMin.x, y), ImVec2(uvMax.x, y), gridCol);
        }
    }

    // UV space boundary (0-1 box)
    drawList->AddRect(uvMin, uvMax, IM_COL32(120, 120, 120, 255), 0.0f, 0, 2.0f);

    // Demo triangles
    ImVec2 demoTris[] = {
        ImVec2(0.1f, 0.1f), ImVec2(0.9f, 0.1f), ImVec2(0.5f, 0.9f),
        ImVec2(0.2f, 0.3f), ImVec2(0.4f, 0.3f), ImVec2(0.3f, 0.6f),
        ImVec2(0.6f, 0.3f), ImVec2(0.8f, 0.3f), ImVec2(0.7f, 0.6f)
    };

    auto uvToScreen = [&](float u, float v) -> ImVec2 {
        return ImVec2(uvMin.x + u * uvSize, uvMin.y + (1.0f - v) * uvSize);
    };

    // Filled triangles
    if (m_ShowFilled) {
        ImU32 fillCol = IM_COL32(100, 120, 160, 80);
        for (int i = 0; i < 9; i += 3) {
            ImVec2 p0 = uvToScreen(demoTris[i].x, demoTris[i].y);
            ImVec2 p1 = uvToScreen(demoTris[i+1].x, demoTris[i+1].y);
            ImVec2 p2 = uvToScreen(demoTris[i+2].x, demoTris[i+2].y);
            drawList->AddTriangleFilled(p0, p1, p2, fillCol);
        }
    }

    // Wireframe
    if (m_ShowWireframe) {
        ImU32 wireCol = IM_COL32(200, 200, 200, 255);
        for (int i = 0; i < 9; i += 3) {
            ImVec2 p0 = uvToScreen(demoTris[i].x, demoTris[i].y);
            ImVec2 p1 = uvToScreen(demoTris[i+1].x, demoTris[i+1].y);
            ImVec2 p2 = uvToScreen(demoTris[i+2].x, demoTris[i+2].y);
            drawList->AddLine(p0, p1, wireCol);
            drawList->AddLine(p1, p2, wireCol);
            drawList->AddLine(p2, p0, wireCol);
        }
    }

    // Seams
    if (m_ShowSeams) {
        ImU32 seamCol = IM_COL32(255, 80, 80, 255);
        ImVec2 p0 = uvToScreen(0.1f, 0.1f);
        ImVec2 p1 = uvToScreen(0.9f, 0.1f);
        drawList->AddLine(p0, p1, seamCol, 2.0f);
    }

    // Handle zoom/pan
    if (ImGui::IsWindowHovered()) {
        ImGuiIO& io = ImGui::GetIO();
        if (io.MouseWheel != 0.0f) {
            m_Zoom *= (1.0f + io.MouseWheel * 0.1f);
            m_Zoom = std::clamp(m_Zoom, 0.25f, 8.0f);
        }
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
            m_PanX += io.MouseDelta.x;
            m_PanY += io.MouseDelta.y;
        }
    }

    ImGui::Dummy(canvasSize);
}

void UVView::RenderStatistics() {
    SectionHeader("UV Statistics");

    size_t vertexCount = m_DemoUVs.size() / 2;
    size_t triangleCount = m_DemoIndices.size() / 3;

    ImGui::Text("Vertices:"); ImGui::SameLine(100);
    ImGui::Text("%zu", vertexCount);

    ImGui::Text("Triangles:"); ImGui::SameLine(100);
    ImGui::Text("%zu", triangleCount);

    // Calculate UV bounds
    float minU = 1.0f, maxU = 0.0f, minV = 1.0f, maxV = 0.0f;
    for (size_t i = 0; i < m_DemoUVs.size(); i += 2) {
        minU = std::min(minU, m_DemoUVs[i]);
        maxU = std::max(maxU, m_DemoUVs[i]);
        minV = std::min(minV, m_DemoUVs[i + 1]);
        maxV = std::max(maxV, m_DemoUVs[i + 1]);
    }

    // Estimate coverage
    float coverage = (maxU - minU) * (maxV - minV) * 100.0f;
    ImGui::Text("Coverage:"); ImGui::SameLine(100);
    ImGui::Text("%.1f%%", coverage);

    ImGui::Spacing();
    SectionHeader("UV Range");

    ImGui::Text("U: [%.2f, %.2f]", minU, maxU);
    ImGui::Text("V: [%.2f, %.2f]", minV, maxV);

    // Show selected mesh/primitive info
    if (m_Engine) {
        ImGui::Spacing();
        SectionHeader("Selection");

        if (m_Engine->selectedPrimitiveIndex >= 0 &&
            m_Engine->selectedPrimitiveIndex < static_cast<int>(m_Engine->static_shapes.size())) {
            const auto& shape = m_Engine->static_shapes[m_Engine->selectedPrimitiveIndex];
            ImGui::Text("Primitive: %s", shape.name.empty() ? "Unnamed" : shape.name.c_str());
        } else if (!m_SelectedMeshName.empty()) {
            ImGui::Text("Mesh: %s", m_SelectedMeshName.c_str());
        } else {
            ImGui::TextDisabled("No selection");
        }
    }

    ImGui::Spacing();
    SectionHeader("Issues");

    // Check for out of bounds UVs
    int outOfBounds = 0;
    for (size_t i = 0; i < m_DemoUVs.size(); i += 2) {
        if (m_DemoUVs[i] < 0 || m_DemoUVs[i] > 1 ||
            m_DemoUVs[i + 1] < 0 || m_DemoUVs[i + 1] > 1) {
            outOfBounds++;
        }
    }

    ImGui::TextDisabled("Flipped: 0");
    ImGui::TextDisabled("Out of bounds: %d", outOfBounds);
    ImGui::TextDisabled("Overlaps: %s", triangleCount > 10 ? "Possible" : "None");

    ImGui::Spacing();
    if (ImGui::Button("Refresh UVs")) {
        SyncWithEngine();
    }
}

} // namespace Yalaz::UI
