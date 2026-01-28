#pragma once

namespace Yalaz::Geometry {

/**
 * @brief Types of procedurally generated primitive meshes
 *
 * Used by PrimitiveGenerator to create common geometric shapes.
 */
enum class PrimitiveType {
    Cube = 0,
    Sphere,
    Capsule,
    Cylinder,
    Plane,
    Cone,
    Torus,
    Triangle,
    Count  // Number of primitive types
};

/**
 * @brief Get human-readable name for a primitive type
 * @param type The primitive type
 * @return String name of the primitive
 */
inline const char* GetPrimitiveTypeName(PrimitiveType type) {
    switch (type) {
        case PrimitiveType::Cube:     return "Cube";
        case PrimitiveType::Sphere:   return "Sphere";
        case PrimitiveType::Capsule:  return "Capsule";
        case PrimitiveType::Cylinder: return "Cylinder";
        case PrimitiveType::Plane:    return "Plane";
        case PrimitiveType::Cone:     return "Cone";
        case PrimitiveType::Torus:    return "Torus";
        case PrimitiveType::Triangle: return "Triangle";
        default:                      return "Unknown";
    }
}

} // namespace Yalaz::Geometry

// Backwards compatibility - alias to old namespace
// TODO: Remove after full migration
using PrimitiveType = Yalaz::Geometry::PrimitiveType;
