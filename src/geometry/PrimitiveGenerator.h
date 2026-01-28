#pragma once

#include "PrimitiveType.h"
#include "core/ISubsystem.h"
#include "vk_types.h"  // For Vertex, GPUMeshBuffers
#include <vector>
#include <span>
#include <glm/glm.hpp>
#include <functional>

namespace Yalaz::Geometry {

/**
 * @brief Raw mesh data (CPU-side) before GPU upload
 */
struct MeshData {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};

/**
 * @brief Function type for uploading mesh data to GPU
 * This allows PrimitiveGenerator to be decoupled from Vulkan
 */
using MeshUploadFunc = std::function<GPUMeshBuffers(std::span<uint32_t>, std::span<Vertex>)>;

/**
 * @brief Factory for generating procedural primitive meshes
 *
 * Design Patterns:
 * - Factory: Creates mesh data for different primitive types
 * - Strategy: Upload function can be swapped
 *
 * SOLID:
 * - Single Responsibility: Only generates geometry, doesn't upload
 * - Open/Closed: New primitives can be added without modifying existing
 */
class PrimitiveGenerator : public Core::ISubsystem {
public:
    static PrimitiveGenerator& Get() {
        static PrimitiveGenerator instance;
        return instance;
    }

    // ISubsystem interface
    void OnInit() override;
    void OnShutdown() override;
    const char* GetName() const override { return "PrimitiveGenerator"; }

    /**
     * @brief Set the function used to upload meshes to GPU
     * Must be set before generating GPU meshes
     */
    void SetUploadFunction(MeshUploadFunc func) { m_UploadFunc = func; }

    // =========================================================================
    // CPU-side mesh generation (returns raw vertex/index data)
    // =========================================================================

    MeshData GenerateTriangleData();
    MeshData GeneratePlaneData();
    MeshData GenerateCubeData();
    MeshData GenerateSphereData(int resolution = 24, int rings = 16);
    MeshData GenerateCylinderData(int segments = 32);
    MeshData GenerateConeData(int segments = 32);
    MeshData GenerateCapsuleData(int segments = 32, int rings = 8);
    MeshData GenerateTorusData(int segments = 32, int rings = 16);

    /**
     * @brief Generate mesh data for any primitive type
     * @param type The primitive type to generate
     * @return CPU-side mesh data
     */
    MeshData GenerateData(PrimitiveType type);

    // =========================================================================
    // GPU mesh generation (uploads to GPU via callback)
    // Requires SetUploadFunction() to be called first
    // =========================================================================

    GPUMeshBuffers GenerateTriangle();
    GPUMeshBuffers GeneratePlane();
    GPUMeshBuffers GenerateCube();
    GPUMeshBuffers GenerateSphere(int resolution = 24, int rings = 16);
    GPUMeshBuffers GenerateCylinder(int segments = 32);
    GPUMeshBuffers GenerateCone(int segments = 32);
    GPUMeshBuffers GenerateCapsule(int segments = 32, int rings = 8);
    GPUMeshBuffers GenerateTorus(int segments = 32, int rings = 16);

    /**
     * @brief Generate and upload mesh for any primitive type
     * @param type The primitive type to generate
     * @return GPU mesh buffers
     */
    GPUMeshBuffers Generate(PrimitiveType type);

    // =========================================================================
    // Grid generation
    // =========================================================================

    /**
     * @brief Generate a grid plane mesh
     * @param numX Number of grid cells in X direction
     * @param numZ Number of grid cells in Z direction
     * @param spacing Distance between grid lines
     * @return CPU-side mesh data
     */
    MeshData GenerateGridData(int numX, int numZ, float spacing);

private:
    PrimitiveGenerator() = default;
    ~PrimitiveGenerator() = default;

    MeshUploadFunc m_UploadFunc;

    // Helper to upload mesh data
    GPUMeshBuffers UploadMesh(MeshData& data);
};

} // namespace Yalaz::Geometry
