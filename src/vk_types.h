// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

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
#include <vulkan/vk_enum_string_helper.h>
#include <vk_mem_alloc.h>

#include <fmt/core.h>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>


// we will add our main reusable types here
struct AllocatedImage {
    VkImage image;
    VkImageView imageView;
    VmaAllocation allocation;
    VkExtent3D imageExtent;
    VkFormat imageFormat;


};

//struct AllocatedBuffer {
//    VkBuffer buffer;
//    VmaAllocation allocation;
//    VmaAllocationInfo info;
//};

struct AllocatedBuffer {
    VkBuffer buffer;
    VmaAllocation allocation;
    VmaAllocationInfo info;
    size_t size = 0; // ✅ buffer boyutu artık burada tutulur
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

// GPU-aligned point light structure (32 bytes, matches std140 layout)
// Layout: [position.xyz, radius] [color.xyz, intensity] = 2 x vec4 = 32 bytes
struct alignas(16) PointLight {
    glm::vec3 position;     // offset 0,  size 12
    float radius;           // offset 12, size 4  (packs with vec3)

    glm::vec3 color;        // offset 16, size 12
    float intensity;        // offset 28, size 4  (packs with vec3)
    // Total: 32 bytes per light
};
static_assert(sizeof(PointLight) == 32, "PointLight must be 32 bytes for GPU alignment");

// GPU Scene Data - Uniform buffer structure (std140 layout)
// Contains view/projection matrices, lighting info, and point light array
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
    PointLight pointLights[MAX_POINT_LIGHTS];   // offset 256, size 64 * 32 = 2048

    // === Point Light Count (16 bytes for alignment) ===
    int pointLightCount;                        // offset 2304, size 4
    float _pad0;                                // offset 2308, size 4
    float _pad1;                                // offset 2312, size 4
    float _pad2;                                // offset 2316, size 4
    // Total: 2320 bytes
};
static_assert(sizeof(GPUSceneData) == 2320, "GPUSceneData must be 2320 bytes for GPU alignment");

enum class ShaderOnlyMaterial : uint8_t {
    DEFAULT,
    GRID,
    EMISSIVE,
    POINTLIGHT_VIS
    // vs...
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





struct DrawContext;

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