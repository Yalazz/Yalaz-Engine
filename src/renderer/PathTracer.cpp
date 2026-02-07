#include "PathTracer.h"
#include "vk_engine.h"
#include "vk_initializers.h"
#include "vk_pipelines.h"
#include "vk_loader.h"
#include "geometry/PrimitiveGenerator.h"
#include <algorithm>
#include <functional>
#include <fmt/core.h>

namespace Yalaz::Renderer {

PathTracer::PathTracer(VulkanEngine* engine)
    : _engine(engine) {}

PathTracer::~PathTracer() {
    cleanup();
}

void PathTracer::init() {
    // Only create pipeline - images will be created lazily when extent is valid
    createPipeline();
    _lastViewMatrix = glm::mat4(1.0f);
    _imagesInitialized = false;
}

void PathTracer::cleanup() {
    vkDeviceWaitIdle(_engine->_device);

    _imagesInitialized = false;
    _imageExtent = {0, 0};
    _drawImageView = VK_NULL_HANDLE;

    if (_accumulationImage.image != VK_NULL_HANDLE) {
        _engine->destroy_image(_accumulationImage);
        _accumulationImage = {};
    }
    if (_triangleBuffer.buffer != VK_NULL_HANDLE) {
        _engine->destroy_buffer(_triangleBuffer);
        _triangleBuffer = {};
    }
    if (_bvhBuffer.buffer != VK_NULL_HANDLE) {
        _engine->destroy_buffer(_bvhBuffer);
        _bvhBuffer = {};
    }
    if (_materialBuffer.buffer != VK_NULL_HANDLE) {
        _engine->destroy_buffer(_materialBuffer);
        _materialBuffer = {};
    }
    if (_pathTracePipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(_engine->_device, _pathTracePipeline, nullptr);
        _pathTracePipeline = VK_NULL_HANDLE;
    }
    if (_pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(_engine->_device, _pipelineLayout, nullptr);
        _pipelineLayout = VK_NULL_HANDLE;
    }
    if (_descriptorLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(_engine->_device, _descriptorLayout, nullptr);
        _descriptorLayout = VK_NULL_HANDLE;
    }
}

void PathTracer::ensureImagesReady() {
    VkExtent2D currentExtent = _engine->_drawExtent;

    // Skip if extent is invalid (0x0)
    if (currentExtent.width == 0 || currentExtent.height == 0) {
        return;
    }

    // Check if we need to recreate images (first time or resize)
    bool needsRecreate = !_imagesInitialized ||
                         currentExtent.width != _imageExtent.width ||
                         currentExtent.height != _imageExtent.height;

    if (!needsRecreate) {
        return;
    }

    // Wait for device to be idle before recreating
    vkDeviceWaitIdle(_engine->_device);

    // Cleanup old accumulation image if it exists
    if (_accumulationImage.image != VK_NULL_HANDLE) {
        _engine->destroy_image(_accumulationImage);
        _accumulationImage = {};
    }

    createImages();
    _imageExtent = currentExtent;
    _imagesInitialized = true;

    // Update descriptors with new images (BVH will be built on demand)
    if (_triangleBuffer.buffer != VK_NULL_HANDLE) {
        updateDescriptors();
    }

    // Reset accumulation since images changed
    resetAccumulation();

    fmt::print("[PathTracer] Images created: {}x{}\n", currentExtent.width, currentExtent.height);
}

void PathTracer::createImages() {
    VkExtent3D extent = {
        _engine->_drawExtent.width,
        _engine->_drawExtent.height,
        1
    };

    // Accumulation buffer (RGBA32F for high precision temporal averaging)
    _accumulationImage = _engine->create_image(
        extent,
        VK_FORMAT_R32G32B32A32_SFLOAT,
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
    );
}

void PathTracer::createPipeline() {
    // Descriptor layout
    VkDescriptorSetLayoutBinding bindings[6] = {};

    // Output image
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    // Accumulation image
    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    // Triangle buffer
    bindings[2].binding = 2;
    bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[2].descriptorCount = 1;
    bindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    // BVH buffer
    bindings[3].binding = 3;
    bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[3].descriptorCount = 1;
    bindings[3].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    // Material buffer
    bindings[4].binding = 4;
    bindings[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[4].descriptorCount = 1;
    bindings[4].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    // Environment cubemap
    bindings[5].binding = 5;
    bindings[5].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[5].descriptorCount = 1;
    bindings[5].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 6;
    layoutInfo.pBindings = bindings;

    VK_CHECK(vkCreateDescriptorSetLayout(_engine->_device, &layoutInfo, nullptr, &_descriptorLayout));

    // Push constants
    VkPushConstantRange pushConstant{};
    pushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pushConstant.offset = 0;
    pushConstant.size = sizeof(PathTracePushConstants);

    // Pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &_descriptorLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstant;

    VK_CHECK(vkCreatePipelineLayout(_engine->_device, &pipelineLayoutInfo, nullptr, &_pipelineLayout));

    // Load compute shader
    VkShaderModule shader = _engine->load_shader_module("../../shaders/pathtrace_bvh.comp.spv");

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.layout = _pipelineLayout;
    pipelineInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipelineInfo.stage.module = shader;
    pipelineInfo.stage.pName = "main";

    VK_CHECK(vkCreateComputePipelines(_engine->_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_pathTracePipeline));

    vkDestroyShaderModule(_engine->_device, shader, nullptr);

    // Allocate descriptor set
    _descriptorSet = _engine->globalDescriptorAllocator.allocate(_engine->_device, _descriptorLayout);
}

void PathTracer::buildBVH() {
    fmt::print("[PathTracer] Building BVH from scene geometry...\n");

    // Collect all triangles from the scene
    _triangles.clear();
    _materials.clear();

    // Default material (index 0)
    GPUPathTraceMaterial defaultMat{};
    defaultMat.albedo = glm::vec3(0.8f);
    defaultMat.metallic = 0.0f;
    defaultMat.roughness = 0.5f;
    defaultMat.emission = glm::vec3(0.0f);
    defaultMat.ior = 1.5f;
    defaultMat.transmission = 0.0f;
    defaultMat.albedoTexture = UINT32_MAX;
    defaultMat.normalTexture = UINT32_MAX;
    _materials.push_back(defaultMat);

    // Helper to add a triangle with transform
    auto addTriangle = [&](const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2,
                           const glm::mat4& transform, uint32_t matIndex) {
        GPUTriangle tri{};
        tri.v0 = glm::vec3(transform * glm::vec4(v0, 1.0f));
        tri.v1 = glm::vec3(transform * glm::vec4(v1, 1.0f));
        tri.v2 = glm::vec3(transform * glm::vec4(v2, 1.0f));
        tri.materialIndex = matIndex;
        _triangles.push_back(tri);
    };

    // ==========================================================================
    // Extract triangles from static primitives
    // ==========================================================================
    for (auto& shape : _engine->static_shapes) {
        if (!shape.visible) continue;

        // Create material for this primitive
        uint32_t matIndex = static_cast<uint32_t>(_materials.size());
        GPUPathTraceMaterial mat{};
        mat.albedo = glm::vec3(shape.mainColor);
        mat.metallic = shape.metallic;
        mat.roughness = shape.roughness;
        mat.emission = glm::vec3(0.0f);
        mat.ior = 1.5f;
        mat.transmission = 0.0f;
        mat.albedoTexture = UINT32_MAX;
        mat.normalTexture = UINT32_MAX;
        _materials.push_back(mat);

        // Get transform matrix
        glm::mat4 transform = shape.get_transform();

        // Generate CPU-side mesh data for this primitive type
        auto& generator = Yalaz::Geometry::PrimitiveGenerator::Get();
        Yalaz::Geometry::MeshData meshData = generator.GenerateData(
            static_cast<Yalaz::Geometry::PrimitiveType>(shape.type));

        // Extract triangles from mesh
        for (size_t i = 0; i + 2 < meshData.indices.size(); i += 3) {
            uint32_t i0 = meshData.indices[i];
            uint32_t i1 = meshData.indices[i + 1];
            uint32_t i2 = meshData.indices[i + 2];

            if (i0 < meshData.vertices.size() && i1 < meshData.vertices.size() && i2 < meshData.vertices.size()) {
                addTriangle(
                    meshData.vertices[i0].position,
                    meshData.vertices[i1].position,
                    meshData.vertices[i2].position,
                    transform,
                    matIndex
                );
            }
        }
    }

    // ==========================================================================
    // Extract triangles from loaded GLTF scenes
    // ==========================================================================
    for (auto& [sceneName, scene] : _engine->loadedScenes) {
        // Recursive function to process nodes
        std::function<void(std::shared_ptr<Node>, const glm::mat4&)> processNode =
            [&](std::shared_ptr<Node> node, const glm::mat4& parentTransform) {
            if (!node) return;

            glm::mat4 nodeTransform = parentTransform * node->worldTransform;

            // Process mesh if this is a MeshNode
            if (auto meshNode = dynamic_cast<MeshNode*>(node.get())) {
                auto meshAsset = meshNode->mesh;
                if (meshAsset && meshAsset->hasCpuData) {
                    // Extract per-surface with real GLTF materials
                    for (auto& surface : meshAsset->surfaces) {
                        // Create material from GLTF surface material data
                        uint32_t matIndex = static_cast<uint32_t>(_materials.size());
                        GPUPathTraceMaterial ptMat{};
                        ptMat.albedo = glm::vec3(0.8f);
                        ptMat.metallic = 0.0f;
                        ptMat.roughness = 0.5f;
                        ptMat.emission = glm::vec3(0.0f);
                        ptMat.ior = 1.5f;
                        ptMat.transmission = 0.0f;
                        ptMat.albedoTexture = UINT32_MAX;
                        ptMat.normalTexture = UINT32_MAX;

                        // Extract material constants from materialDataBuffer
                        if (surface.material && scene->materialDataBuffer.buffer != VK_NULL_HANDLE &&
                            scene->materialDataBuffer.info.pMappedData != nullptr) {
                            auto* constants = reinterpret_cast<GLTFMetallic_Roughness::MaterialConstants*>(
                                reinterpret_cast<uint8_t*>(scene->materialDataBuffer.info.pMappedData) +
                                surface.material->bufferOffset);
                            ptMat.albedo = glm::vec3(constants->colorFactors);
                            ptMat.metallic = constants->metal_rough_factors.x;
                            ptMat.roughness = constants->metal_rough_factors.y;
                        }

                        // Compute average vertex color for this surface (many GLTF models
                        // use vertex colors instead of base color textures)
                        glm::vec3 avgColor(0.0f);
                        int colorSamples = 0;
                        uint32_t startIdx = surface.startIndex;
                        uint32_t endIdx = startIdx + surface.count;
                        for (uint32_t i = startIdx; i < endIdx && i < meshAsset->cpuIndices.size(); i++) {
                            uint32_t vi = meshAsset->cpuIndices[i];
                            if (vi < meshAsset->cpuVertices.size()) {
                                avgColor += glm::vec3(meshAsset->cpuVertices[vi].color);
                                colorSamples++;
                            }
                        }
                        if (colorSamples > 0) {
                            avgColor /= static_cast<float>(colorSamples);
                            // Only use vertex color if it's not all white (default)
                            if (avgColor.x < 0.99f || avgColor.y < 0.99f || avgColor.z < 0.99f) {
                                ptMat.albedo = avgColor * glm::vec3(ptMat.albedo);
                            }
                        }

                        _materials.push_back(ptMat);

                        // Extract triangles for this surface
                        for (uint32_t i = startIdx; i + 2 < endIdx; i += 3) {
                            if (i + 2 >= meshAsset->cpuIndices.size()) break;

                            uint32_t i0 = meshAsset->cpuIndices[i];
                            uint32_t i1 = meshAsset->cpuIndices[i + 1];
                            uint32_t i2 = meshAsset->cpuIndices[i + 2];

                            if (i0 < meshAsset->cpuVertices.size() &&
                                i1 < meshAsset->cpuVertices.size() &&
                                i2 < meshAsset->cpuVertices.size()) {
                                addTriangle(
                                    meshAsset->cpuVertices[i0].position,
                                    meshAsset->cpuVertices[i1].position,
                                    meshAsset->cpuVertices[i2].position,
                                    nodeTransform,
                                    matIndex
                                );
                            }
                        }
                    }
                }
            }

            // Process children
            for (auto& child : node->children) {
                processNode(child, nodeTransform);
            }
        };

        // Process all top-level nodes
        for (auto& topNode : scene->topNodes) {
            processNode(topNode, glm::mat4(1.0f));
        }
    }

    if (_triangles.empty()) {
        fmt::print("[PathTracer] No scene geometry found\n");
    }

    stats.triangleCount = static_cast<uint32_t>(_triangles.size());
    fmt::print("[PathTracer] Extracted {} triangles, {} materials\n",
        stats.triangleCount, _materials.size());

    // Debug: print material colors
    for (size_t i = 0; i < _materials.size(); i++) {
        auto& m = _materials[i];
        fmt::print("[PathTracer]   Mat[{}]: albedo=({:.3f},{:.3f},{:.3f}) metal={:.2f} rough={:.2f}\n",
            i, m.albedo.x, m.albedo.y, m.albedo.z, m.metallic, m.roughness);
    }

    // Build BVH
    _bvhNodes.clear();
    _primitiveIndices.clear();

    if (!_triangles.empty()) {
        // Initialize primitive indices
        _primitiveIndices.resize(_triangles.size());
        for (uint32_t i = 0; i < _triangles.size(); ++i) {
            _primitiveIndices[i] = i;
        }

        // Create root node
        BVHBuildNode root{};
        root.leftFirst = 0;
        root.primCount = static_cast<int>(_triangles.size());
        _bvhNodes.push_back(root);

        // Build recursively
        updateNodeBounds(0);
        buildBVHRecursive(0, 0, static_cast<uint32_t>(_triangles.size()));
    }

    stats.bvhNodeCount = static_cast<uint32_t>(_bvhNodes.size());
    fmt::print("[PathTracer] BVH built with {} nodes\n", stats.bvhNodeCount);

    // Wait for GPU to finish all commands before touching buffers
    vkDeviceWaitIdle(_engine->_device);

    // Upload to GPU - save old buffers, create new ones, then destroy old
    AllocatedBuffer oldTriBuf = _triangleBuffer;
    AllocatedBuffer oldBvhBuf = _bvhBuffer;
    AllocatedBuffer oldMatBuf = _materialBuffer;
    _triangleBuffer = {};
    _bvhBuffer = {};
    _materialBuffer = {};

    createBuffers();
    uploadSceneData();
    updateDescriptors();

    // Now safe to destroy old buffers (GPU is idle, descriptor points to new ones)
    if (oldTriBuf.buffer != VK_NULL_HANDLE)
        _engine->destroy_buffer(oldTriBuf);
    if (oldBvhBuf.buffer != VK_NULL_HANDLE)
        _engine->destroy_buffer(oldBvhBuf);
    if (oldMatBuf.buffer != VK_NULL_HANDLE)
        _engine->destroy_buffer(oldMatBuf);
}

void PathTracer::buildBVHRecursive(uint32_t nodeIdx, uint32_t start, uint32_t count) {
    // Leaf threshold
    const uint32_t MAX_PRIMS_PER_LEAF = 4;

    if (count <= MAX_PRIMS_PER_LEAF) {
        _bvhNodes[nodeIdx].leftFirst = start;
        _bvhNodes[nodeIdx].primCount = count;
        return;
    }

    // Find best split axis using extent
    glm::vec3 extent = _bvhNodes[nodeIdx].boundsMax - _bvhNodes[nodeIdx].boundsMin;
    int bestAxis = 0;
    if (extent.y > extent.x) bestAxis = 1;
    if (extent.z > extent[bestAxis]) bestAxis = 2;

    float splitPos = _bvhNodes[nodeIdx].boundsMin[bestAxis] + extent[bestAxis] * 0.5f;

    // Partition primitives based on centroid (spatial median)
    uint32_t i = start;
    uint32_t j = start + count - 1;
    while (i <= j && j < _primitiveIndices.size()) {
        uint32_t triIdx = _primitiveIndices[i];
        const GPUTriangle& tri = _triangles[triIdx];
        glm::vec3 centroid = (tri.v0 + tri.v1 + tri.v2) / 3.0f;

        if (centroid[bestAxis] < splitPos) {
            i++;
        } else {
            std::swap(_primitiveIndices[i], _primitiveIndices[j]);
            if (j == 0) break;
            j--;
        }
    }

    uint32_t leftCount = i - start;
    if (leftCount == 0 || leftCount == count) {
        // Spatial median failed - fall back to object median (sort and split in half)
        int axis = bestAxis;
        std::sort(_primitiveIndices.begin() + start, _primitiveIndices.begin() + start + count,
            [&](uint32_t a, uint32_t b) {
                glm::vec3 ca = (_triangles[a].v0 + _triangles[a].v1 + _triangles[a].v2) / 3.0f;
                glm::vec3 cb = (_triangles[b].v0 + _triangles[b].v1 + _triangles[b].v2) / 3.0f;
                return ca[axis] < cb[axis];
            });
        leftCount = count / 2;
    }

    // Create child nodes
    uint32_t leftChild = static_cast<uint32_t>(_bvhNodes.size());
    _bvhNodes.push_back({});
    _bvhNodes.push_back({});

    _bvhNodes[nodeIdx].leftFirst = leftChild;
    _bvhNodes[nodeIdx].primCount = 0;  // Internal node

    _bvhNodes[leftChild].leftFirst = start;
    _bvhNodes[leftChild].primCount = leftCount;
    updateNodeBounds(leftChild);

    _bvhNodes[leftChild + 1].leftFirst = start + leftCount;
    _bvhNodes[leftChild + 1].primCount = count - leftCount;
    updateNodeBounds(leftChild + 1);

    // Recurse
    buildBVHRecursive(leftChild, start, leftCount);
    buildBVHRecursive(leftChild + 1, start + leftCount, count - leftCount);
}

void PathTracer::updateNodeBounds(uint32_t nodeIdx) {
    BVHBuildNode& node = _bvhNodes[nodeIdx];
    node.boundsMin = glm::vec3(1e30f);
    node.boundsMax = glm::vec3(-1e30f);

    // Expand bounds to include all triangles in this node
    uint32_t first = node.leftFirst;
    uint32_t count = node.primCount;

    for (uint32_t i = 0; i < count; ++i) {
        uint32_t triIdx = _primitiveIndices[first + i];
        const GPUTriangle& tri = _triangles[triIdx];

        node.boundsMin = glm::min(node.boundsMin, tri.v0);
        node.boundsMin = glm::min(node.boundsMin, tri.v1);
        node.boundsMin = glm::min(node.boundsMin, tri.v2);

        node.boundsMax = glm::max(node.boundsMax, tri.v0);
        node.boundsMax = glm::max(node.boundsMax, tri.v1);
        node.boundsMax = glm::max(node.boundsMax, tri.v2);
    }

    // Small epsilon to avoid degenerate bounds
    glm::vec3 eps(0.0001f);
    node.boundsMin -= eps;
    node.boundsMax += eps;
}

float PathTracer::evaluateSAH(uint32_t nodeIdx, int axis, float pos) {
    // Surface Area Heuristic for BVH construction
    // Returns cost of splitting at given position
    return 1.0f;  // Placeholder
}

void PathTracer::createBuffers() {
    // Note: old buffers should be cleared by caller (buildBVH) before calling this.
    // They are destroyed AFTER updateDescriptors() to avoid invalid descriptor references.

    // Triangle buffer
    size_t triangleSize = std::max(size_t(1), _triangles.size()) * sizeof(GPUTriangle);
    _triangleBuffer = _engine->create_buffer(
        triangleSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY
    );

    // BVH buffer - convert BVHBuildNode to GPUBVHNode format
    size_t bvhSize = std::max(size_t(1), _bvhNodes.size()) * sizeof(GPUBVHNode);
    _bvhBuffer = _engine->create_buffer(
        bvhSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY
    );

    // Material buffer
    size_t materialSize = std::max(size_t(1), _materials.size()) * sizeof(GPUPathTraceMaterial);
    _materialBuffer = _engine->create_buffer(
        materialSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY
    );
}

void PathTracer::uploadSceneData() {
    if (_triangles.empty() || _bvhNodes.empty()) return;

    // Upload triangles (reordered by BVH)
    std::vector<GPUTriangle> reorderedTriangles(_triangles.size());
    for (size_t i = 0; i < _primitiveIndices.size(); ++i) {
        reorderedTriangles[i] = _triangles[_primitiveIndices[i]];
    }

    // Upload triangles
    {
        size_t dataSize = reorderedTriangles.size() * sizeof(GPUTriangle);
        AllocatedBuffer staging = _engine->create_buffer(
            dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU
        );

        void* mapped;
        vmaMapMemory(_engine->_allocator, staging.allocation, &mapped);
        memcpy(mapped, reorderedTriangles.data(), dataSize);
        vmaUnmapMemory(_engine->_allocator, staging.allocation);

        _engine->immediate_submit([&](VkCommandBuffer cmd) {
            VkBufferCopy copy{};
            copy.size = dataSize;
            vkCmdCopyBuffer(cmd, staging.buffer, _triangleBuffer.buffer, 1, &copy);
        });

        _engine->destroy_buffer(staging);
    }

    // Convert and upload BVH nodes
    {
        std::vector<GPUBVHNode> gpuNodes(_bvhNodes.size());
        for (size_t i = 0; i < _bvhNodes.size(); ++i) {
            gpuNodes[i].boundsMin = _bvhNodes[i].boundsMin;
            gpuNodes[i].leftFirst = _bvhNodes[i].leftFirst;
            gpuNodes[i].boundsMax = _bvhNodes[i].boundsMax;
            gpuNodes[i].primCount = _bvhNodes[i].primCount;
        }

        size_t dataSize = gpuNodes.size() * sizeof(GPUBVHNode);
        AllocatedBuffer staging = _engine->create_buffer(
            dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU
        );

        void* mapped;
        vmaMapMemory(_engine->_allocator, staging.allocation, &mapped);
        memcpy(mapped, gpuNodes.data(), dataSize);
        vmaUnmapMemory(_engine->_allocator, staging.allocation);

        _engine->immediate_submit([&](VkCommandBuffer cmd) {
            VkBufferCopy copy{};
            copy.size = dataSize;
            vkCmdCopyBuffer(cmd, staging.buffer, _bvhBuffer.buffer, 1, &copy);
        });

        _engine->destroy_buffer(staging);
    }

    // Upload materials
    {
        size_t dataSize = _materials.size() * sizeof(GPUPathTraceMaterial);
        AllocatedBuffer staging = _engine->create_buffer(
            dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU
        );

        void* mapped;
        vmaMapMemory(_engine->_allocator, staging.allocation, &mapped);
        memcpy(mapped, _materials.data(), dataSize);
        vmaUnmapMemory(_engine->_allocator, staging.allocation);

        _engine->immediate_submit([&](VkCommandBuffer cmd) {
            VkBufferCopy copy{};
            copy.size = dataSize;
            vkCmdCopyBuffer(cmd, staging.buffer, _materialBuffer.buffer, 1, &copy);
        });

        _engine->destroy_buffer(staging);
    }

    fmt::print("[PathTracer] Uploaded {} triangles, {} BVH nodes, {} materials to GPU\n",
        _triangles.size(), _bvhNodes.size(), _materials.size());
}

void PathTracer::updateDescriptors() {
    // Binding 0: engine's draw image (path tracer writes geometry pixels directly)
    VkDescriptorImageInfo outputInfo{};
    outputInfo.imageView = _drawImageView;
    outputInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

    VkDescriptorImageInfo accumInfo{};
    accumInfo.imageView = _accumulationImage.imageView;
    accumInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

    VkDescriptorBufferInfo triangleInfo{};
    triangleInfo.buffer = _triangleBuffer.buffer;
    triangleInfo.offset = 0;
    triangleInfo.range = VK_WHOLE_SIZE;

    VkDescriptorBufferInfo bvhInfo{};
    bvhInfo.buffer = _bvhBuffer.buffer;
    bvhInfo.offset = 0;
    bvhInfo.range = VK_WHOLE_SIZE;

    VkDescriptorBufferInfo materialInfo{};
    materialInfo.buffer = _materialBuffer.buffer;
    materialInfo.offset = 0;
    materialInfo.range = VK_WHOLE_SIZE;

    uint32_t writeCount = 5;
    VkWriteDescriptorSet writes[6] = {};

    writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].dstSet = _descriptorSet;
    writes[0].dstBinding = 0;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    writes[0].descriptorCount = 1;
    writes[0].pImageInfo = &outputInfo;

    writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[1].dstSet = _descriptorSet;
    writes[1].dstBinding = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    writes[1].descriptorCount = 1;
    writes[1].pImageInfo = &accumInfo;

    writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[2].dstSet = _descriptorSet;
    writes[2].dstBinding = 2;
    writes[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[2].descriptorCount = 1;
    writes[2].pBufferInfo = &triangleInfo;

    writes[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[3].dstSet = _descriptorSet;
    writes[3].dstBinding = 3;
    writes[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[3].descriptorCount = 1;
    writes[3].pBufferInfo = &bvhInfo;

    writes[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[4].dstSet = _descriptorSet;
    writes[4].dstBinding = 4;
    writes[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[4].descriptorCount = 1;
    writes[4].pBufferInfo = &materialInfo;

    // Environment cubemap (binding 5)
    VkDescriptorImageInfo cubemapInfo{};
    if (_envCubemapView != VK_NULL_HANDLE && _envCubemapSampler != VK_NULL_HANDLE) {
        cubemapInfo.imageView = _envCubemapView;
        cubemapInfo.sampler = _envCubemapSampler;
        cubemapInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        writes[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[5].dstSet = _descriptorSet;
        writes[5].dstBinding = 5;
        writes[5].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[5].descriptorCount = 1;
        writes[5].pImageInfo = &cubemapInfo;

        writeCount = 6;
    }

    vkUpdateDescriptorSets(_engine->_device, writeCount, writes, 0, nullptr);
}

void PathTracer::setDrawImage(VkImageView drawImageView) {
    if (_drawImageView != drawImageView) {
        _drawImageView = drawImageView;
        if (_imagesInitialized && _descriptorSet != VK_NULL_HANDLE) {
            updateDescriptors();
        }
    }
}

void PathTracer::notifySceneChanged() {
    _needsBVHRebuild = true;
}

void PathTracer::processPendingRebuild() {
    if (_needsBVHRebuild) {
        _needsBVHRebuild = false;
        buildBVH();
    }
}

void PathTracer::setEnvironmentCubemap(VkImageView cubemapView, VkSampler cubemapSampler) {
    _envCubemapView = cubemapView;
    _envCubemapSampler = cubemapSampler;

    // Re-update descriptors if images are already initialized
    if (_imagesInitialized && _descriptorSet != VK_NULL_HANDLE) {
        updateDescriptors();
        resetAccumulation();
    }
}

void PathTracer::render(VkCommandBuffer cmd) {
    // Ensure accumulation image is created (lazy initialization)
    ensureImagesReady();

    // Skip if images aren't ready or draw image not set
    if (!_imagesInitialized || _drawImageView == VK_NULL_HANDLE) {
        return;
    }

    // Skip rendering if BVH rebuild is pending (will be done outside command recording)
    if (_needsBVHRebuild) {
        return;
    }

    // Skip if no geometry loaded
    if (stats.triangleCount == 0) {
        return;
    }

    // Check if camera moved - reset accumulation
    glm::mat4 currentView = _engine->sceneData.view;
    if (currentView != _lastViewMatrix) {
        resetAccumulation();
        _lastViewMatrix = currentView;
    }

    // Check if max accumulation reached - switch to display-only mode
    bool displayOnly = stats.accumulatedFrames >= static_cast<uint32_t>(settings.maxAccumulatedFrames);

    // Prepare push constants
    PathTracePushConstants pc{};
    pc.invView = glm::inverse(_engine->sceneData.view);
    pc.invProj = glm::inverse(_engine->sceneData.proj);
    pc.cameraPos = _engine->sceneData.cameraPosition;
    pc.sunDirection = _engine->sceneData.sunlightDirection;
    pc.sunColor = _engine->sceneData.sunlightColor;
    pc.skyColor = glm::vec4(settings.skyColor, settings.skyIntensity);
    pc.frameIndex = _frameIndex++;
    pc.accumulatedFrames = stats.accumulatedFrames;
    pc.triangleCount = stats.triangleCount;
    pc.maxBounces = settings.maxBounces;
    // samplesPerPixel=0 tells shader to use display-only mode (read from accumulation, don't trace)
    pc.samplesPerPixel = displayOnly ? 0 : settings.samplesPerPixel;
    pc.enableNEE = settings.enableNEE ? 1 : 0;
    pc.enableRR = settings.enableRussianRoulette ? 1 : 0;
    pc.rrDepth = settings.russianRouletteDepth;
    pc.useCubemap = (_envCubemapView != VK_NULL_HANDLE && _envCubemapSampler != VK_NULL_HANDLE) ? 1 : 0;

    // Accumulation image barrier: MUST preserve content for temporal accumulation
    VkImageMemoryBarrier accumBarrier{};
    accumBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    accumBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    accumBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    accumBarrier.image = _accumulationImage.image;
    accumBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    accumBarrier.subresourceRange.baseMipLevel = 0;
    accumBarrier.subresourceRange.levelCount = 1;
    accumBarrier.subresourceRange.baseArrayLayer = 0;
    accumBarrier.subresourceRange.layerCount = 1;

    if (stats.accumulatedFrames == 0) {
        accumBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        accumBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        accumBarrier.srcAccessMask = 0;
        accumBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    } else {
        accumBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
        accumBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        accumBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        accumBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    }

    VkPipelineStageFlags srcStage = (stats.accumulatedFrames == 0)
        ? VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
        : VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    vkCmdPipelineBarrier(cmd, srcStage,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &accumBarrier);

    // Bind and dispatch
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _pathTracePipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _pipelineLayout, 0, 1, &_descriptorSet, 0, nullptr);
    vkCmdPushConstants(cmd, _pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(PathTracePushConstants), &pc);

    uint32_t groupsX = (_engine->_drawExtent.width + 7) / 8;
    uint32_t groupsY = (_engine->_drawExtent.height + 7) / 8;
    vkCmdDispatch(cmd, groupsX, groupsY, 1);

    if (!displayOnly) {
        stats.accumulatedFrames++;
    }
}

void PathTracer::resetAccumulation() {
    stats.accumulatedFrames = 0;
    _frameIndex = 0;
}

} // namespace Yalaz::Renderer
