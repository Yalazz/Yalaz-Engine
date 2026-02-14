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
#include <algorithm>
#include <cstdlib>
#include <cctype>
#include <limits>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#ifdef _WIN32
#include <windows.h>
#endif

#include "vk_engine.h"
#include "vk_initializers.h"
#include "vk_types.h"
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/parser.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/util.hpp>

#include "tiny_obj_loader.h"
#include "ui/views/ConsoleView.h"

namespace {
std::unordered_map<std::string, std::string> gConvertedObjSourceDir;

std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

std::string shellQuote(const std::string& s) {
    std::string out = "\"";
    for (char c : s) {
        if (c == '"') out += "\\\"";
        else out += c;
    }
    out += "\"";
    return out;
}

#ifndef _WIN32
bool runCommand(const std::string& cmd) {
    int rc = std::system(cmd.c_str());
    return rc == 0;
}
#endif

bool runCommandArgs(const std::string& exe, const std::vector<std::string>& args) {
#ifdef _WIN32
    std::string commandLine = shellQuote(exe);
    for (const auto& arg : args) {
        commandLine += " " + shellQuote(arg);
    }

    STARTUPINFOA si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};

    std::vector<char> mutableCmd(commandLine.begin(), commandLine.end());
    mutableCmd.push_back('\0');

    const BOOL ok = CreateProcessA(
        nullptr,
        mutableCmd.data(),
        nullptr,
        nullptr,
        FALSE,
        CREATE_NO_WINDOW,
        nullptr,
        nullptr,
        &si,
        &pi);
    if (!ok) {
        return false;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exitCode = 1;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return exitCode == 0;
#else
    std::string cmd = shellQuote(exe);
    for (const auto& arg : args) {
        cmd += " " + shellQuote(arg);
    }
    return runCommand(cmd);
#endif
}

std::vector<std::string> getAssimpCandidates() {
    std::vector<std::string> assimpCandidates;
    if (const char* assimpBin = std::getenv("ASSIMP_BIN")) {
        assimpCandidates.emplace_back(assimpBin);
    }
    if (const char* assimpRoot = std::getenv("ASSIMP_ROOT")) {
        const std::filesystem::path root(assimpRoot);
        assimpCandidates.push_back((root / "bin" / "assimp.exe").string());
        assimpCandidates.push_back((root / "assimp.exe").string());
    }

    std::vector<std::string> defaultCandidates = {
        "assimp",
        "assimp.exe",
        "C:\\Program Files\\assimp\\bin\\assimp.exe",
        "C:\\Program Files\\Assimp\\bin\\assimp.exe",
        "C:\\Program Files (x86)\\Assimp\\bin\\assimp.exe",
        "C:\\msys64\\mingw64\\bin\\assimp.exe"
    };
    assimpCandidates.insert(assimpCandidates.end(), defaultCandidates.begin(), defaultCandidates.end());
    return assimpCandidates;
}

bool tryAssimpExport(const std::string& inputAbsPath, const std::filesystem::path& outPath, const char* formatTag) {
    const auto candidates = getAssimpCandidates();
    for (const auto& tool : candidates) {
        const bool hasPathSeparators = (tool.find('\\') != std::string::npos || tool.find('/') != std::string::npos);
        if (hasPathSeparators && !std::filesystem::exists(std::filesystem::path(tool))) {
            continue;
        }
        if (runCommandArgs(tool, { "export", inputAbsPath, outPath.string(), "-f", formatTag })) {
            if (std::filesystem::exists(outPath) && std::filesystem::file_size(outPath) > 0) {
                return true;
            }
        }
    }
    return false;
}

bool convertModelToGltf(const std::filesystem::path& inputPath, std::filesystem::path& outPath) {
    try {
        const auto absIn = std::filesystem::absolute(inputPath).string();
        constexpr uint64_t kImportCacheVersion = 3; // Bumped: fixes for FBX/DAE animation
        const auto writeTime = std::filesystem::last_write_time(inputPath).time_since_epoch().count();
        std::hash<std::string> h;
        const auto hash = h(absIn + "|" + std::to_string(writeTime) + "|" + std::to_string(kImportCacheVersion));

        auto cacheDir = std::filesystem::temp_directory_path() / "yalaz_import_cache";
        std::filesystem::create_directories(cacheDir);

        const std::filesystem::path outGlb = cacheDir / (inputPath.stem().string() + "_" + std::to_string(hash) + ".glb");
        const std::filesystem::path outGltf = cacheDir / (inputPath.stem().string() + "_" + std::to_string(hash) + ".gltf");

        auto gltfScore = [](const std::filesystem::path& p) -> int {
            if (!std::filesystem::exists(p) || std::filesystem::file_size(p) == 0) return -1;

            fastgltf::Parser parser(fastgltf::Extensions::KHR_lights_punctual);
            constexpr auto opts = fastgltf::Options::DontRequireValidAssetMember
                | fastgltf::Options::AllowDouble
                | fastgltf::Options::LoadGLBBuffers
                | fastgltf::Options::LoadExternalBuffers;

            fastgltf::GltfDataBuffer data;
            if (!data.loadFromFile(p.string())) return -1;

            auto type = fastgltf::determineGltfFileType(&data);

            fastgltf::Asset asset;
            if (type == fastgltf::GltfType::glTF) {
                auto res = parser.loadGLTF(&data, p.parent_path(), opts);
                if (!res) return -1;
                asset = std::move(res.get());
            } else {
                auto res = parser.loadBinaryGLTF(&data, p.parent_path(), opts);
                if (!res) return -1;
                asset = std::move(res.get());
            }

            // Strongly prefer files that keep animation + skinning.
            return static_cast<int>(asset.animations.size()) * 100000
                + static_cast<int>(asset.skins.size()) * 10000
                + static_cast<int>(asset.nodes.size()) * 100
                + static_cast<int>(asset.meshes.size());
        };

        auto chooseBestExisting = [&]() -> bool {
            const int glbScore = gltfScore(outGlb);
            const int gltfScoreVal = gltfScore(outGltf);
            if (glbScore < 0 && gltfScoreVal < 0) return false;
            outPath = (gltfScoreVal > glbScore) ? outGltf : outGlb;
            return true;
        };

        if (chooseBestExisting()) {
            return true;
        }

        if (tryAssimpExport(absIn, outGlb, "glb2")) {
            if (chooseBestExisting()) return true;
        }
        if (tryAssimpExport(absIn, outGltf, "gltf2")) {
            if (chooseBestExisting()) return true;
        }

        return false;
    } catch (...) {
        return false;
    }
}

bool convertModelToObj(const std::filesystem::path& inputPath, std::filesystem::path& outPath) {
    try {
        const auto absIn = std::filesystem::absolute(inputPath).string();
        constexpr uint64_t kImportCacheVersion = 2;
        const auto writeTime = std::filesystem::last_write_time(inputPath).time_since_epoch().count();
        std::hash<std::string> h;
        const auto hash = h(absIn + "|" + std::to_string(writeTime) + "|" + std::to_string(kImportCacheVersion));

        auto cacheDir = std::filesystem::temp_directory_path() / "yalaz_import_cache";
        std::filesystem::create_directories(cacheDir);

        const std::filesystem::path outObj = cacheDir / (inputPath.stem().string() + "_" + std::to_string(hash) + ".obj");
        if (std::filesystem::exists(outObj) && std::filesystem::file_size(outObj) > 0) {
            outPath = outObj;
            gConvertedObjSourceDir[outObj.string()] = std::filesystem::absolute(inputPath).parent_path().string();
            return true;
        }

        if (tryAssimpExport(absIn, outObj, "obj")) {
            outPath = outObj;
            gConvertedObjSourceDir[outObj.string()] = std::filesystem::absolute(inputPath).parent_path().string();
            return true;
        }
        return false;
    } catch (...) {
        return false;
    }
}

bool collectObjPoints(const std::string& path, std::vector<glm::vec3>& points) {
    tinyobj::ObjReader reader;
    tinyobj::ObjReaderConfig cfg;
    cfg.mtl_search_path = std::filesystem::path(path).parent_path().string();
    if (!reader.ParseFromFile(path, cfg)) return false;

    const auto& attrib = reader.GetAttrib();
    if (attrib.vertices.empty()) return false;
    points.reserve(attrib.vertices.size() / 3);
    for (size_t i = 0; i + 2 < attrib.vertices.size(); i += 3) {
        points.emplace_back(attrib.vertices[i + 0], attrib.vertices[i + 1], attrib.vertices[i + 2]);
    }
    constexpr size_t kMaxPreviewPoints = 15000;
    if (points.size() > kMaxPreviewPoints) {
        std::vector<glm::vec3> reduced;
        reduced.reserve(kMaxPreviewPoints);
        const float step = static_cast<float>(points.size()) / static_cast<float>(kMaxPreviewPoints);
        for (size_t i = 0; i < kMaxPreviewPoints; ++i) {
            const size_t src = std::min(points.size() - 1, static_cast<size_t>(i * step));
            reduced.push_back(points[src]);
        }
        points.swap(reduced);
    }
    return !points.empty();
}

bool collectGltfPoints(const std::string& path, std::vector<glm::vec3>& points) {
    fastgltf::Parser parser(fastgltf::Extensions::KHR_lights_punctual);
    constexpr auto options = fastgltf::Options::DontRequireValidAssetMember
        | fastgltf::Options::AllowDouble
        | fastgltf::Options::LoadGLBBuffers
        | fastgltf::Options::LoadExternalBuffers;

    fastgltf::GltfDataBuffer data;
    data.loadFromFile(path);

    fastgltf::Asset gltf;
    auto type = fastgltf::determineGltfFileType(&data);
    if (type == fastgltf::GltfType::glTF) {
        auto load = parser.loadGLTF(&data, std::filesystem::path(path).parent_path(), options);
        if (load.error() != fastgltf::Error::None) return false;
        gltf = std::move(load.get());
    } else if (type == fastgltf::GltfType::GLB) {
        auto load = parser.loadBinaryGLTF(&data, std::filesystem::path(path).parent_path(), options);
        if (load.error() != fastgltf::Error::None) return false;
        gltf = std::move(load.get());
    } else {
        return false;
    }

    for (auto& mesh : gltf.meshes) {
        for (auto& prim : mesh.primitives) {
            auto posIt = prim.findAttribute("POSITION");
            if (posIt == prim.attributes.end()) continue;
            auto& posAccessor = gltf.accessors[posIt->second];
            size_t base = points.size();
            points.resize(base + posAccessor.count);
            fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, posAccessor,
                [&](glm::vec3 v, size_t idx) { points[base + idx] = v; });
        }
    }
    constexpr size_t kMaxPreviewPoints = 15000;
    if (points.size() > kMaxPreviewPoints) {
        std::vector<glm::vec3> reduced;
        reduced.reserve(kMaxPreviewPoints);
        const float step = static_cast<float>(points.size()) / static_cast<float>(kMaxPreviewPoints);
        for (size_t i = 0; i < kMaxPreviewPoints; ++i) {
            const size_t src = std::min(points.size() - 1, static_cast<size_t>(i * step));
            reduced.push_back(points[src]);
        }
        points.swap(reduced);
    }
    return !points.empty();
}
} // namespace
//> loadimg
//std::optional<AllocatedImage> load_image(VulkanEngine* engine, fastgltf::Asset& asset, fastgltf::Image& image)

// URL-decode helper for GLTF URI paths (handles %20, %2F, etc.)
static std::string urlDecode(const std::string& encoded) {
    std::string decoded;
    decoded.reserve(encoded.size());
    for (size_t i = 0; i < encoded.size(); ++i) {
        if (encoded[i] == '%' && i + 2 < encoded.size()) {
            int high = 0, low = 0;
            char h = encoded[i + 1], l = encoded[i + 2];
            high = (h >= '0' && h <= '9') ? h - '0' : (h >= 'a' && h <= 'f') ? h - 'a' + 10 : (h >= 'A' && h <= 'F') ? h - 'A' + 10 : -1;
            low  = (l >= '0' && l <= '9') ? l - '0' : (l >= 'a' && l <= 'f') ? l - 'a' + 10 : (l >= 'A' && l <= 'F') ? l - 'A' + 10 : -1;
            if (high >= 0 && low >= 0) {
                decoded += static_cast<char>(high * 16 + low);
                i += 2;
                continue;
            }
        }
        decoded += encoded[i];
    }
    return decoded;
}

std::optional<AllocatedImage> load_image(VulkanEngine* engine, fastgltf::Asset& asset, fastgltf::Image& image, const std::filesystem::path& basePath)

{
    AllocatedImage newImage{};

    int width, height, nrChannels;

    std::visit(
        fastgltf::visitor{
            [](auto& arg) {},
            [&](fastgltf::sources::URI& filePath) {
                assert(filePath.fileByteOffset == 0);
                assert(filePath.uri.isLocalPath());

                // URL-decode the URI path and normalize separators
                std::string rawPath(filePath.uri.path().begin(), filePath.uri.path().end());
                std::string decodedPath = urlDecode(rawPath);

                // Remove leading ./ if present
                if (decodedPath.size() > 2 && decodedPath[0] == '.' && (decodedPath[1] == '/' || decodedPath[1] == '\\')) {
                    decodedPath = decodedPath.substr(2);
                }

                // Build full path
                std::filesystem::path texPath = basePath / decodedPath;
                std::string path = texPath.string();

                // Try the path directly first
                if (!std::filesystem::exists(texPath)) {
                    // Try with normalized separators
                    std::replace(decodedPath.begin(), decodedPath.end(), '/', '\\');
                    texPath = basePath / decodedPath;
                    path = texPath.string();

                    if (!std::filesystem::exists(texPath)) {
                        fmt::print("[Texture] NOT FOUND: {} (base: {})\n", rawPath, basePath.string());
                        // List what's in the directory for debugging
                        auto parentDir = texPath.parent_path();
                        if (std::filesystem::exists(parentDir)) {
                            fmt::print("[Texture] Files in {}:\n", parentDir.string());
                            int count = 0;
                            for (auto& entry : std::filesystem::directory_iterator(parentDir)) {
                                fmt::print("  - {}\n", entry.path().filename().string());
                                if (++count > 10) { fmt::print("  ... (more files)\n"); break; }
                            }
                        }
                        return;
                    }
                }

                unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 4);
                if (data) {
                    VkExtent3D imagesize;
                    imagesize.width = width;
                    imagesize.height = height;
                    imagesize.depth = 1;

                    newImage = engine->create_image(data, imagesize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, true);

                    stbi_image_free(data);
                }
                else {
                    fmt::print("[Texture] stbi_load failed: {}\n", path);
                    newImage = engine->_whiteImage;
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

                    newImage = engine->create_image(data, imagesize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, true);

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

                            newImage = engine->create_image(data, imagesize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, true);

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
    fmt::print("Loading GLTF: {}\n", filePath);

    // Check file exists and get size
    std::filesystem::path path = filePath;
    if (!std::filesystem::exists(path)) {
        fmt::print("ERROR: GLTF file does not exist: {}\n", filePath);
        return {};
    }

    auto fileSize = std::filesystem::file_size(path);
    fmt::print("  File size: {:.2f} MB\n", fileSize / (1024.0 * 1024.0));

    // Warn for very large files
    if (fileSize > 500 * 1024 * 1024) {  // 500 MB
        fmt::print("WARNING: Very large file (>500MB). Loading may take time or fail.\n");
    }

    try {
        std::shared_ptr<LoadedGLTF> scene = std::make_shared<LoadedGLTF>();
        scene->creator = engine;
        LoadedGLTF& file = *scene.get();

        fastgltf::Parser parser(fastgltf::Extensions::KHR_lights_punctual);

        constexpr auto gltfOptions = fastgltf::Options::DontRequireValidAssetMember | fastgltf::Options::AllowDouble | fastgltf::Options::LoadGLBBuffers | fastgltf::Options::LoadExternalBuffers;
        // fastgltf::Options::LoadExternalImages;

        fastgltf::GltfDataBuffer data;
        if (!data.loadFromFile(filePath)) {
            fmt::print("ERROR: Failed to load GLTF data from file\n");
            return {};
        }

        fastgltf::Asset gltf;

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

    // Print scene statistics
    fmt::print("  GLTF Scene Statistics:\n");
    fmt::print("    Meshes: {}\n", gltf.meshes.size());
    fmt::print("    Materials: {}\n", gltf.materials.size());
    fmt::print("    Textures: {}\n", gltf.images.size());
    fmt::print("    Nodes: {}\n", gltf.nodes.size());
    fmt::print("    Samplers: {}\n", gltf.samplers.size());

    // Limit checks
    const size_t MAX_MATERIALS = 512;
    const size_t MAX_MESHES = 1000;

    if (gltf.materials.size() > MAX_MATERIALS) {
        fmt::print("ERROR: Too many materials ({} > {}). Scene may not load correctly.\n",
            gltf.materials.size(), MAX_MATERIALS);
    }

    if (gltf.meshes.size() > MAX_MESHES) {
        fmt::print("WARNING: Large number of meshes ({}). Loading may take time.\n",
            gltf.meshes.size());
    }

    //> load_2
        // we can stimate the descriptors we will need accurately
    std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> sizes = { { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 } };

    // Limit descriptor pool size
    size_t materialCount = std::min(gltf.materials.size(), MAX_MATERIALS);
    file.descriptorPool.init(engine->_device, materialCount, sizes);
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
    // NOTE: Do not push engine default sampler into file.samplers.
    // LoadedGLTF::clearAll destroys all samplers in this vector; including a global
    // engine sampler here would destroy shared state and corrupt subsequent renders.
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

    // Limit textures to prevent resource exhaustion
    const size_t MAX_TEXTURES = 256;
    size_t textureCount = gltf.images.size();

    if (textureCount > MAX_TEXTURES) {
        fmt::print("WARNING: GLTF has {} textures, limiting to {} to prevent crashes\n",
            textureCount, MAX_TEXTURES);
        textureCount = MAX_TEXTURES;
    }

    fmt::print("  Loading {} textures...\n", textureCount);

    size_t loadedTextures = 0;
    for (size_t i = 0; i < textureCount && i < gltf.images.size(); i++) {
        fastgltf::Image& image = gltf.images[i];

        // Progress every 10 textures
        if (textureCount > 10 && i % 10 == 0) {
            fmt::print("    Texture {}/{}: {}\n", i + 1, textureCount, image.name);
        }

        std::optional<AllocatedImage> img = load_image(engine, gltf, image, basePath);

        if (img.has_value()) {
            images.push_back(*img);
            file.images[image.name.c_str()] = *img;
            loadedTextures++;
        }
        else {
            // we failed to load, so lets give the slot a default white texture to not
            // completely break loading
            images.push_back(engine->_errorCheckerboardImage);
            fmt::print("    Failed to load texture: {}\n", image.name);
        }

        // Flush GPU every 50 textures to prevent resource exhaustion
        if (i > 0 && i % 50 == 0) {
            vkDeviceWaitIdle(engine->_device);
        }
    }

    // Fill remaining slots with default texture if we limited
    for (size_t i = textureCount; i < gltf.images.size(); i++) {
        images.push_back(engine->_whiteImage);
    }

    fmt::print("  Loaded {}/{} textures successfully\n", loadedTextures, gltf.images.size());




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

        GLTFMetallic_Roughness::MaterialConstants constants{};
        constants.colorFactors.x = mat.pbrData.baseColorFactor[0];
        constants.colorFactors.y = mat.pbrData.baseColorFactor[1];
        constants.colorFactors.z = mat.pbrData.baseColorFactor[2];
        constants.colorFactors.w = mat.pbrData.baseColorFactor[3];

        constants.metal_rough_factors.x = mat.pbrData.metallicFactor;
        constants.metal_rough_factors.y = mat.pbrData.roughnessFactor;
        constants.metal_rough_factors.z = 1.0f; // AO default
        constants.metal_rough_factors.w = 1.0f; // Normal strength default
        constants.normalTexID = 0;
        constants.emissiveTexID = 0;

        MaterialPass passType = MaterialPass::MainColor;
        const float baseAlpha = constants.colorFactors.w;
        const bool alphaFactorTransparent = baseAlpha < 0.999f;

        // glTF BLEND => transparent pass.
        // Fallback: if alpha factor is already < 1, treat as transparent even if alphaMode is OPAQUE.
        if (mat.alphaMode == fastgltf::AlphaMode::Blend || alphaFactorTransparent) {
            passType = MaterialPass::Transparent;
        }
        // glTF MASK is alpha-tested and should remain in opaque pass.
        if (mat.alphaMode == fastgltf::AlphaMode::Mask) {
            constants.extra[1].y = mat.alphaCutoff;
            passType = MaterialPass::MainColor;
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
        auto resolveTextureBinding = [&](size_t textureIndex, size_t& outImg, VkSampler& outSampler) -> bool {
            if (textureIndex >= gltf.textures.size()) return false;
            const auto& tex = gltf.textures[textureIndex];
            if (!tex.imageIndex.has_value()) return false;
            outImg = tex.imageIndex.value();
            if (outImg >= images.size()) return false;

            size_t samplerIndex = 0;
            if (tex.samplerIndex.has_value() && tex.samplerIndex.value() < file.samplers.size()) {
                samplerIndex = tex.samplerIndex.value();
            }
            outSampler = file.samplers[samplerIndex];
            return true;
        };

        // Load base color (albedo) texture
        if (mat.pbrData.baseColorTexture.has_value()) {
            size_t img = 0;
            VkSampler sampler = engine->_defaultSamplerLinear;
            if (resolveTextureBinding(mat.pbrData.baseColorTexture.value().textureIndex, img, sampler)) {
                materialResources.colorImage = images[img];
                materialResources.colorSampler = sampler;

                // Add to bindless texture cache so the shader can sample it
                auto& colorImg = images[img];
                if (colorImg.image != VK_NULL_HANDLE && colorImg.image != engine->_whiteImage.image) {
                    TextureID colorTexID = engine->texCache.AddTexture(
                        colorImg.imageView, sampler,
                        std::string("color_") + std::to_string(data_index));
                    constants.colorTexID = colorTexID.Index;
                }
            }
        }

        // Load metallic-roughness texture
        if (mat.pbrData.metallicRoughnessTexture.has_value()) {
            size_t img = 0;
            VkSampler sampler = engine->_defaultSamplerLinear;
            if (resolveTextureBinding(mat.pbrData.metallicRoughnessTexture.value().textureIndex, img, sampler)) {
                materialResources.metalRoughImage = images[img];
                materialResources.metalRoughSampler = sampler;

                // Add to bindless texture cache for shader sampling
                auto& mrImg = images[img];
                if (mrImg.image != VK_NULL_HANDLE && mrImg.image != engine->_whiteImage.image) {
                    TextureID mrTexID = engine->texCache.AddTexture(
                        mrImg.imageView, sampler,
                        std::string("metalrough_") + std::to_string(data_index));
                    constants.metalRoughTexID = mrTexID.Index;
                }
            }
        }

        // Load emissive factor into material constants (works with or without texture)
        // Note: extra[0] is used for emissive (xyz=color, w=strength)
        // Check if any emissive factor component is non-zero
        bool hasEmissive = (mat.emissiveFactor[0] > 0.0f ||
                           mat.emissiveFactor[1] > 0.0f ||
                           mat.emissiveFactor[2] > 0.0f);

        if (hasEmissive) {
            constants.extra[0] = glm::vec4(
                mat.emissiveFactor[0],
                mat.emissiveFactor[1],
                mat.emissiveFactor[2],
                1.0f  // Emissive strength multiplier
            );
        }

        // Load normal map texture
        if (mat.normalTexture.has_value()) {
            size_t img = 0;
            VkSampler ignoredSampler = engine->_defaultSamplerLinear;
            if (resolveTextureBinding(mat.normalTexture.value().textureIndex, img, ignoredSampler)) {
                // Store normal texture in the texture cache and set the ID
                auto& normalImage = images[img];
                if (normalImage.image != VK_NULL_HANDLE && normalImage.image != engine->_whiteImage.image &&
                    normalImage.image != engine->_errorCheckerboardImage.image) {
                    TextureID normalTexID = engine->texCache.AddTexture(
                        normalImage.imageView, engine->_defaultSamplerLinear,
                        std::string("normal_") + std::to_string(data_index));
                    constants.normalTexID = normalTexID.Index;
                    constants.metal_rough_factors.w = mat.normalTexture.value().scale; // normal strength
                }
            }
        }

        // Load emissive texture
        if (mat.emissiveTexture.has_value()) {
            size_t img = 0;
            VkSampler ignoredSampler = engine->_defaultSamplerLinear;
            if (resolveTextureBinding(mat.emissiveTexture.value().textureIndex, img, ignoredSampler)) {
                auto& emissiveImage = images[img];
                if (emissiveImage.image != VK_NULL_HANDLE && emissiveImage.image != engine->_whiteImage.image &&
                    emissiveImage.image != engine->_errorCheckerboardImage.image) {
                    TextureID emissiveTexID = engine->texCache.AddTexture(
                        emissiveImage.imageView, engine->_defaultSamplerLinear,
                        std::string("emissive_") + std::to_string(data_index));
                    constants.emissiveTexID = emissiveTexID.Index;
                    // If no emissive factor was set, default to white so the texture shows
                    if (!hasEmissive) {
                        constants.extra[0] = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
                    }
                }
            }
        }

        // Load occlusion texture (stored in metalRough R channel or extra field)
        if (mat.occlusionTexture.has_value()) {
            constants.metal_rough_factors.z = mat.occlusionTexture.value().strength; // AO strength
        }

        // Write final material constants to GPU buffer (after all textures loaded)
        sceneMaterialConstants[data_index] = constants;

        // build material
        const bool isDoubleSided = mat.doubleSided;
        newMat->data = engine->metalRoughMaterial.write_material(
            engine->_device, passType, isDoubleSided, materialResources, file.descriptorPool);

        const char* alphaModeStr = "OPAQUE";
        if (mat.alphaMode == fastgltf::AlphaMode::Blend) alphaModeStr = "BLEND";
        else if (mat.alphaMode == fastgltf::AlphaMode::Mask) alphaModeStr = "MASK";
        const char* passStr = (passType == MaterialPass::Transparent) ? "Transparent" : "Opaque";
        fmt::print("  [GLTF][Material:{}] alphaMode={} baseA={:.3f} cutoff={:.3f} doubleSided={} -> pass={}\n",
            mat.name.empty() ? "<unnamed>" : mat.name,
            alphaModeStr,
            baseAlpha,
            constants.extra[1].y,
            isDoubleSided ? 1 : 0,
            passStr);
        newMat->bufferOffset = materialResources.dataBufferOffset;  // Store for runtime updates

        data_index++;
    }
    //< load_material

        // use the same vectors for all meshes so that the memory doesnt reallocate as
        // often
    std::vector<uint32_t> indices;
    std::vector<Vertex> vertices;
    std::vector<SkinVertexData> skinData;

    fmt::print("  Loading {} meshes...\n", gltf.meshes.size());
    size_t meshIndex = 0;
    size_t totalVertices = 0;
    size_t totalIndices = 0;

    for (fastgltf::Mesh& mesh : gltf.meshes) {
        std::shared_ptr<MeshAsset> newmesh = std::make_shared<MeshAsset>();
        meshes.push_back(newmesh);
        file.meshes[mesh.name.c_str()] = newmesh;
        newmesh->name = mesh.name;

        // Progress log for large files
        if (gltf.meshes.size() > 10 && meshIndex % 10 == 0) {
            fmt::print("    Mesh {}/{}: {}\n", meshIndex + 1, gltf.meshes.size(), mesh.name);
        }

        // clear the mesh arrays each mesh, we dont want to merge them by error
        indices.clear();
        vertices.clear();
        skinData.clear();
        bool meshHasSkinData = false;

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
                skinData.resize(skinData.size() + posAccessor.count);

                fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, posAccessor,
                    [&](glm::vec3 v, size_t index) {
                        Vertex newvtx;
                        newvtx.position = v;
                        newvtx.normal = { 1, 0, 0 };
                        newvtx.color = glm::vec4{ 1.f };
                        newvtx.uv_x = 0;
                        newvtx.uv_y = 0;
                        vertices[initial_vtx + index] = newvtx;
                        skinData[initial_vtx + index] = SkinVertexData{};
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

            // load tangents (needed for normal mapping)
            auto tangents = p.findAttribute("TANGENT");
            if (tangents != p.attributes.end()) {
                fastgltf::iterateAccessorWithIndex<glm::vec4>(gltf, gltf.accessors[(*tangents).second],
                    [&](glm::vec4 v, size_t index) {
                        vertices[initial_vtx + index].tangent = v;
                    });
            }

            // load skinning data (JOINTS_0, WEIGHTS_0) into dedicated skin buffer stream
            auto jointsAttr = p.findAttribute("JOINTS_0");
            if (jointsAttr != p.attributes.end()) {
                meshHasSkinData = true;
                fastgltf::iterateAccessorWithIndex<glm::uvec4>(gltf, gltf.accessors[(*jointsAttr).second],
                    [&](glm::uvec4 v, size_t index) {
                        skinData[initial_vtx + index].joints = glm::ivec4(v);
                    });
            }

            auto weightsAttr = p.findAttribute("WEIGHTS_0");
            if (weightsAttr != p.attributes.end()) {
                meshHasSkinData = true;
                fastgltf::iterateAccessorWithIndex<glm::vec4>(gltf, gltf.accessors[(*weightsAttr).second],
                    [&](glm::vec4 v, size_t index) {
                        glm::vec4 w = glm::max(v, glm::vec4(0.0f));
                        float sum = w.x + w.y + w.z + w.w;
                        skinData[initial_vtx + index].weights = (sum > 0.000001f)
                            ? (w / sum)
                            : glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
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

        totalVertices += vertices.size();
        totalIndices += indices.size();

        // Retain CPU-side vertex data for path tracing BVH
        newmesh->cpuVertices = vertices;
        newmesh->cpuIndices = indices;
        newmesh->hasCpuData = true;

        newmesh->meshBuffers = engine->uploadMesh(indices, vertices);
        if (meshHasSkinData && skinData.size() == vertices.size()) {
            newmesh->skinBuffer = engine->uploadSkinBuffer(skinData);
            if (newmesh->skinBuffer.buffer != VK_NULL_HANDLE) {
                VkBufferDeviceAddressInfo skinAddressInfo{
                    .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
                    .buffer = newmesh->skinBuffer.buffer
                };
                newmesh->skinBufferAddress = vkGetBufferDeviceAddress(engine->_device, &skinAddressInfo);
                newmesh->hasSkinData = (newmesh->skinBufferAddress != 0);
            }
        }
        meshIndex++;
    }

    fmt::print("  Total: {} vertices, {} triangles\n", totalVertices, totalIndices / 3);
    //> load_nodes
        // load all nodes and their meshes
    std::vector<std::string> nodeNames(gltf.nodes.size());
    for (size_t nodeIdx = 0; nodeIdx < gltf.nodes.size(); ++nodeIdx) {
        fastgltf::Node& node = gltf.nodes[nodeIdx];
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
        std::string nodeName = node.name.empty() ? ("Node_" + std::to_string(nodeIdx)) : std::string(node.name);
        nodeNames[nodeIdx] = nodeName;
        file.nodes[nodeName] = newNode;

        std::visit(fastgltf::visitor{ [&](fastgltf::Node::TransformMatrix matrix) {
                                          // Keep authoring matrix as-is. Decomposing and rebuilding TRS here can
                                          // break FBX/DAE converted rigs that rely on matrix-space bind/orient data.
                                          memcpy(&newNode->localTransform, matrix.data(), sizeof(matrix));
                                          newNode->hasLocalTRS = false;
                                      },
                       [&](fastgltf::Node::TRS transform) {
                           glm::vec3 tl(transform.translation[0], transform.translation[1],
                               transform.translation[2]);
                           glm::quat rot(transform.rotation[3], transform.rotation[0], transform.rotation[1],
                               transform.rotation[2]);
                           glm::vec3 sc(transform.scale[0], transform.scale[1], transform.scale[2]);

                           newNode->localTranslation = tl;
                           newNode->localRotation = rot;
                           newNode->localScale = sc;
                           newNode->hasLocalTRS = true;
                           newNode->rebuildLocalTransformFromTRS();
                       } },
            node.transform);
    }
    file.indexedNodes = nodes;
    //< load_nodes
    //> load_graph
        // run loop again to setup transform hierarchy
    std::vector<int> parentIndices(gltf.nodes.size(), -1);
    for (int i = 0; i < gltf.nodes.size(); i++) {
        fastgltf::Node& node = gltf.nodes[i];
        std::shared_ptr<Node>& sceneNode = nodes[i];

        for (auto& c : node.children) {
            sceneNode->children.push_back(nodes[c]);
            nodes[c]->parent = sceneNode;
            parentIndices[c] = i;
        }
    }

    // find the top nodes, with no parents
    for (auto& node : nodes) {
        if (node->parent.lock() == nullptr) {
            file.topNodes.push_back(node);
            node->refreshTransform(glm::mat4{ 1.f });
        }
    }

    //> load_skins
    std::vector<int> skinToSkeletonIndex(gltf.skins.size(), -1);
    if (!gltf.skins.empty()) {
        fmt::print("  Loading {} skins...\n", gltf.skins.size());

        for (size_t skinIdx = 0; skinIdx < gltf.skins.size(); ++skinIdx) {
            const auto& skin = gltf.skins[skinIdx];
            SkeletonData skeleton;
            skeleton.name = skin.name.empty() ? ("Skin_" + std::to_string(skinIdx)) : std::string(skin.name);
            skeleton.sourceScene = path.string();
            skeleton.bones.reserve(skin.joints.size());

            std::unordered_map<int, int> nodeToBone;
            for (size_t j = 0; j < skin.joints.size(); ++j) {
                nodeToBone[static_cast<int>(skin.joints[j])] = static_cast<int>(j);
            }

            std::vector<glm::mat4> inverseBindMatrices;
            if (skin.inverseBindMatrices.has_value() && skin.inverseBindMatrices.value() < gltf.accessors.size()) {
                auto& ibmAccessor = gltf.accessors[skin.inverseBindMatrices.value()];
                inverseBindMatrices.resize(ibmAccessor.count);
                fastgltf::iterateAccessorWithIndex<glm::mat4>(gltf, ibmAccessor,
                    [&](const glm::mat4& m, size_t i) {
                        inverseBindMatrices[i] = m;
                    });
            }

            for (size_t jointIdx = 0; jointIdx < skin.joints.size(); ++jointIdx) {
                const int nodeIndex = static_cast<int>(skin.joints[jointIdx]);
                SkeletonBoneData bone;
                bone.nodeIndex = nodeIndex;
                bone.name = (nodeIndex >= 0 && nodeIndex < static_cast<int>(nodeNames.size()))
                    ? nodeNames[nodeIndex]
                    : ("Joint_" + std::to_string(jointIdx));

                int parentNodeIndex = (nodeIndex >= 0 && nodeIndex < static_cast<int>(parentIndices.size()))
                    ? parentIndices[nodeIndex] : -1;
                auto parentIt = nodeToBone.find(parentNodeIndex);
                bone.parentIndex = (parentIt != nodeToBone.end()) ? parentIt->second : -1;

                if (nodeIndex >= 0 && nodeIndex < static_cast<int>(nodes.size())) {
                    glm::vec3 scale, translation, skew;
                    glm::vec4 perspective;
                    glm::quat rotation;
                    if (glm::decompose(nodes[nodeIndex]->localTransform, scale, rotation, translation, skew, perspective)) {
                        bone.localPosition = translation;
                        bone.localRotation = rotation;
                        bone.localScale = scale;
                    }
                }

                if (jointIdx < inverseBindMatrices.size()) {
                    bone.inverseBindMatrix = inverseBindMatrices[jointIdx];
                }

                skeleton.bones.push_back(bone);
            }

            engine->skeletons.push_back(skeleton);
            skinToSkeletonIndex[skinIdx] = static_cast<int>(engine->skeletons.size()) - 1;
            if (engine->activeSkeletonIndex < 0) {
                engine->activeSkeletonIndex = skinToSkeletonIndex[skinIdx];
            }
        }
    }

    // Compute meshBindTransform for each skeleton.
    // FBX-converted GLTF files keep vertex data in FBX space (e.g. Z-up) and add a
    // root rotation node for axis conversion. The skinning equation (jointWorld * IBM)
    // cancels to identity at bind pose, leaving vertices un-rotated (character lays
    // down). meshBindTransform captures the axis-conversion transform so that it's
    // applied to vertices through the skinning matrix.
    //
    // Strategy: Try to find the node with both mesh+skin first. If that fails,
    // walk up from the skeleton root to the scene root, collecting the transforms
    // of all non-skeleton ancestor nodes. This gives us the axis conversion.
    {
        // Build set of skeleton joint node indices per skin for fast lookup.
        std::vector<std::unordered_set<int>> skinJointNodes(gltf.skins.size());
        for (size_t skinIdx = 0; skinIdx < gltf.skins.size(); ++skinIdx) {
            for (auto j : gltf.skins[skinIdx].joints) {
                skinJointNodes[skinIdx].insert(static_cast<int>(j));
            }
        }

        // Method 1: Find node with both meshIndex + skinIndex.
        for (size_t nodeIdx = 0; nodeIdx < gltf.nodes.size(); ++nodeIdx) {
            const auto& gltfNode = gltf.nodes[nodeIdx];
            if (gltfNode.skinIndex.has_value() && gltfNode.meshIndex.has_value()) {
                size_t skinIdx = gltfNode.skinIndex.value();
                if (skinIdx < skinToSkeletonIndex.size()) {
                    int skelIdx = skinToSkeletonIndex[skinIdx];
                    if (skelIdx >= 0 && skelIdx < static_cast<int>(engine->skeletons.size())) {
                        auto& skel = engine->skeletons[skelIdx];
                        if (nodeIdx < nodes.size() && nodes[nodeIdx]) {
                            skel.meshBindTransform = nodes[nodeIdx]->worldTransform;
                            fmt::print("  [Skin] meshBindTransform from mesh+skin node {} ('{}')\n",
                                nodeIdx, nodeNames[nodeIdx]);
                        }
                    }
                }
            }
        }

        // Method 2 (fallback): walk up from skeleton root to scene root.
        // Collect the world transform of the nearest non-skeleton ancestor.
        for (size_t skinIdx = 0; skinIdx < gltf.skins.size(); ++skinIdx) {
            int skelIdx = (skinIdx < skinToSkeletonIndex.size()) ? skinToSkeletonIndex[skinIdx] : -1;
            if (skelIdx < 0 || skelIdx >= static_cast<int>(engine->skeletons.size())) continue;
            auto& skel = engine->skeletons[skelIdx];

            // Skip if already set by method 1 (non-identity).
            if (skel.meshBindTransform != glm::mat4(1.0f)) continue;

            // Find the first joint and walk up to the first non-skeleton parent.
            if (gltf.skins[skinIdx].joints.empty()) continue;
            int startNode = static_cast<int>(gltf.skins[skinIdx].joints[0]);

            // Walk up to skeleton root first (a joint whose parent is not a joint).
            int current = startNode;
            while (current >= 0 && current < static_cast<int>(parentIndices.size())) {
                int parent = parentIndices[current];
                if (parent < 0) break;
                if (skinJointNodes[skinIdx].find(parent) == skinJointNodes[skinIdx].end()) break;
                current = parent;
            }
            // 'current' is now the skeleton root joint.
            // Its parent (if any) is a non-skeleton node whose worldTransform
            // contains the axis conversion.
            int skelRootParent = (current >= 0 && current < static_cast<int>(parentIndices.size()))
                ? parentIndices[current] : -1;
            if (skelRootParent >= 0 && skelRootParent < static_cast<int>(nodes.size()) && nodes[skelRootParent]) {
                skel.meshBindTransform = nodes[skelRootParent]->worldTransform;
                fmt::print("  [Skin] meshBindTransform from skeleton parent node {} ('{}')\n",
                    skelRootParent, nodeNames[skelRootParent]);
            }
        }
    }

    // Force TRS decomposition for all skeleton joint nodes.
    // FBX/DAE-converted GLTF files may store bone transforms as matrices instead
    // of TRS. The animation system operates on TRS, so lazy decomposition during
    // playback via glm::decompose can produce flipped quaternions / wrong scale.
    // Decompose eagerly here with negative-determinant handling.
    for (int skelIdx : skinToSkeletonIndex) {
        if (skelIdx < 0 || skelIdx >= static_cast<int>(engine->skeletons.size())) continue;
        const auto& skel = engine->skeletons[skelIdx];
        for (const auto& bone : skel.bones) {
            if (bone.nodeIndex < 0 || bone.nodeIndex >= static_cast<int>(nodes.size())) continue;
            auto& node = nodes[bone.nodeIndex];
            if (node && !node->hasLocalTRS) {
                glm::vec3 scale, translation, skew;
                glm::vec4 perspective;
                glm::quat rotation;
                if (glm::decompose(node->localTransform, scale, rotation, translation, skew, perspective)) {
                    // Negative determinant means a reflection; absorb it into the quaternion
                    if (glm::determinant(glm::mat3(node->localTransform)) < 0.0f) {
                        scale = -scale;
                        rotation = glm::conjugate(rotation);
                    }
                    node->localTranslation = translation;
                    node->localRotation = glm::normalize(rotation);
                    node->localScale = scale;
                    node->hasLocalTRS = true;
                    node->rebuildLocalTransformFromTRS();
                }
            }
        }
    }

    // Bake meshBindTransform into the inverse bind matrices.
    // For FBX-converted files, meshBindTransform = axis conversion rotation R.
    // IBM_new = IBM * R  makes the skinning equation produce:
    //   bind pose:  jointWorld * IBM * R * pos = identity * R * pos = R * pos  (standing)
    //   animated:   jointWorld * IBM * R * pos = R * Janim * Jbind^-1 * pos   (correct)
    // For native GLTF files, R = identity, so this is a no-op.
    for (int skelIdx : skinToSkeletonIndex) {
        if (skelIdx < 0 || skelIdx >= static_cast<int>(engine->skeletons.size())) continue;
        auto& skel = engine->skeletons[skelIdx];
        if (skel.meshBindTransform == glm::mat4(1.0f)) continue; // identity, skip
        for (auto& bone : skel.bones) {
            bone.inverseBindMatrix = bone.inverseBindMatrix * skel.meshBindTransform;
        }
        fmt::print("  [Skin] Baked axis conversion into {} IBMs for skeleton '{}'\n",
            skel.bones.size(), skel.name);
    }
    //< load_skins

    //> load_animations
    if (!gltf.animations.empty()) {
        fmt::print("  Loading {} animations...\n", gltf.animations.size());

        for (size_t animIdx = 0; animIdx < gltf.animations.size(); ++animIdx) {
            const auto& anim = gltf.animations[animIdx];
            AnimationClipData clip;
            clip.name = anim.name.empty() ? ("Animation_" + std::to_string(animIdx)) : std::string(anim.name);
            clip.sourceScene = path.string();
            clip.duration = 0.0f;

            for (const auto& channel : anim.channels) {
                if (channel.samplerIndex >= anim.samplers.size()) {
                    continue;
                }
                const auto& sampler = anim.samplers[channel.samplerIndex];
                if (sampler.inputAccessor >= gltf.accessors.size() || sampler.outputAccessor >= gltf.accessors.size()) {
                    continue;
                }

                std::vector<float> inputTimes;
                fastgltf::iterateAccessor<float>(gltf, gltf.accessors[sampler.inputAccessor],
                    [&](float t) { inputTimes.push_back(t); });
                if (inputTimes.empty()) {
                    continue;
                }

                for (float t : inputTimes) {
                    clip.duration = std::max(clip.duration, t);
                }

                AnimationTrackData track;
                track.sourceScene = clip.sourceScene;
                track.targetNodeIndex = static_cast<int>(channel.nodeIndex);
                if (track.targetNodeIndex >= 0 && track.targetNodeIndex < static_cast<int>(nodeNames.size())) {
                    track.targetNode = nodeNames[track.targetNodeIndex];
                } else {
                    track.targetNode = "Node_" + std::to_string(track.targetNodeIndex);
                }

                switch (channel.path) {
                case fastgltf::AnimationPath::Translation: track.property = "translation"; break;
                case fastgltf::AnimationPath::Rotation: track.property = "rotation"; break;
                case fastgltf::AnimationPath::Scale: track.property = "scale"; break;
                default: track.property = "weights"; break;
                }
                if (track.property == "weights") {
                    continue;
                }

                int interpolation = 1;
                if (sampler.interpolation == fastgltf::AnimationInterpolation::Step) interpolation = 0;
                if (sampler.interpolation == fastgltf::AnimationInterpolation::CubicSpline) interpolation = 2;

                auto resolveBoneIndex = [&](int nodeIndex) -> int {
                    for (int skelIndex : skinToSkeletonIndex) {
                        if (skelIndex < 0 || skelIndex >= static_cast<int>(engine->skeletons.size())) continue;
                        const auto& skel = engine->skeletons[skelIndex];
                        for (size_t bi = 0; bi < skel.bones.size(); ++bi) {
                            if (skel.bones[bi].nodeIndex == nodeIndex) {
                                if (clip.skeletonIndex < 0) clip.skeletonIndex = skelIndex;
                                return static_cast<int>(bi);
                            }
                        }
                    }
                    return -1;
                };
                track.targetBoneIndex = resolveBoneIndex(track.targetNodeIndex);

                const auto& outputAccessor = gltf.accessors[sampler.outputAccessor];
                if (interpolation == 2) {
                    if (track.property == "rotation") {
                        std::vector<glm::vec4> raw;
                        fastgltf::iterateAccessor<glm::vec4>(gltf, outputAccessor, [&](glm::vec4 v) { raw.push_back(v); });
                        size_t keyCount = std::min(inputTimes.size(), raw.size() / 3);
                        for (size_t i = 0; i < keyCount; ++i) {
                            AnimationKeyframeData key;
                            key.time = inputTimes[i];
                            key.interpolation = interpolation;
                            key.inTangent = raw[i * 3 + 0];
                            key.value = glm::normalize(raw[i * 3 + 1]);
                            key.outTangent = raw[i * 3 + 2];
                            key.hasTangents = true;
                            track.keyframes.push_back(key);
                        }
                    } else {
                        std::vector<glm::vec3> raw;
                        fastgltf::iterateAccessor<glm::vec3>(gltf, outputAccessor, [&](glm::vec3 v) { raw.push_back(v); });
                        size_t keyCount = std::min(inputTimes.size(), raw.size() / 3);
                        for (size_t i = 0; i < keyCount; ++i) {
                            AnimationKeyframeData key;
                            key.time = inputTimes[i];
                            key.interpolation = interpolation;
                            key.inTangent = glm::vec4(raw[i * 3 + 0], 0.0f);
                            key.value = glm::vec4(raw[i * 3 + 1], 0.0f);
                            key.outTangent = glm::vec4(raw[i * 3 + 2], 0.0f);
                            key.hasTangents = true;
                            track.keyframes.push_back(key);
                        }
                    }
                } else {
                    if (track.property == "rotation") {
                        std::vector<glm::vec4> values;
                        fastgltf::iterateAccessor<glm::vec4>(gltf, outputAccessor, [&](glm::vec4 v) { values.push_back(v); });
                        size_t keyCount = std::min(inputTimes.size(), values.size());
                        for (size_t i = 0; i < keyCount; ++i) {
                            AnimationKeyframeData key;
                            key.time = inputTimes[i];
                            key.value = glm::normalize(values[i]);
                            key.interpolation = interpolation;
                            track.keyframes.push_back(key);
                        }
                    } else {
                        std::vector<glm::vec3> values;
                        fastgltf::iterateAccessor<glm::vec3>(gltf, outputAccessor, [&](glm::vec3 v) { values.push_back(v); });
                        size_t keyCount = std::min(inputTimes.size(), values.size());
                        for (size_t i = 0; i < keyCount; ++i) {
                            AnimationKeyframeData key;
                            key.time = inputTimes[i];
                            key.value = glm::vec4(values[i], 0.0f);
                            key.interpolation = interpolation;
                            track.keyframes.push_back(key);
                        }
                    }
                }

                if (!track.keyframes.empty()) {
                    clip.tracks.push_back(track);
                }
            }

            if (!clip.tracks.empty()) {
                engine->animationClips.push_back(clip);
                if (engine->activeAnimationIndex < 0) {
                    engine->activeAnimationIndex = static_cast<int>(engine->animationClips.size()) - 1;
                }
            }
        }
    }

    // Force TRS decomposition for any node targeted by an animation track
    // that still only has a matrix transform.
    for (const auto& clip : engine->animationClips) {
        if (clip.sourceScene != path.string()) continue;
        for (const auto& track : clip.tracks) {
            if (track.targetNodeIndex < 0 || track.targetNodeIndex >= static_cast<int>(nodes.size())) continue;
            auto& node = nodes[track.targetNodeIndex];
            if (node && !node->hasLocalTRS) {
                glm::vec3 scale, translation, skew;
                glm::vec4 perspective;
                glm::quat rotation;
                if (glm::decompose(node->localTransform, scale, rotation, translation, skew, perspective)) {
                    if (glm::determinant(glm::mat3(node->localTransform)) < 0.0f) {
                        scale = -scale;
                        rotation = glm::conjugate(rotation);
                    }
                    node->localTranslation = translation;
                    node->localRotation = glm::normalize(rotation);
                    node->localScale = scale;
                    node->hasLocalTRS = true;
                    node->rebuildLocalTransformFromTRS();
                }
            }
        }
    }
    //< load_animations

    //> load_cameras
    // Load cameras from GLTF
    if (!gltf.cameras.empty()) {
        fmt::print("  Loading {} cameras...\n", gltf.cameras.size());

        for (size_t camIdx = 0; camIdx < gltf.cameras.size(); camIdx++) {
            const fastgltf::Camera& gltfCam = gltf.cameras[camIdx];

            GLTFCamera newCamera;
            newCamera.name = gltfCam.name.empty() ?
                ("Camera_" + std::to_string(camIdx)) : std::string(gltfCam.name);

            // Check camera type and extract parameters
            if (auto* persp = std::get_if<fastgltf::Camera::Perspective>(&gltfCam.camera)) {
                newCamera.isPerspective = true;
                newCamera.fov = glm::degrees(persp->yfov);
                newCamera.aspectRatio = persp->aspectRatio.has_value() ?
                    persp->aspectRatio.value() : 0.0f;
                newCamera.nearPlane = persp->znear;
                newCamera.farPlane = persp->zfar.has_value() ?
                    persp->zfar.value() : 10000.0f;

                fmt::print("    Camera '{}': Perspective, FOV={:.1f}°, near={:.3f}, far={:.1f}\n",
                    newCamera.name, newCamera.fov, newCamera.nearPlane, newCamera.farPlane);
            }
            else if (auto* ortho = std::get_if<fastgltf::Camera::Orthographic>(&gltfCam.camera)) {
                newCamera.isPerspective = false;
                newCamera.orthoWidth = ortho->xmag * 2.0f;
                newCamera.orthoHeight = ortho->ymag * 2.0f;
                newCamera.nearPlane = ortho->znear;
                newCamera.farPlane = ortho->zfar;

                fmt::print("    Camera '{}': Orthographic, size={:.1f}x{:.1f}\n",
                    newCamera.name, newCamera.orthoWidth, newCamera.orthoHeight);
            }

            file.cameras.push_back(newCamera);
        }

        // Find nodes that reference cameras and extract their transforms
        for (size_t nodeIdx = 0; nodeIdx < gltf.nodes.size(); nodeIdx++) {
            const fastgltf::Node& gltfNode = gltf.nodes[nodeIdx];

            if (gltfNode.cameraIndex.has_value()) {
                size_t camIdx = gltfNode.cameraIndex.value();
                if (camIdx < file.cameras.size()) {
                    // Get world transform from our scene node
                    glm::mat4 worldTransform = nodes[nodeIdx]->worldTransform;
                    file.cameras[camIdx].worldTransform = worldTransform;

                    // Extract position from transform
                    file.cameras[camIdx].position = glm::vec3(worldTransform[3]);
                    file.cameras[camIdx].sourceNode = nodes[nodeIdx].get();

                    // Extract forward and up vectors (GLTF cameras look down -Z)
                    file.cameras[camIdx].forward = glm::normalize(
                        glm::vec3(-worldTransform[2]));  // -Z axis
                    file.cameras[camIdx].up = glm::normalize(
                        glm::vec3(worldTransform[1]));   // Y axis

                    fmt::print("    Camera '{}' at position ({:.2f}, {:.2f}, {:.2f})\n",
                        file.cameras[camIdx].name,
                        file.cameras[camIdx].position.x,
                        file.cameras[camIdx].position.y,
                        file.cameras[camIdx].position.z);
                }
            }
        }
    }
    //< load_cameras

    //> load_lights
    // Load lights from KHR_lights_punctual extension
    if (!gltf.lights.empty()) {
        fmt::print("  Loading {} lights from GLTF...\n", gltf.lights.size());

        // Find nodes that reference lights and extract their world transforms
        for (size_t nodeIdx = 0; nodeIdx < gltf.nodes.size(); nodeIdx++) {
            const fastgltf::Node& gltfNode = gltf.nodes[nodeIdx];

            if (gltfNode.lightIndex.has_value()) {
                size_t lightIdx = gltfNode.lightIndex.value();
                if (lightIdx >= gltf.lights.size()) continue;

                const fastgltf::Light& gltfLight = gltf.lights[lightIdx];
                glm::mat4 worldTransform = nodes[nodeIdx]->worldTransform;
                glm::vec3 lightPos = glm::vec3(worldTransform[3]);
                // Light direction: GLTF lights shine along -Z in local space
                glm::vec3 lightDir = glm::normalize(glm::vec3(-worldTransform[2]));
                glm::vec3 lightColor(gltfLight.color[0], gltfLight.color[1], gltfLight.color[2]);
                float intensity = gltfLight.intensity;
                GLTFLight importedLight;
                importedLight.name = gltfLight.name.empty() ? ("Light_" + std::to_string(lightIdx)) : std::string(gltfLight.name);
                importedLight.position = lightPos;
                importedLight.direction = lightDir;
                importedLight.color = lightColor;
                importedLight.sourceNode = nodes[nodeIdx].get();

                if (gltfLight.type == fastgltf::LightType::Directional) {
                    importedLight.type = 0;
                    // Apply as sun/directional light
                    // Convert intensity from lux to a reasonable engine scale
                    float scaledIntensity = std::min(intensity / 1000.0f, 10.0f);
                    if (scaledIntensity < 0.01f) scaledIntensity = 1.0f;
                    importedLight.intensity = scaledIntensity;

                    engine->sceneData.sunlightDirection = glm::vec4(-lightDir, scaledIntensity);
                    engine->sceneData.sunlightColor = glm::vec4(lightColor, 1.0f);

                    fmt::print("    Directional light '{}': dir=({:.2f},{:.2f},{:.2f}), color=({:.2f},{:.2f},{:.2f}), intensity={:.2f}\n",
                        std::string(gltfLight.name), lightDir.x, lightDir.y, lightDir.z,
                        lightColor.r, lightColor.g, lightColor.b, scaledIntensity);
                }
                else if (gltfLight.type == fastgltf::LightType::Point) {
                    importedLight.type = 1;
                    // Add as point light
                    float range = gltfLight.range.has_value() ? gltfLight.range.value() : 25.0f;
                    float scaledIntensity = std::min(intensity / 100.0f, 50.0f);
                    if (scaledIntensity < 0.01f) scaledIntensity = 1.0f;
                    importedLight.range = range;
                    importedLight.intensity = scaledIntensity;

                    PointLight p{};
                    p.position = lightPos;
                    p.radius = range;
                    p.color = lightColor;
                    p.intensity = scaledIntensity;
                    engine->scenePointLights.push_back(p);
                    importedLight.runtimePointLightIndex = static_cast<int>(engine->scenePointLights.size()) - 1;

                    fmt::print("    Point light '{}': pos=({:.2f},{:.2f},{:.2f}), range={:.1f}, intensity={:.2f}\n",
                        std::string(gltfLight.name), lightPos.x, lightPos.y, lightPos.z, range, scaledIntensity);
                }
                else if (gltfLight.type == fastgltf::LightType::Spot) {
                    importedLight.type = 2;
                    // Keep spot support lightweight by storing as point light runtime.
                    float range = gltfLight.range.has_value() ? gltfLight.range.value() : 25.0f;
                    float scaledIntensity = std::min(intensity / 100.0f, 50.0f);
                    if (scaledIntensity < 0.01f) scaledIntensity = 1.0f;
                    importedLight.range = range;
                    importedLight.intensity = scaledIntensity;

                    PointLight p{};
                    p.position = lightPos;
                    p.radius = range;
                    p.color = lightColor;
                    p.intensity = scaledIntensity;
                    engine->scenePointLights.push_back(p);
                    importedLight.runtimePointLightIndex = static_cast<int>(engine->scenePointLights.size()) - 1;

                    fmt::print("    Spot light '{}' (as point): pos=({:.2f},{:.2f},{:.2f}), range={:.1f}\n",
                        std::string(gltfLight.name), lightPos.x, lightPos.y, lightPos.z, range);
                }

                file.lights.push_back(importedLight);
            }
        }
    }
    //< load_lights

        fmt::print("GLTF loaded successfully: {} nodes, {} meshes, {} materials, {} cameras, {} lights\n",
            scene->nodes.size(), scene->meshes.size(), scene->materials.size(), scene->cameras.size(), gltf.lights.size());
        return scene;
        //< load_graph

    } catch (const std::bad_alloc& e) {
        fmt::print("ERROR: Out of memory while loading GLTF: {}\n", e.what());
        fmt::print("  Try reducing model complexity or texture resolution\n");
        return {};
    } catch (const std::exception& e) {
        fmt::print("ERROR: Exception while loading GLTF: {}\n", e.what());
        return {};
    } catch (...) {
        fmt::print("ERROR: Unknown exception while loading GLTF\n");
        return {};
    }
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
    config.triangulate = true;

    const std::string objPath = std::filesystem::path(filePath).string();
    std::string sourceDir;
    if (auto it = gConvertedObjSourceDir.find(objPath); it != gConvertedObjSourceDir.end()) {
        sourceDir = it->second;
    } else {
        // Try absolute-key fallback.
        const std::string absObj = std::filesystem::absolute(std::filesystem::path(filePath)).string();
        if (auto itAbs = gConvertedObjSourceDir.find(absObj); itAbs != gConvertedObjSourceDir.end()) {
            sourceDir = itAbs->second;
        }
    }

    if (!reader.ParseFromFile(std::string(filePath), config)) {
        std::cerr << "Failed to load OBJ: " << reader.Error() << std::endl;
        return {};
    }
    if (!reader.Warning().empty()) fmt::print("OBJ warning: {}\n", reader.Warning());

    const auto& attrib = reader.GetAttrib();
    const auto& shapes = reader.GetShapes();
    const auto& materialsTiny = reader.GetMaterials();

    // +1 for the default material at slot 0, then imported OBJ/MTL materials at [1..N]
    const size_t materialCount = std::max<size_t>(1, materialsTiny.size() + 1);
    file.materialDataBuffer = engine->create_buffer(
        sizeof(GLTFMetallic_Roughness::MaterialConstants) * materialCount,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

    std::shared_ptr<GLTFMaterial> defaultMat = nullptr;
    {
        GLTFMetallic_Roughness::MaterialConstants constants{};
        constants.colorFactors = glm::vec4(0.85f, 0.85f, 0.85f, 1.0f);
        constants.metal_rough_factors = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
        auto* mapped = reinterpret_cast<GLTFMetallic_Roughness::MaterialConstants*>(file.materialDataBuffer.info.pMappedData);
        mapped[0] = constants;

        GLTFMetallic_Roughness::MaterialResources res;
        res.colorImage = engine->_whiteImage;
        res.colorSampler = engine->_defaultSamplerLinear;
        res.metalRoughImage = engine->_whiteImage;
        res.metalRoughSampler = engine->_defaultSamplerLinear;
        res.dataBuffer = file.materialDataBuffer.buffer;
        res.dataBufferOffset = 0;

        defaultMat = std::make_shared<GLTFMaterial>();
        defaultMat->data = engine->metalRoughMaterial.write_material(
            engine->_device, MaterialPass::MainColor, true, res, file.descriptorPool);
        materials.push_back(defaultMat);
        file.materials["_default"] = defaultMat;
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
        mapped[i + 1] = constants;

        GLTFMetallic_Roughness::MaterialResources res;
        res.colorImage = engine->_whiteImage;
        res.colorSampler = engine->_defaultSamplerLinear;
        res.metalRoughImage = engine->_whiteImage;
        res.metalRoughSampler = engine->_defaultSamplerLinear;
        res.dataBuffer = file.materialDataBuffer.buffer;
        res.dataBufferOffset = (i + 1) * sizeof(GLTFMetallic_Roughness::MaterialConstants);

        if (!mat.diffuse_texname.empty()) {
            std::vector<std::filesystem::path> textureCandidates;
            const std::filesystem::path texRel = std::filesystem::path(mat.diffuse_texname);
            std::vector<std::filesystem::path> texRelVariants;
            texRelVariants.push_back(texRel);

            // Assimp OBJ export can emit "file.tga.png"; try "file.tga" as fallback.
            if (texRel.extension() == ".png") {
                std::filesystem::path withoutPng = texRel;
                withoutPng.replace_extension("");
                if (withoutPng != texRel) {
                    texRelVariants.push_back(withoutPng);
                }
            }

            for (const auto& rel : texRelVariants) {
                const std::string texFileName = rel.filename().string();

                textureCandidates.push_back(std::filesystem::path(config.mtl_search_path) / rel);
                if (!sourceDir.empty()) {
                    textureCandidates.push_back(std::filesystem::path(sourceDir) / rel);
                    textureCandidates.push_back(std::filesystem::path(sourceDir) / texFileName);
                    textureCandidates.push_back(std::filesystem::path(sourceDir) / "textures" / texFileName);
                    textureCandidates.push_back(std::filesystem::path(sourceDir).parent_path() / "textures" / texFileName);
                }
            }

            std::filesystem::path texPath;
            for (const auto& cand : textureCandidates) {
                if (std::filesystem::exists(cand)) {
                    texPath = cand;
                    break;
                }
            }

            int w, h, ch;
            unsigned char* data = nullptr;
            if (!texPath.empty()) {
                data = stbi_load(texPath.string().c_str(), &w, &h, &ch, 4);
            }
            if (data) {
                VkExtent3D size{ (uint32_t)w, (uint32_t)h, 1 };
                AllocatedImage tex = engine->create_image(data, size, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, true);
                stbi_image_free(data);
                res.colorImage = tex;
            }
            else {
                fmt::print("Texture load failed: {} (sourceDir={})\n",
                    mat.diffuse_texname,
                    sourceDir.empty() ? "<none>" : sourceDir);
            }
        }

        newMat->data = engine->metalRoughMaterial.write_material(
            engine->_device, MaterialPass::MainColor, true, res, file.descriptorPool);
    }

    std::vector<uint32_t> indices;
    std::vector<Vertex> vertices;

    size_t meshCounter = 0;
    for (const auto& shape : shapes) {
        std::shared_ptr<MeshAsset> newmesh = std::make_shared<MeshAsset>();
        meshes.push_back(newmesh);
        const std::string meshName = shape.name.empty() ? ("obj_mesh_" + std::to_string(meshCounter++)) : shape.name;
        file.meshes[meshName] = newmesh;
        newmesh->name = meshName;

        indices.clear();
        vertices.clear();
        newmesh->surfaces.clear();

        size_t index_offset = 0;
        int activeMatId = std::numeric_limits<int>::min();
        bool hasActiveSurface = false;
        GeoSurface activeSurface{};
        glm::vec3 activeMin(0.0f);
        glm::vec3 activeMax(0.0f);

        auto flushActiveSurface = [&]() {
            if (!hasActiveSurface) return;
            const uint32_t start = activeSurface.startIndex;
            const uint32_t end = static_cast<uint32_t>(indices.size());
            if (end > start) {
                activeSurface.count = end - start;
                activeSurface.bounds.origin = (activeMax + activeMin) * 0.5f;
                activeSurface.bounds.extents = (activeMax - activeMin) * 0.5f;
                activeSurface.bounds.sphereRadius = glm::length(activeSurface.bounds.extents);
                newmesh->surfaces.push_back(activeSurface);
            }
            hasActiveSurface = false;
        };

        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
            const int fv = shape.mesh.num_face_vertices[f];
            if (fv < 3) {
                index_offset += static_cast<size_t>(std::max(fv, 0));
                continue;
            }

            int matId = shape.mesh.material_ids.size() > f ? shape.mesh.material_ids[f] : -1;
            std::shared_ptr<GLTFMaterial> mat = defaultMat;
            if (matId >= 0 && static_cast<size_t>(matId) < materialsTiny.size()) {
                mat = materials[static_cast<size_t>(matId) + 1];
            }

            if (!hasActiveSurface || matId != activeMatId) {
                flushActiveSurface();
                activeMatId = matId;
                hasActiveSurface = true;
                activeSurface = {};
                activeSurface.startIndex = static_cast<uint32_t>(indices.size());
                activeSurface.material = mat;
                const float maxF = std::numeric_limits<float>::max();
                const float minF = std::numeric_limits<float>::lowest();
                activeMin = glm::vec3(maxF, maxF, maxF);
                activeMax = glm::vec3(minF, minF, minF);
            }

            std::vector<uint32_t> faceVertexIndices;
            faceVertexIndices.reserve(static_cast<size_t>(fv));
            bool faceHasNormals = true;

            for (int v = 0; v < fv; v++) {
                const tinyobj::index_t idx = shape.mesh.indices[index_offset + static_cast<size_t>(v)];
                if (idx.vertex_index < 0) {
                    continue;
                }

                const size_t posBase = static_cast<size_t>(idx.vertex_index) * 3;
                if (posBase + 2 >= attrib.vertices.size()) {
                    continue;
                }

                Vertex vert{};
                vert.position = {
                    attrib.vertices[posBase + 0],
                    attrib.vertices[posBase + 1],
                    attrib.vertices[posBase + 2]
                };
                if (idx.normal_index >= 0 && !attrib.normals.empty()) {
                    const size_t normalBase = static_cast<size_t>(idx.normal_index) * 3;
                    if (normalBase + 2 < attrib.normals.size()) {
                        vert.normal = glm::vec3(
                            attrib.normals[normalBase + 0],
                            attrib.normals[normalBase + 1],
                            attrib.normals[normalBase + 2]);
                    } else {
                        faceHasNormals = false;
                        vert.normal = glm::vec3(0.0f);
                    }
                } else {
                    faceHasNormals = false;
                    vert.normal = glm::vec3(0.0f);
                }
                if (idx.texcoord_index >= 0 && !attrib.texcoords.empty()) {
                    const size_t uvBase = static_cast<size_t>(idx.texcoord_index) * 2;
                    if (uvBase + 1 < attrib.texcoords.size()) {
                        vert.uv_x = attrib.texcoords[uvBase + 0];
                        vert.uv_y = 1.0f - attrib.texcoords[uvBase + 1];
                    } else {
                        vert.uv_x = 0.0f;
                        vert.uv_y = 0.0f;
                    }
                }
                else {
                    vert.uv_x = 0.0f;
                    vert.uv_y = 0.0f;
                }
                vert.color = glm::vec4(1.0f);

                vertices.push_back(vert);
                const uint32_t newIndex = static_cast<uint32_t>(vertices.size() - 1);
                faceVertexIndices.push_back(newIndex);
                activeMin = glm::min(activeMin, vert.position);
                activeMax = glm::max(activeMax, vert.position);
            }
            index_offset += static_cast<size_t>(fv);

            if (faceVertexIndices.size() < 3) {
                continue;
            }

            if (!faceHasNormals) {
                const glm::vec3 p0 = vertices[faceVertexIndices[0]].position;
                const glm::vec3 p1 = vertices[faceVertexIndices[1]].position;
                const glm::vec3 p2 = vertices[faceVertexIndices[2]].position;
                glm::vec3 n = glm::cross(p1 - p0, p2 - p0);
                const float nLen = glm::length(n);
                if (nLen > 1e-6f) {
                    n /= nLen;
                } else {
                    n = glm::vec3(0.0f, 1.0f, 0.0f);
                }
                for (uint32_t vtxIdx : faceVertexIndices) {
                    vertices[vtxIdx].normal = n;
                }
            }

            for (size_t tri = 1; tri + 1 < faceVertexIndices.size(); ++tri) {
                indices.push_back(faceVertexIndices[0]);
                indices.push_back(faceVertexIndices[tri]);
                indices.push_back(faceVertexIndices[tri + 1]);
            }
        }
        flushActiveSurface();

        if (indices.empty() || vertices.empty() || newmesh->surfaces.empty()) {
            meshes.pop_back();
            file.meshes.erase(meshName);
            continue;
        }

        newmesh->meshBuffers = engine->uploadMesh(indices, vertices);
    }

    for (const auto& mesh : meshes) {
        std::shared_ptr<Node> newNode = std::make_shared<MeshNode>();
        newNode->localTransform = glm::mat4(1.0f);
        newNode->worldTransform = glm::mat4(1.0f);
        static_cast<MeshNode*>(newNode.get())->mesh = mesh;
        nodes.push_back(newNode);
        file.nodes[mesh->name.c_str()] = newNode;
    }

    for (auto& node : nodes) {
        file.topNodes.push_back(node);
        node->refreshTransform(glm::mat4{ 1.f });
    }
    file.indexedNodes = nodes;

    return scene;
}

std::optional<std::shared_ptr<LoadedGLTF>> loadSceneAsset(VulkanEngine* engine, std::string_view filePath) {
    const std::string path(filePath);
    std::filesystem::path p(path);
    const std::string ext = toLower(p.extension().string());

    if (ext == ".gltf" || ext == ".glb") {
        return loadGltf(engine, path);
    }
    if (ext == ".obj") {
        return loadObj(engine, path);
    }
    if (ext == ".mtl") {
        std::filesystem::path objCandidate = p.parent_path() / (p.stem().string() + ".obj");
        if (std::filesystem::exists(objCandidate)) {
            return loadObj(engine, objCandidate.string());
        }
        Yalaz::UI::Console::Warn("MTL selected but matching OBJ not found: " + objCandidate.string());
        return {};
    }
    if (ext == ".fbx" || ext == ".dae") {
        std::filesystem::path convertedPath;
        if (convertModelToGltf(p, convertedPath)) {
            const size_t clipCountBefore = engine->animationClips.size();
            const size_t skeletonCountBefore = engine->skeletons.size();

            auto loaded = loadGltf(engine, convertedPath.string());
            if (loaded.has_value()) {
                // FBX/DAE is converted into temp GLTF/GLB files, but runtime scene identity
                // is tracked by the original source path. Remap imported animation/skeleton
                // source path to the original file so animation-node matching works.
                const std::string originalSource = std::filesystem::absolute(p).string();
                for (size_t i = clipCountBefore; i < engine->animationClips.size(); ++i) {
                    engine->animationClips[i].sourceScene = originalSource;
                    for (auto& tr : engine->animationClips[i].tracks) {
                        tr.sourceScene = originalSource;
                    }
                }
                for (size_t i = skeletonCountBefore; i < engine->skeletons.size(); ++i) {
                    engine->skeletons[i].sourceScene = originalSource;
                }

                const size_t newClips = engine->animationClips.size() - clipCountBefore;
                const size_t newSkels = engine->skeletons.size() - skeletonCountBefore;
                if (newClips > 0 || newSkels > 0) {
                    fmt::print("[Import] {} -> GLTF: {} animation(s), {} skeleton(s)\n",
                        p.filename().string(), newClips, newSkels);
                } else {
                    Yalaz::UI::Console::Warn(
                        ext + " conversion for '" + p.filename().string() +
                        "' produced no animation/skeleton data. "
                        "If the file contains animations, try converting to GLTF/GLB with Blender or another tool.");
                }
            }
            return loaded;
        }
        if (convertModelToObj(p, convertedPath)) {
            Yalaz::UI::Console::Warn(
                "FBX/DAE -> OBJ fallback used for: " + path +
                ". OBJ does not preserve skeletal animation; import as GLTF/GLB for animation.");
            // OBJ fallback does not carry skeleton/animation data.
            // Remove any stale animation/skeleton entries for this source scene to prevent
            // mismatched clips from driving unrelated nodes with the same filename.
            auto fileNameOnly = [](const std::string& in) -> std::string {
                size_t s1 = in.find_last_of('/');
                size_t s2 = in.find_last_of('\\');
                size_t pos = std::string::npos;
                if (s1 == std::string::npos) pos = s2;
                else if (s2 == std::string::npos) pos = s1;
                else pos = std::max(s1, s2);
                return (pos == std::string::npos) ? in : in.substr(pos + 1);
            };

            const std::string sourceFile = fileNameOnly(path);
            engine->animationClips.erase(
                std::remove_if(engine->animationClips.begin(), engine->animationClips.end(),
                    [&](const AnimationClipData& c) {
                        if (c.sourceScene.empty()) return false;
                        if (c.sourceScene == path) return true;
                        return fileNameOnly(c.sourceScene) == sourceFile;
                    }),
                engine->animationClips.end());

            engine->skeletons.erase(
                std::remove_if(engine->skeletons.begin(), engine->skeletons.end(),
                    [&](const SkeletonData& s) {
                        if (s.sourceScene.empty()) return false;
                        if (s.sourceScene == path) return true;
                        return fileNameOnly(s.sourceScene) == sourceFile;
                    }),
                engine->skeletons.end());

            if (engine->animationClips.empty()) engine->activeAnimationIndex = -1;
            else if (engine->activeAnimationIndex >= static_cast<int>(engine->animationClips.size())) engine->activeAnimationIndex = 0;
            if (engine->skeletons.empty()) engine->activeSkeletonIndex = -1;
            else if (engine->activeSkeletonIndex >= static_cast<int>(engine->skeletons.size())) engine->activeSkeletonIndex = 0;

            return loadObj(engine, convertedPath.string());
        }
        Yalaz::UI::Console::Error(
            "FBX/DAE conversion failed for: " + path +
            ". Install Assimp CLI (`assimp`) or place assimp.exe under C:\\Program Files\\Assimp\\bin. "
            "You can also set ASSIMP_BIN or ASSIMP_ROOT.");
        return {};
    }

    Yalaz::UI::Console::Warn("Unsupported model format: " + path);
    return {};
}

bool generateModelPreviewRGBA(std::string_view filePath, int maxSize, std::vector<uint8_t>& outRGBA, int& outW, int& outH) {
    outRGBA.clear();
    outW = std::max(64, maxSize);
    outH = outW;

    const std::string path(filePath);
    std::filesystem::path p(path);
    std::string ext = toLower(p.extension().string());

    std::vector<glm::vec3> points;
    if (ext == ".obj" || ext == ".mtl") {
        std::filesystem::path objPath = (ext == ".obj") ? p : (p.parent_path() / (p.stem().string() + ".obj"));
        if (!collectObjPoints(objPath.string(), points)) return false;
    } else if (ext == ".gltf" || ext == ".glb") {
        if (!collectGltfPoints(path, points)) return false;
    } else if (ext == ".fbx" || ext == ".dae") {
        std::filesystem::path convertedPath;
        if (convertModelToGltf(p, convertedPath)) {
            if (!collectGltfPoints(convertedPath.string(), points)) return false;
        } else if (convertModelToObj(p, convertedPath)) {
            if (!collectObjPoints(convertedPath.string(), points)) return false;
        } else {
            return false;
        }
    } else {
        return false;
    }

    if (points.empty()) return false;

    glm::vec3 minP(1e30f), maxP(-1e30f);
    for (const auto& v : points) {
        minP = glm::min(minP, v);
        maxP = glm::max(maxP, v);
    }
    glm::vec3 center = (minP + maxP) * 0.5f;
    glm::vec3 extents = glm::max((maxP - minP) * 0.5f, glm::vec3(1e-5f));
    float maxE = std::max(extents.x, std::max(extents.y, extents.z));
    if (maxE <= 1e-6f) maxE = 1.0f;

    glm::mat4 rotY = glm::rotate(glm::mat4(1.0f), glm::radians(32.0f), glm::vec3(0, 1, 0));
    glm::mat4 rotX = glm::rotate(glm::mat4(1.0f), glm::radians(-20.0f), glm::vec3(1, 0, 0));
    glm::mat4 rot = rotY * rotX;

    outRGBA.assign(static_cast<size_t>(outW) * static_cast<size_t>(outH) * 4u, 0);
    for (int y = 0; y < outH; ++y) {
        float t = static_cast<float>(y) / static_cast<float>(std::max(1, outH - 1));
        uint8_t r = static_cast<uint8_t>(26 + t * 18);
        uint8_t g = static_cast<uint8_t>(32 + t * 20);
        uint8_t b = static_cast<uint8_t>(42 + t * 24);
        for (int x = 0; x < outW; ++x) {
            size_t idx = static_cast<size_t>(y) * outW * 4u + static_cast<size_t>(x) * 4u;
            outRGBA[idx + 0] = r;
            outRGBA[idx + 1] = g;
            outRGBA[idx + 2] = b;
            outRGBA[idx + 3] = 255;
        }
    }

    auto putPx = [&](int x, int y, uint8_t r, uint8_t g, uint8_t b) {
        if (x < 0 || y < 0 || x >= outW || y >= outH) return;
        size_t idx = static_cast<size_t>(y) * outW * 4u + static_cast<size_t>(x) * 4u;
        outRGBA[idx + 0] = r;
        outRGBA[idx + 1] = g;
        outRGBA[idx + 2] = b;
        outRGBA[idx + 3] = 255;
    };

    // Draw a dense point-cloud style preview.
    for (const auto& p3 : points) {
        glm::vec3 n = (p3 - center) / maxE;
        glm::vec4 rp = rot * glm::vec4(n, 1.0f);
        float sx = (rp.x * 0.42f + 0.5f) * static_cast<float>(outW);
        float sy = (0.5f - rp.y * 0.42f) * static_cast<float>(outH);
        int x = static_cast<int>(sx);
        int y = static_cast<int>(sy);
        uint8_t shade = static_cast<uint8_t>(std::clamp(160.0f + rp.z * 60.0f, 90.0f, 240.0f));
        putPx(x, y, shade, static_cast<uint8_t>(shade + 8), static_cast<uint8_t>(shade + 18));
        putPx(x + 1, y, shade, static_cast<uint8_t>(shade + 8), static_cast<uint8_t>(shade + 18));
        putPx(x, y + 1, shade, static_cast<uint8_t>(shade + 8), static_cast<uint8_t>(shade + 18));
    }

    return true;
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
    if (!creator) return;

    VkDevice dv = creator->_device;
    // IMPORTANT:
    // Scene unload is deferred and executed after per-frame fence wait
    // (VulkanEngine::processPendingSceneUnloads). Do not block the whole device here.

    // Clear all shared_ptr references first to prevent dangling pointers
    topNodes.clear();
    nodes.clear();
    materials.clear();

    // Destroy mesh buffers
    for (auto& [k, v] : meshes) {
        if (v) {
            if (v->meshBuffers.indexBuffer.buffer != VK_NULL_HANDLE) {
                creator->destroy_buffer(v->meshBuffers.indexBuffer);
            }
            if (v->meshBuffers.vertexBuffer.buffer != VK_NULL_HANDLE) {
                creator->destroy_buffer(v->meshBuffers.vertexBuffer);
            }
            if (v->skinBuffer.buffer != VK_NULL_HANDLE) {
                creator->destroy_buffer(v->skinBuffer);
                v->skinBuffer = {};
                v->skinBufferAddress = 0;
                v->hasSkinData = false;
            }
        }
    }
    meshes.clear();

    // Invalidate texture cache entries that reference this scene's images
    // MUST happen BEFORE destroying the actual VkImages/VkImageViews
    {
        std::vector<VkImageView> deadViews;
        for (auto& [k, v] : images) {
            if (v.image == VK_NULL_HANDLE) continue;
            if (v.image == creator->_errorCheckerboardImage.image) continue;
            if (v.image == creator->_whiteImage.image) continue;
            if (v.imageView != VK_NULL_HANDLE) {
                deadViews.push_back(v.imageView);
            }
        }
        if (!deadViews.empty()) {
            creator->texCache.InvalidateImageViews(
                deadViews,
                creator->_whiteImage.imageView,
                creator->_defaultSamplerLinear);
        }
    }

    // Destroy images (skip default/error images)
    for (auto& [k, v] : images) {
        if (v.image == VK_NULL_HANDLE) continue;
        if (v.image == creator->_errorCheckerboardImage.image) continue;
        if (v.image == creator->_whiteImage.image) continue;
        creator->destroy_image(v);
    }
    images.clear();

    // Destroy samplers (only once)
    for (auto& sampler : samplers) {
        if (sampler != VK_NULL_HANDLE) {
            vkDestroySampler(dv, sampler, nullptr);
        }
    }
    samplers.clear();

    // NOTE:
    // Do not destroy scene descriptor pools immediately here.
    // On some frame overlap / submission orders, descriptor sets can still be
    // referenced by pending command buffers even after per-frame fence waits.
    // Immediate pool destruction then causes invalid-handle and in-use validation
    // errors when handles get recycled.
    //
    // Keeping these pools alive avoids use-after-free descriptor hazards.
    // (They are released when the process exits; this trades memory for safety.)
    // descriptorPool.destroy_pools(dv);

    // Destroy material data buffer
    if (materialDataBuffer.buffer != VK_NULL_HANDLE) {
        creator->destroy_buffer(materialDataBuffer);
        materialDataBuffer = {};
    }
}
