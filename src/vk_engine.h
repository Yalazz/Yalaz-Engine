// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once
#include <vk_types.h>
#include <vector>
#include "vk_mem_alloc.h"
#include <deque>
#include <functional>
#include "vk_descriptors.h"
#include <glm/glm.hpp>
#include <vk_loader.h>
#include <camera.h>
#include <vk_pipelines.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "vk_types.h" // GPUMeshBuffers vs için

struct MeshAsset;
namespace fastgltf {
    struct Mesh;
}

// TextureID, TextureCache, DeletionQueue are now defined in vk_types.h

struct PathTracePushConstants {
    glm::mat4 invView;          // 64 bytes
    glm::mat4 invProj;          // 64 bytes
    glm::vec4 cameraPos;        // 16 bytes (xyz = pos, w = unused) - aligned for GPU
    glm::vec4 sunlightDir;      // 16 bytes (xyz = direction, w = intensity)
    glm::vec4 sunlightColor;    // 16 bytes
    glm::vec4 ambientColor;     // 16 bytes (rgb = color, a = intensity)
    uint32_t frameIndex;        // 4 bytes
    uint32_t padding[3];        // 12 bytes for alignment
};  // Total: 208 bytes




struct ComputePushConstants {
    glm::vec4 data1;
    glm::vec4 data2;
    glm::vec4 data3;
    glm::vec4 data4;
};
struct ComputeEffect {
    const char* name;             // Shader adı
    VkPipeline pipeline;          // Compute pipeline
    VkPipelineLayout layout;      // Pipeline layout
    ComputePushConstants data;    // Push Constants verisi
};

// RenderObject is now defined in vk_types.h

//< meshnode

//struct StaticMeshData {
//    GPUMeshBuffers mesh;
//    glm::vec3 position;
//    glm::vec3 scale;
//};
//struct StaticMeshData {
//    GPUMeshBuffers mesh;
//    glm::vec3 position{ 0.0f };
//    glm::vec3 rotation{ 0.0f }; // Euler açıları (radyan)
//    glm::vec3 scale{ 1.0f };
//
//   /* glm::mat4 get_transform() const {
//        glm::mat4 t = glm::translate(glm::mat4(1.0f), position);
//        t = glm::rotate(t, rotation.x, glm::vec3(1, 0, 0));
//        t = glm::rotate(t, rotation.y, glm::vec3(0, 1, 0));
//        t = glm::rotate(t, rotation.z, glm::vec3(0, 0, 1));
//        t = glm::scale(t, scale);
//        return t;
//    }*/
//};
// === HEADER ===
//struct StaticMeshData {
//    GPUMeshBuffers mesh;
//    glm::vec3 position = glm::vec3(0.0f);
//    glm::vec3 rotation = glm::vec3(0.0f);
//    glm::vec3 scale = glm::vec3(1.0f);
//    glm::vec4 color = glm::vec4(1.0f);
//
//    glm::mat4 get_transform() const {
//        glm::mat4 t = glm::translate(glm::mat4(1.0f), position);
//        t = glm::rotate(t, rotation.x, glm::vec3(1, 0, 0));
//        t = glm::rotate(t, rotation.y, glm::vec3(0, 1, 0));
//        t = glm::rotate(t, rotation.z, glm::vec3(0, 0, 1));
//        t = glm::scale(t, scale);
//        return t;
//    }
//};
//struct StaticMeshData {
//    GPUMeshBuffers mesh;
//    glm::vec3 position = glm::vec3(0.0f);
//    glm::vec3 rotation = glm::vec3(0.0f);
//    glm::vec3 scale = glm::vec3(1.0f);
//    glm::vec4 color = glm::vec4(1.0f);
//    glm::vec4 faceColors[6] = { // Front, Right, Back, Left, Top, Bottom
//        glm::vec4(1, 0, 0, 1),
//        glm::vec4(0, 1, 0, 1),
//        glm::vec4(0, 0, 1, 1),
//        glm::vec4(1, 1, 0, 1),
//        glm::vec4(1, 0, 1, 1),
//        glm::vec4(0, 1, 1, 1)
//    };
//
//    glm::mat4 get_transform() const {
//        glm::mat4 t = glm::translate(glm::mat4(1.0f), position);
//        t = glm::rotate(t, rotation.x, glm::vec3(1, 0, 0));
//        t = glm::rotate(t, rotation.y, glm::vec3(0, 1, 0));
//        t = glm::rotate(t, rotation.z, glm::vec3(0, 0, 1));
//        t = glm::scale(t, scale);
//        return t;
//    }
//};



struct FrameData {
    VkSemaphore _swapchainSemaphore, _renderSemaphore;
    VkFence _renderFence;

    DescriptorAllocatorGrowable _frameDescriptors;
    DeletionQueue _deletionQueue;

    VkCommandPool _commandPool;
    VkCommandBuffer _mainCommandBuffer;
    AllocatedBuffer sceneDataBuffer;

    VkDescriptorSet drawImageDescriptorSet;

};

constexpr unsigned int FRAME_OVERLAP = 3;  // Must match swapchain image count for proper semaphore handling





struct MeshNode : public Node {

    std::shared_ptr<MeshAsset> mesh;

    virtual void Draw(const glm::mat4& topMatrix, DrawContext& ctx) override;
};



// vk_engine.h
struct MeshPushConstants {
    glm::mat4 worldMatrix;
    VkDeviceAddress vertexBuffer;
};



// DrawContext is now defined in vk_types.h

struct EngineStats {
    float frametime;
    int triangle_count;
    int drawcall_count;
    float scene_update_time;
    float mesh_draw_time;
    int visible_count;       // Görünen nesne sayısı
    int shader_count;        // Shader pipeline sayısı
    std::vector<std::string> visibleObjects;  //  Görünen nesnelerin adları
    std::vector<std::string> shaderNames;     //  Shader pipeline'larının adları

};

//struct GLTFMetallic_Roughness {
//    MaterialPipeline opaquePipeline;
//    MaterialPipeline transparentPipeline;
//
//    VkDescriptorSetLayout materialLayout;
//
//    struct MaterialConstants {
//        glm::vec4 colorFactors;
//        glm::vec4 metal_rough_factors;
//        //padding, we need it anyway for uniform buffers
//        glm::vec4 extra[14];
//    };
//
//    struct MaterialResources {
//        AllocatedImage colorImage;
//        VkSampler colorSampler;
//        AllocatedImage metalRoughImage;
//        VkSampler metalRoughSampler;
//        VkBuffer dataBuffer;
//        uint32_t dataBufferOffset;
//    };
//
//    DescriptorWriter writer;
//
//    void build_pipelines(VulkanEngine* engine);
//    void clear_resources(VkDevice device);
//
//   MaterialInstance write_material(VkDevice device, MaterialPass pass, const MaterialResources& resources, DescriptorAllocatorGrowable& descriptorAllocator);
//};
enum class PrimitiveType {
    Cube,
    Sphere,
    Capsule,
    Cylinder,
    Plane,
    Cone,
    Torus,
    Triangle
};

//struct StaticMeshData {
//    GPUMeshBuffers mesh;
//    glm::vec3 position = glm::vec3(0.0f);
//    glm::vec3 rotation = glm::vec3(0.0f);
//    glm::vec3 scale = glm::vec3(1.0f);
//    glm::vec4 faceColors[6] = {
//        glm::vec4(1, 0, 0, 1), // front
//        glm::vec4(0, 1, 0, 1), // right
//        glm::vec4(0, 0, 1, 1), // back
//        glm::vec4(1, 1, 0, 1), // left
//        glm::vec4(1, 0, 1, 1), // top
//        glm::vec4(0, 1, 1, 1), // bottom
//    };
//    PrimitiveType type = PrimitiveType::Cube;
//
//
//
//    glm::mat4 get_transform() const {
//        glm::mat4 t = glm::translate(glm::mat4(1.0f), position);
//        t = glm::rotate(t, rotation.x, glm::vec3(1, 0, 0));
//        t = glm::rotate(t, rotation.y, glm::vec3(0, 1, 0));
//        t = glm::rotate(t, rotation.z, glm::vec3(0, 0, 1));
//        t = glm::scale(t, scale);
//        return t;
//    }
//};

//struct StaticMeshData {
//    GPUMeshBuffers mesh;
//    glm::vec3 position = glm::vec3(0.0f);
//    glm::vec3 rotation = glm::vec3(0.0f);
//    glm::vec3 scale = glm::vec3(1.0f);
//    glm::vec4 faceColors[6] = {
//        glm::vec4(1, 0, 0, 1),
//        glm::vec4(0, 1, 0, 1),
//        glm::vec4(0, 0, 1, 1),
//        glm::vec4(1, 1, 0, 1),
//        glm::vec4(1, 0, 1, 1),
//        glm::vec4(0, 1, 1, 1),
//    };
//
//    PrimitiveType type = PrimitiveType::Cube;
//
//    // 🔽 Bunları EKLE
//    ShaderOnlyMaterial materialType = ShaderOnlyMaterial::DEFAULT;
//    MaterialPass passType = MaterialPass::MainColor;
//
//    glm::mat4 get_transform() const {
//        glm::mat4 t = glm::translate(glm::mat4(1.0f), position);
//        t = glm::rotate(t, rotation.x, glm::vec3(1, 0, 0));
//        t = glm::rotate(t, rotation.y, glm::vec3(0, 1, 0));
//        t = glm::rotate(t, rotation.z, glm::vec3(0, 0, 1));
//        t = glm::scale(t, scale);
//        return t;
//    }
//};


//struct StaticMeshData {
//    GPUMeshBuffers mesh;
//    glm::vec3 position = glm::vec3(0.0f);
//    glm::vec3 rotation = glm::vec3(0.0f);
//    glm::vec3 scale = glm::vec3(1.0f);
//
//    // Artık kullanılmıyor (shader tarafında desteklenmiyor)
//    glm::vec4 faceColors[6] = {
//        glm::vec4(1, 0, 0, 1),
//        glm::vec4(0, 1, 0, 1),
//        glm::vec4(0, 0, 1, 1),
//        glm::vec4(1, 1, 0, 1),
//        glm::vec4(1, 0, 1, 1),
//        glm::vec4(0, 1, 1, 1),
//    };
//
//    PrimitiveType type = PrimitiveType::Cube;
//
//    ShaderOnlyMaterial materialType = ShaderOnlyMaterial::DEFAULT;
//    MaterialPass passType = MaterialPass::MainColor;
//
//    glm::vec4 color = glm::vec4(1.0f); // ✅ Dinamik baseColor gönderimi için eklendi
//
//    glm::mat4 get_transform() const {
//        glm::mat4 t = glm::translate(glm::mat4(1.0f), position);
//        t = glm::rotate(t, rotation.x, glm::vec3(1, 0, 0));
//        t = glm::rotate(t, rotation.y, glm::vec3(0, 1, 0));
//        t = glm::rotate(t, rotation.z, glm::vec3(0, 0, 1));
//        t = glm::scale(t, scale);
//        return t;
//    }
//};
// Push constants for primitive rendering - MUST match shader
// Updated to support PBR material properties
struct PrimitivePushConstants {
    glm::mat4 worldMatrix;      // 64 bytes (offset 0)
    glm::vec4 mainColor;        // 16 bytes (offset 64) - RGBA base color
    glm::vec4 faceColors[6];    // 96 bytes (offset 80) - Per-face colors
    glm::vec4 pbrParams;        // 16 bytes (offset 176) - x=metallic, y=roughness, z=ao, w=unused
    glm::vec4 emission;         // 16 bytes (offset 192) - xyz=emission color, w=emission strength
    int useFaceColors;          // 4 bytes (offset 208)
    int padding[3];             // 12 bytes (offset 212) - alignment padding
};  // Total: 224 bytes (within 256 byte limit)

struct StaticMeshData {
    GPUMeshBuffers mesh;
    std::string name;                           // Named primitive for UI
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);

    glm::vec4 mainColor = glm::vec4(1.0f);      // Overall tint color
    glm::vec4 faceColors[6] = {                 // Per-face colors
        glm::vec4(1.0f, 0.3f, 0.3f, 1.0f),      // Front (+Z) - Red
        glm::vec4(0.3f, 1.0f, 0.3f, 1.0f),      // Back (-Z) - Green
        glm::vec4(0.3f, 0.3f, 1.0f, 1.0f),      // Right (+X) - Blue
        glm::vec4(1.0f, 1.0f, 0.3f, 1.0f),      // Left (-X) - Yellow
        glm::vec4(1.0f, 0.3f, 1.0f, 1.0f),      // Top (+Y) - Magenta
        glm::vec4(0.3f, 1.0f, 1.0f, 1.0f),      // Bottom (-Y) - Cyan
    };
    bool useFaceColors = false;                 // Toggle for face coloring
    bool visible = true;                        // Visibility toggle
    bool selected = false;                      // Selection state for UI/gizmo

    // PBR material properties
    float metallic = 0.0f;                      // Metallic factor (0-1)
    float roughness = 0.5f;                     // Roughness factor (0-1)
    glm::vec3 emission = glm::vec3(0.0f);       // Emission color * strength

    PrimitiveType type = PrimitiveType::Cube;
    ShaderOnlyMaterial materialType = ShaderOnlyMaterial::DEFAULT;
    MaterialPass passType = MaterialPass::MainColor;

    glm::mat4 get_transform() const {
        glm::mat4 t = glm::translate(glm::mat4(1.0f), position);
        t = glm::rotate(t, rotation.x, glm::vec3(1, 0, 0));
        t = glm::rotate(t, rotation.y, glm::vec3(0, 1, 0));
        t = glm::rotate(t, rotation.z, glm::vec3(0, 0, 1));
        t = glm::scale(t, scale);
        return t;
    }

    // Get push constants for this primitive
    PrimitivePushConstants get_push_constants() const {
        PrimitivePushConstants pc{};
        pc.worldMatrix = get_transform();
        pc.mainColor = mainColor;
        for (int i = 0; i < 6; ++i) {
            pc.faceColors[i] = faceColors[i];
        }
        // PBR parameters: metallic, roughness, AO (fixed at 1.0), unused
        pc.pbrParams = glm::vec4(metallic, roughness, 1.0f, 0.0f);
        // Emission: RGB color with strength in alpha
        float emissionStrength = glm::length(emission);
        glm::vec3 emissionColor = emissionStrength > 0.001f ? emission / emissionStrength : glm::vec3(0.0f);
        pc.emission = glm::vec4(emissionColor, emissionStrength);
        pc.useFaceColors = useFaceColors ? 1 : 0;
        pc.padding[0] = pc.padding[1] = pc.padding[2] = 0;
        return pc;
    }
};





// =============================================================================
// ENGINE SUBSYSTEMS - Animation, Physics, Plugin, Shader Systems
// =============================================================================

// Animation System Structures
struct AnimationKeyframeData {
    float time = 0.0f;
    glm::vec4 value = glm::vec4(0.0f);
    int interpolation = 1;  // 0=Step, 1=Linear, 2=Cubic
};

struct AnimationTrackData {
    std::string targetNode;
    std::string property;  // "translation", "rotation", "scale"
    std::vector<AnimationKeyframeData> keyframes;
};

struct AnimationClipData {
    std::string name;
    float duration = 0.0f;
    float currentTime = 0.0f;
    bool isPlaying = false;
    bool loop = true;
    float speed = 1.0f;
    std::vector<AnimationTrackData> tracks;
};

struct SkeletonBoneData {
    std::string name;
    int parentIndex = -1;
    glm::vec3 localPosition = glm::vec3(0.0f);
    glm::quat localRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 localScale = glm::vec3(1.0f);
    glm::mat4 inverseBindMatrix = glm::mat4(1.0f);
};

struct SkeletonData {
    std::string name;
    std::vector<SkeletonBoneData> bones;
};

// Physics System Structures
struct PhysicsBodyData {
    std::string name;
    int type = 0;  // 0=Static, 1=Dynamic, 2=Kinematic
    glm::vec3 position = glm::vec3(0.0f);
    glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 velocity = glm::vec3(0.0f);
    glm::vec3 angularVelocity = glm::vec3(0.0f);
    float mass = 1.0f;
    float friction = 0.5f;
    float restitution = 0.3f;
    bool isAwake = true;
    int colliderType = 0;  // 0=Box, 1=Sphere, 2=Capsule, 3=Mesh
    glm::vec3 colliderSize = glm::vec3(1.0f);
};

struct PhysicsConstraintData {
    std::string name;
    int bodyA = -1;
    int bodyB = -1;
    int type = 0;  // 0=Point, 1=Hinge, 2=Slider, 3=6DOF
    glm::vec3 pivotA = glm::vec3(0.0f);
    glm::vec3 pivotB = glm::vec3(0.0f);
};

struct PhysicsWorldSettings {
    glm::vec3 gravity = glm::vec3(0.0f, -9.81f, 0.0f);
    float timeStep = 1.0f / 60.0f;
    int maxSubSteps = 4;
    bool debugDraw = false;
    bool paused = false;
};

// Plugin/Subsystem Structures
enum class SubsystemState {
    Unloaded,
    Loading,
    Loaded,
    Active,
    Error
};

struct SubsystemInfo {
    std::string id;
    std::string name;
    std::string version;
    SubsystemState state = SubsystemState::Loaded;
    float loadTimeMs = 0.0f;
    size_t memoryUsage = 0;
    bool isCore = false;
};

// Shader System Structures
struct ShaderPipelineInfo {
    std::string name;
    std::string vertPath;
    std::string fragPath;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout layout = VK_NULL_HANDLE;
    bool isValid = true;
    float compileTimeMs = 0.0f;
    std::string errorLog;
    int uniformCount = 0;
    int textureCount = 0;
};

struct ShaderUniformInfo {
    std::string name;
    std::string type;
    int set = 0;
    int binding = 0;
    size_t size = 0;
};

// =============================================================================

struct GLTFMetallic_Roughness {
    MaterialPipeline opaquePipeline;
    MaterialPipeline transparentPipeline;

    VkDescriptorSetLayout materialLayout;

    //struct MaterialConstants {
    //    glm::vec4 colorFactors;
    //    glm::vec4 metal_rough_factors;
    //    //padding, we need it anyway for uniform buffers
    //    glm::vec4 extra[14];
    //};
    //struct MaterialConstants {
    //    glm::vec4 colorFactors;
    //    glm::vec4 metal_rough_factors;
    //    uint32_t colorTexID;
    //    uint32_t metalRoughTexID;
    //    uint32_t pad1;
    //    uint32_t pad2;
    //    glm::vec4 extra[13];
    //};
    struct MaterialConstants {
        glm::vec4 colorFactors;          // baseColor RGBA
        glm::vec4 metal_rough_factors;   // x = metallic, y = roughness, z,w = boş
        uint32_t colorTexID;             // TextureCache'den alınan ID
        uint32_t metalRoughTexID;        // TextureCache'den alınan ID
        uint32_t pad1;               // Uniform buffer alignment için padding
        uint32_t pad2;
        glm::vec4 extra[13];             // Genişletilebilir alan (parallax, emissive, occlusion vs için boş yer)
    };



    struct MaterialResources {
        AllocatedImage colorImage;
        VkSampler colorSampler;
        AllocatedImage metalRoughImage;
        VkSampler metalRoughSampler;
        VkBuffer dataBuffer;
        uint32_t dataBufferOffset;
    };

    DescriptorWriter writer;

    void build_pipelines(VulkanEngine* engine);
    void clear_resources(VkDevice device);

    MaterialInstance write_material(VkDevice device, MaterialPass pass, const MaterialResources& resources, DescriptorAllocatorGrowable& descriptorAllocator);
};


class VulkanEngine {
public:
    // =========================================================================
    // ANIMATION SYSTEM
    // =========================================================================
    std::vector<AnimationClipData> animationClips;
    std::vector<SkeletonData> skeletons;
    int activeAnimationIndex = -1;
    int activeSkeletonIndex = -1;

    void updateAnimations(float deltaTime);
    void playAnimation(int index);
    void stopAnimation(int index);
    void addAnimationClip(const AnimationClipData& clip);

    // =========================================================================
    // PHYSICS SYSTEM
    // =========================================================================
    std::vector<PhysicsBodyData> physicsBodies;
    std::vector<PhysicsConstraintData> physicsConstraints;
    PhysicsWorldSettings physicsSettings;
    bool physicsEnabled = false;

    void updatePhysics(float deltaTime);
    void addPhysicsBody(const PhysicsBodyData& body);
    void removePhysicsBody(int index);
    void setPhysicsPaused(bool paused);

    // =========================================================================
    // PLUGIN/SUBSYSTEM SYSTEM
    // =========================================================================
    std::vector<SubsystemInfo> subsystems;

    void initSubsystems();
    void registerSubsystem(const SubsystemInfo& info);
    SubsystemInfo* getSubsystem(const std::string& id);

    // =========================================================================
    // SHADER SYSTEM
    // =========================================================================
    std::vector<ShaderPipelineInfo> shaderPipelines;
    std::vector<ShaderUniformInfo> shaderUniforms;

    void registerShaderPipeline(const ShaderPipelineInfo& info);
    void recompileShader(int index);
    void recompileAllShaders();

    // =========================================================================
    // SNAP SETTINGS (shared across all views)
    // =========================================================================
    bool snapEnabled = false;
    float snapPositionValue = 1.0f;
    bool snapRotationEnabled = false;
    float snapRotationAngle = 15.0f;
    bool snapScaleEnabled = false;
    float snapScaleValue = 0.1f;

    // =========================================================================
    // EXISTING SYSTEMS
    // =========================================================================
    std::vector<PointLight> scenePointLights;         // Sahneye konan ışık objeleri
    AllocatedBuffer pointLightBuffer;                // GPU'ya gönderilecek buffer
    void sync_point_light_spheres();
    enum class ViewMode {
        Solid = 0,             // Flat color, no lighting - fastest
        Shaded = 1,            // Hemisphere + N·L studio lighting
        MaterialPreview = 2,   // IBL-based material preview
        Rendered = 3,          // Full PBR with scene lights
        Wireframe = 4,         // Edge visualization
        Normals = 5,           // World-space normals as RGB
        UVChecker = 6,         // UV checker pattern debug
        PathTraced = 7         // Real-time path tracing (compute shader)
    };
    bool _showGrid = true;
    bool _showOutline = true;
    GridSettings _gridSettings;  // Dynamic grid configuration
    VkPipeline _wireframePipeline = VK_NULL_HANDLE;
    VkPipelineLayout _wireframePipelineLayout = VK_NULL_HANDLE;
    void init_wireframe_pipeline();

    // View mode pipelines
    VkPipeline _solidPipeline = VK_NULL_HANDLE;
    VkPipelineLayout _solidPipelineLayout = VK_NULL_HANDLE;
    void init_solid_pipeline();

    VkPipeline _shadedPipeline = VK_NULL_HANDLE;
    VkPipelineLayout _shadedPipelineLayout = VK_NULL_HANDLE;
    void init_shaded_pipeline();

    VkPipeline _normalsPipeline = VK_NULL_HANDLE;
    VkPipelineLayout _normalsPipelineLayout = VK_NULL_HANDLE;
    void init_normals_pipeline();

    VkPipeline _uvCheckerPipeline = VK_NULL_HANDLE;
    VkPipelineLayout _uvCheckerPipelineLayout = VK_NULL_HANDLE;
    void init_uvchecker_pipeline();
    void init_outline_pipeline();
	/*void draw_outline(VkCommandBuffer cmd, VkDescriptorSet globalDescriptor, VkViewport viewport, VkRect2D scissor);*/
    //void draw_visible_objects_panel();
    void draw_outline(VkCommandBuffer cmd, const RenderObject& obj, VkDescriptorSet descriptor);
    std::string selectedObjectName;
    
    MeshNode* findNodeByName(const std::string& name);
    MeshNode* findNodeRecursive(std::shared_ptr<Node> node, const std::string& name);
    // Header dosyasında (vk_engine.h)
    MeshNode* raycast_scene_objects(const glm::vec3& rayOrigin, const glm::vec3& rayDir);
    // VulkanEngine sınıfına özel:
    std::vector<RenderObject> pickableRenderObjects;
    void init_pathtrace_present_pipeline();
    // Path Trace görüntüleme için pipeline ve layout
    VkPipelineLayout _pathTracePresentPipelineLayout = VK_NULL_HANDLE;
    VkPipeline _pathTracePresentPipeline = VK_NULL_HANDLE;
    void draw_present_pathtraced(VkCommandBuffer cmd);
    void draw_background_effect(VkCommandBuffer cmd);
    VkDescriptorSet _drawImageDescriptorSet;
    void allocate_draw_image_descriptor_set();

    VkPipeline _2dPipeline = VK_NULL_HANDLE;
    VkPipeline _gridPipeline = VK_NULL_HANDLE;
    VkPipeline _emissivePipeline = VK_NULL_HANDLE;

    // Primitive pipeline with face color support
    VkPipeline _primitivePipeline = VK_NULL_HANDLE;
    VkPipelineLayout _primitivePipelineLayout = VK_NULL_HANDLE;
    void init_primitive_pipeline();
    void draw_primitives(VkCommandBuffer cmd, VkDescriptorSet globalDescriptor);
    void draw_primitives_with_viewport(VkCommandBuffer cmd, VkDescriptorSet globalDescriptor, VkViewport viewport, VkRect2D scissor);
    void init_point_light_vis_pipeline();

    VkPipelineLayout _gridPipelineLayout = VK_NULL_HANDLE;
    VkPipelineLayout _emissivePipelineLayout = VK_NULL_HANDLE;

    VkPipeline _pointLightVisPipeline = VK_NULL_HANDLE;
    VkPipelineLayout _pointLightVisPipelineLayout = VK_NULL_HANDLE;





    VkPipeline _outlinePipeline = VK_NULL_HANDLE;
    VkPipelineLayout _outlinePipelineLayout = VK_NULL_HANDLE;
	//void draw_shader_only_static_shapes(VkCommandBuffer cmd, VkDescriptorSet globalDescriptor, VkViewport viewport, VkRect2D scissor);
    
    void draw_shader_only_static_shapes(VkCommandBuffer cmd, VkDescriptorSet globalDescriptor, VkViewport viewport, VkRect2D scissor);
    void init_default_meshes();
    ViewMode _currentViewMode = ViewMode::Shaded;
    // primitive mesh oluşturucular:
    GPUMeshBuffers generate_cube_mesh();
    GPUMeshBuffers generate_plane_mesh();
    GPUMeshBuffers generate_sphere_mesh(int resolution = 24, int rings = 16);
    GPUMeshBuffers generate_cylinder_mesh(int segments = 32);
    GPUMeshBuffers generate_cone_mesh();
    GPUMeshBuffers generate_capsule_mesh();
    GPUMeshBuffers generate_torus_mesh();
    GPUMeshBuffers generate_triangle_mesh();
    void draw_shaded(
        VkCommandBuffer cmd,
        VkDescriptorSet globalDescriptor,
        VkViewport viewport,
        VkRect2D scissor,
        const std::vector<uint32_t>& opaque_draws);

    void draw_solid(
        VkCommandBuffer cmd,
        VkDescriptorSet globalDescriptor,
        VkViewport viewport,
        VkRect2D scissor,
        const std::vector<uint32_t>& opaque_draws);

    void draw_normals(
        VkCommandBuffer cmd,
        VkDescriptorSet globalDescriptor,
        VkViewport viewport,
        VkRect2D scissor,
        const std::vector<uint32_t>& opaque_draws);

    void draw_uvchecker(
        VkCommandBuffer cmd,
        VkDescriptorSet globalDescriptor,
        VkViewport viewport,
        VkRect2D scissor,
        const std::vector<uint32_t>& opaque_draws);
    void draw_viewing(VkCommandBuffer cmd);
    // Static şekil listesi:
    std::vector<StaticMeshData> static_shapes;
    bool enableBackfaceCulling = true;
    void draw_primitive_spawner_imgui();
    void draw_pipeline_settings_imgui();
    void draw_static_mesh_imgui();
    void draw_scene_hierarchy_imgui();  // Unified panel combining spawner + list
    void draw_inspector_panel_imgui();  // Separate inspector panel
    glm::mat4 _materialCubeTransform;
    GPUMeshBuffers _materialCubeMesh;
    MaterialInstance _materialCubeMaterial;
    void init_material_cube();
    VkPipeline _2dPipelineDoubleSided = VK_NULL_HANDLE;
    VkPipeline _2dPipelineCulled = VK_NULL_HANDLE;
    
    void init_plane_pipeline();
    void init_2d_pipeline(bool enableBackfaceCulling);
    //void draw_outline(VkCommandBuffer cmd, const RenderObject& obj);
    //void init_grid_pipeline();
    //void draw_grid(VkCommandBuffer cmd);
    VkShaderModule load_shader_module(const char* filePath);
    VkPipeline gridPipeline = VK_NULL_HANDLE;
    std::unordered_map<PrimitiveType, GPUMeshBuffers> defaultMeshes;
    GPUMeshBuffers _cachedLightSphereMesh;
    bool _lightMeshCached = false;

    void sync_point_light_billboards();
    
    

    VkPipeline _2dPipeline_CullOn = VK_NULL_HANDLE;
    VkPipeline _2dPipeline_CullOff = VK_NULL_HANDLE;

    VkPipelineLayout _2dPipelineLayout = VK_NULL_HANDLE;


    void update_imgui();
    void draw_node_selector();
    void draw_node_gizmo();
    void draw_node_recursive_ui(std::shared_ptr<Node> node);

    MeshNode* selectedNode = nullptr;      // Selected GLTF node
    int selectedPrimitiveIndex = -1;       // Selected primitive shape index (-1 = none)
    int selectedLightIndex = -1;           // Selected light index (-1 = none)

    VkPipelineLayout gridPipelineLayout;
    VkRenderPass _renderPass;
    VkDevice _device;

    bool _isInitialized{ false };
    int _frameNumber{ 0 };
    bool stop_rendering{ false };
    bool resize_requested{ false };
    bool freeze_rendering{ false };
    float renderScale = 1.f;
    /*VkExtent2D _windowExtent{ 1700 , 900 };*/
    /*VkExtent2D _windowExtent{ 2560 , 1440 };*/
    /*VkExtent2D _windowExtent{ 1920 , 1080 };*/
    /*VkExtent2D _windowExtent{ 1920, 1080 };*/
    VkExtent2D _windowExtent{ 1700, 900 };
    //VkPipelineLayout outlinePipelineLayout;
    //VkPipeline outlinePipeline;
    VkDescriptorSet globalDescriptor = VK_NULL_HANDLE;

    // Sınıfın public veya protected bölümüne ekleyin:
    VkViewport get_letterbox_viewport() const;

    // Eğer yoksa, şu şekilde tanımlayın:


    VkDescriptorSet _2dDescriptorSet = VK_NULL_HANDLE;


    struct SDL_Window* _window{ nullptr };

    VkInstance _instance;
    VkDebugUtilsMessengerEXT _debug_messenger;
    VkPhysicalDevice _chosenGPU;
    
    AllocatedBuffer _defaultGLTFMaterialData;

    FrameData _frames[FRAME_OVERLAP];
   /* FrameData& 
   
   () { return _frames[_frameNumber % FRAME_OVERLAP]; };*/

    FrameData& get_current_frame();
    FrameData& get_last_frame();

	VkPipelineLayout _meshPipelineLayout;
	VkPipeline _meshPipeline;


    MaterialInstance defaultData;
    GLTFMetallic_Roughness metalRoughMaterial;

    DrawContext mainDrawContext;
    TextureCache texCache;


    std::unordered_map<std::string, std::shared_ptr<Node>> loadedNodes;

    GPUSceneData sceneData;
    VkDescriptorSetLayout _gpuSceneDataDescriptorLayout;

    VkDescriptorSetLayout _gltfMatDescriptorLayout;

	// Mesh için veri saklayacak buffer
	GPUMeshBuffers rectangle;
    DrawContext drawCommands;
    std::vector<std::shared_ptr<MeshAsset>> testMeshes;


    VkQueue _graphicsQueue;
    uint32_t _graphicsQueueFamily;

    VkSurfaceKHR _surface;
    VkSwapchainKHR _swapchain;
    VkFormat _swapchainImageFormat;
    VkExtent2D _swapchainExtent;
    VmaAllocator _allocator;

    VkExtent2D _drawExtent;

    Camera mainCamera;

    /*DescriptorAllocatorGrowable globalDescriptorAllocator;*/
    DescriptorAllocator globalDescriptorAllocator;

    VkPipeline _2dPipelineGrid = VK_NULL_HANDLE;

    VkPipeline _gradientPipeline;
    VkPipelineLayout _gradientPipelineLayout;

    std::vector<VkFramebuffer> _framebuffers;
    std::vector<VkImage> _swapchainImages;
    std::vector<VkImageView> _swapchainImageViews;



    VkDescriptorSet _drawImageDescriptors;
    VkDescriptorSetLayout _drawImageDescriptorLayout;
    VkDescriptorSetLayout _singleImageDescriptorLayout;
    DeletionQueue _mainDeletionQueue;

    VkFence _immFence;
    VkCommandBuffer _immCommandBuffer;
    VkCommandPool _immCommandPool;



    AllocatedImage _drawImage;
    AllocatedImage _depthImage;

    AllocatedImage _whiteImage;
    AllocatedImage _blackImage;
    AllocatedImage _greyImage;
    AllocatedImage _errorCheckerboardImage;

    VkSampler _defaultSamplerLinear;
    VkSampler _defaultSamplerNearest;

    EngineStats stats;
    std::vector<ComputeEffect> backgroundEffects;
    int currentBackgroundEffect{ 0 };

    GPUMeshBuffers uploadMesh(std::span<uint32_t> indices, std::span<Vertex> vertices);

    AllocatedBuffer create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);

    AllocatedImage create_image(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
    AllocatedImage create_image(void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
    


    VkDescriptorSet _pathTraceDescriptorSet = VK_NULL_HANDLE;
    VkPipeline _pathTracePipeline = VK_NULL_HANDLE;
    VkPipelineLayout _pathTracePipelineLayout = VK_NULL_HANDLE;
    // VulkanEngine class tanımına ekle
    bool lightMeshesAdded = false;
    void init_emissive_pipeline();

    
    // **Triangle Pipeline Yapısı**
    VkPipeline _trianglePipeline;
    VkPipelineLayout _trianglePipelineLayout;
    VkPipelineLayout _materialPreviewPipelineLayout;
    VkPipeline _materialPreviewPipeline;           
    void init_material_preview_pipeline();
    void draw_wireframe(VkCommandBuffer cmd, VkDescriptorSet globalDescriptor, VkViewport viewport, VkRect2D scissor, const std::vector<uint32_t>& opaque_draws);
    void draw_material_preview(VkCommandBuffer cmd, VkDescriptorSet globalDescriptor, VkViewport viewport, VkRect2D scissor, const std::vector<uint32_t>& opaque_draws);
    void draw_rendered(VkCommandBuffer cmd, VkDescriptorSet globalDescriptor, VkViewport viewport, VkRect2D scissor, const std::vector<uint32_t>& opaque_draws);

    void draw_rendered_pathtraced(VkCommandBuffer cmd);
	void init_pathtrace_pipeline();

    static VulkanEngine& Get();
    void destroy_buffer(const AllocatedBuffer& buffer);
    void draw_scene_light_imgui();
    void init_grid_pipeline();
    /*void draw_grid(VkCommandBuffer cmd);*/
    void draw_grid(VkCommandBuffer cmd, VkDescriptorSet globalDescriptor);
    GPUMeshBuffers gridMesh;
	void init_outline_wireframe_pipeline();
    void draw_wireframe_outline(VkCommandBuffer cmd, const RenderObject& obj, VkDescriptorSet descriptor, VkViewport viewport, VkRect2D scissor);
    void generate_grid_plane_mesh(int numX, int numZ, float spacing, std::vector<Vertex>& outVertices, std::vector<uint32_t>& outIndices);
    VkPipeline _wireframeOutlinePipeline = VK_NULL_HANDLE;
    VkPipelineLayout _wireframeOutlinePipelineLayout = VK_NULL_HANDLE;
    void select_object_under_mouse(float mouseX, float mouseY);
    void compute_ray_from_mouse(float mouseX, float mouseY, glm::vec3& outOrigin, glm::vec3& outDirection);
    void raycast_node_recursive(std::shared_ptr<Node> node, const glm::vec3& rayOrigin, const glm::vec3& rayDir, float& closestHit);

    void init_light_sphere();

    void immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function);

    std::unordered_map<std::string, std::shared_ptr<LoadedGLTF>> loadedScenes;
    std::unordered_map<std::string, std::string> sceneFilePaths; // name -> file path

    // Engine state save/load/reset
    void saveState(const std::string& filepath);
    void loadState(const std::string& filepath);
    void resetState();

    void destroy_image(const AllocatedImage& img);
    void init();
    void cleanup();
    void draw();
    void draw_main(VkCommandBuffer cmd);

    void draw_imgui(VkCommandBuffer cmd, VkImageView targetImageView);

    //void calculate_bounding_box(const std::shared_ptr<Node>& node, glm::vec3& minBounds, glm::vec3& maxBounds);
    void render_nodes();

    void draw_geometry(VkCommandBuffer cmd);
    void init_scene_data();

    void draw_background(VkCommandBuffer cmd);

    //void draw_geometry(VkCommandBuffer cmd);  // **BU UCGENI EKLEMEYE YARIYORDU UNUTMA**

    void create_material_constant_buffer(const GLTFMetallic_Roughness::MaterialConstants& data);
    void run();
    
    void update_scene();
    // void init_triangle_pipeline();

private:
private:

    void rebuild_swapchain();
    void init_vulkan();
    void init_swapchain();
    void create_swapchain(uint32_t width, uint32_t height);
    void destroy_swapchain();
	void resize_swapchain();
    void init_commands();
    void init_pipelines();
    void init_background_pipelines();
	void init_triangle_pipeline();
	void init_mesh_pipeline();
    void init_descriptors();
    void init_sync_structures();
    void init_renderables();
    void init_imgui();
    void init_default_data();
};