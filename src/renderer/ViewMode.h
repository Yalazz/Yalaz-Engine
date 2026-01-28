#pragma once

namespace Yalaz::Renderer {

/**
 * @brief Rendering view modes for different visualization styles
 *
 * Strategy Pattern: Each mode represents a different rendering strategy
 */
enum class ViewMode {
    Solid = 0,             // Flat color, no lighting - fastest
    Shaded = 1,            // Hemisphere + N·L studio lighting
    MaterialPreview = 2,   // IBL-based material preview
    Rendered = 3,          // Full PBR with scene lights
    Wireframe = 4,         // Edge visualization
    Normals = 5,           // World-space normals as RGB
    UVChecker = 6,         // UV checker pattern debug
    PathTraced = 7,        // Real-time path tracing (compute shader)
    Count                  // Number of view modes
};

/**
 * @brief Get human-readable name for a view mode
 */
inline const char* GetViewModeName(ViewMode mode) {
    switch (mode) {
        case ViewMode::Solid:           return "Solid";
        case ViewMode::Shaded:          return "Shaded";
        case ViewMode::MaterialPreview: return "Material Preview";
        case ViewMode::Rendered:        return "Rendered";
        case ViewMode::Wireframe:       return "Wireframe";
        case ViewMode::Normals:         return "Normals";
        case ViewMode::UVChecker:       return "UV Checker";
        case ViewMode::PathTraced:      return "Path Traced";
        default:                        return "Unknown";
    }
}

} // namespace Yalaz::Renderer

// Backwards compatibility alias
using ViewMode = Yalaz::Renderer::ViewMode;
