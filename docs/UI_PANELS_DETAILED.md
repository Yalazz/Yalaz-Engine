# UI Panels Detailed Guide

This document describes what each editor panel is for, what it edits/displays, and where to modify behavior in code.

## Core Panels

### Scene
- Type: `SceneView`
- Files: `src/ui/views/SceneView.cpp`, `src/ui/views/SceneView.h`
- Purpose:
  - Main viewport for scene interaction.
  - View mode switching (solid/shaded/rendered/wireframe/normals/uv/path traced).
  - Scene overlays and viewport controls.
- Engine links:
  - `VulkanEngine::_currentViewMode`
  - Camera state and viewport rendering path.

### Game
- Type: `GameView`
- Files: `src/ui/views/GameView.cpp`, `src/ui/views/GameView.h`
- Purpose:
  - Game-like preview window.
  - Runtime-style visualization without full editor clutter.

### Hierarchy
- Type: `HierarchyView`
- Files: `src/ui/views/HierarchyView.cpp`, `src/ui/views/HierarchyView.h`
- Purpose:
  - Lists loaded scenes, nodes, cameras, lights, primitives.
  - Selection entry point for inspector and gizmo.
  - Add/remove/duplicate object actions.
- Engine links:
  - `selectedNode`, `selectedPrimitiveIndex`, `selectedLightIndex`
  - `loadedScenes`, `static_shapes`, `scenePointLights`

### Inspector
- Type: `InspectorView` (implemented by `ObjectInspectorView`)
- Files: `src/ui/views/ObjectInspectorView.cpp`, `src/ui/views/ObjectInspectorView.h`
- Purpose:
  - Contextual editing for selected primitive/light/node/GLTF material surface.
  - Transform editing, material data, camera/light data, animation/skeleton sections.
- Engine links:
  - Selection fields + animation/skeleton runtime data.

### Asset Browser
- Type: `AssetBrowserView`
- Files: `src/ui/views/AssetBrowserView.cpp`, `src/ui/views/AssetBrowserView.h`
- Purpose:
  - Browse assets and directories.
  - Scene loading from model files.
  - Texture/HDRI thumbnail previews and drag/drop.
  - Loaded scene management (focus/unload).
- Recent updates:
  - HDR/EXR preview tone-mapping path.
  - Direct path box + `Go` navigation.

## Debug Panels

### Console
- Type: `ConsoleView`
- Files: `src/ui/views/ConsoleView.cpp`, `src/ui/views/ConsoleView.h`
- Purpose:
  - Runtime logs, warnings, and errors.
  - Filtering and log inspection.

### Profiler
- Type: `ProfilerView`
- Files: `src/ui/views/ProfilerView.cpp`, `src/ui/views/ProfilerView.h`
- Purpose:
  - Performance counters and frame timing summaries.

### Shader Debug
- Type: `ShaderDebugView`
- Files: `src/ui/views/ShaderDebugView.cpp`, `src/ui/views/ShaderDebugView.h`
- Purpose:
  - Displays tracked shader/pipeline entries.
  - Recompile actions and compile status/error logs.
- Engine links:
  - `VulkanEngine::shaderPipelines`
  - `recompileShader()`, `recompileAllShaders()`

### GPU Debug
- Type: `GPUDebugView`
- Files: `src/ui/views/GPUDebugView.cpp`, `src/ui/views/GPUDebugView.h`
- Purpose:
  - Draw-call/memory/counter inspection.
  - Render-target visualization tab (live images).
- Engine links:
  - `_drawImage`, `_depthImage`, `_gBufferNormals`, `_gBufferMetalRough`

### Physics Debug
- Type: `PhysicsDebugView`
- Files: `src/ui/views/PhysicsDebugView.cpp`, `src/ui/views/PhysicsDebugView.h`
- Purpose:
  - Physics world/body debug visualization and tuning values.

## Graphics Panels

### Material Editor
- Type: `MaterialView`
- Files: `src/ui/views/MaterialView.cpp`, `src/ui/views/MaterialView.h`
- Purpose:
  - Primitive and GLTF material editing.
  - PBR controls, textures, alpha/transparency options, presets.
  - Syncs material changes with selected object/surface.

### Texture Inspector
- Type: `TextureView`
- Files: `src/ui/views/TextureView.cpp`, `src/ui/views/TextureView.h`
- Purpose:
  - Texture list/metadata preview by scene assets.
  - Channel and zoom controls.

### UV Editor
- Type: `UVView`
- Files: `src/ui/views/UVView.cpp`, `src/ui/views/UVView.h`
- Purpose:
  - UV diagnostics for selected mesh/primitive.

### Lighting Debug
- Type: `LightingDebugView`
- Files: `src/ui/views/LightingDebugView.cpp`, `src/ui/views/LightingDebugView.h`
- Purpose:
  - Light list and per-light editing.
  - Shadow toggles, bias tuning, visualization helpers.

### Render Settings
- Type: `RenderSettingsView`
- Files: `src/ui/views/RenderSettingsView.cpp`, `src/ui/views/RenderSettingsView.h`
- Purpose:
  - Global render/post-process/environment controls.
  - Bloom/tonemap/SSR/IBL/path tracing and cubemap workflows.

## Animation Panels

### Animation
- Type: `AnimationView`
- Files: `src/ui/views/AnimationView.cpp`, `src/ui/views/AnimationView.h`
- Purpose:
  - Animation clips, timeline, tracks, playback controls.
  - Animation graph/state transition editing and runtime state display.
  - Bone selection and hierarchy-related UI.

### Camera Sequencer
- Type: `CameraSequencerView`
- Files: `src/ui/views/CameraSequencerView.cpp`, `src/ui/views/CameraSequencerView.h`
- Purpose:
  - Keyframed camera sequencing/timeline style editing.

## System Panels

### Settings
- Type: `SettingsView`
- Files: `src/ui/views/SettingsView.cpp`, `src/ui/views/SettingsView.h`
- Purpose:
  - Engine/editor/system settings profiles and quality tuning.
  - Applies runtime graphics/shadow/editor toggles.

### Plugins
- Type: `PluginManagerView`
- Files: `src/ui/views/PluginManagerView.cpp`, `src/ui/views/PluginManagerView.h`
- Purpose:
  - Plugin list/state management UI.

## Operational Notes

1. Panel registration:
   - `src/ui/views/Views.h`
2. Panel lifetime and menu visibility:
   - `src/ui/views/ViewManager.cpp`
3. Editor integration entry:
   - `src/ui/EditorUI.cpp`
4. Shared view base behavior:
   - `src/ui/views/EditorView.h`

