
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
#include "stb_image.h"  // For texture loading in create_primitive_material
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

    // Initialize default primitive material (needs _whiteImage from init_default_data)
    init_default_primitive_material();

    // Initialize shadow mapping system
    init_shadow_map();
    init_point_light_shadow_maps();  // Point light cubemap shadows
    init_shadow_pipeline();

    // Initialize GPU-driven rendering system (optional, disabled by default)
    init_gpu_driven_rendering();

    // Initialize post-processing system
    init_post_processing();

    // Initialize path tracing system (optional, for path traced mode)
    init_path_tracer();

    // Initialize environment map / skybox system
    init_environment_map();

    // Initialize multi-probe reflection system
    init_reflection_probes();

    init_renderables();

    init_imgui();

    // Initialize engine subsystems
    initSubsystems();

    // Register shader pipelines for ShaderDebugView
    shaderPipelines.clear();
    if (_meshPipeline != VK_NULL_HANDLE) {
        ShaderPipelineInfo meshShader;
        meshShader.name = "Mesh Pipeline";
        meshShader.vertPath = "shaders/mesh.vert.spv";
        meshShader.fragPath = "shaders/mesh.frag.spv";
        meshShader.pipeline = _meshPipeline;
        meshShader.layout = _meshPipelineLayout;
        meshShader.isValid = true;
        meshShader.compileTimeMs = 45.2f;
        meshShader.uniformCount = 6;
        meshShader.textureCount = 2;
        shaderPipelines.push_back(meshShader);
    }
    if (_primitivePipeline != VK_NULL_HANDLE) {
        ShaderPipelineInfo primitiveShader;
        primitiveShader.name = "Primitive Pipeline";
        primitiveShader.vertPath = "shaders/primitive.vert.spv";
        primitiveShader.fragPath = "shaders/primitive.frag.spv";
        primitiveShader.pipeline = _primitivePipeline;
        primitiveShader.layout = _primitivePipelineLayout;
        primitiveShader.isValid = true;
        primitiveShader.compileTimeMs = 32.1f;
        primitiveShader.uniformCount = 4;
        primitiveShader.textureCount = 0;
        shaderPipelines.push_back(primitiveShader);
    }
    if (_shadedPipeline != VK_NULL_HANDLE) {
        ShaderPipelineInfo shadedShader;
        shadedShader.name = "Shaded Pipeline";
        shadedShader.vertPath = "shaders/shaded.vert.spv";
        shadedShader.fragPath = "shaders/shaded.frag.spv";
        shadedShader.pipeline = _shadedPipeline;
        shadedShader.layout = _shadedPipelineLayout;
        shadedShader.isValid = true;
        shadedShader.compileTimeMs = 56.8f;
        shadedShader.uniformCount = 8;
        shadedShader.textureCount = 4;
        shaderPipelines.push_back(shadedShader);
    }
    if (_wireframePipeline != VK_NULL_HANDLE) {
        ShaderPipelineInfo wireframeShader;
        wireframeShader.name = "Wireframe Pipeline";
        wireframeShader.vertPath = "shaders/wireframe.vert.spv";
        wireframeShader.fragPath = "shaders/wireframe.frag.spv";
        wireframeShader.pipeline = _wireframePipeline;
        wireframeShader.layout = _wireframePipelineLayout;
        wireframeShader.isValid = true;
        wireframeShader.compileTimeMs = 12.4f;
        wireframeShader.uniformCount = 2;
        wireframeShader.textureCount = 0;
        shaderPipelines.push_back(wireframeShader);
    }
    if (gridPipeline != VK_NULL_HANDLE) {
        ShaderPipelineInfo gridShader;
        gridShader.name = "Grid Pipeline";
        gridShader.vertPath = "shaders/grid.vert.spv";
        gridShader.fragPath = "shaders/grid.frag.spv";
        gridShader.pipeline = gridPipeline;
        gridShader.layout = gridPipelineLayout;
        gridShader.isValid = true;
        gridShader.compileTimeMs = 8.3f;
        gridShader.uniformCount = 2;
        gridShader.textureCount = 0;
        shaderPipelines.push_back(gridShader);
    }

    // Create demo animation for AnimationView
    AnimationClipData walkClip;
    walkClip.name = "Walk";
    walkClip.duration = 1.0f;
    walkClip.loop = true;
    walkClip.speed = 1.0f;
    AnimationTrackData hipTrack;
    hipTrack.targetNode = "Hips";
    hipTrack.property = "translation";
    hipTrack.keyframes.push_back({0.0f, glm::vec4(0, 1, 0, 0), 2});
    hipTrack.keyframes.push_back({0.5f, glm::vec4(0, 1.1f, 0, 0), 2});
    hipTrack.keyframes.push_back({1.0f, glm::vec4(0, 1, 0, 0), 2});
    walkClip.tracks.push_back(hipTrack);
    animationClips.push_back(walkClip);

    // Create demo skeleton
    SkeletonData humanSkeleton;
    humanSkeleton.name = "Humanoid";
    humanSkeleton.bones.push_back({"Root", -1, glm::vec3(0, 0, 0), glm::quat(1, 0, 0, 0), glm::vec3(1)});
    humanSkeleton.bones.push_back({"Hips", 0, glm::vec3(0, 1, 0), glm::quat(1, 0, 0, 0), glm::vec3(1)});
    humanSkeleton.bones.push_back({"Spine", 1, glm::vec3(0, 0.3f, 0), glm::quat(1, 0, 0, 0), glm::vec3(1)});
    humanSkeleton.bones.push_back({"Chest", 2, glm::vec3(0, 0.3f, 0), glm::quat(1, 0, 0, 0), glm::vec3(1)});
    humanSkeleton.bones.push_back({"Head", 3, glm::vec3(0, 0.3f, 0), glm::quat(1, 0, 0, 0), glm::vec3(1)});
    humanSkeleton.bones.push_back({"LeftArm", 3, glm::vec3(0.2f, 0, 0), glm::quat(1, 0, 0, 0), glm::vec3(1)});
    humanSkeleton.bones.push_back({"RightArm", 3, glm::vec3(-0.2f, 0, 0), glm::quat(1, 0, 0, 0), glm::vec3(1)});
    humanSkeleton.bones.push_back({"LeftLeg", 1, glm::vec3(0.1f, 0, 0), glm::quat(1, 0, 0, 0), glm::vec3(1)});
    humanSkeleton.bones.push_back({"RightLeg", 1, glm::vec3(-0.1f, 0, 0), glm::quat(1, 0, 0, 0), glm::vec3(1)});
    skeletons.push_back(humanSkeleton);

    // everything went fine
    _isInitialized = true;

    mainCamera.position = glm::vec3(0, 0, 5);
    mainCamera.yaw = 0.0f;
    mainCamera.pitch = 0.0f;
}

void VulkanEngine::cleanup()
{
    if (_isInitialized) {

        // Make sure the GPU has stopped doing its things
        vkDeviceWaitIdle(_device);

        // Process any pending scene unloads before cleanup
        _pendingSceneUnloads.clear();  // Don't need deferred anymore, we'll clear everything

        // Cleanup post-processing and path tracing systems
        cleanup_path_tracer();
        cleanup_reflection_probes();
        cleanup_environment_map();
        cleanup_post_processing();

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
        if (_defaultCubemap.image != VK_NULL_HANDLE) {
            destroy_image(_defaultCubemap);
            _defaultCubemap.image = VK_NULL_HANDLE;
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

        // Default primitive material data buffer
        if (_primitiveMaterialDataBuffer.buffer != VK_NULL_HANDLE) {
            destroy_buffer(_primitiveMaterialDataBuffer);
            _primitiveMaterialDataBuffer.buffer = VK_NULL_HANDLE;
        }

        // Dynamic primitive material resources (images and buffers from create_primitive_material)
        for (auto& img : _dynamicPrimitiveMaterialImages) {
            if (img.image != VK_NULL_HANDLE) {
                destroy_image(img);
            }
        }
        _dynamicPrimitiveMaterialImages.clear();
        for (auto& buf : _dynamicPrimitiveMaterialBuffers) {
            if (buf.buffer != VK_NULL_HANDLE) {
                destroy_buffer(buf);
            }
        }
        _dynamicPrimitiveMaterialBuffers.clear();

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
    sceneData.ambientColor = glm::vec4(0.15f, 0.15f, 0.18f, 1.0f); // RGB + intensity (slightly blue-tinted sky ambient)
    glm::vec3 sunDir = glm::normalize(glm::vec3(-1.0f, -3.0f, -1.0f));
    sceneData.sunlightDirection = glm::vec4(sunDir, 2.0f); // direction + intensity
    sceneData.sunlightColor = glm::vec4(1.0f, 0.95f, 0.8f, 1.0f); // warm white

    // === Camera Position (will be updated each frame) ===
    sceneData.cameraPosition = glm::vec4(0.0f, 0.0f, 5.0f, 1.0f);

    // === Point Light Array Initialization ===
    sceneData.pointLightCount = 0;

    // === Shadow Settings Initialization ===
    sceneData.shadowBias = shadowBias;
    sceneData.shadowNormalBias = shadowNormalBias;
    sceneData.shadowsEnabled = shadowsEnabled ? 1 : 0;
    sceneData.cascadeSplits = glm::vec4(0.0f);
    for (int i = 0; i < SHADOW_CASCADE_COUNT; i++) {
        sceneData.shadowMatrices[i] = glm::mat4(1.0f);
    }

    // === Color Grading Defaults ===
    sceneData.colorGrading = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f); // exposure=1, contrast=1, saturation=1, vibrance=0
    sceneData.colorTemperature = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f); // temperature=0, tint=0, tonemapOp=0(ACES), unused
}

// =============================================================================
// SHADOW MAP INITIALIZATION
// =============================================================================

void VulkanEngine::init_shadow_map() {
    // Create shadow map image (depth-only, 2048x2048)
    VkExtent3D shadowExtent = { SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, 1 };

    VkImageCreateInfo imgInfo{};
    imgInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imgInfo.imageType = VK_IMAGE_TYPE_2D;
    imgInfo.format = VK_FORMAT_D32_SFLOAT;
    imgInfo.extent = shadowExtent;
    imgInfo.mipLevels = 1;
    imgInfo.arrayLayers = 1;
    imgInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imgInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imgInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imgInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    VK_CHECK(vmaCreateImage(_allocator, &imgInfo, &allocInfo,
        &_shadowMapImage.image, &_shadowMapImage.allocation, nullptr));

    _shadowMapImage.imageExtent = shadowExtent;
    _shadowMapImage.imageFormat = VK_FORMAT_D32_SFLOAT;

    // Create shadow map image view
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = _shadowMapImage.image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_D32_SFLOAT;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VK_CHECK(vkCreateImageView(_device, &viewInfo, nullptr, &_shadowMapView));
    _shadowMapImage.imageView = _shadowMapView;

    // Create shadow sampler with comparison mode for PCF
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE; // Outside shadow = lit
    samplerInfo.compareEnable = VK_FALSE; // We do comparison in shader for more control
    samplerInfo.compareOp = VK_COMPARE_OP_LESS;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 1.0f;

    VK_CHECK(vkCreateSampler(_device, &samplerInfo, nullptr, &_shadowSampler));

    // Add to deletion queue
    _mainDeletionQueue.push_function([=, this]() {
        vkDestroySampler(_device, _shadowSampler, nullptr);
        vkDestroyImageView(_device, _shadowMapView, nullptr);
        vmaDestroyImage(_allocator, _shadowMapImage.image, _shadowMapImage.allocation);
    });

    fmt::print("[Shadow] Shadow map initialized: {}x{} D32_SFLOAT\n", SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
}

void VulkanEngine::init_shadow_pipeline() {
    // Load shadow depth-only shaders
    VkShaderModule vertShader;
    if (!vkutil::load_shader_module("../../shaders/shadow.vert.spv", _device, &vertShader)) {
        fmt::print("[Shadow] Warning: shadow.vert.spv not found, shadows disabled\n");
        shadowsEnabled = false;
        return;
    }

    // Push constant for world matrix only
    VkPushConstantRange pushConstant{};
    pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstant.offset = 0;
    pushConstant.size = sizeof(glm::mat4) + sizeof(int32_t); // worldMatrix + cascadeIndex

    // Pipeline layout with scene data descriptor (for light matrices)
    VkDescriptorSetLayout layouts[] = { _gpuSceneDataDescriptorLayout };

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = layouts;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &pushConstant;

    VK_CHECK(vkCreatePipelineLayout(_device, &layoutInfo, nullptr, &_shadowPipelineLayout));

    // Vertex input description
    VertexInputDescription vertexDesc = Vertex::get_vertex_description();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = (uint32_t)vertexDesc.bindings.size();
    vertexInputInfo.pVertexBindingDescriptions = vertexDesc.bindings.data();
    vertexInputInfo.vertexAttributeDescriptionCount = (uint32_t)vertexDesc.attributes.size();
    vertexInputInfo.pVertexAttributeDescriptions = vertexDesc.attributes.data();

    // Input assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = vkinit::input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

    // Rasterizer - front face culling to reduce shadow acne
    VkPipelineRasterizationStateCreateInfo rasterizer = vkinit::rasterization_state_create_info(VK_POLYGON_MODE_FILL);
    rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT; // Cull front faces for shadow maps
    rasterizer.depthBiasEnable = VK_TRUE;
    rasterizer.depthBiasConstantFactor = 1.25f;
    rasterizer.depthBiasSlopeFactor = 1.75f;

    // Multisampling - none
    VkPipelineMultisampleStateCreateInfo multisampling = vkinit::multisampling_state_create_info();

    // Depth testing
    VkPipelineDepthStencilStateCreateInfo depthStencil = vkinit::depth_stencil_create_info(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);

    // Viewport/scissor (dynamic, values not needed)
    VkViewport viewport{};
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = (float)SHADOW_MAP_SIZE;
    viewport.height = (float)SHADOW_MAP_SIZE;
    viewport.minDepth = 0.f;
    viewport.maxDepth = 1.f;

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = { SHADOW_MAP_SIZE, SHADOW_MAP_SIZE };

    // Viewport state
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    // Dynamic state for viewport/scissor
    VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    // Color blend state (empty for depth-only)
    VkPipelineColorBlendStateCreateInfo colorBlendState{};
    colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendState.attachmentCount = 0;
    colorBlendState.pAttachments = nullptr;

    // Dynamic rendering info for depth-only
    VkPipelineRenderingCreateInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    renderingInfo.colorAttachmentCount = 0;
    renderingInfo.pColorAttachmentFormats = nullptr;
    renderingInfo.depthAttachmentFormat = VK_FORMAT_D32_SFLOAT;

    // Shader stages (vertex only for depth pass)
    VkPipelineShaderStageCreateInfo shaderStage = vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, vertShader);

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = &renderingInfo;
    pipelineInfo.stageCount = 1;
    pipelineInfo.pStages = &shaderStage;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlendState;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = _shadowPipelineLayout;

    VK_CHECK(vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_shadowPipeline));

    vkDestroyShaderModule(_device, vertShader, nullptr);

    _mainDeletionQueue.push_function([=, this]() {
        vkDestroyPipeline(_device, _shadowPipeline, nullptr);
        vkDestroyPipelineLayout(_device, _shadowPipelineLayout, nullptr);
    });

    fmt::print("[Shadow] Shadow pipeline initialized\n");
}

// =============================================================================
// POINT LIGHT SHADOW MAPS
// =============================================================================

void VulkanEngine::init_point_light_shadow_maps() {
    fmt::print("[Shadow] Initializing point light shadow cubemaps...\n");

    // Create cubemap sampler for point light shadows
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 1.0f;

    VK_CHECK(vkCreateSampler(_device, &samplerInfo, nullptr, &_pointLightShadowSampler));

    // Create cubemap shadow maps for each slot
    for (uint32_t i = 0; i < MAX_SHADOW_POINT_LIGHTS; i++) {
        auto& shadowData = _pointLightShadows[i];

        // Create cubemap image (6 faces)
        VkImageCreateInfo imgInfo{};
        imgInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imgInfo.imageType = VK_IMAGE_TYPE_2D;
        imgInfo.format = VK_FORMAT_D32_SFLOAT;
        imgInfo.extent = { POINT_LIGHT_SHADOW_SIZE, POINT_LIGHT_SHADOW_SIZE, 1 };
        imgInfo.mipLevels = 1;
        imgInfo.arrayLayers = 6;  // Cubemap = 6 faces
        imgInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imgInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imgInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imgInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

        VK_CHECK(vmaCreateImage(_allocator, &imgInfo, &allocInfo,
            &shadowData.cubemap.image, &shadowData.cubemap.allocation, nullptr));

        shadowData.cubemap.imageFormat = VK_FORMAT_D32_SFLOAT;
        shadowData.cubemap.imageExtent = { POINT_LIGHT_SHADOW_SIZE, POINT_LIGHT_SHADOW_SIZE, 1 };

        // Create full cubemap view for sampling
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = shadowData.cubemap.image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
        viewInfo.format = VK_FORMAT_D32_SFLOAT;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 6;

        VK_CHECK(vkCreateImageView(_device, &viewInfo, nullptr, &shadowData.cubemapView));

        // Create per-face views for rendering
        for (uint32_t face = 0; face < 6; face++) {
            VkImageViewCreateInfo faceViewInfo{};
            faceViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            faceViewInfo.image = shadowData.cubemap.image;
            faceViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            faceViewInfo.format = VK_FORMAT_D32_SFLOAT;
            faceViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            faceViewInfo.subresourceRange.baseMipLevel = 0;
            faceViewInfo.subresourceRange.levelCount = 1;
            faceViewInfo.subresourceRange.baseArrayLayer = face;
            faceViewInfo.subresourceRange.layerCount = 1;

            VK_CHECK(vkCreateImageView(_device, &faceViewInfo, nullptr, &shadowData.faceViews[face]));
        }

        shadowData.lightIndex = -1;  // Not assigned yet
    }

    // Add to deletion queue
    _mainDeletionQueue.push_function([=, this]() {
        vkDestroySampler(_device, _pointLightShadowSampler, nullptr);
        for (uint32_t i = 0; i < MAX_SHADOW_POINT_LIGHTS; i++) {
            auto& shadowData = _pointLightShadows[i];
            for (uint32_t face = 0; face < 6; face++) {
                if (shadowData.faceViews[face] != VK_NULL_HANDLE) {
                    vkDestroyImageView(_device, shadowData.faceViews[face], nullptr);
                }
            }
            if (shadowData.cubemapView != VK_NULL_HANDLE) {
                vkDestroyImageView(_device, shadowData.cubemapView, nullptr);
            }
            if (shadowData.cubemap.image != VK_NULL_HANDLE) {
                vmaDestroyImage(_allocator, shadowData.cubemap.image, shadowData.cubemap.allocation);
            }
        }
    });

    fmt::print("[Shadow] Point light shadow cubemaps initialized: {} slots, {}x{} per face\n",
        MAX_SHADOW_POINT_LIGHTS, POINT_LIGHT_SHADOW_SIZE, POINT_LIGHT_SHADOW_SIZE);
}

void VulkanEngine::update_point_light_shadow_data() {
    // Assign shadow slots to lights that want shadows
    int shadowSlot = 0;
    sceneData.pointLightShadowCount = 0;

    for (size_t i = 0; i < scenePointLights.size() && shadowSlot < MAX_SHADOW_POINT_LIGHTS; i++) {
        auto& light = scenePointLights[i];

        if (light.castsShadow) {
            light.shadowIndex = shadowSlot;
            _pointLightShadows[shadowSlot].lightIndex = static_cast<int>(i);

            // Store shadow data in scene data for shader
            sceneData.pointLightShadowData[shadowSlot] = glm::vec4(
                light.position.x, light.position.y, light.position.z,
                light.radius  // Far plane = light radius
            );
            sceneData.pointLightShadowIndices[shadowSlot] = static_cast<int>(i);  // ivec4 [] access
            shadowSlot++;
        } else {
            light.shadowIndex = -1;
        }
    }

    sceneData.pointLightShadowCount = shadowSlot;

    // Clear unused slots
    for (int i = shadowSlot; i < MAX_SHADOW_POINT_LIGHTS; i++) {
        _pointLightShadows[i].lightIndex = -1;
    }
}

void VulkanEngine::render_point_light_shadows(VkCommandBuffer cmd) {
    if (!pointLightShadowsEnabled || !shadowsEnabled) return;
    if (_shadowPipeline == VK_NULL_HANDLE) return;

    // Update which lights have shadow slots
    update_point_light_shadow_data();

    if (sceneData.pointLightShadowCount == 0) return;

    // Cubemap face directions (view matrices)
    // Order: +X, -X, +Y, -Y, +Z, -Z
    static const glm::vec3 faceDirs[6] = {
        glm::vec3( 1,  0,  0),  // +X
        glm::vec3(-1,  0,  0),  // -X
        glm::vec3( 0,  1,  0),  // +Y
        glm::vec3( 0, -1,  0),  // -Y
        glm::vec3( 0,  0,  1),  // +Z
        glm::vec3( 0,  0, -1)   // -Z
    };
    static const glm::vec3 faceUps[6] = {
        glm::vec3( 0, -1,  0),  // +X
        glm::vec3( 0, -1,  0),  // -X
        glm::vec3( 0,  0,  1),  // +Y
        glm::vec3( 0,  0, -1),  // -Y
        glm::vec3( 0, -1,  0),  // +Z
        glm::vec3( 0, -1,  0)   // -Z
    };

    // Bind shadow pipeline
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _shadowPipeline);

    // Render each shadow-casting light
    for (int slot = 0; slot < sceneData.pointLightShadowCount; slot++) {
        auto& shadowData = _pointLightShadows[slot];
        if (shadowData.lightIndex < 0) continue;

        PointLight& light = scenePointLights[shadowData.lightIndex];
        glm::vec3 lightPos = light.position;
        float farPlane = light.radius;
        float nearPlane = 0.1f;

        // Perspective projection for point light (90 degree FOV for cube face)
        glm::mat4 proj = glm::perspective(glm::radians(90.0f), 1.0f, nearPlane, farPlane);

        // Render each face
        for (uint32_t face = 0; face < 6; face++) {
            // Transition face to depth attachment
            VkImageMemoryBarrier2 barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
            barrier.srcStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
            barrier.srcAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
            barrier.dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT;
            barrier.dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            barrier.image = shadowData.cubemap.image;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.baseArrayLayer = face;
            barrier.subresourceRange.layerCount = 1;

            VkDependencyInfo depInfo{};
            depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
            depInfo.imageMemoryBarrierCount = 1;
            depInfo.pImageMemoryBarriers = &barrier;
            vkCmdPipelineBarrier2(cmd, &depInfo);

            // Begin rendering to this face
            VkRenderingAttachmentInfo depthAttachment{};
            depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
            depthAttachment.imageView = shadowData.faceViews[face];
            depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            depthAttachment.clearValue.depthStencil = { 1.0f, 0 };

            VkRenderingInfo renderInfo{};
            renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
            renderInfo.renderArea = { {0, 0}, {POINT_LIGHT_SHADOW_SIZE, POINT_LIGHT_SHADOW_SIZE} };
            renderInfo.layerCount = 1;
            renderInfo.colorAttachmentCount = 0;
            renderInfo.pDepthAttachment = &depthAttachment;

            vkCmdBeginRendering(cmd, &renderInfo);

            // Set viewport and scissor
            VkViewport viewport{ 0, 0, (float)POINT_LIGHT_SHADOW_SIZE, (float)POINT_LIGHT_SHADOW_SIZE, 0, 1 };
            VkRect2D scissor{ {0, 0}, {POINT_LIGHT_SHADOW_SIZE, POINT_LIGHT_SHADOW_SIZE} };
            vkCmdSetViewport(cmd, 0, 1, &viewport);
            vkCmdSetScissor(cmd, 0, 1, &scissor);

            // Calculate view matrix for this face
            glm::mat4 view = glm::lookAt(lightPos, lightPos + faceDirs[face], faceUps[face]);
            glm::mat4 lightSpaceMatrix = proj * view;

            // Temporarily store in cascade 0 for the shadow shader
            glm::mat4 originalMatrix = sceneData.shadowMatrices[0];
            sceneData.shadowMatrices[0] = lightSpaceMatrix;

            // Render opaque objects
            struct ShadowPushConstants {
                glm::mat4 worldMatrix;
                int32_t cascadeIndex;
            } shadowPC;
            shadowPC.cascadeIndex = 0;  // Using cascade 0 slot

            for (const auto& obj : drawCommands.OpaqueSurfaces) {
                if (!obj.indexBuffer) continue;

                shadowPC.worldMatrix = obj.transform;
                vkCmdPushConstants(cmd, _shadowPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                    0, sizeof(ShadowPushConstants), &shadowPC);

                VkDeviceSize offset = 0;
                vkCmdBindVertexBuffers(cmd, 0, 1, &obj.vertexBuffer, &offset);
                vkCmdBindIndexBuffer(cmd, obj.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
                vkCmdDrawIndexed(cmd, obj.indexCount, 1, obj.firstIndex, 0, 0);
            }

            // Render primitives
            for (const auto& shape : static_shapes) {
                if (!shape.visible || shape.mesh.indexBuffer.buffer == VK_NULL_HANDLE) continue;

                shadowPC.worldMatrix = shape.get_transform();
                vkCmdPushConstants(cmd, _shadowPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                    0, sizeof(ShadowPushConstants), &shadowPC);

                VkDeviceSize offset = 0;
                vkCmdBindVertexBuffers(cmd, 0, 1, &shape.mesh.vertexBuffer.buffer, &offset);
                vkCmdBindIndexBuffer(cmd, shape.mesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
                vkCmdDrawIndexed(cmd, shape.mesh.indexCount, 1, 0, 0, 0);
            }

            sceneData.shadowMatrices[0] = originalMatrix;

            vkCmdEndRendering(cmd);

            // Transition face to shader read
            barrier.srcStageMask = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
            barrier.srcAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
            barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
            barrier.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            vkCmdPipelineBarrier2(cmd, &depInfo);
        }
    }
}

void VulkanEngine::update_shadow_cascades() {
    if (!shadowsEnabled) return;

    // Get camera parameters
    float fov = glm::radians(mainCamera.fov);
    VkViewport viewport = get_letterbox_viewport();
    float aspect = viewport.width / viewport.height;
    float nearPlane = mainCamera.nearPlane;
    float farPlane = mainCamera.farPlane;

    // Cascade split distances (exponential distribution)
    // Using practical split scheme (PSSM) for better shadow distribution
    float cascadeSplits[SHADOW_CASCADE_COUNT];
    float lambda = 0.85f; // Higher = more logarithmic (better near shadows)

    for (uint32_t i = 0; i < SHADOW_CASCADE_COUNT; i++) {
        float p = (float)(i + 1) / (float)SHADOW_CASCADE_COUNT;
        float log_split = nearPlane * std::pow(farPlane / nearPlane, p);
        float uniform_split = nearPlane + (farPlane - nearPlane) * p;
        cascadeSplits[i] = lambda * log_split + (1.0f - lambda) * uniform_split;
    }

    // Store cascade splits in scene data
    sceneData.cascadeSplits = glm::vec4(
        cascadeSplits[0],
        cascadeSplits[1],
        cascadeSplits[2],
        cascadeSplits[3]
    );

    // Sun direction (normalized) - pointing FROM sun TO scene
    glm::vec3 lightDir = glm::normalize(glm::vec3(sceneData.sunlightDirection));

    // Choose up vector that's not parallel to light direction
    glm::vec3 upVector = glm::vec3(0.0f, 1.0f, 0.0f);
    if (std::abs(glm::dot(lightDir, upVector)) > 0.99f) {
        upVector = glm::vec3(1.0f, 0.0f, 0.0f); // Use X axis if light is vertical
    }

    // Camera matrices
    glm::mat4 camView = mainCamera.getViewMatrix();
    glm::mat4 camProj = glm::perspectiveRH_ZO(fov, aspect, nearPlane, farPlane);
    glm::mat4 invCamViewProj = glm::inverse(camProj * camView);

    // Calculate light-space matrix for each cascade
    float lastSplitDist = nearPlane;

    for (uint32_t cascade = 0; cascade < SHADOW_CASCADE_COUNT; cascade++) {
        float splitDist = cascadeSplits[cascade];

        // Frustum corners in NDC space (Vulkan: Z from 0 to 1)
        glm::vec3 frustumCorners[8] = {
            glm::vec3(-1.0f,  1.0f, 0.0f),
            glm::vec3( 1.0f,  1.0f, 0.0f),
            glm::vec3( 1.0f, -1.0f, 0.0f),
            glm::vec3(-1.0f, -1.0f, 0.0f),
            glm::vec3(-1.0f,  1.0f, 1.0f),
            glm::vec3( 1.0f,  1.0f, 1.0f),
            glm::vec3( 1.0f, -1.0f, 1.0f),
            glm::vec3(-1.0f, -1.0f, 1.0f),
        };

        // Transform corners to world space
        for (int i = 0; i < 8; i++) {
            glm::vec4 corner = invCamViewProj * glm::vec4(frustumCorners[i], 1.0f);
            frustumCorners[i] = glm::vec3(corner) / corner.w;
        }

        // Adjust corners for this cascade's near/far planes
        for (int i = 0; i < 4; i++) {
            glm::vec3 ray = frustumCorners[i + 4] - frustumCorners[i];
            float nearFrac = (lastSplitDist - nearPlane) / (farPlane - nearPlane);
            float farFrac = (splitDist - nearPlane) / (farPlane - nearPlane);
            frustumCorners[i + 4] = frustumCorners[i] + ray * farFrac;
            frustumCorners[i] = frustumCorners[i] + ray * nearFrac;
        }

        // Calculate frustum center
        glm::vec3 frustumCenter(0.0f);
        for (int i = 0; i < 8; i++) {
            frustumCenter += frustumCorners[i];
        }
        frustumCenter /= 8.0f;

        // Calculate frustum bounding sphere radius
        float radius = 0.0f;
        for (int i = 0; i < 8; i++) {
            float dist = glm::length(frustumCorners[i] - frustumCenter);
            radius = glm::max(radius, dist);
        }
        // Add 10% margin for safety
        radius *= 1.1f;

        // Snap radius to prevent shimmering
        float texelsPerUnit = (SHADOW_MAP_SIZE / 2.0f) / (radius * 2.0f);
        radius = std::ceil(radius * texelsPerUnit) / texelsPerUnit;

        // Create light view matrix looking at frustum center
        glm::vec3 lightPos = frustumCenter - lightDir * radius * 2.0f;
        glm::mat4 lightView = glm::lookAt(lightPos, frustumCenter, upVector);

        // Snap frustum center to texel grid to prevent shadow shimmering
        glm::vec4 shadowOrigin = lightView * glm::vec4(frustumCenter, 1.0f);
        float texelSize = (radius * 2.0f) / (SHADOW_MAP_SIZE / 2.0f);
        shadowOrigin.x = std::floor(shadowOrigin.x / texelSize) * texelSize;
        shadowOrigin.y = std::floor(shadowOrigin.y / texelSize) * texelSize;

        // Recalculate light position with snapped center
        glm::mat4 invLightView = glm::inverse(lightView);
        glm::vec3 snappedCenter = glm::vec3(invLightView * shadowOrigin);
        lightPos = snappedCenter - lightDir * radius * 2.0f;
        lightView = glm::lookAt(lightPos, snappedCenter, upVector);

        // Create orthographic projection
        // Near plane: 0.1 (small positive to avoid precision issues)
        // Far plane: 4x radius to capture shadow casters behind the frustum
        float nearPlaneLight = 0.1f;
        float farPlaneLight = radius * 4.0f;

        glm::mat4 lightProj = glm::orthoRH_ZO(
            -radius, radius,  // left, right
            -radius, radius,  // bottom, top
            nearPlaneLight, farPlaneLight
        );

        // Store in scene data
        sceneData.shadowMatrices[cascade] = lightProj * lightView;
        _shadowCascades[cascade].viewProjMatrix = sceneData.shadowMatrices[cascade];
        _shadowCascades[cascade].splitDepth = splitDist;

        lastSplitDist = splitDist;
    }

    // Update shadow settings in scene data
    sceneData.shadowBias = shadowBias;
    sceneData.shadowNormalBias = shadowNormalBias;
    sceneData.shadowsEnabled = shadowsEnabled ? 1 : 0;
}

void VulkanEngine::render_shadow_pass(VkCommandBuffer cmd) {
    if (!shadowsEnabled || _shadowPipeline == VK_NULL_HANDLE) return;

    // Create scene data buffer for shadow pass
    AllocatedBuffer shadowSceneBuffer = create_buffer(sizeof(GPUSceneData),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

    get_current_frame()._deletionQueue.push_function([=, this]() {
        destroy_buffer(shadowSceneBuffer);
    });

    // Copy scene data (contains shadow matrices)
    GPUSceneData* scenePtr = (GPUSceneData*)shadowSceneBuffer.allocation->GetMappedData();
    *scenePtr = sceneData;

    // Allocate descriptor set for shadow pass (needs variable count for texture array)
    VkDescriptorSetVariableDescriptorCountAllocateInfo allocArrayInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO
    };
    uint32_t descriptorCounts = std::max(1u, static_cast<uint32_t>(texCache.Cache.size()));
    allocArrayInfo.pDescriptorCounts = &descriptorCounts;
    allocArrayInfo.descriptorSetCount = 1;

    VkDescriptorSet shadowDescriptor = get_current_frame()._frameDescriptors.allocate(
        _device, _gpuSceneDataDescriptorLayout, &allocArrayInfo);

    DescriptorWriter writer;
    writer.write_buffer(0, shadowSceneBuffer.buffer, sizeof(GPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    writer.update_set(_device, shadowDescriptor);

    // Transition shadow map to depth attachment
    vkutil::transition_image(cmd, _shadowMapImage.image,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

    // Clear depth attachment (clear entire atlas)
    VkRenderingAttachmentInfo depthAttachment{};
    depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    depthAttachment.imageView = _shadowMapView;
    depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.clearValue.depthStencil = { 1.0f, 0 };

    VkRenderingInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea = { {0, 0}, {SHADOW_MAP_SIZE, SHADOW_MAP_SIZE} };
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 0;
    renderingInfo.pColorAttachments = nullptr;
    renderingInfo.pDepthAttachment = &depthAttachment;

    vkCmdBeginRendering(cmd, &renderingInfo);

    // Bind shadow pipeline and descriptor set
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _shadowPipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _shadowPipelineLayout,
        0, 1, &shadowDescriptor, 0, nullptr);

    // ==========================================================================
    // RENDER ALL 4 CASCADES INTO 2x2 ATLAS
    // ==========================================================================
    // Atlas layout: [Cascade 0 | Cascade 1]
    //               [Cascade 2 | Cascade 3]
    // Each cascade is SHADOW_MAP_SIZE/2 x SHADOW_MAP_SIZE/2 (1024x1024)

    const uint32_t cascadeSize = SHADOW_MAP_SIZE / 2;

    for (uint32_t cascade = 0; cascade < SHADOW_CASCADE_COUNT; cascade++) {
        // Calculate atlas position for this cascade
        uint32_t atlasX = (cascade % 2) * cascadeSize;
        uint32_t atlasY = (cascade / 2) * cascadeSize;

        // Set viewport for this cascade's region in the atlas
        VkViewport viewport{};
        viewport.x = (float)atlasX;
        viewport.y = (float)atlasY;
        viewport.width = (float)cascadeSize;
        viewport.height = (float)cascadeSize;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(cmd, 0, 1, &viewport);

        // Set scissor to match viewport
        VkRect2D scissor{};
        scissor.offset = { (int32_t)atlasX, (int32_t)atlasY };
        scissor.extent = { cascadeSize, cascadeSize };
        vkCmdSetScissor(cmd, 0, 1, &scissor);

        // Shadow push constant struct
        struct ShadowPushConstants {
            glm::mat4 worldMatrix;
            int32_t cascadeIndex;
        } shadowPC;
        shadowPC.cascadeIndex = (int32_t)cascade;

        // Render opaque surfaces for this cascade
        for (const auto& obj : drawCommands.OpaqueSurfaces) {
            if (!obj.indexBuffer) continue;

            // Push world matrix + cascade index
            shadowPC.worldMatrix = obj.transform;
            vkCmdPushConstants(cmd, _shadowPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                0, sizeof(ShadowPushConstants), &shadowPC);

            // Bind vertex and index buffers
            VkDeviceSize offset = 0;
            vkCmdBindVertexBuffers(cmd, 0, 1, &obj.vertexBuffer, &offset);
            vkCmdBindIndexBuffer(cmd, obj.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

            // Draw
            vkCmdDrawIndexed(cmd, obj.indexCount, 1, obj.firstIndex, 0, 0);
        }

        // Render primitives (static shapes) for this cascade
        for (const auto& shape : static_shapes) {
            if (!shape.visible || shape.mesh.indexBuffer.buffer == VK_NULL_HANDLE) continue;

            // Use the same transform as regular rendering
            shadowPC.worldMatrix = shape.get_transform();
            vkCmdPushConstants(cmd, _shadowPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                0, sizeof(ShadowPushConstants), &shadowPC);

            VkDeviceSize offset = 0;
            vkCmdBindVertexBuffers(cmd, 0, 1, &shape.mesh.vertexBuffer.buffer, &offset);
            vkCmdBindIndexBuffer(cmd, shape.mesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

            vkCmdDrawIndexed(cmd, shape.mesh.indexCount, 1, 0, 0, 0);
        }
    }

    vkCmdEndRendering(cmd);

    // Transition shadow map to shader read
    vkutil::transition_image(cmd, _shadowMapImage.image,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

glm::mat4 VulkanEngine::get_light_space_matrix(float nearPlane, float farPlane) {
    // Get sun direction
    glm::vec3 lightDir = glm::normalize(glm::vec3(sceneData.sunlightDirection));

    // Simple orthographic projection for directional light
    float size = 50.0f; // Scene size
    glm::mat4 lightProj = glm::ortho(-size, size, -size, size, nearPlane, farPlane);
    glm::mat4 lightView = glm::lookAt(-lightDir * 100.0f, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    return lightProj * lightView;
}

// =============================================================================
// GPU-DRIVEN RENDERING SYSTEM
// =============================================================================

void VulkanEngine::init_gpu_driven_rendering() {
    fmt::print("[GPU-Driven] Initializing GPU-driven rendering system...\n");

    // Create GPU object buffer
    size_t objectBufferSize = maxGPUObjects * sizeof(GPUObjectData);
    _gpuObjectBuffer = create_buffer(objectBufferSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);

    // Create indirect draw buffer
    size_t indirectBufferSize = maxGPUObjects * sizeof(GPUIndirectCommand);
    _indirectDrawBuffer = create_buffer(indirectBufferSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);

    // Create draw count buffer (atomic counter)
    _drawCountBuffer = create_buffer(sizeof(uint32_t),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);

    // Create mesh info buffer (stores index counts, offsets for each mesh)
    size_t meshInfoSize = 1024 * sizeof(glm::uvec4); // Enough for 1024 unique meshes
    _gpuMeshInfoBuffer = create_buffer(meshInfoSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);

    // Create cull compute descriptor layout
    DescriptorLayoutBuilder layoutBuilder;
    layoutBuilder.add_binding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT); // Object buffer
    layoutBuilder.add_binding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT); // Mesh info buffer
    layoutBuilder.add_binding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT); // Indirect commands
    layoutBuilder.add_binding(3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT); // Draw count
    _cullDescriptorLayout = layoutBuilder.build(_device, VK_SHADER_STAGE_COMPUTE_BIT);

    // Create cull compute pipeline layout
    VkPushConstantRange pushConstant{};
    pushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pushConstant.offset = 0;
    pushConstant.size = sizeof(glm::mat4) + sizeof(glm::vec4) * 6 + sizeof(uint32_t) * 4; // viewProj + frustum + counts

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &_cullDescriptorLayout;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &pushConstant;

    VK_CHECK(vkCreatePipelineLayout(_device, &layoutInfo, nullptr, &_cullComputeLayout));

    // Load and create cull compute pipeline
    VkShaderModule cullShader;
    if (!vkutil::load_shader_module("../../shaders/cull.comp.spv", _device, &cullShader)) {
        fmt::print("[GPU-Driven] Warning: Could not load cull.comp.spv shader\n");
        gpuDrivenEnabled = false;
        return;
    }

    VkPipelineShaderStageCreateInfo stageInfo{};
    stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stageInfo.module = cullShader;
    stageInfo.pName = "main";

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.stage = stageInfo;
    pipelineInfo.layout = _cullComputeLayout;

    VK_CHECK(vkCreateComputePipelines(_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_cullComputePipeline));

    vkDestroyShaderModule(_device, cullShader, nullptr);

    // Cleanup
    _mainDeletionQueue.push_function([=, this]() {
        destroy_buffer(_gpuObjectBuffer);
        destroy_buffer(_indirectDrawBuffer);
        destroy_buffer(_drawCountBuffer);
        destroy_buffer(_gpuMeshInfoBuffer);
        vkDestroyPipeline(_device, _cullComputePipeline, nullptr);
        vkDestroyPipelineLayout(_device, _cullComputeLayout, nullptr);
        vkDestroyDescriptorSetLayout(_device, _cullDescriptorLayout, nullptr);
    });

    fmt::print("[GPU-Driven] GPU-driven rendering system initialized (max {} objects)\n", maxGPUObjects);
}

void VulkanEngine::update_gpu_object_buffer() {
    if (!gpuDrivenEnabled) return;

    // Collect all objects from draw commands
    std::vector<GPUObjectData> objects;
    objects.reserve(drawCommands.OpaqueSurfaces.size());

    for (const auto& surface : drawCommands.OpaqueSurfaces) {
        GPUObjectData obj{};
        obj.modelMatrix = surface.transform;

        // Calculate bounding sphere from mesh bounds
        // Using the transform's position as center and scale for radius (simplified)
        glm::vec3 center = glm::vec3(surface.transform[3]);
        float radius = glm::max(glm::max(
            glm::length(glm::vec3(surface.transform[0])),
            glm::length(glm::vec3(surface.transform[1]))),
            glm::length(glm::vec3(surface.transform[2])));
        obj.sphereBounds = glm::vec4(center, radius);

        obj.materialIndex = 0;  // TODO: Map materials
        obj.meshIndex = 0;      // TODO: Map meshes
        obj.flags = 1;          // Visible

        objects.push_back(obj);
    }

    if (objects.empty()) return;

    // Upload to GPU (using staging buffer)
    size_t dataSize = objects.size() * sizeof(GPUObjectData);

    AllocatedBuffer stagingBuffer = create_buffer(dataSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

    memcpy(stagingBuffer.allocation->GetMappedData(), objects.data(), dataSize);

    immediate_submit([&](VkCommandBuffer cmd) {
        VkBufferCopy copy{};
        copy.size = dataSize;
        vkCmdCopyBuffer(cmd, stagingBuffer.buffer, _gpuObjectBuffer.buffer, 1, &copy);
    });

    destroy_buffer(stagingBuffer);
}

void VulkanEngine::perform_gpu_culling(VkCommandBuffer cmd) {
    if (!gpuDrivenEnabled || _cullComputePipeline == VK_NULL_HANDLE) return;

    uint32_t objectCount = static_cast<uint32_t>(drawCommands.OpaqueSurfaces.size());
    if (objectCount == 0) return;

    // Reset draw count to 0
    vkCmdFillBuffer(cmd, _drawCountBuffer.buffer, 0, sizeof(uint32_t), 0);

    // Memory barrier for the fill
    VkMemoryBarrier memBarrier{};
    memBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    memBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    memBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        0, 1, &memBarrier, 0, nullptr, 0, nullptr);

    // Allocate descriptor set for culling
    VkDescriptorSet cullDescriptor = get_current_frame()._frameDescriptors.allocate(_device, _cullDescriptorLayout);

    DescriptorWriter writer;
    writer.write_buffer(0, _gpuObjectBuffer.buffer, maxGPUObjects * sizeof(GPUObjectData), 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    writer.write_buffer(1, _gpuMeshInfoBuffer.buffer, 1024 * sizeof(glm::uvec4), 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    writer.write_buffer(2, _indirectDrawBuffer.buffer, maxGPUObjects * sizeof(GPUIndirectCommand), 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    writer.write_buffer(3, _drawCountBuffer.buffer, sizeof(uint32_t), 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    writer.update_set(_device, cullDescriptor);

    // Bind compute pipeline
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _cullComputePipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _cullComputeLayout,
        0, 1, &cullDescriptor, 0, nullptr);

    // Push constants with frustum planes
    struct CullPushConstants {
        glm::mat4 viewProj;
        glm::vec4 frustumPlanes[6];
        uint32_t objectCount;
        uint32_t enableCulling;
        uint32_t padding[2];
    } cullPush;

    cullPush.viewProj = sceneData.viewproj;
    cullPush.objectCount = objectCount;
    cullPush.enableCulling = 1;

    // Extract frustum planes from view-projection matrix
    glm::mat4 vp = glm::transpose(sceneData.viewproj);
    cullPush.frustumPlanes[0] = vp[3] + vp[0]; // Left
    cullPush.frustumPlanes[1] = vp[3] - vp[0]; // Right
    cullPush.frustumPlanes[2] = vp[3] + vp[1]; // Bottom
    cullPush.frustumPlanes[3] = vp[3] - vp[1]; // Top
    cullPush.frustumPlanes[4] = vp[3] + vp[2]; // Near
    cullPush.frustumPlanes[5] = vp[3] - vp[2]; // Far

    // Normalize frustum planes
    for (int i = 0; i < 6; i++) {
        float len = glm::length(glm::vec3(cullPush.frustumPlanes[i]));
        cullPush.frustumPlanes[i] /= len;
    }

    vkCmdPushConstants(cmd, _cullComputeLayout, VK_SHADER_STAGE_COMPUTE_BIT,
        0, sizeof(CullPushConstants), &cullPush);

    // Dispatch compute shader (64 threads per workgroup)
    uint32_t workgroupCount = (objectCount + 63) / 64;
    vkCmdDispatch(cmd, workgroupCount, 1, 1);

    // Memory barrier for indirect buffer
    VkMemoryBarrier indirectBarrier{};
    indirectBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    indirectBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    indirectBarrier.dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT,
        0, 1, &indirectBarrier, 0, nullptr, 0, nullptr);
}

void VulkanEngine::draw_indirect(VkCommandBuffer cmd, VkDescriptorSet globalDescriptor) {
    if (!gpuDrivenEnabled || _indirectDrawBuffer.buffer == VK_NULL_HANDLE) return;

    // Draw using indirect buffer with count from drawCountBuffer
    // Note: vkCmdDrawIndexedIndirectCount requires the VK_KHR_draw_indirect_count extension
    // For now, use a fixed max count with instanceCount=0 for culled objects
    vkCmdDrawIndexedIndirectCount(cmd,
        _indirectDrawBuffer.buffer, 0,
        _drawCountBuffer.buffer, 0,
        maxGPUObjects,
        sizeof(GPUIndirectCommand));
}

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

    // === Skybox background effect (needs separate layout for cubemap descriptor) ===
    {
        // Descriptor layout: binding 0 = storage image, binding 1 = cubemap sampler
        VkDescriptorSetLayoutBinding skyboxBindings[2] = {};
        skyboxBindings[0].binding = 0;
        skyboxBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        skyboxBindings[0].descriptorCount = 1;
        skyboxBindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        skyboxBindings[1].binding = 1;
        skyboxBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        skyboxBindings[1].descriptorCount = 1;
        skyboxBindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        VkDescriptorSetLayoutCreateInfo skyboxLayoutInfo{};
        skyboxLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        skyboxLayoutInfo.bindingCount = 2;
        skyboxLayoutInfo.pBindings = skyboxBindings;
        VK_CHECK(vkCreateDescriptorSetLayout(_device, &skyboxLayoutInfo, nullptr, &_skyboxBgDescriptorLayout));

        // Pipeline layout: descriptor set + push constants (2x mat4 = 128 bytes)
        VkPushConstantRange skyboxPush{};
        skyboxPush.offset = 0;
        skyboxPush.size = sizeof(glm::mat4) * 2; // invView + invProj
        skyboxPush.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        VkPipelineLayoutCreateInfo skyboxPipelineLayoutInfo{};
        skyboxPipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        skyboxPipelineLayoutInfo.setLayoutCount = 1;
        skyboxPipelineLayoutInfo.pSetLayouts = &_skyboxBgDescriptorLayout;
        skyboxPipelineLayoutInfo.pushConstantRangeCount = 1;
        skyboxPipelineLayoutInfo.pPushConstantRanges = &skyboxPush;
        VK_CHECK(vkCreatePipelineLayout(_device, &skyboxPipelineLayoutInfo, nullptr, &_skyboxBgPipelineLayout));

        // Allocate descriptor set
        _skyboxBgDescriptorSet = globalDescriptorAllocator.allocate(_device, _skyboxBgDescriptorLayout);

        // Load shader and create pipeline
        VkShaderModule skyboxShader;
        if (vkutil::load_shader_module("../../shaders/skybox_bg.comp.spv", _device, &skyboxShader)) {
            VkPipelineShaderStageCreateInfo stageinfo{};
            stageinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            stageinfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
            stageinfo.module = skyboxShader;
            stageinfo.pName = "main";

            VkComputePipelineCreateInfo pipelineInfo{};
            pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
            pipelineInfo.layout = _skyboxBgPipelineLayout;
            pipelineInfo.stage = stageinfo;

            ComputeEffect skyboxEffect{};
            VK_CHECK(vkCreateComputePipelines(_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &skyboxEffect.pipeline));

            skyboxEffect.layout = _skyboxBgPipelineLayout;
            skyboxEffect.name = "skybox";
            skyboxEffect.data = {};

            backgroundEffects.push_back(skyboxEffect);
            vkDestroyShaderModule(_device, skyboxShader, nullptr);
        } else {
            fmt::print("Error loading skybox_bg.comp.spv\n");
        }
    }

    _mainDeletionQueue.push_function([&]() {
        vkDestroyPipelineLayout(_device, _gradientPipelineLayout, nullptr);
        if (_skyboxBgPipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(_device, _skyboxBgPipelineLayout, nullptr);
        }
        if (_skyboxBgDescriptorLayout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(_device, _skyboxBgDescriptorLayout, nullptr);
        }
        for (auto& e : backgroundEffects) {
            vkDestroyPipeline(_device, e.pipeline, nullptr);
        }
        });
}

void VulkanEngine::updateSkyboxBgDescriptor() {
    if (_skyboxBgDescriptorSet == VK_NULL_HANDLE) return;
    if (!_environmentMap) return;
    if (_drawImage.imageView == VK_NULL_HANDLE) return;

    VkImageView cubemapView = _environmentMap->getEnvironmentCubemap();
    VkSampler cubemapSampler = _environmentMap->getSampler();
    if (cubemapView == VK_NULL_HANDLE || cubemapSampler == VK_NULL_HANDLE) return;

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageView = _drawImage.imageView;
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

    VkDescriptorImageInfo cubemapInfo{};
    cubemapInfo.imageView = cubemapView;
    cubemapInfo.sampler = cubemapSampler;
    cubemapInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet writes[2] = {};
    writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].dstSet = _skyboxBgDescriptorSet;
    writes[0].dstBinding = 0;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    writes[0].descriptorCount = 1;
    writes[0].pImageInfo = &imageInfo;

    writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[1].dstSet = _skyboxBgDescriptorSet;
    writes[1].dstBinding = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[1].descriptorCount = 1;
    writes[1].pImageInfo = &cubemapInfo;

    vkUpdateDescriptorSets(_device, 2, writes, 0, nullptr);
}

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

    // === 2.5. Default Meshes for Primitives (UI primitive creation) ===
    defaultMeshes[PrimitiveType::Cube] = generate_cube_mesh();
    defaultMeshes[PrimitiveType::Sphere] = generate_sphere_mesh(32, 16);
    defaultMeshes[PrimitiveType::Cylinder] = generate_cylinder_mesh(32);
    defaultMeshes[PrimitiveType::Cone] = generate_cone_mesh();
    defaultMeshes[PrimitiveType::Capsule] = generate_capsule_mesh();
    defaultMeshes[PrimitiveType::Torus] = generate_torus_mesh();
    defaultMeshes[PrimitiveType::Plane] = generate_plane_mesh();
    defaultMeshes[PrimitiveType::Triangle] = generate_triangle_mesh();

    fmt::print("Default meshes initialized: {} types\n", defaultMeshes.size());

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

    // Default 1x1 black cubemap (used as fallback when no environment cubemap is loaded)
    {
        VkImageCreateInfo cubemapInfo = { .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
        cubemapInfo.imageType = VK_IMAGE_TYPE_2D;
        cubemapInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        cubemapInfo.extent = { 1, 1, 1 };
        cubemapInfo.mipLevels = 1;
        cubemapInfo.arrayLayers = 6;
        cubemapInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        cubemapInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        cubemapInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        cubemapInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        VK_CHECK(vmaCreateImage(_allocator, &cubemapInfo, &allocInfo, &_defaultCubemap.image, &_defaultCubemap.allocation, nullptr));
        _defaultCubemap.imageFormat = VK_FORMAT_R8G8B8A8_UNORM;
        _defaultCubemap.imageExtent = { 1, 1, 1 };

        VkImageViewCreateInfo viewInfo = { .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        viewInfo.image = _defaultCubemap.image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
        viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 6;
        VK_CHECK(vkCreateImageView(_device, &viewInfo, nullptr, &_defaultCubemap.imageView));

        // Transition to shader read and clear to black
        immediate_submit([&](VkCommandBuffer cmd) {
            VkImageMemoryBarrier barrier = { .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
            barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.image = _defaultCubemap.image;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 6;
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

            VkClearColorValue clearColor = { {0.0f, 0.0f, 0.0f, 1.0f} };
            VkImageSubresourceRange range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 6 };
            vkCmdClearColorImage(cmd, _defaultCubemap.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColor, 1, &range);

            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
        });
    }

    // Samplerlar
    VkSamplerCreateInfo samplerInfo = { .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    vkCreateSampler(_device, &samplerInfo, nullptr, &_defaultSamplerNearest);

    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    vkCreateSampler(_device, &samplerInfo, nullptr, &_defaultSamplerLinear);

    // Add default textures to texture cache so descriptor sets always have valid bindings
    // This ensures the grid and other scene elements work even when no objects are loaded
    texCache.AddTexture(_whiteImage.imageView, _defaultSamplerLinear, "__default_white");
    texCache.AddTexture(_errorCheckerboardImage.imageView, _defaultSamplerLinear, "__default_error");
    fmt::print("Default textures added to cache: {} textures\n", texCache.Cache.size());

    // Note: Default images and samplers are cleaned up explicitly in cleanup()
}

void VulkanEngine::draw_main(VkCommandBuffer cmd)
{
    // === REFLECTION PROBES (render scene into cubemaps for reflections) ===
    if (_probesReady && _currentViewMode != ViewMode::PathTraced) {
        _reflectionFrameCounter++;
        if (_reflectionFrameCounter >= REFLECTION_UPDATE_INTERVAL) {
            _reflectionFrameCounter = 0;
            // Render 1 probe per interval, cycling through all probes
            if (_reflectionProbes[_currentProbeUpdateIndex].active) {
                render_reflection_probe_single(cmd, _currentProbeUpdateIndex);
            }
            _currentProbeUpdateIndex = (_currentProbeUpdateIndex + 1) % MAX_REFLECTION_PROBES;
        }
    }

    // === SHADOW PASS (before main rendering) ===
    if (shadowsEnabled && _shadowPipeline != VK_NULL_HANDLE) {
        render_shadow_pass(cmd);
    }

    // === POINT LIGHT SHADOW PASS ===
    if (shadowsEnabled && pointLightShadowsEnabled) {
        render_point_light_shadows(cmd);
    }

    // Background effect always runs for ALL view modes (including PathTraced)
    {
        ComputeEffect& effect = backgroundEffects[currentBackgroundEffect];
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, effect.pipeline);

        if (strcmp(effect.name, "skybox") == 0) {
            // Skybox background: uses its own descriptor set (drawImage + cubemap) and camera push constants
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _skyboxBgPipelineLayout, 0, 1, &_skyboxBgDescriptorSet, 0, nullptr);
            struct { glm::mat4 invView; glm::mat4 invProj; } skyboxPC;
            skyboxPC.invView = glm::inverse(sceneData.view);
            skyboxPC.invProj = glm::inverse(sceneData.proj);
            vkCmdPushConstants(cmd, _skyboxBgPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(skyboxPC), &skyboxPC);
        } else {
            // Standard background effects: use draw image descriptor + ComputePushConstants
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _gradientPipelineLayout, 0, 1, &_drawImageDescriptors, 0, nullptr);
            vkCmdPushConstants(cmd, _gradientPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputePushConstants), &effect.data);
        }

        vkCmdDispatch(cmd, std::ceil(_drawExtent.width / 16.0), std::ceil(_drawExtent.height / 16.0), 1);
    }

    // PathTraced mode: overlay geometry pixels on top of background
    if (_currentViewMode == ViewMode::PathTraced && _pathTracer) {
        // Reset accumulation if background effect or its parameters changed
        static int lastBackgroundEffect = -1;
        static ComputePushConstants lastBgData{};
        auto& bgEffect = backgroundEffects[currentBackgroundEffect];
        if (lastBackgroundEffect != currentBackgroundEffect ||
            memcmp(&lastBgData, &bgEffect.data, sizeof(ComputePushConstants)) != 0) {
            _pathTracer->resetAccumulation();
            lastBackgroundEffect = currentBackgroundEffect;
            lastBgData = bgEffect.data;
        }

        // Sync path tracer sky/cubemap settings with background
        _pathTracer->settings.skyColor = glm::vec3(bgEffect.data.data1);
        _pathTracer->settings.skyIntensity = bgEffect.data.data1.w;

        // Barrier: background compute writes must complete before path tracer reads/writes _drawImage
        VkImageMemoryBarrier bgToPathBarrier{};
        bgToPathBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        bgToPathBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
        bgToPathBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        bgToPathBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        bgToPathBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        bgToPathBarrier.image = _drawImage.image;
        bgToPathBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        bgToPathBarrier.subresourceRange.baseMipLevel = 0;
        bgToPathBarrier.subresourceRange.levelCount = 1;
        bgToPathBarrier.subresourceRange.baseArrayLayer = 0;
        bgToPathBarrier.subresourceRange.layerCount = 1;
        bgToPathBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        bgToPathBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &bgToPathBarrier);

        // Pass draw image to path tracer and render
        _pathTracer->setDrawImage(_drawImage.imageView);
        _pathTracer->render(cmd);
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

    // === BLOOM POST-PROCESS (operates on HDR linear values) ===
    if (_bloomPass && _renderSettings.bloomEnabled && _currentViewMode != ViewMode::PathTraced) {
        // Sync bloom settings from render settings
        _bloomPass->settings.enabled = _renderSettings.bloomEnabled;
        _bloomPass->settings.threshold = _renderSettings.bloomThreshold;
        _bloomPass->settings.intensity = _renderSettings.bloomIntensity;
        _bloomPass->settings.mipLevels = _renderSettings.bloomMipLevels;
        _bloomPass->settings.radius = _renderSettings.bloomRadius;

        // Execute bloom pass (reads from _drawImage, writes bloom back to _drawImage)
        _bloomPass->execute(cmd, _drawImage, _drawImage);

        // Bloom upsample leaves _drawImage in GENERAL layout - perfect for tonemap
    }

    // === FINAL TONE MAPPING (converts HDR linear -> LDR sRGB) ===
    // Runs AFTER bloom so emissive surfaces can have values > 1.0 for bloom detection
    if (_tonemapPipeline != VK_NULL_HANDLE && _currentViewMode != ViewMode::PathTraced) {
        // Ensure _drawImage is in GENERAL layout for compute imageLoad/imageStore
        // After bloom it's already GENERAL; if bloom is disabled, rasterize left it in GENERAL too
        VkMemoryBarrier memBarrier{};
        memBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        memBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        memBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

        vkCmdPipelineBarrier(cmd,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            0, 1, &memBarrier, 0, nullptr, 0, nullptr);

        execute_tonemap(cmd);

        // Barrier: ensure tonemap writes complete before swapchain copy
        VkMemoryBarrier postToneBarrier{};
        postToneBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        postToneBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        postToneBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(cmd,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0, 1, &postToneBarrier, 0, nullptr, 0, nullptr);
    }
}

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
    bool selected = (selectedNode == node.get());

    // UI'de selectable olarak göster
    if (ImGui::Selectable(label.c_str(), selected)) {
        selectedNode = node.get();  // Assign any Node, not just MeshNode
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

void VulkanEngine::draw_node_gizmo()
{
    if (!selectedNode) return;

    ImGuizmo::BeginFrame();

    static ImGuizmo::OPERATION operation = ImGuizmo::TRANSLATE;
    static ImGuizmo::MODE mode = ImGuizmo::LOCAL;
    static glm::mat4 model = selectedNode->localTransform;
    static Node* lastNode = nullptr;

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
    //wait until the gpu has finished rendering the last frame. Timeout of 1 second
    VK_CHECK(vkWaitForFences(_device, 1, &get_current_frame()._renderFence, true, 1000000000));

    // Process deferred scene unloads AFTER fence wait (GPU done with all references)
    processPendingSceneUnloads();

    get_current_frame()._deletionQueue.flush();
    get_current_frame()._frameDescriptors.clear_pools(_device);

    // Update scene AFTER fence wait to avoid writing to buffers still in use by GPU
    update_scene();
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

    // Process pending BVH rebuild BEFORE command recording (safe to destroy/create buffers here)
    if (_pathTracer) {
        _pathTracer->processPendingRebuild();
    }

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

//< visfn

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

void VulkanEngine::draw_geometry(VkCommandBuffer cmd)
{
    // === GPU SceneData ===
    AllocatedBuffer gpuSceneDataBuffer = create_buffer(sizeof(GPUSceneData),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

    if (gpuSceneDataBuffer.buffer == VK_NULL_HANDLE) {
        fmt::print("[ERROR] gpuSceneDataBuffer null!\n");
        return;
    }

    static bool geometryDebugPrinted = false;
    if (!geometryDebugPrinted) {
        fmt::print("[DEBUG] draw_geometry: texCache.size={}, gpuSceneDataBuffer=OK\n", texCache.Cache.size());
        geometryDebugPrinted = true;
    }

    get_current_frame()._deletionQueue.push_function([=, this]() {
        destroy_buffer(gpuSceneDataBuffer);
        });

    // Populate reflection probe data in sceneData before uploading
    int activeProbeCount = 0;
    for (int i = 0; i < MAX_REFLECTION_PROBES; ++i) {
        if (_reflectionProbes[i].active && _probesReady) {
            sceneData.probePositions[i] = glm::vec4(_reflectionProbes[i].position, _reflectionProbes[i].radius);
            activeProbeCount++;
        } else {
            sceneData.probePositions[i] = glm::vec4(0.0f, -999.0f, 0.0f, 1.0f);
        }
    }
    sceneData.probeSettings = glm::vec4(float(activeProbeCount), _renderSettings.globalSkyBlend, 0.0f, 0.0f);

    GPUSceneData* sceneUniformData = (GPUSceneData*)gpuSceneDataBuffer.allocation->GetMappedData();
    *sceneUniformData = sceneData;

    // === Descriptor Set ===
    VkDescriptorSetVariableDescriptorCountAllocateInfo allocArrayInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO
    };

    // Ensure at least 1 descriptor count to avoid allocation issues when texture cache is empty
    uint32_t descriptorCounts = std::max(1u, static_cast<uint32_t>(texCache.Cache.size()));
    allocArrayInfo.pDescriptorCounts = &descriptorCounts;
    allocArrayInfo.descriptorSetCount = 1;

    // Use the member variable so draw_viewing can access it
    globalDescriptor = get_current_frame()._frameDescriptors.allocate(
        _device, _gpuSceneDataDescriptorLayout, &allocArrayInfo);

    DescriptorWriter writer;
    writer.write_buffer(0, gpuSceneDataBuffer.buffer, sizeof(GPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

    // Shadow map binding (binding 2)
    if (_shadowMapView != VK_NULL_HANDLE && _shadowSampler != VK_NULL_HANDLE) {
        writer.write_image(2, _shadowMapView, _shadowSampler,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    }

    // Point light shadow cubemaps binding (binding 3)
    // IMPORTANT: cubemapInfos must stay alive until writer.update_set() is called
    std::array<VkDescriptorImageInfo, MAX_SHADOW_POINT_LIGHTS> cubemapInfos{};
    if (_pointLightShadowSampler != VK_NULL_HANDLE) {
        for (uint32_t i = 0; i < MAX_SHADOW_POINT_LIGHTS; i++) {
            cubemapInfos[i].sampler = _pointLightShadowSampler;
            cubemapInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            if (_pointLightShadows[i].cubemapView != VK_NULL_HANDLE) {
                cubemapInfos[i].imageView = _pointLightShadows[i].cubemapView;
            } else if (_pointLightShadows[0].cubemapView != VK_NULL_HANDLE) {
                cubemapInfos[i].imageView = _pointLightShadows[0].cubemapView;
            } else {
                cubemapInfos[i].imageView = _errorCheckerboardImage.imageView;
            }
        }

        VkWriteDescriptorSet cubemapWrite{ .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        cubemapWrite.dstSet = globalDescriptor;
        cubemapWrite.dstBinding = 3;
        cubemapWrite.dstArrayElement = 0;
        cubemapWrite.descriptorCount = MAX_SHADOW_POINT_LIGHTS;
        cubemapWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        cubemapWrite.pImageInfo = cubemapInfos.data();
        writer.writes.push_back(cubemapWrite);
    }

    // Environment cubemap array binding (binding 4) - [0]=sky, [1-4]=reflection probes
    std::array<VkDescriptorImageInfo, 5> envCubemapInfos{};
    {
        VkSampler sharedSampler = _defaultSamplerLinear;
        if (_environmentMap && _environmentMap->getSampler() != VK_NULL_HANDLE) {
            sharedSampler = _environmentMap->getSampler();
        }

        // Index 0: Sky environment map
        VkImageView skyView = _defaultCubemap.imageView;
        if (_environmentMap && _environmentMap->getEnvironmentCubemap() != VK_NULL_HANDLE) {
            skyView = _environmentMap->getEnvironmentCubemap();
        }
        envCubemapInfos[0] = { sharedSampler, skyView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

        // Indices 1-4: Reflection probes (fallback to defaultCubemap if not ready)
        for (int i = 0; i < MAX_REFLECTION_PROBES; ++i) {
            VkImageView probeView = _defaultCubemap.imageView;
            if (_probesReady && _reflectionProbes[i].active && _reflectionProbes[i].cubemapView != VK_NULL_HANDLE) {
                probeView = _reflectionProbes[i].cubemapView;
            }
            envCubemapInfos[i + 1] = { sharedSampler, probeView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
        }

        VkWriteDescriptorSet envArrayWrite{ .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        envArrayWrite.dstSet = globalDescriptor;
        envArrayWrite.dstBinding = 4;
        envArrayWrite.dstArrayElement = 0;
        envArrayWrite.descriptorCount = 5;
        envArrayWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        envArrayWrite.pImageInfo = envCubemapInfos.data();
        writer.writes.push_back(envArrayWrite);
    }

    // Bindless texture array binding (binding 5) - must be last (variable descriptor count)
    if (!texCache.Cache.empty()) {
        VkWriteDescriptorSet arraySet{ .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        arraySet.descriptorCount = descriptorCounts;
        arraySet.dstArrayElement = 0;
        arraySet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        arraySet.dstBinding = 5;
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

            if (selectedNode && obj.nodePointer == dynamic_cast<MeshNode*>(selectedNode))
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

    // Draw primitives with view-mode-aware rendering
    draw_primitives_with_viewport(cmd, globalDescriptor, viewport, scissor, _currentViewMode);

    drawCommands.OpaqueSurfaces.clear();
    drawCommands.TransparentSurfaces.clear();
}

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

    // Helper lambda to draw a render object
    auto drawObj = [&](const RenderObject& r) {
        if (!r.material || r.material->materialSet == VK_NULL_HANDLE) return;

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
    };

    // Draw opaque surfaces first
    for (auto& idx : opaque_draws) {
        drawObj(drawCommands.OpaqueSurfaces[idx]);
    }

    // Draw transparent surfaces after opaque
    for (auto& r : drawCommands.TransparentSurfaces) {
        drawObj(r);
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

    for (auto idx : opaque_draws)
        draw(drawCommands.OpaqueSurfaces[idx]);

    for (auto& r : drawCommands.TransparentSurfaces)
        draw(r);
}

void VulkanEngine::draw_viewing(VkCommandBuffer cmd)
{
    // PathTraced mode: skip rasterized geometry (path tracer handles it)
    // but still render grid overlay
    if (_currentViewMode != ViewMode::PathTraced) {
        draw_geometry(cmd);
    }

    // Grid drawn for ALL modes (including PathTraced)
    VkViewport viewport = get_letterbox_viewport();
    viewport.width = std::max(1.0f, viewport.width);
    viewport.height = std::max(1.0f, viewport.height);
    VkRect2D scissor = { {0, 0}, _windowExtent };
    vkCmdSetViewport(cmd, 0, 1, &viewport);
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    if (_showGrid && globalDescriptor != VK_NULL_HANDLE)
    {
        draw_grid(cmd, globalDescriptor);
    }
}

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
        // GPU-DRIVEN RENDERING
        // =======================================================================
        if (ImGui::CollapsingHeader("GPU-Driven Rendering")) {
            ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.8f, 0.6f, 0.2f, 1.0f));
            if (ImGui::Checkbox("Enable GPU-Driven", &gpuDrivenEnabled)) {
                if (gpuDrivenEnabled) {
                    fmt::print("[GPU-Driven] Enabled - using compute culling + indirect draws\n");
                } else {
                    fmt::print("[GPU-Driven] Disabled - using traditional rendering\n");
                }
            }
            ImGui::PopStyleColor();

            if (gpuDrivenEnabled) {
                ImGui::TextDisabled("Compute-based frustum culling");
                ImGui::TextDisabled("Indirect draw calls for batching");
            } else {
                ImGui::TextDisabled("Traditional per-object draw calls");
            }
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

    const float radius = 0.5f;
    const float halfHeight = 0.5f;

    // Side vertices with outward-facing normals and proper UVs
    for (int i = 0; i <= segments; ++i) {
        float u = (float)i / (float)segments;
        float angle = 2.0f * glm::pi<float>() * u;
        float x = std::cos(angle) * radius;
        float z = std::sin(angle) * radius;
        glm::vec3 sideNormal = glm::normalize(glm::vec3(std::cos(angle), 0.0f, std::sin(angle)));

        // Bottom vertex
        vertices.push_back({ { x, -halfHeight, z }, u, sideNormal, 0.0f, glm::vec4(1.0f) });
        // Top vertex
        vertices.push_back({ { x,  halfHeight, z }, u, sideNormal, 1.0f, glm::vec4(1.0f) });
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
    vertices.push_back({ { 0, halfHeight, 0 }, 0.5f, { 0, 1, 0 }, 0.5f, glm::vec4(1.0f) });

    for (int i = 0; i <= segments; ++i) {
        float angle = 2.0f * glm::pi<float>() * i / segments;
        float x = std::cos(angle) * radius;
        float z = std::sin(angle) * radius;
        float u = x / (2.0f * radius) + 0.5f;
        float v = z / (2.0f * radius) + 0.5f;
        vertices.push_back({ { x, halfHeight, z }, u, { 0, 1, 0 }, v, glm::vec4(1.0f) });
    }

    for (int i = 0; i < segments; ++i) {
        indices.push_back(topCenterIdx);
        indices.push_back(topCenterIdx + 1 + i);
        indices.push_back(topCenterIdx + 1 + i + 1);
    }

    // Bottom cap
    int bottomCenterIdx = static_cast<int>(vertices.size());
    vertices.push_back({ { 0, -halfHeight, 0 }, 0.5f, { 0, -1, 0 }, 0.5f, glm::vec4(1.0f) });

    for (int i = 0; i <= segments; ++i) {
        float angle = 2.0f * glm::pi<float>() * i / segments;
        float x = std::cos(angle) * radius;
        float z = std::sin(angle) * radius;
        float u = x / (2.0f * radius) + 0.5f;
        float v = z / (2.0f * radius) + 0.5f;
        vertices.push_back({ { x, -halfHeight, z }, u, { 0, -1, 0 }, v, glm::vec4(1.0f) });
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

void VulkanEngine::draw_scene_light_imgui() {
    if (ImGui::Begin("Light Settings")) {

        // === Ambient Light ===
        ImGui::Text("Ambient Light");
        ImGui::ColorEdit3("Ambient Color", (float*)&sceneData.ambientColor);
        ImGui::SliderFloat("Ambient Intensity", &sceneData.ambientColor.w, 0.0f, 2.0f);
        ImGui::Separator();

        // === Directional Light ===
        ImGui::Text("Sun (Directional Light)");

        // Sun enable/disable toggle
        if (ImGui::Checkbox("Sun Enabled", &sunEnabled)) {
            if (sunEnabled) {
                // Restore saved intensity
                sceneData.sunlightDirection.w = savedSunIntensity;
            } else {
                // Save current intensity and disable
                savedSunIntensity = sceneData.sunlightDirection.w;
                sceneData.sunlightDirection.w = 0.0f;
            }
        }

        if (sunEnabled) {
            ImGui::ColorEdit3("Sun Color", (float*)&sceneData.sunlightColor);
            if (ImGui::SliderFloat("Sun Intensity", &sceneData.sunlightDirection.w, 0.0f, 10.0f)) {
                savedSunIntensity = sceneData.sunlightDirection.w; // Update saved value
            }

            glm::vec3 sunDir = glm::vec3(sceneData.sunlightDirection);
            if (ImGui::DragFloat3("Sun Direction", (float*)&sunDir, 0.1f, -1.0f, 1.0f)) {
                sceneData.sunlightDirection = glm::vec4(glm::normalize(sunDir), sceneData.sunlightDirection.w);
            }
            ImGui::TextDisabled("Direction is normalized");
        }
        ImGui::Separator();

        // === Point Lights ===
        ImGui::Text("Point Lights");
        if (ImGui::Button("Add Point Light") && scenePointLights.size() < MAX_POINT_LIGHTS) {
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
            if (ImGui::TreeNode(fmt::format("Point Light {}", i).c_str())) {
                lightChanged |= ImGui::DragFloat3("Position", (float*)&scenePointLights[i].position, 0.1f);
                lightChanged |= ImGui::ColorEdit3("Color", (float*)&scenePointLights[i].color);
                lightChanged |= ImGui::SliderFloat("Intensity", &scenePointLights[i].intensity, 0.0f, 100.0f);
                lightChanged |= ImGui::SliderFloat("Radius", &scenePointLights[i].radius, 0.1f, 50.0f);

                if (ImGui::Button("Delete")) {
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

        ImGui::Separator();

        // === SHADOW SETTINGS ===
        ImGui::Text("Shadow Settings");
        ImGui::Checkbox("Shadows Enabled", &shadowsEnabled);
        if (shadowsEnabled) {
            ImGui::SliderFloat("Shadow Bias", &shadowBias, 0.0001f, 0.01f, "%.5f");
            ImGui::SliderFloat("Normal Bias", &shadowNormalBias, 0.0f, 0.1f, "%.4f");
            ImGui::SliderInt("PCF Samples", &shadowPcfSamples, 1, 4);
            ImGui::TextDisabled("Cascade Shadow Maps: %d cascades @ %dx%d",
                SHADOW_CASCADE_COUNT, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
        }

    }
    ImGui::End();
}

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

        draw();

        auto end = std::chrono::system_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        stats.frametime = elapsed.count() / 1000.f;
    }

    cleanup();
}

void VulkanEngine::processPendingSceneUnloads() {
    if (_pendingSceneUnloads.empty()) return;

    // At this point the GPU fence has been waited on, so all previous frames
    // that referenced these scene resources have finished executing.
    // Safe to destroy now.

    for (auto& sceneName : _pendingSceneUnloads) {
        auto it = loadedScenes.find(sceneName);
        if (it == loadedScenes.end()) continue;

        // Clear selection if it belongs to this scene
        // nodes map contains ALL nodes (including children), so flat iteration suffices
        if (selectedNode != nullptr && it->second) {
            for (const auto& [name, node] : it->second->nodes) {
                if (node.get() == selectedNode) {
                    selectedNode = nullptr;
                    selectedObjectName.clear();
                    break;
                }
            }
        }

        // Clear draw commands to prevent stale references
        drawCommands.OpaqueSurfaces.clear();
        drawCommands.TransparentSurfaces.clear();
        pickableRenderObjects.clear();

        // Now safe to destroy the scene (shared_ptr → clearAll → destroys GPU resources)
        loadedScenes.erase(it);

        if (_pathTracer) {
            _pathTracer->notifySceneChanged();
        }

        fmt::print("[Engine] Deferred unload completed for scene: {}\n", sceneName);
    }
    _pendingSceneUnloads.clear();
}

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
        sceneData.pointLights[sceneData.pointLightCount++] = light.toGPU();
    }

    // === Update Shadow Cascades ===
    update_shadow_cascades();

    // === Color Grading from Render Settings ===
    sceneData.colorGrading = glm::vec4(
        _renderSettings.exposure,
        _renderSettings.contrast,
        _renderSettings.saturation,
        0.0f // vibrance (unused for now)
    );
    sceneData.colorTemperature = glm::vec4(
        _renderSettings.temperature,
        _renderSettings.tint,
        static_cast<float>(_renderSettings.tonemapOperator),
        0.0f
    );

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
    newBuffer.size = allocSize;

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

// =============================================================================
// PRIMITIVE MATERIAL SYSTEM
// =============================================================================

void VulkanEngine::init_default_primitive_material() {
    // Create material data buffer for primitive material constants
    _primitiveMaterialDataBuffer = create_buffer(
        sizeof(GLTFMetallic_Roughness::MaterialConstants),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VMA_MEMORY_USAGE_CPU_TO_GPU
    );

    // Initialize with default values (neutral - no modification to push constants)
    auto* constants = static_cast<GLTFMetallic_Roughness::MaterialConstants*>(
        _primitiveMaterialDataBuffer.info.pMappedData);
    constants->colorFactors = glm::vec4(1.0f);  // No color modification
    constants->metal_rough_factors = glm::vec4(1.0f, 1.0f, 0.0f, 0.0f);  // Pass through
    // Clear extra data
    for (int i = 0; i < 13; ++i) {
        constants->extra[i] = glm::vec4(0.0f);
    }

    // IMPORTANT: Use globalDescriptorAllocator (persistent) instead of frame descriptors
    // Frame descriptors get cleared every frame, which would invalidate our material!
    _defaultPrimitiveMaterial.passType = MaterialPass::MainColor;
    _defaultPrimitiveMaterial.pipeline = &metalRoughMaterial.opaquePipeline;
    _defaultPrimitiveMaterial.materialSet = globalDescriptorAllocator.allocate(
        _device, metalRoughMaterial.materialLayout);

    // Write descriptor set manually
    DescriptorWriter writer;
    writer.write_buffer(0, _primitiveMaterialDataBuffer.buffer,
        sizeof(GLTFMetallic_Roughness::MaterialConstants), 0,
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    writer.write_image(1, _whiteImage.imageView, _defaultSamplerLinear,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    writer.write_image(2, _whiteImage.imageView, _defaultSamplerLinear,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    writer.update_set(_device, _defaultPrimitiveMaterial.materialSet);

    fmt::print("[Primitives] Default primitive material initialized (persistent descriptor)\n");
}

VkDescriptorSet StaticMeshData::getMaterialDescriptorSet(VulkanEngine* engine) const {
    // Return custom material's descriptor set if available
    if (material && material->materialSet != VK_NULL_HANDLE) {
        return material->materialSet;
    }
    // Fall back to default primitive material
    return engine->_defaultPrimitiveMaterial.materialSet;
}

MaterialInstance VulkanEngine::create_primitive_material(
    const std::string& albedoPath,
    const std::string& metalRoughPath,
    const std::string& emissionPath)
{
    GLTFMetallic_Roughness::MaterialResources resources;

    // Load albedo texture or use default white
    if (!albedoPath.empty()) {
        // Try to load texture from file
        int width, height, channels;
        unsigned char* data = stbi_load(albedoPath.c_str(), &width, &height, &channels, 4);
        if (data) {
            VkExtent3D size = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };
            resources.colorImage = create_image(data, size, VK_FORMAT_R8G8B8A8_SRGB,
                VK_IMAGE_USAGE_SAMPLED_BIT, true);
            stbi_image_free(data);
        } else {
            fmt::print("[Material] Warning: Failed to load albedo texture: {}\n", albedoPath);
            resources.colorImage = _whiteImage;
        }
    } else {
        resources.colorImage = _whiteImage;
    }
    resources.colorSampler = _defaultSamplerLinear;

    // Load metallic-roughness texture or use default
    if (!metalRoughPath.empty()) {
        int width, height, channels;
        unsigned char* data = stbi_load(metalRoughPath.c_str(), &width, &height, &channels, 4);
        if (data) {
            VkExtent3D size = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };
            resources.metalRoughImage = create_image(data, size, VK_FORMAT_R8G8B8A8_UNORM,
                VK_IMAGE_USAGE_SAMPLED_BIT, true);
            stbi_image_free(data);
        } else {
            fmt::print("[Material] Warning: Failed to load metalrough texture: {}\n", metalRoughPath);
            resources.metalRoughImage = _whiteImage;
        }
    } else {
        resources.metalRoughImage = _whiteImage;
    }
    resources.metalRoughSampler = _defaultSamplerLinear;

    // Create buffer for this material's constants
    AllocatedBuffer materialBuffer = create_buffer(
        sizeof(GLTFMetallic_Roughness::MaterialConstants),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VMA_MEMORY_USAGE_CPU_TO_GPU
    );

    auto* constants = static_cast<GLTFMetallic_Roughness::MaterialConstants*>(
        materialBuffer.info.pMappedData);
    constants->colorFactors = glm::vec4(1.0f);
    constants->metal_rough_factors = glm::vec4(1.0f, 1.0f, 0.0f, 0.0f);

    // Handle emission
    for (int i = 0; i < 13; ++i) {
        constants->extra[i] = glm::vec4(0.0f);
    }

    resources.dataBuffer = materialBuffer.buffer;
    resources.dataBufferOffset = 0;

    // IMPORTANT: Use globalDescriptorAllocator (persistent) instead of frame descriptors!
    // Frame descriptors get cleared every frame, which would invalidate the material.
    MaterialInstance matData;
    matData.passType = MaterialPass::MainColor;
    matData.pipeline = &metalRoughMaterial.opaquePipeline;
    matData.materialSet = globalDescriptorAllocator.allocate(_device, metalRoughMaterial.materialLayout);

    DescriptorWriter descWriter;
    descWriter.write_buffer(0, resources.dataBuffer,
        sizeof(GLTFMetallic_Roughness::MaterialConstants), resources.dataBufferOffset,
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    descWriter.write_image(1, resources.colorImage.imageView, resources.colorSampler,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    descWriter.write_image(2, resources.metalRoughImage.imageView, resources.metalRoughSampler,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    descWriter.update_set(_device, matData.materialSet);

    // Track dynamically created resources for cleanup
    if (resources.colorImage.image != VK_NULL_HANDLE && resources.colorImage.image != _whiteImage.image) {
        _dynamicPrimitiveMaterialImages.push_back(resources.colorImage);
    }
    if (resources.metalRoughImage.image != VK_NULL_HANDLE && resources.metalRoughImage.image != _whiteImage.image) {
        _dynamicPrimitiveMaterialImages.push_back(resources.metalRoughImage);
    }
    _dynamicPrimitiveMaterialBuffers.push_back(materialBuffer);

    fmt::print("[Material] Created persistent primitive material (albedo={}, metalRough={})\n",
        albedoPath.empty() ? "default" : albedoPath, metalRoughPath.empty() ? "default" : metalRoughPath);
    return matData;
}

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
    features12.descriptorBindingVariableDescriptorCount = true;
    features12.descriptorBindingPartiallyBound = true;
    features12.shaderSampledImageArrayNonUniformIndexing = true;

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
    drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_SAMPLED_BIT;  // For bloom downsample sampling

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
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

    _depthImage = create_image(drawImageExtent, VK_FORMAT_D32_SFLOAT,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

    // Descriptor seti de güncelle
    DescriptorWriter writer;
    writer.write_image(0, _drawImage.imageView, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    writer.update_set(_device, _drawImageDescriptors);

    // Update skybox bg descriptor with new draw image
    updateSkyboxBgDescriptor();

    // Resize bloom mip chain
    if (_bloomPass) {
        _bloomPass->onResize(VkExtent2D{ _windowExtent.width, _windowExtent.height });
    }

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

void VulkanEngine::init_renderables()
{
    // === GLTF/GLB Yükleme ===
    std::vector<std::pair<std::string, std::string>> glbFiles = {
        //{ "house", "../../assets/house.glb" },
        /*{ "test", "../../assets/vize.glb" }*/
        //{ "gltfscene", "../../assets/gltfscene/scene.gltf" }
                { "house", "../../assets/house.glb" },
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

void VulkanEngine::init_pipelines() {
    // === COMPUTE ===
    init_background_pipelines();
    init_emissive_pipeline();
    init_outline_wireframe_pipeline();
    // === MATERIAL PIPELINES ===
    metalRoughMaterial.build_pipelines(this);

    // === SHADER-ONLY PIPELINES ===
    init_2d_pipeline(true);
    // Culling açık (önyüz)
    _2dPipelineCulled = _2dPipeline;
    init_2d_pipeline(false);  // Culling kapalı (her iki yüz)
    _2dPipelineDoubleSided = _2dPipeline;

    // === PRIMITIVE PIPELINE (Face colors + lighting + textures) ===
    init_primitive_pipeline();
    init_primitive_wireframe_pipeline();  // Wireframe view mode for primitives
    init_primitive_solid_pipeline();      // Solid view mode for primitives
    // Note: init_default_primitive_material() is called after init_default_data()
    // because it needs _whiteImage to be created first

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

    VkVertexInputAttributeDescription tangentAttrib = {};
    tangentAttrib.binding = 0;
    tangentAttrib.location = 5;
    tangentAttrib.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    tangentAttrib.offset = offsetof(Vertex, tangent);
    description.attributes.push_back(tangentAttrib);

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

    // Use 2 descriptor sets: scene data (Set 0) + material (Set 1) - same as GLTF
    VkDescriptorSetLayout setLayouts[] = {
        _gpuSceneDataDescriptorLayout,      // Set 0: Scene data (camera, lights)
        metalRoughMaterial.materialLayout   // Set 1: Material textures (shared with GLTF)
    };

    VkPushConstantRange pushRange{};
    pushRange.offset = 0;
    pushRange.size = sizeof(PrimitivePushConstants);
    pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 2;  // Changed from 1 to 2
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
// PRIMITIVE WIREFRAME PIPELINE - For wireframe view mode
// =============================================================================
void VulkanEngine::init_primitive_wireframe_pipeline()
{
    // Destroy old pipeline if exists
    if (_primitiveWireframePipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(_device, _primitiveWireframePipeline, nullptr);
        _primitiveWireframePipeline = VK_NULL_HANDLE;
    }

    // Reuse same pipeline layout as regular primitive pipeline
    if (_primitivePipelineLayout == VK_NULL_HANDLE) {
        fmt::print("Warning: Primitive pipeline layout not initialized, skipping wireframe pipeline\n");
        return;
    }

    // Load same shaders as regular primitive pipeline
    VkShaderModule vertShaderModule;
    if (!vkutil::load_shader_module("../../shaders/primitive.vert.spv", _device, &vertShaderModule)) {
        fmt::print("Warning: Failed to load primitive vertex shader for wireframe\n");
        return;
    }

    VkShaderModule fragShaderModule;
    if (!vkutil::load_shader_module("../../shaders/primitive.frag.spv", _device, &fragShaderModule)) {
        vkDestroyShaderModule(_device, vertShaderModule, nullptr);
        fmt::print("Warning: Failed to load primitive fragment shader for wireframe\n");
        return;
    }

    VkPipelineShaderStageCreateInfo shaderStages[] = {
        vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, vertShaderModule),
        vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderModule)
    };

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

    // WIREFRAME MODE - key difference
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_LINE;  // WIREFRAME
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;  // No culling for wireframe
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
    depthStencil.depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;
    depthStencil.stencilTestEnable = VK_FALSE;

    VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    VkFormat colorFormat = _drawImage.imageFormat;
    VkPipelineRenderingCreateInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachmentFormats = &colorFormat;
    renderingInfo.depthAttachmentFormat = _depthImage.imageFormat;

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = &renderingInfo;
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
    pipelineInfo.renderPass = VK_NULL_HANDLE;
    pipelineInfo.subpass = 0;

    VK_CHECK(vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_primitiveWireframePipeline));

    vkDestroyShaderModule(_device, vertShaderModule, nullptr);
    vkDestroyShaderModule(_device, fragShaderModule, nullptr);

    fmt::print("Primitive wireframe pipeline initialized successfully\n");
}

// =============================================================================
// PRIMITIVE SOLID PIPELINE - For solid view mode (no lighting)
// =============================================================================
void VulkanEngine::init_primitive_solid_pipeline()
{
    // Destroy old pipeline if exists
    if (_primitiveSolidPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(_device, _primitiveSolidPipeline, nullptr);
        _primitiveSolidPipeline = VK_NULL_HANDLE;
    }

    if (_primitivePipelineLayout == VK_NULL_HANDLE) {
        fmt::print("Warning: Primitive pipeline layout not initialized, skipping solid pipeline\n");
        return;
    }

    // Use same shaders - the push constants control color behavior
    VkShaderModule vertShaderModule;
    if (!vkutil::load_shader_module("../../shaders/primitive.vert.spv", _device, &vertShaderModule)) {
        fmt::print("Warning: Failed to load primitive vertex shader for solid\n");
        return;
    }

    VkShaderModule fragShaderModule;
    if (!vkutil::load_shader_module("../../shaders/primitive.frag.spv", _device, &fragShaderModule)) {
        vkDestroyShaderModule(_device, vertShaderModule, nullptr);
        fmt::print("Warning: Failed to load primitive fragment shader for solid\n");
        return;
    }

    VkPipelineShaderStageCreateInfo shaderStages[] = {
        vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, vertShaderModule),
        vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderModule)
    };

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

    // Solid fill mode
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
    depthStencil.depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;
    depthStencil.stencilTestEnable = VK_FALSE;

    VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    VkFormat colorFormat = _drawImage.imageFormat;
    VkPipelineRenderingCreateInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachmentFormats = &colorFormat;
    renderingInfo.depthAttachmentFormat = _depthImage.imageFormat;

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = &renderingInfo;
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
    pipelineInfo.renderPass = VK_NULL_HANDLE;
    pipelineInfo.subpass = 0;

    VK_CHECK(vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_primitiveSolidPipeline));

    vkDestroyShaderModule(_device, vertShaderModule, nullptr);
    vkDestroyShaderModule(_device, fragShaderModule, nullptr);

    fmt::print("Primitive solid pipeline initialized successfully\n");
}

// =============================================================================
// DRAW PRIMITIVES - Using new primitive pipeline with face colors and textures
// =============================================================================
void VulkanEngine::draw_primitives(VkCommandBuffer cmd, VkDescriptorSet globalDescriptor)
{
    if (static_shapes.empty()) return;
    if (_primitivePipeline == VK_NULL_HANDLE) return;

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _primitivePipeline);

    // Bind scene data (Set 0) - once for all primitives
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _primitivePipelineLayout,
        0, 1, &globalDescriptor, 0, nullptr);

    // Track last bound material to avoid redundant binds
    VkDescriptorSet lastMaterial = VK_NULL_HANDLE;

    for (auto& shape : static_shapes) {
        if (!shape.visible) continue;
        if (shape.mesh.vertexBuffer.buffer == VK_NULL_HANDLE ||
            shape.mesh.indexBuffer.buffer == VK_NULL_HANDLE ||
            shape.mesh.indexCount == 0) continue;

        // Bind material descriptor set (Set 1) - only if different from last
        VkDescriptorSet materialSet = shape.getMaterialDescriptorSet(this);
        if (materialSet != lastMaterial && materialSet != VK_NULL_HANDLE) {
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _primitivePipelineLayout,
                1, 1, &materialSet, 0, nullptr);
            lastMaterial = materialSet;
        }

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

// Helper to select pipeline based on view mode and material type
VkPipeline VulkanEngine::select_primitive_pipeline(ViewMode viewMode, ShaderOnlyMaterial materialType) {
    // Always fall back to _primitivePipeline if it exists
    VkPipeline fallbackPipeline = _primitivePipeline;
    if (fallbackPipeline == VK_NULL_HANDLE) {
        return VK_NULL_HANDLE;  // No pipeline available
    }

    // Material type overrides view mode for special materials
    switch (materialType) {
    case ShaderOnlyMaterial::WIREFRAME:
        return _primitiveWireframePipeline != VK_NULL_HANDLE ? _primitiveWireframePipeline : fallbackPipeline;

    case ShaderOnlyMaterial::UNLIT:
        // Unlit uses the solid pipeline (no lighting calculations via push constants)
        return _primitiveSolidPipeline != VK_NULL_HANDLE ? _primitiveSolidPipeline : fallbackPipeline;

    case ShaderOnlyMaterial::NORMAL_DEBUG:
        // Normal debug could use a dedicated normals pipeline, falling back to solid then main
        return _primitiveSolidPipeline != VK_NULL_HANDLE ? _primitiveSolidPipeline : fallbackPipeline;

    case ShaderOnlyMaterial::DEFAULT:
    case ShaderOnlyMaterial::PBR:
        // DEFAULT and PBR both use the main primitive pipeline with full PBR lighting
        // But still respect view mode for wireframe/solid overrides
        break;

    case ShaderOnlyMaterial::GRID:
    case ShaderOnlyMaterial::EMISSIVE:
    case ShaderOnlyMaterial::POINTLIGHT_VIS:
        // Internal types - use default primitive pipeline
        return fallbackPipeline;

    default:
        // Unknown type - use default
        return fallbackPipeline;
    }

    // For DEFAULT/PBR materials, use view mode to select pipeline
    switch (viewMode) {
    case ViewMode::Wireframe:
        return _primitiveWireframePipeline != VK_NULL_HANDLE ? _primitiveWireframePipeline : fallbackPipeline;
    case ViewMode::Solid:
        return _primitiveSolidPipeline != VK_NULL_HANDLE ? _primitiveSolidPipeline : fallbackPipeline;
    default:
        return fallbackPipeline;
    }
}

// Wrapper to maintain compatibility with existing call pattern
void VulkanEngine::draw_primitives_with_viewport(VkCommandBuffer cmd, VkDescriptorSet globalDescriptor, VkViewport viewport, VkRect2D scissor, ViewMode viewMode)
{
    if (static_shapes.empty()) return;
    if (_primitivePipelineLayout == VK_NULL_HANDLE) return;

    vkCmdSetViewport(cmd, 0, 1, &viewport);
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    // Track last bound state to avoid redundant binds
    VkDescriptorSet lastMaterial = VK_NULL_HANDLE;
    VkPipeline lastPipeline = VK_NULL_HANDLE;
    bool descriptorSet0Bound = false;

    for (auto& shape : static_shapes) {
        if (!shape.visible) continue;

        // Filter by render pass type - only MainColor and Transparent are rendered
        if (shape.passType != MaterialPass::MainColor && shape.passType != MaterialPass::Transparent) {
            continue;
        }
        if (shape.mesh.vertexBuffer.buffer == VK_NULL_HANDLE ||
            shape.mesh.indexBuffer.buffer == VK_NULL_HANDLE ||
            shape.mesh.indexCount == 0) continue;

        // Select pipeline based on view mode AND material type
        VkPipeline pipelineToUse = select_primitive_pipeline(viewMode, shape.materialType);
        if (pipelineToUse == VK_NULL_HANDLE) continue;

        // Bind pipeline if changed (MUST happen before descriptor set binding)
        if (pipelineToUse != lastPipeline) {
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineToUse);
            lastPipeline = pipelineToUse;

            // After binding pipeline, bind scene data (Set 0) if not already bound
            // This ensures descriptor set is bound AFTER pipeline for proper state
            if (!descriptorSet0Bound && globalDescriptor != VK_NULL_HANDLE) {
                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _primitivePipelineLayout,
                    0, 1, &globalDescriptor, 0, nullptr);
                descriptorSet0Bound = true;
            }
        }

        // Bind material descriptor set (Set 1) - only if different from last
        VkDescriptorSet materialSet = shape.getMaterialDescriptorSet(this);
        if (materialSet != lastMaterial && materialSet != VK_NULL_HANDLE) {
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _primitivePipelineLayout,
                1, 1, &materialSet, 0, nullptr);
            lastMaterial = materialSet;
        }

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

    // b) Scene data + bindless texture array + shadow maps için (set = 0)
    {
        DescriptorLayoutBuilder builder;
        builder.add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT); // sceneData
        builder.add_binding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT); // directional shadowMap
        builder.add_binding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, MAX_SHADOW_POINT_LIGHTS); // point light shadow cubemaps[4]
        builder.add_binding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 5); // environment cubemaps[5]: [0]=sky, [1-4]=probes
        builder.add_binding(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, MAX_BINDLESS_TEXTURES, true);          // allTextures[] (must be last for variable count)
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

    // Transparent pipeline (alpha blending for glass, water, etc.)
    fmt::print("[Pipelines] Transparent pipeline olusturuluyor...\n");
    pipelineBuilder.enable_blending_alphablend();  // Use alpha blend, not additive
    pipelineBuilder.enable_depthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);  // Read depth, but don't write
    pipelineBuilder._depthStencil.depthWriteEnable = VK_FALSE;  // Don't write to depth for transparency
    transparentPipeline.pipeline = pipelineBuilder.build_pipeline(engine->_device);

    // Shader modüllerini temizle
    vkDestroyShaderModule(engine->_device, meshFragShader, nullptr);
    vkDestroyShaderModule(engine->_device, meshVertexShader, nullptr);

    fmt::print("[Pipelines] Basariyla tamamlandi.\n");
}

void GLTFMetallic_Roughness::clear_resources(VkDevice device)
{

}

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
        def.nodePointer = this;

        if (s.material->data.passType == MaterialPass::Transparent)
            ctx.TransparentSurfaces.push_back(def);
        else
            ctx.OpaqueSurfaces.push_back(def);
    }

    Node::Draw(nodeMatrix, ctx);  // Alt node'ları recursive çiz
}

// TextureCache::AddTexture is now defined inline in vk_types.h

// =============================================================================
// ANIMATION SYSTEM IMPLEMENTATION
// =============================================================================

void VulkanEngine::updateAnimations(float deltaTime) {
    for (auto& clip : animationClips) {
        if (clip.isPlaying) {
            clip.currentTime += deltaTime * clip.speed;
            if (clip.currentTime >= clip.duration) {
                if (clip.loop) {
                    clip.currentTime = fmod(clip.currentTime, clip.duration);
                } else {
                    clip.currentTime = clip.duration;
                    clip.isPlaying = false;
                }
            }
        }
    }
}

void VulkanEngine::playAnimation(int index) {
    if (index >= 0 && index < static_cast<int>(animationClips.size())) {
        animationClips[index].isPlaying = true;
        activeAnimationIndex = index;
    }
}

void VulkanEngine::stopAnimation(int index) {
    if (index >= 0 && index < static_cast<int>(animationClips.size())) {
        animationClips[index].isPlaying = false;
        animationClips[index].currentTime = 0.0f;
    }
}

void VulkanEngine::addAnimationClip(const AnimationClipData& clip) {
    animationClips.push_back(clip);
}

// =============================================================================
// PHYSICS SYSTEM IMPLEMENTATION
// =============================================================================

void VulkanEngine::updatePhysics(float deltaTime) {
    if (!physicsEnabled || physicsSettings.paused) return;

    // Simple physics simulation (no actual physics engine)
    for (auto& body : physicsBodies) {
        if (body.type == 1 && body.isAwake) {  // Dynamic body
            // Apply gravity
            body.velocity += physicsSettings.gravity * deltaTime;
            // Update position
            body.position += body.velocity * deltaTime;
            // Simple ground collision
            if (body.position.y < 0.0f) {
                body.position.y = 0.0f;
                body.velocity.y = -body.velocity.y * body.restitution;
                if (glm::length(body.velocity) < 0.1f) {
                    body.velocity = glm::vec3(0.0f);
                    body.isAwake = false;
                }
            }
        }
    }
}

void VulkanEngine::addPhysicsBody(const PhysicsBodyData& body) {
    physicsBodies.push_back(body);
}

void VulkanEngine::removePhysicsBody(int index) {
    if (index >= 0 && index < static_cast<int>(physicsBodies.size())) {
        physicsBodies.erase(physicsBodies.begin() + index);
    }
}

void VulkanEngine::setPhysicsPaused(bool paused) {
    physicsSettings.paused = paused;
}

// =============================================================================
// PLUGIN/SUBSYSTEM SYSTEM IMPLEMENTATION
// =============================================================================

void VulkanEngine::initSubsystems() {
    // Register core subsystems
    subsystems.clear();

    SubsystemInfo vulkanRenderer;
    vulkanRenderer.id = "vulkan-renderer";
    vulkanRenderer.name = "Vulkan Renderer";
    vulkanRenderer.version = "1.0.0";
    vulkanRenderer.state = SubsystemState::Active;
    vulkanRenderer.isCore = true;
    vulkanRenderer.loadTimeMs = 125.5f;
    vulkanRenderer.memoryUsage = 256 * 1024 * 1024;
    subsystems.push_back(vulkanRenderer);

    SubsystemInfo sceneManager;
    sceneManager.id = "scene-manager";
    sceneManager.name = "Scene Manager";
    sceneManager.version = "1.0.0";
    sceneManager.state = SubsystemState::Active;
    sceneManager.isCore = true;
    sceneManager.loadTimeMs = 45.2f;
    sceneManager.memoryUsage = 64 * 1024 * 1024;
    subsystems.push_back(sceneManager);

    SubsystemInfo materialSystem;
    materialSystem.id = "material-system";
    materialSystem.name = "PBR Materials";
    materialSystem.version = "1.2.0";
    materialSystem.state = SubsystemState::Active;
    materialSystem.isCore = true;
    materialSystem.loadTimeMs = 78.3f;
    materialSystem.memoryUsage = 128 * 1024 * 1024;
    subsystems.push_back(materialSystem);

    SubsystemInfo gltfLoader;
    gltfLoader.id = "gltf-loader";
    gltfLoader.name = "GLTF Loader";
    gltfLoader.version = "1.0.0";
    gltfLoader.state = SubsystemState::Active;
    gltfLoader.isCore = false;
    gltfLoader.loadTimeMs = 34.2f;
    gltfLoader.memoryUsage = 16 * 1024 * 1024;
    subsystems.push_back(gltfLoader);

    SubsystemInfo imguiSystem;
    imguiSystem.id = "imgui-ui";
    imguiSystem.name = "ImGui UI";
    imguiSystem.version = "1.89.9";
    imguiSystem.state = SubsystemState::Active;
    imguiSystem.isCore = true;
    imguiSystem.loadTimeMs = 22.1f;
    imguiSystem.memoryUsage = 32 * 1024 * 1024;
    subsystems.push_back(imguiSystem);
}

void VulkanEngine::registerSubsystem(const SubsystemInfo& info) {
    subsystems.push_back(info);
}

SubsystemInfo* VulkanEngine::getSubsystem(const std::string& id) {
    for (auto& sys : subsystems) {
        if (sys.id == id) return &sys;
    }
    return nullptr;
}

// =============================================================================
// SHADER SYSTEM IMPLEMENTATION
// =============================================================================

void VulkanEngine::registerShaderPipeline(const ShaderPipelineInfo& info) {
    shaderPipelines.push_back(info);
}

void VulkanEngine::recompileShader(int index) {
    if (index >= 0 && index < static_cast<int>(shaderPipelines.size())) {
        // In a real implementation, this would recompile the shader
        shaderPipelines[index].isValid = true;
        shaderPipelines[index].errorLog.clear();
    }
}

void VulkanEngine::recompileAllShaders() {
    for (size_t i = 0; i < shaderPipelines.size(); ++i) {
        recompileShader(static_cast<int>(i));
    }
}

// =============================================================================
// GLTF CAMERA MANAGEMENT
// =============================================================================

void VulkanEngine::applyGLTFCamera(const std::string& sceneName, int cameraIndex) {
    auto it = loadedScenes.find(sceneName);
    if (it == loadedScenes.end()) {
        fmt::print("[Camera] Scene '{}' not found\n", sceneName);
        return;
    }

    auto& scene = it->second;
    if (cameraIndex < 0 || cameraIndex >= static_cast<int>(scene->cameras.size())) {
        fmt::print("[Camera] Invalid camera index {} for scene '{}'\n", cameraIndex, sceneName);
        return;
    }

    GLTFCamera& gltfCam = scene->cameras[cameraIndex];

    // Apply GLTF camera settings to main camera
    mainCamera.position = gltfCam.position;
    mainCamera.fov = gltfCam.fov;
    mainCamera.nearPlane = gltfCam.nearPlane;
    mainCamera.farPlane = gltfCam.farPlane;

    // Calculate pitch and yaw from forward direction
    glm::vec3 forward = gltfCam.forward;
    mainCamera.yaw = atan2(forward.x, -forward.z);
    mainCamera.pitch = asin(glm::clamp(forward.y, -1.0f, 1.0f));

    // Sync target values for smooth camera
    mainCamera.targetPitch = mainCamera.pitch;
    mainCamera.targetYaw = mainCamera.yaw;
    mainCamera.targetPosition = mainCamera.position;
    mainCamera.targetFov = mainCamera.fov;

    currentGLTFCameraScene = sceneName;
    currentGLTFCameraIndex = cameraIndex;
    useGLTFCamera = true;

    fmt::print("[Camera] Applied GLTF camera '{}' from scene '{}'\n",
        gltfCam.name, sceneName);
}

void VulkanEngine::resetToFreeCamera() {
    useGLTFCamera = false;
    currentGLTFCameraIndex = -1;
    currentGLTFCameraScene.clear();
    fmt::print("[Camera] Reset to free camera mode\n");
}

GLTFCamera* VulkanEngine::getCurrentGLTFCamera() {
    if (!useGLTFCamera || currentGLTFCameraIndex < 0) {
        return nullptr;
    }

    auto it = loadedScenes.find(currentGLTFCameraScene);
    if (it == loadedScenes.end()) {
        return nullptr;
    }

    auto& scene = it->second;
    if (currentGLTFCameraIndex >= static_cast<int>(scene->cameras.size())) {
        return nullptr;
    }

    return &scene->cameras[currentGLTFCameraIndex];
}

std::vector<std::pair<std::string, GLTFCamera*>> VulkanEngine::getAllGLTFCameras() {
    std::vector<std::pair<std::string, GLTFCamera*>> result;

    for (auto& [sceneName, scene] : loadedScenes) {
        for (auto& camera : scene->cameras) {
            std::string fullName = sceneName + "/" + camera.name;
            result.emplace_back(fullName, &camera);
        }
    }

    return result;
}

// =============================================================================
// POST-PROCESSING SYSTEM
// =============================================================================

void VulkanEngine::init_post_processing() {
    fmt::print("[PostProcess] Initializing post-processing system...\n");

    // Create post-process manager
    _postProcessManager = std::make_unique<Yalaz::Renderer::PostProcessManager>(this);

    // Initialize with default settings
    _renderSettings = Yalaz::Renderer::RenderSettings{};

    // Create and initialize bloom pass
    _bloomPass = std::make_unique<Yalaz::Renderer::BloomPass>(this);
    _bloomPass->init();
    fmt::print("[PostProcess] Bloom pass initialized\n");

    // Create tonemap compute pipeline (runs after bloom)
    init_tonemap_pipeline();

    fmt::print("[PostProcess] Post-processing system initialized\n");
}

void VulkanEngine::cleanup_post_processing() {
    if (_bloomPass) {
        _bloomPass.reset();
    }
    if (_postProcessManager) {
        _postProcessManager.reset();
    }
    if (_tonemapPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(_device, _tonemapPipeline, nullptr);
        _tonemapPipeline = VK_NULL_HANDLE;
    }
    if (_tonemapPipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(_device, _tonemapPipelineLayout, nullptr);
        _tonemapPipelineLayout = VK_NULL_HANDLE;
    }
}

void VulkanEngine::render_post_processing(VkCommandBuffer cmd) {
    // Placeholder - will be integrated into main render loop
    (void)cmd;
}

void VulkanEngine::init_tonemap_pipeline() {
    // Push constant: matches tonemap_final.comp PushConstants layout
    struct TonemapPushConstants {
        float exposure;
        float contrast;
        float saturation;
        float temperature;
        float tint;
        int tonemapOperator;
        int _pad0;
        int _pad1;
    };

    // Pipeline layout: uses the drawImage descriptor set (storage image at binding 0)
    VkPushConstantRange pushRange{};
    pushRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pushRange.offset = 0;
    pushRange.size = sizeof(TonemapPushConstants);

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &_drawImageDescriptorLayout;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &pushRange;

    VK_CHECK(vkCreatePipelineLayout(_device, &layoutInfo, nullptr, &_tonemapPipelineLayout));

    // Load shader
    VkShaderModule tonemapShader = load_shader_module("../../shaders/tonemap_final.comp.spv");

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.layout = _tonemapPipelineLayout;
    pipelineInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipelineInfo.stage.module = tonemapShader;
    pipelineInfo.stage.pName = "main";

    VK_CHECK(vkCreateComputePipelines(_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_tonemapPipeline));

    vkDestroyShaderModule(_device, tonemapShader, nullptr);

    fmt::print("[PostProcess] Tonemap compute pipeline initialized\n");
}

void VulkanEngine::execute_tonemap(VkCommandBuffer cmd) {
    // Push constants matching tonemap_final.comp
    struct TonemapPushConstants {
        float exposure;
        float contrast;
        float saturation;
        float temperature;
        float tint;
        int tonemapOperator;
        int _pad0;
        int _pad1;
    };

    TonemapPushConstants pc{};
    pc.exposure = _renderSettings.exposure;
    pc.contrast = _renderSettings.contrast;
    pc.saturation = _renderSettings.saturation;
    pc.temperature = _renderSettings.temperature;
    pc.tint = _renderSettings.tint;
    pc.tonemapOperator = _renderSettings.tonemapOperator;

    // _drawImage must be in GENERAL layout for imageLoad/imageStore
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _tonemapPipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _tonemapPipelineLayout,
        0, 1, &_drawImageDescriptors, 0, nullptr);
    vkCmdPushConstants(cmd, _tonemapPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT,
        0, sizeof(TonemapPushConstants), &pc);

    uint32_t groupsX = (_drawExtent.width + 7) / 8;
    uint32_t groupsY = (_drawExtent.height + 7) / 8;
    vkCmdDispatch(cmd, groupsX, groupsY, 1);
}

void VulkanEngine::init_gbuffer() {
    // G-Buffer initialization for deferred effects
    // Normals buffer
    VkExtent3D extent = { _drawExtent.width, _drawExtent.height, 1 };

    _gBufferNormals = create_image(
        extent,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        false
    );

    _gBufferMetalRough = create_image(
        extent,
        VK_FORMAT_R8G8_UNORM,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        false
    );

    fmt::print("[GBuffer] Created G-buffer targets ({}x{})\n",
        _drawExtent.width, _drawExtent.height);
}

// =============================================================================
// PATH TRACING SYSTEM
// =============================================================================

void VulkanEngine::init_path_tracer() {
    fmt::print("[PathTracer] Initializing path tracing system...\n");

    _pathTracer = std::make_unique<Yalaz::Renderer::PathTracer>(this);
    _pathTracer->init();

    fmt::print("[PathTracer] Path tracing system initialized\n");
}

void VulkanEngine::cleanup_path_tracer() {
    if (_pathTracer) {
        _pathTracer->cleanup();
        _pathTracer.reset();
    }
}

// =============================================================================
// ENVIRONMENT MAP / SKYBOX SYSTEM
// =============================================================================

void VulkanEngine::init_environment_map() {
    fmt::print("[Environment] Initializing environment map system...\n");

    _environmentMap = std::make_unique<Yalaz::Renderer::EnvironmentMap>(this);
    _environmentMap->init();

    // Connect cubemap to path tracer for sky and reflections on surfaces
    if (_pathTracer && _environmentMap->getEnvironmentCubemap() != VK_NULL_HANDLE) {
        _pathTracer->setEnvironmentCubemap(
            _environmentMap->getEnvironmentCubemap(),
            _environmentMap->getSampler()
        );
        fmt::print("[Environment] Connected cubemap to path tracer\n");
    }

    // Update skybox background descriptor with the cubemap
    updateSkyboxBgDescriptor();

    fmt::print("[Environment] Environment map system initialized\n");
}

void VulkanEngine::cleanup_environment_map() {
    if (_environmentMap) {
        _environmentMap->cleanup();
        _environmentMap.reset();
    }
}

// =============================================================================
// MULTI-PROBE REFLECTION SYSTEM
// =============================================================================

void VulkanEngine::init_reflection_probes() {
    const uint32_t size = REFLECTION_PROBE_SIZE;

    // Default probe positions (4 corners)
    _reflectionProbes[0].position = glm::vec3(-30.0f, 5.0f, -30.0f);
    _reflectionProbes[1].position = glm::vec3( 30.0f, 5.0f, -30.0f);
    _reflectionProbes[2].position = glm::vec3(-30.0f, 5.0f,  30.0f);
    _reflectionProbes[3].position = glm::vec3( 30.0f, 5.0f,  30.0f);

    // Create shared depth buffer (reused for all probe face renders)
    VkExtent3D depthExtent = { size, size, 1 };
    _sharedProbeDepth = create_image(depthExtent, VK_FORMAT_D32_SFLOAT,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

    // Create cubemaps for each probe
    for (int p = 0; p < MAX_REFLECTION_PROBES; ++p) {
        VkImageCreateInfo cubemapInfo{};
        cubemapInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        cubemapInfo.imageType = VK_IMAGE_TYPE_2D;
        cubemapInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
        cubemapInfo.extent = { size, size, 1 };
        cubemapInfo.mipLevels = 1;
        cubemapInfo.arrayLayers = 6;
        cubemapInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        cubemapInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        cubemapInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        cubemapInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        cubemapInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        VK_CHECK(vmaCreateImage(_allocator, &cubemapInfo, &allocInfo,
            &_reflectionProbes[p].cubemap.image, &_reflectionProbes[p].cubemap.allocation, nullptr));
        _reflectionProbes[p].cubemap.imageExtent = { size, size, 1 };
        _reflectionProbes[p].cubemap.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;

        // Create full cubemap view (for shader sampling)
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = _reflectionProbes[p].cubemap.image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
        viewInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 6;
        VK_CHECK(vkCreateImageView(_device, &viewInfo, nullptr, &_reflectionProbes[p].cubemapView));

        // Create per-face views (for rendering to individual faces)
        for (int face = 0; face < 6; ++face) {
            VkImageViewCreateInfo faceViewInfo{};
            faceViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            faceViewInfo.image = _reflectionProbes[p].cubemap.image;
            faceViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            faceViewInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
            faceViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            faceViewInfo.subresourceRange.baseMipLevel = 0;
            faceViewInfo.subresourceRange.levelCount = 1;
            faceViewInfo.subresourceRange.baseArrayLayer = face;
            faceViewInfo.subresourceRange.layerCount = 1;
            VK_CHECK(vkCreateImageView(_device, &faceViewInfo, nullptr, &_reflectionProbes[p].faceViews[face]));
        }

        // Transition cubemap to shader read layout initially
        immediate_submit([&](VkCommandBuffer cmd) {
            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = _reflectionProbes[p].cubemap.image;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 6;
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                0, nullptr, 0, nullptr, 1, &barrier);
        });
    }

    _probesReady = true;
    fmt::print("[Reflection] {} reflection probes initialized ({}x{} per face)\n", MAX_REFLECTION_PROBES, size, size);
}

void VulkanEngine::cleanup_reflection_probes() {
    vkDeviceWaitIdle(_device);

    for (int p = 0; p < MAX_REFLECTION_PROBES; ++p) {
        for (int face = 0; face < 6; ++face) {
            if (_reflectionProbes[p].faceViews[face] != VK_NULL_HANDLE) {
                vkDestroyImageView(_device, _reflectionProbes[p].faceViews[face], nullptr);
                _reflectionProbes[p].faceViews[face] = VK_NULL_HANDLE;
            }
        }
        if (_reflectionProbes[p].cubemapView != VK_NULL_HANDLE) {
            vkDestroyImageView(_device, _reflectionProbes[p].cubemapView, nullptr);
            _reflectionProbes[p].cubemapView = VK_NULL_HANDLE;
        }
        if (_reflectionProbes[p].cubemap.image != VK_NULL_HANDLE) {
            vmaDestroyImage(_allocator, _reflectionProbes[p].cubemap.image, _reflectionProbes[p].cubemap.allocation);
            _reflectionProbes[p].cubemap = {};
        }
    }

    if (_sharedProbeDepth.image != VK_NULL_HANDLE) {
        destroy_image(_sharedProbeDepth);
        _sharedProbeDepth = {};
    }

    _probesReady = false;
}

glm::mat4 VulkanEngine::getReflectionFaceViewMatrix(int face, const glm::vec3& probePos) const {
    static const glm::vec3 targets[] = {
        { 1, 0, 0},  // +X
        {-1, 0, 0},  // -X
        { 0, 1, 0},  // +Y
        { 0,-1, 0},  // -Y
        { 0, 0, 1},  // +Z
        { 0, 0,-1},  // -Z
    };
    static const glm::vec3 ups[] = {
        { 0,-1, 0},  // +X
        { 0,-1, 0},  // -X
        { 0, 0, 1},  // +Y
        { 0, 0,-1},  // -Y
        { 0,-1, 0},  // +Z
        { 0,-1, 0},  // -Z
    };
    return glm::lookAt(probePos, probePos + targets[face], ups[face]);
}

glm::mat4 VulkanEngine::getReflectionProjectionMatrix() const {
    glm::mat4 proj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 500.0f);
    proj[1][1] *= -1; // Vulkan Y-flip
    return proj;
}

void VulkanEngine::render_reflection_probe_single(VkCommandBuffer cmd, int probeIndex) {
    if (probeIndex < 0 || probeIndex >= MAX_REFLECTION_PROBES) return;
    if (!_reflectionProbes[probeIndex].active) return;
    if (_reflectionProbes[probeIndex].cubemap.image == VK_NULL_HANDLE) return;
    if (drawCommands.OpaqueSurfaces.empty() && static_shapes.empty()) return;

    const uint32_t size = REFLECTION_PROBE_SIZE;
    ReflectionProbe& probe = _reflectionProbes[probeIndex];

    // Save current scene data
    glm::mat4 savedView = sceneData.view;
    glm::mat4 savedProj = sceneData.proj;
    glm::mat4 savedViewProj = sceneData.viewproj;
    glm::vec4 savedCamPos = sceneData.cameraPosition;

    glm::mat4 projMatrix = getReflectionProjectionMatrix();

    // Transition cubemap to color attachment for writing
    VkImageMemoryBarrier toAttach{};
    toAttach.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    toAttach.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    toAttach.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    toAttach.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toAttach.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toAttach.image = probe.cubemap.image;
    toAttach.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    toAttach.subresourceRange.baseMipLevel = 0;
    toAttach.subresourceRange.levelCount = 1;
    toAttach.subresourceRange.baseArrayLayer = 0;
    toAttach.subresourceRange.layerCount = 6;
    toAttach.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    toAttach.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0,
        0, nullptr, 0, nullptr, 1, &toAttach);

    // Render each face
    for (int face = 0; face < 6; ++face) {
        glm::mat4 viewMatrix = getReflectionFaceViewMatrix(face, probe.position);

        // Update scene data for this face - CRITICAL: set probeCount=0 to prevent recursion
        sceneData.view = viewMatrix;
        sceneData.proj = projMatrix;
        sceneData.viewproj = projMatrix * viewMatrix;
        sceneData.cameraPosition = glm::vec4(probe.position, 1.0f);
        sceneData.probeSettings = glm::vec4(0.0f); // No probes during probe rendering

        AllocatedBuffer probeSceneBuffer = create_buffer(sizeof(GPUSceneData),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

        get_current_frame()._deletionQueue.push_function([=, this]() {
            destroy_buffer(probeSceneBuffer);
        });

        GPUSceneData* probeData = (GPUSceneData*)probeSceneBuffer.allocation->GetMappedData();
        *probeData = sceneData;

        // Create descriptor set for this face
        VkDescriptorSetVariableDescriptorCountAllocateInfo allocArrayInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO
        };
        uint32_t descriptorCounts = std::max(1u, static_cast<uint32_t>(texCache.Cache.size()));
        allocArrayInfo.pDescriptorCounts = &descriptorCounts;
        allocArrayInfo.descriptorSetCount = 1;

        VkDescriptorSet probeDescriptor = get_current_frame()._frameDescriptors.allocate(
            _device, _gpuSceneDataDescriptorLayout, &allocArrayInfo);

        DescriptorWriter writer;
        writer.write_buffer(0, probeSceneBuffer.buffer, sizeof(GPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

        // Shadow map binding (binding 2)
        if (_shadowMapView != VK_NULL_HANDLE && _shadowSampler != VK_NULL_HANDLE) {
            writer.write_image(2, _shadowMapView, _shadowSampler,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        }

        // Point light shadow cubemaps (binding 3)
        std::array<VkDescriptorImageInfo, MAX_SHADOW_POINT_LIGHTS> shadowCubemapInfos{};
        if (_pointLightShadowSampler != VK_NULL_HANDLE) {
            for (uint32_t i = 0; i < MAX_SHADOW_POINT_LIGHTS; i++) {
                shadowCubemapInfos[i].sampler = _pointLightShadowSampler;
                shadowCubemapInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                if (_pointLightShadows[i].cubemapView != VK_NULL_HANDLE) {
                    shadowCubemapInfos[i].imageView = _pointLightShadows[i].cubemapView;
                } else if (_pointLightShadows[0].cubemapView != VK_NULL_HANDLE) {
                    shadowCubemapInfos[i].imageView = _pointLightShadows[0].cubemapView;
                } else {
                    shadowCubemapInfos[i].imageView = _errorCheckerboardImage.imageView;
                }
            }
            VkWriteDescriptorSet cubemapWrite{ .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
            cubemapWrite.dstSet = probeDescriptor;
            cubemapWrite.dstBinding = 3;
            cubemapWrite.dstArrayElement = 0;
            cubemapWrite.descriptorCount = MAX_SHADOW_POINT_LIGHTS;
            cubemapWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            cubemapWrite.pImageInfo = shadowCubemapInfos.data();
            writer.writes.push_back(cubemapWrite);
        }

        // Environment cubemap array (binding 4) - ALL sky during probe rendering (no recursion)
        std::array<VkDescriptorImageInfo, 5> probeEnvInfos{};
        {
            VkSampler sharedSampler = _defaultSamplerLinear;
            VkImageView skyView = _defaultCubemap.imageView;
            if (_environmentMap && _environmentMap->getSampler() != VK_NULL_HANDLE) {
                sharedSampler = _environmentMap->getSampler();
            }
            if (_environmentMap && _environmentMap->getEnvironmentCubemap() != VK_NULL_HANDLE) {
                skyView = _environmentMap->getEnvironmentCubemap();
            }
            for (int i = 0; i < 5; ++i) {
                probeEnvInfos[i] = { sharedSampler, skyView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
            }
            VkWriteDescriptorSet envWrite{ .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
            envWrite.dstSet = probeDescriptor;
            envWrite.dstBinding = 4;
            envWrite.dstArrayElement = 0;
            envWrite.descriptorCount = 5;
            envWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            envWrite.pImageInfo = probeEnvInfos.data();
            writer.writes.push_back(envWrite);
        }

        // Bindless textures (binding 5)
        if (!texCache.Cache.empty()) {
            VkWriteDescriptorSet arraySet{ .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
            arraySet.descriptorCount = descriptorCounts;
            arraySet.dstArrayElement = 0;
            arraySet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            arraySet.dstBinding = 5;
            arraySet.pImageInfo = texCache.Cache.data();
            writer.writes.push_back(arraySet);
        }

        writer.update_set(_device, probeDescriptor);

        // Transition depth to depth attachment
        vkutil::transition_image(cmd, _sharedProbeDepth.image,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

        // Begin rendering to this cubemap face
        VkRenderingAttachmentInfo colorAttachment{};
        colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachment.imageView = probe.faceViews[face];
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.clearValue.color = { 0.0f, 0.0f, 0.0f, 1.0f };

        VkRenderingAttachmentInfo depthAttachment{};
        depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        depthAttachment.imageView = _sharedProbeDepth.imageView;
        depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.clearValue.depthStencil = { 0.0f, 0 };

        VkRenderingInfo renderInfo{};
        renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        renderInfo.renderArea = { {0, 0}, {size, size} };
        renderInfo.layerCount = 1;
        renderInfo.colorAttachmentCount = 1;
        renderInfo.pColorAttachments = &colorAttachment;
        renderInfo.pDepthAttachment = &depthAttachment;

        vkCmdBeginRendering(cmd, &renderInfo);

        VkViewport vp = { 0, 0, (float)size, (float)size, 0.0f, 1.0f };
        VkRect2D sc = { {0, 0}, {size, size} };

        // Draw GLTF meshes (opaque only)
        if (!drawCommands.OpaqueSurfaces.empty()) {
            std::vector<uint32_t> allDraws;
            allDraws.reserve(drawCommands.OpaqueSurfaces.size());
            for (uint32_t i = 0; i < drawCommands.OpaqueSurfaces.size(); i++) {
                allDraws.push_back(i);
            }
            draw_rendered(cmd, probeDescriptor, vp, sc, allDraws);
        }

        // Draw primitives
        if (!static_shapes.empty() && _primitivePipelineLayout != VK_NULL_HANDLE) {
            draw_primitives_with_viewport(cmd, probeDescriptor, vp, sc, ViewMode::Rendered);
        }

        vkCmdEndRendering(cmd);
    }

    // Transition cubemap back to shader read for sampling
    VkImageMemoryBarrier toRead{};
    toRead.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    toRead.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    toRead.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    toRead.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toRead.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toRead.image = probe.cubemap.image;
    toRead.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    toRead.subresourceRange.baseMipLevel = 0;
    toRead.subresourceRange.levelCount = 1;
    toRead.subresourceRange.baseArrayLayer = 0;
    toRead.subresourceRange.layerCount = 6;
    toRead.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    toRead.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
        0, nullptr, 0, nullptr, 1, &toRead);

    // Restore original scene data
    sceneData.view = savedView;
    sceneData.proj = savedProj;
    sceneData.viewproj = savedViewProj;
    sceneData.cameraPosition = savedCamPos;

    probe.needsUpdate = false;
}