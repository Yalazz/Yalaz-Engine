// =============================================================================
// YALAZ ENGINE - Shader Debug View Implementation
// =============================================================================

#include "ShaderDebugView.h"
#include "../../vk_engine.h"

namespace Yalaz::UI {

void ShaderDebugView::OnInit(VulkanEngine* engine) {
    EditorView::OnInit(engine);
    SyncWithEngine();
}

void ShaderDebugView::SyncWithEngine() {
    if (!m_Engine) return;

    m_Shaders.clear();

    // Sync with engine shader pipelines
    for (const auto& pipeline : m_Engine->shaderPipelines) {
        ShaderEntry entry;
        entry.name = pipeline.name;
        entry.path = pipeline.vertPath;
        entry.isCompiled = pipeline.isValid;
        entry.compileLog = pipeline.errorLog;
        entry.compileTimeMs = pipeline.compileTimeMs;
        entry.instructionCount = 128;  // Estimated
        entry.registerCount = 16;
        entry.textureCount = pipeline.textureCount;
        m_Shaders.push_back(entry);
    }

    // Add entries from engine stats if available
    for (const auto& shaderName : m_Engine->stats.shaderNames) {
        bool exists = false;
        for (const auto& s : m_Shaders) {
            if (s.name == shaderName) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            ShaderEntry entry;
            entry.name = shaderName;
            entry.path = "shaders/" + shaderName + ".spv";
            entry.isCompiled = true;
            entry.compileTimeMs = 20.0f;
            entry.instructionCount = 64;
            entry.registerCount = 8;
            entry.textureCount = 0;
            m_Shaders.push_back(entry);
        }
    }

    // If still empty, add some default entries
    if (m_Shaders.empty()) {
        m_Shaders.push_back({"Default Pipeline", "shaders/default.spv", true, "", 15.0f, 64, 8, 1});
    }
}

void ShaderDebugView::OnRender() {
    if (!BeginView(ImGuiWindowFlags_MenuBar)) {
        EndView();
        return;
    }

    if (ImGui::BeginMenuBar()) {
        ImGui::SetNextItemWidth(150);
        ImGui::InputTextWithHint("##Search", "Search...", m_SearchBuffer, sizeof(m_SearchBuffer));

        ImGui::SameLine();
        if (ImGui::Button("Recompile All")) {
            if (m_Engine) {
                m_Engine->recompileAllShaders();
                SyncWithEngine();
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Refresh")) {
            SyncWithEngine();
        }

        ImGui::EndMenuBar();
    }

    // Shader list
    ImGui::BeginChild("ShaderList", ImVec2(m_ListWidth, 0), true);
    RenderShaderList();
    ImGui::EndChild();

    ImGui::SameLine();

    // Splitter
    ImGui::Button("##Splitter", ImVec2(4, -1));
    if (ImGui::IsItemActive()) {
        m_ListWidth += ImGui::GetIO().MouseDelta.x;
        m_ListWidth = std::max(150.0f, std::min(400.0f, m_ListWidth));
    }

    ImGui::SameLine();

    // Details
    ImGui::BeginChild("ShaderDetails", ImVec2(0, 0), true);
    RenderShaderDetails();
    ImGui::EndChild();

    EndView();
}

void ShaderDebugView::RenderShaderList() {
    ImGui::Text("Shaders (%zu)", m_Shaders.size());
    ImGui::Separator();

    for (size_t i = 0; i < m_Shaders.size(); ++i) {
        const auto& shader = m_Shaders[i];

        // Filter
        if (m_SearchBuffer[0] != '\0' &&
            shader.name.find(m_SearchBuffer) == std::string::npos) {
            continue;
        }

        ImGui::PushID(static_cast<int>(i));

        // Status indicator
        if (shader.isCompiled) {
            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "O");
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "X");
        }
        ImGui::SameLine();

        bool isSelected = (m_SelectedShader == static_cast<int>(i));
        if (ImGui::Selectable(shader.name.c_str(), isSelected)) {
            m_SelectedShader = static_cast<int>(i);
        }

        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", shader.path.c_str());
        }

        ImGui::PopID();
    }
}

void ShaderDebugView::RenderShaderDetails() {
    if (m_SelectedShader < 0 || m_SelectedShader >= static_cast<int>(m_Shaders.size())) {
        ImVec2 avail = ImGui::GetContentRegionAvail();
        ImGui::SetCursorPos(ImVec2(avail.x * 0.3f, avail.y * 0.5f));
        ImGui::TextDisabled("Select a shader from the list");
        return;
    }

    const auto& shader = m_Shaders[m_SelectedShader];

    ImGui::Text("%s", shader.name.c_str());
    ImGui::TextDisabled("%s", shader.path.c_str());

    // Status
    if (shader.isCompiled) {
        ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "Compiled (%.1f ms)", shader.compileTimeMs);
    } else {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Compilation Failed");
    }

    ImGui::Separator();

    // Tabs
    if (ImGui::BeginTabBar("ShaderTabs")) {
        if (!shader.isCompiled && ImGui::BeginTabItem("Errors")) {
            ImGui::Spacing();
            // Parse log lines
            std::string log = shader.compileLog;
            size_t pos;
            while ((pos = log.find('\n')) != std::string::npos) {
                std::string line = log.substr(0, pos);
                if (line.find("error:") != std::string::npos) {
                    ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", line.c_str());
                } else if (line.find("warning:") != std::string::npos) {
                    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "%s", line.c_str());
                } else {
                    ImGui::TextDisabled("%s", line.c_str());
                }
                log.erase(0, pos + 1);
            }
            ImGui::EndTabItem();
        }

        if (shader.isCompiled && ImGui::BeginTabItem("Uniforms")) {
            RenderUniforms();
            ImGui::EndTabItem();
        }

        if (shader.isCompiled && ImGui::BeginTabItem("Statistics")) {
            RenderStatistics();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Source")) {
            ImGui::TextDisabled("Source code preview not available");
            ImGui::Spacing();

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.8f, 0.6f, 1.0f));
            ImGui::Text("#version 450");
            ImGui::PopStyleColor();
            ImGui::NewLine();

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.8f, 1.0f));
            ImGui::Text("layout(location = 0) in vec3 inPosition;");
            ImGui::Text("layout(location = 1) in vec3 inNormal;");
            ImGui::Text("layout(location = 2) in vec2 inUV;");
            ImGui::PopStyleColor();
            ImGui::NewLine();

            ImGui::Text("void main() {");
            ImGui::Text("    gl_Position = mvp * vec4(inPosition, 1.0);");
            ImGui::Text("}");

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}

void ShaderDebugView::RenderUniforms() {
    if (ImGui::BeginTable("Uniforms", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 70);
        ImGui::TableSetupColumn("Set", ImGuiTableColumnFlags_WidthFixed, 35);
        ImGui::TableSetupColumn("Binding", ImGuiTableColumnFlags_WidthFixed, 50);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        // Demo uniforms
        const char* names[] = {"modelMatrix", "viewMatrix", "projMatrix", "baseColor", "metallic", "roughness"};
        const char* types[] = {"mat4", "mat4", "mat4", "vec4", "float", "float"};
        const char* values[] = {"identity", "camera", "camera", "(1,1,1,1)", "0.0", "0.5"};

        for (int i = 0; i < 6; ++i) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::Text("%s", names[i]);
            ImGui::TableNextColumn(); ImGui::TextDisabled("%s", types[i]);
            ImGui::TableNextColumn(); ImGui::Text("0");
            ImGui::TableNextColumn(); ImGui::Text("%d", i);
            ImGui::TableNextColumn(); ImGui::TextDisabled("%s", values[i]);
        }

        ImGui::EndTable();
    }
}

void ShaderDebugView::RenderStatistics() {
    if (m_SelectedShader < 0) return;
    const auto& shader = m_Shaders[m_SelectedShader];

    SectionHeader("Statistics");

    ImGui::Columns(2, nullptr, false);

    ImGui::TextDisabled("Instructions:"); ImGui::NextColumn();
    ImGui::Text("%d", shader.instructionCount); ImGui::NextColumn();

    ImGui::TextDisabled("Registers:"); ImGui::NextColumn();
    ImGui::Text("%d", shader.registerCount); ImGui::NextColumn();

    ImGui::TextDisabled("Textures:"); ImGui::NextColumn();
    ImGui::Text("%d", shader.textureCount); ImGui::NextColumn();

    ImGui::TextDisabled("Compile Time:"); ImGui::NextColumn();
    ImGui::Text("%.1f ms", shader.compileTimeMs); ImGui::NextColumn();

    ImGui::Columns(1);

    ImGui::Spacing();
    SectionHeader("Performance Notes");

    if (shader.instructionCount > 200) {
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "High instruction count - consider optimizing");
    }
    if (shader.registerCount > 24) {
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "High register usage - may reduce occupancy");
    }
    if (shader.instructionCount <= 200 && shader.registerCount <= 24) {
        ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "Shader looks efficient");
    }
}

} // namespace Yalaz::UI
