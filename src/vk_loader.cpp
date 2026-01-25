////#define STB_IMAGE_IMPLEMENTATION
//#include "stb_image.h"
//#include <iostream>
//#include <vk_loader.h>
//
//#include "vk_engine.h"
//#include "vk_initializers.h"
//#include "vk_types.h"
//#include <glm/gtx/quaternion.hpp>
//
//#include <fastgltf/glm_element_traits.hpp>
//#include <fastgltf/parser.hpp>
//#include <fastgltf/tools.hpp>
//#include <fastgltf/util.hpp>
//
//
//
//
//
//
//
////> loadimg
//std::optional<AllocatedImage> load_image(VulkanEngine * engine, fastgltf::Asset & asset, fastgltf::Image & image)
//{
//    AllocatedImage newImage{};
//
//    int width, height, nrChannels;
//
//    std::visit(
//        fastgltf::visitor{
//            [](auto& arg) {},
//            [&](fastgltf::sources::URI& filePath) {
//                assert(filePath.fileByteOffset == 0); // We don't support offsets with stbi.
//                assert(filePath.uri.isLocalPath()); // We're only capable of loading
//                // local files.
//
//const std::string path(filePath.uri.path().begin(),
//    filePath.uri.path().end()); // Thanks C++.
//unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 4);
//if (data) {
//    VkExtent3D imagesize;
//    imagesize.width = width;
//    imagesize.height = height;
//    imagesize.depth = 1;
//
//    newImage = engine->create_image(data, imagesize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT,false);
//
//    stbi_image_free(data);
//}
//},
//[&](fastgltf::sources::Vector& vector) {
//    unsigned char* data = stbi_load_from_memory(vector.bytes.data(), static_cast<int>(vector.bytes.size()),
//        &width, &height, &nrChannels, 4);
//    if (data) {
//        VkExtent3D imagesize;
//        imagesize.width = width;
//        imagesize.height = height;
//        imagesize.depth = 1;
//
//        newImage = engine->create_image(data, imagesize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT,false);
//
//        stbi_image_free(data);
//    }
//},
//[&](fastgltf::sources::BufferView& view) {
//    auto& bufferView = asset.bufferViews[view.bufferViewIndex];
//    auto& buffer = asset.buffers[bufferView.bufferIndex];
//
//    std::visit(fastgltf::visitor { // We only care about VectorWithMime here, because we
//        // specify LoadExternalBuffers, meaning all buffers
//        // are already loaded into a vector.
//[](auto& arg) {},
//[&](fastgltf::sources::Vector& vector) {
//    unsigned char* data = stbi_load_from_memory(vector.bytes.data() + bufferView.byteOffset,
//        static_cast<int>(bufferView.byteLength),
//        &width, &height, &nrChannels, 4);
//    if (data) {
//        VkExtent3D imagesize;
//        imagesize.width = width;
//        imagesize.height = height;
//        imagesize.depth = 1;
//
//        newImage = engine->create_image(data, imagesize, VK_FORMAT_R8G8B8A8_UNORM,
//            VK_IMAGE_USAGE_SAMPLED_BIT,false);
//
//        stbi_image_free(data);
//    }
//} },
//buffer.data);
//},
//        },
//        image.data);
//
//    // if any of the attempts to load the data failed, we havent written the image
//    // so handle is null
//    if (newImage.image == VK_NULL_HANDLE) {
//        return {};
//    }
//    else {
//        return newImage;
//    }
//}
////< loadimg
////> filters
//VkFilter extract_filter(fastgltf::Filter filter)
//{
//    switch (filter) {
//        // nearest samplers
//    case fastgltf::Filter::Nearest:
//    case fastgltf::Filter::NearestMipMapNearest:
//    case fastgltf::Filter::NearestMipMapLinear:
//        return VK_FILTER_NEAREST;
//
//        // linear samplers
//    case fastgltf::Filter::Linear:
//    case fastgltf::Filter::LinearMipMapNearest:
//    case fastgltf::Filter::LinearMipMapLinear:
//    default:
//        return VK_FILTER_LINEAR;
//    }
//}
//
//VkSamplerMipmapMode extract_mipmap_mode(fastgltf::Filter filter)
//{
//    switch (filter) {
//    case fastgltf::Filter::NearestMipMapNearest:
//    case fastgltf::Filter::LinearMipMapNearest:
//        return VK_SAMPLER_MIPMAP_MODE_NEAREST;
//
//    case fastgltf::Filter::NearestMipMapLinear:
//    case fastgltf::Filter::LinearMipMapLinear:
//    default:
//        return VK_SAMPLER_MIPMAP_MODE_LINEAR;
//    }
//}
////< filters
//
//std::optional<std::shared_ptr<LoadedGLTF>> loadGltf(VulkanEngine* engine, std::string_view filePath)
//{
//    //> load_1
//    fmt::print("Loading GLTF: {}", filePath);
//
//    std::shared_ptr<LoadedGLTF> scene = std::make_shared<LoadedGLTF>();
//    scene->creator = engine;
//    LoadedGLTF& file = *scene.get();
//
//    fastgltf::Parser parser{};
//
//    constexpr auto gltfOptions = fastgltf::Options::DontRequireValidAssetMember | fastgltf::Options::AllowDouble | fastgltf::Options::LoadGLBBuffers | fastgltf::Options::LoadExternalBuffers;
//    // fastgltf::Options::LoadExternalImages;
//
//    fastgltf::GltfDataBuffer data;
//    data.loadFromFile(filePath);
//
//    fastgltf::Asset gltf;
//
//    std::filesystem::path path = filePath;
//
//    auto type = fastgltf::determineGltfFileType(&data);
//    if (type == fastgltf::GltfType::glTF) {
//        auto load = parser.loadGLTF(&data, path.parent_path(), gltfOptions);
//        if (load) {
//            gltf = std::move(load.get());
//        }
//        else {
//            std::cerr << "Failed to load glTF: " << fastgltf::to_underlying(load.error()) << std::endl;
//            return {};
//        }
//    }
//    else if (type == fastgltf::GltfType::GLB) {
//        auto load = parser.loadBinaryGLTF(&data, path.parent_path(), gltfOptions);
//        if (load) {
//            gltf = std::move(load.get());
//        }
//        else {
//            std::cerr << "Failed to load glTF: " << fastgltf::to_underlying(load.error()) << std::endl;
//            return {};
//        }
//    }
//    else {
//        std::cerr << "Failed to determine glTF container" << std::endl;
//        return {};
//    }
//    //< load_1
//    //> load_2
//        // we can stimate the descriptors we will need accurately
//    std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> sizes = { { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3 },
//        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 },
//        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 } };
//
//    file.descriptorPool.init(engine->_device, gltf.materials.size(), sizes);
//    //< load_2
//    //> load_samplers
//
//        // load samplers
//    for (fastgltf::Sampler& sampler : gltf.samplers) {
//
//        VkSamplerCreateInfo sampl = { .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO, .pNext = nullptr };
//        sampl.maxLod = VK_LOD_CLAMP_NONE;
//        sampl.minLod = 0;
//
//        sampl.magFilter = extract_filter(sampler.magFilter.value_or(fastgltf::Filter::Nearest));
//        sampl.minFilter = extract_filter(sampler.minFilter.value_or(fastgltf::Filter::Nearest));
//
//        sampl.mipmapMode = extract_mipmap_mode(sampler.minFilter.value_or(fastgltf::Filter::Nearest));
//
//        VkSampler newSampler;
//        vkCreateSampler(engine->_device, &sampl, nullptr, &newSampler);
//
//        file.samplers.push_back(newSampler);
//    }
//    //< load_samplers
//    //> load_arrays
//        // temporal arrays for all the objects to use while creating the GLTF data
//    std::vector<std::shared_ptr<MeshAsset>> meshes;
//    std::vector<std::shared_ptr<Node>> nodes;
//    std::vector<AllocatedImage> images;
//    std::vector<std::shared_ptr<GLTFMaterial>> materials;
//    //< load_arrays
//
//        // load all textures
//    for (fastgltf::Image& image : gltf.images) {
//        std::optional<AllocatedImage> img = load_image(engine, gltf, image);
//
//        if (img.has_value()) {
//            images.push_back(*img);
//            file.images[image.name.c_str()] = *img;
//        }
//        else {
//            // we failed to load, so lets give the slot a default white texture to not
//            // completely break loading
//            images.push_back(engine->_errorCheckerboardImage);
//            std::cout << "gltf failed to load texture " << image.name << std::endl;
//        }
//    }
//
//    //> load_buffer
//        // create buffer to hold the material data
//    file.materialDataBuffer = engine->create_buffer(sizeof(GLTFMetallic_Roughness::MaterialConstants) * gltf.materials.size(),
//        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
//    int data_index = 0;
//    GLTFMetallic_Roughness::MaterialConstants* sceneMaterialConstants = (GLTFMetallic_Roughness::MaterialConstants*)file.materialDataBuffer.info.pMappedData;
//    //< load_buffer
//        //
//    //> load_material
//    for (fastgltf::Material& mat : gltf.materials) {
//        std::shared_ptr<GLTFMaterial> newMat = std::make_shared<GLTFMaterial>();
//        materials.push_back(newMat);
//        file.materials[mat.name.c_str()] = newMat;
//
//        GLTFMetallic_Roughness::MaterialConstants constants;
//        constants.colorFactors.x = mat.pbrData.baseColorFactor[0];
//        constants.colorFactors.y = mat.pbrData.baseColorFactor[1];
//        constants.colorFactors.z = mat.pbrData.baseColorFactor[2];
//        constants.colorFactors.w = mat.pbrData.baseColorFactor[3];
//
//        constants.metal_rough_factors.x = mat.pbrData.metallicFactor;
//        constants.metal_rough_factors.y = mat.pbrData.roughnessFactor;
//        // write material parameters to buffer
//        sceneMaterialConstants[data_index] = constants;
//
//        MaterialPass passType = MaterialPass::MainColor;
//        if (mat.alphaMode == fastgltf::AlphaMode::Blend) {
//            passType = MaterialPass::Transparent;
//        }
//
//        GLTFMetallic_Roughness::MaterialResources materialResources;
//        // default the material textures
//        materialResources.colorImage = engine->_whiteImage;
//        materialResources.colorSampler = engine->_defaultSamplerLinear;
//        materialResources.metalRoughImage = engine->_whiteImage;
//        materialResources.metalRoughSampler = engine->_defaultSamplerLinear;
//
//        // set the uniform buffer for the material data
//        materialResources.dataBuffer = file.materialDataBuffer.buffer;
//        materialResources.dataBufferOffset = data_index * sizeof(GLTFMetallic_Roughness::MaterialConstants);
//        // grab textures from gltf file
//        if (mat.pbrData.baseColorTexture.has_value()) {
//            size_t img = gltf.textures[mat.pbrData.baseColorTexture.value().textureIndex].imageIndex.value();
//            size_t sampler = gltf.textures[mat.pbrData.baseColorTexture.value().textureIndex].samplerIndex.value();
//
//            materialResources.colorImage = images[img];
//            materialResources.colorSampler = file.samplers[sampler];
//        }
//        // build material
//        newMat->data = engine->metalRoughMaterial.write_material(engine->_device, passType, materialResources, file.descriptorPool);
//
//        data_index++;
//    }
//    //< load_material
//
//        // use the same vectors for all meshes so that the memory doesnt reallocate as
//        // often
//    std::vector<uint32_t> indices;
//    std::vector<Vertex> vertices;
//
//    for (fastgltf::Mesh& mesh : gltf.meshes) {
//        std::shared_ptr<MeshAsset> newmesh = std::make_shared<MeshAsset>();
//        meshes.push_back(newmesh);
//        file.meshes[mesh.name.c_str()] = newmesh;
//        newmesh->name = mesh.name;
//
//        // clear the mesh arrays each mesh, we dont want to merge them by error
//        indices.clear();
//        vertices.clear();
//
//        for (auto&& p : mesh.primitives) {
//            GeoSurface newSurface;
//            newSurface.startIndex = (uint32_t)indices.size();
//            newSurface.count = (uint32_t)gltf.accessors[p.indicesAccessor.value()].count;
//
//            size_t initial_vtx = vertices.size();
//
//            // load indexes
//            {
//                fastgltf::Accessor& indexaccessor = gltf.accessors[p.indicesAccessor.value()];
//                indices.reserve(indices.size() + indexaccessor.count);
//
//                fastgltf::iterateAccessor<std::uint32_t>(gltf, indexaccessor,
//                    [&](std::uint32_t idx) {
//                        indices.push_back(idx + initial_vtx);
//                    });
//            }
//
//            // load vertex positions
//            {
//                fastgltf::Accessor& posAccessor = gltf.accessors[p.findAttribute("POSITION")->second];
//                vertices.resize(vertices.size() + posAccessor.count);
//
//                fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, posAccessor,
//                    [&](glm::vec3 v, size_t index) {
//                        Vertex newvtx;
//                        newvtx.position = v;
//                        newvtx.normal = { 1, 0, 0 };
//                        newvtx.color = glm::vec4{ 1.f };
//                        newvtx.uv_x = 0;
//                        newvtx.uv_y = 0;
//                        vertices[initial_vtx + index] = newvtx;
//                    });
//            }
//
//            // load vertex normals
//            auto normals = p.findAttribute("NORMAL");
//            if (normals != p.attributes.end()) {
//
//                fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, gltf.accessors[(*normals).second],
//                    [&](glm::vec3 v, size_t index) {
//                        vertices[initial_vtx + index].normal = v;
//                    });
//            }
//
//            // load UVs
//            auto uv = p.findAttribute("TEXCOORD_0");
//            if (uv != p.attributes.end()) {
//
//                fastgltf::iterateAccessorWithIndex<glm::vec2>(gltf, gltf.accessors[(*uv).second],
//                    [&](glm::vec2 v, size_t index) {
//                        vertices[initial_vtx + index].uv_x = v.x;
//                        vertices[initial_vtx + index].uv_y = v.y;
//                    });
//            }
//
//            // load vertex colors
//            auto colors = p.findAttribute("COLOR_0");
//            if (colors != p.attributes.end()) {
//
//                fastgltf::iterateAccessorWithIndex<glm::vec4>(gltf, gltf.accessors[(*colors).second],
//                    [&](glm::vec4 v, size_t index) {
//                        vertices[initial_vtx + index].color = v;
//                    });
//            }
//
//            if (p.materialIndex.has_value()) {
//                newSurface.material = materials[p.materialIndex.value()];
//            }
//            else {
//                newSurface.material = materials[0];
//            }
//
//            glm::vec3 minpos = vertices[initial_vtx].position;
//            glm::vec3 maxpos = vertices[initial_vtx].position;
//            for (int i = initial_vtx; i < vertices.size(); i++) {
//                minpos = glm::min(minpos, vertices[i].position);
//                maxpos = glm::max(maxpos, vertices[i].position);
//            }
//
//            newSurface.bounds.origin = (maxpos + minpos) / 2.f;
//            newSurface.bounds.extents = (maxpos - minpos) / 2.f;
//            newSurface.bounds.sphereRadius = glm::length(newSurface.bounds.extents);
//            newmesh->surfaces.push_back(newSurface);
//        }
//
//        newmesh->meshBuffers = engine->uploadMesh(indices, vertices);
//    }
//    //> load_nodes
//        // load all nodes and their meshes
//    for (fastgltf::Node& node : gltf.nodes) {
//        std::shared_ptr<Node> newNode;
//
//        // find if the node has a mesh, and if it does hook it to the mesh pointer and allocate it with the meshnode class
//        if (node.meshIndex.has_value()) {
//            newNode = std::make_shared<MeshNode>();
//            static_cast<MeshNode*>(newNode.get())->mesh = meshes[*node.meshIndex];
//        }
//        else {
//            newNode = std::make_shared<Node>();
//        }
//
//        nodes.push_back(newNode);
//        file.nodes[node.name.c_str()];
//
//        std::visit(fastgltf::visitor{ [&](fastgltf::Node::TransformMatrix matrix) {
//                                          memcpy(&newNode->localTransform, matrix.data(), sizeof(matrix));
//                                      },
//                       [&](fastgltf::Node::TRS transform) {
//                           glm::vec3 tl(transform.translation[0], transform.translation[1],
//                               transform.translation[2]);
//                           glm::quat rot(transform.rotation[3], transform.rotation[0], transform.rotation[1],
//                               transform.rotation[2]);
//                           glm::vec3 sc(transform.scale[0], transform.scale[1], transform.scale[2]);
//
//                           glm::mat4 tm = glm::translate(glm::mat4(1.f), tl);
//                           glm::mat4 rm = glm::toMat4(rot);
//                           glm::mat4 sm = glm::scale(glm::mat4(1.f), sc);
//
//                           newNode->localTransform = tm * rm * sm;
//                       } },
//            node.transform);
//    }
//    //< load_nodes
//    //> load_graph
//        // run loop again to setup transform hierarchy
//    for (int i = 0; i < gltf.nodes.size(); i++) {
//        fastgltf::Node& node = gltf.nodes[i];
//        std::shared_ptr<Node>& sceneNode = nodes[i];
//
//        for (auto& c : node.children) {
//            sceneNode->children.push_back(nodes[c]);
//            nodes[c]->parent = sceneNode;
//        }
//    }
//
//    // find the top nodes, with no parents
//    for (auto& node : nodes) {
//        if (node->parent.lock() == nullptr) {
//            file.topNodes.push_back(node);
//            node->refreshTransform(glm::mat4{ 1.f });
//        }
//    }
//    return scene;
//    //< load_graph
//}
//
//void LoadedGLTF::Draw(const glm::mat4& topMatrix, DrawContext& ctx)
//{
//    // create renderables from the scenenodes
//    for (auto& n : topNodes) {
//        n->Draw(topMatrix, ctx);
//    }
//}
//
//void LoadedGLTF::clearAll()
//{
//    VkDevice dv = creator->_device;
//
//    for (auto& [k, v] : meshes) {
//
//        creator->destroy_buffer(v->meshBuffers.indexBuffer);
//        creator->destroy_buffer(v->meshBuffers.vertexBuffer);
//    }
//
//    for (auto& [k, v] : images) {
//
//        if (v.image == creator->_errorCheckerboardImage.image) {
//            // dont destroy the default images
//            continue;
//        }
//        creator->destroy_image(v);
//    }
//
//    for (auto& sampler : samplers) {
//        vkDestroySampler(dv, sampler, nullptr);
//    }
//
//    auto materialBuffer = materialDataBuffer;
//    auto samplersToDestroy = samplers;
//
//    descriptorPool.destroy_pools(dv);
//
//    creator->destroy_buffer(materialBuffer);
//}

//#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <iostream>
#include <vk_loader.h>
#include <fstream>

#include "vk_engine.h"
#include "vk_initializers.h"
#include "vk_types.h"
#include <glm/gtx/quaternion.hpp>

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/parser.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/util.hpp>

#include "tiny_obj_loader.h"
//> loadimg
//std::optional<AllocatedImage> load_image(VulkanEngine* engine, fastgltf::Asset& asset, fastgltf::Image& image)
std::optional<AllocatedImage> load_image(VulkanEngine* engine, fastgltf::Asset& asset, fastgltf::Image& image, const std::filesystem::path& basePath)

{
    AllocatedImage newImage{};

    int width, height, nrChannels;

    std::visit(
        fastgltf::visitor{
            [](auto& arg) {},
            [&](fastgltf::sources::URI& filePath) {
                assert(filePath.fileByteOffset == 0); // We don't support offsets with stbi.
                assert(filePath.uri.isLocalPath()); // We're only capable of loading
                // local files.

//const std::string path(filePath.uri.path().begin(),
//    filePath.uri.path().end()); // Thanks C++.
                //const std::string basePath = "C:/YalazEngine/Yalaz-Engine/assets/";
                //const std::string path = basePath + std::string(filePath.uri.path().begin(), filePath.uri.path().end());
                std::filesystem::path texPath = basePath / std::string(filePath.uri.path().begin(), filePath.uri.path().end());
                const std::string path = texPath.string();

                fmt::print("Loading texture from file: {}\n", path);

                // Dosya yolunu kontrol et
                std::ifstream file(path);
                if (!file.good()) {
                    fmt::print("File does not exist or cannot be accessed: {}\n", path);
                    return;
                }

                unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 4);
                if (data) {
                    fmt::print("Loaded texture: {} ({}x{}, {} channels)\n", path, width, height, nrChannels);
                    VkExtent3D imagesize;
                    imagesize.width = width;
                    imagesize.height = height;
                    imagesize.depth = 1;

                    newImage = engine->create_image(data, imagesize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, false);

                    stbi_image_free(data);
                }
                else {
                    fmt::print("Failed to load texture from file: {}\n", path);
                    newImage = engine->_whiteImage; // Alternatif doku kullan
                }
        },
            [&](fastgltf::sources::Vector& vector) {
                fmt::print("Loading texture from memory vector\n");
                unsigned char* data = stbi_load_from_memory(vector.bytes.data(), static_cast<int>(vector.bytes.size()), &width, &height, &nrChannels, 4);
                if (data) {
                    fmt::print("Loaded texture from memory vector ({}x{}, {} channels)\n", width, height, nrChannels);
                    VkExtent3D imagesize;
                    imagesize.width = width;
                    imagesize.height = height;
                    imagesize.depth = 1;

                    newImage = engine->create_image(data, imagesize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, false);

                    stbi_image_free(data);
                }
 else {
  fmt::print("Failed to load texture from memory vector\n");
}
},
 [&](fastgltf::sources::BufferView& view) {
                auto& bufferView = asset.bufferViews[view.bufferViewIndex];
                auto& buffer = asset.buffers[bufferView.bufferIndex];

                std::visit(fastgltf::visitor{
                    [](auto& arg) {},
                    [&](fastgltf::sources::Vector& vector) {
                        fmt::print("Loading texture from buffer view\n");
                        unsigned char* data = stbi_load_from_memory(vector.bytes.data() + bufferView.byteOffset, static_cast<int>(bufferView.byteLength), &width, &height, &nrChannels, 4);
                        if (data) {
                            fmt::print("Loaded texture from buffer view ({}x{}, {} channels)\n", width, height, nrChannels);
                            VkExtent3D imagesize;
                            imagesize.width = width;
                            imagesize.height = height;
                            imagesize.depth = 1;

                            newImage = engine->create_image(data, imagesize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, false);

                            stbi_image_free(data);
                        }
 else {
  fmt::print("Failed to load texture from buffer view\n");
}
}
}, buffer.data);
}
        },
        image.data);

    // if any of the attempts to load the data failed, we havent written the image
    // so handle is null
    if (newImage.image == VK_NULL_HANDLE) {
        fmt::print("Failed to create Vulkan image\n");
        return {};
    }
    else {
        fmt::print("Successfully created Vulkan image\n");
        return newImage;
    }
}
//< loadimg
//> filters
VkFilter extract_filter(fastgltf::Filter filter)
{
    switch (filter) {
        // nearest samplers
    case fastgltf::Filter::Nearest:
    case fastgltf::Filter::NearestMipMapNearest:
    case fastgltf::Filter::NearestMipMapLinear:
        return VK_FILTER_NEAREST;

        // linear samplers
    case fastgltf::Filter::Linear:
    case fastgltf::Filter::LinearMipMapNearest:
    case fastgltf::Filter::LinearMipMapLinear:
    default:
        return VK_FILTER_LINEAR;
    }
}

VkSamplerMipmapMode extract_mipmap_mode(fastgltf::Filter filter)
{
    switch (filter) {
    case fastgltf::Filter::NearestMipMapNearest:
    case fastgltf::Filter::LinearMipMapNearest:
        return VK_SAMPLER_MIPMAP_MODE_NEAREST;

    case fastgltf::Filter::NearestMipMapLinear:
    case fastgltf::Filter::LinearMipMapLinear:
    default:
        return VK_SAMPLER_MIPMAP_MODE_LINEAR;
    }
}
//< filters

std::optional<std::shared_ptr<LoadedGLTF>> loadGltf(VulkanEngine* engine, std::string_view filePath)
{
    //> load_1
    fmt::print("Loading GLTF: {}", filePath);

    std::shared_ptr<LoadedGLTF> scene = std::make_shared<LoadedGLTF>();
    scene->creator = engine;
    LoadedGLTF& file = *scene.get();

    fastgltf::Parser parser{};

    constexpr auto gltfOptions = fastgltf::Options::DontRequireValidAssetMember | fastgltf::Options::AllowDouble | fastgltf::Options::LoadGLBBuffers | fastgltf::Options::LoadExternalBuffers;
    // fastgltf::Options::LoadExternalImages;

    fastgltf::GltfDataBuffer data;
    data.loadFromFile(filePath);

    fastgltf::Asset gltf;

    std::filesystem::path path = filePath;

    auto type = fastgltf::determineGltfFileType(&data);
    if (type == fastgltf::GltfType::glTF) {
        auto load = parser.loadGLTF(&data, path.parent_path(), gltfOptions);
        if (load) {
            gltf = std::move(load.get());
        }
        else {
            std::cerr << "Failed to load glTF: " << fastgltf::to_underlying(load.error()) << std::endl;
            return {};
        }
    }
    else if (type == fastgltf::GltfType::GLB) {
        auto load = parser.loadBinaryGLTF(&data, path.parent_path(), gltfOptions);
        if (load) {
            gltf = std::move(load.get());
        }
        else {
            std::cerr << "Failed to load glTF: " << fastgltf::to_underlying(load.error()) << std::endl;
            return {};
        }
    }
    else {
        std::cerr << "Failed to determine glTF container" << std::endl;
        return {};
    }
    //< load_1
    //> load_2
        // we can stimate the descriptors we will need accurately
    std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> sizes = { { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 } };

    file.descriptorPool.init(engine->_device, gltf.materials.size(), sizes);
    //< load_2
    //> load_samplers

        // load samplers
    for (fastgltf::Sampler& sampler : gltf.samplers) {

        VkSamplerCreateInfo sampl = { .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO, .pNext = nullptr };
        sampl.maxLod = VK_LOD_CLAMP_NONE;
        sampl.minLod = 0;

        sampl.magFilter = extract_filter(sampler.magFilter.value_or(fastgltf::Filter::Nearest));
        sampl.minFilter = extract_filter(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

        sampl.mipmapMode = extract_mipmap_mode(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

        VkSampler newSampler;
        vkCreateSampler(engine->_device, &sampl, nullptr, &newSampler);

        file.samplers.push_back(newSampler);
    }
    //< load_samplers
    //> load_arrays
        // temporal arrays for all the objects to use while creating the GLTF data
    std::vector<std::shared_ptr<MeshAsset>> meshes;
    std::vector<std::shared_ptr<Node>> nodes;
    std::vector<AllocatedImage> images;
    std::vector<std::shared_ptr<GLTFMaterial>> materials;
    //< load_arrays

        // load all textures
// load all textures
    //for (fastgltf::Image& image : gltf.images) {
    //    fmt::print("Attempting to load texture: {}\n", image.name);
    //    std::optional<AllocatedImage> img = load_image(engine, gltf, image);

    //    if (img.has_value()) {
    //        images.push_back(*img);
    //        file.images[image.name.c_str()] = *img;
    //        fmt::print("Successfully loaded texture: {}\n", image.name);
    //    }
    //    else {
    //        // we failed to load, so lets give the slot a default white texture to not
    //        // completely break loading
    //        images.push_back(engine->_errorCheckerboardImage);
    //        fmt::print("gltf failed to load texture {}\n", image.name);
    //    }
    //}


    std::filesystem::path basePath = path.parent_path();
    for (fastgltf::Image& image : gltf.images) {
        fmt::print("Attempting to load texture: {}\n", image.name);
        std::optional<AllocatedImage> img = load_image(engine, gltf, image, basePath);

        if (img.has_value()) {
            images.push_back(*img);
            file.images[image.name.c_str()] = *img;
            fmt::print("Successfully loaded texture: {}\n", image.name);
        }
        else {
            // we failed to load, so lets give the slot a default white texture to not
            // completely break loading
            images.push_back(engine->_errorCheckerboardImage);
            fmt::print("gltf failed to load texture {}\n", image.name);
        }
    }




    //> load_buffer
        // create buffer to hold the material data
    file.materialDataBuffer = engine->create_buffer(sizeof(GLTFMetallic_Roughness::MaterialConstants) * gltf.materials.size(),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
    int data_index = 0;
    GLTFMetallic_Roughness::MaterialConstants* sceneMaterialConstants = (GLTFMetallic_Roughness::MaterialConstants*)file.materialDataBuffer.info.pMappedData;
    //< load_buffer
        //
    //> load_material
    for (fastgltf::Material& mat : gltf.materials) {
        std::shared_ptr<GLTFMaterial> newMat = std::make_shared<GLTFMaterial>();
        materials.push_back(newMat);
        file.materials[mat.name.c_str()] = newMat;

        GLTFMetallic_Roughness::MaterialConstants constants;
        constants.colorFactors.x = mat.pbrData.baseColorFactor[0];
        constants.colorFactors.y = mat.pbrData.baseColorFactor[1];
        constants.colorFactors.z = mat.pbrData.baseColorFactor[2];
        constants.colorFactors.w = mat.pbrData.baseColorFactor[3];

        constants.metal_rough_factors.x = mat.pbrData.metallicFactor;
        constants.metal_rough_factors.y = mat.pbrData.roughnessFactor;
        // write material parameters to buffer
        sceneMaterialConstants[data_index] = constants;

        MaterialPass passType = MaterialPass::MainColor;
        if (mat.alphaMode == fastgltf::AlphaMode::Blend) {
            passType = MaterialPass::Transparent;
        }

        GLTFMetallic_Roughness::MaterialResources materialResources;
        // default the material textures
        materialResources.colorImage = engine->_whiteImage;
        materialResources.colorSampler = engine->_defaultSamplerLinear;
        materialResources.metalRoughImage = engine->_whiteImage;
        materialResources.metalRoughSampler = engine->_defaultSamplerLinear;

        // set the uniform buffer for the material data
        materialResources.dataBuffer = file.materialDataBuffer.buffer;
        materialResources.dataBufferOffset = data_index * sizeof(GLTFMetallic_Roughness::MaterialConstants);
        // grab textures from gltf file
        if (mat.pbrData.baseColorTexture.has_value()) {
            size_t img = gltf.textures[mat.pbrData.baseColorTexture.value().textureIndex].imageIndex.value();
            size_t sampler = gltf.textures[mat.pbrData.baseColorTexture.value().textureIndex].samplerIndex.value();

            materialResources.colorImage = images[img];
            materialResources.colorSampler = file.samplers[sampler];
        }
        // build material
        newMat->data = engine->metalRoughMaterial.write_material(engine->_device, passType, materialResources, file.descriptorPool);

        data_index++;
    }
    //< load_material

        // use the same vectors for all meshes so that the memory doesnt reallocate as
        // often
    std::vector<uint32_t> indices;
    std::vector<Vertex> vertices;

    for (fastgltf::Mesh& mesh : gltf.meshes) {
        std::shared_ptr<MeshAsset> newmesh = std::make_shared<MeshAsset>();
        meshes.push_back(newmesh);
        file.meshes[mesh.name.c_str()] = newmesh;
        newmesh->name = mesh.name;

        // clear the mesh arrays each mesh, we dont want to merge them by error
        indices.clear();
        vertices.clear();

        for (auto&& p : mesh.primitives) {
            GeoSurface newSurface;
            newSurface.startIndex = (uint32_t)indices.size();
            newSurface.count = (uint32_t)gltf.accessors[p.indicesAccessor.value()].count;

            size_t initial_vtx = vertices.size();

            // load indexes
            {
                fastgltf::Accessor& indexaccessor = gltf.accessors[p.indicesAccessor.value()];
                indices.reserve(indices.size() + indexaccessor.count);

                fastgltf::iterateAccessor<std::uint32_t>(gltf, indexaccessor,
                    [&](std::uint32_t idx) {
                        indices.push_back(idx + initial_vtx);
                    });
            }

            // load vertex positions
            {
                fastgltf::Accessor& posAccessor = gltf.accessors[p.findAttribute("POSITION")->second];
                vertices.resize(vertices.size() + posAccessor.count);

                fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, posAccessor,
                    [&](glm::vec3 v, size_t index) {
                        Vertex newvtx;
                        newvtx.position = v;
                        newvtx.normal = { 1, 0, 0 };
                        newvtx.color = glm::vec4{ 1.f };
                        newvtx.uv_x = 0;
                        newvtx.uv_y = 0;
                        vertices[initial_vtx + index] = newvtx;
                    });
            }

            // load vertex normals
            auto normals = p.findAttribute("NORMAL");
            if (normals != p.attributes.end()) {

                fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, gltf.accessors[(*normals).second],
                    [&](glm::vec3 v, size_t index) {
                        vertices[initial_vtx + index].normal = v;
                    });
            }

            // load UVs
            auto uv = p.findAttribute("TEXCOORD_0");
            if (uv != p.attributes.end()) {

                fastgltf::iterateAccessorWithIndex<glm::vec2>(gltf, gltf.accessors[(*uv).second],
                    [&](glm::vec2 v, size_t index) {
                        vertices[initial_vtx + index].uv_x = v.x;
                        vertices[initial_vtx + index].uv_y = v.y;
                    });
            }

            // load vertex colors
            auto colors = p.findAttribute("COLOR_0");
            if (colors != p.attributes.end()) {

                fastgltf::iterateAccessorWithIndex<glm::vec4>(gltf, gltf.accessors[(*colors).second],
                    [&](glm::vec4 v, size_t index) {
                        vertices[initial_vtx + index].color = v;
                    });
            }

            if (p.materialIndex.has_value()) {
                newSurface.material = materials[p.materialIndex.value()];
            }
            else {
                newSurface.material = materials[0];
            }

            glm::vec3 minpos = vertices[initial_vtx].position;
            glm::vec3 maxpos = vertices[initial_vtx].position;
            for (int i = initial_vtx; i < vertices.size(); i++) {
                minpos = glm::min(minpos, vertices[i].position);
                maxpos = glm::max(maxpos, vertices[i].position);
            }

            newSurface.bounds.origin = (maxpos + minpos) / 2.f;
            newSurface.bounds.extents = (maxpos - minpos) / 2.f;
            newSurface.bounds.sphereRadius = glm::length(newSurface.bounds.extents);
            newmesh->surfaces.push_back(newSurface);
        }

        newmesh->meshBuffers = engine->uploadMesh(indices, vertices);
    }
    //> load_nodes
        // load all nodes and their meshes
    for (fastgltf::Node& node : gltf.nodes) {
        std::shared_ptr<Node> newNode;

        // find if the node has a mesh, and if it does hook it to the mesh pointer and allocate it with the meshnode class
        if (node.meshIndex.has_value()) {
            newNode = std::make_shared<MeshNode>();
            static_cast<MeshNode*>(newNode.get())->mesh = meshes[*node.meshIndex];
        }
        else {
            newNode = std::make_shared<Node>();
        }

        nodes.push_back(newNode);
        file.nodes[node.name.c_str()];

        std::visit(fastgltf::visitor{ [&](fastgltf::Node::TransformMatrix matrix) {
                                          memcpy(&newNode->localTransform, matrix.data(), sizeof(matrix));
                                      },
                       [&](fastgltf::Node::TRS transform) {
                           glm::vec3 tl(transform.translation[0], transform.translation[1],
                               transform.translation[2]);
                           glm::quat rot(transform.rotation[3], transform.rotation[0], transform.rotation[1],
                               transform.rotation[2]);
                           glm::vec3 sc(transform.scale[0], transform.scale[1], transform.scale[2]);

                           glm::mat4 tm = glm::translate(glm::mat4(1.f), tl);
                           glm::mat4 rm = glm::toMat4(rot);
                           glm::mat4 sm = glm::scale(glm::mat4(1.f), sc);

                           newNode->localTransform = tm * rm * sm;
                       } },
            node.transform);
    }
    //< load_nodes
    //> load_graph
        // run loop again to setup transform hierarchy
    for (int i = 0; i < gltf.nodes.size(); i++) {
        fastgltf::Node& node = gltf.nodes[i];
        std::shared_ptr<Node>& sceneNode = nodes[i];

        for (auto& c : node.children) {
            sceneNode->children.push_back(nodes[c]);
            nodes[c]->parent = sceneNode;
        }
    }

    // find the top nodes, with no parents
    for (auto& node : nodes) {
        if (node->parent.lock() == nullptr) {
            file.topNodes.push_back(node);
            node->refreshTransform(glm::mat4{ 1.f });
        }
    }
    return scene;
    //< load_graph
}




std::optional<std::shared_ptr<LoadedGLTF>> loadObj(VulkanEngine* engine, std::string_view filePath) {
    fmt::print("Loading OBJ: {}\n", filePath);

    std::shared_ptr<LoadedGLTF> scene = std::make_shared<LoadedGLTF>();
    scene->creator = engine;
    LoadedGLTF& file = *scene.get();

    std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> sizes = {
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 }
    };
    file.descriptorPool.init(engine->_device, 1, sizes);

    std::vector<std::shared_ptr<MeshAsset>> meshes;
    std::vector<std::shared_ptr<Node>> nodes;
    std::vector<std::shared_ptr<GLTFMaterial>> materials;

    tinyobj::ObjReader reader;
    tinyobj::ObjReaderConfig config;
    config.mtl_search_path = std::filesystem::path(filePath).parent_path().string();

    if (!reader.ParseFromFile(std::string(filePath), config)) {
        std::cerr << "Failed to load OBJ: " << reader.Error() << std::endl;
        return {};
    }
    if (!reader.Warning().empty()) fmt::print("OBJ warning: {}\n", reader.Warning());

    const auto& attrib = reader.GetAttrib();
    const auto& shapes = reader.GetShapes();
    const auto& materialsTiny = reader.GetMaterials();

    if (!materialsTiny.empty()) {
        file.materialDataBuffer = engine->create_buffer(
            sizeof(GLTFMetallic_Roughness::MaterialConstants) * materialsTiny.size(),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
    }

    for (size_t i = 0; i < materialsTiny.size(); i++) {
        const auto& mat = materialsTiny[i];
        std::shared_ptr<GLTFMaterial> newMat = std::make_shared<GLTFMaterial>();
        materials.push_back(newMat);
        file.materials[mat.name.c_str()] = newMat;

        GLTFMetallic_Roughness::MaterialConstants constants{};
        constants.colorFactors = glm::vec4(mat.diffuse[0], mat.diffuse[1], mat.diffuse[2], 1.0f);
        constants.metal_rough_factors = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);

        auto* mapped = reinterpret_cast<GLTFMetallic_Roughness::MaterialConstants*>(file.materialDataBuffer.info.pMappedData);
        mapped[i] = constants;

        GLTFMetallic_Roughness::MaterialResources res;
        res.colorImage = engine->_whiteImage;
        res.colorSampler = engine->_defaultSamplerLinear;
        res.metalRoughImage = engine->_whiteImage;
        res.metalRoughSampler = engine->_defaultSamplerLinear;
        res.dataBuffer = file.materialDataBuffer.buffer;
        res.dataBufferOffset = i * sizeof(GLTFMetallic_Roughness::MaterialConstants);

        if (!mat.diffuse_texname.empty()) {
            std::string texPath = config.mtl_search_path + "/" + mat.diffuse_texname;
            int w, h, ch;
            unsigned char* data = stbi_load(texPath.c_str(), &w, &h, &ch, 4);
            if (data) {
                VkExtent3D size{ (uint32_t)w, (uint32_t)h, 1 };
                AllocatedImage tex = engine->create_image(data, size, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, false);
                stbi_image_free(data);
                res.colorImage = tex;
            }
            else {
                fmt::print("⚠️ Texture load failed: {}\n", texPath);
            }
        }

        newMat->data = engine->metalRoughMaterial.write_material(engine->_device, MaterialPass::MainColor, res, file.descriptorPool);
    }

    std::vector<uint32_t> indices;
    std::vector<Vertex> vertices;

    for (const auto& shape : shapes) {
        std::shared_ptr<MeshAsset> newmesh = std::make_shared<MeshAsset>();
        meshes.push_back(newmesh);
        file.meshes[shape.name.c_str()] = newmesh;
        newmesh->name = shape.name;

        indices.clear();
        vertices.clear();

        size_t index_offset = 0;
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
            int fv = shape.mesh.num_face_vertices[f];
            int matId = shape.mesh.material_ids.size() > f ? shape.mesh.material_ids[f] : -1;
            std::shared_ptr<GLTFMaterial> mat = (matId >= 0 && matId < materials.size()) ? materials[matId] : nullptr;

            for (int v = 0; v < fv; v++) {
                tinyobj::index_t idx = shape.mesh.indices[index_offset + v];
                Vertex vert{};
                vert.position = {
                    attrib.vertices[3 * idx.vertex_index + 0],
                    attrib.vertices[3 * idx.vertex_index + 1],
                    attrib.vertices[3 * idx.vertex_index + 2]
                };
                vert.normal = (idx.normal_index >= 0 && !attrib.normals.empty())
                    ? glm::vec3(
                        attrib.normals[3 * idx.normal_index + 0],
                        attrib.normals[3 * idx.normal_index + 1],
                        attrib.normals[3 * idx.normal_index + 2])
                    : glm::vec3(0, 1, 0);
                if (idx.texcoord_index >= 0 && !attrib.texcoords.empty()) {
                    vert.uv_x = attrib.texcoords[2 * idx.texcoord_index + 0];
                    vert.uv_y = 1.0f - attrib.texcoords[2 * idx.texcoord_index + 1]; // Vulkan uyumu
                }
                else {
                    vert.uv_x = 0; vert.uv_y = 0;
                }
                vert.color = glm::vec4(1.0f);
                vertices.push_back(vert);
                indices.push_back(static_cast<uint32_t>(vertices.size() - 1));
            }

            GeoSurface surface;
            surface.startIndex = indices.size() - fv;
            surface.count = fv;
            surface.material = mat;

            // Bound
            glm::vec3 minpos = vertices[surface.startIndex].position;
            glm::vec3 maxpos = minpos;
            for (uint32_t i = 0; i < surface.count; ++i) {
                minpos = glm::min(minpos, vertices[surface.startIndex + i].position);
                maxpos = glm::max(maxpos, vertices[surface.startIndex + i].position);
            }
            surface.bounds.origin = (maxpos + minpos) / 2.0f;
            surface.bounds.extents = (maxpos - minpos) / 2.0f;
            surface.bounds.sphereRadius = glm::length(surface.bounds.extents);

            newmesh->surfaces.push_back(surface);
        }

        newmesh->meshBuffers = engine->uploadMesh(indices, vertices);
    }

    for (const auto& mesh : meshes) {
        std::shared_ptr<Node> newNode = std::make_shared<MeshNode>();
        static_cast<MeshNode*>(newNode.get())->mesh = mesh;
        nodes.push_back(newNode);
        file.nodes[mesh->name.c_str()] = newNode;
    }

    for (auto& node : nodes) {
        file.topNodes.push_back(node);
        node->refreshTransform(glm::mat4{ 1.f });
    }

    return scene;
}



void LoadedGLTF::Draw(const glm::mat4& topMatrix, DrawContext& ctx)
{
    // create renderables from the scenenodes
    for (auto& n : topNodes) {
        n->Draw(topMatrix, ctx);
    }
}

void LoadedGLTF::clearAll()
{
    VkDevice dv = creator->_device;

    for (auto& [k, v] : meshes) {

        creator->destroy_buffer(v->meshBuffers.indexBuffer);
        creator->destroy_buffer(v->meshBuffers.vertexBuffer);
    }

    for (auto& [k, v] : images) {

        if (v.image == creator->_errorCheckerboardImage.image) {
            // dont destroy the default images
            continue;
        }
        creator->destroy_image(v);
    }

    for (auto& sampler : samplers) {
        
        vkDestroySampler(dv, sampler, nullptr);
    }

    auto materialBuffer = materialDataBuffer;
    auto samplersToDestroy = samplers;

    descriptorPool.destroy_pools(dv);

    creator->destroy_buffer(materialBuffer);
}