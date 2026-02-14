//#pragma once
#pragma once

#include <vk_types.h>
#include "vk_descriptors.h"
#include <unordered_map>
#include <filesystem>
#include <glm/gtc/matrix_transform.hpp>



class VulkanEngine;

// Bounds is now defined in vk_types.h

struct GLTFMaterial {
    MaterialInstance data;
    uint32_t bufferOffset = 0;  // Offset in materialDataBuffer for real-time updates
};

// GLTF Camera - stores camera data from GLTF files
struct GLTFCamera {
    std::string name;

    // Camera type
    bool isPerspective = true;

    // Perspective camera params
    float fov = 60.0f;           // Vertical FOV in degrees
    float aspectRatio = 0.0f;    // 0 = use window aspect
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;

    // Orthographic camera params
    float orthoWidth = 10.0f;    // xmag
    float orthoHeight = 10.0f;   // ymag

    // Transform from node hierarchy (world space)
    glm::mat4 worldTransform = glm::mat4(1.0f);
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 forward = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    Node* sourceNode = nullptr;  // Node that owns this camera in the loaded scene graph

    // Helper to get view matrix
    glm::mat4 getViewMatrix() const {
        return glm::inverse(worldTransform);
    }

    // Helper to get projection matrix
    glm::mat4 getProjectionMatrix(float windowAspect) const {
        float aspect = (aspectRatio > 0.0f) ? aspectRatio : windowAspect;
        if (isPerspective) {
            return glm::perspective(glm::radians(fov), aspect, nearPlane, farPlane);
        } else {
            float halfW = orthoWidth * 0.5f;
            float halfH = orthoHeight * 0.5f;
            return glm::ortho(-halfW, halfW, -halfH, halfH, nearPlane, farPlane);
        }
    }
};

struct GLTFLight {
    std::string name;
    int type = 1; // 0=directional, 1=point, 2=spot
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 direction = glm::vec3(0.0f, -1.0f, 0.0f);
    glm::vec3 color = glm::vec3(1.0f);
    float intensity = 1.0f;
    float range = 25.0f;
    Node* sourceNode = nullptr;
    int runtimePointLightIndex = -1;
};

struct GeoSurface {
    uint32_t startIndex;
    uint32_t count;
    Bounds bounds;
    std::shared_ptr<GLTFMaterial> material;
};

struct MeshAsset {
    std::string name;
    std::vector<GeoSurface> surfaces;
    GPUMeshBuffers meshBuffers;
    AllocatedBuffer skinBuffer;
    VkDeviceAddress skinBufferAddress = 0;
    bool hasSkinData = false;

    // CPU-side vertex data for path tracing (optional, populated on demand)
    std::vector<Vertex> cpuVertices;
    std::vector<uint32_t> cpuIndices;
    bool hasCpuData = false;
};

struct LoadedGLTF : public IRenderable {

    // storage for all the data on a given gltf file
    std::unordered_map<std::string, std::shared_ptr<MeshAsset>> meshes;
    std::unordered_map<std::string, std::shared_ptr<Node>> nodes;
    std::unordered_map<std::string, AllocatedImage> images;
    std::unordered_map<std::string, std::shared_ptr<GLTFMaterial>> materials;
    std::vector<std::shared_ptr<Node>> indexedNodes; // Stable GLTF node-index to Node mapping

    // GLTF cameras loaded from the scene
    std::vector<GLTFCamera> cameras;
    std::vector<GLTFLight> lights;

    // nodes that dont have a parent, for iterating through the file in tree order
    std::vector<std::shared_ptr<Node>> topNodes;

    std::vector<VkSampler> samplers;

    DescriptorAllocatorGrowable descriptorPool;

    AllocatedBuffer materialDataBuffer;

    VulkanEngine* creator;

    ~LoadedGLTF() { clearAll(); };

    virtual void Draw(const glm::mat4& topMatrix, DrawContext& ctx);

private:

    void clearAll();
};
// .obj dosyasını yükleyip GPUMeshBuffers döndürür
//GPUMeshBuffers load_obj_mesh(VulkanEngine* engine, const std::string& filename);
std::optional<std::shared_ptr<LoadedGLTF>> loadObj(VulkanEngine* engine, std::string_view filePath);

std::optional<std::shared_ptr<LoadedGLTF>> loadGltf(VulkanEngine* engine, std::string_view filePath);

// Unified scene/model loader used by UI + scene restore.
// Supports: .gltf, .glb, .obj, .fbx, .dae and .mtl (via matching .obj).
std::optional<std::shared_ptr<LoadedGLTF>> loadSceneAsset(VulkanEngine* engine, std::string_view filePath);

// Lightweight CPU thumbnail preview generator for model files.
bool generateModelPreviewRGBA(std::string_view filePath, int maxSize, std::vector<uint8_t>& outRGBA, int& outW, int& outH);
