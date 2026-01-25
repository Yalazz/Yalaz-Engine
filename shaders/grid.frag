#version 450

// =============================================================================
// PROFESSIONAL DYNAMIC GRID SHADER - Like Blender/Unity/Unreal
// All settings controlled via push constants from ImGui
// =============================================================================

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec3 nearPoint;
layout(location = 2) in vec3 farPoint;
layout(location = 3) in float fragCameraHeight;
layout(location = 4) in vec2 fragCameraPosXZ;  // Camera XZ position for fade calculation

layout(location = 0) out vec4 outColor;

// Push constants - ALL grid settings from CPU
layout(push_constant) uniform PushConstants {
    mat4 worldMatrix;
    vec4 gridParams;    // x=cellSize, y=fadeDistance, z=lineWidth, w=opacity
    vec4 gridParams2;   // x=dynamicLOD, y=showAxisColors, z=showSubdivisions, w=axisLineWidth
    vec4 gridParams3;   // x=lodBias, y=antiAliasing, z=minFadeAlpha, w=majorMultiplier
    vec4 minorColor;
    vec4 majorColor;
    vec4 xAxisColor;
    vec4 zAxisColor;
} pc;

// =============================================================================
// GRID LINE FUNCTIONS
// =============================================================================

float computeLODLevel(float cameraHeight, float baseCellSize, float lodBias) {
    float scaledHeight = cameraHeight / baseCellSize;
    float logDist = log(max(scaledHeight, 1.0)) / log(10.0);
    return max(0.0, logDist - 0.5 + lodBias);
}

float gridLine(vec2 uv, float gridSize, float lineWidth, bool useAA) {
    if (useAA) {
        vec2 dudv = fwidth(uv);
        vec2 uvMod = mod(uv, gridSize);
        vec2 uvDist = min(uvMod, gridSize - uvMod);
        vec2 line = smoothstep(vec2(0.0), dudv * lineWidth, uvDist);
        return 1.0 - min(line.x, line.y);
    } else {
        vec2 uvMod = mod(uv, gridSize);
        vec2 uvDist = min(uvMod, gridSize - uvMod);
        float threshold = gridSize * 0.02 * lineWidth;
        return (min(uvDist.x, uvDist.y) < threshold) ? 1.0 : 0.0;
    }
}

// =============================================================================
// MAIN GRID COMPUTATION
// =============================================================================

void main() {
    // === Read ALL settings from push constants ===
    float cellSize = pc.gridParams.x;
    float fadeDistance = pc.gridParams.y;
    float lineWidth = pc.gridParams.z;
    float opacity = pc.gridParams.w;

    bool dynamicLOD = pc.gridParams2.x > 0.5;
    bool showAxisColors = pc.gridParams2.y > 0.5;
    bool showSubdivisions = pc.gridParams2.z > 0.5;
    float axisLineWidth = pc.gridParams2.w;

    float lodBias = pc.gridParams3.x;
    bool antiAliasing = pc.gridParams3.y > 0.5;
    float minFadeAlpha = pc.gridParams3.z;
    float majorMultiplier = pc.gridParams3.w;

    vec3 minorLineColor = pc.minorColor.rgb;
    vec3 majorLineColor = pc.majorColor.rgb;
    vec3 xAxisCol = pc.xAxisColor.rgb;
    vec3 zAxisCol = pc.zAxisColor.rgb;

    // === Grid position ===
    vec2 gridPos = fragWorldPos.xz;
    float cameraHeight = max(abs(fragCameraHeight), 0.1);

    // === Compute grid ===
    float gridIntensity = 0.0;
    float isMajorLine = 0.0;
    vec3 gridColor = minorLineColor;

    if (dynamicLOD) {
        // Dynamic LOD - grid scales with camera distance
        float lod = computeLODLevel(cameraHeight, cellSize, lodBias);
        float lodFloor = floor(lod);
        float lodBlend = fract(lod);

        float gridSize0 = cellSize * pow(10.0, lodFloor);
        float gridSize1 = gridSize0 * majorMultiplier;
        float gridSize2 = gridSize1 * majorMultiplier;

        // Minor grid (finest level)
        float grid0 = 0.0;
        if (showSubdivisions) {
            grid0 = gridLine(gridPos, gridSize0, lineWidth, antiAliasing);
            grid0 *= (1.0 - lodBlend); // Fade out as we zoom out
        }

        // Major grid
        float grid1 = gridLine(gridPos, gridSize1, lineWidth * 1.3, antiAliasing);

        // Super-major grid
        float grid2 = gridLine(gridPos, gridSize2, lineWidth * 1.6, antiAliasing);

        isMajorLine = max(grid1, grid2);
        gridIntensity = max(grid0, max(grid1, grid2));
        gridColor = mix(minorLineColor, majorLineColor, isMajorLine);

    } else {
        // Fixed grid - no LOD
        float majorSize = cellSize * majorMultiplier;

        float minorGrid = 0.0;
        if (showSubdivisions) {
            minorGrid = gridLine(gridPos, cellSize, lineWidth, antiAliasing);
        }

        float majorGrid = gridLine(gridPos, majorSize, lineWidth * 1.5, antiAliasing);

        isMajorLine = majorGrid;
        gridIntensity = max(minorGrid, majorGrid);
        gridColor = mix(minorLineColor, majorLineColor, isMajorLine);
    }

    // === Axis lines ===
    if (showAxisColors) {
        vec2 dudv = antiAliasing ? fwidth(gridPos) : vec2(0.02);
        float axisWidth = axisLineWidth * lineWidth;

        // X axis (along X, at Z=0) - colored line
        float onXAxis = 1.0 - smoothstep(0.0, dudv.y * axisWidth, abs(gridPos.y));

        // Z axis (along Z, at X=0) - colored line
        float onZAxis = 1.0 - smoothstep(0.0, dudv.x * axisWidth, abs(gridPos.x));

        // Apply axis colors
        if (onXAxis > 0.01) {
            gridColor = mix(gridColor, xAxisCol, onXAxis * 0.9);
            gridIntensity = max(gridIntensity, onXAxis);
        }
        if (onZAxis > 0.01) {
            gridColor = mix(gridColor, zAxisCol, onZAxis * 0.9);
            gridIntensity = max(gridIntensity, onZAxis);
        }

        // Origin point (brighter)
        float atOrigin = onXAxis * onZAxis;
        if (atOrigin > 0.3) {
            gridColor = vec3(1.0, 1.0, 1.0);
            gridIntensity = 1.0;
        }
    }

    // === Distance fade ===
    // zAxisColor.a > 0.5 = fade from camera, else fade from origin
    bool fadeFromCamera = pc.zAxisColor.a > 0.5;
    vec2 fadeCenter = fadeFromCamera ? fragCameraPosXZ : vec2(0.0);
    float dist = length(gridPos - fadeCenter);
    float fadeStart = fadeDistance * 0.4;
    float distanceFade = 1.0 - smoothstep(fadeStart, fadeDistance, dist);
    distanceFade = max(distanceFade, minFadeAlpha);

    // === Height fade (for low angles) ===
    float heightFade = smoothstep(0.0, 3.0, cameraHeight);

    // === Final alpha ===
    float finalAlpha = gridIntensity * opacity * distanceFade * heightFade;

    if (finalAlpha < 0.001) {
        discard;
    }

    outColor = vec4(gridColor, finalAlpha);
}
