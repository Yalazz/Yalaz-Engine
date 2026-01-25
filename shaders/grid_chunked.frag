#version 450

// =============================================================================
// CHUNKED GRID FRAGMENT SHADER - High Performance
// =============================================================================
// Optimizations:
// - LOD-based complexity reduction
// - Early discard
// - Simplified calculations for distant chunks
// - No AA for far LODs
// =============================================================================

layout(location = 0) in vec2 fragGridPos;
layout(location = 1) in float fragLodLevel;
layout(location = 2) in float fragOpacity;
layout(location = 3) in float fragDistToCamera;

layout(location = 0) out vec4 outColor;

// Push constants
layout(push_constant) uniform PushConstants {
    vec4 gridParams;   // x=cellSize, y=fadeDistance, z=lineWidth, w=opacity
    vec4 gridParams2;  // x=majorMultiplier, yzw=reserved
    vec4 minorColor;
    vec4 majorColor;
    vec4 axisColors;
} pc;

// =============================================================================
// OPTIMIZED GRID LINE - Fast version for performance
// =============================================================================
float gridLineFast(vec2 uv, float gridSize, float lineWidth) {
    vec2 uvMod = mod(uv, gridSize);
    vec2 uvDist = min(uvMod, gridSize - uvMod);
    float threshold = gridSize * 0.015 * lineWidth;
    return (min(uvDist.x, uvDist.y) < threshold) ? 1.0 : 0.0;
}

// Anti-aliased version (only for close chunks)
float gridLineAA(vec2 uv, float gridSize, float lineWidth) {
    vec2 dudv = fwidth(uv);
    vec2 uvMod = mod(uv, gridSize);
    vec2 uvDist = min(uvMod, gridSize - uvMod);
    vec2 line = smoothstep(vec2(0.0), dudv * lineWidth, uvDist);
    return 1.0 - min(line.x, line.y);
}

void main() {
    // Early-out for very transparent
    if (fragOpacity < 0.01) {
        discard;
    }

    float cellSize = pc.gridParams.x;
    float lineWidth = pc.gridParams.z;
    float majorMult = pc.gridParams2.x;

    vec3 gridColor;
    float gridIntensity = 0.0;

    // LOD 0: Full detail with AA and subdivisions
    if (fragLodLevel < 0.5) {
        float majorSize = cellSize * majorMult;

        // Minor grid (subdivisions)
        float minorGrid = gridLineAA(fragGridPos, cellSize, lineWidth);

        // Major grid
        float majorGrid = gridLineAA(fragGridPos, majorSize, lineWidth * 1.5);

        gridIntensity = max(minorGrid * 0.5, majorGrid);
        gridColor = mix(pc.minorColor.rgb, pc.majorColor.rgb, majorGrid);
    }
    // LOD 1: Major lines only, with AA
    else if (fragLodLevel < 1.5) {
        float majorSize = cellSize * majorMult;
        gridIntensity = gridLineAA(fragGridPos, majorSize, lineWidth * 1.3);
        gridColor = pc.majorColor.rgb;
    }
    // LOD 2+: Super-major lines only, no AA (fastest)
    else {
        float superMajorSize = cellSize * majorMult * majorMult;
        gridIntensity = gridLineFast(fragGridPos, superMajorSize, lineWidth);
        gridColor = pc.majorColor.rgb;
    }

    // Axis lines (only for close chunks)
    if (fragLodLevel < 1.5) {
        float axisWidth = lineWidth * 2.0;

        // X axis (red) - at Z = 0
        if (abs(fragGridPos.y) < axisWidth * cellSize * 0.05) {
            gridColor = vec3(pc.axisColors.x, pc.axisColors.y, 0.2);
            gridIntensity = max(gridIntensity, 0.9);
        }

        // Z axis (blue) - at X = 0
        if (abs(fragGridPos.x) < axisWidth * cellSize * 0.05) {
            gridColor = vec3(0.2, pc.axisColors.z, pc.axisColors.w);
            gridIntensity = max(gridIntensity, 0.9);
        }
    }

    // Final alpha with distance fade
    float distFade = 1.0 - smoothstep(pc.gridParams.y * 0.5, pc.gridParams.y, fragDistToCamera);
    float finalAlpha = gridIntensity * fragOpacity * distFade;

    // Discard transparent pixels early
    if (finalAlpha < 0.005) {
        discard;
    }

    outColor = vec4(gridColor, finalAlpha);
}
