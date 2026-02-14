// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once
#include <vk_types.h>
#include <vector>
#include <array>
#include <memory>
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
#include "renderer/PostProcess.h"
#include "renderer/BloomPass.h"
#include "renderer/PathTracer.h"
#include "renderer/EnvironmentMap.h"
#include "geometry/PrimitiveType.h"

struct MeshAsset;
namespace fastgltf {
    struct Mesh;
}


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

struct FrameData {
    VkSemaphore _swapchainSemaphore, _renderSemaphore;
    VkFence _renderFence;

    DescriptorAllocatorGrowable _frameDescriptors;
    DeletionQueue _deletionQueue;

    VkCommandPool _commandPool;
    VkCommandBuffer _mainCommandBuffer;
    VkCommandPool _uiCommandPool = VK_NULL_HANDLE;
    VkCommandBuffer _uiCommandBuffer = VK_NULL_HANDLE;
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

// Push constants for primitive rendering - MUST match shader
// Updated to support PBR material properties
struct PrimitivePushConstants {
    glm::mat4 worldMatrix;      // 64 bytes (offset 0)
    glm::vec4 mainColor;        // 16 bytes (offset 64) - RGBA base color
    glm::vec4 faceColors[6];    // 96 bytes (offset 80) - Per-face colors
    glm::vec4 pbrParams;        // 16 bytes (offset 176) - x=metallic, y=roughness, z=ao, w=reflectionIntensity
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
    float reflectionIntensity = 0.0f;           // Cubemap reflection (0=none, 1=full)

    PrimitiveType type = PrimitiveType::Cube;
    ShaderOnlyMaterial materialType = ShaderOnlyMaterial::DEFAULT;
    MaterialPass passType = MaterialPass::MainColor;

    // Texture support - shared material with descriptor set
    std::shared_ptr<MaterialInstance> material = nullptr;

    // Texture paths for UI/serialization (empty = use defaults)
    std::string albedoTexturePath;
    std::string metalRoughTexturePath;
    std::string emissionTexturePath;
    std::string displacementTexturePath;
    float displacementScale = 0.0f;
    float displacementBias = 0.0f;

    // Helper to get material descriptor set (returns default if no custom material)
    VkDescriptorSet getMaterialDescriptorSet(class VulkanEngine* engine) const;

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
        // PBR parameters: metallic, roughness, AO (fixed at 1.0), reflectionIntensity
        pc.pbrParams = glm::vec4(metallic, roughness, 1.0f, reflectionIntensity);
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
// REFLECTION PROBE - Multi-probe reflection system
// =============================================================================
struct ReflectionProbe {
    glm::vec3 position = glm::vec3(0.0f);
    float radius = 50.0f;
    float skyBlendFactor = 0.3f;
    bool active = true;

    AllocatedImage cubemap;
    VkImageView cubemapView = VK_NULL_HANDLE;
    VkImageView faceViews[6] = {};
    bool needsUpdate = true;
};

// =============================================================================
// ENGINE SUBSYSTEMS - Animation, Physics, Plugin, Shader Systems
// =============================================================================

// Animation System Structures
struct AnimationKeyframeData {
    float time = 0.0f;
    glm::vec4 value = glm::vec4(0.0f);
    glm::vec4 inTangent = glm::vec4(0.0f);
    glm::vec4 outTangent = glm::vec4(0.0f);
    bool hasTangents = false;
    int interpolation = 1;  // 0=Step, 1=Linear, 2=Cubic
};

struct AnimationTrackData {
    std::string sourceScene;
    std::string targetNode;
    int targetNodeIndex = -1;
    int targetBoneIndex = -1;
    std::string property;  // "translation", "rotation", "scale"
    std::vector<AnimationKeyframeData> keyframes;
};

struct AnimationClipData {
    std::string name;
    std::string sourceScene;
    int skeletonIndex = -1;
    float duration = 0.0f;
    float currentTime = 0.0f;
    bool isPlaying = false;
    bool loop = true;
    bool pingPong = false;
    bool reverse = false;
    float speed = 1.0f;
    std::vector<AnimationTrackData> tracks;
};

struct SkeletonBoneData {
    std::string name;
    int nodeIndex = -1;
    int parentIndex = -1;
    glm::vec3 localPosition = glm::vec3(0.0f);
    glm::quat localRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 localScale = glm::vec3(1.0f);
    glm::mat4 inverseBindMatrix = glm::mat4(1.0f);
};

struct SkeletonData {
    std::string name;
    std::string sourceScene;
    std::vector<SkeletonBoneData> bones;
    // World transform of the mesh node that references this skin at bind time.
    // For FBX-converted files, this includes the axis conversion root rotation.
    // Applied in skinning matrix: skinMat = jointWorld * IBM * meshBindTransform
    // so that vertex data (potentially in FBX Z-up space) gets rotated correctly.
    glm::mat4 meshBindTransform = glm::mat4(1.0f);
};

struct AnimationGraphParameter {
    std::string name;
    float value = 0.0f;
    bool isBool = false;
};

struct AnimationGraphStateData {
    std::string name;
    int clipIndex = -1;
    bool isDefault = false;
    float positionX = 0.0f;
    float positionY = 0.0f;
};

struct AnimationGraphTransitionData {
    int fromState = -1;
    int toState = -1;
    std::string parameter = "speed";
    int comparison = 0; // 0: >, 1: <, 2: >=, 3: <=, 4: ==, 5: !=
    float threshold = 0.5f;
    bool hasExitTime = true;
    float exitTime = 0.9f;
    float blendTime = 0.2f;
    bool enabled = true;
};

struct AnimationGraphRuntime {
    bool enabled = false;
    std::vector<AnimationGraphStateData> states;
    std::vector<AnimationGraphTransitionData> transitions;
    std::vector<AnimationGraphParameter> parameters;

    int activeState = -1;
    int nextState = -1;
    bool blending = false;
    float blendDuration = 0.0f;
    float blendElapsed = 0.0f;
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
    MaterialPipeline transparentDoubleSidedPipeline;
    MaterialPipeline opaqueSkinnedPipeline;
    MaterialPipeline transparentSkinnedPipeline;
    MaterialPipeline transparentDoubleSidedSkinnedPipeline;

    VkDescriptorSetLayout materialLayout;

    struct MaterialConstants {
        glm::vec4 colorFactors;          // baseColor RGBA
        glm::vec4 metal_rough_factors;   // x = metallic, y = roughness, z = ao, w = normalStrength
        uint32_t colorTexID;             // Bindless texture ID for base color
        uint32_t metalRoughTexID;        // Bindless texture ID for metallic-roughness
        uint32_t normalTexID;            // Bindless texture ID for normal map (0 = none)
        uint32_t emissiveTexID;          // Bindless texture ID for emissive map (0 = none)
        glm::vec4 extra[13];             // extra[0] = emission (xyz=color, w=strength)
                                         // extra[1].x = reflection intensity
                                         // extra[2].x = displacement scale, extra[2].y = displacement bias
                                         // extra[11].x = displacement texture ID (float-encoded)
                                         // extra[11].y = AO texture ID (float-encoded)
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

    MaterialInstance write_material(VkDevice device, MaterialPass pass, bool doubleSided, const MaterialResources& resources, DescriptorAllocatorGrowable& descriptorAllocator);
};


class VulkanEngine {
public:
    // =========================================================================
    // ANIMATION SYSTEM
    // =========================================================================
    std::vector<AnimationClipData> animationClips;
    std::vector<SkeletonData> skeletons;
    AnimationGraphRuntime animationGraph;
    int activeAnimationIndex = -1;
    int activeSkeletonIndex = -1;
    AllocatedBuffer skinningMatrixBuffer;
    VkDeviceAddress skinningMatrixBufferAddress = 0;
    std::array<glm::mat4, 1024> skinningMatrices{};

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
    void rebuildShaderPipelineRegistry();

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
    VkPipeline _wireframeSkinnedPipeline = VK_NULL_HANDLE;
    VkPipelineLayout _wireframeSkinnedPipelineLayout = VK_NULL_HANDLE;
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
    void draw_background_effect(VkCommandBuffer cmd);
    VkDescriptorSet _drawImageDescriptorSet;
    void allocate_draw_image_descriptor_set();

    VkPipeline _2dPipeline = VK_NULL_HANDLE;
    VkPipeline _gridPipeline = VK_NULL_HANDLE;
    VkPipeline _emissivePipeline = VK_NULL_HANDLE;

    // Primitive pipeline with face color support
    VkPipeline _primitivePipeline = VK_NULL_HANDLE;
    VkPipelineLayout _primitivePipelineLayout = VK_NULL_HANDLE;
    VkPipeline _primitiveWireframePipeline = VK_NULL_HANDLE;  // Wireframe mode for primitives
    VkPipeline _primitiveSolidPipeline = VK_NULL_HANDLE;      // Solid color mode for primitives
    void init_primitive_pipeline();
    void init_primitive_wireframe_pipeline();
    void init_primitive_solid_pipeline();
    void draw_primitives(VkCommandBuffer cmd, VkDescriptorSet globalDescriptor);
    void draw_primitives_with_viewport(VkCommandBuffer cmd, VkDescriptorSet globalDescriptor, VkViewport viewport, VkRect2D scissor, ViewMode viewMode);
    VkPipeline select_primitive_pipeline(ViewMode viewMode, ShaderOnlyMaterial materialType);
    void init_point_light_vis_pipeline();

    VkPipelineLayout _gridPipelineLayout = VK_NULL_HANDLE;
    VkPipelineLayout _emissivePipelineLayout = VK_NULL_HANDLE;

    VkPipeline _pointLightVisPipeline = VK_NULL_HANDLE;
    VkPipelineLayout _pointLightVisPipelineLayout = VK_NULL_HANDLE;


    VkPipeline _outlinePipeline = VK_NULL_HANDLE;
    VkPipelineLayout _outlinePipelineLayout = VK_NULL_HANDLE;
    void draw_shader_only_static_shapes(VkCommandBuffer cmd, VkDescriptorSet globalDescriptor, VkViewport viewport, VkRect2D scissor);
    void init_default_meshes();
    ViewMode _currentViewMode = ViewMode::Rendered;  // Rendered = full PBR with shadows
    // primitive mesh oluşturucular:
    GPUMeshBuffers generate_cube_mesh();
    GPUMeshBuffers generate_plane_mesh();
    GPUMeshBuffers generate_sphere_mesh(int resolution = 24, int rings = 16);
    GPUMeshBuffers generate_cylinder_mesh(int segments = 32);
    GPUMeshBuffers generate_cone_mesh();
    GPUMeshBuffers generate_capsule_mesh();
    GPUMeshBuffers generate_torus_mesh();
    GPUMeshBuffers generate_triangle_mesh();
    GPUMeshBuffers generateMeshForPrimitiveType(PrimitiveType type);
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

    Node* selectedNode = nullptr;      // Selected GLTF node (Node* to support both mesh and non-mesh nodes)
    int selectedGLTFSurfaceIndex = 0;      // Shared GLTF surface selection for MaterialView/ObjectInspector
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
    VkExtent2D _windowExtent{ 1700, 900 };
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

    // GLTF Camera management
    std::string currentGLTFCameraScene;  // Scene name that camera belongs to
    int currentGLTFCameraIndex = -1;     // -1 = using mainCamera, >= 0 = GLTF camera index
    bool useGLTFCamera = false;

    // Apply a GLTF camera to the main camera
    void applyGLTFCamera(const std::string& sceneName, int cameraIndex);
    void resetToFreeCamera();
    GLTFCamera* getCurrentGLTFCamera();
    std::vector<std::pair<std::string, GLTFCamera*>> getAllGLTFCameras();

    DescriptorAllocator globalDescriptorAllocator;

    VkPipeline _2dPipelineGrid = VK_NULL_HANDLE;

    VkPipeline _gradientPipeline;
    VkPipelineLayout _gradientPipelineLayout;

    // Skybox background effect (needs cubemap descriptor)
    VkPipelineLayout _skyboxBgPipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout _skyboxBgDescriptorLayout = VK_NULL_HANDLE;
    VkDescriptorSet _skyboxBgDescriptorSet = VK_NULL_HANDLE;
    VkImageView _skyboxBgLastDrawImageView = VK_NULL_HANDLE;
    VkImageView _skyboxBgLastCubemapView = VK_NULL_HANDLE;
    VkSampler _skyboxBgLastCubemapSampler = VK_NULL_HANDLE;
    void updateSkyboxBgDescriptor();

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
    AllocatedImage _defaultCubemap; // 1x1 black cubemap for fallback

    VkSampler _defaultSamplerLinear;
    VkSampler _defaultSamplerNearest;

    // Default material for primitives (uses white textures)
    MaterialInstance _defaultPrimitiveMaterial;
    AllocatedBuffer _primitiveMaterialDataBuffer;

    // Track dynamically created primitive material resources for cleanup
    std::vector<AllocatedImage> _dynamicPrimitiveMaterialImages;
    std::vector<AllocatedBuffer> _dynamicPrimitiveMaterialBuffers;

    // Helper to create primitive material with custom textures
    MaterialInstance create_primitive_material(
        const std::string& albedoPath = "",
        const std::string& metalRoughPath = "",
        const std::string& emissionPath = "",
        const std::string& displacementPath = "",
        float displacementScale = 0.0f,
        float displacementBias = 0.0f
    );
    void init_default_primitive_material();

    // ==========================================================================
    // SHADOW MAPPING SYSTEM
    // ==========================================================================
    static constexpr uint32_t SHADOW_MAP_SIZE = 4096;  // Shadow map resolution (4K atlas, 2K per cascade)
    static constexpr uint32_t SHADOW_CASCADE_COUNT = 4;  // Number of cascade levels

    // Shadow map textures (one per cascade)
    AllocatedImage _shadowMapImage;
    VkImageView _shadowMapView = VK_NULL_HANDLE;
    VkSampler _shadowSampler = VK_NULL_HANDLE;

    // Shadow pipeline (depth-only rendering)
    VkPipeline _shadowPipeline = VK_NULL_HANDLE;
    VkPipelineLayout _shadowPipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout _shadowDescriptorLayout = VK_NULL_HANDLE;
    VkDescriptorSet _shadowDescriptorSet = VK_NULL_HANDLE;

    // Cascade shadow map data
    struct CascadeData {
        glm::mat4 viewProjMatrix;
        float splitDepth;
    };
    std::array<CascadeData, SHADOW_CASCADE_COUNT> _shadowCascades;

    // Shadow mapping functions
    void init_shadow_map();
    void init_shadow_pipeline();
    void update_shadow_cascades();
    void render_shadow_pass(VkCommandBuffer cmd);
    glm::mat4 get_light_space_matrix(float nearPlane, float farPlane);

    // Shadow settings (exposed to UI)
    float shadowBias = 0.002f;   // Base bias for shadow acne prevention
    float shadowNormalBias = 0.015f;  // Normal offset bias

    // === Point Light Shadow Cubemaps ===
    static constexpr uint32_t POINT_LIGHT_SHADOW_SIZE = 1024;  // Resolution per face
    static constexpr uint32_t MAX_SHADOW_POINT_LIGHTS = 4;    // Max lights with shadows

    struct PointLightShadowData {
        AllocatedImage cubemap;           // Cubemap depth image (6 faces)
        VkImageView cubemapView = VK_NULL_HANDLE;        // Full cubemap view
        VkImageView faceViews[6] = {};    // Per-face views for rendering
        int lightIndex = -1;              // Index into scenePointLights
    };
    std::array<PointLightShadowData, MAX_SHADOW_POINT_LIGHTS> _pointLightShadows;
    VkSampler _pointLightShadowSampler = VK_NULL_HANDLE;
    VkPipeline _pointLightShadowPipeline = VK_NULL_HANDLE;
    VkPipelineLayout _pointLightShadowPipelineLayout = VK_NULL_HANDLE;
    bool pointLightShadowsEnabled = true;

    // Point light shadow functions
    void init_point_light_shadow_maps();
    void render_point_light_shadows(VkCommandBuffer cmd);
    void update_point_light_shadow_data();
    bool shadowsEnabled = true;

    // Sun/Directional light toggle
    bool sunEnabled = true;
    float savedSunIntensity = 3.0f;  // Store intensity when disabled

    // ==========================================================================
    // POST-PROCESSING SYSTEM
    // ==========================================================================
    std::unique_ptr<Yalaz::Renderer::PostProcessManager> _postProcessManager;
    std::unique_ptr<Yalaz::Renderer::BloomPass> _bloomPass;
    Yalaz::Renderer::RenderSettings _renderSettings;

    // Final tone mapping compute pass (runs AFTER bloom)
    VkPipeline _tonemapPipeline = VK_NULL_HANDLE;
    VkPipelineLayout _tonemapPipelineLayout = VK_NULL_HANDLE;
    void init_tonemap_pipeline();
    void execute_tonemap(VkCommandBuffer cmd);

    // G-Buffer for deferred effects (SSAO, SSR)
    AllocatedImage _gBufferNormals;       // RGB = world normals
    AllocatedImage _gBufferMetalRough;    // R = metallic, G = roughness

    // HDR render target (before post-processing)
    AllocatedImage _hdrBuffer;

    // Post-process functions
    void init_post_processing();
    void cleanup_post_processing();
    void render_post_processing(VkCommandBuffer cmd);
    void init_gbuffer();

    // ==========================================================================
    // PATH TRACING SYSTEM
    // ==========================================================================
    std::unique_ptr<Yalaz::Renderer::PathTracer> _pathTracer;
    void init_path_tracer();
    void cleanup_path_tracer();

    // ==========================================================================
    // ENVIRONMENT MAP / SKYBOX SYSTEM
    // ==========================================================================
    std::unique_ptr<Yalaz::Renderer::EnvironmentMap> _environmentMap;
    void init_environment_map();
    void cleanup_environment_map();

    // ==========================================================================
    // MULTI-PROBE REFLECTION SYSTEM
    // ==========================================================================
    static constexpr uint32_t REFLECTION_PROBE_SIZE = 256;
    static constexpr int REFLECTION_UPDATE_INTERVAL = 6;
    static constexpr int MAX_REFLECTION_PROBES = 4;

    std::array<ReflectionProbe, 4> _reflectionProbes;
    int _currentProbeUpdateIndex = 0;
    int _reflectionFrameCounter = 0;
    AllocatedImage _sharedProbeDepth;
    bool _probesReady = false;

    void init_reflection_probes();
    void cleanup_reflection_probes();
    void render_reflection_probe_single(VkCommandBuffer cmd, int probeIndex);
    glm::mat4 getReflectionFaceViewMatrix(int face, const glm::vec3& probePos) const;
    glm::mat4 getReflectionProjectionMatrix() const;

    // ==========================================================================
    // SPOT LIGHT SYSTEM
    // ==========================================================================
    std::vector<SpotLight> sceneSpotLights;
    AllocatedBuffer _spotLightBuffer;
    static constexpr uint32_t SPOT_LIGHT_SHADOW_SIZE = 1024;

    struct SpotLightShadowData {
        AllocatedImage shadowMap;
        VkImageView shadowView = VK_NULL_HANDLE;
        glm::mat4 viewProjMatrix;
        int lightIndex = -1;
    };
    std::array<SpotLightShadowData, MAX_SHADOW_CASTING_SPOT_LIGHTS> _spotLightShadows;
    VkSampler _spotLightShadowSampler = VK_NULL_HANDLE;
    VkPipeline _spotLightShadowPipeline = VK_NULL_HANDLE;
    VkPipelineLayout _spotLightShadowPipelineLayout = VK_NULL_HANDLE;

    void init_spot_light_shadows();
    void render_spot_light_shadows(VkCommandBuffer cmd);
    void sync_spot_lights();

    // ==========================================================================
    // GPU-DRIVEN RENDERING SYSTEM
    // ==========================================================================
    // Efficient batched rendering using indirect draw calls and compute culling
    // ==========================================================================

    // GPU object data structure (per-object transform + bounds for culling)
    struct GPUObjectData {
        glm::mat4 modelMatrix;      // Object transform
        glm::vec4 sphereBounds;     // xyz = center, w = radius (for frustum culling)
        uint32_t materialIndex;     // Index into material buffer
        uint32_t meshIndex;         // Index into mesh buffer
        uint32_t flags;             // Visibility, shadow, etc.
        uint32_t padding;
    };

    // Indirect draw command (matches VkDrawIndexedIndirectCommand)
    struct GPUIndirectCommand {
        uint32_t indexCount;
        uint32_t instanceCount;
        uint32_t firstIndex;
        int32_t  vertexOffset;
        uint32_t firstInstance;     // Used as object index
    };

    // GPU-Driven rendering buffers
    AllocatedBuffer _gpuObjectBuffer;           // All object data
    AllocatedBuffer _indirectDrawBuffer;        // Indirect draw commands
    AllocatedBuffer _drawCountBuffer;           // Number of draws after culling
    AllocatedBuffer _gpuMeshInfoBuffer;         // Mesh vertex/index info

    // Compute shader for frustum culling
    VkPipeline _cullComputePipeline = VK_NULL_HANDLE;
    VkPipelineLayout _cullComputeLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout _cullDescriptorLayout = VK_NULL_HANDLE;

    // GPU-Driven rendering settings
    bool gpuDrivenEnabled = false;              // Toggle for GPU-driven mode
    uint32_t maxGPUObjects = 10000;             // Max objects in GPU buffer

    // GPU-Driven rendering functions
    void init_gpu_driven_rendering();
    void update_gpu_object_buffer();
    void perform_gpu_culling(VkCommandBuffer cmd);
    void draw_indirect(VkCommandBuffer cmd, VkDescriptorSet globalDescriptor);
    int shadowPcfSamples = 3;  // PCF filter kernel size (1, 2, 3 for 3x3, 5x5, 7x7)

    EngineStats stats;
    std::vector<ComputeEffect> backgroundEffects;
    int currentBackgroundEffect{ 0 };

    GPUMeshBuffers uploadMesh(std::span<uint32_t> indices, std::span<Vertex> vertices);
    AllocatedBuffer uploadSkinBuffer(std::span<SkinVertexData> skinData);

    AllocatedBuffer create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);

    AllocatedImage create_image(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
    AllocatedImage create_image(void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
    

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

    static VulkanEngine& Get();
    void destroy_buffer(const AllocatedBuffer& buffer);
    void draw_scene_light_imgui();
    void init_grid_pipeline();
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

    // Deferred scene unload: scenes marked for removal are destroyed at start of next frame
    // (after GPU fence wait) instead of during ImGui rendering (mid-command-buffer)
    std::vector<std::string> _pendingSceneUnloads;
    void processPendingSceneUnloads();

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

    void render_nodes();

    void draw_geometry(VkCommandBuffer cmd);
    void init_scene_data();

    void draw_background(VkCommandBuffer cmd);

    void create_material_constant_buffer(const GLTFMetallic_Roughness::MaterialConstants& data);
    void run();
    
    void update_scene();

private:

    void rebuild_swapchain();
    void init_vulkan();
    void init_swapchain();
    void create_swapchain(uint32_t width, uint32_t height);
    void destroy_swapchain();
	void resize_swapchain();
    void init_commands();
    void init_pipelines();
    void cleanup_reloadable_pipelines();
    bool compile_shader_to_spirv(const std::string& spvPath, std::string& outLog, float& outMs);
    void rebuild_pipelines_after_shader_recompile();
    void init_background_pipelines();
	void init_triangle_pipeline();
	void init_mesh_pipeline();
    void init_descriptors();
    void init_sync_structures();
    void init_renderables();
    void init_imgui();
    void init_default_data();
};
