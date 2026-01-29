
#include "vk_engine.h"
#include <iostream>
#include <SDL.h>
#include <SDL_vulkan.h>
#include <thread> // std::this_thread::sleep_for kullanımı için
#include <algorithm>
#include <cmath>    // std::floor, std::abs for dynamic grid

#include <glslang/Include/visibility.h>
#include <shaderc/visibility.h>
#include "tiny_obj_loader.h"

#include "vk_loader.h"
#include "engine_state.h"

#include <vk_types.h>
#include <vk_initializers.h>

#include "VkBootstrap.h"
#include <array>
#include <fmt/core.h>
#include <fstream>

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_vulkan.h"

#include <unordered_set>  // Benzersiz shader pipeline'larını saklamak için
#include <vector>         //  Görünür objeleri saklamak için
#include <string>         //  Objelerin isimlerini saklamak için

#include <ImGuizmo.h>

// Professional UI System
#include "ui/EditorUI.h"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"
#include "vk_images.h"
#include "vk_pipelines.h"
#include "vk_descriptors.h"
#include <glm/gtx/transform.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> // glm::translate, rotate, scale için şart
#include <glm/gtc/type_ptr.hpp>


using namespace std;

// Auto-detect Debug/Release: validation ON in Debug, OFF in Release
#ifdef NDEBUG
    constexpr bool bUseValidationLayers = false;  // Release mode - max performance
#else
    constexpr bool bUseValidationLayers = true;   // Debug mode - validation enabled
#endif

// Platform-specific descriptor limits (MoltenVK has stricter limits)
#ifdef __APPLE__
    constexpr uint32_t MAX_BINDLESS_TEXTURES = 8;    // MoltenVK maxPerStageDescriptorSamplers is 16, leave plenty of room for other samplers
#else
    constexpr uint32_t MAX_BINDLESS_TEXTURES = 1024; // Desktop GPUs support more
#endif

VulkanEngine* loadedEngine = nullptr;

VulkanEngine& VulkanEngine::Get() { return *loadedEngine; }

void VulkanEngine::init()
{
    // only one engine initialization is allowed with the application.
    assert(loadedEngine == nullptr);
    loadedEngine = this;

    // We initialize SDL and create a window with it.
    SDL_Init(SDL_INIT_VIDEO);

    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

    _window = SDL_CreateWindow("Yalaz Engine", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, _windowExtent.width,
        _windowExtent.height, window_flags);

    // --- EKLENECEK KISIM ---
    int w, h;
    SDL_GetWindowSize(_window, &w, &h);
    _windowExtent.width = w;
    _windowExtent.height = h;
    // -----------------------

    SDL_SetWindowMinimumSize(_window, 64, 64); // min değer

    init_vulkan();

    init_swapchain();

    init_commands();

    init_sync_structures();
    init_scene_data();
    init_descriptors();

    init_pipelines();

    init_default_data();

    init_renderables();

    init_imgui();

    // everything went fine
    _isInitialized = true;

    mainCamera.position = glm::vec3(0, 0, 5);
    mainCamera.yaw = 0.0f;
    mainCamera.pitch = 0.0f;
}




//void VulkanEngine::init_default_data() {
//    std::array<Vertex, 4> rect_vertices;
//
//    rect_vertices[0].position = { 0.5,-0.5, 0 };
//    rect_vertices[1].position = { 0.5,0.5, 0 };
//    rect_vertices[2].position = { -0.5,-0.5, 0 };
//    rect_vertices[3].position = { -0.5,0.5, 0 };
//
//    rect_vertices[0].color = { 0,0, 0,1 };
//    rect_vertices[1].color = { 0.5,0.5,0.5 ,1 };
//    rect_vertices[2].color = { 1,0, 0,1 };
//    rect_vertices[3].color = { 0,1, 0,1 };
//
//    rect_vertices[0].uv_x = 1;
//    rect_vertices[0].uv_y = 0;
//    rect_vertices[1].uv_x = 0;
//    rect_vertices[1].uv_y = 0;
//    rect_vertices[2].uv_x = 1;
//    rect_vertices[2].uv_y = 1;
//    rect_vertices[3].uv_x = 0;
//    rect_vertices[3].uv_y = 1;
//
//    std::array<uint32_t, 6> rect_indices;
//
//    rect_indices[0] = 0;
//    rect_indices[1] = 1;
//    rect_indices[2] = 2;
//
//    rect_indices[3] = 2;
//    rect_indices[4] = 1;
//    rect_indices[5] = 3;
//
//    rectangle = uploadMesh(rect_indices, rect_vertices);
//
//    //3 default textures, white, grey, black. 1 pixel each
//    uint32_t white = glm::packUnorm4x8(glm::vec4(1, 1, 1, 1));
//    _whiteImage = create_image((void*)&white, VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R8G8B8A8_UNORM,
//        VK_IMAGE_USAGE_SAMPLED_BIT);
//
//    uint32_t grey = glm::packUnorm4x8(glm::vec4(0.66f, 0.66f, 0.66f, 1));
//    _greyImage = create_image((void*)&grey, VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R8G8B8A8_UNORM,
//        VK_IMAGE_USAGE_SAMPLED_BIT);
//
//    uint32_t black = glm::packUnorm4x8(glm::vec4(0, 0, 0, 0));
//    _blackImage = create_image((void*)&black, VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R8G8B8A8_UNORM,
//        VK_IMAGE_USAGE_SAMPLED_BIT);
//
//    //checkerboard image
//    uint32_t magenta = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));
//    std::array<uint32_t, 16 * 16 > pixels; //for 16x16 checkerboard texture
//    for (int x = 0; x < 16; x++) {
//        for (int y = 0; y < 16; y++) {
//            pixels[y * 16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
//        }
//    }
//
//    _errorCheckerboardImage = create_image(pixels.data(), VkExtent3D{ 16, 16, 1 }, VK_FORMAT_R8G8B8A8_UNORM,
//        VK_IMAGE_USAGE_SAMPLED_BIT);
//
//    VkSamplerCreateInfo sampl = { .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
//
//    sampl.magFilter = VK_FILTER_NEAREST;
//    sampl.minFilter = VK_FILTER_NEAREST;
//
//    vkCreateSampler(_device, &sampl, nullptr, &_defaultSamplerNearest);
//
//    sampl.magFilter = VK_FILTER_LINEAR;
//    sampl.minFilter = VK_FILTER_LINEAR;
//    vkCreateSampler(_device, &sampl, nullptr, &_defaultSamplerLinear);
//}


//void VulkanEngine::init_default_data() {
//    std::array<Vertex, 24> cubeVertices;
//
//    // === FRONT (face 0)
//    cubeVertices[0].position = { -1, -1,  1 };
//    cubeVertices[1].position = { 1, -1,  1 };
//    cubeVertices[2].position = { 1,  1,  1 };
//    cubeVertices[3].position = { -1,  1,  1 };
//
//    // === RIGHT (face 1)
//    cubeVertices[4].position = { 1, -1,  1 };
//    cubeVertices[5].position = { 1, -1, -1 };
//    cubeVertices[6].position = { 1,  1, -1 };
//    cubeVertices[7].position = { 1,  1,  1 };
//
//    // === BACK (face 2)
//    cubeVertices[8].position = { 1, -1, -1 };
//    cubeVertices[9].position = { -1, -1, -1 };
//    cubeVertices[10].position = { -1,  1, -1 };
//    cubeVertices[11].position = { 1,  1, -1 };
//
//    // === LEFT (face 3)
//    cubeVertices[12].position = { -1, -1, -1 };
//    cubeVertices[13].position = { -1, -1,  1 };
//    cubeVertices[14].position = { -1,  1,  1 };
//    cubeVertices[15].position = { -1,  1, -1 };
//
//    // === TOP (face 4)
//    cubeVertices[16].position = { -1, 1,  1 };
//    cubeVertices[17].position = { 1, 1,  1 };
//    cubeVertices[18].position = { 1, 1, -1 };
//    cubeVertices[19].position = { -1, 1, -1 };
//
//    // === BOTTOM (face 5)
//    cubeVertices[20].position = { -1, -1, -1 };
//    cubeVertices[21].position = { 1, -1, -1 };
//    cubeVertices[22].position = { 1, -1,  1 };
//    cubeVertices[23].position = { -1, -1,  1 };
//
//    // === Her yüzeye karşılık gelen renkler (debug için)
//    glm::vec4 faceColors[6] = {
//        { 1, 0, 0, 1 },   // Front - kırmızı
//        { 0, 1, 0, 1 },   // Right - yeşil
//        { 0, 0, 1, 1 },   // Back - mavi
//        { 1, 1, 0, 1 },   // Left - sarı
//        { 1, 0, 1, 1 },   // Top - mor
//        { 0, 1, 1, 1 }    // Bottom - camgöbeği
//    };
//
//    // === Ortak veriler
//    for (int f = 0; f < 6; f++) {
//        for (int v = 0; v < 4; v++) {
//            int idx = f * 4 + v;
//            cubeVertices[idx].normal = glm::vec3(0.0f);
//            cubeVertices[idx].uv_x = 0.0f;
//            cubeVertices[idx].uv_y = 0.0f;
//            cubeVertices[idx].color = faceColors[f]; // sadece debug için
//        }
//    }
//
//    std::array<uint32_t, 36> cubeIndices = {
//        0, 1, 2,  2, 3, 0,         // Front
//        4, 5, 6,  6, 7, 4,         // Right
//        8, 9,10, 10,11, 8,         // Back
//       12,13,14, 14,15,12,         // Left
//       16,17,18, 18,19,16,         // Top
//       20,21,22, 22,23,20          // Bottom
//    };
//
//    // === Mesh’i GPU’ya yükle
//    rectangle = uploadMesh(cubeIndices, cubeVertices);
//
//    // === Texture ve Sampler kısmı aynı kalır ===
//    uint32_t white = glm::packUnorm4x8(glm::vec4(1, 1, 1, 1));
//    _whiteImage = create_image((void*)&white, VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);
//
//    uint32_t grey = glm::packUnorm4x8(glm::vec4(0.66f, 0.66f, 0.66f, 1));
//    _greyImage = create_image((void*)&grey, VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);
//
//    uint32_t black = glm::packUnorm4x8(glm::vec4(0, 0, 0, 0));
//    _blackImage = create_image((void*)&black, VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);
//
//    uint32_t magenta = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));
//    std::array<uint32_t, 16 * 16> pixels;
//    for (int x = 0; x < 16; x++) {
//        for (int y = 0; y < 16; y++) {
//            pixels[y * 16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
//        }
//    }
//
//    _errorCheckerboardImage = create_image(pixels.data(), VkExtent3D{ 16, 16, 1 }, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);
//
//    VkSamplerCreateInfo sampl = { .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
//    sampl.magFilter = VK_FILTER_NEAREST;
//    sampl.minFilter = VK_FILTER_NEAREST;
//    vkCreateSampler(_device, &sampl, nullptr, &_defaultSamplerNearest);
//
//    sampl.magFilter = VK_FILTER_LINEAR;
//    sampl.minFilter = VK_FILTER_LINEAR;
//    vkCreateSampler(_device, &sampl, nullptr, &_defaultSamplerLinear);
//}


//void VulkanEngine::init_default_data() {
//    std::array<Vertex, 4> rect_vertices;
//
//    rect_vertices[0].position = { 0.5,-0.5, 0 };
//    rect_vertices[1].position = { 0.5,0.5, 0 };
//    rect_vertices[2].position = { -0.5,-0.5, 0 };
//    rect_vertices[3].position = { -0.5,0.5, 0 };
//
//    rect_vertices[0].color = { 0,0, 0,1 };
//    rect_vertices[1].color = { 0.5,0.5,0.5 ,1 };
//    rect_vertices[2].color = { 1,0, 0,1 };
//    rect_vertices[3].color = { 0,1, 0,1 };
//
//    rect_vertices[0].uv_x = 1;
//    rect_vertices[0].uv_y = 0;
//    rect_vertices[1].uv_x = 0;
//    rect_vertices[1].uv_y = 0;
//    rect_vertices[2].uv_x = 1;
//    rect_vertices[2].uv_y = 1;
//    rect_vertices[3].uv_x = 0;
//    rect_vertices[3].uv_y = 1;
//
//    std::array<uint32_t, 6> rect_indices;
//
//    rect_indices[0] = 0;
//    rect_indices[1] = 1;
//    rect_indices[2] = 2;
//
//    rect_indices[3] = 2;
//    rect_indices[4] = 1;
//    rect_indices[5] = 3;
//
//    rectangle = uploadMesh(rect_indices, rect_vertices);
//
//    //3 default textures, white, grey, black. 1 pixel each
//    uint32_t white = glm::packUnorm4x8(glm::vec4(1, 1, 1, 1));
//    _whiteImage = create_image((void*)&white, VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R8G8B8A8_UNORM,
//        VK_IMAGE_USAGE_SAMPLED_BIT);
//
//    uint32_t grey = glm::packUnorm4x8(glm::vec4(0.66f, 0.66f, 0.66f, 1));
//    _greyImage = create_image((void*)&grey, VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R8G8B8A8_UNORM,
//        VK_IMAGE_USAGE_SAMPLED_BIT);
//
//    uint32_t black = glm::packUnorm4x8(glm::vec4(0, 0, 0, 0));
//    _blackImage = create_image((void*)&black, VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R8G8B8A8_UNORM,
//        VK_IMAGE_USAGE_SAMPLED_BIT);
//
//    //checkerboard image
//    uint32_t magenta = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));
//    std::array<uint32_t, 16 * 16 > pixels; //for 16x16 checkerboard texture
//    for (int x = 0; x < 16; x++) {
//        for (int y = 0; y < 16; y++) {
//            pixels[y * 16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
//        }
//    }
//
//    _errorCheckerboardImage = create_image(pixels.data(), VkExtent3D{ 16, 16, 1 }, VK_FORMAT_R8G8B8A8_UNORM,
//        VK_IMAGE_USAGE_SAMPLED_BIT);
//
//    VkSamplerCreateInfo sampl = { .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
//
//    sampl.magFilter = VK_FILTER_NEAREST;
//    sampl.minFilter = VK_FILTER_NEAREST;
//
//    vkCreateSampler(_device, &sampl, nullptr, &_defaultSamplerNearest);
//
//    sampl.magFilter = VK_FILTER_LINEAR;
//    sampl.minFilter = VK_FILTER_LINEAR;
//    vkCreateSampler(_device, &sampl, nullptr, &_defaultSamplerLinear);
//}




//void VulkanEngine::draw_geometry(VkCommandBuffer cmd)
//{
//    stats.drawcall_count = 0;
//    stats.triangle_count = 0;
//
//    // === GPU Scene Data Buffer ===
//    AllocatedBuffer gpuSceneDataBuffer = create_buffer(sizeof(GPUSceneData),
//        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
//        VMA_MEMORY_USAGE_CPU_TO_GPU);
//
//    get_current_frame()._deletionQueue.push_function([=, this]() {
//        destroy_buffer(gpuSceneDataBuffer);
//        });
//
//    GPUSceneData* sceneUniformData = (GPUSceneData*)gpuSceneDataBuffer.allocation->GetMappedData();
//    *sceneUniformData = sceneData;
//
//    // === Variable Descriptor Count ===
//    VkDescriptorSetVariableDescriptorCountAllocateInfo allocArrayInfo{
//        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO
//    };
//
//    uint32_t descriptorCounts = static_cast<uint32_t>(texCache.Cache.size());
//    allocArrayInfo.pDescriptorCounts = &descriptorCounts;
//    allocArrayInfo.descriptorSetCount = 1;
//
//    VkDescriptorSet globalDescriptor = get_current_frame()._frameDescriptors.allocate(
//        _device, _gpuSceneDataDescriptorLayout, &allocArrayInfo);
//
//    DescriptorWriter writer;
//    writer.write_buffer(0, gpuSceneDataBuffer.buffer, sizeof(GPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
//
//    if (!texCache.Cache.empty()) {
//        VkWriteDescriptorSet arraySet{ .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
//        arraySet.descriptorCount = static_cast<uint32_t>(texCache.Cache.size());
//        arraySet.dstArrayElement = 0;
//        arraySet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
//        arraySet.dstBinding = 1;
//        arraySet.pImageInfo = texCache.Cache.data();
//        writer.writes.push_back(arraySet);
//    }
//
//    writer.update_set(_device, globalDescriptor);
//
//    // === Viewport & Scissor ===
//    VkViewport viewport = get_letterbox_viewport();
//    vkCmdSetViewport(cmd, 0, 1, &viewport);
//
//    VkRect2D scissor{};
//    scissor.offset = { 0, 0 };
//    scissor.extent = _windowExtent;
//    vkCmdSetScissor(cmd, 0, 1, &scissor);
//
//    // === GRID YARDIMCI DÜZLEMİNİ ÇİZ ===
//    {
//        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, gridPipeline);
//
//        VkDescriptorSet descriptorSets[] = { globalDescriptor };
//        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, gridPipelineLayout,
//            0, 1, descriptorSets, 0, nullptr);
//
//        GPUDrawPushConstants push{};
//        push.worldMatrix = glm::mat4(1.0f);
//        push.vertexBuffer = 0;
//        memset(push.faceColors, 0, sizeof(push.faceColors));
//
//        vkCmdPushConstants(cmd, gridPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
//            sizeof(GPUDrawPushConstants), &push);
//
//        vkCmdDraw(cmd, 6, 1, 0, 0); // fullscreen quad
//    }
//
//    // === SHADER-ONLY KÜPLERİ ÇİZ ===
//    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _2dPipeline);
//    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _2dPipelineLayout,
//        0, 1, &globalDescriptor, 0, nullptr);
//
//    VkDeviceSize offsets[] = { 0 };
//
//    for (auto& shape : static_shapes) {
//        vkCmdBindVertexBuffers(cmd, 0, 1, &shape.mesh.vertexBuffer.buffer, offsets);
//        vkCmdBindIndexBuffer(cmd, shape.mesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
//
//        GPUDrawPushConstants push{};
//        push.worldMatrix = shape.get_transform();
//        push.vertexBuffer = shape.mesh.vertexBufferAddress;
//        memcpy(push.faceColors, shape.faceColors, sizeof(glm::vec4) * 6);
//
//        vkCmdPushConstants(cmd, _2dPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0,
//            sizeof(GPUDrawPushConstants), &push);
//
//        vkCmdDrawIndexed(cmd, shape.mesh.indexCount, 1, 0, 0, 0);
//    }
//
//    // === 3D SAHNE OBJELERİNİ ÇİZ ===
//    std::vector<uint32_t> opaque_draws;
//    opaque_draws.reserve(drawCommands.OpaqueSurfaces.size());
//
//    for (uint32_t i = 0; i < drawCommands.OpaqueSurfaces.size(); i++) {
//        if (is_visible(drawCommands.OpaqueSurfaces[i], sceneData.viewproj)) {
//            opaque_draws.push_back(i);
//        }
//    }
//
//    std::sort(opaque_draws.begin(), opaque_draws.end(), [&](uint32_t a, uint32_t b) {
//        const RenderObject& A = drawCommands.OpaqueSurfaces[a];
//        const RenderObject& B = drawCommands.OpaqueSurfaces[b];
//        return A.material == B.material ? A.indexBuffer < B.indexBuffer : A.material < B.material;
//        });
//
//    MaterialPipeline* lastPipeline = nullptr;
//    MaterialInstance* lastMaterial = nullptr;
//    VkBuffer lastIndexBuffer = VK_NULL_HANDLE;
//
//    auto draw = [&](const RenderObject& r) {
//        if (!r.material || !r.material->pipeline) return;
//
//        if (r.material != lastMaterial) {
//            lastMaterial = r.material;
//
//            if (r.material->pipeline != lastPipeline) {
//                lastPipeline = r.material->pipeline;
//                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.material->pipeline->pipeline);
//                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.material->pipeline->layout,
//                    0, 1, &globalDescriptor, 0, nullptr);
//
//                vkCmdSetViewport(cmd, 0, 1, &viewport);
//                vkCmdSetScissor(cmd, 0, 1, &scissor);
//            }
//
//            if (r.material->materialSet != VK_NULL_HANDLE) {
//                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.material->pipeline->layout,
//                    1, 1, &r.material->materialSet, 0, nullptr);
//            }
//        }
//
//        if (r.indexBuffer != lastIndexBuffer) {
//            lastIndexBuffer = r.indexBuffer;
//            vkCmdBindIndexBuffer(cmd, r.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
//        }
//
//        GPUDrawPushConstants push{};
//        push.worldMatrix = r.transform;
//        push.vertexBuffer = r.vertexBufferAddress;
//
//        vkCmdPushConstants(cmd, r.material->pipeline->layout, VK_SHADER_STAGE_VERTEX_BIT, 0,
//            sizeof(GPUDrawPushConstants), &push);
//
//        stats.drawcall_count++;
//        stats.triangle_count += r.indexCount / 3;
//
//        vkCmdDrawIndexed(cmd, r.indexCount, 1, r.firstIndex, 0, 0);
//        };
//
//    for (auto& idx : opaque_draws) draw(drawCommands.OpaqueSurfaces[idx]);
//    for (auto& r : drawCommands.TransparentSurfaces) draw(r);
//
//    stats.visible_count = static_cast<int>(opaque_draws.size() + drawCommands.TransparentSurfaces.size());
//
//    drawCommands.OpaqueSurfaces.clear();
//    drawCommands.TransparentSurfaces.clear();
//}
//




//void VulkanEngine::init_default_data() {
//    // 🧊 8 köşe tanımlı, 12 üçgen = 36 index
//    std::array<Vertex, 8> cube_vertices;
//
//    cube_vertices[0].position = { -1.0f, -1.0f, -1.0f }; // Sol alt arka
//    cube_vertices[1].position = { 1.0f, -1.0f, -1.0f }; // Sağ alt arka
//    cube_vertices[2].position = { 1.0f,  1.0f, -1.0f }; // Sağ üst arka
//    cube_vertices[3].position = { -1.0f,  1.0f, -1.0f }; // Sol üst arka
//
//    cube_vertices[4].position = { -1.0f, -1.0f,  1.0f }; // Sol alt ön
//    cube_vertices[5].position = { 1.0f, -1.0f,  1.0f }; // Sağ alt ön
//    cube_vertices[6].position = { 1.0f,  1.0f,  1.0f }; // Sağ üst ön
//    cube_vertices[7].position = { -1.0f,  1.0f,  1.0f }; // Sol üst ön
//
//    // Renkleri köşelere göre ayarla
//    for (int i = 0; i < 8; i++) {
//        cube_vertices[i].color = glm::vec4(
//            (i & 1) ? 1.0f : 0.0f,
//            (i & 2) ? 1.0f : 0.0f,
//            (i & 4) ? 1.0f : 0.0f,
//            1.0f
//        );
//        cube_vertices[i].uv_x = (i & 1) ? 1.0f : 0.0f;
//        cube_vertices[i].uv_y = (i & 2) ? 1.0f : 0.0f;
//    }
//
//    std::array<uint32_t, 36> cube_indices = {
//        // Arka yüz
//        0, 1, 2,  2, 3, 0,
//        // Ön yüz
//        4, 6, 5,  6, 4, 7,
//        // Sol yüz
//        4, 0, 3,  3, 7, 4,
//        // Sağ yüz
//        1, 5, 6,  6, 2, 1,
//        // Alt yüz
//        4, 5, 1,  1, 0, 4,
//        // Üst yüz
//        3, 2, 6,  6, 7, 3
//    };
//
//    rectangle = uploadMesh(cube_indices, cube_vertices);
//
//    // 📦 1x1'lik doku (white, grey, black)
//    uint32_t white = glm::packUnorm4x8(glm::vec4(1, 1, 1, 1));
//    _whiteImage = create_image((void*)&white, VkExtent3D{ 1, 1, 1 },
//        VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);
//
//    uint32_t grey = glm::packUnorm4x8(glm::vec4(0.66f, 0.66f, 0.66f, 1));
//    _greyImage = create_image((void*)&grey, VkExtent3D{ 1, 1, 1 },
//        VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);
//
//    uint32_t black = glm::packUnorm4x8(glm::vec4(0, 0, 0, 0));
//    _blackImage = create_image((void*)&black, VkExtent3D{ 1, 1, 1 },
//        VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);
//
//    // 🧩 16x16 checkerboard pattern
//    uint32_t magenta = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));
//    std::array<uint32_t, 16 * 16> pixels;
//    for (int x = 0; x < 16; x++) {
//        for (int y = 0; y < 16; y++) {
//            pixels[y * 16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
//        }
//    }
//
//    _errorCheckerboardImage = create_image(pixels.data(), VkExtent3D{ 16, 16, 1 },
//        VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);
//
//    // 🔄 Sampler oluştur
//    VkSamplerCreateInfo sampl = { .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
//
//    sampl.magFilter = VK_FILTER_NEAREST;
//    sampl.minFilter = VK_FILTER_NEAREST;
//    vkCreateSampler(_device, &sampl, nullptr, &_defaultSamplerNearest);
//
//    sampl.magFilter = VK_FILTER_LINEAR;
//    sampl.minFilter = VK_FILTER_LINEAR;
//    vkCreateSampler(_device, &sampl, nullptr, &_defaultSamplerLinear);
//}







void VulkanEngine::cleanup()
{
    if (_isInitialized) {

        // Make sure the GPU has stopped doing its things
        vkDeviceWaitIdle(_device);

        // 1. Shutdown EditorUI FIRST (while ImGui is still fully active)
        Yalaz::UI::EditorUI::Get().Shutdown();

        // 2. Flush frame deletion queues (contains per-frame resources from draw_geometry)
        for (auto& frame : _frames) {
            frame._deletionQueue.flush();
        }

        // 3. Clear static_shapes (DO NOT destroy buffers - they are owned by other systems)
        // static_shapes only holds references to meshes owned by:
        // - _cachedLightSphereMesh (cleaned up below)
        // - defaultMeshes (cleaned up below)
        // - loadedScenes (cleaned up by their destructors)
        static_shapes.clear();

        // 4. Clear loaded GLTF scenes (calls destructors which clean up their resources)
        loadedScenes.clear();

        // 5. Flush main deletion queue (contains ImGui_ImplVulkan_Shutdown, descriptor pools, pipelines, etc.)
        _mainDeletionQueue.flush();

        // 6. Shutdown ImGui SDL backend and destroy context AFTER Vulkan backend shutdown
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();

        // 7. Explicitly destroy VMA resources that might have been missed by deletion queues
        // Draw and depth images
        if (_drawImage.image != VK_NULL_HANDLE) {
            vkDestroyImageView(_device, _drawImage.imageView, nullptr);
            vmaDestroyImage(_allocator, _drawImage.image, _drawImage.allocation);
            _drawImage.image = VK_NULL_HANDLE;
        }
        if (_depthImage.image != VK_NULL_HANDLE) {
            vkDestroyImageView(_device, _depthImage.imageView, nullptr);
            vmaDestroyImage(_allocator, _depthImage.image, _depthImage.allocation);
            _depthImage.image = VK_NULL_HANDLE;
        }

        // Default images
        if (_whiteImage.image != VK_NULL_HANDLE) {
            destroy_image(_whiteImage);
            _whiteImage.image = VK_NULL_HANDLE;
        }
        if (_greyImage.image != VK_NULL_HANDLE) {
            destroy_image(_greyImage);
            _greyImage.image = VK_NULL_HANDLE;
        }
        if (_blackImage.image != VK_NULL_HANDLE) {
            destroy_image(_blackImage);
            _blackImage.image = VK_NULL_HANDLE;
        }
        if (_errorCheckerboardImage.image != VK_NULL_HANDLE) {
            destroy_image(_errorCheckerboardImage);
            _errorCheckerboardImage.image = VK_NULL_HANDLE;
        }

        // Default samplers
        if (_defaultSamplerNearest != VK_NULL_HANDLE) {
            vkDestroySampler(_device, _defaultSamplerNearest, nullptr);
            _defaultSamplerNearest = VK_NULL_HANDLE;
        }
        if (_defaultSamplerLinear != VK_NULL_HANDLE) {
            vkDestroySampler(_device, _defaultSamplerLinear, nullptr);
            _defaultSamplerLinear = VK_NULL_HANDLE;
        }

        // Scene data buffers
        for (size_t i = 0; i < FRAME_OVERLAP; i++) {
            if (_frames[i].sceneDataBuffer.buffer != VK_NULL_HANDLE) {
                destroy_buffer(_frames[i].sceneDataBuffer);
                _frames[i].sceneDataBuffer.buffer = VK_NULL_HANDLE;
            }
        }

        // Cached light sphere mesh
        if (_lightMeshCached && _cachedLightSphereMesh.vertexBuffer.buffer != VK_NULL_HANDLE) {
            destroy_buffer(_cachedLightSphereMesh.indexBuffer);
            destroy_buffer(_cachedLightSphereMesh.vertexBuffer);
            _lightMeshCached = false;
        }

        // Default meshes
        for (auto& [type, mesh] : defaultMeshes) {
            if (mesh.vertexBuffer.buffer != VK_NULL_HANDLE) {
                destroy_buffer(mesh.indexBuffer);
                destroy_buffer(mesh.vertexBuffer);
            }
        }
        defaultMeshes.clear();

        // Rectangle mesh
        if (rectangle.vertexBuffer.buffer != VK_NULL_HANDLE) {
            destroy_buffer(rectangle.indexBuffer);
            destroy_buffer(rectangle.vertexBuffer);
            rectangle.vertexBuffer.buffer = VK_NULL_HANDLE;
        }

        // Grid mesh
        if (gridMesh.vertexBuffer.buffer != VK_NULL_HANDLE) {
            destroy_buffer(gridMesh.indexBuffer);
            destroy_buffer(gridMesh.vertexBuffer);
            gridMesh.vertexBuffer.buffer = VK_NULL_HANDLE;
        }

        // Material cube mesh
        if (_materialCubeMesh.vertexBuffer.buffer != VK_NULL_HANDLE) {
            destroy_buffer(_materialCubeMesh.indexBuffer);
            destroy_buffer(_materialCubeMesh.vertexBuffer);
            _materialCubeMesh.vertexBuffer.buffer = VK_NULL_HANDLE;
        }

        // Point light buffer
        if (pointLightBuffer.buffer != VK_NULL_HANDLE) {
            destroy_buffer(pointLightBuffer);
            pointLightBuffer.buffer = VK_NULL_HANDLE;
        }

        // Default GLTF material data buffer
        if (_defaultGLTFMaterialData.buffer != VK_NULL_HANDLE) {
            destroy_buffer(_defaultGLTFMaterialData);
            _defaultGLTFMaterialData.buffer = VK_NULL_HANDLE;
        }

        // 8. Destroy swapchain
        destroy_swapchain();

        // 9. Destroy surface (instance-level object)
        vkDestroySurfaceKHR(_instance, _surface, nullptr);

        // 10. Destroy VMA allocator (must be before device)
        vmaDestroyAllocator(_allocator);

        // 11. Destroy device
        vkDestroyDevice(_device, nullptr);

        // 12. Destroy debug messenger and instance
        if (bUseValidationLayers && _debug_messenger != VK_NULL_HANDLE) {
            vkb::destroy_debug_utils_messenger(_instance, _debug_messenger);
        }
        vkDestroyInstance(_instance, nullptr);

        // 13. SDL cleanup (must be last)
        SDL_DestroyWindow(_window);
        SDL_Quit();

        _isInitialized = false;
    }
}

//void VulkanEngine::init_scene_data() {
//    for (int i = 0; i < FRAME_OVERLAP; i++) {
//        _frames[i].sceneDataBuffer = create_buffer(
//            sizeof(GPUSceneData),
//            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
//            VMA_MEMORY_USAGE_CPU_TO_GPU
//        );
//    }
//}


void VulkanEngine::init_scene_data() {
    // Her frame için SceneData buffer oluştur
    for (int i = 0; i < FRAME_OVERLAP; i++) {
        _frames[i].sceneDataBuffer = create_buffer(
            sizeof(GPUSceneData),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU
        );
    }

    // Note: Scene data buffers are cleaned up explicitly in cleanup()

    // === Initial Lighting Setup ===
    sceneData.ambientColor = glm::vec4(0.1f, 0.1f, 0.1f, 0.5f); // RGB + intensity
    glm::vec3 sunDir = glm::normalize(glm::vec3(-1.0f, -3.0f, -1.0f));
    sceneData.sunlightDirection = glm::vec4(sunDir, 3.0f); // direction + intensity
    sceneData.sunlightColor = glm::vec4(1.0f, 0.95f, 0.8f, 1.0f); // warm white

    // === Camera Position (will be updated each frame) ===
    sceneData.cameraPosition = glm::vec4(0.0f, 0.0f, 5.0f, 1.0f);

    // === Point Light Array Initialization ===
    sceneData.pointLightCount = 0;
}



//void VulkanEngine::init_background_pipelines()
//{
//    VkPipelineLayoutCreateInfo computeLayout{};
//    computeLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
//    computeLayout.pNext = nullptr;
//    computeLayout.pSetLayouts = &_drawImageDescriptorLayout;
//    computeLayout.setLayoutCount = 1;
//
//    VkPushConstantRange pushConstant{};
//    pushConstant.offset = 0;
//    pushConstant.size = sizeof(ComputePushConstants);
//    pushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
//
//    computeLayout.pPushConstantRanges = &pushConstant;
//    computeLayout.pushConstantRangeCount = 1;
//
//    VK_CHECK(vkCreatePipelineLayout(_device, &computeLayout, nullptr, &_gradientPipelineLayout));
//
//    VkShaderModule gradientShader;
//    if (!vkutil::load_shader_module("../../shaders/gradient_color.comp.spv", _device, &gradientShader)) {
//        fmt::print("Error when building the compute shader \n");
//    }
//
//    VkShaderModule skyShader;
//    if (!vkutil::load_shader_module("../../shaders/sky.comp.spv", _device, &skyShader)) {
//        fmt::print("Error when building the compute shader\n");
//    }
//
//    VkShaderModule gridShader;
//    if (!vkutil::load_shader_module("../../shaders/grid.comp.spv", _device, &gridShader)) {
//        fmt::print("Error when building the compute shader\n");
//    }
//    VkShaderModule pathtraceShader;
//    if (!vkutil::load_shader_module("../../shaders/pathtrace.comp.spv", _device, &pathtraceShader)) {
//        fmt::print("Error when building path tracer shader\n");
//    }
//
//    VkPipelineShaderStageCreateInfo stageinfo{};
//    stageinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
//    stageinfo.pNext = nullptr;
//    stageinfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
//    stageinfo.module = gradientShader;
//    stageinfo.pName = "main";
//
//    VkComputePipelineCreateInfo computePipelineCreateInfo{};
//    computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
//    computePipelineCreateInfo.pNext = nullptr;
//    computePipelineCreateInfo.layout = _gradientPipelineLayout;
//    computePipelineCreateInfo.stage = stageinfo;
//
//    ComputeEffect gradient;
//    gradient.layout = _gradientPipelineLayout;
//    gradient.name = "gradient";
//    gradient.data = {};
//
//    //default colors
//    gradient.data.data1 = glm::vec4(1, 0, 0, 1);
//    gradient.data.data2 = glm::vec4(0, 0, 1, 1);
//
//    VK_CHECK(vkCreateComputePipelines(_device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &gradient.pipeline));
//
//    //change the shader module only to create the sky shader
//    computePipelineCreateInfo.stage.module = skyShader;
//
//    ComputeEffect sky;
//    sky.layout = _gradientPipelineLayout;
//    sky.name = "sky";
//    sky.data = {};
//    //default sky parameters
//    sky.data.data1 = glm::vec4(0.1, 0.2, 0.4, 0.97);
//
//
//    computePipelineCreateInfo.stage.module = pathtraceShader;
//
//    ComputeEffect pathtrace;
//    pathtrace.layout = _gradientPipelineLayout;
//    pathtrace.name = "pathtracer";
//    pathtrace.data = {};
//
//    VK_CHECK(vkCreateComputePipelines(_device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &pathtrace.pipeline));
//
//    // Efektler listesine ekle:
//    backgroundEffects.push_back(pathtrace);
//
//    // ShaderModule temizle:
//    vkDestroyShaderModule(_device, pathtraceShader, nullptr);
//
//
//
//
//
//    VK_CHECK(vkCreateComputePipelines(_device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &sky.pipeline));
//
//    //change the shader module only to create the grid shader
//    computePipelineCreateInfo.stage.module = gridShader;
//
//    ComputeEffect grid;
//    grid.layout = _gradientPipelineLayout;
//    grid.name = "grid";
//    grid.data = {};
//
//    VK_CHECK(vkCreateComputePipelines(_device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &grid.pipeline));
//
//    //add the 3 background effects into the array
//    backgroundEffects.push_back(gradient);
//    backgroundEffects.push_back(sky);
//    backgroundEffects.push_back(grid);
//
//    //destroy structures properly
//    vkDestroyShaderModule(_device, gradientShader, nullptr);
//    vkDestroyShaderModule(_device, skyShader, nullptr);
//    vkDestroyShaderModule(_device, gridShader, nullptr);
//    _mainDeletionQueue.push_function([&]() {
//        vkDestroyPipelineLayout(_device, _gradientPipelineLayout, nullptr);
//        vkDestroyPipeline(_device, sky.pipeline, nullptr);
//        vkDestroyPipeline(_device, gradient.pipeline, nullptr);
//        vkDestroyPipeline(_device, grid.pipeline, nullptr);
//        });
//}
void VulkanEngine::init_background_pipelines()
{
    VkPipelineLayoutCreateInfo computeLayout{};
    computeLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    computeLayout.setLayoutCount = 1;
    computeLayout.pSetLayouts = &_drawImageDescriptorLayout;

    VkPushConstantRange pushConstant{};
    pushConstant.offset = 0;
    pushConstant.size = sizeof(ComputePushConstants);
    pushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    computeLayout.pushConstantRangeCount = 1;
    computeLayout.pPushConstantRanges = &pushConstant;

    VK_CHECK(vkCreatePipelineLayout(_device, &computeLayout, nullptr, &_gradientPipelineLayout));

    auto create_compute_pipeline = [&](const char* shaderPath, ComputeEffect& effect, const char* effectName) {
        VkShaderModule shaderModule;
        if (!vkutil::load_shader_module(shaderPath, _device, &shaderModule)) {
            fmt::print("Error loading compute shader: {}\n", shaderPath);
            return;
        }

        VkPipelineShaderStageCreateInfo stageinfo{};
        stageinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stageinfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        stageinfo.module = shaderModule;
        stageinfo.pName = "main";

        VkComputePipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipelineInfo.layout = _gradientPipelineLayout;
        pipelineInfo.stage = stageinfo;

        VK_CHECK(vkCreateComputePipelines(_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &effect.pipeline));

        effect.layout = _gradientPipelineLayout;
        effect.name = effectName;
        effect.data = {}; // Default push constant verisi (opsiyonel)

        backgroundEffects.push_back(effect);

        vkDestroyShaderModule(_device, shaderModule, nullptr);
        };

    ComputeEffect gradient;
    gradient.data.data1 = glm::vec4(1, 0, 0, 1);
    gradient.data.data2 = glm::vec4(0, 0, 1, 1);
    create_compute_pipeline("../../shaders/gradient_color.comp.spv", gradient, "gradient");

    ComputeEffect sky;
    sky.data.data1 = glm::vec4(0.1, 0.2, 0.4, 0.97);
    create_compute_pipeline("../../shaders/sky.comp.spv", sky, "sky");

    ComputeEffect grid;
    create_compute_pipeline("../../shaders/grid.comp.spv", grid, "grid");

    //ComputeEffect pathtrace;
    //create_compute_pipeline("../../shaders/pathtrace.comp.spv", pathtrace, "pathtrace");

    _mainDeletionQueue.push_function([&]() {
        vkDestroyPipelineLayout(_device, _gradientPipelineLayout, nullptr);
        for (auto& e : backgroundEffects) {
            vkDestroyPipeline(_device, e.pipeline, nullptr);
        }
        });
}


//void VulkanEngine::init_default_data() {
//    std::array<Vertex, 4> rect_vertices;
//
//    rect_vertices[0].position = { 0.5,-0.5, 0 };
//    rect_vertices[1].position = { 0.5,0.5, 0 };
//    rect_vertices[2].position = { -0.5,-0.5, 0 };
//    rect_vertices[3].position = { -0.5,0.5, 0 };
//
//    rect_vertices[0].color = { 0,0, 0,1 };
//    rect_vertices[1].color = { 0.5,0.5,0.5 ,1 };
//    rect_vertices[2].color = { 1,0, 0,1 };
//    rect_vertices[3].color = { 0,1, 0,1 };
//
//    rect_vertices[0].uv_x = 1;
//    rect_vertices[0].uv_y = 0;
//    rect_vertices[1].uv_x = 0;
//    rect_vertices[1].uv_y = 0;
//    rect_vertices[2].uv_x = 1;
//    rect_vertices[2].uv_y = 1;
//    rect_vertices[3].uv_x = 0;
//    rect_vertices[3].uv_y = 1;
//
//    std::array<uint32_t, 6> rect_indices;
//
//    rect_indices[0] = 0;
//    rect_indices[1] = 1;
//    rect_indices[2] = 2;
//
//    rect_indices[3] = 2;
//    rect_indices[4] = 1;
//    rect_indices[5] = 3;
//
//    rectangle = uploadMesh(rect_indices, rect_vertices);
//
//    //3 default textures, white, grey, black. 1 pixel each
//    uint32_t white = glm::packUnorm4x8(glm::vec4(1, 1, 1, 1));
//    _whiteImage = create_image((void*)&white, VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R8G8B8A8_UNORM,
//        VK_IMAGE_USAGE_SAMPLED_BIT);
//
//    uint32_t grey = glm::packUnorm4x8(glm::vec4(0.66f, 0.66f, 0.66f, 1));
//    _greyImage = create_image((void*)&grey, VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R8G8B8A8_UNORM,
//        VK_IMAGE_USAGE_SAMPLED_BIT);
//
//    uint32_t black = glm::packUnorm4x8(glm::vec4(0, 0, 0, 0));
//    _blackImage = create_image((void*)&black, VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R8G8B8A8_UNORM,
//        VK_IMAGE_USAGE_SAMPLED_BIT);
//
//    //checkerboard image
//    uint32_t magenta = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));
//    std::array<uint32_t, 16 * 16 > pixels; //for 16x16 checkerboard texture
//    for (int x = 0; x < 16; x++) {
//        for (int y = 0; y < 16; y++) {
//            pixels[y * 16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
//        }
//    }
//
//    _errorCheckerboardImage = create_image(pixels.data(), VkExtent3D{ 16, 16, 1 }, VK_FORMAT_R8G8B8A8_UNORM,
//        VK_IMAGE_USAGE_SAMPLED_BIT);
//
//    VkSamplerCreateInfo sampl = { .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
//
//    sampl.magFilter = VK_FILTER_NEAREST;
//    sampl.minFilter = VK_FILTER_NEAREST;
//
//    vkCreateSampler(_device, &sampl, nullptr, &_defaultSamplerNearest);
//
//    sampl.magFilter = VK_FILTER_LINEAR;
//    sampl.minFilter = VK_FILTER_LINEAR;
//    vkCreateSampler(_device, &sampl, nullptr, &_defaultSamplerLinear);
//}

//void VulkanEngine::init_default_data() {
//
//    // Küçük bir quad (dikdörtgen) vertexleri
//    std::array<Vertex, 4> rect_vertices;
//
//    rect_vertices[0].position = { 0.5f, -0.5f, 0.0f };
//    rect_vertices[1].position = { 0.5f,  0.5f, 0.0f };
//    rect_vertices[2].position = { -0.5f, -0.5f, 0.0f };
//    rect_vertices[3].position = { -0.5f,  0.5f, 0.0f };
//
//    rect_vertices[0].color = { 0, 0, 0, 1 };
//    rect_vertices[1].color = { 0.5f, 0.5f, 0.5f, 1 };
//    rect_vertices[2].color = { 1, 0, 0, 1 };
//    rect_vertices[3].color = { 0, 1, 0, 1 };
//
//    rect_vertices[0].uv_x = 1.0f;
//    rect_vertices[0].uv_y = 0.0f;
//    rect_vertices[1].uv_x = 0.0f;
//    rect_vertices[1].uv_y = 0.0f;
//    rect_vertices[2].uv_x = 1.0f;
//    rect_vertices[2].uv_y = 1.0f;
//    rect_vertices[3].uv_x = 0.0f;
//    rect_vertices[3].uv_y = 1.0f;
//
//    // İndeksler (2 üçgen)
//    std::array<uint32_t, 6> rect_indices = {
//        0, 1, 2,
//        2, 1, 3
//    };
//
//    // GPU'ya yükle
//    rectangle = uploadMesh(rect_indices, rect_vertices);
//
//    // Büyük grid mesh için StaticMeshData oluştur ve sahneye ekle
//    StaticMeshData gridMesh;
//    gridMesh.mesh = rectangle;
//    gridMesh.position = glm::vec3(0.0f, 0.0f, 0.0f);
//
//    // XZ düzlemine yatay yapmak için X ekseninde -90 derece döndür
//    gridMesh.rotation = glm::vec3(-glm::half_pi<float>(), 0.0f, 0.0f);
//
//    // 100x100 birim büyüklüğünde geniş grid (X ve Z yönlerinde)
//    gridMesh.scale = glm::vec3(100.0f, 1.0f, 100.0f);
//
//    gridMesh.type = PrimitiveType::Plane;
//
//    // Listeye ekle
//    static_shapes.push_back(gridMesh);
//
//    // 3 adet temel renkli 1x1 texture oluştur
//    uint32_t white = glm::packUnorm4x8(glm::vec4(1, 1, 1, 1));
//    _whiteImage = create_image((void*)&white, VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R8G8B8A8_UNORM,
//        VK_IMAGE_USAGE_SAMPLED_BIT);
//
//    uint32_t grey = glm::packUnorm4x8(glm::vec4(0.66f, 0.66f, 0.66f, 1));
//    _greyImage = create_image((void*)&grey, VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R8G8B8A8_UNORM,
//        VK_IMAGE_USAGE_SAMPLED_BIT);
//
//    uint32_t black = glm::packUnorm4x8(glm::vec4(0, 0, 0, 0));
//    _blackImage = create_image((void*)&black, VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R8G8B8A8_UNORM,
//        VK_IMAGE_USAGE_SAMPLED_BIT);
//
//    // 16x16 checkerboard pattern texture oluştur
//    uint32_t magenta = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));
//    std::array<uint32_t, 16 * 16> pixels;
//    for (int x = 0; x < 16; ++x) {
//        for (int y = 0; y < 16; ++y) {
//            pixels[y * 16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
//        }
//    }
//    _errorCheckerboardImage = create_image(pixels.data(), VkExtent3D{ 16, 16, 1 }, VK_FORMAT_R8G8B8A8_UNORM,
//        VK_IMAGE_USAGE_SAMPLED_BIT);
//
//    // Sampler oluştur
//    VkSamplerCreateInfo samplerInfo = { .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
//    samplerInfo.magFilter = VK_FILTER_NEAREST;
//    samplerInfo.minFilter = VK_FILTER_NEAREST;
//    vkCreateSampler(_device, &samplerInfo, nullptr, &_defaultSamplerNearest);
//
//    samplerInfo.magFilter = VK_FILTER_LINEAR;
//    samplerInfo.minFilter = VK_FILTER_LINEAR;
//    vkCreateSampler(_device, &samplerInfo, nullptr, &_defaultSamplerLinear);
//
//
//    //gridMesh = rectangle;  // gridPipeline için ayrı mesh tanımlıyorsan, rectangle zaten uygun
//
//}

//void VulkanEngine::init_default_data() {
//
//    // === 1. Statik Küçük Quad Mesh (Shader-only static shapes için kullanılacak) ===
//    std::array<Vertex, 4> rect_vertices;
//
//    rect_vertices[0].position = { 0.5f,  -0.5f,  0.0f };
//    rect_vertices[1].position = { 0.5f,   0.5f,  0.0f };
//    rect_vertices[2].position = { -0.5f,  -0.5f,  0.0f };
//    rect_vertices[3].position = { -0.5f,   0.5f,  0.0f };
//
//    for (auto& v : rect_vertices) {
//        v.color = { 1.0f, 1.0f, 1.0f, 1.0f };
//        v.uv_x = 0.0f;
//        v.uv_y = 0.0f;
//    }
//
//    std::array<uint32_t, 6> rect_indices = {
//        0, 1, 2,
//        2, 1, 3
//    };
//
//    rectangle = uploadMesh(rect_indices, rect_vertices);
//
//
//    // === 2. Grid Mesh (World-space plane için özel mesh) ===
//    std::array<Vertex, 4> grid_vertices;
//
//    grid_vertices[0].position = { 0.5f,  0.0f, -0.5f };  // XZ düzlemi
//    grid_vertices[1].position = { -0.5f,  0.0f, -0.5f };
//    grid_vertices[2].position = { 0.5f,  0.0f,  0.5f };
//    grid_vertices[3].position = { -0.5f,  0.0f,  0.5f };
//
//    for (auto& v : grid_vertices) {
//        v.color = { 1.0f, 1.0f, 1.0f, 1.0f };  // Grid shader renklendirecek zaten
//        v.uv_x = 0.0f;
//        v.uv_y = 0.0f;
//    }
//
//    std::array<uint32_t, 6> grid_indices = {
//        0, 1, 2,
//        2, 1, 3
//    };
//
//    gridMesh = uploadMesh(grid_indices, grid_vertices);
//
//
//    // === 3. Statik Şekil Olarak Grid Plane Eklenebilir (opsiyonel) ===
//    StaticMeshData gridShape;
//    gridShape.mesh = gridMesh;
//    gridShape.position = glm::vec3(0.0f, 0.0f, 0.0f);
//    gridShape.rotation = glm::vec3(0.0f, 0.0f, 0.0f);  // XZ düzleminde zaten
//    gridShape.scale = glm::vec3(100.0f, 1.0f, 100.0f);  // 100x100 boyutunda grid
//    gridShape.type = PrimitiveType::Plane;
//    static_shapes.push_back(gridShape);
//
//
//    // === 4. Texture ve Samplerlar ===
//    uint32_t white = glm::packUnorm4x8(glm::vec4(1, 1, 1, 1));
//    _whiteImage = create_image(&white, VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);
//
//    uint32_t grey = glm::packUnorm4x8(glm::vec4(0.66f, 0.66f, 0.66f, 1.0f));
//    _greyImage = create_image(&grey, VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);
//
//    uint32_t black = glm::packUnorm4x8(glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
//    _blackImage = create_image(&black, VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);
//
//    uint32_t magenta = glm::packUnorm4x8(glm::vec4(1.0f, 0.0f, 1.0f, 1.0f));
//    std::array<uint32_t, 16 * 16> checkerboard;
//    for (int x = 0; x < 16; ++x) {
//        for (int y = 0; y < 16; ++y) {
//            checkerboard[y * 16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
//        }
//    }
//    _errorCheckerboardImage = create_image(checkerboard.data(), VkExtent3D{ 16, 16, 1 }, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);
//
//
//    VkSamplerCreateInfo samplerInfo = { .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
//    samplerInfo.magFilter = VK_FILTER_NEAREST;
//    samplerInfo.minFilter = VK_FILTER_NEAREST;
//    vkCreateSampler(_device, &samplerInfo, nullptr, &_defaultSamplerNearest);
//
//    samplerInfo.magFilter = VK_FILTER_LINEAR;
//    samplerInfo.minFilter = VK_FILTER_LINEAR;
//    vkCreateSampler(_device, &samplerInfo, nullptr, &_defaultSamplerLinear);
//}

void VulkanEngine::init_default_data() {
    // === 1. Statik Küçük Quad Mesh (Shader-only shapes için) ===
    std::array<Vertex, 4> rect_vertices;

    rect_vertices[0].position = { 0.5f,  -0.5f,  0.0f };
    rect_vertices[1].position = { 0.5f,   0.5f,  0.0f };
    rect_vertices[2].position = { -0.5f, -0.5f,  0.0f };
    rect_vertices[3].position = { -0.5f,  0.5f,  0.0f };

    for (auto& v : rect_vertices) {
        v.color = { 1.0f, 1.0f, 1.0f, 1.0f };
        v.uv_x = 0.0f;
        v.uv_y = 0.0f;
    }

    std::array<uint32_t, 6> rect_indices = {
        0, 1, 2,
        2, 1, 3
    };

    rectangle = uploadMesh(rect_indices, rect_vertices);

    // === 2. Dünya-Koordinatlı Grid Plane Mesh (1000x1000) ===
    std::array<Vertex, 4> grid_vertices;

    grid_vertices[0].position = { 500.0f, 0.0f, -500.0f };
    grid_vertices[1].position = { -500.0f, 0.0f, -500.0f };
    grid_vertices[2].position = { 500.0f, 0.0f, 500.0f };
    grid_vertices[3].position = { -500.0f, 0.0f, 500.0f };

    for (auto& v : grid_vertices) {
        v.color = { 1.0f, 1.0f, 1.0f, 1.0f };  // Grid shader kullanacak
        v.uv_x = 0.0f;
        v.uv_y = 0.0f;
    }

    std::array<uint32_t, 6> grid_indices = {
        0, 1, 2,
        2, 1, 3
    };

    gridMesh = uploadMesh(grid_indices, grid_vertices);

    // === (Opsiyonel) Grid plane'i sahneye statik mesh olarak ekle ===
    //StaticMeshData gridShape;
    //gridShape.mesh = gridMesh;
    //gridShape.position = glm::vec3(0.0f, 0.0f, 0.0f);
    //gridShape.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
    //gridShape.scale = glm::vec3(1.0f, 1.0f, 1.0f);   // 1x1 scale, çünkü vertexler 500 birimden
    //gridShape.type = PrimitiveType::Plane;

    //static_shapes.push_back(gridShape);

    // === 3. Textures ve Samplerlar ===
    uint32_t white = glm::packUnorm4x8(glm::vec4(1, 1, 1, 1));
    _whiteImage = create_image(&white, VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

    uint32_t grey = glm::packUnorm4x8(glm::vec4(0.66f, 0.66f, 0.66f, 1.0f));
    _greyImage = create_image(&grey, VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

    uint32_t black = glm::packUnorm4x8(glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
    _blackImage = create_image(&black, VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

    uint32_t magenta = glm::packUnorm4x8(glm::vec4(1.0f, 0.0f, 1.0f, 1.0f));
    std::array<uint32_t, 16 * 16> checkerboard;
    for (int x = 0; x < 16; ++x) {
        for (int y = 0; y < 16; ++y) {
            checkerboard[y * 16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
        }
    }
    _errorCheckerboardImage = create_image(checkerboard.data(), VkExtent3D{ 16, 16, 1 }, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

    // Samplerlar
    VkSamplerCreateInfo samplerInfo = { .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    vkCreateSampler(_device, &samplerInfo, nullptr, &_defaultSamplerNearest);

    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    vkCreateSampler(_device, &samplerInfo, nullptr, &_defaultSamplerLinear);

    // Note: Default images and samplers are cleaned up explicitly in cleanup()
}


//void VulkanEngine::init_light_sphere()
//{
//    StaticMeshData lightBall;
//    lightBall.mesh = generate_sphere_mesh(32, 16);
//    lightBall.position = glm::vec3(0.0f, 1.0f, 0.0f); // Sahne ortasına koy
//    lightBall.scale = glm::vec3(0.25f);              // Küçük bir nokta gibi görünsün
//    lightBall.rotation = glm::vec3(0.0f);
//    lightBall.type = PrimitiveType::Sphere;
//    lightBall.materialType = ShaderOnlyMaterial::EMISSIVE; // 💡 Shader'da özel renk verebiliriz
//    lightBall.passType = MaterialPass::MainColor;
//
//    static_shapes.push_back(lightBall);
//}






//void VulkanEngine::draw_main(VkCommandBuffer cmd)
//{
//    ComputeEffect& effect = backgroundEffects[currentBackgroundEffect];
//
//    // bind the background compute pipeline
//    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, effect.pipeline);
//
//    // bind the descriptor set containing the draw image for the compute pipeline
//    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _gradientPipelineLayout, 0, 1, &_drawImageDescriptors, 0, nullptr);
//
//    vkCmdPushConstants(cmd, _gradientPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputePushConstants), &effect.data);
//    // execute the compute pipeline dispatch. We are using 16x16 workgroup size so we need to divide by it
//    vkCmdDispatch(cmd, std::ceil(_windowExtent.width / 16.0), std::ceil(_windowExtent.height / 16.0), 1);
//
//    //draw the triangle
//
//    vkutil::transition_image(cmd, _drawImage.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
//
//    VkRenderingAttachmentInfo colorAttachment = vkinit::attachment_info(_drawImage.imageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
//    VkRenderingAttachmentInfo depthAttachment = vkinit::depth_attachment_info(_depthImage.imageView, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
//
//    VkRenderingInfo renderInfo = vkinit::rendering_info(_windowExtent, &colorAttachment, &depthAttachment);
//
//    vkCmdBeginRendering(cmd, &renderInfo);
//    auto start = std::chrono::system_clock::now();
//    draw_geometry(cmd);
//
//    auto end = std::chrono::system_clock::now();
//    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
//
//    stats.mesh_draw_time = elapsed.count() / 1000.f;
//
//    vkCmdEndRendering(cmd);
//}


//
//void VulkanEngine::draw_main(VkCommandBuffer cmd)
//{
//    ComputeEffect& effect = backgroundEffects[currentBackgroundEffect];
//
//    // bind the background compute pipeline
//    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, effect.pipeline);
//
//    // bind the descriptor set containing the draw image for the compute pipeline
//    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _gradientPipelineLayout, 0, 1, &_drawImageDescriptors, 0, nullptr);
//
//    vkCmdPushConstants(cmd, _gradientPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputePushConstants), &effect.data);
//    // execute the compute pipeline dispatch. We are using 16x16 workgroup size so we need to divide by it
//    vkCmdDispatch(cmd, std::ceil(_drawExtent.width / 16.0), std::ceil(_drawExtent.height / 16.0), 1);
//
//    //draw the triangle
//
//    VkRenderingAttachmentInfo colorAttachment = vkinit::attachment_info(_drawImage.imageView, nullptr, VK_IMAGE_LAYOUT_GENERAL);
//    VkRenderingAttachmentInfo depthAttachment = vkinit::depth_attachment_info(_depthImage.imageView, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
//
//    VkRenderingInfo renderInfo = vkinit::rendering_info(_drawExtent, &colorAttachment, &depthAttachment);
//
//    vkCmdBeginRendering(cmd, &renderInfo);
//    auto start = std::chrono::system_clock::now();
//    draw_geometry(cmd);
//
//    auto end = std::chrono::system_clock::now();
//    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
//
//    stats.mesh_draw_time = elapsed.count() / 1000.f;
//
//    vkCmdEndRendering(cmd);
//}

void VulkanEngine::draw_main(VkCommandBuffer cmd)
{
    // PathTraced mode uses compute shader path tracing
    if (_currentViewMode == ViewMode::PathTraced) {
        draw_rendered_pathtraced(cmd);
    } else {
        // Normal background effect compute shader
        ComputeEffect& effect = backgroundEffects[currentBackgroundEffect];
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, effect.pipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _gradientPipelineLayout, 0, 1, &_drawImageDescriptors, 0, nullptr);
        vkCmdPushConstants(cmd, _gradientPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputePushConstants), &effect.data);
        vkCmdDispatch(cmd, std::ceil(_drawExtent.width / 16.0), std::ceil(_drawExtent.height / 16.0), 1);
    }

    // Memory barrier: ensure compute shader writes complete before graphics reads
    // Critical for MoltenVK/macOS which is stricter about synchronization
    VkImageMemoryBarrier2 imageBarrier{ .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
    imageBarrier.pNext = nullptr;
    imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    imageBarrier.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
    imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    imageBarrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    imageBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
    imageBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    imageBarrier.image = _drawImage.image;
    imageBarrier.subresourceRange = vkinit::image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT);

    VkDependencyInfo depInfo{};
    depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    depInfo.pNext = nullptr;
    depInfo.imageMemoryBarrierCount = 1;
    depInfo.pImageMemoryBarriers = &imageBarrier;

    vkCmdPipelineBarrier2(cmd, &depInfo);

    // Rasterize pass
    VkRenderingAttachmentInfo colorAttachment = vkinit::attachment_info(_drawImage.imageView, nullptr, VK_IMAGE_LAYOUT_GENERAL);
    VkRenderingAttachmentInfo depthAttachment = vkinit::depth_attachment_info(_depthImage.imageView, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

    VkRenderingInfo renderInfo = vkinit::rendering_info(_drawExtent, &colorAttachment, &depthAttachment);

    vkCmdBeginRendering(cmd, &renderInfo);

    // Draw based on view mode
    draw_viewing(cmd);

    vkCmdEndRendering(cmd);
}











//void VulkanEngine::draw_main(VkCommandBuffer cmd) {
//    ComputeEffect& effect = backgroundEffects[currentBackgroundEffect];
//
//    // 🔹 Compute: arka plan efekti uygula
//    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, effect.pipeline);
//    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _gradientPipelineLayout, 0, 1, &_drawImageDescriptors, 0, nullptr);
//    vkCmdPushConstants(cmd, _gradientPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputePushConstants), &effect.data);
//    vkCmdDispatch(cmd, std::ceil(_windowExtent.width / 16.0), std::ceil(_windowExtent.height / 16.0), 1);
//
//    // 🔁 Compute sonrası renk ve derinlik hedefini geçiş yap
//    vkutil::transition_image(cmd, _drawImage.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
//    vkutil::transition_image(cmd, _depthImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
//
//    // 🔹 Begin Rendering (hem 2D hem 3D)
//    VkRenderingAttachmentInfo colorAttachment = vkinit::attachment_info(_drawImage.imageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
//    VkRenderingAttachmentInfo depthAttachment = vkinit::depth_attachment_info(_depthImage.imageView, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
//    VkRenderingInfo renderInfo = vkinit::rendering_info(_windowExtent, &colorAttachment, &depthAttachment);
//
//    vkCmdBeginRendering(cmd, &renderInfo);
//
//    // 🔹 Sadece draw_geometry çağır (içeride 2D + 3D)
//    draw_geometry(cmd);
//
//    vkCmdEndRendering(cmd);
//}

void VulkanEngine::update_imgui()
{
    // === PROFESSIONAL OOP UI SYSTEM ===
    // Blender 4.0 Dark Theme with proper panel architecture
    Yalaz::UI::EditorUI::Get().Render();

    // Keep gizmo for now (will integrate into UI system later)
    draw_node_gizmo();
}





void VulkanEngine::draw_node_selector()
{
    ImGui::Begin("Scene Graph");

    for (auto& [name, gltf] : loadedScenes)
    {
        if (!gltf) continue;
        for (auto& root : gltf->topNodes) {
            draw_node_recursive_ui(root);
        }
    }

    ImGui::End();
}

//void VulkanEngine::draw_node_recursive_ui(std::shared_ptr<Node> node)
//{
//    MeshNode* meshNode = dynamic_cast<MeshNode*>(node.get());
//    std::string label = meshNode ? "MeshNode" : "Node";
//
//    bool isSelected = (selectedNode == meshNode);
//
//    if (ImGui::Selectable(label.c_str(), isSelected)) {
//        if (meshNode) {
//            selectedNode = meshNode;
//        }
//    }
//
//    for (auto& child : node->children) {
//        draw_node_recursive_ui(child);
//    }
//}

//void VulkanEngine::draw_node_recursive_ui(std::shared_ptr<Node> node)
//{
//    MeshNode* meshNode = dynamic_cast<MeshNode*>(node.get());
//
//    std::string label = meshNode && !meshNode->mesh->name.empty()
//        ? meshNode->mesh->name + "##" + std::to_string(reinterpret_cast<uintptr_t>(meshNode))
//        : std::string("MeshNode##") + std::to_string(reinterpret_cast<uintptr_t>(meshNode));
//
//
//    bool selected = (selectedNode == meshNode);
//
//    if (ImGui::Selectable(label.c_str(), selected)) {
//        if (meshNode) selectedNode = meshNode;
//        else selectedNode = nullptr;
//    }
//
//    for (auto& child : node->children) {
//        draw_node_recursive_ui(child);
//    }
//}

//void VulkanEngine::draw_node_recursive_ui(std::shared_ptr<Node> node)
//{
//    // MeshNode olup olmadığını kontrol et (raw pointer olarak alıyoruz)
//    MeshNode* meshNode = dynamic_cast<MeshNode*>(node.get());
//
//    //std::string label = (meshNode && meshNode->mesh && !meshNode->mesh->name.empty())
//    //    ? meshNode->mesh->name + "##" + std::to_string(reinterpret_cast<uintptr_t>(meshNode))
//    //    : "MeshNode##" + std::to_string(reinterpret_cast<uintptr_t>(node.get()));
//    std::string label;
//    if (meshNode && meshNode->mesh && !meshNode->mesh->name.empty()) {
//        label = meshNode->mesh->name + "##" + std::to_string(reinterpret_cast<uintptr_t>(meshNode));
//    }
//    else {
//        label = "MeshNode##" + std::to_string(reinterpret_cast<uintptr_t>(meshNode));
//    }
//
//
//    // Karşılaştırmayı MeshNode* üzerinden yapıyoruz
//    bool selected = (selectedNode == meshNode);
//
//    if (ImGui::Selectable(label.c_str(), selected)) {
//        selectedNode = meshNode; // ✔️ artık MeshNode* türünde
//    }
//
//    // Alt node'lara devam et
//    for (auto& child : node->children) {
//        draw_node_recursive_ui(child);
//    }
//}
void VulkanEngine::draw_node_recursive_ui(std::shared_ptr<Node> node)
{
    // Bu node bir MeshNode mu?
    MeshNode* meshNode = dynamic_cast<MeshNode*>(node.get());

    // Etiket belirlemesi
    std::string label;
    if (meshNode && meshNode->mesh && !meshNode->mesh->name.empty()) {
        label = meshNode->mesh->name + "##" + std::to_string(reinterpret_cast<uintptr_t>(meshNode));
    }
    else {
        label = "MeshNode##" + std::to_string(reinterpret_cast<uintptr_t>(node.get()));
    }

    // Bu node seçili mi?
    bool selected = (selectedNode == meshNode);

    // UI'de selectable olarak göster
    if (ImGui::Selectable(label.c_str(), selected)) {
        selectedNode = meshNode;  //
    }

    // Alt node'lar varsa onları da göster (recursive)
    for (auto& child : node->children) {
        draw_node_recursive_ui(child);
    }
}

MeshNode* VulkanEngine::findNodeByName(const std::string& name)
{
    for (auto& [sceneName, gltf] : loadedScenes)
    {
        if (!gltf) continue;

        for (auto& root : gltf->topNodes)
        {
            MeshNode* found = findNodeRecursive(root, name);
            if (found) return found;
        }
    }

    return nullptr;
}

MeshNode* VulkanEngine::findNodeRecursive(std::shared_ptr<Node> node, const std::string& name)
{
    MeshNode* meshNode = dynamic_cast<MeshNode*>(node.get());

    if (meshNode && meshNode->mesh && meshNode->mesh->name == name) {
        return meshNode;
    }

    for (auto& child : node->children)
    {
        MeshNode* found = findNodeRecursive(child, name);
        if (found) return found;
    }

    return nullptr;
}



//void VulkanEngine::draw_node_gizmo()
//{
//    if (!selectedNode) return;
//
//    ImGuizmo::BeginFrame();
//    ImGui::Begin("Gizmo");
//
//    static ImGuizmo::OPERATION operation = ImGuizmo::TRANSLATE;
//    static ImGuizmo::MODE mode = ImGuizmo::LOCAL;
//
//    if (ImGui::RadioButton("Translate", operation == ImGuizmo::TRANSLATE)) operation = ImGuizmo::TRANSLATE;
//    ImGui::SameLine();
//    if (ImGui::RadioButton("Rotate", operation == ImGuizmo::ROTATE)) operation = ImGuizmo::ROTATE;
//    ImGui::SameLine();
//    if (ImGui::RadioButton("Scale", operation == ImGuizmo::SCALE)) operation = ImGuizmo::SCALE;
//
//    if (operation != ImGuizmo::SCALE) {
//        if (ImGui::RadioButton("Local", mode == ImGuizmo::LOCAL)) mode = ImGuizmo::LOCAL;
//        ImGui::SameLine();
//        if (ImGui::RadioButton("World", mode == ImGuizmo::WORLD)) mode = ImGuizmo::WORLD;
//    }
//
//    glm::mat4 model = selectedNode->localTransform;
//
//    glm::mat4 view = mainCamera.getViewMatrix();
//    glm::mat4 proj = mainCamera.getProjectionMatrix();
//    // ❌ Vulkan uyumu için kullanılan bu satır kaldırıldı
//    // proj[1][1] *= -1.0f;
//
//    ImGuizmo::SetRect(0, 0, ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y);
//
//    ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(proj),
//        operation, mode, glm::value_ptr(model));
//
//    if (ImGuizmo::IsUsing()) {
//        selectedNode->localTransform = model;
//
//        glm::mat4 parentMatrix = glm::mat4(1.0f);
//        if (auto p = selectedNode->parent.lock()) {
//            parentMatrix = p->worldTransform;
//        }
//        selectedNode->refreshTransform(parentMatrix);
//    }
//
//    ImGui::End();
//}
//void VulkanEngine::draw_node_gizmo()
//{
//    if (!selectedNode) return;
//
//    ImGuizmo::BeginFrame();
//    ImGui::Begin("Gizmo");
//
//    static ImGuizmo::OPERATION operation = ImGuizmo::TRANSLATE;
//    static ImGuizmo::MODE mode = ImGuizmo::LOCAL;
//    static glm::mat4 model = selectedNode->localTransform;
//
//    // 🎯 Shift + F → sadece gizmo sıfırlama
//    bool shiftDown = ImGui::GetIO().KeyShift;
//    if (ImGui::IsKeyPressed(ImGuiKey_F)) {
//        model = selectedNode->localTransform;
//
//        if (!shiftDown) {
//            // Kamera da objeye odaklansın
//            glm::vec3 objPos = glm::vec3(selectedNode->worldTransform[3]);
//            glm::vec3 camDir = mainCamera.getLookDirection();
//            mainCamera.position = objPos - camDir * 5.0f;
//        }
//    }
//
//    // 🎛️ Mod kontrolleri
//    if (ImGui::RadioButton("Translate", operation == ImGuizmo::TRANSLATE)) operation = ImGuizmo::TRANSLATE;
//    ImGui::SameLine();
//    if (ImGui::RadioButton("Rotate", operation == ImGuizmo::ROTATE)) operation = ImGuizmo::ROTATE;
//    ImGui::SameLine();
//    if (ImGui::RadioButton("Scale", operation == ImGuizmo::SCALE)) operation = ImGuizmo::SCALE;
//
//    if (operation != ImGuizmo::SCALE) {
//        if (ImGui::RadioButton("Local", mode == ImGuizmo::LOCAL)) mode = ImGuizmo::LOCAL;
//        ImGui::SameLine();
//        if (ImGui::RadioButton("World", mode == ImGuizmo::WORLD)) mode = ImGuizmo::WORLD;
//    }
//
//    glm::mat4 view = mainCamera.getViewMatrix();
//    glm::mat4 proj = mainCamera.getProjectionMatrix();
//
//    ImGuizmo::SetRect(0, 0, ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y);
//
//    float snap[3] = { 0.1f, 0.1f, 0.1f };
//    float* snapPtr = ImGui::IsKeyDown(ImGuiKey_Tab) ? ((operation == ImGuizmo::ROTATE) ? &snap[0] : snap) : nullptr;
//
//    ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(proj),
//        operation, mode, glm::value_ptr(model), nullptr, snapPtr);
//
//    if (ImGuizmo::IsUsing()) {
//        selectedNode->localTransform = model;
//
//        glm::mat4 parentMatrix = glm::mat4(1.0f);
//        if (auto p = selectedNode->parent.lock()) {
//            parentMatrix = p->worldTransform;
//        }
//        selectedNode->refreshTransform(parentMatrix);
//    }
//
//    // ✨ ZEMİN GÖLGE EFEKTİ
//    {
//        glm::vec4 worldPos4 = selectedNode->worldTransform[3];
//        glm::vec3 worldPos = glm::vec3(worldPos4);
//
//        glm::vec4 clip = proj * view * glm::vec4(worldPos, 1.0f);
//        if (clip.w != 0.0f) {
//            glm::vec3 ndc = glm::vec3(clip) / clip.w;
//            ImVec2 screenPos;
//            screenPos.x = (ndc.x * 0.5f + 0.5f) * ImGui::GetIO().DisplaySize.x;
//            screenPos.y = (1.0f - (ndc.y * 0.5f + 0.5f)) * ImGui::GetIO().DisplaySize.y;
//
//            ImDrawList* drawList = ImGui::GetForegroundDrawList();
//            float radius = 20.0f;
//            drawList->AddCircleFilled(screenPos, radius, IM_COL32(0, 0, 0, 40), 32);
//        }
//    }
//
//
//    ImGui::End();
//}
void VulkanEngine::draw_node_gizmo()
{
    if (!selectedNode) return;

    ImGuizmo::BeginFrame();

    static ImGuizmo::OPERATION operation = ImGuizmo::TRANSLATE;
    static ImGuizmo::MODE mode = ImGuizmo::LOCAL;
    static glm::mat4 model = selectedNode->localTransform;
    static MeshNode* lastNode = nullptr;

    // Update model when selection changes
    if (lastNode != selectedNode) {
        model = selectedNode->localTransform;
        lastNode = selectedNode;
    }

    // F key: Focus camera on object
    if (ImGui::IsKeyPressed(ImGuiKey_F)) {
        glm::vec3 objPos = glm::vec3(selectedNode->worldTransform[3]);
        mainCamera.focusOnPoint(objPos, 5.0f);
    }

    // Camera matrices
    glm::mat4 view = mainCamera.getViewMatrix();
    glm::mat4 proj = mainCamera.getProjectionMatrix();

    // Set gizmo rect to full screen
    ImGuizmo::SetRect(0, 0, ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y);

    // Snap (TAB to enable)
    float snap[3] = { 0.1f, 0.1f, 0.1f };
    float* snapPtr = ImGui::IsKeyDown(ImGuiKey_Tab) ? ((operation == ImGuizmo::ROTATE) ? &snap[0] : snap) : nullptr;

    // Apply gizmo
    ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(proj),
        operation, mode, glm::value_ptr(model), nullptr, snapPtr);

    // Update transform when using gizmo
    if (ImGuizmo::IsUsing()) {
        selectedNode->localTransform = model;

        glm::mat4 parentMatrix = glm::mat4(1.0f);
        if (auto p = selectedNode->parent.lock()) {
            parentMatrix = p->worldTransform;
        }
        selectedNode->refreshTransform(parentMatrix);
    }

    // Compact gizmo toolbar overlay (top-center of viewport)
    {
        float viewportX = 280.0f;  // Left panel width
        float viewportW = ImGui::GetIO().DisplaySize.x - 280.0f - 320.0f;  // Viewport width
        float toolbarW = 300.0f;
        float toolbarX = viewportX + (viewportW - toolbarW) * 0.5f;

        ImGui::SetNextWindowPos(ImVec2(toolbarX, 30.0f), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(toolbarW, 0), ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.85f);

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                  ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                                  ImGuiWindowFlags_AlwaysAutoResize;

        ImGui::Begin("##GizmoToolbar", nullptr, flags);

        // Transform mode buttons
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 4));

        bool isTranslate = (operation == ImGuizmo::TRANSLATE);
        bool isRotate = (operation == ImGuizmo::ROTATE);
        bool isScale = (operation == ImGuizmo::SCALE);

        if (isTranslate) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.8f, 1.0f));
        if (ImGui::Button("W Move")) operation = ImGuizmo::TRANSLATE;
        if (isTranslate) ImGui::PopStyleColor();

        ImGui::SameLine();
        if (isRotate) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.8f, 1.0f));
        if (ImGui::Button("E Rotate")) operation = ImGuizmo::ROTATE;
        if (isRotate) ImGui::PopStyleColor();

        ImGui::SameLine();
        if (isScale) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.8f, 1.0f));
        if (ImGui::Button("R Scale")) operation = ImGuizmo::SCALE;
        if (isScale) ImGui::PopStyleColor();

        // Space mode toggle
        if (operation != ImGuizmo::SCALE) {
            ImGui::SameLine();
            ImGui::Separator();
            ImGui::SameLine();
            if (ImGui::Button(mode == ImGuizmo::LOCAL ? "Local" : "World")) {
                mode = (mode == ImGuizmo::LOCAL) ? ImGuizmo::WORLD : ImGuizmo::LOCAL;
            }
        }

        ImGui::PopStyleVar();
        ImGui::End();
    }

    // Keyboard shortcuts for gizmo modes
    if (!ImGui::GetIO().WantTextInput) {
        if (ImGui::IsKeyPressed(ImGuiKey_W)) operation = ImGuizmo::TRANSLATE;
        if (ImGui::IsKeyPressed(ImGuiKey_E)) operation = ImGuizmo::ROTATE;
        if (ImGui::IsKeyPressed(ImGuiKey_R)) operation = ImGuizmo::SCALE;
    }
}



glm::vec2 worldToScreen(const glm::vec3& worldPos, const glm::mat4& view, const glm::mat4& proj, ImVec2 viewportSize)
{
    glm::vec4 clip = proj * view * glm::vec4(worldPos, 1.0f);
    if (clip.w == 0.0f) return { 0, 0 };

    glm::vec3 ndc = glm::vec3(clip) / clip.w;
    glm::vec2 screen;
    screen.x = (ndc.x * 0.5f + 0.5f) * viewportSize.x;
    screen.y = (1.0f - (ndc.y * 0.5f + 0.5f)) * viewportSize.y; // Vulkan uyumu

    return screen;
}










void VulkanEngine::draw_imgui(VkCommandBuffer cmd, VkImageView targetImageView)
{
    VkRenderingAttachmentInfo colorAttachment = vkinit::attachment_info(targetImageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    VkRenderingInfo renderInfo = vkinit::rendering_info(_swapchainExtent, &colorAttachment, nullptr);

    vkCmdBeginRendering(cmd, &renderInfo);

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

    vkCmdEndRendering(cmd);
}




void VulkanEngine::draw()
{
    update_scene();
    //wait until the gpu has finished rendering the last frame. Timeout of 1 second
    VK_CHECK(vkWaitForFences(_device, 1, &get_current_frame()._renderFence, true, 1000000000));

    get_current_frame()._deletionQueue.flush();
    get_current_frame()._frameDescriptors.clear_pools(_device);
    //request image from the swapchain
    uint32_t swapchainImageIndex;

    VkResult e = vkAcquireNextImageKHR(_device, _swapchain, 1000000000, get_current_frame()._swapchainSemaphore, nullptr, &swapchainImageIndex);
    if (e == VK_ERROR_OUT_OF_DATE_KHR) {
        resize_requested = true;
        return;
    }

    _drawExtent.height = std::min(_swapchainExtent.height, _drawImage.imageExtent.height) * renderScale;
    _drawExtent.width = std::min(_swapchainExtent.width, _drawImage.imageExtent.width) * renderScale;


    VK_CHECK(vkResetFences(_device, 1, &get_current_frame()._renderFence));

    //now that we are sure that the commands finished executing, we can safely reset the command buffer to begin recording again.
    VK_CHECK(vkResetCommandBuffer(get_current_frame()._mainCommandBuffer, 0));

    //naming it cmd for shorter writing
    VkCommandBuffer cmd = get_current_frame()._mainCommandBuffer;

    //begin the command buffer recording. We will use this command buffer exactly once, so we want to let vulkan know that
    VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    //> draw_first
    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    // transition our main draw image into general layout so we can write into it
    // we will overwrite it all so we dont care about what was the older layout
    vkutil::transition_image(cmd, _drawImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
    vkutil::transition_image(cmd, _depthImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

    draw_main(cmd);

    //transtion the draw image and the swapchain image into their correct transfer layouts
    vkutil::transition_image(cmd, _drawImage.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    vkutil::transition_image(cmd, _swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    VkExtent2D extent;
    extent.height = _windowExtent.height;
    extent.width = _windowExtent.width;
    //< draw_first
    //> imgui_draw
    // execute a copy from the draw image into the swapchain
    vkutil::copy_image_to_image(cmd, _drawImage.image, _swapchainImages[swapchainImageIndex], _drawExtent, _swapchainExtent);

    // set swapchain image layout to Attachment Optimal so we can draw it
    vkutil::transition_image(cmd, _swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    //draw imgui into the swapchain image
    draw_imgui(cmd, _swapchainImageViews[swapchainImageIndex]);

    // set swapchain image layout to Present so we can draw it
    vkutil::transition_image(cmd, _swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    //finalize the command buffer (we can no longer add commands, but it can now be executed)
    VK_CHECK(vkEndCommandBuffer(cmd));

    //prepare the submission to the queue. 
    //we want to wait on the _presentSemaphore, as that semaphore is signaled when the swapchain is ready
    //we will signal the _renderSemaphore, to signal that rendering has finished

    VkCommandBufferSubmitInfo cmdinfo = vkinit::command_buffer_submit_info(cmd);

    VkSemaphoreSubmitInfo waitInfo = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, get_current_frame()._swapchainSemaphore);
    VkSemaphoreSubmitInfo signalInfo = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, get_current_frame()._renderSemaphore);

    VkSubmitInfo2 submit = vkinit::submit_info(&cmdinfo, &signalInfo, &waitInfo);

    //submit command buffer to the queue and execute it.
    // _renderFence will now block until the graphic commands finish execution
    VK_CHECK(vkQueueSubmit2(_graphicsQueue, 1, &submit, get_current_frame()._renderFence));



    //prepare present
    // this will put the image we just rendered to into the visible window.
    // we want to wait on the _renderSemaphore for that, 
    // as its necessary that drawing commands have finished before the image is displayed to the user
    VkPresentInfoKHR presentInfo = vkinit::present_info();

    presentInfo.pSwapchains = &_swapchain;
    presentInfo.swapchainCount = 1;

    presentInfo.pWaitSemaphores = &get_current_frame()._renderSemaphore;
    presentInfo.waitSemaphoreCount = 1;

    presentInfo.pImageIndices = &swapchainImageIndex;

    VkResult presentResult = vkQueuePresentKHR(_graphicsQueue, &presentInfo);
    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR) {
        resize_requested = true;
        return;
    }
    //increase the number of frames drawn
    _frameNumber++;
}


//bool is_visible(const RenderObject& obj, const glm::mat4& viewproj) {
//    std::array<glm::vec3, 8> corners{
//        glm::vec3{ 1, 1, 1 },
//        glm::vec3{ 1, 1, -1 },
//        glm::vec3{ 1, -1, 1 },
//        glm::vec3{ 1, -1, -1 },
//        glm::vec3{ -1, 1, 1 },
//        glm::vec3{ -1, 1, -1 },
//        glm::vec3{ -1, -1, 1 },
//        glm::vec3{ -1, -1, -1 },
//    };
//
//    glm::mat4 matrix = viewproj * obj.transform;
//
//    glm::vec3 min = glm::vec3(FLT_MAX);
//    glm::vec3 max = glm::vec3(-FLT_MAX);
//
//    for (const auto& corner : corners) {
//        glm::vec4 v = matrix * glm::vec4(obj.bounds.origin + (corner * obj.bounds.extents), 1.f);
//
//        if (v.w != 0.0f) {
//            v.x /= v.w;
//            v.y /= v.w;
//            v.z /= v.w;
//        }
//
//        min = glm::min(glm::vec3(v), min);
//        max = glm::max(glm::vec3(v), max);
//    }
//
//    // **🛠️ Daha iyi bir frustum culling kontrolü**
//    bool visible =
//        !(min.z > 1.0f || max.z < 0.0f ||  // **Derinlik sınırları**
//            min.x > 1.0f || max.x < -1.0f || // **X ekseni sınırları**
//            min.y > 1.0f || max.y < -1.0f);  // **Y ekseni sınırları**
//
//    return visible;
//}
//> visfn


//bool is_visible(const RenderObject& obj, const glm::mat4& viewproj) {
//    std::array<glm::vec3, 8> corners{
//        glm::vec3 { 1, 1, 1 },
//        glm::vec3 { 1, 1, -1 },
//        glm::vec3 { 1, -1, 1 },
//        glm::vec3 { 1, -1, -1 },
//        glm::vec3 { -1, 1, 1 },
//        glm::vec3 { -1, 1, -1 },
//        glm::vec3 { -1, -1, 1 },
//        glm::vec3 { -1, -1, -1 },
//    };
//
//    glm::mat4 matrix = viewproj * obj.transform;
//
//    glm::vec3 min = { 1.5, 1.5, 1.5 };
//    glm::vec3 max = { -1.5, -1.5, -1.5 };
//
//    for (int c = 0; c < 8; c++) {
//        // project each corner into clip space
//        glm::vec4 v = matrix * glm::vec4(obj.bounds.origin + (corners[c] * obj.bounds.extents), 1.f);
//
//        // perspective correction
//        v.x = v.x / v.w;
//        v.y = v.y / v.w;
//        v.z = v.z / v.w;
//
//        min = glm::min(glm::vec3{ v.x, v.y, v.z }, min);
//        max = glm::max(glm::vec3{ v.x, v.y, v.z }, max);
//    }
//
//    // check the clip space box is within the view
//    if (min.z > 1.f || max.z < 0.f || min.x > 1.f || max.x < -1.f || min.y > 1.f || max.y < -1.f) {
//        return false;
//    }
//    else {
//        return true;
//    }
//}
//bool is_visible(const RenderObject& obj, const glm::mat4& viewproj) {
//    // 8 köşe: AABB'nin köşeleri
//    std::array<glm::vec3, 8> corners{
//        glm::vec3{ 1, 1, 1 },
//        glm::vec3{ 1, 1, -1 },
//        glm::vec3{ 1, -1, 1 },
//        glm::vec3{ 1, -1, -1 },
//        glm::vec3{ -1, 1, 1 },
//        glm::vec3{ -1, 1, -1 },
//        glm::vec3{ -1, -1, 1 },
//        glm::vec3{ -1, -1, -1 },
//    };
//
//    // World->Clip space dönüşümü
//    glm::mat4 matrix = viewproj * obj.transform;
//
//    glm::vec3 min = glm::vec3(FLT_MAX);
//    glm::vec3 max = glm::vec3(-FLT_MAX);
//
//    for (int c = 0; c < 8; c++) {
//        // Köşe pozisyonunu world space'e taşı
//        glm::vec3 worldCorner = obj.bounds.origin + (corners[c] * obj.bounds.extents);
//        glm::vec4 v = matrix * glm::vec4(worldCorner, 1.f);
//
//        // Perspective divide
//        if (v.w != 0.0f) {
//            v.x /= v.w;
//            v.y /= v.w;
//            v.z /= v.w;
//        }
//
//        min = glm::min(glm::vec3{ v.x, v.y, v.z }, min);
//        max = glm::max(glm::vec3{ v.x, v.y, v.z }, max);
//    }
//
//    // Vulkan NDC: x,y ∈ [-1,1], z ∈ [0,1]
//    if (min.z > 1.f || max.z < 0.f ||
//        min.x > 1.f || max.x < -1.f ||
//        min.y > 1.f || max.y < -1.f) {
//        return false;
//    }
//    return true;
//}
//bool is_visible(const RenderObject& obj, const glm::mat4& viewproj) {
//    std::array<glm::vec3, 8> corners = {
//        glm::vec3{ 1,  1,  1}, glm::vec3{-1,  1,  1},
//        glm::vec3{ 1, -1,  1}, glm::vec3{-1, -1,  1},
//        glm::vec3{ 1,  1, -1}, glm::vec3{-1,  1, -1},
//        glm::vec3{ 1, -1, -1}, glm::vec3{-1, -1, -1}
//    };
//
//    glm::mat4 matrix = viewproj * obj.transform;
//    glm::vec3 min(1.0f);
//    glm::vec3 max(-1.0f);
//    bool any_in_front = false;
//
//    for (const auto& corner : corners) {
//        glm::vec3 world_pos = obj.bounds.origin + (corner * obj.bounds.extents);
//        glm::vec4 clip = matrix * glm::vec4(world_pos, 1.0f);
//        if (clip.w <= 0.0f) continue;
//        glm::vec3 ndc = glm::vec3(clip) / clip.w;
//        min = glm::min(min, ndc);
//        max = glm::max(max, ndc);
//        any_in_front = true;
//    }
//    if (!any_in_front) return false;
//    // Reverse-Z için doğru sınır: max.z < 0.0f || min.z > 1.0f
//    if (min.x > 1.0f || max.x < -1.0f ||
//        min.y > 1.0f || max.y < -1.0f ||
//        max.z < 0.0f || min.z > 1.0f) {
//        return false;
//    }
//
//    return true;
//}
bool is_visible(const RenderObject& obj, const glm::mat4& viewproj) {
    std::array<glm::vec3, 8> corners = {
        glm::vec3{ 1,  1,  1}, glm::vec3{-1,  1,  1},
        glm::vec3{ 1, -1,  1}, glm::vec3{-1, -1,  1},
        glm::vec3{ 1,  1, -1}, glm::vec3{-1,  1, -1},
        glm::vec3{ 1, -1, -1}, glm::vec3{-1, -1, -1}
    };

    glm::mat4 matrix = viewproj * obj.transform;
    glm::vec3 min(1.0f);
    glm::vec3 max(-1.0f);
    bool any_in_front = false;

    for (const auto& corner : corners) {
        glm::vec3 world_pos = obj.bounds.origin + (corner * obj.bounds.extents);
        glm::vec4 clip = matrix * glm::vec4(world_pos, 1.0f);
        if (clip.w <= 0.0f) continue;
        glm::vec3 ndc = glm::vec3(clip) / clip.w;
        min = glm::min(min, ndc);
        max = glm::max(max, ndc);
        any_in_front = true;
    }

    if (!any_in_front)
        return false;

    if (min.x > 1.0f || max.x < -1.0f ||
        min.y > 1.0f || max.y < -1.0f ||
        min.z > 1.0f || max.z < 0.0f)  // ✅ Reverse-Z sınırı
    {
        return false;
    }

    return true;
}
void VulkanEngine::select_object_under_mouse(float mouseX, float mouseY)
{
    glm::vec3 rayOrigin;
    glm::vec3 rayDir;
    compute_ray_from_mouse(mouseX, mouseY, rayOrigin, rayDir);

    MeshNode* hitNode = raycast_scene_objects(rayOrigin, rayDir);

    if (hitNode)
    {
        fmt::print("[SEÇİM] Obje vuruldu ve seçildi!\n");
        selectedNode = hitNode;
    }
    else
    {
        fmt::print("[SEÇİM] Obje vurulamadı. Seçim yok.\n");
    }
}



void VulkanEngine::compute_ray_from_mouse(float mouseX, float mouseY, glm::vec3& outOrigin, glm::vec3& outDirection)
{
    glm::vec2 ndc = glm::vec2(mouseX, 1.0f - mouseY) * 2.0f - 1.0f;

    glm::mat4 invVP = glm::inverse(mainCamera.getProjectionMatrix() * mainCamera.getViewMatrix());

    glm::vec4 nearPoint = invVP * glm::vec4(ndc.x, ndc.y, 0.0f, 1.0f);
    glm::vec4 farPoint = invVP * glm::vec4(ndc.x, ndc.y, 1.0f, 1.0f);

    nearPoint /= nearPoint.w;
    farPoint /= farPoint.w;

    outOrigin = glm::vec3(nearPoint);
    outDirection = glm::normalize(glm::vec3(farPoint - nearPoint));
}



bool ray_intersects_aabb(const glm::vec3& rayOrigin, const glm::vec3& rayDir, const Bounds& bounds, const glm::mat4& transform, float& outDistance)
{
    glm::vec3 aabbMin = bounds.origin - bounds.extents;
    glm::vec3 aabbMax = bounds.origin + bounds.extents;

    glm::vec3 minPoint = glm::vec3(transform * glm::vec4(aabbMin, 1.0f));
    glm::vec3 maxPoint = glm::vec3(transform * glm::vec4(aabbMax, 1.0f));

    glm::vec3 t1 = (minPoint - rayOrigin) / rayDir;
    glm::vec3 t2 = (maxPoint - rayOrigin) / rayDir;

    glm::vec3 tminVec = glm::min(t1, t2);
    glm::vec3 tmaxVec = glm::max(t1, t2);

    float tNear = std::max({ tminVec.x, tminVec.y, tminVec.z });
    float tFar = std::min({ tmaxVec.x, tmaxVec.y, tmaxVec.z });

    if (tNear > tFar || tFar < 0.0f)
        return false;

    outDistance = (tNear >= 0.0f) ? tNear : tFar;
    return true;
}


MeshNode* VulkanEngine::raycast_scene_objects(const glm::vec3& rayOrigin, const glm::vec3& rayDir)
{
    fmt::print("[RAYCAST] {} obje kontrol edilecek.\n", pickableRenderObjects.size());

    MeshNode* bestHit = nullptr;
    float closestHit = 1e30f;

    for (const RenderObject& obj : pickableRenderObjects)
    {
        float hitDistance;
        if (ray_intersects_aabb(rayOrigin, rayDir, obj.bounds, obj.transform, hitDistance))
        {
            if (hitDistance < closestHit)
            {
                closestHit = hitDistance;
                bestHit = obj.nodePointer;
            }
        }
    }

    return bestHit;
}












//void VulkanEngine::draw_debug_aabb(VkCommandBuffer cmd, const RenderObject& obj, const glm::mat4& viewproj) {
//    // 8 köşe
//    std::array<glm::vec3, 8> corners{
//        glm::vec3{ 1, 1, 1 },
//        glm::vec3{ 1, 1, -1 },
//        glm::vec3{ 1, -1, 1 },
//        glm::vec3{ 1, -1, -1 },
//        glm::vec3{ -1, 1, 1 },
//        glm::vec3{ -1, 1, -1 },
//        glm::vec3{ -1, -1, 1 },
//        glm::vec3{ -1, -1, -1 },
//    };
//    glm::mat4 world = obj.transform;
//    std::array<glm::vec3, 8> worldCorners;
//    for (int c = 0; c < 8; c++) {
//        glm::vec3 localCorner = obj.bounds.origin + (corners[c] * obj.bounds.extents);
//        worldCorners[c] = glm::vec3(world * glm::vec4(localCorner, 1.0f));
//    }
//    // 12 kenar için çizgi çiz (örnek: 0-1, 1-2, 2-3, 3-0, ...)
//    // Bunu kendi çizgi çizim fonksiyonunla uygula
//}






//< visfn


//void VulkanEngine::draw_geometry(VkCommandBuffer cmd)
//{
//    // === SAHNE VERİSİ GPU'YA GÖNDERİLİYOR ===
//    AllocatedBuffer gpuSceneDataBuffer = create_buffer(sizeof(GPUSceneData),
//        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
//        VMA_MEMORY_USAGE_CPU_TO_GPU);
//
//    get_current_frame()._deletionQueue.push_function([=, this]() {
//        destroy_buffer(gpuSceneDataBuffer);
//        });
//
//    GPUSceneData* sceneUniformData = (GPUSceneData*)gpuSceneDataBuffer.allocation->GetMappedData();
//    *sceneUniformData = sceneData;
//
//    VkDescriptorSet globalDescriptor = get_current_frame()._frameDescriptors.allocate(_device, _gpuSceneDataDescriptorLayout);
//
//    DescriptorWriter writer;
//    writer.write_buffer(0, gpuSceneDataBuffer.buffer, sizeof(GPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
//    writer.update_set(_device, globalDescriptor);
//
//    // === 2D KÜP (ya da kare) çizimi ===
//    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _2dPipeline);
//    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
//        _2dPipelineLayout, 0, 1, &globalDescriptor, 0, nullptr);
//
//    VkDeviceSize offsets[] = { 0 };
//    vkCmdBindVertexBuffers(cmd, 0, 1, &rectangle.vertexBuffer.buffer, offsets);
//    vkCmdBindIndexBuffer(cmd, rectangle.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
//
//    GPUDrawPushConstants push2D{};
//    push2D.worldMatrix = glm::mat4(1.0f); // İsteğe göre pozisyon ver
//    push2D.worldMatrix = glm::translate(push2D.worldMatrix, glm::vec3(5.0f, 0.0f, -10.0f));
//    push2D.worldMatrix = glm::scale(push2D.worldMatrix, glm::vec3(1.0f, 1.0f, 5.0f));
//
//
//    push2D.vertexBuffer = rectangle.vertexBufferAddress;
//
//    vkCmdPushConstants(cmd, _2dPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0,
//        sizeof(GPUDrawPushConstants), &push2D);
//
//    vkCmdDrawIndexed(cmd, 36, 1, 0, 0, 0); // 🧊 12 üçgenlik küp = 36 index
//
//    // === 3D SAHNE OBJELERİNİ ÇİZ ===
//    std::vector<uint32_t> opaque_draws;
//    opaque_draws.reserve(drawCommands.OpaqueSurfaces.size());
//
//    for (int i = 0; i < drawCommands.OpaqueSurfaces.size(); i++) {
//        if (is_visible(drawCommands.OpaqueSurfaces[i], sceneData.viewproj)) {
//            opaque_draws.push_back(i);
//        }
//    }
//
//    std::sort(opaque_draws.begin(), opaque_draws.end(), [&](const auto& iA, const auto& iB) {
//        const RenderObject& A = drawCommands.OpaqueSurfaces[iA];
//        const RenderObject& B = drawCommands.OpaqueSurfaces[iB];
//        return A.material == B.material ? A.indexBuffer < B.indexBuffer : A.material < B.material;
//        });
//
//    MaterialPipeline* lastPipeline = nullptr;
//    MaterialInstance* lastMaterial = nullptr;
//    VkBuffer lastIndexBuffer = VK_NULL_HANDLE;
//
//    auto draw = [&](const RenderObject& r) {
//        if (!r.material || !r.material->pipeline) return;
//
//        if (r.material != lastMaterial) {
//            lastMaterial = r.material;
//
//            if (r.material->pipeline != lastPipeline) {
//                lastPipeline = r.material->pipeline;
//
//                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.material->pipeline->pipeline);
//                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
//                    r.material->pipeline->layout, 0, 1, &globalDescriptor, 0, nullptr);
//
//                VkViewport viewport{};
//                viewport.x = 0;
//                viewport.y = 0;
//                viewport.width = (float)_windowExtent.width;
//                viewport.height = (float)_windowExtent.height;
//                viewport.minDepth = 0.f;
//                viewport.maxDepth = 1.f;
//                vkCmdSetViewport(cmd, 0, 1, &viewport);
//
//                VkRect2D scissor{};
//                scissor.offset = { 0, 0 };
//                scissor.extent = _windowExtent;
//                vkCmdSetScissor(cmd, 0, 1, &scissor);
//            }
//
//            if (r.material->materialSet != VK_NULL_HANDLE) {
//                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
//                    r.material->pipeline->layout, 1, 1, &r.material->materialSet, 0, nullptr);
//            }
//        }
//
//        if (r.indexBuffer != lastIndexBuffer) {
//            lastIndexBuffer = r.indexBuffer;
//            vkCmdBindIndexBuffer(cmd, r.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
//        }
//
//        GPUDrawPushConstants push{};
//        push.worldMatrix = r.transform;
//        push.vertexBuffer = r.vertexBufferAddress;
//
//        vkCmdPushConstants(cmd, r.material->pipeline->layout, VK_SHADER_STAGE_VERTEX_BIT, 0,
//            sizeof(GPUDrawPushConstants), &push);
//
//        stats.drawcall_count++;
//        stats.triangle_count += r.indexCount / 3;
//
//        vkCmdDrawIndexed(cmd, r.indexCount, 1, r.firstIndex, 0, 0);
//        };
//
//    stats.drawcall_count = 0;
//    stats.triangle_count = 0;
//    stats.visible_count = 0;
//
//    for (auto& r : opaque_draws) {
//        draw(drawCommands.OpaqueSurfaces[r]);
//    }
//
//    for (auto& r : drawCommands.TransparentSurfaces) {
//        draw(r);
//    }
//
//    drawCommands.OpaqueSurfaces.clear();
//    drawCommands.TransparentSurfaces.clear();
//}
//void VulkanEngine::draw_geometry(VkCommandBuffer cmd)
//{
//    // === SAHNE VERİSİ GPU'YA GÖNDERİLİYOR ===
//    AllocatedBuffer gpuSceneDataBuffer = create_buffer(sizeof(GPUSceneData),
//        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
//        VMA_MEMORY_USAGE_CPU_TO_GPU);
//
//    get_current_frame()._deletionQueue.push_function([=, this]() {
//        destroy_buffer(gpuSceneDataBuffer);
//        });
//
//    GPUSceneData* sceneUniformData = (GPUSceneData*)gpuSceneDataBuffer.allocation->GetMappedData();
//    *sceneUniformData = sceneData;
//
//    VkDescriptorSet globalDescriptor = get_current_frame()._frameDescriptors.allocate(_device, _gpuSceneDataDescriptorLayout);
//
//    DescriptorWriter writer;
//    writer.write_buffer(0, gpuSceneDataBuffer.buffer, sizeof(GPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
//    writer.update_set(_device, globalDescriptor);
//
//    // === 2D KÜP (ya da kare) çizimi ===
//    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _2dPipeline);
//    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
//        _2dPipelineLayout, 0, 1, &globalDescriptor, 0, nullptr);
//
//    VkDeviceSize offsets[] = { 0 };
//    vkCmdBindVertexBuffers(cmd, 0, 1, &rectangle.vertexBuffer.buffer, offsets);
//    vkCmdBindIndexBuffer(cmd, rectangle.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
//
//    GPUDrawPushConstants push2D{};
//    push2D.worldMatrix = glm::mat4(1.0f); // İsteğe göre pozisyon ver
//    push2D.worldMatrix = glm::translate(push2D.worldMatrix, glm::vec3(5.0f, 0.0f, -10.0f));
//    push2D.worldMatrix = glm::scale(push2D.worldMatrix, glm::vec3(1.0f, 1.0f, 5.0f));
//
//
//    push2D.vertexBuffer = rectangle.vertexBufferAddress;
//
//    vkCmdPushConstants(cmd, _2dPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0,
//        sizeof(GPUDrawPushConstants), &push2D);
//
//    vkCmdDrawIndexed(cmd, 36, 1, 0, 0, 0); // 🧊 12 üçgenlik küp = 36 index
//
//    // === 3D SAHNE OBJELERİNİ ÇİZ ===
//    std::vector<uint32_t> opaque_draws;
//    opaque_draws.reserve(drawCommands.OpaqueSurfaces.size());
//
//    for (int i = 0; i < drawCommands.OpaqueSurfaces.size(); i++) {
//        if (is_visible(drawCommands.OpaqueSurfaces[i], sceneData.viewproj)) {
//            opaque_draws.push_back(i);
//        }
//    }
//
//    std::sort(opaque_draws.begin(), opaque_draws.end(), [&](const auto& iA, const auto& iB) {
//        const RenderObject& A = drawCommands.OpaqueSurfaces[iA];
//        const RenderObject& B = drawCommands.OpaqueSurfaces[iB];
//        return A.material == B.material ? A.indexBuffer < B.indexBuffer : A.material < B.material;
//        });
//
//    MaterialPipeline* lastPipeline = nullptr;
//    MaterialInstance* lastMaterial = nullptr;
//    VkBuffer lastIndexBuffer = VK_NULL_HANDLE;
//
//    auto draw = [&](const RenderObject& r) {
//        if (!r.material || !r.material->pipeline) return;
//
//        if (r.material != lastMaterial) {
//            lastMaterial = r.material;
//
//            if (r.material->pipeline != lastPipeline) {
//                lastPipeline = r.material->pipeline;
//
//                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.material->pipeline->pipeline);
//                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
//                    r.material->pipeline->layout, 0, 1, &globalDescriptor, 0, nullptr);
//
//                VkViewport viewport{};
//                viewport.x = 0;
//                viewport.y = 0;
//                viewport.width = (float)_windowExtent.width;
//                viewport.height = (float)_windowExtent.height;
//                viewport.minDepth = 0.f;
//                viewport.maxDepth = 1.f;
//                vkCmdSetViewport(cmd, 0, 1, &viewport);
//
//                VkRect2D scissor{};
//                scissor.offset = { 0, 0 };
//                scissor.extent = _windowExtent;
//                vkCmdSetScissor(cmd, 0, 1, &scissor);
//            }
//
//            if (r.material->materialSet != VK_NULL_HANDLE) {
//                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
//                    r.material->pipeline->layout, 1, 1, &r.material->materialSet, 0, nullptr);
//            }
//        }
//
//        if (r.indexBuffer != lastIndexBuffer) {
//            lastIndexBuffer = r.indexBuffer;
//            vkCmdBindIndexBuffer(cmd, r.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
//        }
//
//        GPUDrawPushConstants push{};
//        push.worldMatrix = r.transform;
//        push.vertexBuffer = r.vertexBufferAddress;
//
//        vkCmdPushConstants(cmd, r.material->pipeline->layout, VK_SHADER_STAGE_VERTEX_BIT, 0,
//            sizeof(GPUDrawPushConstants), &push);
//
//        stats.drawcall_count++;
//        stats.triangle_count += r.indexCount / 3;
//
//        vkCmdDrawIndexed(cmd, r.indexCount, 1, r.firstIndex, 0, 0);
//        };
//
//    stats.drawcall_count = 0;
//    stats.triangle_count = 0;
//    stats.visible_count = 0;
//
//    for (auto& r : opaque_draws) {
//        draw(drawCommands.OpaqueSurfaces[r]);
//    }
//
//    for (auto& r : drawCommands.TransparentSurfaces) {
//        draw(r);
//    }
//
//    drawCommands.OpaqueSurfaces.clear();
//    drawCommands.TransparentSurfaces.clear();
//}



//void VulkanEngine::draw_geometry(VkCommandBuffer cmd)
//{
//    AllocatedBuffer gpuSceneDataBuffer = create_buffer(sizeof(GPUSceneData),
//        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
//        VMA_MEMORY_USAGE_CPU_TO_GPU);
//
//    get_current_frame()._deletionQueue.push_function([=, this]() {
//        destroy_buffer(gpuSceneDataBuffer);
//        });
//
//    GPUSceneData* sceneUniformData = (GPUSceneData*)gpuSceneDataBuffer.allocation->GetMappedData();
//    *sceneUniformData = sceneData;
//
//    VkDescriptorSet globalDescriptor = get_current_frame()._frameDescriptors.allocate(_device, _gpuSceneDataDescriptorLayout);
//
//    DescriptorWriter writer;
//    writer.write_buffer(0, gpuSceneDataBuffer.buffer, sizeof(GPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
//    writer.update_set(_device, globalDescriptor);
//
//    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _2dPipeline);
//    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _2dPipelineLayout,
//        0, 1, &globalDescriptor, 0, nullptr);
//
//    VkDeviceSize offsets[] = { 0 };
//
//    for (auto& shape : static_shapes) {
//        vkCmdBindVertexBuffers(cmd, 0, 1, &shape.mesh.vertexBuffer.buffer, offsets);
//        vkCmdBindIndexBuffer(cmd, shape.mesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
//
//        GPUDrawPushConstants push{};
//        push.worldMatrix = shape.get_transform();
//        push.vertexBuffer = shape.mesh.vertexBufferAddress;
//
//        vkCmdPushConstants(cmd, _2dPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0,
//            sizeof(GPUDrawPushConstants), &push);
//
//        vkCmdDrawIndexed(cmd, shape.mesh.indexCount, 1, 0, 0, 0);
//    }
//
//    // === 3D SAHNE OBJELERİNİ ÇİZ ===
//    std::vector<uint32_t> opaque_draws;
//    opaque_draws.reserve(drawCommands.OpaqueSurfaces.size());
//
//    for (int i = 0; i < drawCommands.OpaqueSurfaces.size(); i++) {
//        if (is_visible(drawCommands.OpaqueSurfaces[i], sceneData.viewproj)) {
//            opaque_draws.push_back(i);
//        }
//    }
//
//    std::sort(opaque_draws.begin(), opaque_draws.end(), [&](const auto& iA, const auto& iB) {
//        const RenderObject& A = drawCommands.OpaqueSurfaces[iA];
//        const RenderObject& B = drawCommands.OpaqueSurfaces[iB];
//        return A.material == B.material ? A.indexBuffer < B.indexBuffer : A.material < B.material;
//        });
//
//    MaterialPipeline* lastPipeline = nullptr;
//    MaterialInstance* lastMaterial = nullptr;
//    VkBuffer lastIndexBuffer = VK_NULL_HANDLE;
//
//    auto draw = [&](const RenderObject& r) {
//        if (!r.material || !r.material->pipeline) return;
//
//        if (r.material != lastMaterial) {
//            lastMaterial = r.material;
//
//            if (r.material->pipeline != lastPipeline) {
//                lastPipeline = r.material->pipeline;
//
//                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.material->pipeline->pipeline);
//                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
//                    r.material->pipeline->layout, 0, 1, &globalDescriptor, 0, nullptr);
//
//                VkViewport viewport{};
//                viewport.x = 0;
//                viewport.y = 0;
//                viewport.width = (float)_windowExtent.width;
//                viewport.height = (float)_windowExtent.height;
//                viewport.minDepth = 0.f;
//                viewport.maxDepth = 1.f;
//                vkCmdSetViewport(cmd, 0, 1, &viewport);
//
//                VkRect2D scissor{};
//                scissor.offset = { 0, 0 };
//                scissor.extent = _windowExtent;
//                vkCmdSetScissor(cmd, 0, 1, &scissor);
//            }
//
//            if (r.material->materialSet != VK_NULL_HANDLE) {
//                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
//                    r.material->pipeline->layout, 1, 1, &r.material->materialSet, 0, nullptr);
//            }
//        }
//
//        if (r.indexBuffer != lastIndexBuffer) {
//            lastIndexBuffer = r.indexBuffer;
//            vkCmdBindIndexBuffer(cmd, r.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
//        }
//
//        GPUDrawPushConstants push{};
//        push.worldMatrix = r.transform;
//        push.vertexBuffer = r.vertexBufferAddress;
//
//        vkCmdPushConstants(cmd, r.material->pipeline->layout, VK_SHADER_STAGE_VERTEX_BIT, 0,
//            sizeof(GPUDrawPushConstants), &push);
//
//        stats.drawcall_count++;
//        stats.triangle_count += r.indexCount / 3;
//
//        vkCmdDrawIndexed(cmd, r.indexCount, 1, r.firstIndex, 0, 0);
//        };
//
//    for (auto& r : opaque_draws) {
//        draw(drawCommands.OpaqueSurfaces[r]);
//    }
//
//    for (auto& r : drawCommands.TransparentSurfaces) {
//        draw(r);
//    }
//
//    drawCommands.OpaqueSurfaces.clear();
//    drawCommands.TransparentSurfaces.clear();
//}
//void VulkanEngine::draw_geometry(VkCommandBuffer cmd)
//{
//    AllocatedBuffer gpuSceneDataBuffer = create_buffer(sizeof(GPUSceneData),
//        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
//        VMA_MEMORY_USAGE_CPU_TO_GPU);
//
//    get_current_frame()._deletionQueue.push_function([=, this]() {
//        destroy_buffer(gpuSceneDataBuffer);
//        });
//
//    GPUSceneData* sceneUniformData = (GPUSceneData*)gpuSceneDataBuffer.allocation->GetMappedData();
//    *sceneUniformData = sceneData;
//
//    VkDescriptorSet globalDescriptor = get_current_frame()._frameDescriptors.allocate(_device, _gpuSceneDataDescriptorLayout);
//
//    DescriptorWriter writer;
//    writer.write_buffer(0, gpuSceneDataBuffer.buffer, sizeof(GPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
//    writer.update_set(_device, globalDescriptor);
//
//    // === SHADER-ONLY KÜPLERİ ÇİZ ===
//    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _2dPipeline);
//    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _2dPipelineLayout,
//        0, 1, &globalDescriptor, 0, nullptr);
//
//    VkDeviceSize offsets[] = { 0 };
//
//    for (auto& shape : static_shapes) {
//        vkCmdBindVertexBuffers(cmd, 0, 1, &shape.mesh.vertexBuffer.buffer, offsets);
//        vkCmdBindIndexBuffer(cmd, shape.mesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
//
//        GPUDrawPushConstants push{};
//        push.worldMatrix = shape.get_transform();
//        push.vertexBuffer = shape.mesh.vertexBufferAddress;
//        memcpy(push.faceColors, shape.faceColors, sizeof(glm::vec4) * 6);
//
//        vkCmdPushConstants(cmd, _2dPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0,
//            sizeof(GPUDrawPushConstants), &push);
//
//        vkCmdDrawIndexed(cmd, shape.mesh.indexCount, 1, 0, 0, 0);
//    }
//
//    // === 3D SAHNE OBJELERİNİ ÇİZ ===
//    std::vector<uint32_t> opaque_draws;
//    opaque_draws.reserve(drawCommands.OpaqueSurfaces.size());
//
//    for (int i = 0; i < drawCommands.OpaqueSurfaces.size(); i++) {
//        if (is_visible(drawCommands.OpaqueSurfaces[i], sceneData.viewproj)) {
//            opaque_draws.push_back(i);
//        }
//		//opaque_draws.push_back(i); //  Tüm objeleri çizmeye sağlar yani culling olmaz
//    }
//
//    std::sort(opaque_draws.begin(), opaque_draws.end(), [&](const auto& iA, const auto& iB) {
//        const RenderObject& A = drawCommands.OpaqueSurfaces[iA];
//        const RenderObject& B = drawCommands.OpaqueSurfaces[iB];
//        return A.material == B.material ? A.indexBuffer < B.indexBuffer : A.material < B.material;
//        });
//
//    MaterialPipeline* lastPipeline = nullptr;
//    MaterialInstance* lastMaterial = nullptr;
//    VkBuffer lastIndexBuffer = VK_NULL_HANDLE;
//
//    auto draw = [&](const RenderObject& r) {
//        if (!r.material || !r.material->pipeline) return;
//
//        if (r.material != lastMaterial) {
//            lastMaterial = r.material;
//
//            if (r.material->pipeline != lastPipeline) {
//                lastPipeline = r.material->pipeline;
//
//                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.material->pipeline->pipeline);
//                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
//                    r.material->pipeline->layout, 0, 1, &globalDescriptor, 0, nullptr);
//
//                VkViewport viewport{};
//                viewport.x = 0;
//                viewport.y = 0;
//                viewport.width = (float)_windowExtent.width;
//                viewport.height = (float)_windowExtent.height;
//                viewport.minDepth = 0.f;
//                viewport.maxDepth = 1.f;
//                vkCmdSetViewport(cmd, 0, 1, &viewport);
//
//                VkRect2D scissor{};
//                scissor.offset = { 0, 0 };
//                scissor.extent = _windowExtent;
//                vkCmdSetScissor(cmd, 0, 1, &scissor);
//            }
//
//            if (r.material->materialSet != VK_NULL_HANDLE) {
//                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
//                    r.material->pipeline->layout, 1, 1, &r.material->materialSet, 0, nullptr);
//            }
//        }
//
//        if (r.indexBuffer != lastIndexBuffer) {
//            lastIndexBuffer = r.indexBuffer;
//            vkCmdBindIndexBuffer(cmd, r.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
//        }
//
//        GPUDrawPushConstants push{};
//        push.worldMatrix = r.transform;
//        push.vertexBuffer = r.vertexBufferAddress;
//
//        vkCmdPushConstants(cmd, r.material->pipeline->layout, VK_SHADER_STAGE_VERTEX_BIT, 0,
//            sizeof(GPUDrawPushConstants), &push);
//
//        stats.drawcall_count++;
//        stats.triangle_count += r.indexCount / 3;
//
//        vkCmdDrawIndexed(cmd, r.indexCount, 1, r.firstIndex, 0, 0);
//        };
//
//    for (auto& r : opaque_draws) draw(drawCommands.OpaqueSurfaces[r]);
//    for (auto& r : drawCommands.TransparentSurfaces) draw(r);
//
//    drawCommands.OpaqueSurfaces.clear();
//    drawCommands.TransparentSurfaces.clear();
//}
//void VulkanEngine::draw_geometry(VkCommandBuffer cmd)
//{
//    stats.drawcall_count = 0;
//    stats.triangle_count = 0;
//
//    AllocatedBuffer gpuSceneDataBuffer = create_buffer(sizeof(GPUSceneData),
//        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
//        VMA_MEMORY_USAGE_CPU_TO_GPU);
//
//    get_current_frame()._deletionQueue.push_function([=, this]() {
//        destroy_buffer(gpuSceneDataBuffer);
//        });
//
//    GPUSceneData* sceneUniformData = (GPUSceneData*)gpuSceneDataBuffer.allocation->GetMappedData();
//    *sceneUniformData = sceneData;
//
//    VkDescriptorSet globalDescriptor = get_current_frame()._frameDescriptors.allocate(_device, _gpuSceneDataDescriptorLayout);
//
//    DescriptorWriter writer;
//    writer.write_buffer(0, gpuSceneDataBuffer.buffer, sizeof(GPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
//    writer.update_set(_device, globalDescriptor);
//
//    // Viewport ve scissor ayarı (pipeline değişiminde tekrar ayarlamaya gerek yok)
//    VkViewport viewport{};
//    viewport.x = 0;
//    viewport.y = 0;
//    viewport.width = (float)_windowExtent.width;
//    viewport.height = (float)_windowExtent.height;
//    viewport.minDepth = 0.f;
//    viewport.maxDepth = 1.f;
//    vkCmdSetViewport(cmd, 0, 1, &viewport);
//
//    VkRect2D scissor{};
//    scissor.offset = { 0, 0 };
//    scissor.extent = _windowExtent;
//    vkCmdSetScissor(cmd, 0, 1, &scissor);
//
//    // === SHADER-ONLY KÜPLERİ ÇİZ ===
//    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _2dPipeline);
//    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _2dPipelineLayout,
//        0, 1, &globalDescriptor, 0, nullptr);
//
//    VkDeviceSize offsets[] = { 0 };
//
//    for (auto& shape : static_shapes) {
//        vkCmdBindVertexBuffers(cmd, 0, 1, &shape.mesh.vertexBuffer.buffer, offsets);
//        vkCmdBindIndexBuffer(cmd, shape.mesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
//
//        GPUDrawPushConstants push{};
//        push.worldMatrix = shape.get_transform();
//        push.vertexBuffer = shape.mesh.vertexBufferAddress;
//        memcpy(push.faceColors, shape.faceColors, sizeof(glm::vec4) * 6);
//
//        vkCmdPushConstants(cmd, _2dPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0,
//            sizeof(GPUDrawPushConstants), &push);
//
//        vkCmdDrawIndexed(cmd, shape.mesh.indexCount, 1, 0, 0, 0);
//    }
//
//    // === 3D SAHNE OBJELERİNİ ÇİZ ===
//    std::vector<uint32_t> opaque_draws;
//    opaque_draws.reserve(drawCommands.OpaqueSurfaces.size());
//
//    for (int i = 0; i < drawCommands.OpaqueSurfaces.size(); i++) {
//        if (is_visible(drawCommands.OpaqueSurfaces[i], sceneData.viewproj)) {
//            opaque_draws.push_back(i);
//        }
//    }
//
//    std::sort(opaque_draws.begin(), opaque_draws.end(), [&](const auto& iA, const auto& iB) {
//        const RenderObject& A = drawCommands.OpaqueSurfaces[iA];
//        const RenderObject& B = drawCommands.OpaqueSurfaces[iB];
//        return A.material == B.material ? A.indexBuffer < B.indexBuffer : A.material < B.material;
//        });
//
//    MaterialPipeline* lastPipeline = nullptr;
//    MaterialInstance* lastMaterial = nullptr;
//    VkBuffer lastIndexBuffer = VK_NULL_HANDLE;
//
//    auto draw = [&](const RenderObject& r) {
//        if (!r.material || !r.material->pipeline) return;
//
//        if (r.material != lastMaterial) {
//            lastMaterial = r.material;
//
//            if (r.material->pipeline != lastPipeline) {
//                lastPipeline = r.material->pipeline;
//                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.material->pipeline->pipeline);
//                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
//                    r.material->pipeline->layout, 0, 1, &globalDescriptor, 0, nullptr);
//            }
//
//            if (r.material->materialSet != VK_NULL_HANDLE) {
//                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
//                    r.material->pipeline->layout, 1, 1, &r.material->materialSet, 0, nullptr);
//            }
//        }
//
//        if (r.indexBuffer != lastIndexBuffer) {
//            lastIndexBuffer = r.indexBuffer;
//            vkCmdBindIndexBuffer(cmd, r.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
//        }
//
//        GPUDrawPushConstants push{};
//        push.worldMatrix = r.transform;
//        push.vertexBuffer = r.vertexBufferAddress;
//
//        vkCmdPushConstants(cmd, r.material->pipeline->layout, VK_SHADER_STAGE_VERTEX_BIT, 0,
//            sizeof(GPUDrawPushConstants), &push);
//
//        stats.drawcall_count++;
//        stats.triangle_count += r.indexCount / 3;
//
//        vkCmdDrawIndexed(cmd, r.indexCount, 1, r.firstIndex, 0, 0);
//        };
//
//    for (auto& r : opaque_draws) draw(drawCommands.OpaqueSurfaces[r]);
//    for (auto& r : drawCommands.TransparentSurfaces) draw(r);
//
//    stats.visible_count = static_cast<int>(opaque_draws.size() + drawCommands.TransparentSurfaces.size());
//
//    drawCommands.OpaqueSurfaces.clear();
//    drawCommands.TransparentSurfaces.clear();
//}

//void VulkanEngine::draw_geometry(VkCommandBuffer cmd)
//{
//    stats.drawcall_count = 0;
//    stats.triangle_count = 0;
//
//    // === GPU Scene Data Buffer ===
//    AllocatedBuffer gpuSceneDataBuffer = create_buffer(sizeof(GPUSceneData),
//        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
//        VMA_MEMORY_USAGE_CPU_TO_GPU);
//
//    get_current_frame()._deletionQueue.push_function([=, this]() {
//        destroy_buffer(gpuSceneDataBuffer);
//        });
//
//    GPUSceneData* sceneUniformData = (GPUSceneData*)gpuSceneDataBuffer.allocation->GetMappedData();
//    *sceneUniformData = sceneData;
//
//    // === Variable Descriptor Count ===
//    VkDescriptorSetVariableDescriptorCountAllocateInfo allocArrayInfo{
//        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO
//    };
//
//    uint32_t descriptorCounts = texCache.Cache.size();
//    allocArrayInfo.pDescriptorCounts = &descriptorCounts;
//    allocArrayInfo.descriptorSetCount = 1;
//
//    VkDescriptorSet globalDescriptor = get_current_frame()._frameDescriptors.allocate(
//        _device, _gpuSceneDataDescriptorLayout, &allocArrayInfo);
//
//    DescriptorWriter writer;
//    writer.write_buffer(0, gpuSceneDataBuffer.buffer, sizeof(GPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
//
//    if (texCache.Cache.size() > 0) {
//        VkWriteDescriptorSet arraySet{ .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
//        arraySet.descriptorCount = texCache.Cache.size();
//        arraySet.dstArrayElement = 0;
//        arraySet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
//        arraySet.dstBinding = 1;
//        arraySet.pImageInfo = texCache.Cache.data();
//        writer.writes.push_back(arraySet);
//    }
//
//    writer.update_set(_device, globalDescriptor);
//
//    // === Viewport & Scissor ===
//    VkViewport viewport{};
//    viewport.x = 0;
//    viewport.y = 0;
//    viewport.width = (float)_windowExtent.width;
//    viewport.height = (float)_windowExtent.height;
//    viewport.minDepth = 0.f;
//    viewport.maxDepth = 1.f;
//    vkCmdSetViewport(cmd, 0, 1, &viewport);
//
//    VkRect2D scissor{};
//    scissor.offset = { 0, 0 };
//    scissor.extent = _windowExtent;
//    vkCmdSetScissor(cmd, 0, 1, &scissor);
//
//    // === SHADER-ONLY KÜPLERİ ÇİZ ===
//    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _2dPipeline);
//    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _2dPipelineLayout,
//        0, 1, &globalDescriptor, 0, nullptr);
//
//    VkDeviceSize offsets[] = { 0 };
//
//    for (auto& shape : static_shapes) {
//        vkCmdBindVertexBuffers(cmd, 0, 1, &shape.mesh.vertexBuffer.buffer, offsets);
//        vkCmdBindIndexBuffer(cmd, shape.mesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
//
//        GPUDrawPushConstants push{};
//        push.worldMatrix = shape.get_transform();
//        push.vertexBuffer = shape.mesh.vertexBufferAddress;
//        memcpy(push.faceColors, shape.faceColors, sizeof(glm::vec4) * 6);
//
//        vkCmdPushConstants(cmd, _2dPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0,
//            sizeof(GPUDrawPushConstants), &push);
//
//        vkCmdDrawIndexed(cmd, shape.mesh.indexCount, 1, 0, 0, 0);
//    }
//
//    // === 3D SAHNE OBJELERİNİ ÇİZ ===
//    std::vector<uint32_t> opaque_draws;
//    opaque_draws.reserve(drawCommands.OpaqueSurfaces.size());
//
//    for (int i = 0; i < drawCommands.OpaqueSurfaces.size(); i++) {
//        if (is_visible(drawCommands.OpaqueSurfaces[i], sceneData.viewproj)) {
//            opaque_draws.push_back(i);
//        }
//    }
//
//    std::sort(opaque_draws.begin(), opaque_draws.end(), [&](const auto& iA, const auto& iB) {
//        const RenderObject& A = drawCommands.OpaqueSurfaces[iA];
//        const RenderObject& B = drawCommands.OpaqueSurfaces[iB];
//        return A.material == B.material ? A.indexBuffer < B.indexBuffer : A.material < B.material;
//        });
//
//    MaterialPipeline* lastPipeline = nullptr;
//    MaterialInstance* lastMaterial = nullptr;
//    VkBuffer lastIndexBuffer = VK_NULL_HANDLE;
//
//    auto draw = [&](const RenderObject& r) {
//        if (!r.material || !r.material->pipeline) return;
//
//        if (r.material != lastMaterial) {
//            lastMaterial = r.material;
//
//            if (r.material->pipeline != lastPipeline) {
//                lastPipeline = r.material->pipeline;
//                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.material->pipeline->pipeline);
//                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
//                    r.material->pipeline->layout, 0, 1, &globalDescriptor, 0, nullptr);
//
//                vkCmdSetViewport(cmd, 0, 1, &viewport);
//                vkCmdSetScissor(cmd, 0, 1, &scissor);
//            }
//
//            if (r.material->materialSet != VK_NULL_HANDLE) {
//                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
//                    r.material->pipeline->layout, 1, 1, &r.material->materialSet, 0, nullptr);
//            }
//        }
//
//        if (r.indexBuffer != lastIndexBuffer) {
//            lastIndexBuffer = r.indexBuffer;
//            vkCmdBindIndexBuffer(cmd, r.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
//        }
//
//        GPUDrawPushConstants push{};
//        push.worldMatrix = r.transform;
//        push.vertexBuffer = r.vertexBufferAddress;
//
//        vkCmdPushConstants(cmd, r.material->pipeline->layout, VK_SHADER_STAGE_VERTEX_BIT, 0,
//            sizeof(GPUDrawPushConstants), &push);
//
//        stats.drawcall_count++;
//        stats.triangle_count += r.indexCount / 3;
//
//        vkCmdDrawIndexed(cmd, r.indexCount, 1, r.firstIndex, 0, 0);
//        };
//
//    for (auto& r : opaque_draws) draw(drawCommands.OpaqueSurfaces[r]);
//    for (auto& r : drawCommands.TransparentSurfaces) draw(r);
//
//    stats.visible_count = static_cast<int>(opaque_draws.size() + drawCommands.TransparentSurfaces.size());
//
//    drawCommands.OpaqueSurfaces.clear();
//    drawCommands.TransparentSurfaces.clear();
//}



//void VulkanEngine::draw_geometry(VkCommandBuffer cmd)
//{
//    stats.drawcall_count = 0;
//    stats.triangle_count = 0;
//
//    // === GPU Scene Data Buffer ===
//    AllocatedBuffer gpuSceneDataBuffer = create_buffer(sizeof(GPUSceneData),
//        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
//        VMA_MEMORY_USAGE_CPU_TO_GPU);
//
//    get_current_frame()._deletionQueue.push_function([=, this]() {
//        destroy_buffer(gpuSceneDataBuffer);
//        });
//
//    GPUSceneData* sceneUniformData = (GPUSceneData*)gpuSceneDataBuffer.allocation->GetMappedData();
//    *sceneUniformData = sceneData;
//
//    // === Variable Descriptor Count ===
//    VkDescriptorSetVariableDescriptorCountAllocateInfo allocArrayInfo{
//        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO
//    };
//
//    uint32_t descriptorCounts = texCache.Cache.size();
//    allocArrayInfo.pDescriptorCounts = &descriptorCounts;
//    allocArrayInfo.descriptorSetCount = 1;
//
//    VkDescriptorSet globalDescriptor = get_current_frame()._frameDescriptors.allocate(
//        _device, _gpuSceneDataDescriptorLayout, &allocArrayInfo);
//
//    DescriptorWriter writer;
//    writer.write_buffer(0, gpuSceneDataBuffer.buffer, sizeof(GPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
//
//    if (texCache.Cache.size() > 0) {
//        VkWriteDescriptorSet arraySet{ .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
//        arraySet.descriptorCount = texCache.Cache.size();
//        arraySet.dstArrayElement = 0;
//        arraySet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
//        arraySet.dstBinding = 1;
//        arraySet.pImageInfo = texCache.Cache.data();
//        writer.writes.push_back(arraySet);
//    }
//
//    writer.update_set(_device, globalDescriptor);
//
//    // === Viewport & Scissor ===
//    VkViewport viewport{};
//    viewport.x = 0;
//    viewport.y = 0;
//    viewport.width = (float)_windowExtent.width;
//    viewport.height = (float)_windowExtent.height;
//    viewport.minDepth = 0.f;
//    viewport.maxDepth = 1.f;
//
//    VkRect2D scissor{};
//    scissor.offset = { 0, 0 };
//    scissor.extent = _windowExtent;
//
//    // === SHADER-ONLY KÜPLERİ ÇİZ ===
//    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _2dPipeline);
//    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _2dPipelineLayout,
//        0, 1, &globalDescriptor, 0, nullptr);
//
//    VkDeviceSize offsets[] = { 0 };
//
//    for (auto& shape : static_shapes) {
//        vkCmdBindVertexBuffers(cmd, 0, 1, &shape.mesh.vertexBuffer.buffer, offsets);
//        vkCmdBindIndexBuffer(cmd, shape.mesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
//
//        GPUDrawPushConstants push{};
//        push.worldMatrix = shape.get_transform();
//        push.vertexBuffer = shape.mesh.vertexBufferAddress;
//        memcpy(push.faceColors, shape.faceColors, sizeof(glm::vec4) * 6);
//
//        vkCmdPushConstants(cmd, _2dPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0,
//            sizeof(GPUDrawPushConstants), &push);
//
//        vkCmdDrawIndexed(cmd, shape.mesh.indexCount, 1, 0, 0, 0);
//    }
//
//    // === 3D SAHNE OBJELERİNİ ÇİZ ===
//    std::vector<uint32_t> opaque_draws;
//    opaque_draws.reserve(drawCommands.OpaqueSurfaces.size());
//
//    for (int i = 0; i < drawCommands.OpaqueSurfaces.size(); i++) {
//        if (is_visible(drawCommands.OpaqueSurfaces[i], sceneData.viewproj)) {
//            opaque_draws.push_back(i);
//        }
//    }
//
//    std::sort(opaque_draws.begin(), opaque_draws.end(), [&](const auto& iA, const auto& iB) {
//        const RenderObject& A = drawCommands.OpaqueSurfaces[iA];
//        const RenderObject& B = drawCommands.OpaqueSurfaces[iB];
//        return A.material == B.material ? A.indexBuffer < B.indexBuffer : A.material < B.material;
//        });
//
//    MaterialPipeline* lastPipeline = nullptr;
//    MaterialInstance* lastMaterial = nullptr;
//    VkBuffer lastIndexBuffer = VK_NULL_HANDLE;
//
//    auto draw = [&](const RenderObject& r) {
//        if (!r.material || !r.material->pipeline) return;
//
//        if (r.material != lastMaterial) {
//            lastMaterial = r.material;
//
//            if (r.material->pipeline != lastPipeline) {
//                lastPipeline = r.material->pipeline;
//                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.material->pipeline->pipeline);
//                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
//                    r.material->pipeline->layout, 0, 1, &globalDescriptor, 0, nullptr);
//
//                vkCmdSetViewport(cmd, 0, 1, &viewport);
//                vkCmdSetScissor(cmd, 0, 1, &scissor);
//            }
//
//            if (r.material->materialSet != VK_NULL_HANDLE) {
//                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
//                    r.material->pipeline->layout, 1, 1, &r.material->materialSet, 0, nullptr);
//            }
//        }
//
//        if (r.indexBuffer != lastIndexBuffer) {
//            lastIndexBuffer = r.indexBuffer;
//            vkCmdBindIndexBuffer(cmd, r.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
//        }
//
//        GPUDrawPushConstants push{};
//        push.worldMatrix = r.transform;
//        push.vertexBuffer = r.vertexBufferAddress;
//
//        vkCmdPushConstants(cmd, r.material->pipeline->layout, VK_SHADER_STAGE_VERTEX_BIT, 0,
//            sizeof(GPUDrawPushConstants), &push);
//
//        stats.drawcall_count++;
//        stats.triangle_count += r.indexCount / 3;
//
//        vkCmdDrawIndexed(cmd, r.indexCount, 1, r.firstIndex, 0, 0);
//        };
//
//    for (auto& r : opaque_draws) draw(drawCommands.OpaqueSurfaces[r]);
//    for (auto& r : drawCommands.TransparentSurfaces) draw(r);
//
//    stats.visible_count = static_cast<int>(opaque_draws.size() + drawCommands.TransparentSurfaces.size());
//
//    drawCommands.OpaqueSurfaces.clear();
//    drawCommands.TransparentSurfaces.clear();
//}

VkViewport VulkanEngine::get_letterbox_viewport() const
{
    float targetAspect = 16.0f / 9.0f; // veya istediğiniz sabit oran
    float winAspect = static_cast<float>(_windowExtent.width) / static_cast<float>(_windowExtent.height);

    float vpWidth, vpHeight, vpX, vpY;

    if (winAspect > targetAspect) {
        // Pencere geniş, üst-alt siyah bar
        vpHeight = static_cast<float>(_windowExtent.height);
        vpWidth = vpHeight * targetAspect;
        vpX = (_windowExtent.width - vpWidth) * 0.5f;
        vpY = 0.0f;
    }
    else {
        // Pencere dar, sağ-sol siyah bar
        vpWidth = static_cast<float>(_windowExtent.width);
        vpHeight = vpWidth / targetAspect;
        vpX = 0.0f;
        vpY = (_windowExtent.height - vpHeight) * 0.5f;
    }

    VkViewport viewport{};
    viewport.x = vpX;
    viewport.y = vpY;
    viewport.width = vpWidth;
    viewport.height = vpHeight;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    return viewport;
}



//void VulkanEngine::draw_geometry(VkCommandBuffer cmd)
//{
//    stats.drawcall_count = 0;
//    stats.triangle_count = 0;
//
//    // === GPU Scene Data Buffer ===
//    AllocatedBuffer gpuSceneDataBuffer = create_buffer(sizeof(GPUSceneData),
//        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
//        VMA_MEMORY_USAGE_CPU_TO_GPU);
//
//    get_current_frame()._deletionQueue.push_function([=, this]() {
//        destroy_buffer(gpuSceneDataBuffer);
//        });
//
//    GPUSceneData* sceneUniformData = (GPUSceneData*)gpuSceneDataBuffer.allocation->GetMappedData();
//    *sceneUniformData = sceneData;
//
//    // === Variable Descriptor Count ===
//    VkDescriptorSetVariableDescriptorCountAllocateInfo allocArrayInfo{
//        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO
//    };
//
//    uint32_t descriptorCounts = static_cast<uint32_t>(texCache.Cache.size());
//    allocArrayInfo.pDescriptorCounts = &descriptorCounts;
//    allocArrayInfo.descriptorSetCount = 1;
//
//    VkDescriptorSet globalDescriptor = get_current_frame()._frameDescriptors.allocate(
//        _device, _gpuSceneDataDescriptorLayout, &allocArrayInfo);
//
//    DescriptorWriter writer;
//    writer.write_buffer(0, gpuSceneDataBuffer.buffer, sizeof(GPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
//
//    if (!texCache.Cache.empty()) {
//        VkWriteDescriptorSet arraySet{ .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
//        arraySet.descriptorCount = static_cast<uint32_t>(texCache.Cache.size());
//        arraySet.dstArrayElement = 0;
//        arraySet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
//        arraySet.dstBinding = 1;
//        arraySet.pImageInfo = texCache.Cache.data();
//        writer.writes.push_back(arraySet);
//    }
//
//    writer.update_set(_device, globalDescriptor);
//
//    // === Viewport & Scissor ===
//    VkViewport viewport = get_letterbox_viewport();
//    vkCmdSetViewport(cmd, 0, 1, &viewport);
//
//    VkRect2D scissor{};
//    scissor.offset = { 0, 0 };
//    scissor.extent = _windowExtent;
//    vkCmdSetScissor(cmd, 0, 1, &scissor);
//
//    // === SHADER-ONLY KÜPLERİ ÇİZ ===
//    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _2dPipeline);
//    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _2dPipelineLayout,
//        0, 1, &globalDescriptor, 0, nullptr);
//
//    VkDeviceSize offsets[] = { 0 };
//
//    for (auto& shape : static_shapes) {
//        vkCmdBindVertexBuffers(cmd, 0, 1, &shape.mesh.vertexBuffer.buffer, offsets);
//        vkCmdBindIndexBuffer(cmd, shape.mesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
//
//        GPUDrawPushConstants push{};
//        push.worldMatrix = shape.get_transform();
//        push.vertexBuffer = shape.mesh.vertexBufferAddress;
//        memcpy(push.faceColors, shape.faceColors, sizeof(glm::vec4) * 6);
//
//        vkCmdPushConstants(cmd, _2dPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0,
//            sizeof(GPUDrawPushConstants), &push);
//
//        vkCmdDrawIndexed(cmd, shape.mesh.indexCount, 1, 0, 0, 0);
//    }
//
//    // === 3D SAHNE OBJELERİNİ ÇİZ ===
//    std::vector<uint32_t> opaque_draws;
//    opaque_draws.reserve(drawCommands.OpaqueSurfaces.size());
//
//    for (uint32_t i = 0; i < drawCommands.OpaqueSurfaces.size(); i++) {
//        if (is_visible(drawCommands.OpaqueSurfaces[i], sceneData.viewproj)) {
//            opaque_draws.push_back(i);
//        }
//    }
//
//    std::sort(opaque_draws.begin(), opaque_draws.end(), [&](uint32_t a, uint32_t b) {
//        const RenderObject& A = drawCommands.OpaqueSurfaces[a];
//        const RenderObject& B = drawCommands.OpaqueSurfaces[b];
//        return A.material == B.material ? A.indexBuffer < B.indexBuffer : A.material < B.material;
//        });
//
//    MaterialPipeline* lastPipeline = nullptr;
//    MaterialInstance* lastMaterial = nullptr;
//    VkBuffer lastIndexBuffer = VK_NULL_HANDLE;
//
//    auto draw = [&](const RenderObject& r) {
//        if (!r.material || !r.material->pipeline) return;
//
//        if (r.material != lastMaterial) {
//            lastMaterial = r.material;
//
//            if (r.material->pipeline != lastPipeline) {
//                lastPipeline = r.material->pipeline;
//                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.material->pipeline->pipeline);
//                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.material->pipeline->layout,
//                    0, 1, &globalDescriptor, 0, nullptr);
//
//                vkCmdSetViewport(cmd, 0, 1, &viewport);
//                vkCmdSetScissor(cmd, 0, 1, &scissor);
//            }
//
//            if (r.material->materialSet != VK_NULL_HANDLE) {
//                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.material->pipeline->layout,
//                    1, 1, &r.material->materialSet, 0, nullptr);
//            }
//        }
//
//        if (r.indexBuffer != lastIndexBuffer) {
//            lastIndexBuffer = r.indexBuffer;
//            vkCmdBindIndexBuffer(cmd, r.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
//        }
//
//        GPUDrawPushConstants push{};
//        push.worldMatrix = r.transform;
//        push.vertexBuffer = r.vertexBufferAddress;
//
//        vkCmdPushConstants(cmd, r.material->pipeline->layout, VK_SHADER_STAGE_VERTEX_BIT, 0,
//            sizeof(GPUDrawPushConstants), &push);
//
//        stats.drawcall_count++;
//        stats.triangle_count += r.indexCount / 3;
//
//        vkCmdDrawIndexed(cmd, r.indexCount, 1, r.firstIndex, 0, 0);
//        };
//
//    for (auto& idx : opaque_draws) draw(drawCommands.OpaqueSurfaces[idx]);
//    for (auto& r : drawCommands.TransparentSurfaces) draw(r);
//
//    stats.visible_count = static_cast<int>(opaque_draws.size() + drawCommands.TransparentSurfaces.size());
//
//    drawCommands.OpaqueSurfaces.clear();
//    drawCommands.TransparentSurfaces.clear();
//}
//



//void VulkanEngine::draw_geometry(VkCommandBuffer cmd)
//{
//    stats.drawcall_count = 0;
//    stats.triangle_count = 0;
//
//    // === GPU Scene Data Buffer ===
//    AllocatedBuffer gpuSceneDataBuffer = create_buffer(sizeof(GPUSceneData),
//        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
//        VMA_MEMORY_USAGE_CPU_TO_GPU);
//
//    get_current_frame()._deletionQueue.push_function([=, this]() {
//        destroy_buffer(gpuSceneDataBuffer);
//        });
//
//    GPUSceneData* sceneUniformData = (GPUSceneData*)gpuSceneDataBuffer.allocation->GetMappedData();
//    *sceneUniformData = sceneData;
//
//    // === Variable Descriptor Count ===
//    VkDescriptorSetVariableDescriptorCountAllocateInfo allocArrayInfo{
//        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO
//    };
//
//    uint32_t descriptorCounts = static_cast<uint32_t>(texCache.Cache.size());
//    allocArrayInfo.pDescriptorCounts = &descriptorCounts;
//    allocArrayInfo.descriptorSetCount = 1;
//
//    VkDescriptorSet globalDescriptor = get_current_frame()._frameDescriptors.allocate(
//        _device, _gpuSceneDataDescriptorLayout, &allocArrayInfo);
//
//    DescriptorWriter writer;
//    writer.write_buffer(0, gpuSceneDataBuffer.buffer, sizeof(GPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
//
//    if (!texCache.Cache.empty()) {
//        VkWriteDescriptorSet arraySet{ .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
//        arraySet.descriptorCount = static_cast<uint32_t>(texCache.Cache.size());
//        arraySet.dstArrayElement = 0;
//        arraySet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
//        arraySet.dstBinding = 1;
//        arraySet.pImageInfo = texCache.Cache.data();
//        writer.writes.push_back(arraySet);
//    }
//
//    writer.update_set(_device, globalDescriptor);
//
//    // === Viewport & Scissor ===
//    VkViewport viewport = get_letterbox_viewport();
//    vkCmdSetViewport(cmd, 0, 1, &viewport);
//
//    VkRect2D scissor{};
//    scissor.offset = { 0, 0 };
//    scissor.extent = _windowExtent;
//    vkCmdSetScissor(cmd, 0, 1, &scissor);
//
//    // === GRID YARDIMCI DÜZLEMİNİ ÇİZ ===
//    {
//        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, gridPipeline);
//
//        VkDescriptorSet descriptorSets[] = { globalDescriptor };
//        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, gridPipelineLayout,
//            0, 1, descriptorSets, 0, nullptr);
//
//        GPUDrawPushConstants push{};
//        push.worldMatrix = glm::mat4(1.0f);
//        push.vertexBuffer = 0;
//        memset(push.faceColors, 0, sizeof(push.faceColors));
//
//        vkCmdPushConstants(cmd, gridPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
//            sizeof(GPUDrawPushConstants), &push);
//
//        vkCmdDraw(cmd, 6, 1, 0, 0); // fullscreen quad
//    }
//
//    // === SHADER-ONLY KÜPLERİ ÇİZ ===
//    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _2dPipeline);
//    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _2dPipelineLayout,
//        0, 1, &globalDescriptor, 0, nullptr);
//
//    VkDeviceSize offsets[] = { 0 };
//
//    for (auto& shape : static_shapes) {
//        vkCmdBindVertexBuffers(cmd, 0, 1, &shape.mesh.vertexBuffer.buffer, offsets);
//        vkCmdBindIndexBuffer(cmd, shape.mesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
//
//        GPUDrawPushConstants push{};
//        push.worldMatrix = shape.get_transform();
//        push.vertexBuffer = shape.mesh.vertexBufferAddress;
//        memcpy(push.faceColors, shape.faceColors, sizeof(glm::vec4) * 6);
//
//        vkCmdPushConstants(cmd, _2dPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0,
//            sizeof(GPUDrawPushConstants), &push);
//
//        vkCmdDrawIndexed(cmd, shape.mesh.indexCount, 1, 0, 0, 0);
//    }
//
//    // === 3D SAHNE OBJELERİNİ ÇİZ ===
//    std::vector<uint32_t> opaque_draws;
//    opaque_draws.reserve(drawCommands.OpaqueSurfaces.size());
//
//    for (uint32_t i = 0; i < drawCommands.OpaqueSurfaces.size(); i++) {
//        if (is_visible(drawCommands.OpaqueSurfaces[i], sceneData.viewproj)) {
//            opaque_draws.push_back(i);
//        }
//    }
//
//    std::sort(opaque_draws.begin(), opaque_draws.end(), [&](uint32_t a, uint32_t b) {
//        const RenderObject& A = drawCommands.OpaqueSurfaces[a];
//        const RenderObject& B = drawCommands.OpaqueSurfaces[b];
//        return A.material == B.material ? A.indexBuffer < B.indexBuffer : A.material < B.material;
//        });
//
//    MaterialPipeline* lastPipeline = nullptr;
//    MaterialInstance* lastMaterial = nullptr;
//    VkBuffer lastIndexBuffer = VK_NULL_HANDLE;
//
//    auto draw = [&](const RenderObject& r) {
//        if (!r.material || !r.material->pipeline) return;
//
//        if (r.material != lastMaterial) {
//            lastMaterial = r.material;
//
//            if (r.material->pipeline != lastPipeline) {
//                lastPipeline = r.material->pipeline;
//                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.material->pipeline->pipeline);
//                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.material->pipeline->layout,
//                    0, 1, &globalDescriptor, 0, nullptr);
//
//                vkCmdSetViewport(cmd, 0, 1, &viewport);
//                vkCmdSetScissor(cmd, 0, 1, &scissor);
//            }
//
//            if (r.material->materialSet != VK_NULL_HANDLE) {
//                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.material->pipeline->layout,
//                    1, 1, &r.material->materialSet, 0, nullptr);
//            }
//        }
//
//        if (r.indexBuffer != lastIndexBuffer) {
//            lastIndexBuffer = r.indexBuffer;
//            vkCmdBindIndexBuffer(cmd, r.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
//        }
//
//        GPUDrawPushConstants push{};
//        push.worldMatrix = r.transform;
//        push.vertexBuffer = r.vertexBufferAddress;
//
//        vkCmdPushConstants(cmd, r.material->pipeline->layout, VK_SHADER_STAGE_VERTEX_BIT, 0,
//            sizeof(GPUDrawPushConstants), &push);
//
//        stats.drawcall_count++;
//        stats.triangle_count += r.indexCount / 3;
//
//        vkCmdDrawIndexed(cmd, r.indexCount, 1, r.firstIndex, 0, 0);
//        };
//
//    for (auto& idx : opaque_draws) draw(drawCommands.OpaqueSurfaces[idx]);
//    for (auto& r : drawCommands.TransparentSurfaces) draw(r);
//
//    stats.visible_count = static_cast<int>(opaque_draws.size() + drawCommands.TransparentSurfaces.size());
//
//    drawCommands.OpaqueSurfaces.clear();
//    drawCommands.TransparentSurfaces.clear();
//}
//
//void VulkanEngine::draw_geometry(VkCommandBuffer cmd)
//{
//    std::vector<uint32_t> opaque_draws;
//    opaque_draws.reserve(drawCommands.OpaqueSurfaces.size());
//
//    for (int i = 0; i < drawCommands.OpaqueSurfaces.size(); i++) {
//        if (is_visible(drawCommands.OpaqueSurfaces[i], sceneData.viewproj)) {
//            opaque_draws.push_back(i);
//        }
//    }
//
//    //allocate a new uniform buffer for the scene data
//    AllocatedBuffer gpuSceneDataBuffer = create_buffer(sizeof(GPUSceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
//
//    //add it to the deletion queue of this frame so it gets deleted once its been used
//    get_current_frame()._deletionQueue.push_function([=, this]() {
//        destroy_buffer(gpuSceneDataBuffer);
//        });
//
//    //write the buffer
//    GPUSceneData* sceneUniformData = (GPUSceneData*)gpuSceneDataBuffer.allocation->GetMappedData();
//    *sceneUniformData = sceneData;
//
//    VkDescriptorSetVariableDescriptorCountAllocateInfo allocArrayInfo{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO, .pNext = nullptr };
//
//    uint32_t descriptorCounts = texCache.Cache.size();
//    allocArrayInfo.pDescriptorCounts = &descriptorCounts;
//    allocArrayInfo.descriptorSetCount = 1;
//
//
//    //create a descriptor set that binds that buffer and update it
//    VkDescriptorSet globalDescriptor = get_current_frame()._frameDescriptors.allocate(_device, _gpuSceneDataDescriptorLayout, &allocArrayInfo);
//
//    DescriptorWriter writer;
//    writer.write_buffer(0, gpuSceneDataBuffer.buffer, sizeof(GPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
//
//
//    if (texCache.Cache.size() > 0) {
//        VkWriteDescriptorSet arraySet{ .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
//        arraySet.descriptorCount = texCache.Cache.size();
//        arraySet.dstArrayElement = 0;
//        arraySet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
//        arraySet.dstBinding = 1;
//        arraySet.pImageInfo = texCache.Cache.data();
//        writer.writes.push_back(arraySet);
//    }
//
//    writer.update_set(_device, globalDescriptor);
//
//    MaterialPipeline* lastPipeline = nullptr;
//    MaterialInstance* lastMaterial = nullptr;
//    VkBuffer lastIndexBuffer = VK_NULL_HANDLE;
//
//    auto draw = [&](const RenderObject& r) {
//        if (r.material != lastMaterial) {
//            lastMaterial = r.material;
//            if (r.material->pipeline != lastPipeline) {
//
//                lastPipeline = r.material->pipeline;
//                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.material->pipeline->pipeline);
//                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.material->pipeline->layout, 0, 1,
//                    &globalDescriptor, 0, nullptr);
//
//                VkViewport viewport = {};
//                viewport.x = 0;
//                viewport.y = 0;
//                viewport.width = (float)_drawExtent.width;
//                viewport.height = (float)_drawExtent.height;
//                viewport.minDepth = 0.f;
//                viewport.maxDepth = 1.f;
//
//                vkCmdSetViewport(cmd, 0, 1, &viewport);
//
//                VkRect2D scissor = {};
//                scissor.offset.x = 0;
//                scissor.offset.y = 0;
//                scissor.extent.width = _drawExtent.width;
//                scissor.extent.height = _drawExtent.height;
//
//                vkCmdSetScissor(cmd, 0, 1, &scissor);
//            }
//
//            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.material->pipeline->layout, 1, 1,
//                &r.material->materialSet, 0, nullptr);
//        }
//        if (r.indexBuffer != lastIndexBuffer) {
//            lastIndexBuffer = r.indexBuffer;
//            vkCmdBindIndexBuffer(cmd, r.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
//        }
//        // calculate final mesh matrix
//        GPUDrawPushConstants push_constants;
//        push_constants.worldMatrix = r.transform;
//        push_constants.vertexBuffer = r.vertexBufferAddress;
//
//        vkCmdPushConstants(cmd, r.material->pipeline->layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GPUDrawPushConstants), &push_constants);
//
//        stats.drawcall_count++;
//        stats.triangle_count += r.indexCount / 3;
//        vkCmdDrawIndexed(cmd, r.indexCount, 1, r.firstIndex, 0, 0);
//        };
//
//    stats.drawcall_count = 0;
//    stats.triangle_count = 0;
//
//    for (auto& r : opaque_draws) {
//        draw(drawCommands.OpaqueSurfaces[r]);
//    }
//
//    for (auto& r : drawCommands.TransparentSurfaces) {
//        draw(r);
//    }
//
//    // we delete the draw commands now that we processed them
//    drawCommands.OpaqueSurfaces.clear();
//    drawCommands.TransparentSurfaces.clear();
//}
//


//void VulkanEngine::draw_geometry(VkCommandBuffer cmd)
//{
//    std::vector<uint32_t> opaque_draws;
//    opaque_draws.reserve(drawCommands.OpaqueSurfaces.size());
//
//    for (int i = 0; i < drawCommands.OpaqueSurfaces.size(); i++) {
//        if (is_visible(drawCommands.OpaqueSurfaces[i], sceneData.viewproj)) {
//            opaque_draws.push_back(i);
//        }
//    }
//
//    // GPU Scene Data Buffer oluştur ve içerisine güncel sceneData'yı kopyala
//    AllocatedBuffer gpuSceneDataBuffer = create_buffer(sizeof(GPUSceneData),
//        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
//        VMA_MEMORY_USAGE_CPU_TO_GPU);
//
//    get_current_frame()._deletionQueue.push_function([=, this]() {
//        destroy_buffer(gpuSceneDataBuffer);
//        });
//
//    GPUSceneData* sceneUniformData = (GPUSceneData*)gpuSceneDataBuffer.allocation->GetMappedData();
//    *sceneUniformData = sceneData;
//
//    // Descriptor set variable descriptor count info
//    VkDescriptorSetVariableDescriptorCountAllocateInfo allocArrayInfo{
//        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO,
//        .pNext = nullptr
//    };
//
//    uint32_t descriptorCounts = static_cast<uint32_t>(texCache.Cache.size());
//    allocArrayInfo.pDescriptorCounts = &descriptorCounts;
//    allocArrayInfo.descriptorSetCount = 1;
//
//    // Descriptor set al
//    VkDescriptorSet globalDescriptor = get_current_frame()._frameDescriptors.allocate(
//        _device, _gpuSceneDataDescriptorLayout, &allocArrayInfo);
//
//    // Descriptor yazıcı ayarla ve güncelle
//    DescriptorWriter writer;
//    writer.write_buffer(0, gpuSceneDataBuffer.buffer, sizeof(GPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
//
//    if (!texCache.Cache.empty()) {
//        VkWriteDescriptorSet arraySet{ .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
//        arraySet.descriptorCount = static_cast<uint32_t>(texCache.Cache.size());
//        arraySet.dstArrayElement = 0;
//        arraySet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
//        arraySet.dstBinding = 1;
//        arraySet.pImageInfo = texCache.Cache.data();
//        writer.writes.push_back(arraySet);
//    }
//
//    writer.update_set(_device, globalDescriptor);
//
//    // Viewport ve scissor ayarları
//    VkViewport viewport = get_letterbox_viewport();
//    vkCmdSetViewport(cmd, 0, 1, &viewport);
//
//    VkRect2D scissor{};
//    scissor.offset = { 0, 0 };
//    scissor.extent = _windowExtent;
//    vkCmdSetScissor(cmd, 0, 1, &scissor);
//
//    // Shader-only küpler (static_shapes) çizimi
//    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _2dPipeline);
//    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _2dPipelineLayout,
//        0, 1, &globalDescriptor, 0, nullptr);
//
//    VkDeviceSize offsets[] = { 0 };
//
//    for (auto& shape : static_shapes) {
//        vkCmdBindVertexBuffers(cmd, 0, 1, &shape.mesh.vertexBuffer.buffer, offsets);
//        vkCmdBindIndexBuffer(cmd, shape.mesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
//
//        GPUDrawPushConstants push{};
//        push.worldMatrix = shape.get_transform();
//        push.vertexBuffer = shape.mesh.vertexBufferAddress;
//        // faceColors kaldırıldı, o yüzden burayı kaldırdım
//
//        vkCmdPushConstants(cmd, _2dPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0,
//            sizeof(GPUDrawPushConstants), &push);
//
//        vkCmdDrawIndexed(cmd, shape.mesh.indexCount, 1, 0, 0, 0);
//    }
//
//    // 3D sahne objelerinin çizimi
//    MaterialPipeline* lastPipeline = nullptr;
//    MaterialInstance* lastMaterial = nullptr;
//    VkBuffer lastIndexBuffer = VK_NULL_HANDLE;
//
//    auto draw = [&](const RenderObject& r) {
//        if (!r.material || !r.material->pipeline) return;
//
//        if (r.material != lastMaterial) {
//            lastMaterial = r.material;
//
//            if (r.material->pipeline != lastPipeline) {
//                lastPipeline = r.material->pipeline;
//                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.material->pipeline->pipeline);
//                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.material->pipeline->layout,
//                    0, 1, &globalDescriptor, 0, nullptr);
//
//                vkCmdSetViewport(cmd, 0, 1, &viewport);
//                vkCmdSetScissor(cmd, 0, 1, &scissor);
//            }
//
//            if (r.material->materialSet != VK_NULL_HANDLE) {
//                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.material->pipeline->layout,
//                    1, 1, &r.material->materialSet, 0, nullptr);
//            }
//        }
//
//        if (r.indexBuffer != lastIndexBuffer) {
//            lastIndexBuffer = r.indexBuffer;
//            vkCmdBindIndexBuffer(cmd, r.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
//        }
//
//        GPUDrawPushConstants push{};
//        push.worldMatrix = r.transform;
//        push.vertexBuffer = r.vertexBufferAddress;
//
//        vkCmdPushConstants(cmd, r.material->pipeline->layout, VK_SHADER_STAGE_VERTEX_BIT, 0,
//            sizeof(GPUDrawPushConstants), &push);
//
//        stats.drawcall_count++;
//        stats.triangle_count += r.indexCount / 3;
//
//        vkCmdDrawIndexed(cmd, r.indexCount, 1, r.firstIndex, 0, 0);
//        };
//
//    stats.drawcall_count = 0;
//    stats.triangle_count = 0;
//
//    for (auto& idx : opaque_draws) {
//        draw(drawCommands.OpaqueSurfaces[idx]);
//    }
//    for (auto& r : drawCommands.TransparentSurfaces) {
//        draw(r);
//    }
//
//    drawCommands.OpaqueSurfaces.clear();
//    drawCommands.TransparentSurfaces.clear();
//}



//
//
//void VulkanEngine::draw_geometry(VkCommandBuffer cmd)
//{
//    std::vector<uint32_t> opaque_draws;
//    opaque_draws.reserve(drawCommands.OpaqueSurfaces.size());
//
//    for (int i = 0; i < drawCommands.OpaqueSurfaces.size(); i++) {
//        if (is_visible(drawCommands.OpaqueSurfaces[i], sceneData.viewproj)) {
//            opaque_draws.push_back(i);
//        }
//    }
//
//    AllocatedBuffer gpuSceneDataBuffer = create_buffer(sizeof(GPUSceneData),
//        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
//        VMA_MEMORY_USAGE_CPU_TO_GPU);
//
//    get_current_frame()._deletionQueue.push_function([=, this]() {
//        destroy_buffer(gpuSceneDataBuffer);
//        });
//
//    GPUSceneData* sceneUniformData = (GPUSceneData*)gpuSceneDataBuffer.allocation->GetMappedData();
//    *sceneUniformData = sceneData;
//
//    VkDescriptorSetVariableDescriptorCountAllocateInfo allocArrayInfo{
//        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO,
//        .pNext = nullptr
//    };
//
//    uint32_t descriptorCounts = static_cast<uint32_t>(texCache.Cache.size());
//    allocArrayInfo.pDescriptorCounts = &descriptorCounts;
//    allocArrayInfo.descriptorSetCount = 1;
//
//    VkDescriptorSet globalDescriptor = get_current_frame()._frameDescriptors.allocate(
//        _device, _gpuSceneDataDescriptorLayout, &allocArrayInfo);
//
//    DescriptorWriter writer;
//    writer.write_buffer(0, gpuSceneDataBuffer.buffer, sizeof(GPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
//
//    if (!texCache.Cache.empty()) {
//        VkWriteDescriptorSet arraySet{ .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
//        arraySet.descriptorCount = static_cast<uint32_t>(texCache.Cache.size());
//        arraySet.dstArrayElement = 0;
//        arraySet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
//        arraySet.dstBinding = 1;
//        arraySet.pImageInfo = texCache.Cache.data();
//        writer.writes.push_back(arraySet);
//    }
//
//    writer.update_set(_device, globalDescriptor);
//
//    VkViewport viewport = get_letterbox_viewport();
//    vkCmdSetViewport(cmd, 0, 1, &viewport);
//
//    VkRect2D scissor{};
//    scissor.offset = { 0, 0 };
//    scissor.extent = _windowExtent;
//    vkCmdSetScissor(cmd, 0, 1, &scissor);
//
//    stats.drawcall_count = 0;
//    stats.triangle_count = 0;
//
//    // ====== VIEWMODE: SHADED ======
//    if (_currentViewMode == ViewMode::Shaded) {
//        VkPipeline pipeline = enableBackfaceCulling ? _2dPipeline_CullOn : _2dPipeline_CullOff;
//
//        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
//        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _2dPipelineLayout, 0, 1, &globalDescriptor, 0, nullptr);
//
//        VkDeviceSize offsets[] = { 0 };
//        for (auto& shape : static_shapes) {
//            vkCmdBindVertexBuffers(cmd, 0, 1, &shape.mesh.vertexBuffer.buffer, offsets);
//            vkCmdBindIndexBuffer(cmd, shape.mesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
//
//            GPUDrawPushConstants push{};
//            push.worldMatrix = shape.get_transform();
//            push.vertexBuffer = shape.mesh.vertexBufferAddress;
//            push.faceColors[0] = shape.color;
//
//            vkCmdPushConstants(cmd, _2dPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
//                sizeof(GPUDrawPushConstants), &push);
//
//            vkCmdDrawIndexed(cmd, shape.mesh.indexCount, 1, 0, 0, 0);
//        }
//
//        return; // sadece shader-only çizilecek
//    }
//
//    // ====== VIEWMODE: RENDERED ======
//
//    MaterialPipeline* lastPipeline = nullptr;
//    MaterialInstance* lastMaterial = nullptr;
//    VkBuffer lastIndexBuffer = VK_NULL_HANDLE;
//
//    auto draw = [&](const RenderObject& r) {
//        if (!r.material || !r.material->pipeline) return;
//
//        MaterialPipeline* selectedPipeline = r.material->pipeline;
//
//        if (_currentViewMode == ViewMode::Shaded) {
//            // ileriye dönük lightweight pipeline seçimi buraya gelebilir.
//            // şimdilik kullanılmıyor.
//        }
//
//        if (r.material != lastMaterial) {
//            lastMaterial = r.material;
//
//            if (selectedPipeline != lastPipeline) {
//                lastPipeline = selectedPipeline;
//                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, selectedPipeline->pipeline);
//                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, selectedPipeline->layout,
//                    0, 1, &globalDescriptor, 0, nullptr);
//
//                vkCmdSetViewport(cmd, 0, 1, &viewport);
//                vkCmdSetScissor(cmd, 0, 1, &scissor);
//            }
//
//            if (r.material->materialSet != VK_NULL_HANDLE) {
//                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, selectedPipeline->layout,
//                    1, 1, &r.material->materialSet, 0, nullptr);
//            }
//        }
//
//        if (r.indexBuffer != lastIndexBuffer) {
//            lastIndexBuffer = r.indexBuffer;
//            vkCmdBindIndexBuffer(cmd, r.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
//        }
//
//        GPUDrawPushConstants push{};
//        push.worldMatrix = r.transform;
//        push.vertexBuffer = r.vertexBufferAddress;
//        memcpy(push.faceColors, r.faceColor, sizeof(glm::vec4) * 6);
//
//        vkCmdPushConstants(cmd, selectedPipeline->layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
//            sizeof(GPUDrawPushConstants), &push);
//
//        stats.drawcall_count++;
//        stats.triangle_count += r.indexCount / 3;
//
//        vkCmdDrawIndexed(cmd, r.indexCount, 1, r.firstIndex, 0, 0);
//        };
//
//    // sadece ViewMode::Rendered modda RenderObject'ler çizilsin
//    if (_currentViewMode == ViewMode::Rendered) {
//        for (auto& idx : opaque_draws) draw(drawCommands.OpaqueSurfaces[idx]);
//        for (auto& r : drawCommands.TransparentSurfaces) draw(r);
//    }
//
//    drawCommands.OpaqueSurfaces.clear();
//    drawCommands.TransparentSurfaces.clear();
//}
//void VulkanEngine::draw_geometry(VkCommandBuffer cmd)
//{
//    std::vector<uint32_t> opaque_draws;
//    opaque_draws.reserve(drawCommands.OpaqueSurfaces.size());
//
//    for (int i = 0; i < drawCommands.OpaqueSurfaces.size(); i++) {
//        if (is_visible(drawCommands.OpaqueSurfaces[i], sceneData.viewproj)) {
//            opaque_draws.push_back(i);
//        }
//    }
//
//    //allocate a new uniform buffer for the scene data
//    AllocatedBuffer gpuSceneDataBuffer = create_buffer(sizeof(GPUSceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
//
//    //add it to the deletion queue of this frame so it gets deleted once its been used
//    get_current_frame()._deletionQueue.push_function([=, this]() {
//        destroy_buffer(gpuSceneDataBuffer);
//        });
//
//    //write the buffer
//    GPUSceneData* sceneUniformData = (GPUSceneData*)gpuSceneDataBuffer.allocation->GetMappedData();
//    *sceneUniformData = sceneData;
//
//    VkDescriptorSetVariableDescriptorCountAllocateInfo allocArrayInfo{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO, .pNext = nullptr };
//
//    uint32_t descriptorCounts = texCache.Cache.size();
//    allocArrayInfo.pDescriptorCounts = &descriptorCounts;
//    allocArrayInfo.descriptorSetCount = 1;
//
//
//    //create a descriptor set that binds that buffer and update it
//    VkDescriptorSet globalDescriptor = get_current_frame()._frameDescriptors.allocate(_device, _gpuSceneDataDescriptorLayout, &allocArrayInfo);
//
//    DescriptorWriter writer;
//    writer.write_buffer(0, gpuSceneDataBuffer.buffer, sizeof(GPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
//
//
//    if (texCache.Cache.size() > 0) {
//        VkWriteDescriptorSet arraySet{ .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
//        arraySet.descriptorCount = texCache.Cache.size();
//        arraySet.dstArrayElement = 0;
//        arraySet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
//        arraySet.dstBinding = 1;
//        arraySet.pImageInfo = texCache.Cache.data();
//        writer.writes.push_back(arraySet);
//    }
//
//    writer.update_set(_device, globalDescriptor);
//
//    MaterialPipeline* lastPipeline = nullptr;
//    MaterialInstance* lastMaterial = nullptr;
//    VkBuffer lastIndexBuffer = VK_NULL_HANDLE;
//
//    auto draw = [&](const RenderObject& r) {
//        if (r.material != lastMaterial) {
//            lastMaterial = r.material;
//            if (r.material->pipeline != lastPipeline) {
//
//                lastPipeline = r.material->pipeline;
//                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.material->pipeline->pipeline);
//                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.material->pipeline->layout, 0, 1,
//                    &globalDescriptor, 0, nullptr);
//
//                VkViewport viewport = {};
//                viewport.x = 0;
//                viewport.y = 0;
//                viewport.width = (float)_drawExtent.width;
//                viewport.height = (float)_drawExtent.height;
//                viewport.minDepth = 0.f;
//                viewport.maxDepth = 1.f;
//
//                vkCmdSetViewport(cmd, 0, 1, &viewport);
//
//                VkRect2D scissor = {};
//                scissor.offset.x = 0;
//                scissor.offset.y = 0;
//                scissor.extent.width = _drawExtent.width;
//                scissor.extent.height = _drawExtent.height;
//
//                vkCmdSetScissor(cmd, 0, 1, &scissor);
//            }
//
//            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.material->pipeline->layout, 1, 1,
//                &r.material->materialSet, 0, nullptr);
//        }
//        if (r.indexBuffer != lastIndexBuffer) {
//            lastIndexBuffer = r.indexBuffer;
//            vkCmdBindIndexBuffer(cmd, r.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
//        }
//        // calculate final mesh matrix
//        GPUDrawPushConstants push_constants;
//        push_constants.worldMatrix = r.transform;
//        push_constants.vertexBuffer = r.vertexBufferAddress;
//
//        vkCmdPushConstants(cmd, r.material->pipeline->layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GPUDrawPushConstants), &push_constants);
//
//        stats.drawcall_count++;
//        stats.triangle_count += r.indexCount / 3;
//        vkCmdDrawIndexed(cmd, r.indexCount, 1, r.firstIndex, 0, 0);
//        };
//
//    stats.drawcall_count = 0;
//    stats.triangle_count = 0;
//
//    for (auto& r : opaque_draws) {
//        draw(drawCommands.OpaqueSurfaces[r]);
//    }
//
//    for (auto& r : drawCommands.TransparentSurfaces) {
//        draw(r);
//    }
//
//    // we delete the draw commands now that we processed them
//    drawCommands.OpaqueSurfaces.clear();
//    drawCommands.TransparentSurfaces.clear();
//}
//
//

//void VulkanEngine::draw_geometry(VkCommandBuffer cmd)
//{
//    // Uniform buffer oluştur ve kontrol et
//    AllocatedBuffer gpuSceneDataBuffer = create_buffer(
//        sizeof(GPUSceneData),
//        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
//        VMA_MEMORY_USAGE_CPU_TO_GPU
//    );
//
//    if (gpuSceneDataBuffer.buffer == VK_NULL_HANDLE) {
//        fmt::println("❌ gpuSceneDataBuffer.buffer null! create_buffer başarısız.");
//        return; // crash engellenir
//    }
//
//    get_current_frame()._deletionQueue.push_function([=, this]() {
//        destroy_buffer(gpuSceneDataBuffer);
//        });
//
//    GPUSceneData* sceneUniformData = (GPUSceneData*)gpuSceneDataBuffer.allocation->GetMappedData();
//    *sceneUniformData = sceneData;
//
//    // Descriptor set oluştur
//    VkDescriptorSetVariableDescriptorCountAllocateInfo allocArrayInfo{
//        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO,
//        .pNext = nullptr
//    };
//
//    uint32_t descriptorCounts = static_cast<uint32_t>(texCache.Cache.size());
//    allocArrayInfo.pDescriptorCounts = &descriptorCounts;
//    allocArrayInfo.descriptorSetCount = 1;
//
//    VkDescriptorSet globalDescriptor = get_current_frame()._frameDescriptors.allocate(
//        _device, _gpuSceneDataDescriptorLayout, &allocArrayInfo
//    );
//
//    DescriptorWriter writer;
//    writer.write_buffer(0, gpuSceneDataBuffer.buffer, sizeof(GPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
//
//    if (!texCache.Cache.empty()) {
//        VkWriteDescriptorSet arraySet{ .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
//        arraySet.descriptorCount = descriptorCounts;
//        arraySet.dstArrayElement = 0;
//        arraySet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
//        arraySet.dstBinding = 1;
//        arraySet.pImageInfo = texCache.Cache.data();
//        writer.writes.push_back(arraySet);
//    }
//
//    writer.update_set(_device, globalDescriptor);
//
//    // Görünürlük filtresi
//    std::vector<uint32_t> opaque_draws;
//    opaque_draws.reserve(drawCommands.OpaqueSurfaces.size());
//
//    for (int i = 0; i < drawCommands.OpaqueSurfaces.size(); i++) {
//        if (is_visible(drawCommands.OpaqueSurfaces[i], sceneData.viewproj)) {
//            opaque_draws.push_back(i);
//        }
//    }
//
//    stats.drawcall_count = 0;
//    stats.triangle_count = 0;
//
//    VkViewport viewport = get_letterbox_viewport();
//    VkRect2D scissor = { {0, 0}, _windowExtent };
//
//    // === ViewMode::Wireframe ===
//    if (_currentViewMode == ViewMode::Wireframe) {
//        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _wireframePipeline);
//        vkCmdSetViewport(cmd, 0, 1, &viewport);
//        vkCmdSetScissor(cmd, 0, 1, &scissor);
//
//        for (auto& idx : opaque_draws) {
//            const RenderObject& r = drawCommands.OpaqueSurfaces[idx];
//            if (!r.material || r.material->materialSet == VK_NULL_HANDLE) continue;
//
//            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _wireframePipelineLayout,
//                0, 1, &globalDescriptor, 0, nullptr);
//
//            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _wireframePipelineLayout,
//                1, 1, &r.material->materialSet, 0, nullptr);
//
//            vkCmdBindIndexBuffer(cmd, r.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
//
//            GPUDrawPushConstants push{};
//            push.worldMatrix = r.transform;
//            push.vertexBuffer = r.vertexBufferAddress;
//
//            vkCmdPushConstants(cmd, _wireframePipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0,
//                sizeof(GPUDrawPushConstants), &push);
//
//            vkCmdDrawIndexed(cmd, r.indexCount, 1, r.firstIndex, 0, 0);
//        }
//
//        drawCommands.OpaqueSurfaces.clear();
//        drawCommands.TransparentSurfaces.clear();
//        return;
//    }
//
//
//
//
//    // === ViewMode::Shaded ===
//    if (_currentViewMode == ViewMode::Shaded) {
//        VkPipeline pipeline = enableBackfaceCulling ? _2dPipeline_CullOn : _2dPipeline_CullOff;
//
//        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
//        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _2dPipelineLayout, 0, 1, &globalDescriptor, 0, nullptr);
//        vkCmdSetViewport(cmd, 0, 1, &viewport);
//        vkCmdSetScissor(cmd, 0, 1, &scissor);
//
//        for (auto& shape : static_shapes) {
//            vkCmdBindVertexBuffers(cmd, 0, 1, &shape.mesh.vertexBuffer.buffer, nullptr);
//            vkCmdBindIndexBuffer(cmd, shape.mesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
//
//            GPUDrawPushConstants push{};
//            push.worldMatrix = shape.get_transform();
//            push.vertexBuffer = shape.mesh.vertexBufferAddress;
//
//            vkCmdPushConstants(cmd, _2dPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
//                sizeof(GPUDrawPushConstants), &push);
//
//            vkCmdDrawIndexed(cmd, shape.mesh.indexCount, 1, 0, 0, 0);
//        }
//
//        drawCommands.OpaqueSurfaces.clear();
//        drawCommands.TransparentSurfaces.clear();
//        return;
//    }
//
//    // === ViewMode::MaterialPreview ===
//    if (_currentViewMode == ViewMode::MaterialPreview) {
//        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _materialPreviewPipeline);
//        vkCmdSetViewport(cmd, 0, 1, &viewport);
//        vkCmdSetScissor(cmd, 0, 1, &scissor);
//
//        for (auto& idx : opaque_draws) {
//            const RenderObject& r = drawCommands.OpaqueSurfaces[idx];
//            if (!r.material || r.material->materialSet == VK_NULL_HANDLE) continue;
//
//            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _materialPreviewPipelineLayout,
//                0, 1, &globalDescriptor, 0, nullptr);
//
//            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _materialPreviewPipelineLayout,
//                1, 1, &r.material->materialSet, 0, nullptr);
//
//            if (r.indexBuffer != VK_NULL_HANDLE) {
//                vkCmdBindIndexBuffer(cmd, r.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
//
//                GPUDrawPushConstants push{};
//                push.worldMatrix = r.transform;
//                push.vertexBuffer = r.vertexBufferAddress;
//
//                vkCmdPushConstants(cmd, _materialPreviewPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0,
//                    sizeof(GPUDrawPushConstants), &push);
//
//                stats.drawcall_count++;
//                stats.triangle_count += r.indexCount / 3;
//
//                vkCmdDrawIndexed(cmd, r.indexCount, 1, r.firstIndex, 0, 0);
//            }
//        }
//
//        drawCommands.OpaqueSurfaces.clear();
//        drawCommands.TransparentSurfaces.clear();
//        return;
//    }
//
//    // === ViewMode::Rendered ===
//    MaterialPipeline* lastPipeline = nullptr;
//    MaterialInstance* lastMaterial = nullptr;
//    VkBuffer lastIndexBuffer = VK_NULL_HANDLE;
//
//    auto draw = [&](const RenderObject& r) {
//        if (!r.material || !r.material->pipeline) return;
//
//        if (r.material != lastMaterial) {
//            lastMaterial = r.material;
//
//            if (r.material->pipeline != lastPipeline) {
//                lastPipeline = r.material->pipeline;
//                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.material->pipeline->pipeline);
//                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.material->pipeline->layout, 0, 1, &globalDescriptor, 0, nullptr);
//                vkCmdSetViewport(cmd, 0, 1, &viewport);
//                vkCmdSetScissor(cmd, 0, 1, &scissor);
//            }
//
//            if (r.material->materialSet != VK_NULL_HANDLE) {
//                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.material->pipeline->layout,
//                    1, 1, &r.material->materialSet, 0, nullptr);
//            }
//        }
//
//        if (r.indexBuffer != lastIndexBuffer) {
//            lastIndexBuffer = r.indexBuffer;
//            vkCmdBindIndexBuffer(cmd, r.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
//        }
//
//        GPUDrawPushConstants push{};
//        push.worldMatrix = r.transform;
//        push.vertexBuffer = r.vertexBufferAddress;
//gra
//        vkCmdPushConstants(cmd, r.material->pipeline->layout, VK_SHADER_STAGE_VERTEX_BIT, 0,
//            sizeof(GPUDrawPushConstants), &push);
//
//        stats.drawcall_count++;
//        stats.triangle_count += r.indexCount / 3;
//        vkCmdDrawIndexed(cmd, r.indexCount, 1, r.firstIndex, 0, 0);
//        };
//
//    for (auto& idx : opaque_draws) draw(drawCommands.OpaqueSurfaces[idx]);
//    for (auto& r : drawCommands.TransparentSurfaces) draw(r);
//
//    drawCommands.OpaqueSurfaces.clear();
//    drawCommands.TransparentSurfaces.clear();
//}



//void VulkanEngine::draw_geometry(VkCommandBuffer cmd)
//{
//    // Uniform buffer oluştur ve kontrol et
//    AllocatedBuffer gpuSceneDataBuffer = create_buffer(
//        sizeof(GPUSceneData),
//        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
//        VMA_MEMORY_USAGE_CPU_TO_GPU
//    );
//
//    if (gpuSceneDataBuffer.buffer == VK_NULL_HANDLE) {
//        fmt::println("❌ gpuSceneDataBuffer.buffer null! create_buffer başarısız.");
//        return; // crash engellenir
//    }
//
//    get_current_frame()._deletionQueue.push_function([=, this]() {
//        destroy_buffer(gpuSceneDataBuffer);
//        });
//
//    GPUSceneData* sceneUniformData = (GPUSceneData*)gpuSceneDataBuffer.allocation->GetMappedData();
//    *sceneUniformData = sceneData;
//
//    // Descriptor set oluştur
//    VkDescriptorSetVariableDescriptorCountAllocateInfo allocArrayInfo{
//        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO,
//        .pNext = nullptr
//    };
//
//    uint32_t descriptorCounts = static_cast<uint32_t>(texCache.Cache.size());
//    allocArrayInfo.pDescriptorCounts = &descriptorCounts;
//    allocArrayInfo.descriptorSetCount = 1;
//
//    VkDescriptorSet globalDescriptor = get_current_frame()._frameDescriptors.allocate(
//        _device, _gpuSceneDataDescriptorLayout, &allocArrayInfo
//    );
//
//    DescriptorWriter writer;
//    writer.write_buffer(0, gpuSceneDataBuffer.buffer, sizeof(GPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
//
//    if (!texCache.Cache.empty()) {
//        VkWriteDescriptorSet arraySet{ .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
//        arraySet.descriptorCount = descriptorCounts;
//        arraySet.dstArrayElement = 0;
//        arraySet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
//        arraySet.dstBinding = 1;
//        arraySet.pImageInfo = texCache.Cache.data();
//        writer.writes.push_back(arraySet);
//    }
//
//    writer.update_set(_device, globalDescriptor);
//
//    // Görünürlük filtresi
//    std::vector<uint32_t> opaque_draws;
//    opaque_draws.reserve(drawCommands.OpaqueSurfaces.size());
//
//    for (int i = 0; i < drawCommands.OpaqueSurfaces.size(); i++) {
//        if (is_visible(drawCommands.OpaqueSurfaces[i], sceneData.viewproj)) {
//            opaque_draws.push_back(i);
//        }
//    }
//
//    stats.drawcall_count = 0;
//    stats.triangle_count = 0;
//
//    VkViewport viewport = get_letterbox_viewport();
//    VkRect2D scissor = { {0, 0}, _windowExtent };
//
//    // === ViewMode::Wireframe ===
//    if (_currentViewMode == ViewMode::Wireframe) {
//        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _wireframePipeline);
//        vkCmdSetViewport(cmd, 0, 1, &viewport);
//        vkCmdSetScissor(cmd, 0, 1, &scissor);
//
//        for (auto& idx : opaque_draws) {
//            const RenderObject& r = drawCommands.OpaqueSurfaces[idx];
//            if (!r.material || r.material->materialSet == VK_NULL_HANDLE) continue;
//
//            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _wireframePipelineLayout,
//                0, 1, &globalDescriptor, 0, nullptr);
//
//            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _wireframePipelineLayout,
//                1, 1, &r.material->materialSet, 0, nullptr);
//
//            vkCmdBindIndexBuffer(cmd, r.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
//
//            GPUDrawPushConstants push{};
//            push.worldMatrix = r.transform;
//            push.vertexBuffer = r.vertexBufferAddress;
//
//            vkCmdPushConstants(cmd, _wireframePipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0,
//                sizeof(GPUDrawPushConstants), &push);
//
//            vkCmdDrawIndexed(cmd, r.indexCount, 1, r.firstIndex, 0, 0);
//        }
//
//        drawCommands.OpaqueSurfaces.clear();
//        drawCommands.TransparentSurfaces.clear();
//        return;
//    }
//
//
//
//
//    // === ViewMode::Shaded ===
//    if (_currentViewMode == ViewMode::Shaded) {
//        VkPipeline pipeline = enableBackfaceCulling ? _2dPipeline_CullOn : _2dPipeline_CullOff;
//
//        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
//        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _2dPipelineLayout, 0, 1, &globalDescriptor, 0, nullptr);
//        vkCmdSetViewport(cmd, 0, 1, &viewport);
//        vkCmdSetScissor(cmd, 0, 1, &scissor);
//
//        for (auto& shape : static_shapes) {
//            vkCmdBindVertexBuffers(cmd, 0, 1, &shape.mesh.vertexBuffer.buffer, nullptr);
//            vkCmdBindIndexBuffer(cmd, shape.mesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
//
//            GPUDrawPushConstants push{};
//            push.worldMatrix = shape.get_transform();
//            push.vertexBuffer = shape.mesh.vertexBufferAddress;
//
//            vkCmdPushConstants(cmd, _2dPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
//                sizeof(GPUDrawPushConstants), &push);
//
//            vkCmdDrawIndexed(cmd, shape.mesh.indexCount, 1, 0, 0, 0);
//        }
//
//        drawCommands.OpaqueSurfaces.clear();
//        drawCommands.TransparentSurfaces.clear();
//        return;
//    }
//
//    // === ViewMode::MaterialPreview ===
//    if (_currentViewMode == ViewMode::MaterialPreview) {
//        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _materialPreviewPipeline);
//        vkCmdSetViewport(cmd, 0, 1, &viewport);
//        vkCmdSetScissor(cmd, 0, 1, &scissor);
//
//        for (auto& idx : opaque_draws) {
//            const RenderObject& r = drawCommands.OpaqueSurfaces[idx];
//            if (!r.material || r.material->materialSet == VK_NULL_HANDLE) continue;
//
//            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _materialPreviewPipelineLayout,
//                0, 1, &globalDescriptor, 0, nullptr);
//
//            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _materialPreviewPipelineLayout,
//                1, 1, &r.material->materialSet, 0, nullptr);
//
//            if (r.indexBuffer != VK_NULL_HANDLE) {
//                vkCmdBindIndexBuffer(cmd, r.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
//
//                GPUDrawPushConstants push{};
//                push.worldMatrix = r.transform;
//                push.vertexBuffer = r.vertexBufferAddress;
//
//                vkCmdPushConstants(cmd, _materialPreviewPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0,
//                    sizeof(GPUDrawPushConstants), &push);
//
//                stats.drawcall_count++;
//                stats.triangle_count += r.indexCount / 3;
//
//                vkCmdDrawIndexed(cmd, r.indexCount, 1, r.firstIndex, 0, 0);
//            }
//        }
//
//        drawCommands.OpaqueSurfaces.clear();
//        drawCommands.TransparentSurfaces.clear();
//        return;
//    }
//
//    // === ViewMode::Rendered ===
//    MaterialPipeline* lastPipeline = nullptr;
//    MaterialInstance* lastMaterial = nullptr;
//    VkBuffer lastIndexBuffer = VK_NULL_HANDLE;
//
//    auto draw = [&](const RenderObject& r) {
//        if (!r.material || !r.material->pipeline) return;
//
//        if (r.material != lastMaterial) {
//            lastMaterial = r.material;
//
//            if (r.material->pipeline != lastPipeline) {
//                lastPipeline = r.material->pipeline;
//                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.material->pipeline->pipeline);
//                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.material->pipeline->layout, 0, 1, &globalDescriptor, 0, nullptr);
//                vkCmdSetViewport(cmd, 0, 1, &viewport);
//                vkCmdSetScissor(cmd, 0, 1, &scissor);
//            }
//
//            if (r.material->materialSet != VK_NULL_HANDLE) {
//                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.material->pipeline->layout,
//                    1, 1, &r.material->materialSet, 0, nullptr);
//            }
//        }
//
//        if (r.indexBuffer != lastIndexBuffer) {
//            lastIndexBuffer = r.indexBuffer;
//            vkCmdBindIndexBuffer(cmd, r.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
//        }
//
//        GPUDrawPushConstants push{};
//        push.worldMatrix = r.transform;
//        push.vertexBuffer = r.vertexBufferAddress;
//
//        vkCmdPushConstants(cmd, r.material->pipeline->layout, VK_SHADER_STAGE_VERTEX_BIT, 0,
//            sizeof(GPUDrawPushConstants), &push);
//
//        stats.drawcall_count++;
//        stats.triangle_count += r.indexCount / 3;
//        vkCmdDrawIndexed(cmd, r.indexCount, 1, r.firstIndex, 0, 0);
//        };
//
//    for (auto& idx : opaque_draws) draw(drawCommands.OpaqueSurfaces[idx]);
//    for (auto& r : drawCommands.TransparentSurfaces) draw(r);
//
//    drawCommands.OpaqueSurfaces.clear();
//    drawCommands.TransparentSurfaces.clear();
//}


//void VulkanEngine::draw_geometry(VkCommandBuffer cmd)
//{
//    // === GPU SceneData Uniform Buffer ===
//    AllocatedBuffer gpuSceneDataBuffer = create_buffer(
//        sizeof(GPUSceneData),
//        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
//        VMA_MEMORY_USAGE_CPU_TO_GPU
//    );
//
//    if (gpuSceneDataBuffer.buffer == VK_NULL_HANDLE) {
//        fmt::println("❌ gpuSceneDataBuffer.buffer null!");
//        return;
//    }
//
//    get_current_frame()._deletionQueue.push_function([=, this]() {
//        destroy_buffer(gpuSceneDataBuffer);
//        });
//
//    GPUSceneData* sceneUniformData = (GPUSceneData*)gpuSceneDataBuffer.allocation->GetMappedData();
//    *sceneUniformData = sceneData;
//
//    // === Descriptor Set ===
//    VkDescriptorSetVariableDescriptorCountAllocateInfo allocArrayInfo{
//        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO
//    };
//
//    uint32_t descriptorCounts = static_cast<uint32_t>(texCache.Cache.size());
//    allocArrayInfo.pDescriptorCounts = &descriptorCounts;
//    allocArrayInfo.descriptorSetCount = 1;
//
//    VkDescriptorSet globalDescriptor = get_current_frame()._frameDescriptors.allocate(
//        _device, _gpuSceneDataDescriptorLayout, &allocArrayInfo
//    );
//
//    DescriptorWriter writer;
//    writer.write_buffer(0, gpuSceneDataBuffer.buffer, sizeof(GPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
//
//    if (!texCache.Cache.empty()) {
//        VkWriteDescriptorSet arraySet{ .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
//        arraySet.descriptorCount = descriptorCounts;
//        arraySet.dstArrayElement = 0;
//        arraySet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
//        arraySet.dstBinding = 1;
//        arraySet.pImageInfo = texCache.Cache.data();
//        writer.writes.push_back(arraySet);
//    }
//
//    writer.update_set(_device, globalDescriptor);
//
//    // === Görünürlük Filtresi (Frustum Culling) ===
//    std::vector<uint32_t> opaque_draws;
//    opaque_draws.reserve(drawCommands.OpaqueSurfaces.size());
//
//    for (uint32_t i = 0; i < drawCommands.OpaqueSurfaces.size(); i++) {
//        if (is_visible(drawCommands.OpaqueSurfaces[i], sceneData.viewproj)) {
//            opaque_draws.push_back(i);
//        }
//    }
//
//    // === Viewport ve Scissor ===
//    VkViewport viewport = get_letterbox_viewport();
//    viewport.width = std::max(1.0f, viewport.width);
//    viewport.height = std::max(1.0f, viewport.height);
//
//    VkRect2D scissor = { {0, 0}, _windowExtent };
//
//    // === ViewMode'e Göre Mesh Çizimi ===
//    switch (_currentViewMode)
//    {
//    case ViewMode::Wireframe:
//        draw_wireframe(cmd, globalDescriptor, viewport, scissor, opaque_draws);
//        break;
//
//    case ViewMode::MaterialPreview:
//        draw_material_preview(cmd, globalDescriptor, viewport, scissor, opaque_draws);
//        break;
//
//    case ViewMode::Rendered: // PBR Forward Rendering
//        draw_rendered(cmd, globalDescriptor, viewport, scissor, opaque_draws);
//        break;
//
//    case ViewMode::Shaded:
//    default:
//        draw_rendered(cmd, globalDescriptor, viewport, scissor, opaque_draws); // veya draw_shaded()
//        break;
//    }
//
//    // Temizlik
//    drawCommands.OpaqueSurfaces.clear();
//    drawCommands.TransparentSurfaces.clear();
//}

void VulkanEngine::init_outline_wireframe_pipeline()
{
    VkShaderModule vertShader;
    VkShaderModule fragShader;

    // Use outline.vert instead of mesh.vert - outline.vert doesn't require materialData
    vkutil::load_shader_module("../../shaders/outline.vert.spv", _device, &vertShader);
    vkutil::load_shader_module("../../shaders/outline.frag.spv", _device, &fragShader);

    VkPushConstantRange push_constant{};
    push_constant.offset = 0;
    push_constant.size = sizeof(GPUDrawPushConstants);
    push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    // Only use scene data layout - outline shader doesn't need material data
    VkDescriptorSetLayout layouts[] = {
        _gpuSceneDataDescriptorLayout
    };

    VkPipelineLayoutCreateInfo pipeline_layout_info{};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 1;  // Only scene data layout
    pipeline_layout_info.pSetLayouts = layouts;
    pipeline_layout_info.pushConstantRangeCount = 1;
    pipeline_layout_info.pPushConstantRanges = &push_constant;

    VK_CHECK(vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr, &_wireframeOutlinePipelineLayout));

    PipelineBuilder builder;
    builder._pipelineLayout = _wireframeOutlinePipelineLayout;
    builder.set_shaders(vertShader, fragShader);
    builder.set_vertex_input(Vertex::get_vertex_description()); // Add vertex input for position
    builder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    builder.set_polygon_mode(VK_POLYGON_MODE_LINE);  // Wireframe çizim
    builder.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
    builder.set_multisampling_none();
    builder.disable_blending();
    builder.enable_depthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);
    builder.set_color_attachment_format(_drawImage.imageFormat);
    builder.set_depth_format(_depthImage.imageFormat);

    builder._renderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    builder._renderInfo.colorAttachmentCount = 1;
    builder._renderInfo.pColorAttachmentFormats = &builder._colorAttachmentformat;
    builder._renderInfo.depthAttachmentFormat = _depthImage.imageFormat;

    _wireframeOutlinePipeline = builder.build_pipeline(_device);

    vkDestroyShaderModule(_device, vertShader, nullptr);
    vkDestroyShaderModule(_device, fragShader, nullptr);
}





void VulkanEngine::draw_wireframe_outline(VkCommandBuffer cmd, const RenderObject& obj, VkDescriptorSet descriptor, VkViewport viewport, VkRect2D scissor)
{
    if (obj.indexBuffer == VK_NULL_HANDLE || obj.vertexBuffer == VK_NULL_HANDLE)
        return;  //  Mesh yoksa çizim yapma

    // Validate outline pipeline before use
    if (_wireframeOutlinePipeline == VK_NULL_HANDLE) {
        return;
    }

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _wireframeOutlinePipeline);
    vkCmdSetViewport(cmd, 0, 1, &viewport);
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    GPUDrawPushConstants push{};
    push.worldMatrix = obj.transform;
    push.vertexBuffer = obj.vertexBufferAddress;
    push.outlineScale = 0.02f;
    push.padding[0] = 0.0f;
    push.padding[1] = 0.0f;
    push.padding[2] = 0.0f;

    vkCmdPushConstants(cmd, _wireframeOutlinePipelineLayout,
        VK_SHADER_STAGE_VERTEX_BIT, 0,
        sizeof(GPUDrawPushConstants), &push);

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
        _wireframeOutlinePipelineLayout, 0, 1, &descriptor, 0, nullptr);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &obj.vertexBuffer, &offset);
    vkCmdBindIndexBuffer(cmd, obj.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(cmd, obj.indexCount, 1, obj.firstIndex, 0, 0);
}





//void VulkanEngine::draw_geometry(VkCommandBuffer cmd)
//{
//    // === GPU SceneData ===
//    AllocatedBuffer gpuSceneDataBuffer = create_buffer(sizeof(GPUSceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
//    if (gpuSceneDataBuffer.buffer == VK_NULL_HANDLE) {
//        fmt::println(" gpuSceneDataBuffer null!");
//        return;
//    }
//
//    get_current_frame()._deletionQueue.push_function([=, this]() {
//        destroy_buffer(gpuSceneDataBuffer);
//        });
//
//    GPUSceneData* sceneUniformData = (GPUSceneData*)gpuSceneDataBuffer.allocation->GetMappedData();
//    *sceneUniformData = sceneData;
//
//    // === Descriptor Set ===
//    VkDescriptorSetVariableDescriptorCountAllocateInfo allocArrayInfo{
//        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO
//    };
//
//    uint32_t descriptorCounts = static_cast<uint32_t>(texCache.Cache.size());
//    allocArrayInfo.pDescriptorCounts = &descriptorCounts;
//    allocArrayInfo.descriptorSetCount = 1;
//
//    VkDescriptorSet globalDescriptor = get_current_frame()._frameDescriptors.allocate(_device, _gpuSceneDataDescriptorLayout, &allocArrayInfo);
//
//    DescriptorWriter writer;
//    writer.write_buffer(0, gpuSceneDataBuffer.buffer, sizeof(GPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
//
//    if (!texCache.Cache.empty()) {
//        VkWriteDescriptorSet arraySet{ .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
//        arraySet.descriptorCount = descriptorCounts;
//        arraySet.dstArrayElement = 0;
//        arraySet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
//        arraySet.dstBinding = 1;
//        arraySet.pImageInfo = texCache.Cache.data();
//        writer.writes.push_back(arraySet);
//    }
//
//    writer.update_set(_device, globalDescriptor);
//
//    // === Görünürlük Filtresi ===
//    std::vector<uint32_t> opaque_draws;
//    opaque_draws.reserve(drawCommands.OpaqueSurfaces.size());
//
//    stats.visibleObjects.clear();  //  Frame başında temizle
//
//
//    for (uint32_t i = 0; i < drawCommands.OpaqueSurfaces.size(); i++) {
//        if (is_visible(drawCommands.OpaqueSurfaces[i], sceneData.viewproj)) {
//            opaque_draws.push_back(i);
//
//            // ✅ RenderObject'teki adı kullan
//            const auto& surface = drawCommands.OpaqueSurfaces[i];
//            if (!surface.name.empty()) {
//                stats.visibleObjects.push_back(surface.name);
//            }
//            else {
//                stats.visibleObjects.push_back("Unnamed Object");
//            }
//        }
//    }
//    //// === Seçili Obje için Outline ===
//    //for (uint32_t drawID : opaque_draws)
//    //{
//    //    const RenderObject& obj = drawCommands.OpaqueSurfaces[drawID];
//
//    //    if (selectedNode && obj.nodePointer == selectedNode)
//    //    {
//    //        draw_wireframe_outline(cmd, obj, globalDescriptor);
//    //    }
//    //}
//
//    // === Seçili Obje için Outline ===
//    if (_showOutline)   //  Kullanıcı Outline tikini açtıysa
//    {
//        for (uint32_t drawID : opaque_draws)
//        {
//            const RenderObject& obj = drawCommands.OpaqueSurfaces[drawID];
//
//            if (selectedNode && obj.nodePointer == selectedNode)
//            {
//                draw_wireframe_outline(cmd, obj, globalDescriptor);
//            }
//        }
//    }
//
//
//
//
//    stats.drawcall_count = 0;
//    stats.triangle_count = 0;
//
//    VkViewport viewport = get_letterbox_viewport();
//    viewport.width = std::max(1.0f, viewport.width);
//    viewport.height = std::max(1.0f, viewport.height);
//
//    VkRect2D scissor = { {0, 0}, _windowExtent };
//
//    // === ViewMode Ayrımı ===
//    switch (_currentViewMode)
//    {
//    case ViewMode::Wireframe:
//        draw_wireframe(cmd, globalDescriptor, viewport, scissor, opaque_draws);
//        break;
//
//    case ViewMode::MaterialPreview:
//        draw_material_preview(cmd, globalDescriptor, viewport, scissor, opaque_draws);
//        break;
//
//    case ViewMode::Rendered:
//        draw_rendered(cmd, globalDescriptor, viewport, scissor, opaque_draws);
//        break;
//
//    case ViewMode::Shaded:
//    default:
//        draw_shaded(cmd, globalDescriptor, viewport, scissor, opaque_draws);
//        break;
//    }
//
//    drawCommands.OpaqueSurfaces.clear();
//    drawCommands.TransparentSurfaces.clear();
//}


void VulkanEngine::draw_geometry(VkCommandBuffer cmd)
{
    // === GPU SceneData ===
    AllocatedBuffer gpuSceneDataBuffer = create_buffer(sizeof(GPUSceneData),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

    if (gpuSceneDataBuffer.buffer == VK_NULL_HANDLE) {
        fmt::print("gpuSceneDataBuffer null!\n");
        return;
    }

    get_current_frame()._deletionQueue.push_function([=, this]() {
        destroy_buffer(gpuSceneDataBuffer);
        });

    GPUSceneData* sceneUniformData = (GPUSceneData*)gpuSceneDataBuffer.allocation->GetMappedData();
    *sceneUniformData = sceneData;

    // === Descriptor Set ===
    VkDescriptorSetVariableDescriptorCountAllocateInfo allocArrayInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO
    };

    uint32_t descriptorCounts = static_cast<uint32_t>(texCache.Cache.size());
    allocArrayInfo.pDescriptorCounts = &descriptorCounts;
    allocArrayInfo.descriptorSetCount = 1;

    // Use the member variable so draw_viewing can access it
    globalDescriptor = get_current_frame()._frameDescriptors.allocate(
        _device, _gpuSceneDataDescriptorLayout, &allocArrayInfo);

    DescriptorWriter writer;
    writer.write_buffer(0, gpuSceneDataBuffer.buffer, sizeof(GPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

    if (!texCache.Cache.empty()) {
        VkWriteDescriptorSet arraySet{ .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        arraySet.descriptorCount = descriptorCounts;
        arraySet.dstArrayElement = 0;
        arraySet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        arraySet.dstBinding = 1;
        arraySet.pImageInfo = texCache.Cache.data();
        writer.writes.push_back(arraySet);
    }

    writer.update_set(_device, globalDescriptor);

    // === Görünürlük ve Raycast Hazırlığı ===
    std::vector<uint32_t> opaque_draws;
    opaque_draws.reserve(drawCommands.OpaqueSurfaces.size());

    stats.visibleObjects.clear();

    for (uint32_t i = 0; i < drawCommands.OpaqueSurfaces.size(); i++) {
        if (is_visible(drawCommands.OpaqueSurfaces[i], sceneData.viewproj)) {
            opaque_draws.push_back(i);

            const auto& surface = drawCommands.OpaqueSurfaces[i];
            if (!surface.name.empty()) {
                stats.visibleObjects.push_back(surface.name);
            }
            else {
                stats.visibleObjects.push_back("Unnamed Object");
            }
        }
        pickableRenderObjects = drawCommands.OpaqueSurfaces;

    }

    /*fmt::println("[RenderQueue] {} obje render için aktif. Ray Picking kullanılabilir.", opaque_draws.size());*/

    stats.drawcall_count = 0;
    stats.triangle_count = 0;

    // Calculate viewport and scissor BEFORE any drawing operations
    VkViewport viewport = get_letterbox_viewport();
    viewport.width = std::max(1.0f, viewport.width);
    viewport.height = std::max(1.0f, viewport.height);

    VkRect2D scissor = { {0, 0}, _windowExtent };

    // === Seçili Obje için Outline ===
    if (_showOutline)
    {
        for (uint32_t drawID : opaque_draws)
        {
            const RenderObject& obj = drawCommands.OpaqueSurfaces[drawID];

            if (selectedNode && obj.nodePointer == selectedNode)
            {
                /*fmt::println("Seçili obje çiziliyor (outline): {}", obj.name);*/
                draw_wireframe_outline(cmd, obj, globalDescriptor, viewport, scissor);
            }
        }
    }

    // === ViewMode İşleme ===
    switch (_currentViewMode)
    {
    case ViewMode::Solid:
        draw_solid(cmd, globalDescriptor, viewport, scissor, opaque_draws);
        break;

    case ViewMode::Shaded:
        // Use simple hemisphere lighting shader (new shaded pipeline)
        if (_shadedPipeline != VK_NULL_HANDLE) {
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _shadedPipeline);
            vkCmdSetViewport(cmd, 0, 1, &viewport);
            vkCmdSetScissor(cmd, 0, 1, &scissor);
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _shadedPipelineLayout,
                0, 1, &globalDescriptor, 0, nullptr);

            VkBuffer lastIndexBuffer = VK_NULL_HANDLE;
            for (auto idx : opaque_draws) {
                const RenderObject& r = drawCommands.OpaqueSurfaces[idx];
                if (r.indexBuffer != lastIndexBuffer) {
                    lastIndexBuffer = r.indexBuffer;
                    vkCmdBindIndexBuffer(cmd, r.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
                }
                GPUDrawPushConstants push{};
                push.worldMatrix = r.transform;
                push.vertexBuffer = r.vertexBufferAddress;
                push.baseColor = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);
                vkCmdPushConstants(cmd, _shadedPipelineLayout,
                    VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                    0, sizeof(GPUDrawPushConstants), &push);
                stats.drawcall_count++;
                stats.triangle_count += r.indexCount / 3;
                vkCmdDrawIndexed(cmd, r.indexCount, 1, r.firstIndex, 0, 0);
            }
        }
        break;

    case ViewMode::MaterialPreview:
        draw_material_preview(cmd, globalDescriptor, viewport, scissor, opaque_draws);
        break;

    case ViewMode::Rendered:
        draw_rendered(cmd, globalDescriptor, viewport, scissor, opaque_draws);
        break;

    case ViewMode::Wireframe:
        draw_wireframe(cmd, globalDescriptor, viewport, scissor, opaque_draws);
        break;

    case ViewMode::Normals:
        draw_normals(cmd, globalDescriptor, viewport, scissor, opaque_draws);
        break;

    case ViewMode::UVChecker:
        draw_uvchecker(cmd, globalDescriptor, viewport, scissor, opaque_draws);
        break;

    case ViewMode::PathTraced:
        // Path tracing uses compute shader, handled separately in draw_main
        // Fall back to rendered for geometry pass
        draw_rendered(cmd, globalDescriptor, viewport, scissor, opaque_draws);
        break;

    default:
        draw_rendered(cmd, globalDescriptor, viewport, scissor, opaque_draws);
        break;
    }

    // Draw primitives with the new primitive pipeline (face colors + lighting)
    draw_primitives_with_viewport(cmd, globalDescriptor, viewport, scissor);

    drawCommands.OpaqueSurfaces.clear();
    drawCommands.TransparentSurfaces.clear();
}













//void VulkanEngine::init_pathtrace_pipeline()
//{
//    VkShaderModule computeShader;
//    vkutil::load_shader_module("../../shaders/pathtrace.comp.spv", _device, &computeShader);
//
//    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
//    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
//    pipelineLayoutInfo.setLayoutCount = 1;
//    pipelineLayoutInfo.pSetLayouts = &_drawImageDescriptorLayout;
//
//    VK_CHECK(vkCreatePipelineLayout(_device, &pipelineLayoutInfo, nullptr, &_pathTracePipelineLayout));
//
//    VkComputePipelineCreateInfo pipelineInfo{};
//    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
//    pipelineInfo.stage = vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_COMPUTE_BIT, computeShader);
//    pipelineInfo.layout = _pathTracePipelineLayout;
//
//    VK_CHECK(vkCreateComputePipelines(_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_pathTracePipeline));
//
//    vkDestroyShaderModule(_device, computeShader, nullptr);
//}


//void VulkanEngine::draw_rendered_pathtraced(VkCommandBuffer cmd)
//{
//    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _pathTracePipeline);
//
//    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _pathTracePipelineLayout,
//        0, 1, &_pathTraceDescriptorSet, 0, nullptr);
//
//    uint32_t groupCountX = (_windowExtent.width + 7) / 8;
//    uint32_t groupCountY = (_windowExtent.height + 7) / 8;
//
//    vkCmdDispatch(cmd, groupCountX, groupCountY, 1);
//}


//void VulkanEngine::init_pathtrace_present_pipeline()
//{
//    VkShaderModule vertShader;
//    VkShaderModule fragShader;
//
//    vkutil::load_shader_module("../../shaders/fullscreen.vert.spv", _device, &vertShader);
//    vkutil::load_shader_module("../../shaders/fullscreen.frag.spv", _device, &fragShader);
//
//    VkPushConstantRange pushConstant{};
//    pushConstant.offset = 0;
//    pushConstant.size = sizeof(float);  // opsiyonel
//    pushConstant.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
//
//    VkDescriptorSetLayout setLayouts[] = { _drawImageDescriptorLayout };
//
//    VkPipelineLayoutCreateInfo pipelineLayoutInfo = vkinit::pipeline_layout_create_info();
//    pipelineLayoutInfo.setLayoutCount = 1;
//    pipelineLayoutInfo.pSetLayouts = setLayouts;
//    pipelineLayoutInfo.pushConstantRangeCount = 1;
//    pipelineLayoutInfo.pPushConstantRanges = &pushConstant;
//
//    VK_CHECK(vkCreatePipelineLayout(_device, &pipelineLayoutInfo, nullptr, &_pathTracePresentPipelineLayout));
//
//    PipelineBuilder builder;
//    builder._pipelineLayout = _pathTracePresentPipelineLayout;
//    builder.set_shaders(vertShader, fragShader);
//    builder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
//    builder.set_polygon_mode(VK_POLYGON_MODE_FILL);
//    builder.disable_blending();
//    builder.enable_depthtest(false, VK_COMPARE_OP_ALWAYS);
//    builder.set_color_attachment_format(_drawImage.imageFormat);
//
//    builder._renderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
//    builder._renderInfo.colorAttachmentCount = 1;
//    builder._renderInfo.pColorAttachmentFormats = &builder._colorAttachmentformat;
//
//    _pathTracePresentPipeline = builder.build_pipeline(_device);
//
//    vkDestroyShaderModule(_device, vertShader, nullptr);
//    vkDestroyShaderModule(_device, fragShader, nullptr);
//}

void VulkanEngine::draw_background_effect(VkCommandBuffer cmd)
{
    ComputeEffect& selected = backgroundEffects[currentBackgroundEffect];

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, selected.pipeline);

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, selected.layout,
        0, 1, &_drawImageDescriptorSet, 0, nullptr);

    vkCmdPushConstants(cmd, selected.layout, VK_SHADER_STAGE_COMPUTE_BIT,
        0, sizeof(ComputePushConstants), &selected.data);

    uint32_t groupCountX = (_windowExtent.width + 7) / 8;
    uint32_t groupCountY = (_windowExtent.height + 7) / 8;

    vkCmdDispatch(cmd, groupCountX, groupCountY, 1);
}
void VulkanEngine::allocate_draw_image_descriptor_set()
{
    // Descriptor set allocate
    _drawImageDescriptorSet = get_current_frame()._frameDescriptors.allocate(_device, _drawImageDescriptorLayout);

    // Storage image bilgisi
    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageView = _drawImage.imageView;
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

    // Descriptor update
    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = _drawImageDescriptorSet;
    write.dstBinding = 0;
    write.dstArrayElement = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    write.descriptorCount = 1;
    write.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(_device, 1, &write, 0, nullptr);
}


//void VulkanEngine::draw_present_pathtraced(VkCommandBuffer cmd)
//{
//    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _pathTracePresentPipeline);
//
//    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _pathTracePresentPipelineLayout,
//        0, 1, &_pathTraceDescriptorSet, 0, nullptr);
//
//    float exposure = 1.0f;  // örnek push constant (opsiyonel)
//    vkCmdPushConstants(cmd, _pathTracePresentPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(float), &exposure);
//
//    vkCmdDraw(cmd, 3, 1, 0, 0);  // Fullscreen triangle çiz
//}
//






void VulkanEngine::draw_wireframe(VkCommandBuffer cmd, VkDescriptorSet globalDescriptor, VkViewport viewport, VkRect2D scissor, const std::vector<uint32_t>& opaque_draws)
{
    // Validate wireframe pipeline before use - may fail on some MoltenVK configurations
    if (_wireframePipeline == VK_NULL_HANDLE) {
        static bool warningPrinted = false;
        if (!warningPrinted) {
            fmt::print("Warning: Wireframe pipeline not available, falling back to shaded mode\n");
            warningPrinted = true;
        }
        draw_shaded(cmd, globalDescriptor, viewport, scissor, opaque_draws);
        return;
    }

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _wireframePipeline);
    vkCmdSetViewport(cmd, 0, 1, &viewport);
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    for (auto& idx : opaque_draws) {
        const RenderObject& r = drawCommands.OpaqueSurfaces[idx];
        if (!r.material || r.material->materialSet == VK_NULL_HANDLE)
            continue;

        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _wireframePipelineLayout,
            0, 1, &globalDescriptor, 0, nullptr);

        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _wireframePipelineLayout,
            1, 1, &r.material->materialSet, 0, nullptr);

        vkCmdBindIndexBuffer(cmd, r.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        GPUDrawPushConstants push{};
        push.worldMatrix = r.transform;
        push.vertexBuffer = r.vertexBufferAddress;

        vkCmdPushConstants(cmd, _wireframePipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0,
            sizeof(GPUDrawPushConstants), &push);

        vkCmdDrawIndexed(cmd, r.indexCount, 1, r.firstIndex, 0, 0);
    }
}



void VulkanEngine::draw_shaded(
    VkCommandBuffer cmd,
    VkDescriptorSet globalDescriptor,
    VkViewport viewport,
    VkRect2D scissor,
    const std::vector<uint32_t>& opaque_draws)
{
    MaterialPipeline* lastPipeline = nullptr;
    MaterialInstance* lastMaterial = nullptr;
    VkBuffer lastIndexBuffer = VK_NULL_HANDLE;

    auto draw = [&](const RenderObject& r) {
        if (!r.material || !r.material->pipeline)
            return;

        MaterialPipeline* pipeline = r.material->pipeline;

        if (r.material != lastMaterial) {
            lastMaterial = r.material;

            if (pipeline != lastPipeline) {
                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->layout,
                    0, 1, &globalDescriptor, 0, nullptr);

                vkCmdSetViewport(cmd, 0, 1, &viewport);
                vkCmdSetScissor(cmd, 0, 1, &scissor);

                lastPipeline = pipeline;
            }

            if (r.material->materialSet != VK_NULL_HANDLE) {
                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->layout,
                    1, 1, &r.material->materialSet, 0, nullptr);
            }
        }

        if (r.indexBuffer != lastIndexBuffer) {
            lastIndexBuffer = r.indexBuffer;
            vkCmdBindIndexBuffer(cmd, r.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        }

        GPUDrawPushConstants push{};
        push.worldMatrix = r.transform;
        push.vertexBuffer = r.vertexBufferAddress;

        vkCmdPushConstants(cmd, pipeline->layout, VK_SHADER_STAGE_VERTEX_BIT, 0,
            sizeof(GPUDrawPushConstants), &push);

        stats.drawcall_count++;
        stats.triangle_count += r.indexCount / 3;

        vkCmdDrawIndexed(cmd, r.indexCount, 1, r.firstIndex, 0, 0);
        };

    for (auto idx : opaque_draws)
        draw(drawCommands.OpaqueSurfaces[idx]);

    for (auto& r : drawCommands.TransparentSurfaces)
        draw(r);
}

// =============================================================================
// DRAW SOLID - Flat color, no lighting (fastest)
// =============================================================================
void VulkanEngine::draw_solid(
    VkCommandBuffer cmd,
    VkDescriptorSet globalDescriptor,
    VkViewport viewport,
    VkRect2D scissor,
    const std::vector<uint32_t>& opaque_draws)
{
    if (_solidPipeline == VK_NULL_HANDLE) return;

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _solidPipeline);
    vkCmdSetViewport(cmd, 0, 1, &viewport);
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    // Bind only Set 0 (scene data)
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _solidPipelineLayout,
        0, 1, &globalDescriptor, 0, nullptr);

    VkBuffer lastIndexBuffer = VK_NULL_HANDLE;

    for (auto idx : opaque_draws) {
        const RenderObject& r = drawCommands.OpaqueSurfaces[idx];

        if (r.indexBuffer != lastIndexBuffer) {
            lastIndexBuffer = r.indexBuffer;
            vkCmdBindIndexBuffer(cmd, r.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        }

        GPUDrawPushConstants push{};
        push.worldMatrix = r.transform;
        push.vertexBuffer = r.vertexBufferAddress;
        push.baseColor = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);  // Default gray

        vkCmdPushConstants(cmd, _solidPipelineLayout,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            0, sizeof(GPUDrawPushConstants), &push);

        stats.drawcall_count++;
        stats.triangle_count += r.indexCount / 3;

        vkCmdDrawIndexed(cmd, r.indexCount, 1, r.firstIndex, 0, 0);
    }
}

// =============================================================================
// DRAW NORMALS - Visualize world-space normals as RGB
// =============================================================================
void VulkanEngine::draw_normals(
    VkCommandBuffer cmd,
    VkDescriptorSet globalDescriptor,
    VkViewport viewport,
    VkRect2D scissor,
    const std::vector<uint32_t>& opaque_draws)
{
    if (_normalsPipeline == VK_NULL_HANDLE) return;

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _normalsPipeline);
    vkCmdSetViewport(cmd, 0, 1, &viewport);
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    // Bind only Set 0 (scene data)
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _normalsPipelineLayout,
        0, 1, &globalDescriptor, 0, nullptr);

    VkBuffer lastIndexBuffer = VK_NULL_HANDLE;

    for (auto idx : opaque_draws) {
        const RenderObject& r = drawCommands.OpaqueSurfaces[idx];

        if (r.indexBuffer != lastIndexBuffer) {
            lastIndexBuffer = r.indexBuffer;
            vkCmdBindIndexBuffer(cmd, r.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        }

        GPUDrawPushConstants push{};
        push.worldMatrix = r.transform;
        push.vertexBuffer = r.vertexBufferAddress;

        vkCmdPushConstants(cmd, _normalsPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
            0, sizeof(GPUDrawPushConstants), &push);

        stats.drawcall_count++;
        stats.triangle_count += r.indexCount / 3;

        vkCmdDrawIndexed(cmd, r.indexCount, 1, r.firstIndex, 0, 0);
    }
}

// =============================================================================
// DRAW UV CHECKER - Procedural checker pattern for UV debugging
// =============================================================================
void VulkanEngine::draw_uvchecker(
    VkCommandBuffer cmd,
    VkDescriptorSet globalDescriptor,
    VkViewport viewport,
    VkRect2D scissor,
    const std::vector<uint32_t>& opaque_draws)
{
    if (_uvCheckerPipeline == VK_NULL_HANDLE) return;

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _uvCheckerPipeline);
    vkCmdSetViewport(cmd, 0, 1, &viewport);
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    // Bind only Set 0 (scene data)
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _uvCheckerPipelineLayout,
        0, 1, &globalDescriptor, 0, nullptr);

    VkBuffer lastIndexBuffer = VK_NULL_HANDLE;

    for (auto idx : opaque_draws) {
        const RenderObject& r = drawCommands.OpaqueSurfaces[idx];

        if (r.indexBuffer != lastIndexBuffer) {
            lastIndexBuffer = r.indexBuffer;
            vkCmdBindIndexBuffer(cmd, r.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        }

        GPUDrawPushConstants push{};
        push.worldMatrix = r.transform;
        push.vertexBuffer = r.vertexBufferAddress;

        vkCmdPushConstants(cmd, _uvCheckerPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
            0, sizeof(GPUDrawPushConstants), &push);

        stats.drawcall_count++;
        stats.triangle_count += r.indexCount / 3;

        vkCmdDrawIndexed(cmd, r.indexCount, 1, r.firstIndex, 0, 0);
    }
}







void VulkanEngine::draw_material_preview(VkCommandBuffer cmd, VkDescriptorSet globalDescriptor, VkViewport viewport, VkRect2D scissor, const std::vector<uint32_t>& opaque_draws)
{
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _materialPreviewPipeline);
    vkCmdSetViewport(cmd, 0, 1, &viewport);
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    for (auto& idx : opaque_draws) {
        const RenderObject& r = drawCommands.OpaqueSurfaces[idx];
        if (!r.material || r.material->materialSet == VK_NULL_HANDLE) continue;

        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _materialPreviewPipelineLayout,
            0, 1, &globalDescriptor, 0, nullptr);

        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _materialPreviewPipelineLayout,
            1, 1, &r.material->materialSet, 0, nullptr);

        vkCmdBindIndexBuffer(cmd, r.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        GPUDrawPushConstants push{};
        push.worldMatrix = r.transform;
        push.vertexBuffer = r.vertexBufferAddress;

        vkCmdPushConstants(cmd, _materialPreviewPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0,
            sizeof(GPUDrawPushConstants), &push);

        stats.drawcall_count++;
        stats.triangle_count += r.indexCount / 3;

        vkCmdDrawIndexed(cmd, r.indexCount, 1, r.firstIndex, 0, 0);
    }
}

void VulkanEngine::draw_rendered(
    VkCommandBuffer cmd,
    VkDescriptorSet globalDescriptor,
    VkViewport viewport,
    VkRect2D scissor,
    const std::vector<uint32_t>& opaque_draws)
{
    MaterialPipeline* lastPipeline = nullptr;
    MaterialInstance* lastMaterial = nullptr;
    VkBuffer lastIndexBuffer = VK_NULL_HANDLE;

    auto draw = [&](const RenderObject& r) {
        if (!r.material || !r.material->pipeline)
            return;

        MaterialPipeline* pipeline = r.material->pipeline;

        if (r.material != lastMaterial) {
            lastMaterial = r.material;

            if (pipeline != lastPipeline) {
                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->layout,
                    0, 1, &globalDescriptor, 0, nullptr);

                vkCmdSetViewport(cmd, 0, 1, &viewport);
                vkCmdSetScissor(cmd, 0, 1, &scissor);

                lastPipeline = pipeline;
            }

            if (r.material->materialSet != VK_NULL_HANDLE) {
                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->layout,
                    1, 1, &r.material->materialSet, 0, nullptr);
            }
        }

        if (r.indexBuffer != lastIndexBuffer) {
            lastIndexBuffer = r.indexBuffer;
            vkCmdBindIndexBuffer(cmd, r.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        }

        GPUDrawPushConstants push{};
        push.worldMatrix = r.transform;
        push.vertexBuffer = r.vertexBufferAddress;

        vkCmdPushConstants(cmd, pipeline->layout, VK_SHADER_STAGE_VERTEX_BIT, 0,
            sizeof(GPUDrawPushConstants), &push);

        stats.drawcall_count++;
        stats.triangle_count += r.indexCount / 3;

        vkCmdDrawIndexed(cmd, r.indexCount, 1, r.firstIndex, 0, 0);
        };

    // Sadece opaque yüzeyler draw ediliyor (transparent support yoksa silebilirsin)
    for (auto idx : opaque_draws)
        draw(drawCommands.OpaqueSurfaces[idx]);

    for (auto& r : drawCommands.TransparentSurfaces)
        draw(r);
}














//
//void VulkanEngine::draw_pipeline_settings_imgui() {
//    if (ImGui::Begin("Pipeline Ayarları")) {
//        static bool toggle = enableBackfaceCulling;
//        if (ImGui::Checkbox("Backface Culling", &toggle)) {
//            enableBackfaceCulling = toggle;
//
//            // pipeline yeniden oluştur
//            if (_2dPipeline != VK_NULL_HANDLE)
//                vkDestroyPipeline(_device, _2dPipeline, nullptr);
//            if (_2dPipelineLayout != VK_NULL_HANDLE)
//                vkDestroyPipelineLayout(_device, _2dPipelineLayout, nullptr);
//
//            init_2d_pipeline(enableBackfaceCulling);
//        }
//
//        ImGui::Text("Şu anki Culling: %s", enableBackfaceCulling ? "Açık" : "Kapalı");
//    }
//    ImGui::End();
//}

//void VulkanEngine::draw_viewing(VkCommandBuffer cmd)
//{
//    switch (currentViewMode)
//    {
//    case ViewMode::ViewportShaded:
//        // Solid görünüm (shader-only ve matcap gibi)
//        draw_geometry(cmd); // Solid'de genelde simple shading yapılır, senin draw_geometry() zaten her şeyi kapsıyor.
//        break;
//
//    case ViewMode::MaterialPreview:
//        // Materyal preview: belki light'lar basitleştirilir
//        draw_geometry(cmd); // şimdilik aynı çağrılıyor, ileride farklı shader seti kullanılabilir
//        break;
//
//    case ViewMode::Rendered:
//        // Full PBR, ışıklar, HDR vs.
//        draw_geometry(cmd); // burada da tam çözüm, render pipeline'ı zaten material üzerinden seçiliyor
//        break;
//
//    default:
//        // Unknown mode, fallback
//        draw_geometry(cmd);
//        break;
//    }
//}
//void VulkanEngine::draw_viewing(VkCommandBuffer cmd)
//{
//    // Kamera güncellemeleri veya overlay varsa ekle, yoksa sadece sahneyi çiz
//    draw_geometry(cmd);
//
//    // İsteğe bağlı: debug çizimler (mesela sadece Shaded modda grid vs.)
//    if (_currentViewMode == ViewMode::Shaded) {
//        draw_grid(cmd); // eğer gridPipeline kullanıyorsan
//    }
//
//    // İsteğe bağlı: wireframe, helper çizimler
//}

//void VulkanEngine::draw_viewing(VkCommandBuffer cmd)
//{
//    switch (_currentViewMode) {
//    case ViewMode::Shaded:
//        draw_geometry(cmd);
//        draw_grid(cmd);  // Sadece grid veya shader-only objeler
//        break;
//
//    case ViewMode::Rendered:
//        draw_geometry(cmd);  // GLTF/GLB PBR + materyal destekli sahne
//        break;
//
//    case ViewMode::PathTraced:
//        draw_rendered_pathtraced(cmd);
//        return;
//
//    default:
//        draw_geometry(cmd);  // Varsayılan fallback
//        break;
//    }
//}
// draw_viewing() → sahne render yapısını kontrol eder
//void VulkanEngine::draw_viewing(VkCommandBuffer cmd)
//{
//    switch (_currentViewMode)
//    {
//    case ViewMode::Shaded:
//        // Sadece shader-only objeler (örn: static_shapes)
//        draw_grid(cmd, globalDescriptor);
//        draw_geometry(cmd);                    // Statik objeler + 3D sahne
//             // World-space grid plane
//        break;
//
//    case ViewMode::Rendered:
//        // GLTF/GLB materyalli sahne (gerçek PBR forward rendering)
//        draw_geometry(cmd);                    // Materyalli 3D objeler
//        break;
//
//    case ViewMode::PathTraced:
//        // Fullscreen path tracer sonucu çizilir
//        draw_rendered_pathtraced(cmd);
//        return;  // Diğer şeyleri çizme
//
//    case ViewMode::Wireframe:
//    case ViewMode::MaterialPreview:
//    default:
//        draw_geometry(cmd);                    // Diğer modlarda da 3D sahne çizilir
//        break;
//    }
//}
void VulkanEngine::draw_viewing(VkCommandBuffer cmd)
{
    // First, draw geometry to initialize globalDescriptor
    // NOTE: PathTraced compute dispatch must happen OUTSIDE the render pass
    // (done in draw_main before vkCmdBeginRendering)
    // All view modes use draw_geometry which handles the mode-specific rendering
    draw_geometry(cmd);

    // Grid drawn after geometry so globalDescriptor is initialized
    if (_showGrid && globalDescriptor != VK_NULL_HANDLE)
    {
        draw_grid(cmd, globalDescriptor);
    }
}






//
//void VulkanEngine::draw_pipeline_settings_imgui() {
//    if (ImGui::Begin("Pipeline Ayarları")) {
//        // --- Backface Culling Kontrolü ---
//        static bool toggle = enableBackfaceCulling;
//        if (ImGui::Checkbox("Backface Culling", &toggle)) {
//            enableBackfaceCulling = toggle;
//
//            // pipeline yeniden oluştur
//            if (_2dPipeline != VK_NULL_HANDLE)
//                vkDestroyPipeline(_device, _2dPipeline, nullptr);
//            if (_2dPipelineLayout != VK_NULL_HANDLE)
//                vkDestroyPipelineLayout(_device, _2dPipelineLayout, nullptr);
//
//            init_2d_pipeline(enableBackfaceCulling);
//        }
//
//        ImGui::Text("Şu anki Culling: %s", enableBackfaceCulling ? "Açık" : "Kapalı");
//
//        // --- View Mode Seçimi ---
//        ImGui::Separator();
//        static const char* viewModes[] = { "Solid", "Material Preview", "Rendered" };
//        static int currentMode = static_cast<int>(currentViewMode);
//
//        if (ImGui::Combo("View Mode", &currentMode, viewModes, IM_ARRAYSIZE(viewModes))) {
//            currentViewMode = static_cast<ViewMode>(currentMode);
//        }
//
//        ImGui::Text("Aktif Mod: %s", viewModes[currentMode]);
//    }
//    ImGui::End();
//},
//void VulkanEngine::draw_pipeline_settings_imgui() {
//    if (ImGui::Begin("Pipeline Ayarları")) {
//
//        // --- Backface Culling ---
//        ImGui::Checkbox("Backface Culling", &enableBackfaceCulling);
//        ImGui::Text("Şu anki Culling: %s", enableBackfaceCulling ? "Açık" : "Kapalı");
//
//        ImGui::Separator();
//
//        // --- View Mode ---
//        static const char* viewModes[] = {
//            "Wireframe",
//            "Shaded",
//            "Material Preview",
//            "Rendered",
//            "Path Traced"   
//        };
//
//        int currentMode = static_cast<int>(_currentViewMode);
//
//        if (ImGui::Combo("View Mode", &currentMode, viewModes, IM_ARRAYSIZE(viewModes))) {
//            _currentViewMode = static_cast<ViewMode>(currentMode);
//        }
//
//        ImGui::Text("Aktif Mod: %s", viewModes[currentMode]);
//    }
//    ImGui::End();
//}

void VulkanEngine::draw_pipeline_settings_imgui() {
    // Professional styling
    ImGuiStyle& style = ImGui::GetStyle();

    if (ImGui::Begin("Viewport Settings")) {

        // === VIEW MODE ===
        if (ImGui::CollapsingHeader("View Mode", ImGuiTreeNodeFlags_DefaultOpen)) {
            static const char* viewModes[] = { "Solid", "Shaded", "Material Preview", "Rendered", "Wireframe", "Normals", "UV Checker", "Path Traced" };
            int currentMode = static_cast<int>(_currentViewMode);

            ImGui::SetNextItemWidth(-1);
            if (ImGui::Combo("##ViewMode", &currentMode, viewModes, IM_ARRAYSIZE(viewModes))) {
                _currentViewMode = static_cast<ViewMode>(currentMode);
            }

            ImGui::Spacing();
            ImGui::Checkbox("Backface Culling", &enableBackfaceCulling);
            ImGui::Checkbox("Show Outline", &_showOutline);
        }

        ImGui::Spacing();

        // =======================================================================
        // GRID SETTINGS - Professional UI
        // =======================================================================
        if (ImGui::CollapsingHeader("Grid", ImGuiTreeNodeFlags_DefaultOpen)) {

            // Main toggle with icon-like appearance
            ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
            ImGui::Checkbox("Enable Grid", &_showGrid);
            ImGui::PopStyleColor();

            if (!_showGrid) {
                ImGui::BeginDisabled();
            }

            ImGui::Spacing();
            ImGui::Separator();

            // === PRESETS ===
            ImGui::Text("Presets");
            static const char* presets[] = { "Default", "Blender", "Unity", "Unreal", "CAD", "Architectural" };

            ImGui::SetNextItemWidth(-1);
            if (ImGui::Combo("##Preset", &_gridSettings.currentPreset, presets, IM_ARRAYSIZE(presets))) {
                // Apply preset
                switch (_gridSettings.currentPreset) {
                    case 0: // Default
                        _gridSettings.baseGridSize = 1.0f;
                        _gridSettings.majorGridMultiplier = 10.0f;
                        _gridSettings.lineWidth = 1.5f;
                        _gridSettings.gridOpacity = 0.7f;
                        _gridSettings.dynamicLOD = true;
                        _gridSettings.minorLineColor = glm::vec3(0.25f);
                        _gridSettings.majorLineColor = glm::vec3(0.45f);
                        break;
                    case 1: // Blender
                        _gridSettings.baseGridSize = 1.0f;
                        _gridSettings.majorGridMultiplier = 10.0f;
                        _gridSettings.lineWidth = 1.0f;
                        _gridSettings.gridOpacity = 0.5f;
                        _gridSettings.dynamicLOD = true;
                        _gridSettings.minorLineColor = glm::vec3(0.2f, 0.2f, 0.2f);
                        _gridSettings.majorLineColor = glm::vec3(0.35f, 0.35f, 0.35f);
                        _gridSettings.xAxisColor = glm::vec3(0.929f, 0.227f, 0.298f);
                        _gridSettings.zAxisColor = glm::vec3(0.227f, 0.404f, 0.937f);
                        break;
                    case 2: // Unity
                        _gridSettings.baseGridSize = 1.0f;
                        _gridSettings.majorGridMultiplier = 10.0f;
                        _gridSettings.lineWidth = 1.2f;
                        _gridSettings.gridOpacity = 0.6f;
                        _gridSettings.dynamicLOD = true;
                        _gridSettings.minorLineColor = glm::vec3(0.3f, 0.3f, 0.3f);
                        _gridSettings.majorLineColor = glm::vec3(0.5f, 0.5f, 0.5f);
                        _gridSettings.xAxisColor = glm::vec3(0.858f, 0.243f, 0.113f);
                        _gridSettings.zAxisColor = glm::vec3(0.203f, 0.458f, 0.858f);
                        break;
                    case 3: // Unreal
                        _gridSettings.baseGridSize = 10.0f;
                        _gridSettings.majorGridMultiplier = 10.0f;
                        _gridSettings.lineWidth = 1.0f;
                        _gridSettings.gridOpacity = 0.4f;
                        _gridSettings.dynamicLOD = true;
                        _gridSettings.minorLineColor = glm::vec3(0.15f, 0.15f, 0.15f);
                        _gridSettings.majorLineColor = glm::vec3(0.3f, 0.3f, 0.3f);
                        _gridSettings.xAxisColor = glm::vec3(1.0f, 0.0f, 0.0f);
                        _gridSettings.zAxisColor = glm::vec3(0.0f, 0.0f, 1.0f);
                        break;
                    case 4: // CAD
                        _gridSettings.baseGridSize = 0.1f;
                        _gridSettings.majorGridMultiplier = 10.0f;
                        _gridSettings.lineWidth = 0.8f;
                        _gridSettings.gridOpacity = 0.8f;
                        _gridSettings.dynamicLOD = true;
                        _gridSettings.minorLineColor = glm::vec3(0.6f, 0.6f, 0.7f);
                        _gridSettings.majorLineColor = glm::vec3(0.4f, 0.4f, 0.5f);
                        break;
                    case 5: // Architectural
                        _gridSettings.baseGridSize = 1.0f;
                        _gridSettings.majorGridMultiplier = 5.0f;
                        _gridSettings.lineWidth = 1.0f;
                        _gridSettings.gridOpacity = 0.5f;
                        _gridSettings.dynamicLOD = true;
                        _gridSettings.minorLineColor = glm::vec3(0.7f, 0.7f, 0.72f);
                        _gridSettings.majorLineColor = glm::vec3(0.5f, 0.5f, 0.55f);
                        break;
                }
            }

            ImGui::Spacing();
            ImGui::Separator();

            // === CORE SETTINGS ===
            if (ImGui::TreeNodeEx("Grid Size & Scale", ImGuiTreeNodeFlags_DefaultOpen)) {

                ImGui::Text("Cell Size");
                ImGui::SetNextItemWidth(-1);
                ImGui::SliderFloat("##CellSize", &_gridSettings.baseGridSize, 0.01f, 100.0f, "%.2f units", ImGuiSliderFlags_Logarithmic);

                ImGui::Text("Major Line Every");
                ImGui::SetNextItemWidth(-1);
                ImGui::SliderFloat("##MajorMult", &_gridSettings.majorGridMultiplier, 2.0f, 20.0f, "%.0f cells");

                ImGui::Spacing();

                ImGui::Checkbox("Dynamic LOD (Auto-Scale)", &_gridSettings.dynamicLOD);
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Grid scales automatically based on camera distance\nLike Blender, Unity, Unreal Engine");
                }

                if (_gridSettings.dynamicLOD) {
                    ImGui::Text("LOD Bias");
                    ImGui::SetNextItemWidth(-1);
                    ImGui::SliderFloat("##LODBias", &_gridSettings.lodBias, -2.0f, 2.0f, "%.1f");
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("Negative = Finer grid, Positive = Coarser grid");
                    }
                }

                ImGui::TreePop();
            }

            ImGui::Spacing();

            // === APPEARANCE ===
            if (ImGui::TreeNodeEx("Appearance", ImGuiTreeNodeFlags_DefaultOpen)) {

                ImGui::Text("Opacity");
                ImGui::SetNextItemWidth(-1);
                ImGui::SliderFloat("##Opacity", &_gridSettings.gridOpacity, 0.0f, 1.0f, "%.2f");

                ImGui::Text("Line Width");
                ImGui::SetNextItemWidth(-1);
                ImGui::SliderFloat("##LineWidth", &_gridSettings.lineWidth, 0.1f, 5.0f, "%.1f px");

                ImGui::Text("Fade Distance");
                ImGui::SetNextItemWidth(-1);
                ImGui::SliderFloat("##FadeDist", &_gridSettings.fadeDistance, 10.0f, 10000.0f, "%.0f", ImGuiSliderFlags_Logarithmic);

                ImGui::Spacing();

                ImGui::Checkbox("Show Subdivisions", &_gridSettings.showSubdivisions);
                ImGui::Checkbox("Anti-Aliasing", &_gridSettings.antiAliasing);
                ImGui::Checkbox("Infinite Grid", &_gridSettings.infiniteGrid);
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Grid follows camera for infinite appearance");
                }

                ImGui::TreePop();
            }

            ImGui::Spacing();

            // === AXIS COLORS ===
            if (ImGui::TreeNodeEx("Axis Lines", ImGuiTreeNodeFlags_DefaultOpen)) {

                ImGui::Checkbox("Show Axis Colors", &_gridSettings.showAxisColors);

                if (_gridSettings.showAxisColors) {
                    ImGui::Text("Axis Line Width");
                    ImGui::SetNextItemWidth(-1);
                    ImGui::SliderFloat("##AxisWidth", &_gridSettings.axisLineWidth, 1.0f, 10.0f, "%.1f");

                    ImGui::Spacing();

                    ImGui::ColorEdit3("X Axis (Red)", (float*)&_gridSettings.xAxisColor, ImGuiColorEditFlags_NoInputs);
                    ImGui::SameLine();
                    ImGui::ColorEdit3("Z Axis (Blue)", (float*)&_gridSettings.zAxisColor, ImGuiColorEditFlags_NoInputs);
                }

                ImGui::TreePop();
            }

            ImGui::Spacing();

            // === LINE COLORS ===
            if (ImGui::TreeNode("Grid Line Colors")) {

                ImGui::ColorEdit3("Minor Lines", (float*)&_gridSettings.minorLineColor);
                ImGui::ColorEdit3("Major Lines", (float*)&_gridSettings.majorLineColor);

                ImGui::TreePop();
            }

            ImGui::Spacing();

            // === ADVANCED ===
            if (ImGui::TreeNode("Advanced")) {

                ImGui::Text("Grid Height (Y)");
                ImGui::SetNextItemWidth(-1);
                ImGui::SliderFloat("##GridHeight", &_gridSettings.gridHeight, -100.0f, 100.0f, "%.1f");

                ImGui::Text("Min Fade Alpha");
                ImGui::SetNextItemWidth(-1);
                ImGui::SliderFloat("##MinFade", &_gridSettings.minFadeAlpha, 0.0f, 0.5f, "%.2f");

                ImGui::TreePop();
            }

            ImGui::Spacing();
            ImGui::Separator();

            // === INFO & RESET ===
            if (ImGui::Button("Reset to Default", ImVec2(-1, 0))) {
                _gridSettings = GridSettings();
            }

            ImGui::Spacing();

            // Debug info
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
            ImGui::Text("Camera: (%.1f, %.1f, %.1f)", mainCamera.position.x, mainCamera.position.y, mainCamera.position.z);

            if (_gridSettings.dynamicLOD) {
                float camHeight = std::max(std::abs(mainCamera.position.y - _gridSettings.gridHeight), 1.0f);
                float lod = std::max(0.0f, (std::log(camHeight / _gridSettings.baseGridSize) / std::log(10.0f)) - 0.5f + _gridSettings.lodBias);
                float currentSize = _gridSettings.baseGridSize * std::pow(10.0f, std::floor(lod));
                ImGui::Text("LOD Level: %.1f | Grid Size: %.2f", lod, currentSize);
            }
            ImGui::PopStyleColor();

            if (!_showGrid) {
                ImGui::EndDisabled();
            }
        }
    }
    ImGui::End();
}





//void VulkanEngine::draw_primitive_spawner_imgui() {
//    if (ImGui::Begin("Primitive Ekle")) {
//        static int selectedPrimitive = 0;
//        const char* primitiveNames[] = {
//            "Cube", "Sphere", "Capsule", "Cylinder", "Plane", "Cone", "Torus", "Triangle"
//        };
//
//        ImGui::Combo("Primitive", &selectedPrimitive, primitiveNames, IM_ARRAYSIZE(primitiveNames));
//
//        if (ImGui::Button("Ekle")) {
//            StaticMeshData newMesh;
//            newMesh.position = glm::vec3(static_shapes.size() * 3.0f, 0.0f, -5.0f);
//            newMesh.type = static_cast<PrimitiveType>(selectedPrimitive);
//
//            switch (newMesh.type) {
//            case PrimitiveType::Cube:     newMesh.mesh = generate_cube_mesh(); break;
//            case PrimitiveType::Sphere:   newMesh.mesh = generate_sphere_mesh(); break;
//            case PrimitiveType::Capsule:  newMesh.mesh = generate_capsule_mesh(); break;
//            case PrimitiveType::Cylinder: newMesh.mesh = generate_cylinder_mesh(); break;
//            case PrimitiveType::Plane:    newMesh.mesh = generate_plane_mesh(); break;
//            case PrimitiveType::Cone:     newMesh.mesh = generate_cone_mesh(); break;
//            case PrimitiveType::Torus:    newMesh.mesh = generate_torus_mesh(); break;
//            case PrimitiveType::Triangle: newMesh.mesh = generate_triangle_mesh(); break;
//            }
//
//            static_shapes.push_back(newMesh);
//        }
//    }
//    ImGui::End();
//}
// =============================================================================
// PRIMITIVE SPAWNER - Professional Dockable UI with Face Colors
// =============================================================================
void VulkanEngine::draw_primitive_spawner_imgui() {
    // Static variables for spawn settings
    static int selectedTab = 0;  // 0 = 2D, 1 = 3D
    static int selected2D = 0;
    static int selected3D = 0;
    static char primitiveName[64] = "";
    static int primitiveCounter = 0;

    // Spawn transform
    static glm::vec3 spawnPosition = glm::vec3(0.0f, 0.0f, -5.0f);
    static glm::vec3 spawnRotation = glm::vec3(0.0f);
    static glm::vec3 spawnScale = glm::vec3(1.0f);

    // Color settings
    static glm::vec4 mainColor = glm::vec4(1.0f);
    static bool useFaceColors = false;
    static glm::vec4 faceColors[6] = {
        glm::vec4(1.0f, 0.3f, 0.3f, 1.0f),  // Front (+Z) - Red
        glm::vec4(0.3f, 1.0f, 0.3f, 1.0f),  // Back (-Z) - Green
        glm::vec4(0.3f, 0.3f, 1.0f, 1.0f),  // Right (+X) - Blue
        glm::vec4(1.0f, 1.0f, 0.3f, 1.0f),  // Left (-X) - Yellow
        glm::vec4(1.0f, 0.3f, 1.0f, 1.0f),  // Top (+Y) - Magenta
        glm::vec4(0.3f, 1.0f, 1.0f, 1.0f),  // Bottom (-Y) - Cyan
    };

    // Window with docking support
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_None;

    // Dark themed window
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.12f, 0.12f, 0.14f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.08f, 0.08f, 0.10f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.15f, 0.35f, 0.55f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.4f, 0.6f, 0.6f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.25f, 0.5f, 0.7f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.3f, 0.55f, 0.75f, 1.0f));

    ImGui::SetNextWindowSize(ImVec2(350, 550), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Primitive Spawner", nullptr, windowFlags)) {

        // === HEADER ===
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "CREATE PRIMITIVES");
        ImGui::Separator();
        ImGui::Spacing();

        // === TAB BAR ===
        if (ImGui::BeginTabBar("PrimitiveTabBar", ImGuiTabBarFlags_None)) {
            // 2D Shapes Tab
            if (ImGui::BeginTabItem("2D Shapes")) {
                selectedTab = 0;
                ImGui::Spacing();

                const char* shapes2D[] = { "Triangle", "Plane", "Quad" };
                ImGui::Text("Shape Type:");
                ImGui::SetNextItemWidth(-1);
                ImGui::Combo("##Shape2D", &selected2D, shapes2D, IM_ARRAYSIZE(shapes2D));

                ImGui::EndTabItem();
            }

            // 3D Shapes Tab
            if (ImGui::BeginTabItem("3D Shapes")) {
                selectedTab = 1;
                ImGui::Spacing();

                const char* shapes3D[] = { "Cube", "Sphere", "Cylinder", "Cone", "Capsule", "Torus" };
                ImGui::Text("Shape Type:");
                ImGui::SetNextItemWidth(-1);
                ImGui::Combo("##Shape3D", &selected3D, shapes3D, IM_ARRAYSIZE(shapes3D));

                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // === SPAWN SETTINGS ===
        if (ImGui::CollapsingHeader("Transform Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Indent(10.0f);

            ImGui::Text("Position");
            ImGui::SetNextItemWidth(-1);
            ImGui::DragFloat3("##Position", &spawnPosition.x, 0.1f, -1000.0f, 1000.0f, "%.2f");

            ImGui::Text("Rotation (Degrees)");
            ImGui::SetNextItemWidth(-1);
            glm::vec3 rotDegrees = glm::degrees(spawnRotation);
            if (ImGui::DragFloat3("##Rotation", &rotDegrees.x, 1.0f, -360.0f, 360.0f, "%.1f")) {
                spawnRotation = glm::radians(rotDegrees);
            }

            ImGui::Text("Scale");
            ImGui::SetNextItemWidth(-1);
            ImGui::DragFloat3("##Scale", &spawnScale.x, 0.05f, 0.01f, 100.0f, "%.2f");

            if (ImGui::Button("Reset Transform", ImVec2(-1, 0))) {
                spawnPosition = glm::vec3(0.0f, 0.0f, -5.0f);
                spawnRotation = glm::vec3(0.0f);
                spawnScale = glm::vec3(1.0f);
            }

            ImGui::Unindent(10.0f);
        }

        ImGui::Spacing();

        // === COLOR SETTINGS ===
        if (ImGui::CollapsingHeader("Color Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Indent(10.0f);

            // Main color
            ImGui::Text("Main Color");
            ImGui::SetNextItemWidth(-1);
            ImGui::ColorEdit4("##MainColor", &mainColor.x, ImGuiColorEditFlags_AlphaBar);

            ImGui::Spacing();

            // Face colors toggle
            ImGui::Checkbox("Use Face Colors", &useFaceColors);

            if (useFaceColors) {
                ImGui::Indent(10.0f);
                ImGui::Spacing();

                // Face color editors in a grid layout
                const char* faceNames[] = { "Front (+Z)", "Back (-Z)", "Right (+X)", "Left (-X)", "Top (+Y)", "Bottom (-Y)" };

                for (int i = 0; i < 6; i++) {
                    ImGui::PushID(i);
                    ImGui::Text("%s", faceNames[i]);
                    ImGui::SetNextItemWidth(-1);
                    ImGui::ColorEdit4("##FaceColor", &faceColors[i].x, ImGuiColorEditFlags_NoLabel);
                    ImGui::PopID();

                    if (i < 5) ImGui::Spacing();
                }

                ImGui::Spacing();
                if (ImGui::Button("Randomize Face Colors", ImVec2(-1, 0))) {
                    for (int i = 0; i < 6; i++) {
                        faceColors[i] = glm::vec4(
                            (float)(rand() % 100) / 100.0f,
                            (float)(rand() % 100) / 100.0f,
                            (float)(rand() % 100) / 100.0f,
                            1.0f
                        );
                    }
                }

                ImGui::Unindent(10.0f);
            }

            if (ImGui::Button("Reset Colors", ImVec2(-1, 0))) {
                mainColor = glm::vec4(1.0f);
                useFaceColors = false;
                faceColors[0] = glm::vec4(1.0f, 0.3f, 0.3f, 1.0f);
                faceColors[1] = glm::vec4(0.3f, 1.0f, 0.3f, 1.0f);
                faceColors[2] = glm::vec4(0.3f, 0.3f, 1.0f, 1.0f);
                faceColors[3] = glm::vec4(1.0f, 1.0f, 0.3f, 1.0f);
                faceColors[4] = glm::vec4(1.0f, 0.3f, 1.0f, 1.0f);
                faceColors[5] = glm::vec4(0.3f, 1.0f, 1.0f, 1.0f);
            }

            ImGui::Unindent(10.0f);
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // === ADD PRIMITIVE BUTTON ===
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.55f, 0.25f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.65f, 0.35f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.25f, 0.75f, 0.4f, 1.0f));

        if (ImGui::Button("+ Add Primitive", ImVec2(-1, 45))) {
            StaticMeshData newMesh;

            // Auto-generate name
            const char* typeNames2D[] = { "Triangle", "Plane", "Quad" };
            const char* typeNames3D[] = { "Cube", "Sphere", "Cylinder", "Cone", "Capsule", "Torus" };

            if (selectedTab == 0) {
                newMesh.name = std::string(typeNames2D[selected2D]) + "_" + std::to_string(++primitiveCounter);
            } else {
                newMesh.name = std::string(typeNames3D[selected3D]) + "_" + std::to_string(++primitiveCounter);
            }

            // Set transform
            newMesh.position = spawnPosition;
            newMesh.rotation = spawnRotation;
            newMesh.scale = spawnScale;

            // Set colors
            newMesh.mainColor = mainColor;
            newMesh.useFaceColors = useFaceColors;
            for (int i = 0; i < 6; i++) {
                newMesh.faceColors[i] = faceColors[i];
            }

            // Generate mesh based on selection
            if (selectedTab == 0) {
                // 2D Shapes
                switch (selected2D) {
                case 0: newMesh.type = PrimitiveType::Triangle; newMesh.mesh = generate_triangle_mesh(); break;
                case 1: newMesh.type = PrimitiveType::Plane; newMesh.mesh = generate_plane_mesh(); break;
                case 2: newMesh.type = PrimitiveType::Plane; newMesh.mesh = generate_plane_mesh(); break;
                }
            } else {
                // 3D Shapes
                switch (selected3D) {
                case 0: newMesh.type = PrimitiveType::Cube; newMesh.mesh = generate_cube_mesh(); break;
                case 1: newMesh.type = PrimitiveType::Sphere; newMesh.mesh = generate_sphere_mesh(); break;
                case 2: newMesh.type = PrimitiveType::Cylinder; newMesh.mesh = generate_cylinder_mesh(); break;
                case 3: newMesh.type = PrimitiveType::Cone; newMesh.mesh = generate_cone_mesh(); break;
                case 4: newMesh.type = PrimitiveType::Capsule; newMesh.mesh = generate_capsule_mesh(); break;
                case 5: newMesh.type = PrimitiveType::Torus; newMesh.mesh = generate_torus_mesh(); break;
                }
            }

            newMesh.materialType = ShaderOnlyMaterial::DEFAULT;
            newMesh.passType = MaterialPass::MainColor;
            newMesh.visible = true;
            newMesh.selected = false;

            static_shapes.push_back(newMesh);

            // Auto-advance position for next spawn
            spawnPosition.x += 2.5f;
        }
        ImGui::PopStyleColor(3);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // === QUICK ACTIONS ===
        if (ImGui::CollapsingHeader("Quick Actions")) {
            ImGui::Indent(10.0f);

            if (!static_shapes.empty()) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.3f, 0.3f, 1.0f));

                if (ImGui::Button("Delete Last", ImVec2(-1, 0))) {
                    static_shapes.pop_back();
                }

                if (ImGui::Button("Clear All", ImVec2(-1, 0))) {
                    static_shapes.clear();
                    primitiveCounter = 0;
                }

                ImGui::PopStyleColor(2);
            }

            ImGui::Unindent(10.0f);
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // === STATISTICS ===
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Statistics");
        ImGui::Text("Total Primitives: %zu", static_shapes.size());

        // Count visible
        int visibleCount = 0;
        for (const auto& shape : static_shapes) {
            if (shape.visible) visibleCount++;
        }
        ImGui::Text("Visible: %d", visibleCount);
    }
    ImGui::End();

    ImGui::PopStyleColor(6);
}



GPUMeshBuffers load_obj_mesh(VulkanEngine* engine, const std::string& filename) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.c_str());
    if (!warn.empty()) std::cout << "OBJ WARN: " << warn << std::endl;
    if (!err.empty()) std::cerr << "OBJ ERR: " << err << std::endl;
    if (!ret) throw std::runtime_error("Failed to load OBJ file!");

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex v{};
            v.position = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };
            if (index.normal_index >= 0 && !attrib.normals.empty()) {
                v.normal = {
                    attrib.normals[3 * index.normal_index + 0],
                    attrib.normals[3 * index.normal_index + 1],
                    attrib.normals[3 * index.normal_index + 2]
                };
            }
            else {
                v.normal = glm::vec3(0, 1, 0);
            }
            if (index.texcoord_index >= 0 && !attrib.texcoords.empty()) {
                v.uv_x = attrib.texcoords[2 * index.texcoord_index + 0];
                v.uv_y = attrib.texcoords[2 * index.texcoord_index + 1];
            }
            else {
                v.uv_x = 0.0f;
                v.uv_y = 0.0f;
            }
            v.color = glm::vec4(1.0f);
            vertices.push_back(v);
            indices.push_back(static_cast<uint32_t>(indices.size()));
        }
    }

    return engine->uploadMesh(indices, vertices);
}


GPUMeshBuffers VulkanEngine::generate_triangle_mesh() {
    // Triangle facing +Z with proper normals for lighting
    glm::vec3 normal(0.0f, 0.0f, 1.0f);
    std::vector<Vertex> vertices = {
        { glm::vec3(0, 1, 0), 0, normal, 0, glm::vec4(1) },
        { glm::vec3(-1, -1, 0), 0, normal, 0, glm::vec4(1) },
        { glm::vec3(1, -1, 0), 0, normal, 0, glm::vec4(1) },
    };
    std::vector<uint32_t> indices = { 0, 1, 2 };
    return uploadMesh(indices, vertices);
}

GPUMeshBuffers VulkanEngine::generate_plane_mesh() {
    std::vector<Vertex> vertices = {
        { { -1, 0, -1 }, 0, { 0, 1, 0 }, 0, glm::vec4(1) },
        { { 1, 0, -1 }, 0, { 0, 1, 0 }, 0, glm::vec4(1) },
        { { 1, 0, 1 }, 0, { 0, 1, 0 }, 0, glm::vec4(1) },
        { { -1, 0, 1 }, 0, { 0, 1, 0 }, 0, glm::vec4(1) },
    };
    std::vector<uint32_t> indices = {
        0, 1, 2,
        2, 3, 0
    };
    return uploadMesh(indices, vertices);
}

GPUMeshBuffers VulkanEngine::generate_cube_mesh() {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    // Unit cube from -0.5 to +0.5, with per-face normals for proper lighting
    // Each face has 4 vertices with the same normal

    // Front face (+Z) - vertices 0-3
    vertices.push_back({ { -0.5f, -0.5f,  0.5f }, 0.0f, { 0.0f, 0.0f, 1.0f }, 0.0f, glm::vec4(1.0f) });
    vertices.push_back({ {  0.5f, -0.5f,  0.5f }, 1.0f, { 0.0f, 0.0f, 1.0f }, 0.0f, glm::vec4(1.0f) });
    vertices.push_back({ {  0.5f,  0.5f,  0.5f }, 1.0f, { 0.0f, 0.0f, 1.0f }, 1.0f, glm::vec4(1.0f) });
    vertices.push_back({ { -0.5f,  0.5f,  0.5f }, 0.0f, { 0.0f, 0.0f, 1.0f }, 1.0f, glm::vec4(1.0f) });

    // Back face (-Z) - vertices 4-7
    vertices.push_back({ {  0.5f, -0.5f, -0.5f }, 0.0f, { 0.0f, 0.0f, -1.0f }, 0.0f, glm::vec4(1.0f) });
    vertices.push_back({ { -0.5f, -0.5f, -0.5f }, 1.0f, { 0.0f, 0.0f, -1.0f }, 0.0f, glm::vec4(1.0f) });
    vertices.push_back({ { -0.5f,  0.5f, -0.5f }, 1.0f, { 0.0f, 0.0f, -1.0f }, 1.0f, glm::vec4(1.0f) });
    vertices.push_back({ {  0.5f,  0.5f, -0.5f }, 0.0f, { 0.0f, 0.0f, -1.0f }, 1.0f, glm::vec4(1.0f) });

    // Right face (+X) - vertices 8-11
    vertices.push_back({ {  0.5f, -0.5f,  0.5f }, 0.0f, { 1.0f, 0.0f, 0.0f }, 0.0f, glm::vec4(1.0f) });
    vertices.push_back({ {  0.5f, -0.5f, -0.5f }, 1.0f, { 1.0f, 0.0f, 0.0f }, 0.0f, glm::vec4(1.0f) });
    vertices.push_back({ {  0.5f,  0.5f, -0.5f }, 1.0f, { 1.0f, 0.0f, 0.0f }, 1.0f, glm::vec4(1.0f) });
    vertices.push_back({ {  0.5f,  0.5f,  0.5f }, 0.0f, { 1.0f, 0.0f, 0.0f }, 1.0f, glm::vec4(1.0f) });

    // Left face (-X) - vertices 12-15
    vertices.push_back({ { -0.5f, -0.5f, -0.5f }, 0.0f, { -1.0f, 0.0f, 0.0f }, 0.0f, glm::vec4(1.0f) });
    vertices.push_back({ { -0.5f, -0.5f,  0.5f }, 1.0f, { -1.0f, 0.0f, 0.0f }, 0.0f, glm::vec4(1.0f) });
    vertices.push_back({ { -0.5f,  0.5f,  0.5f }, 1.0f, { -1.0f, 0.0f, 0.0f }, 1.0f, glm::vec4(1.0f) });
    vertices.push_back({ { -0.5f,  0.5f, -0.5f }, 0.0f, { -1.0f, 0.0f, 0.0f }, 1.0f, glm::vec4(1.0f) });

    // Top face (+Y) - vertices 16-19
    vertices.push_back({ { -0.5f,  0.5f,  0.5f }, 0.0f, { 0.0f, 1.0f, 0.0f }, 0.0f, glm::vec4(1.0f) });
    vertices.push_back({ {  0.5f,  0.5f,  0.5f }, 1.0f, { 0.0f, 1.0f, 0.0f }, 0.0f, glm::vec4(1.0f) });
    vertices.push_back({ {  0.5f,  0.5f, -0.5f }, 1.0f, { 0.0f, 1.0f, 0.0f }, 1.0f, glm::vec4(1.0f) });
    vertices.push_back({ { -0.5f,  0.5f, -0.5f }, 0.0f, { 0.0f, 1.0f, 0.0f }, 1.0f, glm::vec4(1.0f) });

    // Bottom face (-Y) - vertices 20-23
    vertices.push_back({ { -0.5f, -0.5f, -0.5f }, 0.0f, { 0.0f, -1.0f, 0.0f }, 0.0f, glm::vec4(1.0f) });
    vertices.push_back({ {  0.5f, -0.5f, -0.5f }, 1.0f, { 0.0f, -1.0f, 0.0f }, 0.0f, glm::vec4(1.0f) });
    vertices.push_back({ {  0.5f, -0.5f,  0.5f }, 1.0f, { 0.0f, -1.0f, 0.0f }, 1.0f, glm::vec4(1.0f) });
    vertices.push_back({ { -0.5f, -0.5f,  0.5f }, 0.0f, { 0.0f, -1.0f, 0.0f }, 1.0f, glm::vec4(1.0f) });

    // Indices: 6 faces x 2 triangles x 3 vertices = 36 indices
    // Front face
    indices.push_back(0); indices.push_back(1); indices.push_back(2);
    indices.push_back(2); indices.push_back(3); indices.push_back(0);
    // Back face
    indices.push_back(4); indices.push_back(5); indices.push_back(6);
    indices.push_back(6); indices.push_back(7); indices.push_back(4);
    // Right face
    indices.push_back(8); indices.push_back(9); indices.push_back(10);
    indices.push_back(10); indices.push_back(11); indices.push_back(8);
    // Left face
    indices.push_back(12); indices.push_back(13); indices.push_back(14);
    indices.push_back(14); indices.push_back(15); indices.push_back(12);
    // Top face
    indices.push_back(16); indices.push_back(17); indices.push_back(18);
    indices.push_back(18); indices.push_back(19); indices.push_back(16);
    // Bottom face
    indices.push_back(20); indices.push_back(21); indices.push_back(22);
    indices.push_back(22); indices.push_back(23); indices.push_back(20);

    return uploadMesh(indices, vertices);
}
//void VulkanEngine::draw_shader_only_static_shapes(VkCommandBuffer cmd, VkDescriptorSet globalDescriptor, VkViewport viewport, VkRect2D scissor)
//{
//    vkCmdSetViewport(cmd, 0, 1, &viewport);
//    vkCmdSetScissor(cmd, 0, 1, &scissor);
//
//    // Descriptor set 1 (texture) için set alınmalı
//    VkDescriptorSet drawImageDescriptors = get_current_frame().drawImageDescriptorSet;
//
//    VkDescriptorSet sets[] = {
//        globalDescriptor,
//        drawImageDescriptors
//    };
//
//    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _2dPipeline);
//
//    for (const StaticMeshData& shape : static_shapes)
//    {
//        // Sadece görünür olan ana pass objeleri çizilir
//        if (shape.passType != MaterialPass::MainColor) continue;
//
//        VkDeviceSize offset = 0;
//        vkCmdBindVertexBuffers(cmd, 0, 1, &shape.mesh.vertexBuffer.buffer, &offset);
//        vkCmdBindIndexBuffer(cmd, shape.mesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
//
//        // Descriptor Set bağla
//        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _2dPipelineLayout, 0, 2, sets, 0, nullptr);
//
//        // Push Constants doldur
//        GPUDrawPushConstants push{};
//        push.worldMatrix = shape.get_transform();
//        push.vertexBuffer = shape.mesh.vertexBufferAddress;
//        push.outlineScale = 0.f;
//
//        vkCmdPushConstants(cmd, _2dPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GPUDrawPushConstants), &push);
//
//        vkCmdDrawIndexed(cmd, shape.mesh.indexCount, 1, 0, 0, 0);
//    }
//}

//void VulkanEngine::draw_shader_only_static_shapes(VkCommandBuffer cmd, VkDescriptorSet globalDescriptor, VkViewport viewport, VkRect2D scissor)
//{
//    vkCmdSetViewport(cmd, 0, 1, &viewport);
//    vkCmdSetScissor(cmd, 0, 1, &scissor);
//
//    // Descriptor set 1 (texture) için set alınmalı
//    VkDescriptorSet drawImageDescriptors = get_current_frame().drawImageDescriptorSet;
//
//    VkDescriptorSet sets[] = {
//        globalDescriptor,
//        drawImageDescriptors
//    };
//
//    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _2dPipeline);
//
//    for (const StaticMeshData& shape : static_shapes)
//    {
//        // Sadece görünür olan ana pass objeleri çizilir
//        if (shape.passType != MaterialPass::MainColor) continue;
//
//        VkDeviceSize offset = 0;
//        vkCmdBindVertexBuffers(cmd, 0, 1, &shape.mesh.vertexBuffer.buffer, &offset);
//        vkCmdBindIndexBuffer(cmd, shape.mesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
//
//        // Descriptor Set bağla
//        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _2dPipelineLayout, 0, 2, sets, 0, nullptr);
//
//        // Push Constants doldur
//        GPUDrawPushConstants push{};
//        push.worldMatrix = shape.get_transform();
//        push.vertexBuffer = shape.mesh.vertexBufferAddress;
//        push.outlineScale = 0.f;
//
//        // ✅ Renk belirleme
//        switch (shape.materialType)
//        {
//        case ShaderOnlyMaterial::EMISSIVE:
//            push.baseColor = glm::vec4(1.0f, 1.0f, 0.2f, 1.0f); // sarımsı ışık
//            break;
//        case ShaderOnlyMaterial::DEFAULT:
//        default:
//            push.baseColor = glm::vec4(1.0f); // beyaz
//            break;
//        }
//
//        vkCmdPushConstants(cmd, _2dPipelineLayout,
//            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
//            0, sizeof(GPUDrawPushConstants), &push);
//
//        vkCmdDrawIndexed(cmd, shape.mesh.indexCount, 1, 0, 0, 0);
//    }
//}


//void VulkanEngine::draw_shader_only_static_shapes(VkCommandBuffer cmd, VkDescriptorSet globalDescriptor, VkViewport viewport, VkRect2D scissor)
//{
//    vkCmdSetViewport(cmd, 0, 1, &viewport);
//    vkCmdSetScissor(cmd, 0, 1, &scissor);
//
//    VkDescriptorSet drawImageDescriptors = get_current_frame().drawImageDescriptorSet;
//
//    VkDescriptorSet sets[] = {
//        globalDescriptor,
//        drawImageDescriptors
//    };
//
//    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _2dPipeline);
//
//    for (const StaticMeshData& shape : static_shapes)
//    {
//        if (shape.passType != MaterialPass::MainColor) continue;
//
//        VkDeviceSize offset = 0;
//        vkCmdBindVertexBuffers(cmd, 0, 1, &shape.mesh.vertexBuffer.buffer, &offset);
//        vkCmdBindIndexBuffer(cmd, shape.mesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
//
//        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _2dPipelineLayout, 0, 2, sets, 0, nullptr);
//
//        GPUDrawPushConstants push{};
//        push.worldMatrix = shape.get_transform();
//        push.vertexBuffer = shape.mesh.vertexBufferAddress;
//        push.outlineScale = 0.f;
//
//        // 🔥 Dinamik renk gönder (artık shape.color'dan geliyor)
//        push.baseColor = shape.color;
//
//        vkCmdPushConstants(cmd, _2dPipelineLayout,
//            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
//            0, sizeof(GPUDrawPushConstants), &push);
//
//        vkCmdDrawIndexed(cmd, shape.mesh.indexCount, 1, 0, 0, 0);
//    }
//}
//

void VulkanEngine::draw_shader_only_static_shapes(VkCommandBuffer cmd, VkDescriptorSet globalDescriptor, VkViewport viewport, VkRect2D scissor)
{
    vkCmdSetViewport(cmd, 0, 1, &viewport);
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    VkDescriptorSet drawImageDescriptors = get_current_frame().drawImageDescriptorSet;

    VkDescriptorSet sets[] = {
        globalDescriptor,
        drawImageDescriptors
    };

    for (const StaticMeshData& shape : static_shapes)
    {
        if (shape.passType != MaterialPass::MainColor &&
            shape.passType != MaterialPass::Transparent) continue;

        // ✅ pipeline seçimi materialType'a göre yapılır
        VkPipeline pipelineToUse = _2dPipeline;
        VkPipelineLayout layoutToUse = _2dPipelineLayout;

        switch (shape.materialType)
        {
        case ShaderOnlyMaterial::GRID:
            pipelineToUse = _gridPipeline;
            layoutToUse = _gridPipelineLayout;
            break;
        case ShaderOnlyMaterial::EMISSIVE:
        case ShaderOnlyMaterial::POINTLIGHT_VIS:  // Use emissive pipeline for light visualization
            pipelineToUse = _emissivePipeline;
            layoutToUse = _emissivePipelineLayout;
            break;
        default:
            pipelineToUse = _2dPipeline;
            layoutToUse = _2dPipelineLayout;
            break;
        }

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineToUse);

        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(cmd, 0, 1, &shape.mesh.vertexBuffer.buffer, &offset);
        vkCmdBindIndexBuffer(cmd, shape.mesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layoutToUse, 0, 2, sets, 0, nullptr);

        GPUDrawPushConstants push{};
        push.worldMatrix = shape.get_transform();
        push.vertexBuffer = shape.mesh.vertexBufferAddress;
        push.outlineScale = 0.f;
        push.baseColor = shape.mainColor;

        vkCmdPushConstants(cmd, layoutToUse,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            0, sizeof(GPUDrawPushConstants), &push);

        vkCmdDrawIndexed(cmd, shape.mesh.indexCount, 1, 0, 0, 0);
    }
}






GPUMeshBuffers VulkanEngine::generate_sphere_mesh(int resolution, int rings) {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    for (int y = 0; y <= rings; ++y) {
        for (int x = 0; x <= resolution; ++x) {
            float xSegment = (float)x / (float)resolution;
            float ySegment = (float)y / (float)rings;
            float xPos = std::cos(xSegment * 2.0f * glm::pi<float>()) * std::sin(ySegment * glm::pi<float>());
            float yPos = std::cos(ySegment * glm::pi<float>());
            float zPos = std::sin(xSegment * 2.0f * glm::pi<float>()) * std::sin(ySegment * glm::pi<float>());

            vertices.push_back({ glm::vec3(xPos, yPos, zPos), 0, glm::vec3(xPos, yPos, zPos), 0, glm::vec4(1) });
        }
    }

    for (int y = 0; y < rings; ++y) {
        for (int x = 0; x < resolution; ++x) {
            int i0 = y * (resolution + 1) + x;
            int i1 = i0 + resolution + 1;

            indices.push_back(i0);
            indices.push_back(i1);
            indices.push_back(i0 + 1);

            indices.push_back(i0 + 1);
            indices.push_back(i1);
            indices.push_back(i1 + 1);
        }
    }

    return uploadMesh(indices, vertices);
}

GPUMeshBuffers VulkanEngine::generate_cylinder_mesh(int segments) {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    // Side vertices with outward-facing normals
    for (int i = 0; i <= segments; ++i) {
        float angle = 2.0f * glm::pi<float>() * i / segments;
        float x = std::cos(angle);
        float z = std::sin(angle);
        glm::vec3 sideNormal(x, 0.0f, z);

        vertices.push_back({ { x, -1, z }, 0, sideNormal, 0, glm::vec4(1) });
        vertices.push_back({ { x, 1, z }, 0, sideNormal, 0, glm::vec4(1) });
    }

    // Side triangles
    for (int i = 0; i < segments; ++i) {
        int bottomCurrent = i * 2;
        int topCurrent = i * 2 + 1;
        int bottomNext = (i + 1) * 2;
        int topNext = (i + 1) * 2 + 1;

        indices.push_back(bottomCurrent);
        indices.push_back(topCurrent);
        indices.push_back(bottomNext);

        indices.push_back(bottomNext);
        indices.push_back(topCurrent);
        indices.push_back(topNext);
    }

    // Top cap
    int topCenterIdx = static_cast<int>(vertices.size());
    vertices.push_back({ { 0, 1, 0 }, 0, { 0, 1, 0 }, 0, glm::vec4(1) });

    for (int i = 0; i <= segments; ++i) {
        float angle = 2.0f * glm::pi<float>() * i / segments;
        float x = std::cos(angle);
        float z = std::sin(angle);
        vertices.push_back({ { x, 1, z }, 0, { 0, 1, 0 }, 0, glm::vec4(1) });
    }

    for (int i = 0; i < segments; ++i) {
        indices.push_back(topCenterIdx);
        indices.push_back(topCenterIdx + 1 + i);
        indices.push_back(topCenterIdx + 1 + i + 1);
    }

    // Bottom cap
    int bottomCenterIdx = static_cast<int>(vertices.size());
    vertices.push_back({ { 0, -1, 0 }, 0, { 0, -1, 0 }, 0, glm::vec4(1) });

    for (int i = 0; i <= segments; ++i) {
        float angle = 2.0f * glm::pi<float>() * i / segments;
        float x = std::cos(angle);
        float z = std::sin(angle);
        vertices.push_back({ { x, -1, z }, 0, { 0, -1, 0 }, 0, glm::vec4(1) });
    }

    for (int i = 0; i < segments; ++i) {
        indices.push_back(bottomCenterIdx);
        indices.push_back(bottomCenterIdx + 1 + i + 1);
        indices.push_back(bottomCenterIdx + 1 + i);
    }

    return uploadMesh(indices, vertices);
}

GPUMeshBuffers VulkanEngine::generate_cone_mesh() {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    int segments = 32;

    // Cone tip - normals point outward along the surface
    // For a cone, normal = normalize(outward + up * slope)
    float slope = 0.5f; // height/radius ratio

    for (int i = 0; i <= segments; ++i) {
        float angle = 2.0f * glm::pi<float>() * i / segments;
        float x = std::cos(angle);
        float z = std::sin(angle);

        // Calculate proper cone surface normal
        glm::vec3 sideNormal = glm::normalize(glm::vec3(x, slope, z));

        // Tip vertex (with averaged normal)
        vertices.push_back({ { 0, 1, 0 }, 0, sideNormal, 0, glm::vec4(1) });
        // Base vertex
        vertices.push_back({ { x, -1, z }, 0, sideNormal, 0, glm::vec4(1) });
    }

    // Create triangles
    for (int i = 0; i < segments; ++i) {
        int tipCurrent = i * 2;
        int baseCurrent = i * 2 + 1;
        int baseNext = (i + 1) * 2 + 1;

        indices.push_back(tipCurrent);
        indices.push_back(baseCurrent);
        indices.push_back(baseNext);
    }

    // Bottom cap
    int centerIdx = static_cast<int>(vertices.size());
    vertices.push_back({ { 0, -1, 0 }, 0, { 0, -1, 0 }, 0, glm::vec4(1) });

    for (int i = 0; i < segments; ++i) {
        int baseCurrent = i * 2 + 1;
        int baseNext = (i + 1) * 2 + 1;

        indices.push_back(centerIdx);
        indices.push_back(baseNext);
        indices.push_back(baseCurrent);
    }

    return uploadMesh(indices, vertices);
}

GPUMeshBuffers VulkanEngine::generate_capsule_mesh() {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    const int segments = 24;      // Horizontal segments
    const int rings = 8;          // Rings per hemisphere
    const float radius = 0.5f;
    const float halfHeight = 0.5f; // Half of cylinder height

    // Top hemisphere
    for (int y = 0; y <= rings; ++y) {
        for (int x = 0; x <= segments; ++x) {
            float xSegment = (float)x / (float)segments;
            float ySegment = (float)y / (float)rings * 0.5f; // Only half sphere (0 to 0.5)

            float xPos = std::cos(xSegment * 2.0f * glm::pi<float>()) * std::sin(ySegment * glm::pi<float>());
            float yPos = std::cos(ySegment * glm::pi<float>());
            float zPos = std::sin(xSegment * 2.0f * glm::pi<float>()) * std::sin(ySegment * glm::pi<float>());

            glm::vec3 normal = glm::normalize(glm::vec3(xPos, yPos, zPos));
            glm::vec3 pos = normal * radius;
            pos.y += halfHeight; // Offset to top

            vertices.push_back({ pos, xSegment, normal, ySegment, glm::vec4(1.0f) });
        }
    }

    int topHemiVertCount = (int)vertices.size();

    // Bottom hemisphere
    for (int y = 0; y <= rings; ++y) {
        for (int x = 0; x <= segments; ++x) {
            float xSegment = (float)x / (float)segments;
            float ySegment = 0.5f + (float)y / (float)rings * 0.5f; // Second half (0.5 to 1.0)

            float xPos = std::cos(xSegment * 2.0f * glm::pi<float>()) * std::sin(ySegment * glm::pi<float>());
            float yPos = std::cos(ySegment * glm::pi<float>());
            float zPos = std::sin(xSegment * 2.0f * glm::pi<float>()) * std::sin(ySegment * glm::pi<float>());

            glm::vec3 normal = glm::normalize(glm::vec3(xPos, yPos, zPos));
            glm::vec3 pos = normal * radius;
            pos.y -= halfHeight; // Offset to bottom

            vertices.push_back({ pos, xSegment, normal, ySegment, glm::vec4(1.0f) });
        }
    }

    // Cylinder body connecting hemispheres
    int cylinderStart = (int)vertices.size();
    for (int x = 0; x <= segments; ++x) {
        float angle = 2.0f * glm::pi<float>() * x / segments;
        float xPos = std::cos(angle) * radius;
        float zPos = std::sin(angle) * radius;
        glm::vec3 normal = glm::normalize(glm::vec3(xPos, 0.0f, zPos));

        // Top ring of cylinder
        vertices.push_back({ { xPos, halfHeight, zPos }, (float)x / segments, normal, 0.0f, glm::vec4(1.0f) });
        // Bottom ring of cylinder
        vertices.push_back({ { xPos, -halfHeight, zPos }, (float)x / segments, normal, 1.0f, glm::vec4(1.0f) });
    }

    // Indices for top hemisphere
    for (int y = 0; y < rings; ++y) {
        for (int x = 0; x < segments; ++x) {
            int i0 = y * (segments + 1) + x;
            int i1 = i0 + segments + 1;

            indices.push_back(i0);
            indices.push_back(i1);
            indices.push_back(i0 + 1);

            indices.push_back(i0 + 1);
            indices.push_back(i1);
            indices.push_back(i1 + 1);
        }
    }

    // Indices for bottom hemisphere
    for (int y = 0; y < rings; ++y) {
        for (int x = 0; x < segments; ++x) {
            int i0 = topHemiVertCount + y * (segments + 1) + x;
            int i1 = i0 + segments + 1;

            indices.push_back(i0);
            indices.push_back(i1);
            indices.push_back(i0 + 1);

            indices.push_back(i0 + 1);
            indices.push_back(i1);
            indices.push_back(i1 + 1);
        }
    }

    // Indices for cylinder body
    for (int x = 0; x < segments; ++x) {
        int topCurrent = cylinderStart + x * 2;
        int bottomCurrent = topCurrent + 1;
        int topNext = cylinderStart + (x + 1) * 2;
        int bottomNext = topNext + 1;

        indices.push_back(topCurrent);
        indices.push_back(bottomCurrent);
        indices.push_back(topNext);

        indices.push_back(topNext);
        indices.push_back(bottomCurrent);
        indices.push_back(bottomNext);
    }

    return uploadMesh(indices, vertices);
}

GPUMeshBuffers VulkanEngine::generate_torus_mesh() {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    const int majorSegments = 32;  // Segments around the main ring
    const int minorSegments = 16;  // Segments around the tube
    const float majorRadius = 0.7f; // Distance from center to tube center
    const float minorRadius = 0.3f; // Tube radius

    for (int i = 0; i <= majorSegments; ++i) {
        float majorAngle = 2.0f * glm::pi<float>() * i / majorSegments;
        float cosMajor = std::cos(majorAngle);
        float sinMajor = std::sin(majorAngle);

        for (int j = 0; j <= minorSegments; ++j) {
            float minorAngle = 2.0f * glm::pi<float>() * j / minorSegments;
            float cosMinor = std::cos(minorAngle);
            float sinMinor = std::sin(minorAngle);

            // Position on torus surface
            float x = (majorRadius + minorRadius * cosMinor) * cosMajor;
            float y = minorRadius * sinMinor;
            float z = (majorRadius + minorRadius * cosMinor) * sinMajor;

            // Normal calculation
            float nx = cosMinor * cosMajor;
            float ny = sinMinor;
            float nz = cosMinor * sinMajor;

            float u = (float)i / majorSegments;
            float v = (float)j / minorSegments;

            vertices.push_back({ { x, y, z }, u, { nx, ny, nz }, v, glm::vec4(1.0f) });
        }
    }

    // Generate indices
    for (int i = 0; i < majorSegments; ++i) {
        for (int j = 0; j < minorSegments; ++j) {
            int i0 = i * (minorSegments + 1) + j;
            int i1 = i0 + minorSegments + 1;

            indices.push_back(i0);
            indices.push_back(i1);
            indices.push_back(i0 + 1);

            indices.push_back(i0 + 1);
            indices.push_back(i1);
            indices.push_back(i1 + 1);
        }
    }

    return uploadMesh(indices, vertices);
}






// =============================================================================
// SCENE PRIMITIVES - Professional Dockable UI with Property Editor
// =============================================================================
void VulkanEngine::draw_static_mesh_imgui() {
    static int selectedIndex = -1;
    static char searchBuffer[128] = "";
    static int filterType = -1;  // -1 = All types

    // Dark themed window
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.12f, 0.12f, 0.14f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.08f, 0.08f, 0.10f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.55f, 0.35f, 0.15f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.3f, 0.5f, 0.7f, 0.4f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.4f, 0.6f, 0.8f, 0.6f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.5f, 0.7f, 0.9f, 0.8f));

    ImGui::SetNextWindowSize(ImVec2(400, 600), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Scene Primitives")) {

        // === HEADER ===
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f), "SCENE PRIMITIVES");
        ImGui::Separator();
        ImGui::Spacing();

        // === SEARCH & FILTER ===
        ImGui::Text("Search:");
        ImGui::SetNextItemWidth(-100);
        ImGui::InputText("##Search", searchBuffer, sizeof(searchBuffer));
        ImGui::SameLine();

        const char* typeFilters[] = { "All", "Cube", "Sphere", "Cylinder", "Cone", "Capsule", "Torus", "Triangle", "Plane" };
        ImGui::SetNextItemWidth(90);
        ImGui::Combo("##Filter", &filterType, typeFilters, IM_ARRAYSIZE(typeFilters));

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // === PRIMITIVES LIST ===
        ImGui::BeginChild("PrimitivesList", ImVec2(0, 200), true, ImGuiWindowFlags_HorizontalScrollbar);

        for (int i = 0; i < static_shapes.size(); ++i) {
            StaticMeshData& mesh = static_shapes[i];

            // Apply search filter
            if (strlen(searchBuffer) > 0) {
                if (mesh.name.find(searchBuffer) == std::string::npos) continue;
            }

            // Apply type filter
            if (filterType > 0) {
                PrimitiveType targetType;
                switch (filterType) {
                case 1: targetType = PrimitiveType::Cube; break;
                case 2: targetType = PrimitiveType::Sphere; break;
                case 3: targetType = PrimitiveType::Cylinder; break;
                case 4: targetType = PrimitiveType::Cone; break;
                case 5: targetType = PrimitiveType::Capsule; break;
                case 6: targetType = PrimitiveType::Torus; break;
                case 7: targetType = PrimitiveType::Triangle; break;
                case 8: targetType = PrimitiveType::Plane; break;
                default: targetType = PrimitiveType::Cube; break;
                }
                if (mesh.type != targetType) continue;
            }

            ImGui::PushID(i);

            // Visibility indicator
            ImGui::PushStyleColor(ImGuiCol_Text, mesh.visible ? ImVec4(0.3f, 0.9f, 0.3f, 1.0f) : ImVec4(0.5f, 0.5f, 0.5f, 0.5f));
            if (ImGui::Checkbox("##Visible", &mesh.visible)) {
                // Visibility toggled
            }
            ImGui::PopStyleColor();
            ImGui::SameLine();

            // Selection
            bool isSelected = (selectedIndex == i);
            if (ImGui::Selectable(mesh.name.empty() ? "Unnamed" : mesh.name.c_str(), isSelected, ImGuiSelectableFlags_AllowDoubleClick)) {
                selectedIndex = i;
                mesh.selected = true;
                // Deselect others
                for (int j = 0; j < static_shapes.size(); j++) {
                    if (j != i) static_shapes[j].selected = false;
                }
            }

            ImGui::PopID();
        }

        ImGui::EndChild();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // === PROPERTY INSPECTOR ===
        if (selectedIndex >= 0 && selectedIndex < static_shapes.size()) {
            StaticMeshData& selected = static_shapes[selectedIndex];

            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "PROPERTIES: %s", selected.name.c_str());
            ImGui::Separator();

            // Name editing
            char nameBuffer[64];
            strncpy(nameBuffer, selected.name.c_str(), sizeof(nameBuffer));
            nameBuffer[sizeof(nameBuffer) - 1] = '\0';
            ImGui::Text("Name:");
            ImGui::SetNextItemWidth(-1);
            if (ImGui::InputText("##Name", nameBuffer, sizeof(nameBuffer))) {
                selected.name = nameBuffer;
            }

            ImGui::Spacing();

            // Transform section
            if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Indent(10.0f);

                ImGui::Text("Position");
                ImGui::SetNextItemWidth(-1);
                ImGui::DragFloat3("##Pos", &selected.position.x, 0.1f, -1000.0f, 1000.0f, "%.2f");

                ImGui::Text("Rotation (Degrees)");
                glm::vec3 rotDegrees = glm::degrees(selected.rotation);
                ImGui::SetNextItemWidth(-1);
                if (ImGui::DragFloat3("##Rot", &rotDegrees.x, 1.0f, -360.0f, 360.0f, "%.1f")) {
                    selected.rotation = glm::radians(rotDegrees);
                }

                ImGui::Text("Scale");
                ImGui::SetNextItemWidth(-1);
                ImGui::DragFloat3("##Scale", &selected.scale.x, 0.05f, 0.01f, 100.0f, "%.2f");

                if (ImGui::Button("Reset Transform", ImVec2(-1, 0))) {
                    selected.position = glm::vec3(0.0f, 0.0f, -5.0f);
                    selected.rotation = glm::vec3(0.0f);
                    selected.scale = glm::vec3(1.0f);
                }

                ImGui::Unindent(10.0f);
            }

            // Color section
            if (ImGui::CollapsingHeader("Colors", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Indent(10.0f);

                ImGui::Text("Main Color");
                ImGui::SetNextItemWidth(-1);
                ImGui::ColorEdit4("##MainCol", &selected.mainColor.x, ImGuiColorEditFlags_AlphaBar);

                ImGui::Spacing();
                ImGui::Checkbox("Use Face Colors", &selected.useFaceColors);

                if (selected.useFaceColors) {
                    ImGui::Indent(10.0f);
                    const char* faceNames[] = { "Front (+Z)", "Back (-Z)", "Right (+X)", "Left (-X)", "Top (+Y)", "Bottom (-Y)" };

                    for (int f = 0; f < 6; f++) {
                        ImGui::PushID(f);
                        ImGui::SetNextItemWidth(120);
                        ImGui::ColorEdit4(faceNames[f], &selected.faceColors[f].x, ImGuiColorEditFlags_NoInputs);
                        ImGui::PopID();
                    }
                    ImGui::Unindent(10.0f);
                }

                ImGui::Unindent(10.0f);
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // === ACTIONS ===
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Actions");

            // Duplicate button
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.4f, 0.6f, 1.0f));
            if (ImGui::Button("Duplicate", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f - 5, 0))) {
                StaticMeshData duplicate = selected;
                duplicate.name = selected.name + "_copy";
                duplicate.position.x += 2.0f;
                duplicate.selected = false;
                static_shapes.push_back(duplicate);
            }
            ImGui::PopStyleColor();

            ImGui::SameLine();

            // Delete button
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.3f, 0.3f, 1.0f));
            if (ImGui::Button("Delete", ImVec2(-1, 0))) {
                static_shapes.erase(static_shapes.begin() + selectedIndex);
                selectedIndex = -1;
            }
            ImGui::PopStyleColor(2);

            // Focus camera button
            if (ImGui::Button("Focus Camera", ImVec2(-1, 0))) {
                mainCamera.position = selected.position + glm::vec3(0.0f, 2.0f, 5.0f);
                // Update camera to look at the object
            }
        }
        else {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Select a primitive to edit its properties");
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // === STATISTICS ===
        int totalTriangles = 0;
        int visibleCount = 0;
        for (const auto& shape : static_shapes) {
            totalTriangles += shape.mesh.indexCount / 3;
            if (shape.visible) visibleCount++;
        }

        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Statistics");
        ImGui::Text("Total: %zu | Visible: %d | Triangles: %d", static_shapes.size(), visibleCount, totalTriangles);
    }
    ImGui::End();

    ImGui::PopStyleColor(6);
}

// =============================================================================
// UNIFIED SCENE HIERARCHY PANEL - Professional UI combining spawner + list
// =============================================================================
void VulkanEngine::draw_scene_hierarchy_imgui() {
    // Shared static state between tabs
    static int selectedIndex = -1;
    static char searchBuffer[128] = "";
    static int filterType = 0;  // 0 = All types

    // Spawn settings (from spawner)
    static int selectedTab = 1;  // 0 = 2D, 1 = 3D
    static int selected2D = 0;
    static int selected3D = 0;
    static int primitiveCounter = 0;
    static glm::vec3 spawnPosition = glm::vec3(0.0f, 0.0f, -5.0f);
    static glm::vec3 spawnRotation = glm::vec3(0.0f);
    static glm::vec3 spawnScale = glm::vec3(1.0f);
    static glm::vec4 mainColor = glm::vec4(1.0f);
    static bool useFaceColors = false;
    static glm::vec4 faceColors[6] = {
        glm::vec4(1.0f, 0.3f, 0.3f, 1.0f),
        glm::vec4(0.3f, 1.0f, 0.3f, 1.0f),
        glm::vec4(0.3f, 0.3f, 1.0f, 1.0f),
        glm::vec4(1.0f, 1.0f, 0.3f, 1.0f),
        glm::vec4(1.0f, 0.3f, 1.0f, 1.0f),
        glm::vec4(0.3f, 1.0f, 1.0f, 1.0f),
    };

    // Main panel state
    static int mainTab = 0;  // 0 = Primitives, 1 = Scene Objects

    // Fixed window position on the LEFT side
    ImGuiIO& io = ImGui::GetIO();
    float leftPanelWidth = 320.0f;

    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(leftPanelWidth, io.DisplaySize.y), ImGuiCond_FirstUseEver);

    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse;

    if (ImGui::Begin("Scene Hierarchy", nullptr, windowFlags)) {

        // === MAIN TAB BAR ===
        if (ImGui::BeginTabBar("MainHierarchyTabs", ImGuiTabBarFlags_None)) {

            // ===============================================
            // PRIMITIVES TAB
            // ===============================================
            if (ImGui::BeginTabItem("Primitives")) {
                mainTab = 0;

                // === CREATE SECTION (Collapsible) ===
                ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.15f, 0.45f, 0.35f, 0.8f));
                ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.2f, 0.55f, 0.4f, 0.9f));
                ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.25f, 0.6f, 0.45f, 1.0f));

                if (ImGui::CollapsingHeader("+ Create Primitive", ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::Indent(8.0f);

                    // Shape type tabs
                    if (ImGui::BeginTabBar("ShapeTypeTabs", ImGuiTabBarFlags_FittingPolicyScroll)) {
                        if (ImGui::BeginTabItem("3D")) {
                            selectedTab = 1;
                            const char* shapes3D[] = { "Cube", "Sphere", "Cylinder", "Cone", "Capsule", "Torus" };
                            ImGui::SetNextItemWidth(-1);
                            ImGui::Combo("##Shape3D", &selected3D, shapes3D, IM_ARRAYSIZE(shapes3D));
                            ImGui::EndTabItem();
                        }
                        if (ImGui::BeginTabItem("2D")) {
                            selectedTab = 0;
                            const char* shapes2D[] = { "Triangle", "Plane", "Quad" };
                            ImGui::SetNextItemWidth(-1);
                            ImGui::Combo("##Shape2D", &selected2D, shapes2D, IM_ARRAYSIZE(shapes2D));
                            ImGui::EndTabItem();
                        }
                        ImGui::EndTabBar();
                    }

                    ImGui::Spacing();

                    // Transform
                    if (ImGui::TreeNode("Transform")) {
                        ImGui::SetNextItemWidth(-1);
                        ImGui::DragFloat3("Position", &spawnPosition.x, 0.1f);
                        glm::vec3 rotDeg = glm::degrees(spawnRotation);
                        ImGui::SetNextItemWidth(-1);
                        if (ImGui::DragFloat3("Rotation", &rotDeg.x, 1.0f)) {
                            spawnRotation = glm::radians(rotDeg);
                        }
                        ImGui::SetNextItemWidth(-1);
                        ImGui::DragFloat3("Scale", &spawnScale.x, 0.05f, 0.01f, 100.0f);
                        ImGui::TreePop();
                    }

                    // Color
                    if (ImGui::TreeNode("Color")) {
                        ImGui::SetNextItemWidth(-1);
                        ImGui::ColorEdit4("Main", &mainColor.x, ImGuiColorEditFlags_AlphaBar);
                        ImGui::Checkbox("Face Colors", &useFaceColors);
                        if (useFaceColors) {
                            const char* faces[] = { "Front", "Back", "Right", "Left", "Top", "Bottom" };
                            for (int i = 0; i < 6; i++) {
                                ImGui::PushID(i);
                                ImGui::ColorEdit4(faces[i], &faceColors[i].x, ImGuiColorEditFlags_NoInputs);
                                if (i % 2 == 0 && i < 5) ImGui::SameLine(150);
                                ImGui::PopID();
                            }
                        }
                        ImGui::TreePop();
                    }

                    ImGui::Spacing();

                    // ADD BUTTON
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.55f, 0.25f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.65f, 0.35f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.25f, 0.75f, 0.4f, 1.0f));

                    if (ImGui::Button("+ Add Primitive", ImVec2(-1, 32))) {
                        StaticMeshData newMesh;

                        const char* typeNames2D[] = { "Triangle", "Plane", "Quad" };
                        const char* typeNames3D[] = { "Cube", "Sphere", "Cylinder", "Cone", "Capsule", "Torus" };

                        if (selectedTab == 0) {
                            newMesh.name = std::string(typeNames2D[selected2D]) + "_" + std::to_string(++primitiveCounter);
                        } else {
                            newMesh.name = std::string(typeNames3D[selected3D]) + "_" + std::to_string(++primitiveCounter);
                        }

                        newMesh.position = spawnPosition;
                        newMesh.rotation = spawnRotation;
                        newMesh.scale = spawnScale;
                        newMesh.mainColor = mainColor;
                        newMesh.useFaceColors = useFaceColors;
                        for (int i = 0; i < 6; i++) newMesh.faceColors[i] = faceColors[i];

                        if (selectedTab == 0) {
                            switch (selected2D) {
                                case 0: newMesh.type = PrimitiveType::Triangle; newMesh.mesh = generate_triangle_mesh(); break;
                                case 1: case 2: newMesh.type = PrimitiveType::Plane; newMesh.mesh = generate_plane_mesh(); break;
                            }
                        } else {
                            switch (selected3D) {
                                case 0: newMesh.type = PrimitiveType::Cube; newMesh.mesh = generate_cube_mesh(); break;
                                case 1: newMesh.type = PrimitiveType::Sphere; newMesh.mesh = generate_sphere_mesh(); break;
                                case 2: newMesh.type = PrimitiveType::Cylinder; newMesh.mesh = generate_cylinder_mesh(); break;
                                case 3: newMesh.type = PrimitiveType::Cone; newMesh.mesh = generate_cone_mesh(); break;
                                case 4: newMesh.type = PrimitiveType::Capsule; newMesh.mesh = generate_capsule_mesh(); break;
                                case 5: newMesh.type = PrimitiveType::Torus; newMesh.mesh = generate_torus_mesh(); break;
                            }
                        }

                        newMesh.materialType = ShaderOnlyMaterial::DEFAULT;
                        newMesh.passType = MaterialPass::MainColor;
                        newMesh.visible = true;
                        newMesh.selected = false;

                        static_shapes.push_back(newMesh);
                        spawnPosition.x += 2.5f;
                    }
                    ImGui::PopStyleColor(3);

                    ImGui::Unindent(8.0f);
                }
                ImGui::PopStyleColor(3);

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // === SEARCH & FILTER ===
                ImGui::SetNextItemWidth(-80);
                ImGui::InputTextWithHint("##Search", "Search...", searchBuffer, sizeof(searchBuffer));
                ImGui::SameLine();
                const char* typeFilters[] = { "All", "Cube", "Sphere", "Cyl", "Cone", "Cap", "Tor", "Tri", "Pln" };
                ImGui::SetNextItemWidth(70);
                ImGui::Combo("##Filter", &filterType, typeFilters, IM_ARRAYSIZE(typeFilters));

                ImGui::Spacing();

                // === PRIMITIVES LIST ===
                float listHeight = ImGui::GetContentRegionAvail().y - 100;
                ImGui::BeginChild("PrimitivesList", ImVec2(0, listHeight), true);

                for (int i = 0; i < static_shapes.size(); ++i) {
                    StaticMeshData& mesh = static_shapes[i];

                    // Apply search filter
                    if (strlen(searchBuffer) > 0 && mesh.name.find(searchBuffer) == std::string::npos) continue;

                    // Apply type filter
                    if (filterType > 0) {
                        PrimitiveType targetType;
                        switch (filterType) {
                            case 1: targetType = PrimitiveType::Cube; break;
                            case 2: targetType = PrimitiveType::Sphere; break;
                            case 3: targetType = PrimitiveType::Cylinder; break;
                            case 4: targetType = PrimitiveType::Cone; break;
                            case 5: targetType = PrimitiveType::Capsule; break;
                            case 6: targetType = PrimitiveType::Torus; break;
                            case 7: targetType = PrimitiveType::Triangle; break;
                            case 8: targetType = PrimitiveType::Plane; break;
                            default: targetType = PrimitiveType::Cube;
                        }
                        if (mesh.type != targetType) continue;
                    }

                    ImGui::PushID(i);

                    // Visibility toggle
                    ImGui::PushStyleColor(ImGuiCol_CheckMark, mesh.visible ? ImVec4(0.3f, 0.9f, 0.3f, 1.0f) : ImVec4(0.5f, 0.5f, 0.5f, 0.5f));
                    ImGui::Checkbox("##V", &mesh.visible);
                    ImGui::PopStyleColor();
                    ImGui::SameLine();

                    // Selection
                    bool isSelected = (selectedIndex == i);
                    ImGuiSelectableFlags selFlags = ImGuiSelectableFlags_AllowDoubleClick;

                    if (isSelected) {
                        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.26f, 0.59f, 0.98f, 0.6f));
                    }

                    if (ImGui::Selectable(mesh.name.c_str(), isSelected, selFlags)) {
                        selectedIndex = i;
                        for (int j = 0; j < static_shapes.size(); j++) {
                            static_shapes[j].selected = (j == i);
                        }
                    }

                    if (isSelected) {
                        ImGui::PopStyleColor();
                    }

                    // Context menu
                    if (ImGui::BeginPopupContextItem()) {
                        if (ImGui::MenuItem("Duplicate")) {
                            StaticMeshData dup = mesh;
                            dup.name = mesh.name + "_copy";
                            dup.position.x += 2.0f;
                            dup.selected = false;
                            static_shapes.push_back(dup);
                        }
                        if (ImGui::MenuItem("Delete")) {
                            static_shapes.erase(static_shapes.begin() + i);
                            if (selectedIndex >= static_shapes.size()) selectedIndex = -1;
                        }
                        if (ImGui::MenuItem("Focus Camera")) {
                            mainCamera.position = mesh.position + glm::vec3(0, 2, 5);
                        }
                        ImGui::EndPopup();
                    }

                    ImGui::PopID();
                }

                ImGui::EndChild();

                // === STATISTICS BAR ===
                ImGui::Separator();
                int totalTris = 0, visCount = 0;
                for (const auto& s : static_shapes) {
                    totalTris += s.mesh.indexCount / 3;
                    if (s.visible) visCount++;
                }
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%zu items | %d visible | %d tris",
                    static_shapes.size(), visCount, totalTris);

                // Quick actions
                if (!static_shapes.empty()) {
                    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 80);
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.2f, 0.2f, 0.8f));
                    if (ImGui::SmallButton("Clear All")) {
                        static_shapes.clear();
                        selectedIndex = -1;
                        primitiveCounter = 0;
                    }
                    ImGui::PopStyleColor();
                }

                ImGui::EndTabItem();
            }

            // ===============================================
            // SCENE OBJECTS TAB (GLTF)
            // ===============================================
            if (ImGui::BeginTabItem("Scene Objects")) {
                mainTab = 1;

                ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "GLTF/GLB Scene Objects");
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::BeginChild("SceneObjectsList", ImVec2(0, 0), true);

                for (auto& [name, gltf] : loadedScenes) {
                    if (!gltf) continue;

                    if (ImGui::TreeNode(name.c_str())) {
                        for (auto& root : gltf->topNodes) {
                            draw_node_recursive_ui(root);
                        }
                        ImGui::TreePop();
                    }
                }

                if (loadedScenes.empty()) {
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No scene objects loaded");
                    ImGui::TextColored(ImVec4(0.4f, 0.4f, 0.4f, 1.0f), "Load a GLTF/GLB file to see objects here");
                }

                ImGui::EndChild();

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    }
    ImGui::End();

    // Store selected index for inspector
    if (selectedIndex >= 0 && selectedIndex < static_shapes.size()) {
        // The inspector will use this
    }
}

// =============================================================================
// INSPECTOR PANEL - Property editor for selected object
// =============================================================================
void VulkanEngine::draw_inspector_panel_imgui() {
    // Shared selection state (matches Scene Hierarchy)
    static int selectedIndex = -1;

    // Find currently selected primitive
    for (int i = 0; i < static_shapes.size(); i++) {
        if (static_shapes[i].selected) {
            selectedIndex = i;
            break;
        }
    }

    // Fixed window position on the RIGHT side
    ImGuiIO& io = ImGui::GetIO();
    float inspectorWidth = 300.0f;

    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - inspectorWidth, 0), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(inspectorWidth, io.DisplaySize.y * 0.6f), ImGuiCond_FirstUseEver);

    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse;

    if (ImGui::Begin("Inspector", nullptr, windowFlags)) {

        if (selectedIndex >= 0 && selectedIndex < static_shapes.size()) {
            StaticMeshData& sel = static_shapes[selectedIndex];

            // === HEADER ===
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", sel.name.c_str());

            // Type badge
            const char* typeNames[] = { "Cube", "Sphere", "Capsule", "Cylinder", "Plane", "Cone", "Torus", "Triangle" };
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "[%s]", typeNames[(int)sel.type]);

            ImGui::Separator();
            ImGui::Spacing();

            // === NAME ===
            char nameBuf[64];
            strncpy(nameBuf, sel.name.c_str(), sizeof(nameBuf));
            nameBuf[sizeof(nameBuf) - 1] = '\0';
            ImGui::Text("Name");
            ImGui::SetNextItemWidth(-1);
            if (ImGui::InputText("##Name", nameBuf, sizeof(nameBuf))) {
                sel.name = nameBuf;
            }

            ImGui::Spacing();

            // === TRANSFORM ===
            if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Indent(8.0f);

                ImGui::Text("Position");
                ImGui::SetNextItemWidth(-1);
                ImGui::DragFloat3("##Pos", &sel.position.x, 0.1f, -1000.0f, 1000.0f, "%.2f");

                ImGui::Text("Rotation");
                glm::vec3 rotDeg = glm::degrees(sel.rotation);
                ImGui::SetNextItemWidth(-1);
                if (ImGui::DragFloat3("##Rot", &rotDeg.x, 1.0f, -360.0f, 360.0f, "%.1f")) {
                    sel.rotation = glm::radians(rotDeg);
                }

                ImGui::Text("Scale");
                ImGui::SetNextItemWidth(-1);
                ImGui::DragFloat3("##Scale", &sel.scale.x, 0.05f, 0.01f, 100.0f, "%.2f");

                if (ImGui::Button("Reset Transform", ImVec2(-1, 0))) {
                    sel.position = glm::vec3(0, 0, -5);
                    sel.rotation = glm::vec3(0);
                    sel.scale = glm::vec3(1);
                }

                ImGui::Unindent(8.0f);
            }

            // === COLORS ===
            if (ImGui::CollapsingHeader("Colors", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Indent(8.0f);

                ImGui::Text("Main Color");
                ImGui::SetNextItemWidth(-1);
                ImGui::ColorEdit4("##MainCol", &sel.mainColor.x, ImGuiColorEditFlags_AlphaBar);

                ImGui::Spacing();
                ImGui::Checkbox("Use Face Colors", &sel.useFaceColors);

                if (sel.useFaceColors) {
                    ImGui::Spacing();
                    const char* faces[] = { "Front (+Z)", "Back (-Z)", "Right (+X)", "Left (-X)", "Top (+Y)", "Bottom (-Y)" };
                    for (int f = 0; f < 6; f++) {
                        ImGui::PushID(f);
                        ImGui::ColorEdit4(faces[f], &sel.faceColors[f].x, ImGuiColorEditFlags_NoInputs);
                        ImGui::PopID();
                    }
                }

                ImGui::Unindent(8.0f);
            }

            // === VISIBILITY ===
            if (ImGui::CollapsingHeader("Visibility")) {
                ImGui::Indent(8.0f);
                ImGui::Checkbox("Visible", &sel.visible);
                ImGui::Unindent(8.0f);
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // === ACTIONS ===
            // Duplicate
            if (ImGui::Button("Duplicate", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f - 4, 0))) {
                StaticMeshData dup = sel;
                dup.name = sel.name + "_copy";
                dup.position.x += 2.0f;
                dup.selected = false;
                static_shapes.push_back(dup);
            }
            ImGui::SameLine();

            // Delete
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.3f, 0.3f, 1.0f));
            if (ImGui::Button("Delete", ImVec2(-1, 0))) {
                static_shapes.erase(static_shapes.begin() + selectedIndex);
                selectedIndex = -1;
            }
            ImGui::PopStyleColor(2);

            // Focus camera
            if (ImGui::Button("Focus Camera", ImVec2(-1, 0))) {
                mainCamera.position = sel.position + glm::vec3(0, 2, 5);
            }

        } else {
            // No selection
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No object selected");
            ImGui::Spacing();
            ImGui::TextWrapped("Select a primitive from the Scene Hierarchy to view and edit its properties.");
        }
    }
    ImGui::End();
}








//
//void VulkanEngine::run()
//{
//    SDL_Event e;
//    bool bQuit = false;
//    bool isFullscreen = false;
//
//    while (!bQuit) {
//        auto start = std::chrono::system_clock::now();
//
//        while (SDL_PollEvent(&e) != 0) {
//            if (e.type == SDL_QUIT)
//                bQuit = true;
//
//            // F11 ile tam ekran
//            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_F11) {
//                isFullscreen = !isFullscreen;
//                if (isFullscreen) {
//                    SDL_SetWindowFullscreen(_window, SDL_WINDOW_FULLSCREEN_DESKTOP);
//                }
//                else {
//                    SDL_SetWindowFullscreen(_window, 0);
//                }
//                int w, h;
//                SDL_GetWindowSize(_window, &w, &h);
//                fmt::print("F11 ile tam ekran geçişi: pencere boyutu {}x{}\n", w, h);
//                resize_requested = true;
//            }
//
//
//            if (e.type == SDL_WINDOWEVENT) {
//                if (e.window.event == SDL_WINDOWEVENT_RESIZED) resize_requested = true;
//                if (e.window.event == SDL_WINDOWEVENT_MINIMIZED) freeze_rendering = true;
//                if (e.window.event == SDL_WINDOWEVENT_RESTORED) freeze_rendering = false;
//            }
//
//            mainCamera.processSDLEvent(e);
//            ImGui_ImplSDL2_ProcessEvent(&e);
//        }
//
//        if (freeze_rendering) continue;
//
//        if (resize_requested) {
//            resize_swapchain();
//            fmt::print("drawExtent: {}x{}\n", _drawExtent.width, _drawExtent.height);
//        }
//
//
//        ImGui_ImplVulkan_NewFrame();
//        ImGui_ImplSDL2_NewFrame();
//        ImGui::NewFrame();
//
//        ImGui::Begin("Stats");
//        ImGui::Text("Frametime: %.2f ms", stats.frametime);
//        ImGui::Text("Scene Update: %.2f ms", stats.scene_update_time);
//        ImGui::Text("Mesh Draw Time: %.2f ms", stats.mesh_draw_time);
//        ImGui::Text("Triangles: %d", stats.triangle_count);
//        ImGui::Text("Draw Calls: %d", stats.drawcall_count);
//        ImGui::Text("Visible Objects: %d", stats.visible_count);
//        ImGui::Text("Shader Pipelines: %d", stats.shader_count);
//
//        if (ImGui::CollapsingHeader("Visible Objects")) {
//            for (const auto& objName : stats.visibleObjects) {
//                ImGui::Text("%s", objName.c_str());
//            }
//        }
//
//        if (ImGui::CollapsingHeader("Shaders")) {
//            for (const auto& shaderName : stats.shaderNames) {
//                ImGui::Text("%s", shaderName.c_str());
//            }
//        }
//        ImGui::End();
//
//        if (ImGui::Begin("background")) {
//            ComputeEffect& selected = backgroundEffects[currentBackgroundEffect];
//            ImGui::Text("Selected effect: %s", selected.name);
//            ImGui::SliderInt("Effect Index", &currentBackgroundEffect, 0, backgroundEffects.size() - 1);
//            ImGui::InputFloat4("data1", (float*)&selected.data.data1);
//            ImGui::InputFloat4("data2", (float*)&selected.data.data2);
//            ImGui::InputFloat4("data3", (float*)&selected.data.data3);
//            ImGui::InputFloat4("data4", (float*)&selected.data.data4);
//            ImGui::End();
//        }
//        ImGui_ImplVulkan_NewFrame();
//        ImGui_ImplSDL2_NewFrame();
//        ImGui::NewFrame();
//
//        update_imgui();  // 👈 buraya ekle
//
//        ImGui::Render();
//
//
//
//        draw_pipeline_settings_imgui();    // ⬅️ Bunu ekle
//        draw_primitive_spawner_imgui();
//        draw_static_mesh_imgui();
//        ImGui::Render();
//
//        update_scene();
//        draw();
//
//        auto end = std::chrono::system_clock::now();
//        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
//        stats.frametime = elapsed.count() / 1000.f;
//    }
//
//    // ✅ PROGRAM SONU: KAYNAKLARI TEMİZLE
//    cleanup();  // Bunu mutlaka ekle!
//}
//void VulkanEngine::draw_scene_light_imgui() {
//    if (ImGui::Begin("☀ Işık Ayarları")) {
//
//        //// 🔆 Ambient Light
//        //ImGui::Text("Ambient (Ortam Işığı)");
//        //ImGui::ColorEdit3("Renk", (float*)&sceneData.ambientColor);
//        //ImGui::SliderFloat("Güç", &sceneData.ambientColor.w, 0.0f, 2.0f);
//        //ImGui::Separator();
//
//        //// 🌞 Directional (Güneş) Light
//        //ImGui::Text("Güneş Işığı");
//        //ImGui::ColorEdit3("Güneş Rengi", (float*)&sceneData.sunlightColor);
//        //ImGui::SliderFloat("Güneş Gücü", &sceneData.sunlightDirection.w, 0.0f, 10.0f);
//
//        //glm::vec3 dir = glm::vec3(sceneData.sunlightDirection);
//        //if (ImGui::SliderFloat3("Güneş Yönü", (float*)&dir, -1.0f, 1.0f)) {
//        //    sceneData.sunlightDirection = glm::vec4(glm::normalize(dir), sceneData.sunlightDirection.w);
//        //}
//
//        //ImGui::TextDisabled("Güneş yönü normalize edilir");
//        //ImGui::Separator();
//
//        // 💡 Noktasal Işıklar
//        ImGui::Text("Noktasal Işıklar");
//        if (ImGui::Button("Yeni Noktasal Işık Ekle") && scenePointLights.size() < MAX_POINT_LIGHTS) {
//            PointLight light;
//            light.position = glm::vec3(0.0f, 2.0f, -3.0f);
//            light.radius = 2.0f;
//            light.color = glm::vec3(1.0f, 1.0f, 0.9f);
//            light.intensity = 20.0f;
//            scenePointLights.push_back(light);
//        }
//
//        for (int i = 0; i < scenePointLights.size(); ++i) {
//            ImGui::PushID(i);
//            if (ImGui::TreeNode(fmt::format("💡 Noktasal Işık {}", i).c_str())) {
//                ImGui::DragFloat3("Pozisyon", (float*)&scenePointLights[i].position, 0.1f);
//                ImGui::ColorEdit3("Renk", (float*)&scenePointLights[i].color);
//                ImGui::SliderFloat("Güç", &scenePointLights[i].intensity, 0.0f, 100.0f);
//                ImGui::SliderFloat("Yarıçap", &scenePointLights[i].radius, 0.1f, 50.0f);
//
//                if (ImGui::Button("Sil")) {
//                    scenePointLights.erase(scenePointLights.begin() + i);
//                    ImGui::TreePop();
//                    ImGui::PopID();
//                    break; // Iterator geçersizleşmemesi için break
//                }
//
//                ImGui::TreePop();
//            }
//            ImGui::PopID();
//        }
//
//    }
//    ImGui::End();
//}







void VulkanEngine::draw_scene_light_imgui() {
    if (ImGui::Begin("☀ Işık Ayarları")) {

        // === Ambient Light ===
        ImGui::Text("Ambient (Ortam Işığı)");
        ImGui::ColorEdit3("Ambient Renk", (float*)&sceneData.ambientColor);
        ImGui::SliderFloat("Ambient Güç", &sceneData.ambientColor.w, 0.0f, 2.0f);
        ImGui::Separator();

        // === Directional Light ===
        ImGui::Text("Güneş (Directional Işık)");
        ImGui::ColorEdit3("Güneş Renk", (float*)&sceneData.sunlightColor);
        ImGui::SliderFloat("Güneş Güç", &sceneData.sunlightDirection.w, 0.0f, 10.0f);

        glm::vec3 sunDir = glm::vec3(sceneData.sunlightDirection);
        if (ImGui::DragFloat3("Güneş Yönü", (float*)&sunDir, 0.1f, -1.0f, 1.0f)) {
            sceneData.sunlightDirection = glm::vec4(glm::normalize(sunDir), sceneData.sunlightDirection.w);
        }
        ImGui::TextDisabled("Yön normalize edilmiştir");
        ImGui::Separator();

        // === Noktasal Işıklar ===
        ImGui::Text("Noktasal Işıklar");
        if (ImGui::Button("Yeni Noktasal Işık Ekle") && scenePointLights.size() < MAX_POINT_LIGHTS) {
            PointLight light;
            light.position = mainCamera.position + glm::vec3(0.0f, 1.0f, -2.0f); // Near camera
            light.radius = 10.0f;   // Larger radius for easier testing
            light.color = glm::vec3(1.0f, 0.9f, 0.7f); // Warm white
            light.intensity = 50.0f; // Strong intensity
            scenePointLights.push_back(light);
            sync_point_light_billboards();
            fmt::print("[PointLight] Added light at ({:.2f}, {:.2f}, {:.2f}) radius={:.2f} intensity={:.2f}\n",
                light.position.x, light.position.y, light.position.z, light.radius, light.intensity);
        }

        bool lightChanged = false;
        for (int i = 0; i < scenePointLights.size(); ++i) {
            ImGui::PushID(i);
            if (ImGui::TreeNode(fmt::format("💡 Noktasal Işık {}", i).c_str())) {
                lightChanged |= ImGui::DragFloat3("Pozisyon", (float*)&scenePointLights[i].position, 0.1f);
                lightChanged |= ImGui::ColorEdit3("Renk", (float*)&scenePointLights[i].color);
                lightChanged |= ImGui::SliderFloat("Güç", &scenePointLights[i].intensity, 0.0f, 100.0f);
                lightChanged |= ImGui::SliderFloat("Yarıçap", &scenePointLights[i].radius, 0.1f, 50.0f);

                if (ImGui::Button("Sil")) {
                    scenePointLights.erase(scenePointLights.begin() + i);
                    sync_point_light_billboards(); // Update visualization after delete
                    ImGui::TreePop();
                    ImGui::PopID();
                    break;
                }

                ImGui::TreePop();
            }
            ImGui::PopID();
        }
        // Sync visualization when any light property changes
        if (lightChanged) {
            sync_point_light_billboards();
        }

    }
    ImGui::End();
}
















//void VulkanEngine::draw_visible_objects_panel()
//{
//    if (ImGui::CollapsingHeader("Visible Objects")) {
//
//        if (!selectedObjectName.empty())
//            ImGui::Text("Seçili: %s", selectedObjectName.c_str());
//        else
//            ImGui::Text("Seçili: (hiçbiri)");
//
//        ImGui::Separator();
//
//        for (const auto& objName : stats.visibleObjects) {
//            bool isSelected = (selectedObjectName == objName);
//
//            if (ImGui::Selectable(objName.c_str(), isSelected)) {
//                selectedObjectName = objName;  // ✅ Seçim sadece kaydedilir
//            }
//        }
//    }
//}


//void VulkanEngine::run()
//{
//    SDL_Event e;
//    bool bQuit = false;
//    bool isFullscreen = false;
//
//    while (!bQuit) {
//        auto start = std::chrono::system_clock::now();
//
//        // SDL olaylarını al
//        while (SDL_PollEvent(&e) != 0) {
//            if (e.type == SDL_QUIT)
//                bQuit = true;
//
//
//
//
//            // F11 ile tam ekran geçişi
//            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_F11) {
//                isFullscreen = !isFullscreen;
//                if (isFullscreen) {
//                    SDL_SetWindowFullscreen(_window, SDL_WINDOW_FULLSCREEN_DESKTOP);
//                }
//                else {
//                    SDL_SetWindowFullscreen(_window, 0);
//                }
//                int w, h;
//                SDL_GetWindowSize(_window, &w, &h);
//                fmt::print("F11 ile tam ekran geçişi: pencere boyutu {}x{}\n", w, h);
//                resize_requested = true;
//            }
//
//            if (e.type == SDL_WINDOWEVENT) {
//                if (e.window.event == SDL_WINDOWEVENT_RESIZED) resize_requested = true;
//                if (e.window.event == SDL_WINDOWEVENT_MINIMIZED) freeze_rendering = true;
//                if (e.window.event == SDL_WINDOWEVENT_RESTORED) freeze_rendering = false;
//            }
//
//            mainCamera.processSDLEvent(e);
//            ImGui_ImplSDL2_ProcessEvent(&e);
//        }
//
//        if (freeze_rendering) continue;
//
//        if (resize_requested) {
//            resize_swapchain();
//            fmt::print("drawExtent: {}x{}\n", _drawExtent.width, _drawExtent.height);
//        }
//
//        // ImGui yeni frame hazırlığı
//        ImGui_ImplVulkan_NewFrame();
//        ImGui_ImplSDL2_NewFrame();
//        ImGui::NewFrame();
//
//        // ImGui arayüzlerini oluştur (stats, background, gizmo, vs.)
//        ImGui::Begin("Stats");
//        ImGui::Text("Frametime: %.2f ms", stats.frametime);
//        ImGui::Text("Scene Update: %.2f ms", stats.scene_update_time);
//        ImGui::Text("Mesh Draw Time: %.2f ms", stats.mesh_draw_time);
//        ImGui::Text("Triangles: %d", stats.triangle_count);
//        ImGui::Text("Draw Calls: %d", stats.drawcall_count);
//        ImGui::Text("Visible Objects: %d", stats.visible_count);
//        ImGui::Text("Shader Pipelines: %d", stats.shader_count);
//
//        if (ImGui::CollapsingHeader("Visible Objects")) {
//
//            // ✅ Liste başında seçim bilgisini göster
//            if (!selectedObjectName.empty()) {
//                ImGui::Text("Seçili: %s", selectedObjectName.c_str());
//            }
//            else {
//                ImGui::Text("Seçili: (hiçbiri)");
//            }
//
//            ImGui::Separator();
//
//            for (const auto& objName : stats.visibleObjects) {
//                bool isSelected = (selectedObjectName == objName);
//
//                if (ImGui::Selectable(objName.c_str(), isSelected)) {
//                    selectedObjectName = objName;
//
//                    // ✅ Tıklanan objeye karşılık gelen MeshNode sahnede bulunup seçilir:
//                    selectedNode = findNodeByName(objName);
//                }
//            }
//        }
//
//
//
//
//        if (ImGui::CollapsingHeader("Shaders")) {
//            for (const auto& shaderName : stats.shaderNames) {
//                ImGui::Text("%s", shaderName.c_str());
//            }
//        }
//        ImGui::End();
//
//        if (ImGui::Begin("background")) {
//            ComputeEffect& selected = backgroundEffects[currentBackgroundEffect];
//            ImGui::Text("Selected effect: %s", selected.name);
//            ImGui::SliderInt("Effect Index", &currentBackgroundEffect, 0, static_cast<int>(backgroundEffects.size()) - 1);
//            ImGui::InputFloat4("data1", (float*)&selected.data.data1);
//            ImGui::InputFloat4("data2", (float*)&selected.data.data2);
//            ImGui::InputFloat4("data3", (float*)&selected.data.data3);
//            ImGui::InputFloat4("data4", (float*)&selected.data.data4);
//            ImGui::End();
//        }
//
//        // Kullanıcı tanımlı arayüz panelleri burada
//        update_imgui(); // draw_node_selector, draw_node_gizmo, vs.
//
//        // ImGui render datasını oluştur
//        ImGui::Render();
//
//        // Sahne güncelle ve çiz
//        update_scene(); // logic, hareket vs.
//        draw();         // içinde draw_imgui(cmd, imageView) çağrılır
//
//        auto end = std::chrono::system_clock::now();
//        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
//        stats.frametime = elapsed.count() / 1000.f;
//    }
//
//    cleanup(); // tüm Vulkan kaynaklarını temizle
//}
void VulkanEngine::run()
{
    SDL_Event e;
    bool bQuit = false;
    bool isFullscreen = false;

    while (!bQuit)
    {
        auto start = std::chrono::system_clock::now();

        while (SDL_PollEvent(&e) != 0)
        {
            if (e.type == SDL_QUIT) {
                // Wait for GPU to finish before quitting to prevent crashes
                vkDeviceWaitIdle(_device);
                bQuit = true;
                break; // Exit event loop immediately
            }

            // Skip processing other events if we're quitting
            if (bQuit) break;

            // ✅ Sol tıklama ile obje seçme (ray picking)
            // Only select objects when clicking outside UI panels
            if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT && !ImGui::GetIO().WantCaptureMouse)
            {
                int mouseX, mouseY;
                SDL_GetMouseState(&mouseX, &mouseY);

                int winW, winH;
                SDL_GetWindowSize(_window, &winW, &winH);

                float mouseX_norm = static_cast<float>(mouseX) / static_cast<float>(winW);
                float mouseY_norm = static_cast<float>(mouseY) / static_cast<float>(winH);

                select_object_under_mouse(mouseX_norm, mouseY_norm);
            }
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
            {
                if (_showOutline)
                {
                    selectedNode = nullptr;

                    _showOutline = false;
                    fmt::print("Outline modu ESC ile devre disi birakildi.\n");
                }
            }
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_F11)
            {
                isFullscreen = !isFullscreen;
                SDL_SetWindowFullscreen(_window, isFullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
                resize_requested = true;
            }

            if (e.type == SDL_WINDOWEVENT)
            {
                if (e.window.event == SDL_WINDOWEVENT_RESIZED) resize_requested = true;
                if (e.window.event == SDL_WINDOWEVENT_MINIMIZED) freeze_rendering = true;
                if (e.window.event == SDL_WINDOWEVENT_RESTORED) freeze_rendering = false;
                // Handle focus loss (e.g., Command+Tab on macOS)
                if (e.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
                    // Wait for GPU to finish current work when losing focus
                    vkDeviceWaitIdle(_device);
                }
            }

            mainCamera.processSDLEvent(e);
            ImGui_ImplSDL2_ProcessEvent(&e);
        }

        if (freeze_rendering)
            continue;

        if (resize_requested)
        {
            resize_swapchain();
        }

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // All UI is now handled by the professional panel system
        update_imgui();
        ImGui::Render();

        update_scene();
        draw();

        auto end = std::chrono::system_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        stats.frametime = elapsed.count() / 1000.f;
    }

    cleanup();
}



//void VulkanEngine::update_scene() {
//    mainCamera.update();
//
//    glm::mat4 view = mainCamera.getViewMatrix();
//    glm::mat4 projection = glm::mat4(0.0f);
//    float fov = glm::radians(70.0f);
//    float aspect = (float)_drawExtent.width / (float)_drawExtent.height;
//    float f = 1.0f / tan(fov / 2.0f);
//    float nearPlane = 0.1f;
//    float farPlane = 10000.0f;
//
//    projection[0][0] = f / aspect;
//    projection[1][1] = -f;  // Y'yi ters çevir
//    projection[2][2] = 0.0f;
//    projection[2][3] = -1.0f;
//    projection[3][2] = nearPlane;
//
//    sceneData.view = view;
//    sceneData.proj = projection;
//    sceneData.viewproj = projection * view;
//
//    drawCommands.viewproj = sceneData.viewproj;
//
//    // **🚀 İstatistikleri sıfırla**
//    stats.triangle_count = 0;
//    stats.drawcall_count = 0;
//    stats.visible_count = 0;
//    stats.shader_count = 0;
//
//    // **🚀 Shader pipeline'larını benzersiz saklamak için unordered_set**
//    std::unordered_set<VkPipeline> uniqueShaders;
//    std::vector<std::string> visibleObjects; // ✅ Görünen objelerin adları
//    std::vector<std::string> shaderNames;    // ✅ Shader pipeline isimleri
//
//    // **Sahneyi çiz**
//    for (const auto& [name, scene] : loadedScenes) {
//        scene->Draw(glm::mat4{ 1.f }, drawCommands);
//    }
//
//    for (const auto& obj : drawCommands.OpaqueSurfaces) {
//        if (obj.material && obj.material->pipeline) {
//            uniqueShaders.insert(obj.material->pipeline->pipeline);
//            shaderNames.push_back(obj.material->pipeline->name);  // ✅ Shader adını kaydet
//        }
//        visibleObjects.push_back(obj.name);  // ✅ Objeyi listeye ekle
//        stats.triangle_count += obj.indexCount / 3;
//        stats.drawcall_count++;
//        stats.visible_count++;
//    }
//
//    for (const auto& obj : drawCommands.TransparentSurfaces) {
//        if (obj.material && obj.material->pipeline) {
//            uniqueShaders.insert(obj.material->pipeline->pipeline);
//            shaderNames.push_back(obj.material->pipeline->name);  // ✅ Shader adını kaydet
//        }
//        visibleObjects.push_back(obj.name);  // ✅ Objeyi listeye ekle
//        stats.triangle_count += obj.indexCount / 3;
//        stats.drawcall_count++;
//        stats.visible_count++;
//    }
//
//    stats.shader_count = static_cast<int>(uniqueShaders.size());
//
//
//    // for (int i = 0; i < 16; i++)         {
//    loadedScenes["structure"]->Draw(glm::mat4{ 1.f }, drawCommands);
//    //}
//
//    // **🚀 Shader ve objelerin adlarını ImGui'ye aktarmak için kaydet**
//    stats.visibleObjects = visibleObjects;
//    stats.shaderNames = shaderNames;
//}
//void VulkanEngine::update_scene() {
//    mainCamera.update();
//
//    glm::mat4 view = mainCamera.getViewMatrix();
//    glm::mat4 projection = glm::mat4(0.0f);
//    float fov = glm::radians(70.0f);
//    float aspect = (float)_drawExtent.width / (float)_drawExtent.height;
//    float f = 1.0f / tan(fov / 2.0f);
//    float nearPlane = 0.1f;
//    float farPlane = 10000.0f;
//
//    projection[0][0] = f / aspect;
//    projection[1][1] = -f;  // Y'yi ters çevir
//    projection[2][2] = 0.0f;
//    projection[2][3] = -1.0f;
//    projection[3][2] = nearPlane;
//
//    sceneData.view = view;
//    sceneData.proj = projection;
//    sceneData.viewproj = projection * view;
//
//    drawCommands.viewproj = sceneData.viewproj;
//
//    // 🚀 İstatistikleri sıfırla
//    stats.triangle_count = 0;
//    stats.drawcall_count = 0;
//    stats.visible_count = 0;
//    stats.shader_count = 0;
//
//    std::unordered_set<VkPipeline> uniqueShaders;
//    std::vector<std::string> visibleObjects;
//    std::vector<std::string> shaderNames;
//
//    // 🚀 Sahnede yüklü tüm objeleri çiz
//    for (const auto& [name, scene] : loadedScenes) {
//        scene->Draw(glm::mat4{ 1.f }, drawCommands);
//    }
//
//    for (const auto& obj : drawCommands.OpaqueSurfaces) {
//        if (obj.material && obj.material->pipeline) {
//            uniqueShaders.insert(obj.material->pipeline->pipeline);
//            shaderNames.push_back(obj.material->pipeline->name);
//        }
//        visibleObjects.push_back(obj.name);
//        stats.triangle_count += obj.indexCount / 3;
//        stats.drawcall_count++;
//        stats.visible_count++;
//    }
//
//    for (const auto& obj : drawCommands.TransparentSurfaces) {
//        if (obj.material && obj.material->pipeline) {
//            uniqueShaders.insert(obj.material->pipeline->pipeline);
//            shaderNames.push_back(obj.material->pipeline->name);
//        }
//        visibleObjects.push_back(obj.name);
//        stats.triangle_count += obj.indexCount / 3;
//        stats.drawcall_count++;
//        stats.visible_count++;
//    }
//
//    stats.shader_count = static_cast<int>(uniqueShaders.size());
//    stats.visibleObjects = visibleObjects;
//    stats.shaderNames = shaderNames;
//}
//void VulkanEngine::update_scene() {
//    mainCamera.update();
//
//    glm::mat4 view = mainCamera.getViewMatrix();
//
//    // Letterbox viewport ile aspect oranı
//    VkViewport viewport = get_letterbox_viewport();
//    float aspect = viewport.width / viewport.height;
//
//    float fov = glm::radians(70.0f);
//    float nearPlane = 0.1f;
//    float farPlane = 10000.0f;
//
//    // Reverse-Z perspective
//    glm::mat4 projection = glm::perspectiveRH_ZO(fov, aspect, farPlane, nearPlane);
//    projection[1][1] *= -1; // Vulkan için Y eksenini çevir
//
//    sceneData.view = view;
//    sceneData.proj = projection;
//    sceneData.viewproj = projection * view;
//
//    drawCommands.viewproj = sceneData.viewproj;
//
//    // İstatistikleri sıfırla
//    stats.triangle_count = 0;
//    stats.drawcall_count = 0;
//    stats.visible_count = 0;
//    stats.shader_count = 0;
//
//    std::unordered_set<VkPipeline> uniqueShaders;
//    std::vector<std::string> visibleObjects;
//    std::vector<std::string> shaderNames;
//
//    // Sahnede yüklü tüm objeleri çiz
//    for (const auto& [name, scene] : loadedScenes) {
//        scene->Draw(glm::mat4{ 1.f }, drawCommands);
//    }
//
//    for (const auto& obj : drawCommands.OpaqueSurfaces) {
//        if (obj.material && obj.material->pipeline) {
//            uniqueShaders.insert(obj.material->pipeline->pipeline);
//            shaderNames.push_back(obj.material->pipeline->name);
//        }
//        visibleObjects.push_back(obj.name);
//        stats.triangle_count += obj.indexCount / 3;
//        stats.drawcall_count++;
//        stats.visible_count++;
//    }
//
//    for (const auto& obj : drawCommands.TransparentSurfaces) {
//        if (obj.material && obj.material->pipeline) {
//            uniqueShaders.insert(obj.material->pipeline->pipeline);
//            shaderNames.push_back(obj.material->pipeline->name);
//        }
//        visibleObjects.push_back(obj.name);
//        stats.triangle_count += obj.indexCount / 3;
//        stats.drawcall_count++;
//        stats.visible_count++;
//    }
//
//    stats.shader_count = static_cast<int>(uniqueShaders.size());
//    stats.visibleObjects = visibleObjects;
//    stats.shaderNames = shaderNames;
//}

//void VulkanEngine::update_scene() {
//    mainCamera.update();
//
//    glm::mat4 view = mainCamera.getViewMatrix();
//
//    // Letterbox viewport ile aspect oranı
//    VkViewport viewport = get_letterbox_viewport();
//    float aspect = viewport.width / viewport.height;
//
//    float fov = glm::radians(70.0f);
//    float nearPlane = 0.1f;
//    float farPlane = 10000.0f;
//
//    // Reverse-Z perspective
//    glm::mat4 projection = glm::perspectiveRH_ZO(fov, aspect, farPlane, nearPlane);
//    projection[1][1] *= -1;
//
//    sceneData.view = view;
//    sceneData.proj = projection;
//    sceneData.viewproj = projection * view;
//
//    drawCommands.viewproj = sceneData.viewproj;
//
//    // İstatistikleri sıfırla
//    stats.triangle_count = 0;
//    stats.drawcall_count = 0;
//    stats.visible_count = 0;
//    stats.shader_count = 0;
//
//    std::unordered_set<VkPipeline> uniqueShaders;
//    std::vector<std::string> visibleObjects;
//    std::vector<std::string> shaderNames;
//
//    for (const auto& [name, scene] : loadedScenes) {
//        scene->Draw(glm::mat4{ 1.f }, drawCommands);
//    }
//
//    for (const auto& obj : drawCommands.OpaqueSurfaces) {
//        if (obj.material && obj.material->pipeline) {
//            uniqueShaders.insert(obj.material->pipeline->pipeline);
//            shaderNames.push_back(obj.material->pipeline->name);
//        }
//        visibleObjects.push_back(obj.name);
//        stats.triangle_count += obj.indexCount / 3;
//        stats.drawcall_count++;
//        stats.visible_count++;
//    }
//
//    for (const auto& obj : drawCommands.TransparentSurfaces) {
//        if (obj.material && obj.material->pipeline) {
//            uniqueShaders.insert(obj.material->pipeline->pipeline);
//            shaderNames.push_back(obj.material->pipeline->name);
//        }
//        visibleObjects.push_back(obj.name);
//        stats.triangle_count += obj.indexCount / 3;
//        stats.drawcall_count++;
//        stats.visible_count++;
//    }
//
//    stats.shader_count = static_cast<int>(uniqueShaders.size());
//    stats.visibleObjects = visibleObjects;
//    stats.shaderNames = shaderNames;
//
//    // ❌ BURADA ARTIK KAMERA TAKİBİ YOK
//}



//void VulkanEngine::update_scene() {
//    mainCamera.update();
//
//    glm::mat4 view = mainCamera.getViewMatrix();
//
//    // Letterbox viewport ile aspect oranı
//    VkViewport viewport = get_letterbox_viewport();
//    float aspect = viewport.width / viewport.height;
//
//    float fov = glm::radians(70.0f);
//    float nearPlane = 0.1f;
//    float farPlane = 10000.0f;
//
//    // Reverse-Z perspective
//    glm::mat4 projection = glm::perspectiveRH_ZO(fov, aspect, farPlane, nearPlane);
//    projection[1][1] *= -1;
//
//    sceneData.view = view;
//    sceneData.proj = projection;
//    sceneData.viewproj = projection * view;
//
//    drawCommands.viewproj = sceneData.viewproj;
//
//    // ✅ IŞIK VERİSİ EKLE
//    sceneData.ambientColor = glm::vec4(0.1f, 0.1f, 0.1f, 0.5f); // RGB + intensity
//    glm::vec3 sunDir = glm::normalize(glm::vec3(-1.0f, -3.0f, -1.0f));
//    sceneData.sunlightDirection = glm::vec4(sunDir, 3.0f);      // yön + intensity
//    sceneData.sunlightColor = glm::vec4(1.0f, 0.95f, 0.8f, 1.0f); // sıcak beyaz
//
//    // ✅ sceneDataBuffer'a yaz (mapped pointer ile)
//    AllocatedBuffer& buf = get_current_frame().sceneDataBuffer;
//    if (buf.allocation != VK_NULL_HANDLE && buf.info.pMappedData != nullptr) {
//        memcpy(buf.info.pMappedData, &sceneData, sizeof(GPUSceneData));
//    }
//    else {
//        fmt::print("[ERROR] sceneDataBuffer allocation is null or unmapped!\n");
//    }
//
//    // 🔄 İstatistikleri sıfırla
//    stats.triangle_count = 0;
//    stats.drawcall_count = 0;
//    stats.visible_count = 0;
//    stats.shader_count = 0;
//
//    std::unordered_set<VkPipeline> uniqueShaders;
//    std::vector<std::string> visibleObjects;
//    std::vector<std::string> shaderNames;
//
//    for (const auto& [name, scene] : loadedScenes) {
//        scene->Draw(glm::mat4{ 1.f }, drawCommands);
//    }
//
//    for (const auto& obj : drawCommands.OpaqueSurfaces) {
//        if (obj.material && obj.material->pipeline) {
//            uniqueShaders.insert(obj.material->pipeline->pipeline);
//            shaderNames.push_back(obj.material->pipeline->name);
//        }
//        visibleObjects.push_back(obj.name);
//        stats.triangle_count += obj.indexCount / 3;
//        stats.drawcall_count++;
//        stats.visible_count++;
//    }
//
//    for (const auto& obj : drawCommands.TransparentSurfaces) {
//        if (obj.material && obj.material->pipeline) {
//            uniqueShaders.insert(obj.material->pipeline->pipeline);
//            shaderNames.push_back(obj.material->pipeline->name);
//        }
//        visibleObjects.push_back(obj.name);
//        stats.triangle_count += obj.indexCount / 3;
//        stats.drawcall_count++;
//        stats.visible_count++;
//    }
//
//
//
//    stats.shader_count = static_cast<int>(uniqueShaders.size());
//    stats.visibleObjects = visibleObjects;
//    stats.shaderNames = shaderNames;
//}
//
//

//void VulkanEngine::update_scene() {
//    mainCamera.update();
//
//    glm::mat4 view = mainCamera.getViewMatrix();
//
//    // Letterbox viewport ile aspect oranı
//    VkViewport viewport = get_letterbox_viewport();
//    float aspect = viewport.width / viewport.height;
//
//    float fov = glm::radians(70.0f);
//    float nearPlane = 0.1f;
//    float farPlane = 10000.0f;
//
//    // Reverse-Z perspective
//    glm::mat4 projection = glm::perspectiveRH_ZO(fov, aspect, farPlane, nearPlane);
//    projection[1][1] *= -1;
//
//    // === Camera Matrices ===
//    sceneData.view = view;
//    sceneData.proj = projection;
//    sceneData.viewproj = projection * view;
//
//    drawCommands.viewproj = sceneData.viewproj;
//
//    // === Directional + Ambient Light ===
//    sceneData.ambientColor = glm::vec4(0.1f, 0.1f, 0.1f, 0.5f); // RGB + intensity
//    glm::vec3 sunDir = glm::normalize(glm::vec3(-1.0f, -3.0f, -1.0f));
//    sceneData.sunlightDirection = glm::vec4(sunDir, 3.0f); // yön + intensity
//    sceneData.sunlightColor = glm::vec4(1.0f, 0.95f, 0.8f, 1.0f); // sıcak beyaz
//
//    // === Point Lights ===
//    sceneData.pointLightCount = 0;
//
//    for (const auto& light : scenePointLights) {
//        if (sceneData.pointLightCount >= MAX_POINT_LIGHTS) break;
//        sceneData.pointLights[sceneData.pointLightCount++] = light;
//    }
//
//    // === GPU SceneData Buffer'ına yaz ===
//    AllocatedBuffer& buf = get_current_frame().sceneDataBuffer;
//    if (buf.allocation != VK_NULL_HANDLE && buf.info.pMappedData != nullptr) {
//        memcpy(buf.info.pMappedData, &sceneData, sizeof(GPUSceneData));
//    }
//    else {
//        fmt::print("[ERROR] sceneDataBuffer allocation is null or unmapped!\n");
//    }
//
//    // === İstatistikleri Sıfırla ===
//    stats.triangle_count = 0;
//    stats.drawcall_count = 0;
//    stats.visible_count = 0;
//    stats.shader_count = 0;
//
//    std::unordered_set<VkPipeline> uniqueShaders;
//    std::vector<std::string> visibleObjects;
//    std::vector<std::string> shaderNames;
//
//    // === Sahneyi çizim için işleme ===
//    for (const auto& [name, scene] : loadedScenes) {
//        scene->Draw(glm::mat4{ 1.f }, drawCommands);
//    }
//
//    // === Opaque
//    for (const auto& obj : drawCommands.OpaqueSurfaces) {
//        if (obj.material && obj.material->pipeline) {
//            uniqueShaders.insert(obj.material->pipeline->pipeline);
//            shaderNames.push_back(obj.material->pipeline->name);
//        }
//        visibleObjects.push_back(obj.name);
//        stats.triangle_count += obj.indexCount / 3;
//        stats.drawcall_count++;
//        stats.visible_count++;
//    }
//    // === Noktasal Işıkları Temsil Eden Küre Meshleri ===
//    for (const auto& light : scenePointLights) {
//        if (sceneData.pointLightCount >= MAX_POINT_LIGHTS) break;
//        sceneData.pointLights[sceneData.pointLightCount++] = light;
//
//        // Işık pozisyonuna küre ekle
//        StaticMeshData sphere;
//        sphere.mesh = generate_sphere_mesh(); // veya önceden üretilmiş sphere mesh
//        sphere.position = light.position;
//        sphere.scale = glm::vec3(0.25f);
//        sphere.materialType = ShaderOnlyMaterial::EMISSIVE;
//        sphere.passType = MaterialPass::MainColor;
//
//        static_shapes.push_back(sphere);
//    }
//
//
//    // === Transparent
//    for (const auto& obj : drawCommands.TransparentSurfaces) {
//        if (obj.material && obj.material->pipeline) {
//            uniqueShaders.insert(obj.material->pipeline->pipeline);
//            shaderNames.push_back(obj.material->pipeline->name);
//        }
//        visibleObjects.push_back(obj.name);
//        stats.triangle_count += obj.indexCount / 3;
//        stats.drawcall_count++;
//        stats.visible_count++;
//    }
//
//    stats.shader_count = static_cast<int>(uniqueShaders.size());
//    stats.visibleObjects = visibleObjects;
//    stats.shaderNames = shaderNames;
//}



//void VulkanEngine::update_scene() {
//    mainCamera.update();
//
//    glm::mat4 view = mainCamera.getViewMatrix();
//
//    // Letterbox viewport ile aspect oranı
//    VkViewport viewport = get_letterbox_viewport();
//    float aspect = viewport.width / viewport.height;
//
//    float fov = glm::radians(70.0f);
//    float nearPlane = 0.1f;
//    float farPlane = 10000.0f;
//
//    // Reverse-Z perspective
//    glm::mat4 projection = glm::perspectiveRH_ZO(fov, aspect, farPlane, nearPlane);
//    projection[1][1] *= -1;
//
//    // === Camera Matrices ===
//    sceneData.view = view;
//    sceneData.proj = projection;
//    sceneData.viewproj = projection * view;
//
//    drawCommands.viewproj = sceneData.viewproj;
//
//    // === Directional + Ambient Light ===
//    sceneData.ambientColor = glm::vec4(0.1f, 0.1f, 0.1f, 0.5f); // RGB + intensity
//    glm::vec3 sunDir = glm::normalize(glm::vec3(-1.0f, -3.0f, -1.0f));
//    sceneData.sunlightDirection = glm::vec4(sunDir, 3.0f); // yön + intensity
//    sceneData.sunlightColor = glm::vec4(1.0f, 0.95f, 0.8f, 1.0f); // sıcak beyaz
//
//    // === Point Lights ===
//    sceneData.pointLightCount = 0;
//
//    for (const auto& light : scenePointLights) {
//        if (sceneData.pointLightCount >= MAX_POINT_LIGHTS) break;
//        sceneData.pointLights[sceneData.pointLightCount++] = light;
//    }
//
//    // === GPU SceneData Buffer'ına yaz ===
//    AllocatedBuffer& buf = get_current_frame().sceneDataBuffer;
//    if (buf.allocation != VK_NULL_HANDLE && buf.info.pMappedData != nullptr) {
//        memcpy(buf.info.pMappedData, &sceneData, sizeof(GPUSceneData));
//    }
//    else {
//        fmt::print("[ERROR] sceneDataBuffer allocation is null or unmapped!\n");
//    }
//
//    // === İstatistikleri Sıfırla ===
//    stats.triangle_count = 0;
//    stats.drawcall_count = 0;
//    stats.visible_count = 0;
//    stats.shader_count = 0;
//
//    std::unordered_set<VkPipeline> uniqueShaders;
//    std::vector<std::string> visibleObjects;
//    std::vector<std::string> shaderNames;
//
//    // === Sahneyi çizim için işleme ===
//    for (const auto& [name, scene] : loadedScenes) {
//        scene->Draw(glm::mat4{ 1.f }, drawCommands);
//    }
//
//    // === Opaque
//    for (const auto& obj : drawCommands.OpaqueSurfaces) {
//        if (obj.material && obj.material->pipeline) {
//            uniqueShaders.insert(obj.material->pipeline->pipeline);
//            shaderNames.push_back(obj.material->pipeline->name);
//        }
//        visibleObjects.push_back(obj.name);
//        stats.triangle_count += obj.indexCount / 3;
//        stats.drawcall_count++;
//        stats.visible_count++;
//    }
//
//    // === Noktasal Işıkları Temsil Eden Küre Meshleri ===
//    static bool lightMeshesAdded = false;
//    if (!lightMeshesAdded) {
//        for (const auto& light : scenePointLights) {
//            StaticMeshData sphere;
//            sphere.mesh = generate_sphere_mesh();
//            sphere.position = glm::vec3(light.position); // vec4 → vec3
//            sphere.scale = glm::vec3(0.25f);
//            sphere.materialType = ShaderOnlyMaterial::EMISSIVE;
//            sphere.passType = MaterialPass::MainColor;
//
//            static_shapes.push_back(sphere);
//        }
//        lightMeshesAdded = true;
//    }
//
//    // === Transparent
//    for (const auto& obj : drawCommands.TransparentSurfaces) {
//        if (obj.material && obj.material->pipeline) {
//            uniqueShaders.insert(obj.material->pipeline->pipeline);
//            shaderNames.push_back(obj.material->pipeline->name);
//        }
//        visibleObjects.push_back(obj.name);
//        stats.triangle_count += obj.indexCount / 3;
//        stats.drawcall_count++;
//        stats.visible_count++;
//    }
//
//    stats.shader_count = static_cast<int>(uniqueShaders.size());
//    stats.visibleObjects = visibleObjects;
//    stats.shaderNames = shaderNames;
//}


//void VulkanEngine::update_scene() {
//    mainCamera.update();
//
//    VkViewport viewport = get_letterbox_viewport();
//    float aspect = viewport.width / viewport.height;
//
//    glm::mat4 view = mainCamera.getViewMatrix();
//    glm::mat4 projection = glm::perspectiveRH_ZO(glm::radians(70.0f), aspect, 10000.0f, 0.1f);
//    projection[1][1] *= -1;
//
//    sceneData.view = view;
//    sceneData.proj = projection;
//    sceneData.viewproj = projection * view;
//
//    drawCommands.viewproj = sceneData.viewproj;
//
//    // === Ambient & Directional Light ===
//    sceneData.ambientColor = glm::vec4(0.1f, 0.1f, 0.1f, 0.5f);
//    glm::vec3 sunDir = glm::normalize(glm::vec3(-1.0f, -3.0f, -1.0f));
//    sceneData.sunlightDirection = glm::vec4(sunDir, 3.0f);
//    sceneData.sunlightColor = glm::vec4(1.0f, 0.95f, 0.8f, 1.0f);
//
//    // === Point Lights ===
//    sceneData.pointLightCount = 0;
//
//    for (const auto& light : scenePointLights) {
//        if (sceneData.pointLightCount >= MAX_POINT_LIGHTS) break;
//        sceneData.pointLights[sceneData.pointLightCount++] = light;
//
//        // Sahneye ışık küresi ekle
//        StaticMeshData lightSphere;
//        lightSphere.mesh = generate_sphere_mesh(); // önceden generate edildiğini varsayıyoruz
//        lightSphere.position = light.position;
//        lightSphere.scale = glm::vec3(0.25f);
//        lightSphere.type = PrimitiveType::Sphere;
//        lightSphere.materialType = ShaderOnlyMaterial::EMISSIVE;
//        lightSphere.passType = MaterialPass::MainColor;
//
//        static_shapes.push_back(lightSphere);
//    }
//
//    // === Scene Graph Traversal (objeleri çizime hazırlamak)
//    for (const auto& [name, scene] : loadedScenes) {
//        scene->Draw(glm::mat4(1.0f), drawCommands);
//    }
//
//    // === İstatistikler
//    stats.triangle_count = 0;
//    stats.drawcall_count = 0;
//    stats.visible_count = 0;
//    stats.shader_count = 0;
//    stats.visibleObjects.clear();
//    stats.shaderNames.clear();
//
//    std::unordered_set<VkPipeline> usedShaders;
//    for (const auto& obj : drawCommands.OpaqueSurfaces) {
//        stats.triangle_count += obj.indexCount / 3;
//        stats.drawcall_count++;
//        stats.visible_count++;
//        if (obj.material && obj.material->pipeline) {
//            usedShaders.insert(obj.material->pipeline->pipeline);
//            stats.shaderNames.push_back(obj.material->pipeline->name);
//        }
//        stats.visibleObjects.push_back(obj.name);
//    }
//
//    stats.shader_count = static_cast<int>(usedShaders.size());
//
//    // === SceneData buffer'a yaz
//    AllocatedBuffer& buf = get_current_frame().sceneDataBuffer;
//    if (buf.allocation && buf.info.pMappedData) {
//        memcpy(buf.info.pMappedData, &sceneData, sizeof(GPUSceneData));
//    }
//    else {
//        fmt::println("[ERROR] sceneDataBuffer allocation is null or unmapped!");
//    }
//}
//




//void VulkanEngine::update_scene() {
//    mainCamera.update();
//
//    glm::mat4 view = mainCamera.getViewMatrix();
//
//    // Letterbox viewport ile aspect oranı
//    VkViewport viewport = get_letterbox_viewport();
//    float aspect = viewport.width / viewport.height;
//
//    float fov = glm::radians(70.0f);
//    float nearPlane = 0.1f;
//    float farPlane = 10000.0f;
//
//    // Reverse-Z perspective
//    glm::mat4 projection = glm::perspectiveRH_ZO(fov, aspect, farPlane, nearPlane);
//    projection[1][1] *= -1;
//
//    // === Camera Matrices ===
//    sceneData.view = view;
//    sceneData.proj = projection;
//    sceneData.viewproj = projection * view;
//    drawCommands.viewproj = sceneData.viewproj;
//
//    // === Directional + Ambient Light ===
//    sceneData.ambientColor = glm::vec4(0.1f, 0.1f, 0.1f, 0.5f); // RGB + intensity
//    glm::vec3 sunDir = glm::normalize(glm::vec3(-1.0f, -3.0f, -1.0f));
//    sceneData.sunlightDirection = glm::vec4(sunDir, 3.0f); // yön + intensity
//    sceneData.sunlightColor = glm::vec4(1.0f, 0.95f, 0.8f, 1.0f); // sıcak beyaz
//
//    // === Point Lights ===
//    sceneData.pointLightCount = 0;
//
//    for (const auto& light : scenePointLights) {
//        if (sceneData.pointLightCount >= MAX_POINT_LIGHTS) break;
//        sceneData.pointLights[sceneData.pointLightCount++] = light;
//    }
//
//    // === GPU SceneData Buffer'ına yaz ===
//    AllocatedBuffer& buf = get_current_frame().sceneDataBuffer;
//    if (buf.allocation != VK_NULL_HANDLE && buf.info.pMappedData != nullptr) {
//        memcpy(buf.info.pMappedData, &sceneData, sizeof(GPUSceneData));
//    }
//    else {
//        fmt::print("[ERROR] sceneDataBuffer allocation is null or unmapped!\n");
//    }
//
//    // === İstatistikleri Sıfırla ===
//    stats.triangle_count = 0;
//    stats.drawcall_count = 0;
//    stats.visible_count = 0;
//    stats.shader_count = 0;
//
//    std::unordered_set<VkPipeline> uniqueShaders;
//    std::vector<std::string> visibleObjects;
//    std::vector<std::string> shaderNames;
//
//    // === Sahneyi çizim için işleme ===
//    for (const auto& [name, scene] : loadedScenes) {
//        scene->Draw(glm::mat4{ 1.f }, drawCommands);
//    }
//
//    // === Opaque
//    for (const auto& obj : drawCommands.OpaqueSurfaces) {
//        if (obj.material && obj.material->pipeline) {
//            uniqueShaders.insert(obj.material->pipeline->pipeline);
//            shaderNames.push_back(obj.material->pipeline->name);
//        }
//        visibleObjects.push_back(obj.name);
//        stats.triangle_count += obj.indexCount / 3;
//        stats.drawcall_count++;
//        stats.visible_count++;
//    }
//
//    // === Noktasal Işık Meshlerini Temizle ve Yeniden Oluştur ===
//    std::erase_if(static_shapes, [](const StaticMeshData& mesh) {
//        return mesh.materialType == ShaderOnlyMaterial::EMISSIVE;
//        });
//
//    for (const auto& light : scenePointLights) {
//        StaticMeshData sphere;
//        sphere.mesh = generate_sphere_mesh();  // dilersen cache'li hale getirebilirsin
//        sphere.position = glm::vec3(light.position);
//        sphere.scale = glm::vec3(0.25f);
//        sphere.materialType = ShaderOnlyMaterial::EMISSIVE;
//        sphere.passType = MaterialPass::MainColor;
//        static_shapes.push_back(sphere);
//    }
//
//    // === Transparent
//    for (const auto& obj : drawCommands.TransparentSurfaces) {
//        if (obj.material && obj.material->pipeline) {
//            uniqueShaders.insert(obj.material->pipeline->pipeline);
//            shaderNames.push_back(obj.material->pipeline->name);
//        }
//        visibleObjects.push_back(obj.name);
//        stats.triangle_count += obj.indexCount / 3;
//        stats.drawcall_count++;
//        stats.visible_count++;
//    }
//
//    stats.shader_count = static_cast<int>(uniqueShaders.size());
//    stats.visibleObjects = visibleObjects;
//    stats.shaderNames = shaderNames;
//}



void VulkanEngine::update_scene() {
    float dt = stats.frametime / 1000.f; // frametime is in ms, convert to seconds
    if (dt <= 0.f) dt = 1.f / 60.f;     // fallback for first frame
    mainCamera.update(dt);

    glm::mat4 view = mainCamera.getViewMatrix();

    // Letterbox viewport ile aspect oranı
    VkViewport viewport = get_letterbox_viewport();
    float aspect = viewport.width / viewport.height;

    float fov = glm::radians(mainCamera.fov);
    float nearPlane = mainCamera.nearPlane;
    float farPlane = mainCamera.farPlane;

    // Reverse-Z perspective
    glm::mat4 projection = glm::perspectiveRH_ZO(fov, aspect, farPlane, nearPlane);
    projection[1][1] *= -1;

    // === Camera Matrices ===
    sceneData.view = view;
    sceneData.proj = projection;
    sceneData.viewproj = projection * view;
    drawCommands.viewproj = sceneData.viewproj;

    // === Camera Position (for specular lighting) ===
    sceneData.cameraPosition = glm::vec4(mainCamera.position, 1.0f);

    // === Point Lights ===
    sceneData.pointLightCount = 0;
    for (const auto& light : scenePointLights) {
        if (sceneData.pointLightCount >= MAX_POINT_LIGHTS) break;
        sceneData.pointLights[sceneData.pointLightCount++] = light;
    }

    // === GPU SceneData Buffer'ına yaz ===
    AllocatedBuffer& buf = get_current_frame().sceneDataBuffer;
    if (buf.allocation != VK_NULL_HANDLE && buf.info.pMappedData != nullptr) {
        memcpy(buf.info.pMappedData, &sceneData, sizeof(GPUSceneData));
    }
    else {
        fmt::print("[ERROR] sceneDataBuffer allocation is null or unmapped!\n");
    }

    // === İstatistikleri Sıfırla ===
    stats.triangle_count = 0;
    stats.drawcall_count = 0;
    stats.visible_count = 0;
    stats.shader_count = 0;

    // Use static containers to avoid allocation every frame
    static std::unordered_set<VkPipeline> uniqueShaders;
    static std::vector<std::string> visibleObjects;
    static std::vector<std::string> shaderNames;
    uniqueShaders.clear();
    visibleObjects.clear();
    shaderNames.clear();

    // === Sahneyi çizim için işleme ===
    for (const auto& [name, scene] : loadedScenes) {
        scene->Draw(glm::mat4{ 1.f }, drawCommands);
    }

    // === Opaque
    for (const auto& obj : drawCommands.OpaqueSurfaces) {
        if (obj.material && obj.material->pipeline) {
            uniqueShaders.insert(obj.material->pipeline->pipeline);
            shaderNames.push_back(obj.material->pipeline->name);
        }
        visibleObjects.push_back(obj.name);
        stats.triangle_count += obj.indexCount / 3;
        stats.drawcall_count++;
        stats.visible_count++;
    }

    // === Noktasal Işık Meshlerini Güncelle (CACHED - no mesh generation per frame!) ===
    // Cache light sphere mesh on first use (using class member instead of static)
    if (!_lightMeshCached) {
        _cachedLightSphereMesh = generate_sphere_mesh();
        _lightMeshCached = true;
    }

    // Only update if light count changed
    static size_t lastLightCount = 0;
    if (scenePointLights.size() != lastLightCount) {
        std::erase_if(static_shapes, [](const StaticMeshData& mesh) {
            return mesh.materialType == ShaderOnlyMaterial::EMISSIVE;
        });

        for (const auto& light : scenePointLights) {
            StaticMeshData sphere;
            sphere.mesh = _cachedLightSphereMesh;  // Use cached mesh!
            sphere.position = glm::vec3(light.position);
            sphere.scale = glm::vec3(0.25f);
            sphere.materialType = ShaderOnlyMaterial::EMISSIVE;
            sphere.passType = MaterialPass::MainColor;
            static_shapes.push_back(sphere);
        }
        lastLightCount = scenePointLights.size();
    } else {
        // Just update positions for existing light spheres
        size_t lightIdx = 0;
        for (auto& shape : static_shapes) {
            if (shape.materialType == ShaderOnlyMaterial::EMISSIVE && lightIdx < scenePointLights.size()) {
                shape.position = glm::vec3(scenePointLights[lightIdx].position);
                lightIdx++;
            }
        }
    }

    // === Transparent
    for (const auto& obj : drawCommands.TransparentSurfaces) {
        if (obj.material && obj.material->pipeline) {
            uniqueShaders.insert(obj.material->pipeline->pipeline);
            shaderNames.push_back(obj.material->pipeline->name);
        }
        visibleObjects.push_back(obj.name);
        stats.triangle_count += obj.indexCount / 3;
        stats.drawcall_count++;
        stats.visible_count++;
    }

    stats.shader_count = static_cast<int>(uniqueShaders.size());
    stats.visibleObjects = visibleObjects;
    stats.shaderNames = shaderNames;
}

void VulkanEngine::sync_point_light_spheres() {
    // Önce eski ışık kürelerini temizle
    static_shapes.erase(std::remove_if(static_shapes.begin(), static_shapes.end(),
        [](const StaticMeshData& s) {
            return s.materialType == ShaderOnlyMaterial::EMISSIVE &&
                s.type == PrimitiveType::Sphere;
        }), static_shapes.end());

    // Her noktasal ışık için sahneye bir küre ekle
    for (const auto& light : scenePointLights) {
        StaticMeshData sphere;
        sphere.type = PrimitiveType::Sphere;
        sphere.materialType = ShaderOnlyMaterial::EMISSIVE;
        sphere.passType = MaterialPass::Transparent;
        sphere.position = light.position;
        sphere.scale = glm::vec3(0.1f);
        sphere.mainColor = glm::vec4(light.color, 1.0f);
        static_shapes.push_back(sphere);
    }
}
void VulkanEngine::sync_point_light_billboards() {
    // Remove old point light visualizations
    static_shapes.erase(std::remove_if(static_shapes.begin(), static_shapes.end(),
        [](const StaticMeshData& s) {
            return s.materialType == ShaderOnlyMaterial::POINTLIGHT_VIS ||
                   (s.materialType == ShaderOnlyMaterial::EMISSIVE &&
                    s.type == PrimitiveType::Sphere);
        }), static_shapes.end());

    // Create visual representations for each point light
    for (const auto& light : scenePointLights) {
        // === 1. Small bright sphere at light center (always visible) ===
        StaticMeshData centerSphere;
        centerSphere.type = PrimitiveType::Sphere;
        centerSphere.mesh = defaultMeshes[PrimitiveType::Sphere];
        centerSphere.materialType = ShaderOnlyMaterial::EMISSIVE;
        centerSphere.passType = MaterialPass::MainColor;
        centerSphere.position = light.position;
        centerSphere.scale = glm::vec3(0.15f); // Small visible sphere
        centerSphere.mainColor = glm::vec4(light.color * 2.0f, 1.0f); // Bright color
        static_shapes.push_back(centerSphere);

        // === 2. Outer radius indicator sphere (transparent, shows light range) ===
        StaticMeshData radiusSphere;
        radiusSphere.type = PrimitiveType::Sphere;
        radiusSphere.mesh = defaultMeshes[PrimitiveType::Sphere];
        radiusSphere.materialType = ShaderOnlyMaterial::POINTLIGHT_VIS;
        radiusSphere.passType = MaterialPass::Transparent;
        radiusSphere.position = light.position;
        radiusSphere.scale = glm::vec3(light.radius); // Full radius sphere
        radiusSphere.mainColor = glm::vec4(light.color * 0.3f, 0.15f); // Semi-transparent
        static_shapes.push_back(radiusSphere);
    }
}


//void VulkanEngine::update_scene() {
//    mainCamera.update();
//
//    glm::mat4 view = mainCamera.getViewMatrix();
//
//    // Letterbox viewport ile aspect oranı
//    VkViewport viewport = get_letterbox_viewport();
//    float aspect = viewport.width / viewport.height;
//
//    float fov = glm::radians(70.0f);
//    float nearPlane = 0.1f;
//    float farPlane = 10000.0f;
//
//    // Reverse-Z perspective
//    glm::mat4 projection = glm::perspectiveRH_ZO(fov, aspect, farPlane, nearPlane);
//    projection[1][1] *= -1;
//
//    // === Camera Matrices ===
//    sceneData.view = view;
//    sceneData.proj = projection;
//    sceneData.viewproj = projection * view;
//    drawCommands.viewproj = sceneData.viewproj;
//
//    // === Directional + Ambient Light (sadece ilk karede ayarlanır) ===
//    //static bool firstInit = true;
//    //if (firstInit) {
//    //    sceneData.ambientColor = glm::vec4(0.1f, 0.1f, 0.1f, 0.5f); // RGB + intensity
//    //    glm::vec3 sunDir = glm::normalize(glm::vec3(-1.0f, -3.0f, -1.0f));
//    //    sceneData.sunlightDirection = glm::vec4(sunDir, 3.0f); // yön + intensity
//    //    sceneData.sunlightColor = glm::vec4(1.0f, 0.95f, 0.8f, 1.0f); // sıcak beyaz
//    //    firstInit = false;
//    //}
//
//    // === Point Lights ===
//    sceneData.pointLightCount = 0;
//    for (const auto& light : scenePointLights) {
//        if (sceneData.pointLightCount >= MAX_POINT_LIGHTS) break;
//        sceneData.pointLights[sceneData.pointLightCount++] = light;
//    }
//
//    // === GPU SceneData Buffer'ına yaz ===
//    AllocatedBuffer& buf = get_current_frame().sceneDataBuffer;
//    if (buf.allocation != VK_NULL_HANDLE && buf.info.pMappedData != nullptr) {
//        memcpy(buf.info.pMappedData, &sceneData, sizeof(GPUSceneData));
//    }
//    else {
//        fmt::print("[ERROR] sceneDataBuffer allocation is null or unmapped!\n");
//    }
//
//    // === İstatistikleri Sıfırla ===
//    stats.triangle_count = 0;
//    stats.drawcall_count = 0;
//    stats.visible_count = 0;
//    stats.shader_count = 0;
//
//    std::unordered_set<VkPipeline> uniqueShaders;
//    std::vector<std::string> visibleObjects;
//    std::vector<std::string> shaderNames;
//
//    // === Sahneyi çizim için işleme ===
//    for (const auto& [name, scene] : loadedScenes) {
//        scene->Draw(glm::mat4{ 1.f }, drawCommands);
//    }
//
//    // === Opaque
//    for (const auto& obj : drawCommands.OpaqueSurfaces) {
//        if (obj.material && obj.material->pipeline) {
//            uniqueShaders.insert(obj.material->pipeline->pipeline);
//            shaderNames.push_back(obj.material->pipeline->name);
//        }
//        visibleObjects.push_back(obj.name);
//        stats.triangle_count += obj.indexCount / 3;
//        stats.drawcall_count++;
//        stats.visible_count++;
//    }
//
//    // === Noktasal Işık Meshlerini Temizle ve Yeniden Oluştur ===
//    std::erase_if(static_shapes, [](const StaticMeshData& mesh) {
//        return mesh.materialType == ShaderOnlyMaterial::EMISSIVE;
//        });
//
//    for (const auto& light : scenePointLights) {
//        StaticMeshData sphere;
//        sphere.mesh = generate_sphere_mesh();  // dilersen cache'li hale getirebilirsin
//        sphere.position = glm::vec3(light.position);
//        sphere.scale = glm::vec3(0.25f);
//        sphere.materialType = ShaderOnlyMaterial::EMISSIVE;
//        sphere.passType = MaterialPass::MainColor;
//        static_shapes.push_back(sphere);
//    }
//
//    // === Transparent
//    for (const auto& obj : drawCommands.TransparentSurfaces) {
//        if (obj.material && obj.material->pipeline) {
//            uniqueShaders.insert(obj.material->pipeline->pipeline);
//            shaderNames.push_back(obj.material->pipeline->name);
//        }
//        visibleObjects.push_back(obj.name);
//        stats.triangle_count += obj.indexCount / 3;
//        stats.drawcall_count++;
//        stats.visible_count++;
//    }
//
//    stats.shader_count = static_cast<int>(uniqueShaders.size());
//    stats.visibleObjects = visibleObjects;
//    stats.shaderNames = shaderNames;
//}
//
//void VulkanEngine::sync_point_light_spheres() {
//    // Önce eski ışık kürelerini temizle
//    static_shapes.erase(std::remove_if(static_shapes.begin(), static_shapes.end(),
//        [](const StaticMeshData& s) {
//            return s.materialType == ShaderOnlyMaterial::EMISSIVE &&
//                s.type == PrimitiveType::Sphere;
//        }), static_shapes.end());
//
//    // Her noktasal ışık için sahneye bir küre ekle
//    for (const auto& light : scenePointLights) {
//        StaticMeshData sphere;
//        sphere.type = PrimitiveType::Sphere;
//        sphere.materialType = ShaderOnlyMaterial::EMISSIVE;   // 🔥
//        sphere.passType = MaterialPass::Transparent;          // 🔥
//        sphere.position = light.position;
//        sphere.scale = glm::vec3(0.1f);
//        sphere.color = glm::vec4(light.color, 1.0f);
//        static_shapes.push_back(sphere);
//    }
//}
//void VulkanEngine::sync_point_light_billboards() {
//    // Önce eski glow'ları temizle
//    static_shapes.erase(std::remove_if(static_shapes.begin(), static_shapes.end(),
//        [](const StaticMeshData& s) {
//            return s.materialType == ShaderOnlyMaterial::POINTLIGHT_VIS &&
//                s.type == PrimitiveType::Plane;
//        }), static_shapes.end());
//
//    // Yeni glow düzlemlerini oluştur
//    for (const auto& light : scenePointLights) {
//        StaticMeshData glow;
//        glow.type = PrimitiveType::Plane;
//        glow.mesh = defaultMeshes[PrimitiveType::Plane]; // ✅ Mesh ataması olmazsa çöker
//        glow.materialType = ShaderOnlyMaterial::POINTLIGHT_VIS;
//        glow.passType = MaterialPass::Transparent;
//        glow.position = light.position;
//        glow.scale = glm::vec3(light.radius * 0.4f); // küçük glow efekti
//        glow.color = glm::vec4(light.color * light.intensity * 0.05f, 1.0f); // parlak renk
//
//        static_shapes.push_back(glow);
//    }
//}


//,İKİLİ YARATIYOR 
//void VulkanEngine::update_scene() {
//    mainCamera.update();
//
//    // --- Mouse pozisyonunu al ---
//    int mouseX, mouseY;
//    SDL_GetMouseState(&mouseX, &mouseY);
//
//    // Letterbox viewport ile aspect oranı
//    VkViewport viewport = get_letterbox_viewport();
//    float aspect = viewport.width / viewport.height;
//
//    float fov = glm::radians(70.0f);
//    float nearPlane = 0.1f;
//    float farPlane = 10000.0f;
//
//    // Reverse-Z perspective
//    glm::mat4 projection = glm::perspectiveRH_ZO(fov, aspect, farPlane, nearPlane);
//    projection[1][1] *= -1; // Vulkan için Y eksenini çevir
//
//    glm::mat4 view = mainCamera.getViewMatrix();
//
//    // --- Mouse'u NDC'ye çevir ---
//    float ndc_x = (2.0f * (mouseX - viewport.x)) / viewport.width - 1.0f;
//    float ndc_y = 1.0f - (2.0f * (mouseY - viewport.y)) / viewport.height;
//    glm::vec4 ndcPos = glm::vec4(ndc_x, ndc_y, 1.0f, 1.0f);
//
//    // --- World space'e çevir ---
//    glm::mat4 invViewProj = glm::inverse(projection * view);
//    glm::vec4 worldPos = invViewProj * ndcPos;
//    worldPos /= worldPos.w;
//
//    // --- Kamerayı bu noktaya baktır ---
//    mainCamera.lookAt(glm::vec3(worldPos));
//
//    // --- View ve proj matrislerini tekrar al (kamera bakış noktası değiştiği için) ---
//    view = mainCamera.getViewMatrix();
//    sceneData.view = view;
//    sceneData.proj = projection;
//    sceneData.viewproj = projection * view;
//    drawCommands.viewproj = sceneData.viewproj;
//
//    // İstatistikleri sıfırla
//    stats.triangle_count = 0;
//    stats.drawcall_count = 0;
//    stats.visible_count = 0;
//    stats.shader_count = 0;
//
//    std::unordered_set<VkPipeline> uniqueShaders;
//    std::vector<std::string> visibleObjects;
//    std::vector<std::string> shaderNames;
//
//    // Sahnede yüklü tüm objeleri çiz
//    for (const auto& [name, scene] : loadedScenes) {
//        scene->Draw(glm::mat4{ 1.f }, drawCommands);
//    }
//
//    for (const auto& obj : drawCommands.OpaqueSurfaces) {
//        if (obj.material && obj.material->pipeline) {
//            uniqueShaders.insert(obj.material->pipeline->pipeline);
//            shaderNames.push_back(obj.material->pipeline->name);
//        }
//        visibleObjects.push_back(obj.name);
//        stats.triangle_count += obj.indexCount / 3;
//        stats.drawcall_count++;
//        stats.visible_count++;
//    }
//
//    for (const auto& obj : drawCommands.TransparentSurfaces) {
//        if (obj.material && obj.material->pipeline) {
//            uniqueShaders.insert(obj.material->pipeline->pipeline);
//            shaderNames.push_back(obj.material->pipeline->name);
//        }
//        visibleObjects.push_back(obj.name);
//        stats.triangle_count += obj.indexCount / 3;
//        stats.drawcall_count++;
//        stats.visible_count++;
//    }
//
//    stats.shader_count = static_cast<int>(uniqueShaders.size());
//    stats.visibleObjects = visibleObjects;
//    stats.shaderNames = shaderNames;
//}




//AllocatedBuffer VulkanEngine::create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage)
//{
//    // allocate buffer
//    VkBufferCreateInfo bufferInfo = { .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
//    bufferInfo.pNext = nullptr;
//    bufferInfo.size = allocSize;
//
//    bufferInfo.usage = usage;
//
//    VmaAllocationCreateInfo vmaallocInfo = {};
//    vmaallocInfo.usage = memoryUsage;
//    vmaallocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
//    AllocatedBuffer newBuffer;
//
//    // allocate the buffer
//
//    VK_CHECK(vmaCreateBuffer(_allocator, &bufferInfo, &vmaallocInfo, &newBuffer.buffer, &newBuffer.allocation,
//        &newBuffer.info));
//    if (newBuffer.buffer == VK_NULL_HANDLE) {
//        std::cerr << "Error: Failed to create Vulkan buffer!" << std::endl;
//    }
//
//    return newBuffer;
//}
//AllocatedBuffer VulkanEngine::create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage) 
//{
//    AllocatedBuffer newBuffer{};
//
//    // VkBufferCreateInfo eksiksiz tanım
//    VkBufferCreateInfo bufferInfo = {};
//    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
//    bufferInfo.pNext = nullptr;
//    bufferInfo.flags = 0;
//    bufferInfo.size = allocSize;
//    bufferInfo.usage = usage;
//    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;        // ZORUNLU
//    bufferInfo.queueFamilyIndexCount = 0;                      // Tek kuyruk için 0
//    bufferInfo.pQueueFamilyIndices = nullptr;
//
//    VmaAllocationCreateInfo vmaAllocInfo = {};
//    vmaAllocInfo.usage = memoryUsage;
//    vmaAllocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
//
//    VmaAllocationInfo allocInfo = {};
//    VK_CHECK(vmaCreateBuffer(_allocator, &bufferInfo, &vmaAllocInfo, &newBuffer.buffer, &newBuffer.allocation, &allocInfo));
//
//    newBuffer.info = allocInfo;
//
//    return newBuffer;
//}

AllocatedBuffer VulkanEngine::create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage)
{
    AllocatedBuffer newBuffer{};

    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.pNext = nullptr;
    bufferInfo.flags = 0;
    bufferInfo.size = allocSize;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo vmaAllocInfo = {};
    vmaAllocInfo.usage = memoryUsage;
    vmaAllocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VmaAllocationInfo allocInfo = {};
    VK_CHECK(vmaCreateBuffer(_allocator, &bufferInfo, &vmaAllocInfo, &newBuffer.buffer, &newBuffer.allocation, &allocInfo));

    newBuffer.info = allocInfo;
    newBuffer.size = allocSize; // ✅ EKLENDİ

    return newBuffer;
}




AllocatedImage VulkanEngine::create_image(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped)
{
    AllocatedImage newImage;
    newImage.imageFormat = format;
    newImage.imageExtent = size;

    VkImageCreateInfo img_info = vkinit::image_create_info(format, usage, size);
    if (mipmapped) {
        img_info.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(size.width, size.height)))) + 1;
    }

    // always allocate images on dedicated GPU memory
    VmaAllocationCreateInfo allocinfo = {};
    allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    // allocate and create the image
    VK_CHECK(vmaCreateImage(_allocator, &img_info, &allocinfo, &newImage.image, &newImage.allocation, nullptr));

    // if the format is a depth format, we will need to have it use the correct
    // aspect flag
    VkImageAspectFlags aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;
    if (format == VK_FORMAT_D32_SFLOAT) {
        aspectFlag = VK_IMAGE_ASPECT_DEPTH_BIT;
    }

    // build a image-view for the image
    VkImageViewCreateInfo view_info = vkinit::imageview_create_info(format, newImage.image, aspectFlag);
    view_info.subresourceRange.levelCount = img_info.mipLevels;

    VK_CHECK(vkCreateImageView(_device, &view_info, nullptr, &newImage.imageView));

    return newImage;
}


AllocatedImage VulkanEngine::create_image(void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped)
{
    size_t data_size = size.depth * size.width * size.height * 4;
    AllocatedBuffer uploadbuffer = create_buffer(data_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

    memcpy(uploadbuffer.info.pMappedData, data, data_size);

    AllocatedImage new_image = create_image(size, format, usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, mipmapped);

    immediate_submit([&](VkCommandBuffer cmd) {
        vkutil::transition_image(cmd, new_image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        VkBufferImageCopy copyRegion = {};
        copyRegion.bufferOffset = 0;
        copyRegion.bufferRowLength = 0;
        copyRegion.bufferImageHeight = 0;

        copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.imageSubresource.mipLevel = 0;
        copyRegion.imageSubresource.baseArrayLayer = 0;
        copyRegion.imageSubresource.layerCount = 1;
        copyRegion.imageExtent = size;

        // copy the buffer into the image
        vkCmdCopyBufferToImage(cmd, uploadbuffer.buffer, new_image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
            &copyRegion);

        if (mipmapped) {
            vkutil::generate_mipmaps(cmd, new_image.image, VkExtent2D{ new_image.imageExtent.width,new_image.imageExtent.height });
        }
        else {
            vkutil::transition_image(cmd, new_image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }
        });
    destroy_buffer(uploadbuffer);
    return new_image;
}


GPUMeshBuffers VulkanEngine::uploadMesh(std::span<uint32_t> indices, std::span<Vertex> vertices)
{
    const size_t vertexBufferSize = vertices.size() * sizeof(Vertex);
    const size_t indexBufferSize = indices.size() * sizeof(uint32_t);

    GPUMeshBuffers newSurface;

    newSurface.vertexBuffer = create_buffer(vertexBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);


    VkBufferDeviceAddressInfo deviceAdressInfo{ .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,.buffer = newSurface.vertexBuffer.buffer };
    newSurface.vertexBufferAddress = vkGetBufferDeviceAddress(_device, &deviceAdressInfo);

    newSurface.indexBuffer = create_buffer(indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);

    AllocatedBuffer staging = create_buffer(vertexBufferSize + indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

    void* data = staging.allocation->GetMappedData();

    // copy vertex buffer
    memcpy(data, vertices.data(), vertexBufferSize);
    // copy index buffer
    memcpy((char*)data + vertexBufferSize, indices.data(), indexBufferSize);

    immediate_submit([&](VkCommandBuffer cmd) {
        VkBufferCopy vertexCopy{ 0 };
        vertexCopy.dstOffset = 0;
        vertexCopy.srcOffset = 0;
        vertexCopy.size = vertexBufferSize;

        vkCmdCopyBuffer(cmd, staging.buffer, newSurface.vertexBuffer.buffer, 1, &vertexCopy);

        VkBufferCopy indexCopy{ 0 };
        indexCopy.dstOffset = 0;
        indexCopy.srcOffset = vertexBufferSize;
        indexCopy.size = indexBufferSize;

        vkCmdCopyBuffer(cmd, staging.buffer, newSurface.indexBuffer.buffer, 1, &indexCopy);
        });

    destroy_buffer(staging);

    newSurface.indexCount = static_cast<uint32_t>(indices.size());
    return newSurface;
}


FrameData& VulkanEngine::get_current_frame()
{
    return _frames[_frameNumber % FRAME_OVERLAP];
}

FrameData& VulkanEngine::get_last_frame()
{
    return _frames[(_frameNumber - 1) % FRAME_OVERLAP];
}


void VulkanEngine::immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function)
{
    VK_CHECK(vkResetFences(_device, 1, &_immFence));
    VK_CHECK(vkResetCommandBuffer(_immCommandBuffer, 0));

    VkCommandBuffer cmd = _immCommandBuffer;

    VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    function(cmd);

    VK_CHECK(vkEndCommandBuffer(cmd));

    VkCommandBufferSubmitInfo cmdinfo = vkinit::command_buffer_submit_info(cmd);
    VkSubmitInfo2 submit = vkinit::submit_info(&cmdinfo, nullptr, nullptr);

    // submit command buffer to the queue and execute it.
    //  _renderFence will now block until the graphic commands finish execution
    VK_CHECK(vkQueueSubmit2(_graphicsQueue, 1, &submit, _immFence));

    VK_CHECK(vkWaitForFences(_device, 1, &_immFence, true, 9999999999));
}


void VulkanEngine::destroy_image(const AllocatedImage& img)
{
    vkDestroyImageView(_device, img.imageView, nullptr);
    vmaDestroyImage(_allocator, img.image, img.allocation);
}



void VulkanEngine::destroy_buffer(const AllocatedBuffer& buffer) {
    vmaDestroyBuffer(_allocator, buffer.buffer, buffer.allocation);
}




//void VulkanEngine::init_vulkan()
//{
//    vkb::InstanceBuilder builder;
//
//
//    auto inst_ret = builder.set_app_name("Example Vulkan Application")
//        .request_validation_layers(bUseValidationLayers)
//        .use_default_debug_messenger()
//        .require_api_version(1, 3, 0)
//        .build();
//
//    vkb::Instance vkb_inst = inst_ret.value();
//
//    //grab the instance
//    _instance = vkb_inst.instance;
//    _debug_messenger = vkb_inst.debug_messenger;
//    SDL_Vulkan_CreateSurface(_window, _instance, &_surface);
//
//    //vulkan 1.3
//    VkPhysicalDeviceVulkan13Features features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
//    features.dynamicRendering = true;
//    features.synchronization2 = true;
//
//    //vulkan 1.2
//    VkPhysicalDeviceVulkan12Features features12{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
//    features12.bufferDeviceAddress = true;
//    features12.descriptorIndexing = true;
//
//
//
//    vkb::PhysicalDeviceSelector selector{ vkb_inst };
//    vkb::PhysicalDevice physicalDevice = selector
//        .set_minimum_version(1, 3)
//        .set_required_features_13(features)
//        .set_required_features_12(features12)
//        .set_surface(_surface)
//        .select()
//        .value();
//
//
//
//    vkb::DeviceBuilder deviceBuilder{ physicalDevice };
//
//    vkb::Device vkbDevice = deviceBuilder.build().value();
//
//
//    _device = vkbDevice.device;
//    _chosenGPU = physicalDevice.physical_device;
//    _graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
//    _graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();
//
//
//    // initialize the memory allocator
//    VmaAllocatorCreateInfo allocatorInfo = {};
//    allocatorInfo.physicalDevice = _chosenGPU;
//    allocatorInfo.device = _device;
//    allocatorInfo.instance = _instance;
//    allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
//    vmaCreateAllocator(&allocatorInfo, &_allocator);
//
//    //_mainDeletionQueue.push_function([&]() {
//    //    vmaDestroyAllocator(_allocator);
//    //    });
//}
void VulkanEngine::init_vulkan()
{
    vkb::InstanceBuilder builder;

    builder.set_app_name("Yalaz Engine")
        .request_validation_layers(bUseValidationLayers)
        .use_default_debug_messenger()
        .require_api_version(1, 3, 0);

#ifdef __APPLE__
    // macOS: Enable MoltenVK portability subset
    builder.enable_extension(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif

    auto inst_ret = builder.build();

    if (!inst_ret) {
        fmt::print("Failed to create Vulkan instance: {}\n", inst_ret.error().message());
        abort();
    }

    vkb::Instance vkb_inst = inst_ret.value();

    //grab the instance
    _instance = vkb_inst.instance;
    _debug_messenger = vkb_inst.debug_messenger;
    SDL_Vulkan_CreateSurface(_window, _instance, &_surface);

    //vulkan 1.3
    VkPhysicalDeviceVulkan13Features features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
    features.dynamicRendering = true;
    features.synchronization2 = true;

    //vulkan 1.2
    VkPhysicalDeviceVulkan12Features features12{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
    features12.bufferDeviceAddress = true;
    features12.descriptorIndexing = true;
    features12.runtimeDescriptorArray = true;

    // Vulkan 1.0 features - enable fillModeNonSolid for wireframe rendering
    VkPhysicalDeviceFeatures features10{};
    features10.fillModeNonSolid = VK_TRUE;
#ifndef __APPLE__
    // wideLines not supported on MoltenVK
    features10.wideLines = VK_TRUE;
#endif

    vkb::PhysicalDeviceSelector selector{ vkb_inst };

#ifdef __APPLE__
    // macOS: Add portability subset extension for MoltenVK
    selector.add_required_extension(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
#endif

    auto physicalDevice_ret = selector
        .set_minimum_version(1, 3)
        .set_required_features_13(features)
        .set_required_features_12(features12)
        .set_required_features(features10)
        .set_surface(_surface)
        .select();

    if (!physicalDevice_ret) {
        fmt::print("Failed to select physical device: {}\n", physicalDevice_ret.error().message());
        abort();
    }

    vkb::PhysicalDevice physicalDevice = physicalDevice_ret.value();

    vkb::DeviceBuilder deviceBuilder{ physicalDevice };

    vkb::Device vkbDevice = deviceBuilder.build().value();

    _device = vkbDevice.device;
    _chosenGPU = physicalDevice.physical_device;
    _graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
    _graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

    // initialize the memory allocator
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = _chosenGPU;
    allocatorInfo.device = _device;
    allocatorInfo.instance = _instance;
    allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    vmaCreateAllocator(&allocatorInfo, &_allocator);
}

// =============================================================================
// UV CHECKER PIPELINE - Procedural checker pattern for UV debugging
// =============================================================================
void VulkanEngine::init_uvchecker_pipeline() {
    VkShaderModule vertShader;
    VkShaderModule fragShader;

    if (!vkutil::load_shader_module("../../shaders/uvchecker.vert.spv", _device, &vertShader)) {
        fmt::print("Error: Failed to load uvchecker.vert.spv\n");
        return;
    }
    if (!vkutil::load_shader_module("../../shaders/uvchecker.frag.spv", _device, &fragShader)) {
        fmt::print("Error: Failed to load uvchecker.frag.spv\n");
        vkDestroyShaderModule(_device, vertShader, nullptr);
        return;
    }

    VkPushConstantRange push_constant{};
    push_constant.offset = 0;
    push_constant.size = sizeof(GPUDrawPushConstants);
    push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayout layouts[] = { _gpuSceneDataDescriptorLayout };

    VkPipelineLayoutCreateInfo pipeline_layout_info = vkinit::pipeline_layout_create_info();
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts = layouts;
    pipeline_layout_info.pushConstantRangeCount = 1;
    pipeline_layout_info.pPushConstantRanges = &push_constant;

    VK_CHECK(vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr, &_uvCheckerPipelineLayout));

    PipelineBuilder builder;
    builder._pipelineLayout = _uvCheckerPipelineLayout;
    builder.set_shaders(vertShader, fragShader);
    builder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    builder.set_polygon_mode(VK_POLYGON_MODE_FILL);
    builder.set_cull_mode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE);
    builder.set_multisampling_none();
    builder.disable_blending();
    builder.enable_depthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);
    builder.set_color_attachment_format(_drawImage.imageFormat);
    builder.set_depth_format(_depthImage.imageFormat);

    builder._renderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    builder._renderInfo.colorAttachmentCount = 1;
    builder._renderInfo.pColorAttachmentFormats = &builder._colorAttachmentformat;
    builder._renderInfo.depthAttachmentFormat = _depthImage.imageFormat;

    _uvCheckerPipeline = builder.build_pipeline(_device);

    vkDestroyShaderModule(_device, vertShader, nullptr);
    vkDestroyShaderModule(_device, fragShader, nullptr);
}



void VulkanEngine::init_swapchain()
{
    create_swapchain(_windowExtent.width, _windowExtent.height);

    //depth image size will match the window
    VkExtent3D drawImageExtent = {
        _windowExtent.width,
        _windowExtent.height,
        1
    };

    //hardcoding the draw format to 32 bit float
    _drawImage.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
    _drawImage.imageExtent = drawImageExtent;

    VkImageUsageFlags drawImageUsages{};
    drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    VkImageCreateInfo rimg_info = vkinit::image_create_info(_drawImage.imageFormat, drawImageUsages, drawImageExtent);

    //for the draw image, we want to allocate it from gpu local memory
    VmaAllocationCreateInfo rimg_allocinfo = {};
    rimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    rimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    //allocate and create the image
    vmaCreateImage(_allocator, &rimg_info, &rimg_allocinfo, &_drawImage.image, &_drawImage.allocation, nullptr);

    //build a image-view for the draw image to use for rendering
    VkImageViewCreateInfo rview_info = vkinit::imageview_create_info(_drawImage.imageFormat, _drawImage.image, VK_IMAGE_ASPECT_COLOR_BIT);

    VK_CHECK(vkCreateImageView(_device, &rview_info, nullptr, &_drawImage.imageView));

    //create a depth image too
    //hardcoding the draw format to 32 bit float
    _depthImage.imageFormat = VK_FORMAT_D32_SFLOAT;
    _depthImage.imageExtent = drawImageExtent;

    VkImageUsageFlags depthImageUsages{};
    depthImageUsages |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    VkImageCreateInfo dimg_info = vkinit::image_create_info(_depthImage.imageFormat, depthImageUsages, drawImageExtent);

    //allocate and create the image
    vmaCreateImage(_allocator, &dimg_info, &rimg_allocinfo, &_depthImage.image, &_depthImage.allocation, nullptr);

    //build a image-view for the draw image to use for rendering
    VkImageViewCreateInfo dview_info = vkinit::imageview_create_info(_depthImage.imageFormat, _depthImage.image, VK_IMAGE_ASPECT_DEPTH_BIT);

    VK_CHECK(vkCreateImageView(_device, &dview_info, nullptr, &_depthImage.imageView));

    // Note: Draw and depth images are cleaned up explicitly in cleanup()
}

// =============================================================================
// SOLID PIPELINE - Flat color, no lighting (fastest debug mode)
// =============================================================================
void VulkanEngine::init_solid_pipeline() {
    VkShaderModule vertShader;
    VkShaderModule fragShader;

    if (!vkutil::load_shader_module("../../shaders/solid.vert.spv", _device, &vertShader)) {
        fmt::print("Error: Failed to load solid.vert.spv\n");
        return;
    }
    if (!vkutil::load_shader_module("../../shaders/solid.frag.spv", _device, &fragShader)) {
        fmt::print("Error: Failed to load solid.frag.spv\n");
        vkDestroyShaderModule(_device, vertShader, nullptr);
        return;
    }

    VkPushConstantRange push_constant{};
    push_constant.offset = 0;
    push_constant.size = sizeof(GPUDrawPushConstants);
    push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayout layouts[] = { _gpuSceneDataDescriptorLayout };

    VkPipelineLayoutCreateInfo pipeline_layout_info = vkinit::pipeline_layout_create_info();
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts = layouts;
    pipeline_layout_info.pushConstantRangeCount = 1;
    pipeline_layout_info.pPushConstantRanges = &push_constant;

    VK_CHECK(vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr, &_solidPipelineLayout));

    PipelineBuilder builder;
    builder._pipelineLayout = _solidPipelineLayout;
    builder.set_shaders(vertShader, fragShader);
    builder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    builder.set_polygon_mode(VK_POLYGON_MODE_FILL);
    builder.set_cull_mode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE);
    builder.set_multisampling_none();
    builder.disable_blending();
    builder.enable_depthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);
    builder.set_color_attachment_format(_drawImage.imageFormat);
    builder.set_depth_format(_depthImage.imageFormat);

    builder._renderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    builder._renderInfo.colorAttachmentCount = 1;
    builder._renderInfo.pColorAttachmentFormats = &builder._colorAttachmentformat;
    builder._renderInfo.depthAttachmentFormat = _depthImage.imageFormat;

    _solidPipeline = builder.build_pipeline(_device);

    vkDestroyShaderModule(_device, vertShader, nullptr);
    vkDestroyShaderModule(_device, fragShader, nullptr);
}

// =============================================================================
// SHADED PIPELINE - Hemisphere + N·L studio lighting (no textures)
// =============================================================================
void VulkanEngine::init_shaded_pipeline() {
    VkShaderModule vertShader;
    VkShaderModule fragShader;

    if (!vkutil::load_shader_module("../../shaders/shaded.vert.spv", _device, &vertShader)) {
        fmt::print("Error: Failed to load shaded.vert.spv\n");
        return;
    }
    if (!vkutil::load_shader_module("../../shaders/shaded.frag.spv", _device, &fragShader)) {
        fmt::print("Error: Failed to load shaded.frag.spv\n");
        vkDestroyShaderModule(_device, vertShader, nullptr);
        return;
    }

    VkPushConstantRange push_constant{};
    push_constant.offset = 0;
    push_constant.size = sizeof(GPUDrawPushConstants);
    push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayout layouts[] = { _gpuSceneDataDescriptorLayout };

    VkPipelineLayoutCreateInfo pipeline_layout_info = vkinit::pipeline_layout_create_info();
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts = layouts;
    pipeline_layout_info.pushConstantRangeCount = 1;
    pipeline_layout_info.pPushConstantRanges = &push_constant;

    VK_CHECK(vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr, &_shadedPipelineLayout));

    PipelineBuilder builder;
    builder._pipelineLayout = _shadedPipelineLayout;
    builder.set_shaders(vertShader, fragShader);
    builder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    builder.set_polygon_mode(VK_POLYGON_MODE_FILL);
    builder.set_cull_mode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE);
    builder.set_multisampling_none();
    builder.disable_blending();
    builder.enable_depthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);
    builder.set_color_attachment_format(_drawImage.imageFormat);
    builder.set_depth_format(_depthImage.imageFormat);

    builder._renderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    builder._renderInfo.colorAttachmentCount = 1;
    builder._renderInfo.pColorAttachmentFormats = &builder._colorAttachmentformat;
    builder._renderInfo.depthAttachmentFormat = _depthImage.imageFormat;

    _shadedPipeline = builder.build_pipeline(_device);

    vkDestroyShaderModule(_device, vertShader, nullptr);
    vkDestroyShaderModule(_device, fragShader, nullptr);
}

// =============================================================================
// NORMALS PIPELINE - Visualize world-space normals as RGB colors
// =============================================================================
void VulkanEngine::init_normals_pipeline() {
    VkShaderModule vertShader;
    VkShaderModule fragShader;

    if (!vkutil::load_shader_module("../../shaders/normals.vert.spv", _device, &vertShader)) {
        fmt::print("Error: Failed to load normals.vert.spv\n");
        return;
    }
    if (!vkutil::load_shader_module("../../shaders/normals.frag.spv", _device, &fragShader)) {
        fmt::print("Error: Failed to load normals.frag.spv\n");
        vkDestroyShaderModule(_device, vertShader, nullptr);
        return;
    }

    VkPushConstantRange push_constant{};
    push_constant.offset = 0;
    push_constant.size = sizeof(GPUDrawPushConstants);
    push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayout layouts[] = { _gpuSceneDataDescriptorLayout };

    VkPipelineLayoutCreateInfo pipeline_layout_info = vkinit::pipeline_layout_create_info();
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts = layouts;
    pipeline_layout_info.pushConstantRangeCount = 1;
    pipeline_layout_info.pPushConstantRanges = &push_constant;

    VK_CHECK(vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr, &_normalsPipelineLayout));

    PipelineBuilder builder;
    builder._pipelineLayout = _normalsPipelineLayout;
    builder.set_shaders(vertShader, fragShader);
    builder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    builder.set_polygon_mode(VK_POLYGON_MODE_FILL);
    builder.set_cull_mode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE);
    builder.set_multisampling_none();
    builder.disable_blending();
    builder.enable_depthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);
    builder.set_color_attachment_format(_drawImage.imageFormat);
    builder.set_depth_format(_depthImage.imageFormat);

    builder._renderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    builder._renderInfo.colorAttachmentCount = 1;
    builder._renderInfo.pColorAttachmentFormats = &builder._colorAttachmentformat;
    builder._renderInfo.depthAttachmentFormat = _depthImage.imageFormat;

    _normalsPipeline = builder.build_pipeline(_device);

    vkDestroyShaderModule(_device, vertShader, nullptr);
    vkDestroyShaderModule(_device, fragShader, nullptr);
}



void VulkanEngine::init_material_preview_pipeline() {
    VkShaderModule vertShader;
    VkShaderModule fragShader;

    vkutil::load_shader_module("../../shaders/materialpreview.vert.spv", _device, &vertShader);
    vkutil::load_shader_module("../../shaders/materialpreview.frag.spv", _device, &fragShader);

    VkPushConstantRange push_constant{};
    push_constant.offset = 0;
    push_constant.size = sizeof(GPUDrawPushConstants);
    push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    // Material preview shader uses Set 0 (SceneData) and Set 1 (materialLayout with colorTex at binding 1)
    VkDescriptorSetLayout layouts[] = {
        _gpuSceneDataDescriptorLayout,
        metalRoughMaterial.materialLayout
    };

    VkPipelineLayoutCreateInfo pipeline_layout_info = vkinit::pipeline_layout_create_info();
    pipeline_layout_info.setLayoutCount = 2;
    pipeline_layout_info.pSetLayouts = layouts;
    pipeline_layout_info.pushConstantRangeCount = 1;
    pipeline_layout_info.pPushConstantRanges = &push_constant;

    VK_CHECK(vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr, &_materialPreviewPipelineLayout));

    PipelineBuilder builder;
    builder._pipelineLayout = _materialPreviewPipelineLayout;
    builder.set_shaders(vertShader, fragShader);
    builder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    builder.set_polygon_mode(VK_POLYGON_MODE_FILL);  // Düz yüzey render
    builder.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
    builder.set_multisampling_none();
    builder.disable_blending();
    builder.enable_depthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);
    builder.set_color_attachment_format(_drawImage.imageFormat);
    builder.set_depth_format(_depthImage.imageFormat);

    builder._renderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    builder._renderInfo.colorAttachmentCount = 1;
    builder._renderInfo.pColorAttachmentFormats = &builder._colorAttachmentformat;
    builder._renderInfo.depthAttachmentFormat = _depthImage.imageFormat;

    _materialPreviewPipeline = builder.build_pipeline(_device);

    vkDestroyShaderModule(_device, vertShader, nullptr);
    vkDestroyShaderModule(_device, fragShader, nullptr);
}






void VulkanEngine::create_swapchain(uint32_t width, uint32_t height)
{
    vkb::SwapchainBuilder swapchainBuilder{ _chosenGPU,_device,_surface };

    _swapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

    vkb::Swapchain vkbSwapchain = swapchainBuilder
        //.use_default_format_selection()
        .set_desired_format(VkSurfaceFormatKHR{ .format = _swapchainImageFormat, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
        //use vsync present mode
        .set_desired_present_mode(VK_PRESENT_MODE_MAILBOX_KHR)  // Triple buffering, no VSync cap
        .set_desired_extent(width, height)
        .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
        .build()
        .value();

    _swapchainExtent = vkbSwapchain.extent;
    //store swapchain and its related images
    _swapchain = vkbSwapchain.swapchain;
    _swapchainImages = vkbSwapchain.get_images().value();
    _swapchainImageViews = vkbSwapchain.get_image_views().value();
}

void VulkanEngine::destroy_swapchain()
{
    vkDestroySwapchainKHR(_device, _swapchain, nullptr);


    for (int i = 0; i < _swapchainImageViews.size(); i++) {

        vkDestroyImageView(_device, _swapchainImageViews[i], nullptr);
    }
}



//void VulkanEngine::resize_swapchain()
//{
//    vkDeviceWaitIdle(_device);
//
//    destroy_swapchain();
//
//    int w, h;
//    SDL_GetWindowSize(_window, &w, &h);
//    _windowExtent.width = w;
//    _windowExtent.height = h;
//
//    create_swapchain(_windowExtent.width, _windowExtent.height);
//
//    resize_requested = false;
//}
void VulkanEngine::resize_swapchain()
{
    vkDeviceWaitIdle(_device);

    destroy_swapchain();

    int w, h;
    SDL_GetWindowSize(_window, &w, &h);

    if (w < 64 || h < 64) {
        fmt::print("resize_swapchain: window size too small ({}x{}), skipping swapchain recreation.\n", w, h);
        return;
    }

    _windowExtent.width = w;
    _windowExtent.height = h;

    fmt::print("resize_swapchain: new window size: {}x{}\n", w, h);

    create_swapchain(_windowExtent.width, _windowExtent.height);

    // --- EKLENDİ ---
    // drawImage ve depthImage'ı yeni boyutlara göre yeniden oluştur
    VkExtent3D drawImageExtent = {
        _windowExtent.width,
        _windowExtent.height,
        1
    };

    // Önce eskileri sil
    vkDestroyImageView(_device, _drawImage.imageView, nullptr);
    vmaDestroyImage(_allocator, _drawImage.image, _drawImage.allocation);
    vkDestroyImageView(_device, _depthImage.imageView, nullptr);
    vmaDestroyImage(_allocator, _depthImage.image, _depthImage.allocation);

    // Yenilerini oluştur
    _drawImage = create_image(drawImageExtent, VK_FORMAT_R16G16B16A16_SFLOAT,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);

    _depthImage = create_image(drawImageExtent, VK_FORMAT_D32_SFLOAT,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

    // Descriptor seti de güncelle
    DescriptorWriter writer;
    writer.write_image(0, _drawImage.imageView, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    writer.update_set(_device, _drawImageDescriptors);

    resize_requested = false;
}









void VulkanEngine::init_commands()
{
    // create a command pool for commands submitted to the graphics queue.
    // we also want the pool to allow for resetting of individual command buffers
    VkCommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info(_graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    for (int i = 0; i < FRAME_OVERLAP; i++) {

        VK_CHECK(vkCreateCommandPool(_device, &commandPoolInfo, nullptr, &_frames[i]._commandPool));

        // allocate the default command buffer that we will use for rendering
        VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(_frames[i]._commandPool, 1);

        VK_CHECK(vkAllocateCommandBuffers(_device, &cmdAllocInfo, &_frames[i]._mainCommandBuffer));

        _mainDeletionQueue.push_function([this, i]() { vkDestroyCommandPool(_device, _frames[i]._commandPool, nullptr); });
    }

    VK_CHECK(vkCreateCommandPool(_device, &commandPoolInfo, nullptr, &_immCommandPool));

    // allocate the default command buffer that we will use for rendering
    VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(_immCommandPool, 1);

    VK_CHECK(vkAllocateCommandBuffers(_device, &cmdAllocInfo, &_immCommandBuffer));

    _mainDeletionQueue.push_function([&]() { vkDestroyCommandPool(_device, _immCommandPool, nullptr); });
}



void VulkanEngine::init_sync_structures()
{
    // create syncronization structures
    // one fence to control when the gpu has finished rendering the frame,
    // and 2 semaphores to syncronize rendering with swapchain
    // we want the fence to start signalled so we can wait on it on the first
    // frame
    VkFenceCreateInfo fenceCreateInfo = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
    VK_CHECK(vkCreateFence(_device, &fenceCreateInfo, nullptr, &_immFence));

    _mainDeletionQueue.push_function([&]() { vkDestroyFence(_device, _immFence, nullptr); });

    for (int i = 0; i < FRAME_OVERLAP; i++) {

        VK_CHECK(vkCreateFence(_device, &fenceCreateInfo, nullptr, &_frames[i]._renderFence));

        VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphore_create_info();

        VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_frames[i]._swapchainSemaphore));
        VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_frames[i]._renderSemaphore));

        _mainDeletionQueue.push_function([this, i]() {
            vkDestroyFence(_device, _frames[i]._renderFence, nullptr);
            vkDestroySemaphore(_device, _frames[i]._swapchainSemaphore, nullptr);
            vkDestroySemaphore(_device, _frames[i]._renderSemaphore, nullptr);
            });
    }
}




//void VulkanEngine::init_renderables()
//{
//    std::string structurePath = { "..\\..\\assets\\structure.glb" };
//    auto structureFile = loadGltf(this, structurePath);
//
//    assert(structureFile.has_value());
//
//    loadedScenes["structure"] = *structureFile;
//}

//void VulkanEngine::init_renderables()
//{
//    std::vector<std::pair<std::string, std::string>> glbFiles = {
//        { "house", "..\\..\\assets\\house.glb" },
//        /*{ "building", "..\\..\\assets\\building.glb" }*/
//        /*{ "devil_seed", "..\\..\\assets\\devil_seed.glb" }*/
//        { "gltfscene", "..\\..\\assets\\gltfscene\\scene.gltf" }
//
//    };
//
//    for (const auto& [name, path] : glbFiles) {
//        auto gltfScene = loadGltf(this, path);
//        if (gltfScene.has_value()) {
//            loadedScenes[name] = *gltfScene;
//            fmt::println("✅ Loaded GLB: {} as {}", path, name);
//        }
//        else {
//            fmt::println("⚠️ Failed to load GLB file: {}", path);
//        }
//    }
//}
//void VulkanEngine::init_renderables()
//{
//    // GLTF/GLB yükleme kısmı
//    std::vector<std::pair<std::string, std::string>> glbFiles = {
//        { "house", "..\\..\\assets\\house.glb" },
//        { "gltfscene", "..\\..\\assets\\gltfscene\\scene.gltf" }
//    };
//
//    for (const auto& [name, path] : glbFiles) {
//        auto gltfScene = loadGltf(this, path);
//        if (gltfScene.has_value()) {
//            loadedScenes[name] = *gltfScene;
//            fmt::println(" Loaded GLB: {} as {}", path, name);
//        }
//        else {
//            fmt::println(" Failed to load GLB file: {}", path);
//        }
//    }
//
//    // --- .OBJ dosyalarını yükle ve static_shapes'e ekle ---
//    std::vector<std::pair<std::string, std::string>> objFiles = {
//        { "teapot", "../../assets/teapot.obj" },
//        { "bunny", "../../assets/bunny.obj" }
//    };
//
//    for (size_t i = 0; i < objFiles.size(); ++i) {
//        const auto& [name, path] = objFiles[i];
//        StaticMeshData meshData;
//        meshData.mesh = load_obj_mesh(this, path);
//        meshData.position = glm::vec3(i * 5.0f, 0.0f, -5.0f); // Her biri farklı pozisyonda
//        // İstersen scale, rotation, color, faceColors da ayarlayabilirsin
//        static_shapes.push_back(meshData);
//        fmt::println(" Loaded OBJ: {} as static mesh", path);
//    }
//}


void VulkanEngine::init_renderables()
{
    // === GLTF/GLB Yükleme ===
    std::vector<std::pair<std::string, std::string>> glbFiles = {
        //{ "house", "../../assets/house.glb" },
        /*{ "test", "../../assets/vize.glb" }*/
        //{ "gltfscene", "../../assets/gltfscene/scene.gltf" }
                { "monkey", "../../assets/monkeyHD.glb" },
        /*{ "gltfscene", "..\\..\\assets\\gltfscene\\scene.gltf" }*/
        /*{ "Scene", "..\\..\\assets\\FINAL.glb" }*/



        /*{ "Scene", "..\\..\\assets\\FINAL.glb" }*/



   /*     { "suhos", "..\\..\\assets\\gltfscene\\sikkoIsler.glb" }*/


    };

    for (const auto& [name, path] : glbFiles) {
        auto gltfScene = loadGltf(this, path);
        if (gltfScene.has_value()) {
            loadedScenes[name] = *gltfScene;
            sceneFilePaths[name] = path;
            fmt::print("Loaded GLB: {} as {}\n", path, name);
        }
        else {
            fmt::print("Failed to load GLB file: {}\n", path);
        }
    }

    // === OBJ Yükleme (.obj + .mtl + texture destekli) ===
    std::vector<std::pair<std::string, std::string>> objFiles = {
        //{ "cube", "../../assets/cube.obj" },
        //{ "bunny", "../../assets/bunny.obj" }
    };

    for (size_t i = 0; i < objFiles.size(); ++i) {
        const auto& [name, path] = objFiles[i];
		auto objScene = loadObj(this, path);
        if (objScene.has_value()) {
            loadedScenes[name] = *objScene;
            sceneFilePaths[name] = path;
            fmt::print("Loaded OBJ: {} as {}\n", path, name);
        }
        else {
            fmt::print("Failed to load OBJ file: {}\n", path);
        }
    }
}

void VulkanEngine::saveState(const std::string& filepath) {
    saveEngineState(*this, filepath);
}

void VulkanEngine::loadState(const std::string& filepath) {
    loadEngineState(*this, filepath);
}

void VulkanEngine::resetState() {
    resetEngineState(*this);
}



//void VulkanEngine::init_imgui()
//{
//    // 1: create descriptor pool for IMGUI
//    //  the size of the pool is very oversize, but it's copied from imgui demo
//    //  itself.
//    VkDescriptorPoolSize pool_sizes[] = { { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
//        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
//        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
//        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
//        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
//        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
//        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
//        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
//        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
//        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
//        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 } };
//
//    VkDescriptorPoolCreateInfo pool_info = {};
//    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
//    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
//    pool_info.maxSets = 1000;
//    pool_info.poolSizeCount = (uint32_t)std::size(pool_sizes);
//    pool_info.pPoolSizes = pool_sizes;
//
//    VkDescriptorPool imguiPool;
//    VK_CHECK(vkCreateDescriptorPool(_device, &pool_info, nullptr, &imguiPool));
//
//    // 2: initialize imgui library
//
//    // this initializes the core structures of imgui
//    ImGui::CreateContext();
//
//    // this initializes imgui for SDL
//    ImGui_ImplSDL2_InitForVulkan(_window);
//
//    // this initializes imgui for Vulkan
//    ImGui_ImplVulkan_InitInfo init_info = {};
//    init_info.Instance = _instance;
//    init_info.PhysicalDevice = _chosenGPU;
//    init_info.Device = _device;
//    init_info.Queue = _graphicsQueue;
//    init_info.DescriptorPool = imguiPool;
//    init_info.MinImageCount = 3;
//    init_info.ImageCount = 3;
//    init_info.UseDynamicRendering = true;
//
//    //dynamic rendering parameters for imgui to use
//    init_info.PipelineRenderingCreateInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
//    init_info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
//    init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &_swapchainImageFormat;
//    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
//
//    ImGui_ImplVulkan_Init(&init_info);
//
//    ImGui_ImplVulkan_CreateFontsTexture();
//
//    //_mainDeletionQueue.push_function([this, imguiPool]() {  // <== Explicit yakalama
//    //    ImGui_ImplVulkan_Shutdown();
//    //    vkDestroyDescriptorPool(_device, imguiPool, nullptr);
//    //    });
//
//    _mainDeletionQueue.push_function([&]() {
//        ImGui_ImplVulkan_Shutdown();
//        vkDestroyDescriptorPool(_device, imguiPool, nullptr);
//        });
//
//
//
//}
void VulkanEngine::init_imgui()
{
    // 1: ImGui context oluştur
    ImGui::CreateContext();

    // 2: Enable keyboard navigation
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // 3: Initialize Professional UI System (Blender 4.0 Dark Theme)
    Yalaz::UI::EditorUI::Get().Init(this);

    // 4: SDL2 backend başlat
    ImGui_ImplSDL2_InitForVulkan(_window);

    // 4: Descriptor pool oluştur
    VkDescriptorPoolSize pool_sizes[] = {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000;
    pool_info.poolSizeCount = (uint32_t)std::size(pool_sizes);
    pool_info.pPoolSizes = pool_sizes;

    VkDescriptorPool imguiPool;
    VK_CHECK(vkCreateDescriptorPool(_device, &pool_info, nullptr, &imguiPool));

    // 5: ImGui Vulkan backend başlat
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = _instance;
    init_info.PhysicalDevice = _chosenGPU;
    init_info.Device = _device;
    init_info.Queue = _graphicsQueue;
    init_info.DescriptorPool = imguiPool;
    init_info.MinImageCount = 3;
    init_info.ImageCount = 3;
    init_info.UseDynamicRendering = true;

    // Dynamic rendering attachment format
    init_info.PipelineRenderingCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &_swapchainImageFormat
    };

    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    ImGui_ImplVulkan_Init(&init_info);

    // 6: Font texture oluştur (zorunlu)
    ImGui_ImplVulkan_CreateFontsTexture();

    // 7: Temizlik fonksiyonu
    _mainDeletionQueue.push_function([this, imguiPool]() {
        ImGui_ImplVulkan_Shutdown();
        vkDestroyDescriptorPool(_device, imguiPool, nullptr);
        });
}





//void VulkanEngine::init_pipelines()
//{
//    //COMPUTE PIPELINES
//    init_background_pipelines();
//
//
//    // GRAPHICS PIPELINES
//    //init_triangle_pipeline();
//    /*init_mesh_pipeline();*/
//
//    metalRoughMaterial.build_pipelines(this);
//
//}
void VulkanEngine::init_pipelines() {
    // === COMPUTE ===
    init_background_pipelines();
    init_emissive_pipeline();
    init_pathtrace_pipeline();

    init_outline_wireframe_pipeline();
    // === MATERIAL PIPELINES ===
    metalRoughMaterial.build_pipelines(this);

    // === SHADER-ONLY PIPELINES ===
    init_2d_pipeline(true);
    // Culling açık (önyüz)
    _2dPipelineCulled = _2dPipeline;
    init_2d_pipeline(false);  // Culling kapalı (her iki yüz)
    _2dPipelineDoubleSided = _2dPipeline;

    // === PRIMITIVE PIPELINE (Face colors + lighting) ===
    init_primitive_pipeline();

    // === GRID / DEBUG ===
    init_grid_pipeline();
    //init_light_sphere(); // Küçük bir ışık küresi oluştur

    // === MATERIAL PREVIEW PIPELINE ===
    init_material_preview_pipeline();

    // === VIEW MODE PIPELINES ===
    init_solid_pipeline();      // Flat color, no lighting
    init_shaded_pipeline();     // Hemisphere + N·L studio lighting
    init_normals_pipeline();    // Normal visualization
    init_uvchecker_pipeline();  // UV checker pattern

    // === WIREFRAME PIPELINE ===
    // Note: May fail on MoltenVK/macOS if fillModeNonSolid is not supported
    init_wireframe_pipeline();
}

void VulkanEngine::init_wireframe_pipeline() {
    // Shader modüllerini yükle (ELLEME: mesh.vert ve mesh.frag kullanılacak)
    VkShaderModule vertShader;
    VkShaderModule fragShader;

    vkutil::load_shader_module("../../shaders/mesh.vert.spv", _device, &vertShader);
    vkutil::load_shader_module("../../shaders/mesh.frag.spv", _device, &fragShader);

    VkPushConstantRange push_constant{};
    push_constant.offset = 0;
    push_constant.size = sizeof(GPUDrawPushConstants);
    push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayout layouts[] = {
        _gpuSceneDataDescriptorLayout,
        metalRoughMaterial.materialLayout  // Must match what draw_wireframe() binds
    };

    VkPipelineLayoutCreateInfo pipeline_layout_info = vkinit::pipeline_layout_create_info();
    pipeline_layout_info.setLayoutCount = 2;
    pipeline_layout_info.pSetLayouts = layouts;
    pipeline_layout_info.pushConstantRangeCount = 1;
    pipeline_layout_info.pPushConstantRanges = &push_constant;

    VK_CHECK(vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr, &_wireframePipelineLayout));

    PipelineBuilder builder;
    builder._pipelineLayout = _wireframePipelineLayout;
    builder.set_shaders(vertShader, fragShader);
    builder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    builder.set_polygon_mode(VK_POLYGON_MODE_LINE);        // 🔥 Asıl fark bu
    builder.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
    builder.set_multisampling_none();
    builder.disable_blending();
    builder.enable_depthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);
    builder.set_color_attachment_format(_drawImage.imageFormat);
    builder.set_depth_format(_depthImage.imageFormat);

    builder._renderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    builder._renderInfo.colorAttachmentCount = 1;
    builder._renderInfo.pColorAttachmentFormats = &builder._colorAttachmentformat;
    builder._renderInfo.depthAttachmentFormat = _depthImage.imageFormat;

    _wireframePipeline = builder.build_pipeline(_device);

    if (_wireframePipeline == VK_NULL_HANDLE) {
        fmt::print("Warning: Failed to create wireframe pipeline. VK_POLYGON_MODE_LINE may not be supported on this device (common on MoltenVK/macOS).\n");
        fmt::print("Wireframe mode will fall back to shaded rendering.\n");
    }

    vkDestroyShaderModule(_device, vertShader, nullptr);
    vkDestroyShaderModule(_device, fragShader, nullptr);
}
//void VulkanEngine::init_outline_pipeline() {
//    VkShaderModule vertShader;
//    VkShaderModule fragShader;
//
//    vkutil::load_shader_module("../../shaders/mesh.vert.spv", _device, &vertShader);
//    vkutil::load_shader_module("../../shaders/outline.frag.spv", _device, &fragShader);
//
//    VkPushConstantRange push_constant{};
//    push_constant.offset = 0;
//    push_constant.size = sizeof(GPUDrawPushConstants);
//    push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
//
//    VkDescriptorSetLayout layouts[] = {
//        _gpuSceneDataDescriptorLayout,
//        _drawImageDescriptorLayout
//    };
//
//    VkPipelineLayoutCreateInfo pipeline_layout_info = vkinit::pipeline_layout_create_info();
//    pipeline_layout_info.setLayoutCount = 2;
//    pipeline_layout_info.pSetLayouts = layouts;
//    pipeline_layout_info.pushConstantRangeCount = 1;
//    pipeline_layout_info.pPushConstantRanges = &push_constant;
//
//    VK_CHECK(vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr, &_outlinePipelineLayout));
//
//    PipelineBuilder builder;
//    builder._pipelineLayout = _outlinePipelineLayout;
//    builder.set_shaders(vertShader, fragShader);
//    builder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
//    builder.set_polygon_mode(VK_POLYGON_MODE_FILL);  // ✍️ Solid olarak çizilir
//    builder.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
//    builder.set_multisampling_none();
//    builder.disable_blending();
//    builder.enable_depthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);
//    builder.set_color_attachment_format(_drawImage.imageFormat);
//    builder.set_depth_format(_depthImage.imageFormat);
//
//    builder._renderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
//    builder._renderInfo.colorAttachmentCount = 1;
//    builder._renderInfo.pColorAttachmentFormats = &builder._colorAttachmentformat;
//    builder._renderInfo.depthAttachmentFormat = _depthImage.imageFormat;
//
//    _outlinePipeline = builder.build_pipeline(_device);
//
//    vkDestroyShaderModule(_device, vertShader, nullptr);
//    vkDestroyShaderModule(_device, fragShader, nullptr);
//}


//void VulkanEngine::init_plane_pipeline() {
//    VkShaderModule vertShader, fragShader;
//
//    if (!vkutil::load_shader_module("../../shaders/plane.vert.spv", _device, &vertShader) ||
//        !vkutil::load_shader_module("../../shaders/plane.frag.spv", _device, &fragShader)) {
//        fmt::print("❌ Error loading plane shaders\n");
//        return;
//    }
//
//    VkPipelineShaderStageCreateInfo shaderStages[] = {
//        vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, vertShader),
//        vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, fragShader)
//    };
//
//    auto vertexDesc = Vertex::get_vertex_description();
//
//    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
//    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
//    vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexDesc.bindings.size());
//    vertexInputInfo.pVertexBindingDescriptions = vertexDesc.bindings.data();
//    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexDesc.attributes.size());
//    vertexInputInfo.pVertexAttributeDescriptions = vertexDesc.attributes.data();
//
//    VkPipelineInputAssemblyStateCreateInfo inputAssembly = vkinit::input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
//
//    VkViewport viewport = vkinit::viewport((float)_windowExtent.width, (float)_windowExtent.height);
//    VkRect2D scissor = vkinit::scissor((int)_windowExtent.width, (int)_windowExtent.height);
//    VkPipelineViewportStateCreateInfo viewportState = vkinit::viewport_state_create_info(&viewport, &scissor);
//
//    VkPipelineRasterizationStateCreateInfo rasterizer = vkinit::rasterization_state_create_info(VK_POLYGON_MODE_FILL);
//
//    VkPipelineMultisampleStateCreateInfo multisampling = vkinit::multisampling_state_create_info();
//
//    VkPipelineColorBlendAttachmentState colorBlendAttachment = vkinit::color_blend_attachment_state();
//    VkPipelineColorBlendStateCreateInfo colorBlending = vkinit::color_blend_state_create_info(1, &colorBlendAttachment);
//
//    VkPipelineDepthStencilStateCreateInfo depthStencil = vkinit::depth_stencil_create_info(true, true, VK_COMPARE_OP_LESS);
//
//    VkPushConstantRange pushRange{};
//    pushRange.offset = 0;
//    pushRange.size = sizeof(glm::mat4);
//    pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
//
//    VkPipelineLayoutCreateInfo pipelineLayoutInfo = vkinit::pipeline_layout_create_info();
//    pipelineLayoutInfo.pushConstantRangeCount = 1;
//    pipelineLayoutInfo.pPushConstantRanges = &pushRange;
//
//    pipelineLayoutInfo.setLayoutCount = 0;
//    pipelineLayoutInfo.pSetLayouts = nullptr;
//
//    VK_CHECK(vkCreatePipelineLayout(_device, &pipelineLayoutInfo, nullptr, &planePipelineLayout));
//
//    VkGraphicsPipelineCreateInfo pipelineInfo{};
//    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
//    pipelineInfo.stageCount = 2;
//    pipelineInfo.pStages = shaderStages;
//    pipelineInfo.pVertexInputState = &vertexInputInfo;
//    pipelineInfo.pInputAssemblyState = &inputAssembly;
//    pipelineInfo.pViewportState = &viewportState;
//    pipelineInfo.pRasterizationState = &rasterizer;
//    pipelineInfo.pMultisampleState = &multisampling;
//    pipelineInfo.pDepthStencilState = &depthStencil;
//    pipelineInfo.pColorBlendState = &colorBlending;
//    pipelineInfo.layout = planePipelineLayout;
//    pipelineInfo.renderPass = _renderPass;
//    pipelineInfo.subpass = 0;
//
//    VK_CHECK(vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &planePipeline));
//
//    vkDestroyShaderModule(_device, vertShader, nullptr);
//    vkDestroyShaderModule(_device, fragShader, nullptr);
//
//    fmt::print("✅ Plane graphics pipeline created successfully!\n");
//}



VertexInputDescription Vertex::get_vertex_description() {
    VertexInputDescription description;

    VkVertexInputBindingDescription mainBinding = {};
    mainBinding.binding = 0;
    mainBinding.stride = sizeof(Vertex);
    mainBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    description.bindings.push_back(mainBinding);

    VkVertexInputAttributeDescription posAttrib = {};
    posAttrib.binding = 0;
    posAttrib.location = 0;
    posAttrib.format = VK_FORMAT_R32G32B32_SFLOAT;
    posAttrib.offset = offsetof(Vertex, position);
    description.attributes.push_back(posAttrib);

    VkVertexInputAttributeDescription uvXAttrib = {};
    uvXAttrib.binding = 0;
    uvXAttrib.location = 1;
    uvXAttrib.format = VK_FORMAT_R32_SFLOAT;
    uvXAttrib.offset = offsetof(Vertex, uv_x);
    description.attributes.push_back(uvXAttrib);

    VkVertexInputAttributeDescription normalAttrib = {};
    normalAttrib.binding = 0;
    normalAttrib.location = 2;
    normalAttrib.format = VK_FORMAT_R32G32B32_SFLOAT;
    normalAttrib.offset = offsetof(Vertex, normal);
    description.attributes.push_back(normalAttrib);

    VkVertexInputAttributeDescription uvYAttrib = {};
    uvYAttrib.binding = 0;
    uvYAttrib.location = 3;
    uvYAttrib.format = VK_FORMAT_R32_SFLOAT;
    uvYAttrib.offset = offsetof(Vertex, uv_y);
    description.attributes.push_back(uvYAttrib);

    VkVertexInputAttributeDescription colorAttrib = {};
    colorAttrib.binding = 0;
    colorAttrib.location = 4;
    colorAttrib.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    colorAttrib.offset = offsetof(Vertex, color);
    description.attributes.push_back(colorAttrib);

    return description;
}
void VulkanEngine::init_emissive_pipeline() {
    VkShaderModule vertShader;
    if (!vkutil::load_shader_module("../../shaders/emissive.vert.spv", _device, &vertShader)) {
        throw std::runtime_error("emissive.vert.spv yüklenemedi!");
    }

    VkShaderModule fragShader;
    if (!vkutil::load_shader_module("../../shaders/emissive.frag.spv", _device, &fragShader)) {
        throw std::runtime_error("emissive.frag.spv yüklenemedi!");
    }

    VkPushConstantRange pushConstant{};
    pushConstant.offset = 0;
    pushConstant.size = sizeof(GPUDrawPushConstants);
    pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    // Emissive shader only uses scene data at set 0, not set 1
    VkDescriptorSetLayout layouts[] = {
        _gpuSceneDataDescriptorLayout
    };

    VkPipelineLayoutCreateInfo layoutInfo = vkinit::pipeline_layout_create_info();
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = layouts;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &pushConstant;

    VK_CHECK(vkCreatePipelineLayout(_device, &layoutInfo, nullptr, &_emissivePipelineLayout));

    PipelineBuilder builder;
    builder.set_shaders(vertShader, fragShader);
    builder.set_vertex_input(Vertex::get_vertex_description());
    builder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    builder.set_polygon_mode(VK_POLYGON_MODE_FILL);
    builder.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    builder.set_multisampling_none();
    builder.enable_blending_additive(); // ✨ glow için önemli
    builder.enable_depthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);

    builder.set_color_attachment_format(_drawImage.imageFormat);
    builder.set_depth_format(_depthImage.imageFormat);
    builder._pipelineLayout = _emissivePipelineLayout;

    builder._renderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    builder._renderInfo.colorAttachmentCount = 1;
    builder._renderInfo.pColorAttachmentFormats = &builder._colorAttachmentformat;
    builder._renderInfo.depthAttachmentFormat = _depthImage.imageFormat;

    _emissivePipeline = builder.build_pipeline(_device);

    vkDestroyShaderModule(_device, vertShader, nullptr);
    vkDestroyShaderModule(_device, fragShader, nullptr);
}



void VulkanEngine::init_2d_pipeline(bool enableBackfaceCulling)
{
    // Destroy old pipeline if exists (for hot-reload support)
    if (_2dPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(_device, _2dPipeline, nullptr);
        _2dPipeline = VK_NULL_HANDLE;
    }

    // Always create/recreate pipeline layout with correct stage flags
    if (_2dPipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(_device, _2dPipelineLayout, nullptr);
        _2dPipelineLayout = VK_NULL_HANDLE;
    }

    // 2D shader only uses scene data at set 0, not set 1
    VkDescriptorSetLayout setLayouts[] = {
        _gpuSceneDataDescriptorLayout
    };

    VkPushConstantRange pushRange{};
    pushRange.offset = 0;
    pushRange.size = sizeof(GPUDrawPushConstants);
    pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = setLayouts;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &pushRange;

    VK_CHECK(vkCreatePipelineLayout(_device, &layoutInfo, nullptr, &_2dPipelineLayout));

    VkShaderModule vertShaderModule;
    if (!vkutil::load_shader_module("../../shaders/2d.vert.spv", _device, &vertShaderModule)) {
        throw std::runtime_error("failed to load 2D vertex shader module!");
    }

    VkShaderModule fragShaderModule;
    if (!vkutil::load_shader_module("../../shaders/2d.frag.spv", _device, &fragShaderModule)) {
        throw std::runtime_error("failed to load 2D fragment shader module!");
    }

    VkPipelineShaderStageCreateInfo shaderStages[] = {
        vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, vertShaderModule),
        vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderModule)
    };

    // Use full vertex description for lighting support
    auto vertexDesc = Vertex::get_vertex_description();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexDesc.bindings.size());
    vertexInputInfo.pVertexBindingDescriptions = vertexDesc.bindings.data();
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexDesc.attributes.size());
    vertexInputInfo.pVertexAttributeDescriptions = vertexDesc.attributes.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkViewport viewport{ 0.0f, 0.0f, (float)_windowExtent.width, (float)_windowExtent.height, 0.0f, 1.0f };
    VkRect2D scissor{ {0, 0}, _windowExtent };

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = enableBackfaceCulling ? VK_CULL_MODE_BACK_BIT : VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; // reverse-Z uyumlu
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = 0xF;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL; // reverse-Z için
    depthStencil.stencilTestEnable = VK_FALSE;

    // Dynamic states for viewport/scissor
    VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    // Use Dynamic Rendering (VK_KHR_dynamic_rendering) instead of render pass
    VkFormat colorFormat = _drawImage.imageFormat;
    VkPipelineRenderingCreateInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachmentFormats = &colorFormat;
    renderingInfo.depthAttachmentFormat = _depthImage.imageFormat;

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = &renderingInfo;  // Dynamic rendering info
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = _2dPipelineLayout;
    pipelineInfo.renderPass = VK_NULL_HANDLE;  // No render pass - using dynamic rendering
    pipelineInfo.subpass = 0;

    VK_CHECK(vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_2dPipeline));

    vkDestroyShaderModule(_device, vertShaderModule, nullptr);
    vkDestroyShaderModule(_device, fragShaderModule, nullptr);
}

// =============================================================================
// PRIMITIVE PIPELINE - For primitives with face color support
// =============================================================================
void VulkanEngine::init_primitive_pipeline()
{
    // Destroy old pipeline if exists (for hot-reload support)
    if (_primitivePipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(_device, _primitivePipeline, nullptr);
        _primitivePipeline = VK_NULL_HANDLE;
    }

    // Create pipeline layout with PrimitivePushConstants
    if (_primitivePipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(_device, _primitivePipelineLayout, nullptr);
        _primitivePipelineLayout = VK_NULL_HANDLE;
    }

    VkDescriptorSetLayout setLayouts[] = {
        _gpuSceneDataDescriptorLayout
    };

    VkPushConstantRange pushRange{};
    pushRange.offset = 0;
    pushRange.size = sizeof(PrimitivePushConstants);
    pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = setLayouts;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &pushRange;

    VK_CHECK(vkCreatePipelineLayout(_device, &layoutInfo, nullptr, &_primitivePipelineLayout));

    // Load shaders
    VkShaderModule vertShaderModule;
    if (!vkutil::load_shader_module("../../shaders/primitive.vert.spv", _device, &vertShaderModule)) {
        throw std::runtime_error("failed to load primitive vertex shader module!");
    }

    VkShaderModule fragShaderModule;
    if (!vkutil::load_shader_module("../../shaders/primitive.frag.spv", _device, &fragShaderModule)) {
        throw std::runtime_error("failed to load primitive fragment shader module!");
    }

    VkPipelineShaderStageCreateInfo shaderStages[] = {
        vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, vertShaderModule),
        vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderModule)
    };

    // Use full vertex description for lighting support
    auto vertexDesc = Vertex::get_vertex_description();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexDesc.bindings.size());
    vertexInputInfo.pVertexBindingDescriptions = vertexDesc.bindings.data();
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexDesc.attributes.size());
    vertexInputInfo.pVertexAttributeDescriptions = vertexDesc.attributes.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkViewport viewport{ 0.0f, 0.0f, (float)_windowExtent.width, (float)_windowExtent.height, 0.0f, 1.0f };
    VkRect2D scissor{ {0, 0}, _windowExtent };

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = 0xF;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL; // reverse-Z
    depthStencil.stencilTestEnable = VK_FALSE;

    // Dynamic states for viewport/scissor
    VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    // Use Dynamic Rendering (VK_KHR_dynamic_rendering) instead of render pass
    VkFormat colorFormat = _drawImage.imageFormat;
    VkPipelineRenderingCreateInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachmentFormats = &colorFormat;
    renderingInfo.depthAttachmentFormat = _depthImage.imageFormat;

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = &renderingInfo;  // Dynamic rendering info
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = _primitivePipelineLayout;
    pipelineInfo.renderPass = VK_NULL_HANDLE;  // No render pass - using dynamic rendering
    pipelineInfo.subpass = 0;

    VK_CHECK(vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_primitivePipeline));

    vkDestroyShaderModule(_device, vertShaderModule, nullptr);
    vkDestroyShaderModule(_device, fragShaderModule, nullptr);

    fmt::print("Primitive pipeline initialized successfully\n");
}

// =============================================================================
// DRAW PRIMITIVES - Using new primitive pipeline with face colors
// =============================================================================
void VulkanEngine::draw_primitives(VkCommandBuffer cmd, VkDescriptorSet globalDescriptor)
{
    if (static_shapes.empty()) return;
    if (_primitivePipeline == VK_NULL_HANDLE) return;

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _primitivePipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _primitivePipelineLayout,
        0, 1, &globalDescriptor, 0, nullptr);

    for (auto& shape : static_shapes) {
        if (!shape.visible) continue;

        // Get push constants from the shape
        PrimitivePushConstants pc = shape.get_push_constants();

        vkCmdPushConstants(cmd, _primitivePipelineLayout,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            0, sizeof(PrimitivePushConstants), &pc);

        // Bind vertex buffer
        VkBuffer vertexBuffers[] = { shape.mesh.vertexBuffer.buffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);

        // Bind index buffer and draw
        vkCmdBindIndexBuffer(cmd, shape.mesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(cmd, shape.mesh.indexCount, 1, 0, 0, 0);

        stats.drawcall_count++;
        stats.triangle_count += shape.mesh.indexCount / 3;
    }
}

// Wrapper to maintain compatibility with existing call pattern
void VulkanEngine::draw_primitives_with_viewport(VkCommandBuffer cmd, VkDescriptorSet globalDescriptor, VkViewport viewport, VkRect2D scissor)
{
    vkCmdSetViewport(cmd, 0, 1, &viewport);
    vkCmdSetScissor(cmd, 0, 1, &scissor);
    draw_primitives(cmd, globalDescriptor);
}

void VulkanEngine::init_pathtrace_pipeline()
{
    VkShaderModule computeShader;
    if (!vkutil::load_shader_module("../../shaders/pathtrace.comp.spv", _device, &computeShader)) {
        fmt::print("Warning: pathtrace.comp.spv not found, skipping pathtrace pipeline\n");
        return; // Skip pathtrace pipeline if shader not found
    }

    VkDescriptorSetLayout setLayouts[] = {
        _drawImageDescriptorLayout
    };

    // Push constant range for PathTracePushConstants
    VkPushConstantRange pushConstant{};
    pushConstant.offset = 0;
    pushConstant.size = sizeof(PathTracePushConstants);
    pushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = setLayouts;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstant;

    VK_CHECK(vkCreatePipelineLayout(_device, &pipelineLayoutInfo, nullptr, &_pathTracePipelineLayout));

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.stage = vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_COMPUTE_BIT, computeShader);
    pipelineInfo.layout = _pathTracePipelineLayout;

    VK_CHECK(vkCreateComputePipelines(_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_pathTracePipeline));

    vkDestroyShaderModule(_device, computeShader, nullptr);
}

//void VulkanEngine::draw_rendered_pathtraced(VkCommandBuffer cmd)
//{
//    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _pathTracePipeline);
//
//    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _pathTracePipelineLayout,
//        0, 1, &_drawImageDescriptorSet, 0, nullptr);
//
//    uint32_t groupCountX = (_windowExtent.width + 7) / 8;
//    uint32_t groupCountY = (_windowExtent.height + 7) / 8;
//
//    vkCmdDispatch(cmd, groupCountX, groupCountY, 1);
//}
void VulkanEngine::draw_rendered_pathtraced(VkCommandBuffer cmd)
{
    // Skip if pipeline or descriptor set wasn't initialized
    if (_pathTracePipeline == VK_NULL_HANDLE || _pathTraceDescriptorSet == VK_NULL_HANDLE) {
        static bool warningPrinted = false;
        if (!warningPrinted) {
            fmt::print("Warning: PathTrace pipeline/descriptor not initialized, skipping path tracing\n");
            warningPrinted = true;
        }
        return;
    }

    // Push constant verisi hazırla (kamera + lighting + frameIndex)
    PathTracePushConstants pc{};
    pc.invView = glm::inverse(sceneData.view);
    pc.invProj = glm::inverse(sceneData.proj);
    pc.cameraPos = glm::vec4(mainCamera.position, 0.0f);
    pc.sunlightDir = sceneData.sunlightDirection;
    pc.sunlightColor = sceneData.sunlightColor;
    pc.ambientColor = sceneData.ambientColor;
    pc.frameIndex = _frameNumber;

    // Pipeline'ı bağla
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _pathTracePipeline);

    // Descriptor Set bağla (outImage + sceneData vs.)
    vkCmdBindDescriptorSets(
        cmd,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        _pathTracePipelineLayout,
        0,
        1,
        &_pathTraceDescriptorSet,
        0,
        nullptr
    );

    // Push Constant gönder
    vkCmdPushConstants(
        cmd,
        _pathTracePipelineLayout,
        VK_SHADER_STAGE_COMPUTE_BIT,
        0,
        sizeof(PathTracePushConstants),
        &pc
    );

    // Dispatch
    uint32_t groupCountX = (_windowExtent.width + 7) / 8;
    uint32_t groupCountY = (_windowExtent.height + 7) / 8;

    vkCmdDispatch(cmd, groupCountX, groupCountY, 1);
}



//void VulkanEngine::init_2d_pipeline(bool enableBackfaceCulling)
//{
//    // Eğer layout henüz oluşturulmadıysa oluştur (tek bir kez)
//    if (_2dPipelineLayout == VK_NULL_HANDLE)
//    {
//        VkDescriptorSetLayout setLayouts[] = {
//            _gpuSceneDataDescriptorLayout,
//            _drawImageDescriptorLayout
//        };
//
//        VkPushConstantRange pushRange{};
//        pushRange.offset = 0;
//        pushRange.size = sizeof(GPUDrawPushConstants);
//        pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
//
//        VkPipelineLayoutCreateInfo layoutInfo{};
//        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
//        layoutInfo.setLayoutCount = 2;
//        layoutInfo.pSetLayouts = setLayouts;
//        layoutInfo.pushConstantRangeCount = 1;
//        layoutInfo.pPushConstantRanges = &pushRange;
//
//        VK_CHECK(vkCreatePipelineLayout(_device, &layoutInfo, nullptr, &_2dPipelineLayout));
//    }
//
//    // Shader modüllerini yükle
//    VkShaderModule vertShaderModule;
//    if (!vkutil::load_shader_module("../../shaders/2d.vert.spv", _device, &vertShaderModule)) {
//        throw std::runtime_error("failed to load 2D vertex shader module!");
//    }
//
//    VkShaderModule fragShaderModule;
//    if (!vkutil::load_shader_module("../../shaders/2d.frag.spv", _device, &fragShaderModule)) {
//        throw std::runtime_error("failed to load 2D fragment shader module!");
//    }
//
//    VkPipelineShaderStageCreateInfo shaderStages[] = {
//        vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, vertShaderModule),
//        vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderModule)
//    };
//
//    VkVertexInputBindingDescription bindingDescription{};
//    bindingDescription.binding = 0;
//    bindingDescription.stride = sizeof(Vertex);
//    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
//
//    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
//    attributeDescriptions[0] = { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position) };
//    attributeDescriptions[1] = { 1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, color) };
//    attributeDescriptions[2] = { 2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv_x) };
//
//    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
//    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
//    vertexInputInfo.vertexBindingDescriptionCount = 1;
//    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
//    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
//    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
//
//    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
//    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
//    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
//
//    VkViewport viewport{ 0.0f, 0.0f, (float)_windowExtent.width, (float)_windowExtent.height, 0.0f, 1.0f };
//    VkRect2D scissor{ {0, 0}, _windowExtent };
//
//    VkPipelineViewportStateCreateInfo viewportState{};
//    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
//    viewportState.viewportCount = 1;
//    viewportState.pViewports = &viewport;
//    viewportState.scissorCount = 1;
//    viewportState.pScissors = &scissor;
//
//    VkPipelineRasterizationStateCreateInfo rasterizer{};
//    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
//    rasterizer.depthClampEnable = VK_FALSE;
//    rasterizer.rasterizerDiscardEnable = VK_FALSE;
//    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
//    rasterizer.lineWidth = 1.0f;
//    rasterizer.cullMode = enableBackfaceCulling ? VK_CULL_MODE_BACK_BIT : VK_CULL_MODE_NONE;
//    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
//    rasterizer.depthBiasEnable = VK_FALSE;
//
//    VkPipelineMultisampleStateCreateInfo multisampling{};
//    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
//    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
//
//    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
//    colorBlendAttachment.colorWriteMask = 0xF;
//    colorBlendAttachment.blendEnable = VK_FALSE;
//
//    VkPipelineColorBlendStateCreateInfo colorBlending{};
//    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
//    colorBlending.attachmentCount = 1;
//    colorBlending.pAttachments = &colorBlendAttachment;
//
//    VkPipelineDepthStencilStateCreateInfo depthStencil{};
//    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
//    depthStencil.depthTestEnable = VK_TRUE;
//    depthStencil.depthWriteEnable = VK_TRUE;
//    depthStencil.depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;
//    depthStencil.stencilTestEnable = VK_FALSE;
//
//    VkGraphicsPipelineCreateInfo pipelineInfo{};
//    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
//    pipelineInfo.stageCount = 2;
//    pipelineInfo.pStages = shaderStages;
//    pipelineInfo.pVertexInputState = &vertexInputInfo;
//    pipelineInfo.pInputAssemblyState = &inputAssembly;
//    pipelineInfo.pViewportState = &viewportState;
//    pipelineInfo.pRasterizationState = &rasterizer;
//    pipelineInfo.pMultisampleState = &multisampling;
//    pipelineInfo.pColorBlendState = &colorBlending;
//    pipelineInfo.pDepthStencilState = &depthStencil;
//    pipelineInfo.layout = _2dPipelineLayout;
//    pipelineInfo.renderPass = _renderPass;
//    pipelineInfo.subpass = 0;
//
//    if (enableBackfaceCulling)
//    {
//        VK_CHECK(vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_2dPipeline_CullOn));
//    }
//    else
//    {
//        VK_CHECK(vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_2dPipeline_CullOff));
//    }
//
//    vkDestroyShaderModule(_device, vertShaderModule, nullptr);
//    vkDestroyShaderModule(_device, fragShaderModule, nullptr);
//}




//
//void VulkanEngine::init_2d_pipeline(bool enableBackfaceCulling)
//{
//    // === Pipeline Layout sadece bir kez oluşturulmalı ===
//    if (_2dPipelineLayout == VK_NULL_HANDLE)
//    {
//        VkDescriptorSetLayout setLayouts[] = {
//            _gpuSceneDataDescriptorLayout,
//            _drawImageDescriptorLayout
//        };
//
//        VkPushConstantRange pushRange{};
//        pushRange.offset = 0;
//        pushRange.size = sizeof(GPUDrawPushConstants);
//        pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
//
//        VkPipelineLayoutCreateInfo layoutInfo{};
//        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
//        layoutInfo.setLayoutCount = 2;
//        layoutInfo.pSetLayouts = setLayouts;
//        layoutInfo.pushConstantRangeCount = 1;
//        layoutInfo.pPushConstantRanges = &pushRange;
//
//        VK_CHECK(vkCreatePipelineLayout(_device, &layoutInfo, nullptr, &_2dPipelineLayout));
//    }
//
//    // === Shader Modülleri Yükle ===
//    VkShaderModule vertShaderModule;
//    if (!vkutil::load_shader_module("../../shaders/2d.vert.spv", _device, &vertShaderModule)) {
//        throw std::runtime_error("failed to load 2D vertex shader module!");
//    }
//
//    VkShaderModule fragShaderModule;
//    if (!vkutil::load_shader_module("../../shaders/2d.frag.spv", _device, &fragShaderModule)) {
//        throw std::runtime_error("failed to load 2D fragment shader module!");
//    }
//
//    VkPipelineShaderStageCreateInfo shaderStages[] = {
//        vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, vertShaderModule),
//        vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderModule)
//    };
//
//    // === Vertex Input Yapısı ===
//    VkVertexInputBindingDescription bindingDescription{};
//    bindingDescription.binding = 0;
//    bindingDescription.stride = sizeof(Vertex);
//    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
//
//    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
//    attributeDescriptions[0] = { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position) };
//    attributeDescriptions[1] = { 1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, color) };
//    attributeDescriptions[2] = { 2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv_x) };
//
//    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
//    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
//    vertexInputInfo.vertexBindingDescriptionCount = 1;
//    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
//    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
//    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
//
//    // === Diğer Pipeline Ayarları ===
//    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
//    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
//    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
//
//    VkPipelineViewportStateCreateInfo viewportState{};
//    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
//    viewportState.viewportCount = 1;
//    viewportState.scissorCount = 1;
//
//    VkPipelineRasterizationStateCreateInfo rasterizer{};
//    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
//    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
//    rasterizer.lineWidth = 1.0f;
//    rasterizer.cullMode = enableBackfaceCulling ? VK_CULL_MODE_BACK_BIT : VK_CULL_MODE_NONE;
//    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
//
//    VkPipelineMultisampleStateCreateInfo multisampling{};
//    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
//    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
//
//    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
//    colorBlendAttachment.colorWriteMask = 0xF;
//    colorBlendAttachment.blendEnable = VK_FALSE;
//
//    VkPipelineColorBlendStateCreateInfo colorBlending{};
//    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
//    colorBlending.attachmentCount = 1;
//    colorBlending.pAttachments = &colorBlendAttachment;
//
//    VkPipelineDepthStencilStateCreateInfo depthStencil{};
//    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
//    depthStencil.depthTestEnable = VK_TRUE;
//    depthStencil.depthWriteEnable = VK_TRUE;
//    depthStencil.depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;
//
//    // === Pipeline Oluşturma ===
//    VkGraphicsPipelineCreateInfo pipelineInfo{};
//    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
//    pipelineInfo.stageCount = 2;
//    pipelineInfo.pStages = shaderStages;
//    pipelineInfo.pVertexInputState = &vertexInputInfo;
//    pipelineInfo.pInputAssemblyState = &inputAssembly;
//    pipelineInfo.pViewportState = &viewportState;
//    pipelineInfo.pRasterizationState = &rasterizer;
//    pipelineInfo.pMultisampleState = &multisampling;
//    pipelineInfo.pColorBlendState = &colorBlending;
//    pipelineInfo.pDepthStencilState = &depthStencil;
//    pipelineInfo.layout = _2dPipelineLayout;
//    pipelineInfo.renderPass = _renderPass;
//    pipelineInfo.subpass = 0;
//
//    VK_CHECK(vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_2dPipeline));
//
//    // === Shader Modüllerini Temizle ===
//    vkDestroyShaderModule(_device, vertShaderModule, nullptr);
//    vkDestroyShaderModule(_device, fragShaderModule, nullptr);
//}







//void VulkanEngine::init_grid_pipeline() {
//    VkShaderModule vertShaderModule = load_shader_module("../../shaders/mesh.vert.spv");
//    VkShaderModule fragShaderModule = load_shader_module("../../shaders/mesh.frag.spv");
//
//    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
//    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
//    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
//    vertShaderStageInfo.module = vertShaderModule;
//    vertShaderStageInfo.pName = "main";
//
//    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
//    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
//    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
//    fragShaderStageInfo.module = fragShaderModule;
//    fragShaderStageInfo.pName = "main";
//
//    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };
//
//    // === Vertex input (boş) ===
//    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
//    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
//    vertexInputInfo.vertexBindingDescriptionCount = 0;
//    vertexInputInfo.pVertexBindingDescriptions = nullptr;
//    vertexInputInfo.vertexAttributeDescriptionCount = 0;
//    vertexInputInfo.pVertexAttributeDescriptions = nullptr;
//
//    // === Input Assembly ===
//    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
//    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
//    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
//    inputAssembly.primitiveRestartEnable = VK_FALSE;
//
//    // === Viewport & Scissor dinamik ===
//    VkDynamicState dynamicStates[] = {
//        VK_DYNAMIC_STATE_VIEWPORT,
//        VK_DYNAMIC_STATE_SCISSOR
//    };
//    VkPipelineDynamicStateCreateInfo dynamicState{};
//    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
//    dynamicState.dynamicStateCount = 2;
//    dynamicState.pDynamicStates = dynamicStates;
//
//    // === Rasterizer ===
//    VkPipelineRasterizationStateCreateInfo rasterizer{};
//    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
//    rasterizer.depthClampEnable = VK_FALSE;
//    rasterizer.rasterizerDiscardEnable = VK_FALSE;
//    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
//    rasterizer.lineWidth = 1.0f;
//    rasterizer.cullMode = VK_CULL_MODE_NONE;
//    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
//    rasterizer.depthBiasEnable = VK_FALSE;
//
//    // === Multisample ===
//    VkPipelineMultisampleStateCreateInfo multisampling{};
//    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
//    multisampling.sampleShadingEnable = VK_FALSE;
//    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
//
//    // === Color blend ===
//    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
//    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
//        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
//    colorBlendAttachment.blendEnable = VK_FALSE;
//
//    VkPipelineColorBlendStateCreateInfo colorBlending{};
//    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
//    colorBlending.logicOpEnable = VK_FALSE;
//    colorBlending.attachmentCount = 1;
//    colorBlending.pAttachments = &colorBlendAttachment;
//
//    // === Depth stencil ===
//    VkPipelineDepthStencilStateCreateInfo depthStencil{};
//    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
//    depthStencil.depthTestEnable = VK_TRUE;
//    depthStencil.depthWriteEnable = VK_TRUE;
//    depthStencil.depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;
//    depthStencil.depthBoundsTestEnable = VK_FALSE;
//    depthStencil.stencilTestEnable = VK_FALSE;
//
//    // === Layout ===
//    VkDescriptorSetLayout setLayouts[] = { _gpuSceneDataDescriptorLayout, _drawImageDescriptorLayout };
//
//    VkPushConstantRange pushConstantRange{};
//    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
//    pushConstantRange.offset = 0;
//    pushConstantRange.size = sizeof(GPUDrawPushConstants);
//
//    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
//    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
//    pipelineLayoutInfo.setLayoutCount = 2;
//    pipelineLayoutInfo.pSetLayouts = setLayouts;
//    pipelineLayoutInfo.pushConstantRangeCount = 1;
//    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
//
//    if (vkCreatePipelineLayout(_device, &pipelineLayoutInfo, nullptr, &gridPipelineLayout) != VK_SUCCESS) {
//        throw std::runtime_error("failed to create grid pipeline layout!");
//    }
//
//    // === Graphics Pipeline ===
//    VkGraphicsPipelineCreateInfo pipelineInfo{};
//    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
//    pipelineInfo.stageCount = 2;
//    pipelineInfo.pStages = shaderStages;
//    pipelineInfo.pVertexInputState = &vertexInputInfo;
//    pipelineInfo.pInputAssemblyState = &inputAssembly;
//    pipelineInfo.pRasterizationState = &rasterizer;
//    pipelineInfo.pMultisampleState = &multisampling;
//    pipelineInfo.pDepthStencilState = &depthStencil;
//    pipelineInfo.pColorBlendState = &colorBlending;
//    pipelineInfo.pDynamicState = &dynamicState;
//    pipelineInfo.layout = gridPipelineLayout;
//    pipelineInfo.renderPass = _renderPass;
//    pipelineInfo.subpass = 0;
//
//    if (vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &gridPipeline) != VK_SUCCESS) {
//        throw std::runtime_error("failed to create grid graphics pipeline!");
//    }
//
//    vkDestroyShaderModule(_device, vertShaderModule, nullptr);
//    vkDestroyShaderModule(_device, fragShaderModule, nullptr);
//}
//void VulkanEngine::init_grid_pipeline()
//{
//    VkShaderModule vertShaderModule = load_shader_module("../../shaders/grid.vert.spv");
//    VkShaderModule fragShaderModule = load_shader_module("../../shaders/grid.frag.spv");
//
//    VkPipelineShaderStageCreateInfo shaderStages[] = {
//        vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, vertShaderModule),
//        vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderModule)
//    };
//
//    // === Vertex input (Vertex + Color + UV) ===
//    VkVertexInputBindingDescription bindingDescription{};
//    bindingDescription.binding = 0;
//    bindingDescription.stride = sizeof(Vertex);
//    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
//
//    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
//    attributeDescriptions[0] = { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position) };
//    attributeDescriptions[1] = { 1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, color) };
//    attributeDescriptions[2] = { 2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv_x) };
//
//    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
//    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
//    vertexInputInfo.vertexBindingDescriptionCount = 1;
//    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
//    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
//    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
//
//    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
//    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
//    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
//
//    VkDynamicState dynamicStates[] = {
//        VK_DYNAMIC_STATE_VIEWPORT,
//        VK_DYNAMIC_STATE_SCISSOR
//    };
//    VkPipelineDynamicStateCreateInfo dynamicState{};
//    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
//    dynamicState.dynamicStateCount = 2;
//    dynamicState.pDynamicStates = dynamicStates;
//
//    VkPipelineViewportStateCreateInfo viewportState{};
//    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
//    viewportState.viewportCount = 1;
//    viewportState.scissorCount = 1;
//
//    VkPipelineRasterizationStateCreateInfo rasterizer{};
//    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
//    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
//    rasterizer.lineWidth = 1.0f;
//    rasterizer.cullMode = VK_CULL_MODE_NONE;
//    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
//
//    VkPipelineMultisampleStateCreateInfo multisampling{};
//    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
//    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
//
//    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
//    colorBlendAttachment.colorWriteMask = 0xF;
//    colorBlendAttachment.blendEnable = VK_FALSE;
//
//    VkPipelineColorBlendStateCreateInfo colorBlending{};
//    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
//    colorBlending.attachmentCount = 1;
//    colorBlending.pAttachments = &colorBlendAttachment;
//
//    VkPipelineDepthStencilStateCreateInfo depthStencil{};
//    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
//    depthStencil.depthTestEnable = VK_TRUE;
//    depthStencil.depthWriteEnable = VK_TRUE;
//    depthStencil.depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;
//
//    VkDescriptorSetLayout setLayouts[] = {
//        _gpuSceneDataDescriptorLayout,
//        _drawImageDescriptorLayout
//    };
//
//    VkPushConstantRange pushConstantRange{};
//    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
//    pushConstantRange.offset = 0;
//    pushConstantRange.size = sizeof(GPUDrawPushConstants);
//
//    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
//    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
//    pipelineLayoutInfo.setLayoutCount = 2;
//    pipelineLayoutInfo.pSetLayouts = setLayouts;
//    pipelineLayoutInfo.pushConstantRangeCount = 1;
//    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
//
//    if (vkCreatePipelineLayout(_device, &pipelineLayoutInfo, nullptr, &gridPipelineLayout) != VK_SUCCESS) {
//        throw std::runtime_error("failed to create grid pipeline layout!");
//    }
//
//    VkGraphicsPipelineCreateInfo pipelineInfo{};
//    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
//    pipelineInfo.stageCount = 2;
//    pipelineInfo.pStages = shaderStages;
//    pipelineInfo.pVertexInputState = &vertexInputInfo;
//    pipelineInfo.pInputAssemblyState = &inputAssembly;
//    pipelineInfo.pViewportState = &viewportState;
//    pipelineInfo.pRasterizationState = &rasterizer;
//    pipelineInfo.pMultisampleState = &multisampling;
//    pipelineInfo.pColorBlendState = &colorBlending;
//    pipelineInfo.pDepthStencilState = &depthStencil;
//    pipelineInfo.pDynamicState = &dynamicState;
//    pipelineInfo.layout = gridPipelineLayout;
//    pipelineInfo.renderPass = _renderPass;
//    pipelineInfo.subpass = 0;
//
//    if (vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &gridPipeline) != VK_SUCCESS) {
//        throw std::runtime_error("failed to create grid graphics pipeline!");
//    }
//
//    vkDestroyShaderModule(_device, vertShaderModule, nullptr);
//    vkDestroyShaderModule(_device, fragShaderModule, nullptr);
//}
//
//void VulkanEngine::init_grid_pipeline() {
//    VkShaderModule vertShaderModule = load_shader_module("../../shaders/grid.vert.spv");
//    VkShaderModule fragShaderModule = load_shader_module("../../shaders/grid.frag.spv");
//
//    VkPipelineShaderStageCreateInfo shaderStages[] = {
//        vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, vertShaderModule),
//        vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderModule)
//    };
//
//    VkVertexInputBindingDescription bindingDescription{};
//    bindingDescription.binding = 0;
//    bindingDescription.stride = sizeof(Vertex);
//    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
//
//    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
//    attributeDescriptions[0] = { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position) };
//    attributeDescriptions[1] = { 1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, color) };
//    attributeDescriptions[2] = { 2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv_x) };
//
//    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
//    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
//    vertexInputInfo.vertexBindingDescriptionCount = 1;
//    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
//    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
//    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
//
//    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
//    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
//    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
//
//    VkPipelineRasterizationStateCreateInfo rasterizer{};
//    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
//    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
//    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
//    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
//    rasterizer.lineWidth = 1.0f;
//
//    VkPipelineMultisampleStateCreateInfo multisampling{};
//    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
//    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
//
//    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
//    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
//        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
//
//    VkPipelineColorBlendStateCreateInfo colorBlending{};
//    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
//    colorBlending.attachmentCount = 1;
//    colorBlending.pAttachments = &colorBlendAttachment;
//
//    VkPipelineDepthStencilStateCreateInfo depthStencil{};
//    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
//    depthStencil.depthTestEnable = VK_TRUE;
//    depthStencil.depthWriteEnable = VK_TRUE;
//    depthStencil.depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;  // ✅ Reverse-Z için
//
//    VkPushConstantRange pushConstantRange{};
//    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
//    pushConstantRange.offset = 0;
//    pushConstantRange.size = sizeof(GPUDrawPushConstants);
//
//    VkDescriptorSetLayout setLayouts[] = { _gpuSceneDataDescriptorLayout, _drawImageDescriptorLayout };
//
//    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
//    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
//    pipelineLayoutInfo.setLayoutCount = 2;
//    pipelineLayoutInfo.pSetLayouts = setLayouts;
//    pipelineLayoutInfo.pushConstantRangeCount = 1;
//    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
//
//    vkCreatePipelineLayout(_device, &pipelineLayoutInfo, nullptr, &gridPipelineLayout);
//
//    VkGraphicsPipelineCreateInfo pipelineInfo{};
//    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
//    pipelineInfo.stageCount = 2;
//    pipelineInfo.pStages = shaderStages;
//    pipelineInfo.pVertexInputState = &vertexInputInfo;
//    pipelineInfo.pInputAssemblyState = &inputAssembly;
//    pipelineInfo.pRasterizationState = &rasterizer;
//    pipelineInfo.pMultisampleState = &multisampling;
//    pipelineInfo.pColorBlendState = &colorBlending;
//    pipelineInfo.pDepthStencilState = &depthStencil;
//    pipelineInfo.layout = gridPipelineLayout;
//    pipelineInfo.renderPass = _renderPass;
//
//    vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &gridPipeline);
//
//    vkDestroyShaderModule(_device, vertShaderModule, nullptr);
//    vkDestroyShaderModule(_device, fragShaderModule, nullptr);
//}

void VulkanEngine::init_grid_pipeline() {
    VkShaderModule vertShaderModule = load_shader_module("../../shaders/grid.vert.spv");
    VkShaderModule fragShaderModule = load_shader_module("../../shaders/grid.frag.spv");

    VkPipelineShaderStageCreateInfo shaderStages[2] = {
        vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, vertShaderModule),
        vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderModule)
    };

    auto vertexDescription = Vertex::get_vertex_description();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexDescription.bindings.size());
    vertexInputInfo.pVertexBindingDescriptions = vertexDescription.bindings.data();
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexDescription.attributes.size());
    vertexInputInfo.pVertexAttributeDescriptions = vertexDescription.attributes.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    // Grid should not write to depth buffer, only test against it
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_FALSE;  // Don't write to depth - grid is transparent
    depthStencil.depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;

    // Grid shader only uses Set 0 (SceneData), no Set 1 needed
    VkDescriptorSetLayout setLayouts[] = {
        _gpuSceneDataDescriptorLayout
    };

    // Use GridPushConstants for the grid pipeline
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(GridPushConstants);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = setLayouts;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    VK_CHECK(vkCreatePipelineLayout(_device, &pipelineLayoutInfo, nullptr, &gridPipelineLayout));

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    // Use dynamic rendering (VK 1.3) instead of renderPass
    VkFormat colorFormat = _drawImage.imageFormat;
    VkPipelineRenderingCreateInfo renderInfo{};
    renderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    renderInfo.colorAttachmentCount = 1;
    renderInfo.pColorAttachmentFormats = &colorFormat;
    renderInfo.depthAttachmentFormat = _depthImage.imageFormat;

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = &renderInfo;  // Dynamic rendering instead of renderPass
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;  // This was missing!
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = gridPipelineLayout;
    pipelineInfo.renderPass = VK_NULL_HANDLE;  // Using dynamic rendering
    pipelineInfo.subpass = 0;

    VK_CHECK(vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &gridPipeline));

    vkDestroyShaderModule(_device, vertShaderModule, nullptr);
    vkDestroyShaderModule(_device, fragShaderModule, nullptr);
}












//void VulkanEngine::draw_grid(VkCommandBuffer cmd) {
//    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, gridPipeline);
//
//    // Bind descriptor sets
//    VkDescriptorSet descriptorSets[] = { get_current_frame()._frameDescriptors.allocate(_device, _gpuSceneDataDescriptorLayout) };
//    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, gridPipelineLayout, 0, 1, descriptorSets, 0, nullptr);
//
//    // Set the viewport and scissor
//    VkViewport viewport{};
//    viewport.x = 0.0f;
//    viewport.y = 0.0f;
//    viewport.width = static_cast<float>(_windowExtent.width);
//    viewport.height = static_cast<float>(_windowExtent.height);
//    viewport.minDepth = 0.0f;
//    viewport.maxDepth = 1.0f;
//    vkCmdSetViewport(cmd, 0, 1, &viewport);
//
//    VkRect2D scissor{};
//    scissor.offset = { 0, 0 };
//    scissor.extent = _windowExtent;
//    vkCmdSetScissor(cmd, 0, 1, &scissor);
//
//    // Draw call
//    vkCmdDraw(cmd, 6, 1, 0, 0);
//
//    //// Print grid coordinates
//    //for (float x = -10.0f; x <= 10.0f; x += 1.0f) {
//    //    for (float y = -10.0f; y <= 10.0f; y += 1.0f) {
//    //        std::cout << "Grid Coordinate: (" << x << ", " << y << ")" << std::endl;
//    //    }
//    //}
//}

//void VulkanEngine::draw_grid(VkCommandBuffer cmd, VkDescriptorSet globalDescriptor)
//{
//    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, gridPipeline);
//
//    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, gridPipelineLayout, 0, 1, &globalDescriptor, 0, nullptr);
//
//    VkDeviceSize offset = 0;
//    vkCmdBindVertexBuffers(cmd, 0, 1, &rectangle.vertexBuffer.buffer, &offset);  // rectangle meshini kullanabilirsin
//    vkCmdBindIndexBuffer(cmd, rectangle.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
//
//    GPUDrawPushConstants push{};
//    push.worldMatrix = glm::mat4(1.0f);  // world-space'te grid sabit
//    push.vertexBuffer = rectangle.vertexBufferAddress;
//
//    vkCmdPushConstants(cmd, gridPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
//        sizeof(GPUDrawPushConstants), &push);
//
//    vkCmdDrawIndexed(cmd, rectangle.indexCount, 1, 0, 0, 0);
//}

//void VulkanEngine::draw_grid(VkCommandBuffer cmd, VkDescriptorSet globalDescriptor) {
//    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, gridPipeline);
//
//    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, gridPipelineLayout,
//        0, 1, &globalDescriptor, 0, nullptr);
//
//    VkViewport viewport = get_letterbox_viewport();
//    vkCmdSetViewport(cmd, 0, 1, &viewport);
//
//    VkRect2D scissor = { {0, 0}, _windowExtent };
//    vkCmdSetScissor(cmd, 0, 1, &scissor);
//
//    VkDeviceSize offset = 0;
//
//    // 🔥 Vertex ve index buffer bağla
//    vkCmdBindVertexBuffers(cmd, 0, 1, &gridMesh.vertexBuffer.buffer, &offset);
//    vkCmdBindIndexBuffer(cmd, gridMesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
//
//    // 🔥 Plane mesh çiz
//    vkCmdDrawIndexed(cmd, gridMesh.indexCount, 1, 0, 0, 0);
//}
//void VulkanEngine::draw_grid(VkCommandBuffer cmd, VkDescriptorSet globalDescriptor)
//{
//    // === Pipeline & Descriptor Set ===
//    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, gridPipeline);
//
//    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, gridPipelineLayout,
//        0, 1, &globalDescriptor, 0, nullptr);
//
//    // === Viewport & Scissor ===
//    VkViewport viewport = get_letterbox_viewport();
//    vkCmdSetViewport(cmd, 0, 1, &viewport);
//
//    VkRect2D scissor = { {0, 0}, _windowExtent };
//    vkCmdSetScissor(cmd, 0, 1, &scissor);
//
//    // === Vertex / Index Buffer ===
//    VkDeviceSize offset = 0;
//    vkCmdBindVertexBuffers(cmd, 0, 1, &gridMesh.vertexBuffer.buffer, &offset);
//    vkCmdBindIndexBuffer(cmd, gridMesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
//
//    // === Push Constants (worldMatrix) ===
//    GPUDrawPushConstants push{};
//    push.worldMatrix = glm::translate(glm::vec3(0.0f, 0.0f, 0.0f)) *
//        glm::scale(glm::vec3(100.0f, 1.0f, 100.0f));
//    push.vertexBuffer = gridMesh.vertexBufferAddress;  // Eğer shader’da kullanıyorsan
//
//    vkCmdPushConstants(cmd, gridPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
//        0, sizeof(GPUDrawPushConstants), &push);
//
//    // === Draw ===
//    vkCmdDrawIndexed(cmd, gridMesh.indexCount, 1, 0, 0, 0);
//}

void VulkanEngine::draw_grid(VkCommandBuffer cmd, VkDescriptorSet globalDescriptor)
{
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, gridPipeline);

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, gridPipelineLayout,
        0, 1, &globalDescriptor, 0, nullptr);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &gridMesh.vertexBuffer.buffer, &offset);
    vkCmdBindIndexBuffer(cmd, gridMesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

    // ==========================================================================
    // OPTIMIZED GRID - Reduced scale for better performance
    // ==========================================================================

    glm::vec3 camPos = mainCamera.position;
    float cameraHeight = std::max(std::abs(camPos.y - _gridSettings.gridHeight), 1.0f);

    // Dynamic grid scale based on camera height and fade distance
    float dynamicScale = _gridSettings.infiniteGrid
        ? std::min(std::max(100.0f, cameraHeight * 50.0f), _gridSettings.fadeDistance * 1.5f)
        : _gridSettings.fadeDistance * 1.2f;

    // Snap to prevent jittering
    float snapUnit = _gridSettings.baseGridSize * _gridSettings.majorGridMultiplier;
    float snappedX = _gridSettings.infiniteGrid ? std::floor(camPos.x / snapUnit) * snapUnit : 0.0f;
    float snappedZ = _gridSettings.infiniteGrid ? std::floor(camPos.z / snapUnit) * snapUnit : 0.0f;

    glm::mat4 worldMatrix = glm::translate(glm::vec3(snappedX, _gridSettings.gridHeight, snappedZ)) *
                            glm::scale(glm::vec3(dynamicScale, 1.0f, dynamicScale));

    // === Setup ALL push constants ===
    GridPushConstants push{};
    push.worldMatrix = worldMatrix;

    // gridParams: x=cellSize, y=fadeDistance, z=lineWidth, w=opacity
    push.gridParams = glm::vec4(
        _gridSettings.baseGridSize,
        _gridSettings.fadeDistance,  // No cap - user controls fade distance
        _gridSettings.lineWidth,
        _gridSettings.gridOpacity
    );

    // gridParams2: x=dynamicLOD, y=showAxisColors, z=showSubdivisions, w=axisLineWidth
    push.gridParams2 = glm::vec4(
        _gridSettings.dynamicLOD ? 1.0f : 0.0f,
        _gridSettings.showAxisColors ? 1.0f : 0.0f,
        _gridSettings.showSubdivisions ? 1.0f : 0.0f,
        _gridSettings.axisLineWidth
    );

    // gridParams3: x=lodBias, y=antiAliasing, z=minFadeAlpha, w=majorMultiplier
    push.gridParams3 = glm::vec4(
        _gridSettings.lodBias,
        _gridSettings.antiAliasing ? 1.0f : 0.0f,
        _gridSettings.minFadeAlpha,
        _gridSettings.majorGridMultiplier
    );

    // Colors (zAxisColor.a is used for fadeFromCamera flag: 1.0 = fade from camera, 0.0 = fade from origin)
    push.minorColor = glm::vec4(_gridSettings.minorLineColor, 1.0f);
    push.majorColor = glm::vec4(_gridSettings.majorLineColor, 1.0f);
    push.xAxisColor = glm::vec4(_gridSettings.xAxisColor, 1.0f);
    push.zAxisColor = glm::vec4(_gridSettings.zAxisColor, _gridSettings.fadeFromCamera ? 1.0f : 0.0f);

    vkCmdPushConstants(cmd, gridPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
        sizeof(GridPushConstants), &push);

    vkCmdDrawIndexed(cmd, gridMesh.indexCount, 1, 0, 0, 0);
}














//void VulkanEngine::init_descriptors()
//{
//    // 1. Global descriptor pool (sabit havuz)
//    std::vector<DescriptorAllocator::PoolSizeRatio> sizes = {
//        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3 },
//        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3 },
//        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3 },
//    };
//
//    globalDescriptorAllocator.init_pool(_device, 10, sizes);
//    _mainDeletionQueue.push_function(
//        [&]() { vkDestroyDescriptorPool(_device, globalDescriptorAllocator.pool, nullptr); });
//
//    // 2. Descriptor set layoutları oluştur
//    {
//        DescriptorLayoutBuilder builder;
//        builder.add_binding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
//        _drawImageDescriptorLayout = builder.build(_device, VK_SHADER_STAGE_COMPUTE_BIT);
//    }
//    {
//        DescriptorLayoutBuilder builder;
//        builder.add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
//        _gpuSceneDataDescriptorLayout = builder.build(_device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
//    }
//
//    // 3. Layout'ları silme kuyruğuna ekle
//    _mainDeletionQueue.push_function([&]() {
//        vkDestroyDescriptorSetLayout(_device, _drawImageDescriptorLayout, nullptr);
//        vkDestroyDescriptorSetLayout(_device, _gpuSceneDataDescriptorLayout, nullptr);
//    });
//
//    // 4. Draw image için descriptor set oluştur ve güncelle
//    _drawImageDescriptors = globalDescriptorAllocator.allocate(_device, _drawImageDescriptorLayout);
//    {
//        DescriptorWriter writer;
//        writer.write_image(0, _drawImage.imageView, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
//        writer.update_set(_device, _drawImageDescriptors);
//    }
//
//    // 5. Her frame için growable descriptor pool oluştur
//    for (int i = 0; i < FRAME_OVERLAP; i++) {
//        std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> frame_sizes = {
//            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3 },
//            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3 },
//            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 },
//            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4 },
//        };
//
//        _frames[i]._frameDescriptors = DescriptorAllocatorGrowable{};
//        _frames[i]._frameDescriptors.init(_device, 1000, frame_sizes);
//        _mainDeletionQueue.push_function([&, i]() {
//            _frames[i]._frameDescriptors.destroy_pools(_device);
//        });
//    }
//}
void VulkanEngine::init_descriptors()
{
    // 1. Global descriptor pool (sabit havuz)
    std::vector<DescriptorAllocator::PoolSizeRatio> sizes = {
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 128 }, // Bindless için büyük
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 8 },
    };

    globalDescriptorAllocator.init_pool(_device, 32, sizes);
    _mainDeletionQueue.push_function(
        [&]() { vkDestroyDescriptorPool(_device, globalDescriptorAllocator.pool, nullptr); });

    // 2. Descriptor set layoutları oluştur

    // a) Compute pipeline için: drawImage (storage image)
    {
        DescriptorLayoutBuilder builder;
        builder.add_binding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);
        _drawImageDescriptorLayout = builder.build(_device, VK_SHADER_STAGE_COMPUTE_BIT);
    }

    // b) Scene data + bindless texture array için (set = 0)
    {
        DescriptorLayoutBuilder builder;
        builder.add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT); // sceneData
        builder.add_binding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, MAX_BINDLESS_TEXTURES, true);          // allTextures[]
        _gpuSceneDataDescriptorLayout = builder.build(_device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
    }

    // 3. Layout'ları silme kuyruğuna ekle
    _mainDeletionQueue.push_function([&]() {
        vkDestroyDescriptorSetLayout(_device, _drawImageDescriptorLayout, nullptr);
        vkDestroyDescriptorSetLayout(_device, _gpuSceneDataDescriptorLayout, nullptr);
        });

    // 4. Compute için descriptor set oluştur (draw image binding 0)
    _drawImageDescriptors = globalDescriptorAllocator.allocate(_device, _drawImageDescriptorLayout);
    {
        DescriptorWriter writer;
        writer.write_image(0, _drawImage.imageView, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        writer.update_set(_device, _drawImageDescriptors);
    }

    // 5. Her frame için growable descriptor allocator kur
    for (int i = 0; i < FRAME_OVERLAP; i++) {
        std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> frame_sizes = {
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 8 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 128 },
        };

        _frames[i]._frameDescriptors.init(_device, 1000, frame_sizes);
        _mainDeletionQueue.push_function([&, i]() {
            _frames[i]._frameDescriptors.destroy_pools(_device);
            });
    }
}




VkShaderModule VulkanEngine::load_shader_module(const char* filePath) {
    VkShaderModule shaderModule;
    if (!vkutil::load_shader_module(filePath, _device, &shaderModule)) {
        throw std::runtime_error("failed to load shader module!");
    }
    return shaderModule;
}





//void GLTFMetallic_Roughness::build_pipelines(VulkanEngine* engine)
//{
//    VkShaderModule meshFragShader;
//    if (!vkutil::load_shader_module("../../shaders/mesh.frag.spv", engine->_device, &meshFragShader)) {
//        fmt::println("Error when building the triangle fragment shader module");
//    }
//
//    VkShaderModule meshVertexShader;
//    if (!vkutil::load_shader_module("../../shaders/mesh.vert.spv", engine->_device, &meshVertexShader)) {
//        fmt::println("Error when building the triangle vertex shader module");
//    }
//
//    VkPushConstantRange matrixRange{};
//    matrixRange.offset = 0;
//    matrixRange.size = sizeof(GPUDrawPushConstants);
//    matrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
//
//    DescriptorLayoutBuilder layoutBuilder;
//    layoutBuilder.add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
//    layoutBuilder.add_binding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
//    layoutBuilder.add_binding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
//
//    materialLayout = layoutBuilder.build(engine->_device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
//
//    VkDescriptorSetLayout layouts[] = { engine->_gpuSceneDataDescriptorLayout,
//        materialLayout };
//
//    VkPipelineLayoutCreateInfo mesh_layout_info = vkinit::pipeline_layout_create_info();
//    mesh_layout_info.setLayoutCount = 2;
//    mesh_layout_info.pSetLayouts = layouts;
//    mesh_layout_info.pPushConstantRanges = &matrixRange;
//    mesh_layout_info.pushConstantRangeCount = 1;
//
//    VkPipelineLayout newLayout;
//    VK_CHECK(vkCreatePipelineLayout(engine->_device, &mesh_layout_info, nullptr, &newLayout));
//
//    opaquePipeline.layout = newLayout;
//    transparentPipeline.layout = newLayout;
//
//    // build the stage-create-info for both vertex and fragment stages. This lets
//    // the pipeline know the shader modules per stage
//    PipelineBuilder pipelineBuilder;
//
//    pipelineBuilder.set_shaders(meshVertexShader, meshFragShader);
//
//    pipelineBuilder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
//
//    pipelineBuilder.set_polygon_mode(VK_POLYGON_MODE_FILL);
//
//    pipelineBuilder.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
//
//    pipelineBuilder.set_multisampling_none();
//
//    pipelineBuilder.disable_blending();
//
//    pipelineBuilder.enable_depthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);
//
//    //render format
//    pipelineBuilder.set_color_attachment_format(engine->_drawImage.imageFormat);
//    pipelineBuilder.set_depth_format(engine->_depthImage.imageFormat);
//
//    // use the triangle layout we created
//    pipelineBuilder._pipelineLayout = newLayout;
//
//    // finally build the pipeline
//    opaquePipeline.pipeline = pipelineBuilder.build_pipeline(engine->_device);
//
//    // create the transparent variant
//    pipelineBuilder.enable_blending_additive();
//
//    pipelineBuilder.enable_depthtest(false, VK_COMPARE_OP_GREATER_OR_EQUAL);
//
//    transparentPipeline.pipeline = pipelineBuilder.build_pipeline(engine->_device);
//
//    vkDestroyShaderModule(engine->_device, meshFragShader, nullptr);
//    vkDestroyShaderModule(engine->_device, meshVertexShader, nullptr);
//}












//void GLTFMetallic_Roughness::build_pipelines(VulkanEngine* engine)
//{
//    // === Shader Yükle ===
//    VkShaderModule meshFragShader;
//    if (!vkutil::load_shader_module("../../shaders/mesh_pbr.frag.spv", engine->_device, &meshFragShader)) {
//        fmt::println("Error when building the PBR fragment shader module");
//    }
//
//    VkShaderModule meshVertexShader;
//    if (!vkutil::load_shader_module("../../shaders/mesh.vert.spv", engine->_device, &meshVertexShader)) {
//        fmt::println("Error when building the mesh vertex shader module");
//    }
//
//    // === Push Constant ===
//    VkPushConstantRange matrixRange{};
//    matrixRange.offset = 0;
//    matrixRange.size = sizeof(GPUDrawPushConstants);
//    matrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
//
//    // === Descriptor Layout ===
//    DescriptorLayoutBuilder layoutBuilder;
//    layoutBuilder.add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT); // materialData
//    layoutBuilder.add_binding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1000, true);          // allTextures[]
//    layoutBuilder.add_binding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);                      // metalRoughTex
//
//    materialLayout = layoutBuilder.build(engine->_device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
//
//    // === Pipeline Layout ===
//    VkDescriptorSetLayout layouts[] = {
//        engine->_gpuSceneDataDescriptorLayout, // set 0: sceneData + allTextures[]
//        materialLayout                          // set 1: materialData
//    };
//
//    VkPipelineLayoutCreateInfo mesh_layout_info = vkinit::pipeline_layout_create_info();
//    mesh_layout_info.setLayoutCount = 2;
//    mesh_layout_info.pSetLayouts = layouts;
//    mesh_layout_info.pPushConstantRanges = &matrixRange;
//    mesh_layout_info.pushConstantRangeCount = 1;
//
//    VkPipelineLayout newLayout;
//    VK_CHECK(vkCreatePipelineLayout(engine->_device, &mesh_layout_info, nullptr, &newLayout));
//
//    opaquePipeline.layout = newLayout;
//    transparentPipeline.layout = newLayout;
//
//    // === Pipeline Ayarları ===
//    PipelineBuilder pipelineBuilder;
//    pipelineBuilder.set_shaders(meshVertexShader, meshFragShader);
//    pipelineBuilder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
//    pipelineBuilder.set_polygon_mode(VK_POLYGON_MODE_FILL);
//    pipelineBuilder.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
//    pipelineBuilder.set_multisampling_none();
//    pipelineBuilder.disable_blending(); // opaque için blending yok
//    pipelineBuilder.enable_depthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);
//    pipelineBuilder.set_color_attachment_format(engine->_drawImage.imageFormat);
//    pipelineBuilder.set_depth_format(engine->_depthImage.imageFormat);
//    pipelineBuilder._pipelineLayout = newLayout;
//
//    // === Opaque Pipeline ===
//    opaquePipeline.pipeline = pipelineBuilder.build_pipeline(engine->_device);
//
//    // === Transparent Pipeline ===
//    pipelineBuilder.enable_blending_additive();
//    pipelineBuilder.enable_depthtest(false, VK_COMPARE_OP_GREATER_OR_EQUAL);
//    transparentPipeline.pipeline = pipelineBuilder.build_pipeline(engine->_device);
//
//    // === Shader Temizliği ===
//    vkDestroyShaderModule(engine->_device, meshFragShader, nullptr);
//    vkDestroyShaderModule(engine->_device, meshVertexShader, nullptr);
//}
//void GLTFMetallic_Roughness::build_pipelines(VulkanEngine* engine)
//{
//    VkShaderModule meshFragShader;
//    if (!vkutil::load_shader_module("../../shaders/mesh.frag.spv", engine->_device, &meshFragShader)) {
//        fmt::println("Error when building the PBR fragment shader module");
//    }
//
//    VkShaderModule meshVertexShader;
//    if (!vkutil::load_shader_module("../../shaders/mesh.vert.spv", engine->_device, &meshVertexShader)) {
//        fmt::println("Error when building the mesh vertex shader module");
//    }
//
//    // Push Constant: GPUDrawPushConstants
//    VkPushConstantRange matrixRange{};
//    matrixRange.offset = 0;
//    matrixRange.size = sizeof(GPUDrawPushConstants);
//    matrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
//
//    // Descriptor Layout: material constants + color texture + metal-rough texture
//    DescriptorLayoutBuilder layoutBuilder;
//    layoutBuilder.add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
//    layoutBuilder.add_binding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
//    layoutBuilder.add_binding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
//
//    materialLayout = layoutBuilder.build(engine->_device,
//        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
//
//    // Pipeline Layout (set 0: scene data, set 1: materialData)
//    VkDescriptorSetLayout layouts[] = {
//        engine->_gpuSceneDataDescriptorLayout,
//        materialLayout
//    };
//
//    VkPipelineLayoutCreateInfo mesh_layout_info = vkinit::pipeline_layout_create_info();
//    mesh_layout_info.setLayoutCount = 2;
//    mesh_layout_info.pSetLayouts = layouts;
//    mesh_layout_info.pPushConstantRanges = &matrixRange;
//    mesh_layout_info.pushConstantRangeCount = 1;
//
//    VkPipelineLayout newLayout;
//    VK_CHECK(vkCreatePipelineLayout(engine->_device, &mesh_layout_info, nullptr, &newLayout));
//
//    opaquePipeline.layout = newLayout;
//    transparentPipeline.layout = newLayout;
//
//    // Pipeline Setup
//    PipelineBuilder pipelineBuilder;
//    pipelineBuilder.set_shaders(meshVertexShader, meshFragShader);
//    pipelineBuilder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
//    pipelineBuilder.set_polygon_mode(VK_POLYGON_MODE_FILL);
//    pipelineBuilder.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
//    pipelineBuilder.set_multisampling_none();
//    pipelineBuilder.disable_blending();
//    pipelineBuilder.enable_depthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);
//
//    pipelineBuilder.set_color_attachment_format(engine->_drawImage.imageFormat);
//    pipelineBuilder.set_depth_format(engine->_depthImage.imageFormat);
//    pipelineBuilder._pipelineLayout = newLayout;
//
//    // Opaque pipeline
//    opaquePipeline.pipeline = pipelineBuilder.build_pipeline(engine->_device);
//
//    // Transparent pipeline
//    pipelineBuilder.enable_blending_additive();
//    pipelineBuilder.enable_depthtest(false, VK_COMPARE_OP_GREATER_OR_EQUAL);
//    transparentPipeline.pipeline = pipelineBuilder.build_pipeline(engine->_device);
//
//    // Shader cleanup
//    vkDestroyShaderModule(engine->_device, meshFragShader, nullptr);
//    vkDestroyShaderModule(engine->_device, meshVertexShader, nullptr);
//}

//void GLTFMetallic_Roughness::build_pipelines(VulkanEngine* engine)
//{
//    fmt::println("[Pipelines] Başlatılıyor...");
//
//    VkShaderModule meshFragShader;
//    if (!vkutil::load_shader_module("../../shaders/mesh_pbr.frag.spv", engine->_device, &meshFragShader)) {
//        fmt::println("[Pipelines] mesh_pbr.frag.spv yüklenemedi!");
//    }
//
//    VkShaderModule meshVertexShader;
//    if (!vkutil::load_shader_module("../../shaders/mesh.vert.spv", engine->_device, &meshVertexShader)) {
//        fmt::println("[Pipelines] mesh.vert.spv yüklenemedi!");
//    }
//
//    VkPushConstantRange matrixRange{};
//    matrixRange.offset = 0;
//    matrixRange.size = /*sizeof(GPUDrawPushConstants);*/176;
//    matrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
//
//    fmt::println("[Pipelines] Descriptor layout oluşturuluyor...");
//    DescriptorLayoutBuilder layoutBuilder;
//    layoutBuilder.add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
//    layoutBuilder.add_binding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
//    layoutBuilder.add_binding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
//
//    materialLayout = layoutBuilder.build(engine->_device,
//        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
//
//    VkDescriptorSetLayout layouts[] = {
//        engine->_gpuSceneDataDescriptorLayout,
//        materialLayout
//    };
//
//    VkPipelineLayoutCreateInfo mesh_layout_info = vkinit::pipeline_layout_create_info();
//    mesh_layout_info.setLayoutCount = 2;
//    mesh_layout_info.pSetLayouts = layouts;
//    mesh_layout_info.pPushConstantRanges = &matrixRange;
//    mesh_layout_info.pushConstantRangeCount = 1;
//
//    VkPipelineLayout newLayout;
//    VK_CHECK(vkCreatePipelineLayout(engine->_device, &mesh_layout_info, nullptr, &newLayout));
//
//    opaquePipeline.layout = newLayout;
//    transparentPipeline.layout = newLayout;
//
//    fmt::println("[Pipelines] Pipeline yapısı inşa ediliyor...");
//    PipelineBuilder pipelineBuilder;
//    pipelineBuilder.set_shaders(meshVertexShader, meshFragShader);
//    pipelineBuilder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
//    pipelineBuilder.set_polygon_mode(VK_POLYGON_MODE_FILL);
//    pipelineBuilder.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
//    pipelineBuilder.set_multisampling_none();
//    pipelineBuilder.disable_blending();
//    pipelineBuilder.enable_depthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);
//
//    pipelineBuilder.set_color_attachment_format(engine->_drawImage.imageFormat);
//    pipelineBuilder.set_depth_format(engine->_depthImage.imageFormat);
//    pipelineBuilder._pipelineLayout = newLayout;
//
//    fmt::println("[Pipelines] Opaque pipeline oluşturuluyor...");
//    opaquePipeline.pipeline = pipelineBuilder.build_pipeline(engine->_device);
//
//    fmt::println("[Pipelines] Transparent pipeline oluşturuluyor...");
//    pipelineBuilder.enable_blending_additive();
//    pipelineBuilder.enable_depthtest(false, VK_COMPARE_OP_GREATER_OR_EQUAL);
//    transparentPipeline.pipeline = pipelineBuilder.build_pipeline(engine->_device);
//
//    vkDestroyShaderModule(engine->_device, meshFragShader, nullptr);
//    vkDestroyShaderModule(engine->_device, meshVertexShader, nullptr);
//
//    fmt::println("[Pipelines] Başarıyla tamamlandı.");
//}


void GLTFMetallic_Roughness::build_pipelines(VulkanEngine* engine)
{
    fmt::print("[Pipelines] Baslatiliyor...\n");

    // Shader modüllerini yükle
    VkShaderModule meshFragShader;
    if (!vkutil::load_shader_module("../../shaders/mesh.frag.spv", engine->_device, &meshFragShader)) {
        fmt::print("[Pipelines] mesh_pbr.frag.spv yuklenemedi!\n");
    }

    VkShaderModule meshVertexShader;
    if (!vkutil::load_shader_module("../../shaders/mesh.vert.spv", engine->_device, &meshVertexShader)) {
        fmt::print("[Pipelines] mesh.vert.spv yuklenemedi!\n");
    }

    // Push constant tanımı (shader'da 176 byte kullanılıyor)
    //VkPushConstantRange matrixRange{};
    //matrixRange.offset = 0;
    //matrixRange.size = sizeof(GPUDrawPushConstants); // doğru olan bu

    //matrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    VkPushConstantRange matrixRange{};
    matrixRange.offset = 0;
    matrixRange.size = sizeof(GPUDrawPushConstants); // = 168


    matrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;


    // Descriptor layout tanımı
    fmt::print("[Pipelines] Descriptor layout olusturuluyor...\n");
    DescriptorLayoutBuilder layoutBuilder;
    layoutBuilder.add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT); // SceneData
    layoutBuilder.add_binding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT); // colorTex
    layoutBuilder.add_binding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT); // metalRoughTex

    materialLayout = layoutBuilder.build(engine->_device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

    VkDescriptorSetLayout layouts[] = {
        engine->_gpuSceneDataDescriptorLayout,
        materialLayout
    };

    // Pipeline layout oluşturuluyor
    VkPipelineLayoutCreateInfo mesh_layout_info = vkinit::pipeline_layout_create_info();
    mesh_layout_info.setLayoutCount = 2;
    mesh_layout_info.pSetLayouts = layouts;
    mesh_layout_info.pPushConstantRanges = &matrixRange;
    mesh_layout_info.pushConstantRangeCount = 1;

    VkPipelineLayout newLayout;
    VK_CHECK(vkCreatePipelineLayout(engine->_device, &mesh_layout_info, nullptr, &newLayout));

    opaquePipeline.layout = newLayout;
    transparentPipeline.layout = newLayout;

    // PipelineBuilder kuruluyor
    fmt::print("[Pipelines] Pipeline yapisi insa ediliyor...\n");
    PipelineBuilder pipelineBuilder;
    pipelineBuilder.set_shaders(meshVertexShader, meshFragShader);
    pipelineBuilder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    pipelineBuilder.set_polygon_mode(VK_POLYGON_MODE_FILL);
    pipelineBuilder.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
    pipelineBuilder.set_multisampling_none();
    pipelineBuilder.disable_blending();
    pipelineBuilder.enable_depthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);

    pipelineBuilder.set_color_attachment_format(engine->_drawImage.imageFormat);
    pipelineBuilder.set_depth_format(engine->_depthImage.imageFormat);
    pipelineBuilder._pipelineLayout = newLayout;

    // Dynamic rendering bilgisi
    pipelineBuilder._renderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    pipelineBuilder._renderInfo.colorAttachmentCount = 1;
    pipelineBuilder._renderInfo.pColorAttachmentFormats = &pipelineBuilder._colorAttachmentformat;
    pipelineBuilder._renderInfo.depthAttachmentFormat = engine->_depthImage.imageFormat;
    pipelineBuilder._renderInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;

    // Opaque pipeline
    fmt::print("[Pipelines] Opaque pipeline olusturuluyor...\n");
    opaquePipeline.pipeline = pipelineBuilder.build_pipeline(engine->_device);

    // Transparent pipeline
    fmt::print("[Pipelines] Transparent pipeline olusturuluyor...\n");
    pipelineBuilder.enable_blending_additive();
    pipelineBuilder.enable_depthtest(false, VK_COMPARE_OP_GREATER_OR_EQUAL);
    transparentPipeline.pipeline = pipelineBuilder.build_pipeline(engine->_device);

    // Shader modüllerini temizle
    vkDestroyShaderModule(engine->_device, meshFragShader, nullptr);
    vkDestroyShaderModule(engine->_device, meshVertexShader, nullptr);

    fmt::print("[Pipelines] Basariyla tamamlandi.\n");
}
























//void GLTFMetallic_Roughness::build_pipelines(VulkanEngine* engine)
//{
//    VkShaderModule meshFragShader;
//    if (!vkutil::load_shader_module("../../shaders/mesh_pbr.frag.spv", engine->_device, &meshFragShader)) {
//        fmt::println("Error when building the triangle fragment shader module");
//    }
//
//    VkShaderModule meshVertexShader;
//    if (!vkutil::load_shader_module("../../shaders/mesh.vert.spv", engine->_device, &meshVertexShader)) {
//        fmt::println("Error when building the triangle vertex shader module");
//    }
//
//    VkPushConstantRange matrixRange{};
//    matrixRange.offset = 0;
//    matrixRange.size = sizeof(GPUDrawPushConstants);
//    matrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
//
//    DescriptorLayoutBuilder layoutBuilder;
//    layoutBuilder.add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
//
//    materialLayout = layoutBuilder.build(engine->_device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
//
//    VkDescriptorSetLayout layouts[] = { engine->_gpuSceneDataDescriptorLayout,
//        materialLayout };
//
//    VkPipelineLayoutCreateInfo mesh_layout_info = vkinit::pipeline_layout_create_info();
//    mesh_layout_info.setLayoutCount = 2;
//    mesh_layout_info.pSetLayouts = layouts;
//    mesh_layout_info.pPushConstantRanges = &matrixRange;
//    mesh_layout_info.pushConstantRangeCount = 1;
//
//    VkPipelineLayout newLayout;
//    VK_CHECK(vkCreatePipelineLayout(engine->_device, &mesh_layout_info, nullptr, &newLayout));
//
//    opaquePipeline.layout = newLayout;
//    transparentPipeline.layout = newLayout;
//
//    // build the stage-create-info for both vertex and fragment stages. This lets
//    // the pipeline know the shader modules per stage
//    PipelineBuilder pipelineBuilder;
//
//    pipelineBuilder.set_shaders(meshVertexShader, meshFragShader);
//
//    pipelineBuilder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
//
//    pipelineBuilder.set_polygon_mode(VK_POLYGON_MODE_FILL);
//
//    pipelineBuilder.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
//
//    pipelineBuilder.set_multisampling_none();
//
//    pipelineBuilder.disable_blending();
//
//    pipelineBuilder.enable_depthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);
//
//    //render format
//    pipelineBuilder.set_color_attachment_format(engine->_drawImage.imageFormat);
//    pipelineBuilder.set_depth_format(engine->_depthImage.imageFormat);
//
//    // use the triangle layout we created
//    pipelineBuilder._pipelineLayout = newLayout;
//
//    // finally build the pipeline
//    opaquePipeline.pipeline = pipelineBuilder.build_pipeline(engine->_device);
//
//    // create the transparent variant
//    pipelineBuilder.enable_blending_additive();
//
//    pipelineBuilder.enable_depthtest(false, VK_COMPARE_OP_GREATER_OR_EQUAL);
//
//    transparentPipeline.pipeline = pipelineBuilder.build_pipeline(engine->_device);
//
//    vkDestroyShaderModule(engine->_device, meshFragShader, nullptr);
//    vkDestroyShaderModule(engine->_device, meshVertexShader, nullptr);
//}


void GLTFMetallic_Roughness::clear_resources(VkDevice device)
{

}






//MaterialInstance GLTFMetallic_Roughness::write_material(VkDevice device, MaterialPass pass, const MaterialResources& resources, DescriptorAllocatorGrowable& descriptorAllocator)
//{
//    MaterialInstance matData;
//    matData.passType = pass;
//    if (pass == MaterialPass::Transparent) {
//        matData.pipeline = &transparentPipeline;
//    }
//    else {
//        matData.pipeline = &opaquePipeline;
//    }
//
//    matData.materialSet = descriptorAllocator.allocate(device, materialLayout);
//
//
//    writer.clear();
//    writer.write_buffer(0, resources.dataBuffer, sizeof(MaterialConstants), resources.dataBufferOffset, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
//    writer.write_image(1, resources.colorImage.imageView, resources.colorSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
//    writer.write_image(2, resources.metalRoughImage.imageView, resources.metalRoughSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
//
//    writer.update_set(device, matData.materialSet);
//
//    return matData;
//}

//MaterialInstance GLTFMetallic_Roughness::write_material(VkDevice device, MaterialPass pass, const MaterialResources& resources, DescriptorAllocatorGrowable& descriptorAllocator)
//{
//    fmt::println("[Material] MaterialInstance yazılıyor...");
//
//    MaterialInstance matData;
//    matData.passType = pass;
//    matData.pipeline = (pass == MaterialPass::Transparent) ? &transparentPipeline : &opaquePipeline;
//
//    matData.materialSet = descriptorAllocator.allocate(device, materialLayout);
//
//    writer.clear();
//
//    writer.write_buffer(0, resources.dataBuffer, sizeof(MaterialConstants), resources.dataBufferOffset, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
//    writer.write_image(1, resources.colorImage.imageView, resources.colorSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
//    writer.write_image(2, resources.metalRoughImage.imageView, resources.metalRoughSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
//
//    writer.update_set(device, matData.materialSet);
//
//    fmt::println("[Material] MaterialInstance oluşturuldu.");
//    return matData;
//}

//MaterialInstance GLTFMetallic_Roughness::write_material(VkDevice device, MaterialPass pass, const MaterialResources& resources, DescriptorAllocatorGrowable& descriptorAllocator)



MaterialInstance GLTFMetallic_Roughness::write_material(
    VkDevice device,
    MaterialPass pass,
    const MaterialResources& resources,
    DescriptorAllocatorGrowable& descriptorAllocator)
{
    fmt::print("[Material] MaterialInstance yaziliyor...\n");

    MaterialInstance matData;
    matData.passType = pass;
    matData.pipeline = (pass == MaterialPass::Transparent) ? &transparentPipeline : &opaquePipeline;

    // Descriptor set oluştur
    matData.materialSet = descriptorAllocator.allocate(device, materialLayout);

    writer.clear();

    // Uniform buffer yaz
    writer.write_buffer(
        0,
        resources.dataBuffer,
        sizeof(MaterialConstants),
        resources.dataBufferOffset,
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
    );

    // Texture slotları
    writer.write_image(
        1,
        resources.colorImage.imageView,
        resources.colorSampler,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
    );

    writer.write_image(
        2,
        resources.metalRoughImage.imageView,
        resources.metalRoughSampler,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
    );

    // Descriptor set güncelle
    writer.update_set(device, matData.materialSet);

    fmt::print("[Material] MaterialInstance olusturuldu.\n");
    return matData;
}





//MaterialInstance GLTFMetallic_Roughness::write_material(VkDevice device, MaterialPass pass, const MaterialResources& resources, DescriptorAllocatorGrowable& descriptorAllocator)
//{
//    MaterialInstance matData;
//    matData.passType = pass;
//    matData.pipeline = (pass == MaterialPass::Transparent) ? &transparentPipeline : &opaquePipeline;
//
//    // allocate descriptor set for set = 1 (materialData only)
//    matData.materialSet = descriptorAllocator.allocate(device, materialLayout);
//
//    writer.clear();
//
//    // write bindings
//    writer.write_buffer(0, resources.dataBuffer, sizeof(MaterialConstants), resources.dataBufferOffset, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
//    writer.write_image(1, resources.colorImage.imageView, resources.colorSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
//    writer.write_image(2, resources.metalRoughImage.imageView, resources.metalRoughSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
//
//    writer.update_set(device, matData.materialSet);
//
//    return matData;
//}











//void MeshNode::Draw(const glm::mat4& topMatrix, DrawContext& ctx) {
//    glm::mat4 nodeMatrix = topMatrix * worldTransform;
//
//    for (auto& s : mesh->surfaces) {
//        RenderObject def;
//        def.indexCount = s.count;
//        def.firstIndex = s.startIndex;
//        def.indexBuffer = mesh->meshBuffers.indexBuffer.buffer;
//        def.material = &s.material->data;
//        def.bounds = s.bounds;
//        def.transform = nodeMatrix;
//        def.vertexBufferAddress = mesh->meshBuffers.vertexBufferAddress;
//
//        // **🚀 Nesne görünür mü?**
//        //if (!VulkanEngine::Get().is_visible(def, ctx.viewproj)) {
//        //    continue;  // Eğer görünmüyorsa çizim listesine ekleme
//        //}
//
//        if (s.material->data.passType == MaterialPass::Transparent) {
//            ctx.TransparentSurfaces.push_back(def);
//        }
//        else {
//            ctx.OpaqueSurfaces.push_back(def);
//        }
//    }
//
//    // **Alt düğümleri de kontrol et**
//    Node::Draw(topMatrix, ctx);
//}




//void MeshNode::Draw(const glm::mat4& topMatrix, DrawContext& ctx)
//{
//    if (!mesh)
//        return;  // Mesh yoksa işlem yapma
//
//    glm::mat4 nodeMatrix = topMatrix * worldTransform;
//
//    for (auto& s : mesh->surfaces)
//    {
//        RenderObject def;
//        def.indexCount = s.count;
//        def.firstIndex = s.startIndex;
//        def.indexBuffer = mesh->meshBuffers.indexBuffer.buffer;
//        def.vertexBufferAddress = mesh->meshBuffers.vertexBufferAddress;
//        def.material = &s.material->data;
//        def.bounds = s.bounds;
//        def.transform = nodeMatrix;
//
//        // 🚀 İsteğe bağlı frustum culling (aktif etmek istersen aç)
//        // if (!VulkanEngine::Get().is_visible(def, ctx.viewproj))
//        //     continue;
//
//        if (s.material->data.passType == MaterialPass::Transparent)
//            ctx.TransparentSurfaces.push_back(def);
//        else
//            ctx.OpaqueSurfaces.push_back(def);
//    }
//
//    Node::Draw(nodeMatrix, ctx);  // Alt node'lar için
//}
// 
//void MeshNode::Draw(const glm::mat4& topMatrix, DrawContext& ctx)
//{
//    if (!mesh)
//        return;  // Mesh yoksa hiçbir şey çizilmez
//
//    glm::mat4 nodeMatrix = topMatrix * worldTransform;
//
//    for (auto& s : mesh->surfaces)
//    {
//        RenderObject def;
//        def.indexCount = s.count;
//        def.firstIndex = s.startIndex;
//        def.indexBuffer = mesh->meshBuffers.indexBuffer.buffer;
//        def.vertexBufferAddress = mesh->meshBuffers.vertexBufferAddress;
//        def.material = &s.material->data;
//        def.bounds = s.bounds;
//        def.transform = nodeMatrix;
//
//        def.name = mesh->name;     // Obje adı (Visible Objects için)
//        def.nodePointer = this;    // 🎯 Bu RenderObject hangi MeshNode'a ait: BELLİ!
//
//        if (s.material->data.passType == MaterialPass::Transparent)
//            ctx.TransparentSurfaces.push_back(def);
//        else
//            ctx.OpaqueSurfaces.push_back(def);
//    }
//
//    // Alt node'ları çizdir (recursive)
//    Node::Draw(nodeMatrix, ctx);
//}
void MeshNode::Draw(const glm::mat4& topMatrix, DrawContext& ctx)
{
    if (!mesh || mesh->meshBuffers.indexBuffer.buffer == VK_NULL_HANDLE)
        return;  // 🎯 Mesh veya index buffer yoksa RenderObject oluşturulmaz

    glm::mat4 nodeMatrix = topMatrix * worldTransform;

    for (auto& s : mesh->surfaces)
    {
        RenderObject def;
        def.indexCount = s.count;
        def.firstIndex = s.startIndex;

        def.indexBuffer = mesh->meshBuffers.indexBuffer.buffer;
        def.vertexBuffer = mesh->meshBuffers.vertexBuffer.buffer;
        def.vertexBufferAddress = mesh->meshBuffers.vertexBufferAddress;

        def.material = &s.material->data;
        def.bounds = s.bounds;
        def.transform = nodeMatrix;

        def.name = mesh->name;
        def.nodePointer = this;   // 🎯 RenderObject → MeshNode eşlemesi

        if (s.material->data.passType == MaterialPass::Transparent)
            ctx.TransparentSurfaces.push_back(def);
        else
            ctx.OpaqueSurfaces.push_back(def);
    }

    Node::Draw(nodeMatrix, ctx);  // Alt node'ları recursive çiz
}



// TextureCache::AddTexture is now defined inline in vk_types.h