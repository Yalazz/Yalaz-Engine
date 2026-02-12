// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

// Platform-specific Vulkan configuration
#ifdef __APPLE__
    // macOS: Enable MoltenVK portability extensions
    #define VK_ENABLE_BETA_EXTENSIONS
    #ifndef VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
        #define VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME "VK_KHR_portability_enumeration"
    #endif
    #ifndef VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
        #define VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME "VK_KHR_portability_subset"
    #endif
#endif

#include <vulkan/vulkan.h>

//we will add our main reusable types here

#include <SDL.h>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <span>
#include <array>
#include <functional>
#include <deque>
#include <glm/glm.hpp>
#include <iostream>

#include <fmt/core.h>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vulkan/vk_enum_string_helper.h>
#include <vk_mem_alloc.h>

#include <fmt/core.h>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>


// we will add our main reusable types here
struct AllocatedImage {
    VkImage image = VK_NULL_HANDLE;
    VkImageView imageView = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VkExtent3D imageExtent = {};
    VkFormat imageFormat = {};
};

struct AllocatedBuffer {
    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VmaAllocationInfo info = {};
    size_t size = 0;
};




struct GPUGLTFMaterial {
    glm::vec4 colorFactors;
    glm::vec4 metal_rough_factors;
    glm::vec4 extra[14];
};

static_assert(sizeof(GPUGLTFMaterial) == 256);

// =============================================================================
// POINT LIGHT SYSTEM - GPU-aligned structures for Vulkan uniform buffers
// =============================================================================
// IMPORTANT: These structs use std140 layout rules for GPU compatibility.
// vec3 in std140 aligns to 16 bytes, but a vec3 followed by float packs correctly.
// Keep C++ and GLSL structs synchronized!
// =============================================================================

constexpr int MAX_POINT_LIGHTS = 64;
constexpr int MAX_SHADOW_CASTING_LIGHTS = 4;  // Max point lights that can cast shadows

// GPU-only point light structure (32 bytes, matches std140 layout)
// Layout: [position.xyz, radius] [color.xyz, intensity] = 2 x vec4 = 32 bytes
// This struct is used ONLY in the GPU uniform buffer - no CPU-only fields allowed!
struct GPUPointLight {
    glm::vec3 position;     // offset 0,  size 12
    float radius;           // offset 12, size 4  (packs with vec3)

    glm::vec3 color;        // offset 16, size 12
    float intensity;        // offset 28, size 4  (packs with vec3)
    // Total: 32 bytes per light
};
static_assert(sizeof(GPUPointLight) == 32, "GPUPointLight must be 32 bytes");

// CPU-side point light structure (used in scenePointLights array)
// Contains GPU data + CPU-only tracking fields for shadow casting
struct PointLight {
    glm::vec3 position;
    float radius;
    glm::vec3 color;
    float intensity;

    // CPU-only fields for shadow tracking
    bool castsShadow = false;  // Enable shadow casting for this light
    int shadowIndex = -1;      // Index into shadow cubemap array (-1 = no shadow)

    // Convert to GPU-only struct for uniform buffer
    GPUPointLight toGPU() const {
        return GPUPointLight{ position, radius, color, intensity };
    }
};

// =============================================================================
// SPOT LIGHT SYSTEM - GPU-aligned structures for Vulkan uniform buffers
// =============================================================================
constexpr int MAX_SPOT_LIGHTS = 16;
constexpr int MAX_SHADOW_CASTING_SPOT_LIGHTS = 4;

// GPU-only spot light structure (64 bytes, matches std140 layout)
struct GPUSpotLight {
    glm::vec3 position;       // offset 0,  size 12
    float innerConeAngle;     // offset 12, size 4 (cosine of inner cone angle)

    glm::vec3 direction;      // offset 16, size 12
    float outerConeAngle;     // offset 28, size 4 (cosine of outer cone angle)

    glm::vec3 color;          // offset 32, size 12
    float intensity;          // offset 44, size 4

    float range;              // offset 48, size 4 (max distance)
    int shadowMapIndex;       // offset 52, size 4 (-1 = no shadow)
    float _pad[2];            // offset 56, size 8 (padding to 64 bytes)
    // Total: 64 bytes per light
};
static_assert(sizeof(GPUSpotLight) == 64, "GPUSpotLight must be 64 bytes");

// CPU-side spot light structure
struct SpotLight {
    glm::vec3 position = glm::vec3(0.0f, 5.0f, 0.0f);
    glm::vec3 direction = glm::vec3(0.0f, -1.0f, 0.0f);
    glm::vec3 color = glm::vec3(1.0f);
    float intensity = 10.0f;
    float range = 20.0f;
    float innerConeAngle = 25.0f;  // Degrees
    float outerConeAngle = 35.0f;  // Degrees

    // CPU-only fields
    bool castsShadow = true;
    int shadowMapIndex = -1;
    std::string name = "SpotLight";

    // Convert to GPU struct
    GPUSpotLight toGPU() const {
        GPUSpotLight gpu{};
        gpu.position = position;
        gpu.direction = glm::normalize(direction);
        gpu.color = color;
        gpu.intensity = intensity;
        gpu.range = range;
        gpu.innerConeAngle = glm::cos(glm::radians(innerConeAngle));
        gpu.outerConeAngle = glm::cos(glm::radians(outerConeAngle));
        gpu.shadowMapIndex = castsShadow ? shadowMapIndex : -1;
        return gpu;
    }

    // Calculate view-projection matrix for shadow mapping
    glm::mat4 getViewProjMatrix() const {
        glm::mat4 view = glm::lookAt(position, position + direction, glm::vec3(0, 1, 0));
        float fov = glm::radians(outerConeAngle * 2.0f);
        glm::mat4 proj = glm::perspective(fov, 1.0f, 0.1f, range);
        return proj * view;
    }
};

// GPU Scene Data - Uniform buffer structure (std140 layout)
// Contains view/projection matrices, lighting info, point lights, and shadow data
constexpr int SHADOW_CASCADE_COUNT = 4;

struct alignas(16) GPUSceneData {
    // === Camera Matrices (192 bytes) ===
    glm::mat4 view;                             // offset 0,   size 64
    glm::mat4 proj;                             // offset 64,  size 64
    glm::mat4 viewproj;                         // offset 128, size 64

    // === Global Lighting (48 bytes) ===
    glm::vec4 ambientColor;                     // offset 192, size 16 (rgb = color, a = intensity)
    glm::vec4 sunlightDirection;                // offset 208, size 16 (xyz = dir, w = intensity)
    glm::vec4 sunlightColor;                    // offset 224, size 16

    // === Camera Info for Specular (16 bytes) ===
    glm::vec4 cameraPosition;                   // offset 240, size 16 (xyz = pos, w = unused)

    // === Point Light Array (2048 bytes) ===
    GPUPointLight pointLights[MAX_POINT_LIGHTS];   // offset 256, size 64 * 32 = 2048

    // === Point Light Count + Shadow Settings (16 bytes) ===
    int pointLightCount;                        // offset 2304, size 4
    float shadowBias;                           // offset 2308, size 4
    float shadowNormalBias;                     // offset 2312, size 4
    int shadowsEnabled;                         // offset 2316, size 4 (bool as int for GPU)

    // === Shadow Cascade Matrices (256 bytes) ===
    glm::mat4 shadowMatrices[SHADOW_CASCADE_COUNT]; // offset 2320, size 64 * 4 = 256

    // === Shadow Cascade Split Depths (16 bytes) ===
    glm::vec4 cascadeSplits;                    // offset 2576, size 16 (x,y,z,w = split distances)

    // === Point Light Shadow Data (96 bytes) ===
    // For each shadow-casting point light: position + far plane
    glm::vec4 pointLightShadowData[MAX_SHADOW_CASTING_LIGHTS];  // offset 2592, size 16 * 4 = 64
    // xyz = light position, w = far plane (radius)
    int pointLightShadowCount;                  // offset 2656, size 4
    int _shadowPad1;                            // offset 2660, padding
    int _shadowPad2;                            // offset 2664, padding
    int _shadowPad3;                            // offset 2668, padding
    glm::ivec4 pointLightShadowIndices;         // offset 2672, size 16 (x,y,z,w = indices into pointLights)

    // === Color Grading (32 bytes) ===
    glm::vec4 colorGrading;                     // offset 2688, size 16 (x=exposure, y=contrast, z=saturation, w=vibrance)
    glm::vec4 colorTemperature;                 // offset 2704, size 16 (x=temperature, y=tint, z=tonemapOperator, w=unused)

    // === Reflection Probes (80 bytes) ===
    glm::vec4 probePositions[4];                // offset 2720, size 64 (xyz=position, w=radius)
    glm::vec4 probeSettings;                    // offset 2784, size 16 (x=probeCount, y=globalSkyBlend, z/w=unused)

    // Total: 2800 bytes
};
static_assert(sizeof(GPUSceneData) == 2800, "GPUSceneData must be 2800 bytes for GPU alignment");

enum class ShaderOnlyMaterial : uint8_t {
    DEFAULT = 0,      // Full PBR primitive pipeline (default)
    UNLIT = 1,        // Simple unlit color (no lighting)
    PBR = 2,          // Same as DEFAULT (explicit PBR)
    NORMAL_DEBUG = 3, // Display normals as colors
    WIREFRAME = 4,    // Wireframe rendering
    // Internal types (not shown in UI)
    GRID = 10,
    EMISSIVE = 11,
    POINTLIGHT_VIS = 12
};



enum class MaterialPass :uint8_t {
    MainColor,
    Transparent,
    Other
};



struct MaterialPipeline {
    VkPipeline pipeline;
    VkPipelineLayout layout;

    std::string name;
};

struct MaterialInstance {
    MaterialPipeline* pipeline;
    VkDescriptorSet materialSet;
    MaterialPass passType;
};

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};


struct VertexInputDescription {
    std::vector<VkVertexInputBindingDescription> bindings;
    std::vector<VkVertexInputAttributeDescription> attributes;
};

// 🔽 2. SONRA Vertex struct'ı
struct Vertex {
    glm::vec3 position;
    float uv_x;
    glm::vec3 normal;
    float uv_y;
    glm::vec4 color;
    glm::vec4 tangent = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f); // xyz = tangent dir, w = handedness

    static VertexInputDescription get_vertex_description();
};



// holds the resources needed for a mesh
struct GPUMeshBuffers {

    AllocatedBuffer indexBuffer;
    AllocatedBuffer vertexBuffer;
    VkDeviceAddress vertexBufferAddress;

    uint32_t indexCount;
};





// push constants for our mesh object draws
//struct 
// 
// {
//    glm::mat4 worldMatrix;
//    VkDeviceAddress vertexBuffer;
//};

//struct GPUDrawPushConstants {
//    glm::mat4 worldMatrix;         // 64 byte
//    VkDeviceAddress vertexBuffer;  // 8 byte
//    glm::vec4 faceColors[6];       // 96 byte
//};

//struct GPUDrawPushConstants {
//    glm::mat4 worldMatrix;         // 64 byte
//    VkDeviceAddress vertexBuffer;  // 8 byte
//    glm::vec4 faceColors[6];       // 96 byte
//}; // Toplam: 168 byte

//struct GPUDrawPushConstants {
//    glm::mat4 worldMatrix;         // 64 byte
//    VkDeviceAddress vertexBuffer;  // 8 byte
//    // faceColors kaldırıldı
//}; // Toplam: 72 byte

struct GPUDrawPushConstants {
    glm::mat4 worldMatrix;         // 64 bytes, offset 0-63
    VkDeviceAddress vertexBuffer;  // 8 bytes, offset 64-71
    float outlineScale;            // 4 bytes, offset 72-75
    float padding[5];              // 20 bytes, offset 76-95 (for vec4 16-byte alignment)
    glm::vec4 baseColor;           // 16 bytes, offset 96-111
};  // Total: 112 bytes - matches GLSL std430 layout
static_assert(sizeof(GPUDrawPushConstants) == 112, "GPUDrawPushConstants must be 112 bytes for GPU alignment");

// =============================================================================
// GRID PUSH CONSTANTS - For dynamic infinite grid rendering
// Must match the push_constant block in grid.frag shader!
// =============================================================================
struct GridPushConstants {
    glm::mat4 worldMatrix;         // 64 bytes - Transform matrix

    // gridParams: x=cellSize, y=fadeDistance, z=lineWidth, w=opacity
    glm::vec4 gridParams;          // 16 bytes

    // gridParams2: x=dynamicLOD(0/1), y=showAxisColors(0/1), z=showSubdivisions(0/1), w=axisLineWidth
    glm::vec4 gridParams2;         // 16 bytes

    // gridParams3: x=lodBias, y=antiAliasing(0/1), z=minFadeAlpha, w=majorMultiplier
    glm::vec4 gridParams3;         // 16 bytes

    glm::vec4 minorColor;          // 16 bytes - rgb=minor line color, a=unused
    glm::vec4 majorColor;          // 16 bytes - rgb=major line color, a=unused
    glm::vec4 xAxisColor;          // 16 bytes - rgb=X axis color (red)
    glm::vec4 zAxisColor;          // 16 bytes - rgb=Z axis color (blue)
    // Total: 176 bytes
};

// Grid rendering settings (CPU-side configuration)
struct GridSettings {
    // === Core Grid Settings ===
    float baseGridSize = 1.0f;           // Base grid cell size in world units
    float majorGridMultiplier = 10.0f;   // Major lines every N cells
    float lineWidth = 1.5f;              // Line thickness
    float fadeDistance = 1000.0f;        // Distance at which grid fades out
    float gridOpacity = 0.7f;            // Overall grid opacity

    // === LOD Settings ===
    bool dynamicLOD = true;              // Enable LOD based on camera distance
    float lodBias = 0.0f;                // LOD level bias (-2 to +2)

    // === Axis Settings ===
    bool showAxisColors = true;          // Show X=Red, Z=Blue axis lines
    float axisLineWidth = 3.0f;          // Axis line thickness multiplier
    glm::vec3 xAxisColor = glm::vec3(0.9f, 0.2f, 0.2f);   // X axis color (Red)
    glm::vec3 zAxisColor = glm::vec3(0.2f, 0.4f, 0.9f);   // Z axis color (Blue)
    glm::vec3 originColor = glm::vec3(0.2f, 0.9f, 0.2f);  // Origin color (Green)

    // === Grid Line Colors ===
    glm::vec3 minorLineColor = glm::vec3(0.25f, 0.25f, 0.25f);
    glm::vec3 majorLineColor = glm::vec3(0.45f, 0.45f, 0.45f);

    // === Advanced Settings ===
    bool infiniteGrid = true;            // Grid follows camera position
    bool fadeFromCamera = true;          // true=fade from camera, false=fade from origin
    bool showSubdivisions = true;        // Show minor grid lines
    bool antiAliasing = true;            // Enable line anti-aliasing
    float gridHeight = 0.0f;             // Y position of grid plane
    float minFadeAlpha = 0.0f;           // Minimum alpha at fade distance

    // === Chunked Grid System (Performance) ===
    bool useChunkedGrid = false;         // Use chunk-based rendering for better FPS
    float chunkSize = 50.0f;             // Size of each grid chunk
    float chunkRenderDistance = 300.0f;  // Max distance to render chunks

    // === Presets ===
    int currentPreset = 0;               // 0=Default, 1=Blender, 2=Unity, 3=Unreal, 4=CAD
};



// Total = 64 + 8 + 96 = 168 byte + padding olabilir = 176
//struct GPUDrawPushConstants {
//    glm::mat4 worldMatrix;
//    uint32_t vertexBuffer;
//};





// =============================================================================
// BOUNDS - AABB for culling and picking
// =============================================================================

struct Bounds {
    glm::vec3 origin;
    float sphereRadius;
    glm::vec3 extents;
};

// Forward declarations for rendering
struct MeshNode;

// =============================================================================
// RENDER OBJECT - Drawable surface with material
// =============================================================================

struct RenderObject {
    uint32_t indexCount;
    uint32_t firstIndex;
    VkBuffer indexBuffer;

    MaterialInstance* material;
    Bounds bounds;
    glm::mat4 transform;

    VkBuffer vertexBuffer;
    VkDeviceAddress vertexBufferAddress;

    std::string name;

    MeshNode* nodePointer = nullptr;
};

// =============================================================================
// DRAW CONTEXT - Rendering command buffer
// =============================================================================

struct DrawContext {
    std::vector<RenderObject> OpaqueSurfaces;
    std::vector<RenderObject> TransparentSurfaces;

    glm::mat4 viewproj;
};

// =============================================================================
// RENDERABLE INTERFACE
// =============================================================================

class IRenderable {
    virtual void Draw(const glm::mat4& topMatrix, DrawContext& ctx) = 0;
};

// implementation of a drawable scene node.
// the scene node can hold children and will also keep a transform to propagate
// to them
//struct Node : public IRenderable {
//
//    // parent pointer must be a weak pointer to avoid circular dependencies
//    std::weak_ptr<Node> parent;
//    std::vector<std::shared_ptr<Node>> children;
//
//    glm::mat4 localTransform;
//    glm::mat4 worldTransform;
//
//    void refreshTransform(const glm::mat4& parentMatrix)
//    {
//        worldTransform = parentMatrix * localTransform;
//        for (auto c : children) {
//            c->refreshTransform(worldTransform);
//        }
//    }
//
//    virtual void Draw(const glm::mat4& topMatrix, DrawContext& ctx)
//    {
//        // draw children
//        for (auto& c : children) {
//            c->Draw(topMatrix, ctx);
//        }
//    }
//};





struct Node : public IRenderable {
    std::weak_ptr<Node> parent;
    std::vector<std::shared_ptr<Node>> children;

    glm::mat4 localTransform;
    glm::mat4 worldTransform;

    void refreshTransform(const glm::mat4& parentMatrix)
    {
        worldTransform = parentMatrix * localTransform;
        for (auto c : children) {
            c->refreshTransform(worldTransform);
        }
    }

    virtual void Draw(const glm::mat4& topMatrix, DrawContext& ctx)
    {
        for (auto& c : children) {
            c->Draw(topMatrix, ctx);
        }
    }
};





//
#define VK_CHECK(x) \
    do { \
        VkResult err = x; \
        if (err) { \
            std::cerr << "Detected Vulkan error: " << err << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            abort(); \
        } \
    } while (0)
//< node_types

// =============================================================================
// TEXTURE CACHE - Shared texture management types
// =============================================================================

struct TextureID {
    uint32_t Index;
};

constexpr TextureID INVALID_TEXTURE_ID = { UINT32_MAX };

struct TextureCache {
    std::vector<VkDescriptorImageInfo> Cache;
    std::unordered_map<std::string, TextureID> NameMap;

    TextureID AddTexture(const VkDescriptorImageInfo& info, const std::string& name = "") {
        TextureID id{ static_cast<uint32_t>(Cache.size()) };
        Cache.push_back(info);
        if (!name.empty()) {
            NameMap[name] = id;
        }
        return id;
    }

    TextureID AddTexture(VkImageView view, VkSampler sampler, const std::string& name = "") {
        VkDescriptorImageInfo info{};
        info.sampler = sampler;
        info.imageView = view;
        info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        return AddTexture(info, name);
    }

    // Replace all entries referencing any of the given imageViews with a fallback.
    // This prevents use-after-free when a scene is unloaded and its images destroyed.
    void InvalidateImageViews(const std::vector<VkImageView>& deadViews,
                              VkImageView fallbackView, VkSampler fallbackSampler) {
        for (auto& entry : Cache) {
            for (auto dv : deadViews) {
                if (entry.imageView == dv) {
                    entry.imageView = fallbackView;
                    entry.sampler = fallbackSampler;
                    break;
                }
            }
        }
        // Also remove NameMap entries pointing to invalidated textures
        for (auto it = NameMap.begin(); it != NameMap.end(); ) {
            uint32_t idx = it->second.Index;
            if (idx < Cache.size() && Cache[idx].imageView == fallbackView) {
                it = NameMap.erase(it);
            } else {
                ++it;
            }
        }
    }
};

// =============================================================================
// DELETION QUEUE - RAII-style resource cleanup
// =============================================================================

struct DeletionQueue {
    std::deque<std::function<void()>> deletors;

    void push_function(std::function<void()>&& function) {
        deletors.push_back(function);
    }

    void flush() {
        for (auto it = deletors.rbegin(); it != deletors.rend(); it++) {
            (*it)();
        }
        deletors.clear();
    }
};

//> intro
//#define VK_CHECK(x)                                                     \
//    do {                                                                \
//        VkResult err = x;                                               \
//        if (err) {                                                      \
//             fmt::print("Detected Vulkan error: {}", string_VkResult(err)); \
//            abort();                                                    \
//        }                                                               \
//    } while (0)
//< intro