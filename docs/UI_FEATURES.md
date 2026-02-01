# Yalaz Engine UI Features Documentation

## Overview
This document describes all UI features, their expected behavior, and how they work with different object types (Static Primitives, GLTF/GLB Scenes, Lights).

---

## FEATURE STATUS SUMMARY

### ALL 18 VIEWS NOW CONNECTED TO ENGINE

### FULLY WORKING (Connected to Engine)
| View | Status | Description |
|------|--------|-------------|
| HierarchyView | WORKING | Primitive creation, selection, visibility, context menus |
| ObjectInspectorView | WORKING | Transform/color/material editing for primitives, lights, GLTF nodes |
| ConsoleView | WORKING | Real FPS, stats, log filtering |
| ProfilerView | WORKING | Real performance data from engine |
| SceneView | WORKING | View mode, grid, outline toggles, snap settings |
| AssetBrowserView | WORKING | GLTF/GLB loading, file browsing, scene management |

### CONNECTED VIEWS (Using Real Engine Data)
| View | Status | Description |
|------|--------|-------------|
| GPUDebugView | CONNECTED | Real draw calls, triangles, memory from engine |
| MaterialView | CONNECTED | Syncs with selected primitive, applies colors |
| LightingDebugView | CONNECTED | Real light list from scenePointLights |
| SettingsView | CONNECTED | Grid/camera settings apply to engine |
| TextureView | CONNECTED | Shows textures from loaded GLTF scenes |
| GameView | CONNECTED | Scene preview with primitives, lights, FPS display |
| ShaderDebugView | CONNECTED | Shows registered shader pipelines from engine |
| AnimationView | CONNECTED | Syncs with engine animation clips and skeletons |
| PhysicsDebugView | CONNECTED | Syncs with engine physics bodies and primitives |
| PluginManagerView | CONNECTED | Shows engine subsystems as plugins |
| UVView | CONNECTED | Shows UV data from selected primitive/mesh |
| CameraSequencerView | CONNECTED | Controls engine camera during playback |

---

## 1. HIERARCHY VIEW (FULLY WORKING)

### 1.1 Static Primitives
| Feature | Description | Status |
|---------|-------------|--------|
| Create Primitive | Click "+ Create Primitive" or use Create menu | WORKS |
| 8 Primitive Types | Cube, Sphere, Cylinder, Cone, Capsule, Torus, Plane, Triangle | WORKS |
| Quick Create | Create menu > Quick Create - spawns in front of camera | WORKS |
| Spawn Settings | Position, Rotation, Scale before creation | WORKS |
| Face Colors | Dynamic per-type: Cube=6, Sphere=4, Cylinder=3, etc. | WORKS |
| Color Presets | Default, Rainbow, White, Random | WORKS |
| Selection | Click to select, highlights in list | WORKS |
| Visibility Toggle | [V]/[H] button to show/hide | WORKS |
| Search Filter | Type in search box to filter by name | WORKS |
| Type Filter | Dropdown to filter by primitive type | WORKS |
| Context Menu | Right-click for Focus Camera, Duplicate, Delete | WORKS |
| Stats Bar | Shows count of primitives, lights, scenes | WORKS |

### 1.2 GLTF/GLB Scene Nodes
| Feature | Description | Status |
|---------|-------------|--------|
| Display Loaded Scenes | Tree view of all loaded GLTF scenes | WORKS |
| Node Hierarchy | Expandable tree of scene nodes | WORKS |
| Node Selection | Click to select, shows in Inspector | WORKS |
| Context Menu | Right-click for Focus Camera | WORKS |
| Search Filter | Filters scene nodes by name | WORKS |

### 1.3 Point Lights
| Feature | Description | Status |
|---------|-------------|--------|
| Display Lights | Lists all point lights in scene | WORKS |
| Light Selection | Click to select, shows in Inspector | WORKS |
| Color Indicator | Light name colored by light color | WORKS |
| Add Light Button | Creates new light at camera position | WORKS |
| Context Menu | Focus Camera, Duplicate, Delete | WORKS |

---

## 2. OBJECT INSPECTOR VIEW (FULLY WORKING)

### 2.1 Static Primitive Inspector
| Feature | Description | Status |
|---------|-------------|--------|
| Name Editing | Editable text field for object name | WORKS |
| Type Display | Shows primitive type (Cube, Sphere, etc.) | WORKS |
| Visibility Toggle | Checkbox to show/hide object | WORKS |
| Focus Camera | Button to move camera to object | WORKS |
| **Transform Section** | | |
| Position Editing | X/Y/Z drag fields with colored axes | WORKS |
| Rotation Editing | X/Y/Z drag fields in degrees | WORKS |
| Scale Editing | X/Y/Z drag fields | WORKS |
| Reset Buttons | Reset Position/Rotation/Scale individually | WORKS |
| Snap to Grid | Grid 1, 0.5, 0.25 snap buttons | WORKS |
| **Material Section** | | |
| Main Color | RGBA color picker with alpha | WORKS |
| Material Type | Dropdown: Default, Unlit, PBR, Normal Debug, Wireframe | WORKS |
| Pass Type | Dropdown: Main Color, Shadow, Wireframe, Selection | WORKS |
| Color Presets | Red, Green, Blue, White, Gray, Gold, Silver, Bronze | WORKS |
| **Face Colors Section** | | |
| Use Face Colors | Checkbox to enable per-face coloring | WORKS |
| Dynamic Faces | Shows correct face count per type | WORKS |
| Face Presets | Default, Rainbow, White, Random, Gradients | WORKS |
| **Statistics Section** | | |
| Triangle Count | Approximate triangle count for type | WORKS |
| Vertex Count | Approximate vertex count | WORKS |
| Bounding Box | Min/Max/Size display | WORKS |
| **Actions Section** | | |
| Focus Camera | Moves camera to object | WORKS |
| Duplicate | Creates copy offset by (1, 0, 1) | WORKS |
| Reset Transform | Resets position/rotation/scale | WORKS |
| Move to Origin | Sets position to (0, 0, 0) | WORKS |
| Delete | Removes object from scene | WORKS |
| **Debug Info** | | |
| Index/Type ID | Internal identifiers | WORKS |
| Transform Matrix | 4x4 matrix preview | WORKS |
| Memory Address | Pointer for debugging | WORKS |

### 2.2 GLTF/GLB Node Inspector
| Feature | Description | Status |
|---------|-------------|--------|
| Node Name | Display name from scene | WORKS |
| Focus Camera | Move camera to node position | WORKS |
| **Transform Editing** | Position/Rotation/Scale - FULLY EDITABLE | **WORKS** |
| Reset Position | Reset node position to origin | WORKS |
| Reset Rotation | Reset node rotation | WORKS |
| Reset Scale | Reset node scale to 1 | WORKS |
| Snap to Grid | Grid 1, Grid 0.5 snap buttons | WORKS |
| Mesh Info | Has mesh, surface count, triangle count | WORKS |
| Children Info | Number of child nodes | WORKS |
| **Actions Section** | | |
| Move to Origin | Move node to world origin | WORKS |
| Reset Transform | Reset all transforms to identity | WORKS |
| Uniform Scale | 0.5x, 2x, 0.1x, 10x quick scale buttons | WORKS |
| World Transform Matrix | Full 4x4 matrix display | WORKS |

### 2.3 Light Inspector
| Feature | Description | Status |
|---------|-------------|--------|
| Position Editing | X/Y/Z drag fields | WORKS |
| Move Buttons | Move to Origin, Above Camera | WORKS |
| Color Editing | RGB color picker | WORKS |
| Intensity Slider | 0-100 range | WORKS |
| Radius Slider | 0.1-100 range | WORKS |
| Attenuation Preview | Shows light intensity at distances | WORKS |
| **Light Presets** | | |
| Standard | Warm White, Cool White, Neutral | WORKS |
| Natural | Sun, Moonlight, Candle, Fire | WORKS |
| Artificial | Neon Pink/Blue/Green, LED, Halogen | WORKS |
| Dramatic | Blood Red, Toxic, Purple Haze | WORKS |
| **Actions** | | |
| Focus Camera | Move camera to light | WORKS |
| Duplicate | Create copy of light | WORKS |
| Move to Camera | Move light to camera position | WORKS |
| Delete | Remove light | WORKS |

---

## 3. SCENE VIEW (FULLY WORKING)

| Feature | Description | Status |
|---------|-------------|--------|
| View Mode Dropdown | Solid, Shaded, Material, Rendered, Wireframe, Normals, UV | WORKS |
| Grid Toggle | Show/hide ground grid | WORKS |
| Outline Toggle | Show/hide selection outlines | WORKS |
| Snap Toggle | Enable/disable position snap | WORKS |
| Snap Value | Drag to adjust snap increment | WORKS |
| Snap Presets | Quick buttons for 0.1, 0.25, 0.5, 1.0, 2.0 | WORKS |
| Stats Button | Toggle stats overlay | WORKS |
| Settings Button | Open settings popup | WORKS |
| **Stats Overlay** | | |
| FPS Display | Current frames per second | WORKS |
| Frame Time | Milliseconds per frame | WORKS |
| Draw Calls | From engine stats | WORKS |
| Triangles | From engine stats | WORKS |
| **Settings Popup** | | |
| Grid Settings | Enable/disable, presets | WORKS |
| Snap Settings | Position, Rotation, Scale snap values | WORKS |
| Camera Settings | Position, rotation controls | WORKS |

---

## 4. ASSET BROWSER VIEW (FULLY WORKING)

| Feature | Description | Status |
|---------|-------------|--------|
| Directory Navigation | Browse folders with Up/Home buttons | WORKS |
| Path Bar | Clickable breadcrumb path | WORKS |
| Search Filter | Filter files by name | WORKS |
| Grid/List Toggle | Switch between view modes | WORKS |
| Thumbnail Size | Slider to adjust icon size | WORKS |
| **File Types** | | |
| Folders | [DIR] icon, click to enter | WORKS |
| 3D Models | [3D] icon for .gltf, .glb, .obj, .fbx | WORKS |
| Textures | [TEX] icon for .png, .jpg, .tga, .hdr | WORKS |
| Shaders | [SHD] icon for .glsl, .vert, .frag, .spv | WORKS |
| **GLTF Loading** | | |
| Click to Load | Single click loads GLTF/GLB into scene | WORKS |
| Duplicate Names | Auto-renames if scene name exists | WORKS |
| **Loaded Scenes Panel** | | |
| Scene List | Shows all loaded GLTF scenes | WORKS |
| Node Count | Displays number of nodes per scene | WORKS |
| Focus Button | Focus camera on scene | WORKS |
| Unload Button | Remove scene from memory | WORKS |
| **Context Menu** | | |
| Load Scene | Load GLTF/GLB file | WORKS |
| Show in Explorer | Open file location (Windows) | WORKS |
| Copy Path | Copy file path to clipboard | WORKS |

---

## 5. CONSOLE VIEW (FULLY WORKING)

| Feature | Description | Status |
|---------|-------------|--------|
| Stats Tab | Performance and scene statistics | WORKS |
| Logs Tab | Engine log messages | WORKS |
| **Stats Display** | | |
| FPS | Current and smoothed FPS | WORKS |
| Frame Time | Current frame time in ms | WORKS |
| FPS Graph | Historical FPS chart (120 samples) | WORKS |
| Primitive Count | Number of static shapes | WORKS |
| Light Count | Number of point lights | WORKS |
| Scene Count | Number of loaded GLTF scenes | WORKS |
| View Mode | Current rendering mode | WORKS |
| Camera Info | Position and orientation | WORKS |
| **Log Features** | | |
| Level Filters | Info/Warning/Error toggles | WORKS |
| Text Filter | Search in log messages | WORKS |
| Auto-scroll | Automatically scroll to newest | WORKS |
| Clear Button | Clear all logs | WORKS |
| Color Coding | Green=Info, Yellow=Warn, Red=Error | WORKS |

---

## 6. PROFILER VIEW (FULLY WORKING)

| Feature | Description | Status |
|---------|-------------|--------|
| Pause/Resume | Toggle profiling | WORKS |
| Reset Stats | Clear min/max/history | WORKS |
| CPU/GPU Toggle | Show/hide CPU/GPU bars | WORKS |
| FPS Display | Color-coded FPS in menu bar | WORKS |
| **Overview Tab** | | |
| Current/Avg/Min/Max FPS | Performance metrics | WORKS |
| FPS History Graph | 300 sample history | WORKS |
| Frame Time Graph | Frame time history | WORKS |
| **Timeline Tab** | | |
| CPU Bar | Visual CPU time | WORKS |
| GPU Bar | Visual GPU time | WORKS |
| 60 FPS Target Line | Reference line at 16.67ms | WORKS |
| **Statistics Tab** | | |
| Draw Calls | From engine stats | WORKS |
| Triangles | From engine stats | WORKS |
| Primitive Count | Static shape count | WORKS |
| Light Count | Point light count | WORKS |
| Scene Count | Loaded GLTF count | WORKS |
| **Memory Tab** | | |
| VRAM Usage | Placeholder progress bar | PLACEHOLDER |
| Texture Memory | Placeholder | PLACEHOLDER |
| Buffer Memory | Placeholder | PLACEHOLDER |

---

## 7. MATERIAL VIEW (CONNECTED)

**This view syncs with selected primitive and applies material changes.**

| Feature | Description | Status |
|---------|-------------|--------|
| Preview Panel | Simulated material preview (2D drawing) | WORKS |
| Preview Mesh | Sphere, Cube, Plane selector | WORKS |
| Auto-rotate | Toggle preview rotation | WORKS |
| Selection Sync | Syncs with selected primitive | WORKS |
| **Properties Tab** | | |
| Base Color | RGBA with alpha - applies to selection | WORKS |
| Metallic Slider | 0-100% with quick presets | WORKS |
| Roughness Slider | 0-100% with quick presets | WORKS |
| Quick Presets | Shiny, Matte, Metal, Plastic | WORKS |
| **Textures Tab** | | |
| Texture Slots | Store paths (GPU loading pending) | PARTIAL |
| **Presets Tab** | | |
| Material Presets | Gold, Silver, Copper, Plastics, etc. | WORKS |

---

## 8. GPU DEBUG VIEW (CONNECTED)

**Shows real data from engine primitives and loaded scenes.**

| Feature | Description | Status |
|---------|-------------|--------|
| Overview Tab | Real primitives, lights, scenes counts | WORKS |
| Draw Calls Tab | Real draw calls from primitives + GLTF meshes | WORKS |
| Memory Tab | Estimated memory from mesh data | WORKS |
| Counters Tab | Real triangle/vertex counts | WORKS |
| Frame Time History | Real performance from deltaTime | WORKS |

**Note:** GPU timing requires Vulkan query pools (not implemented).

---

## 9. ANIMATION VIEW (CONNECTED)

**Syncs with engine animation system (animationClips, skeletons).**

| Feature | Description | Status |
|---------|-------------|--------|
| Timeline | Shows engine animation clips and tracks | WORKS |
| State Machine | Animation states and transitions | WORKS |
| Skeleton Preview | Shows engine skeleton bone hierarchy | WORKS |
| Playback Controls | Play/Pause/Stop controls engine animation | WORKS |
| Add Keyframes | Add keyframes to animation tracks | WORKS |
| Events | Animation events at specific times | WORKS |

**Engine Systems Added:**
- `animationClips` - Vector of AnimationClipData
- `skeletons` - Vector of SkeletonData
- `playAnimation()`, `stopAnimation()`, `updateAnimations()` methods

---

## 9.5 LIGHTING DEBUG VIEW (CONNECTED)

**Shows real light data from scenePointLights.**

| Feature | Description | Status |
|---------|-------------|--------|
| Light List Tab | Real lights with editable properties | WORKS |
| Add/Delete Lights | Create and remove lights | WORKS |
| Focus Camera | Focus on selected light | WORKS |
| Visualization Tab | Top-down view of light positions | WORKS |
| Statistics Tab | Light count, intensity stats | WORKS |
| Shadow Maps Tab | Placeholder visualization | PLACEHOLDER |

---

## 9.6 SETTINGS VIEW (CONNECTED)

**Settings that apply in real-time to engine.**

| Feature | Description | Status |
|---------|-------------|--------|
| Grid Settings | Show/hide grid applies to engine | WORKS |
| Camera Move Speed | Applies to camera.moveSpeed | WORKS |
| Camera Rotate Speed | Stored but not applied (needs Camera update) | PARTIAL |
| Snap Settings | Stored locally | PARTIAL |
| Graphics Settings | Quality presets stored | UI ONLY |
| Save/Load Settings | JSON persistence | WORKS |

---

## 10. LAYOUT SYSTEM (FULLY WORKING)

| Feature | Description | Status |
|---------|-------------|--------|
| Lock Layout | Panels auto-arrange with window resize | WORKS |
| Unlock Layout | Panels can be freely moved | WORKS |
| Tile Layout | 6+ views open = grid arrangement | WORKS |
| Standard Layout | <6 views = editor arrangement | WORKS |
| Show All | Opens all views, tiles them | WORKS |
| Hide All | Closes all views | WORKS |
| Reset Layout | Returns to default layout | WORKS |
| Dynamic Resize | Panels resize with window | WORKS |
| Fullscreen Support | Layout adapts to fullscreen | WORKS |

---

## 11. VIEW MENU (FULLY WORKING)

| Feature | Description | Status |
|---------|-------------|--------|
| Core | Scene, Game, Hierarchy, Inspector | WORKS |
| Assets | Asset Browser | WORKS |
| Graphics | Material, Texture, UV Editor | WORKS |
| Debug | Console, Profiler, GPU Debug, Shader Debug, Lighting Debug | WORKS |
| Animation | Animation, Camera Sequencer | WORKS |
| System | Settings, Plugin Manager | WORKS |
| Lock/Unlock Layout | Toggle layout behavior | WORKS |
| Show/Hide All | Bulk view control | WORKS |
| Reset Layout | Restore defaults | WORKS |

---

## CRITICAL FIXES APPLIED

### Bug Fixes:
1. **defaultMeshes not initialized** - FIXED: Added initialization in `init_default_data()` for all 8 primitive types
2. **Light Selection Missing** - FIXED: Added `selectedLightIndex` to engine, updated HierarchyView and ObjectInspectorView
3. **View Mode Names Wrong** - FIXED: ConsoleView now shows correct mode names matching engine enum
4. **Selection Not Exclusive** - FIXED: Clicking primitive/light/node clears other selections
5. **Panels Overlapping** - FIXED: CalculateLayout uses default viewport size when viewport not ready

### View Connections Made:
6. **MaterialView Connected** - Now syncs with selected primitive and applies color changes
7. **GPUDebugView Connected** - Now shows real draw calls, triangles from primitives and GLTF meshes
8. **LightingDebugView Connected** - Now shows real lights from scenePointLights with editing
9. **SettingsView Connected** - Grid and camera speed settings now apply to engine in real-time

---

## Object Type Compatibility Matrix

| Feature | Static Primitives | GLTF/GLB Nodes | Point Lights |
|---------|-------------------|----------------|--------------|
| Selection | YES | YES | YES |
| Transform Edit | YES | **YES** | YES |
| Material Edit | YES | NO | N/A |
| Face Colors | YES | NO | N/A |
| Visibility | YES | NO | N/A |
| Focus Camera | YES | YES | YES |
| Duplicate | YES | NO | YES |
| Delete | YES | NO | YES |
| Color/Intensity | YES (mainColor) | NO | YES |
| Reset Transform | YES | YES | YES |
| Snap to Grid | YES | YES | NO |
| Uniform Scale | NO | YES | NO |

---

## File Paths

- **Assets Directory**: `assets/` (relative to working directory)
- **GLTF/GLB Files**: `assets/*.glb`, `assets/*.gltf`
- **Textures**: `assets/*.png`, `assets/*.jpg`, etc.
- **Subdirectories**: `assets/gltfscene/`, `assets/Shared/`

---

## Keyboard Shortcuts (Scene View)

| Key | Action |
|-----|--------|
| M | Toggle Magnet Snap |
| G | Toggle Grid |
| O | Toggle Outline |

---

## ENGINE SYSTEMS ADDED

### Animation System (VulkanEngine)
- `animationClips` - Vector of AnimationClipData with tracks and keyframes
- `skeletons` - Vector of SkeletonData with bone hierarchy
- `playAnimation()`, `stopAnimation()`, `updateAnimations()` methods
- Default demo animation (Walk) and skeleton (Humanoid) created on init

### Physics System (VulkanEngine)
- `physicsBodies` - Vector of PhysicsBodyData (position, velocity, mass, friction)
- `physicsConstraints` - Vector of PhysicsConstraintData
- `physicsSettings` - Gravity, timestep, max substeps, paused state
- `updatePhysics()`, `addPhysicsBody()`, `removePhysicsBody()` methods
- Simple ground collision simulation

### Plugin/Subsystem System (VulkanEngine)
- `subsystems` - Vector of SubsystemInfo (id, name, version, state, memory)
- `initSubsystems()` - Registers core subsystems on init
- Core subsystems: Vulkan Renderer, Scene Manager, PBR Materials, GLTF Loader, ImGui UI

### Shader System (VulkanEngine)
- `shaderPipelines` - Vector of ShaderPipelineInfo (name, paths, pipeline, layout)
- `shaderUniforms` - Vector of ShaderUniformInfo
- `registerShaderPipeline()`, `recompileShader()`, `recompileAllShaders()` methods
- Auto-registers mesh, primitive, shaded, wireframe, grid pipelines on init

## VIEWS NOW CONNECTED TO ENGINE

### TextureView
- Shows textures from `loadedScenes->images`
- Displays texture name, size, format, estimated memory
- Selectable texture list with scene hierarchy

### GameView
- Shows primitives from `static_shapes` as 2D shapes
- Shows lights from `scenePointLights` with glow effect
- Real-time FPS display from engine stats
- Play mode indicator

### ShaderDebugView
- Shows registered pipelines from `shaderPipelines`
- Recompile All button calls engine recompile methods
- Displays compile time, uniform count, texture count

### AnimationView
- Syncs with `animationClips` for timeline and tracks
- Syncs with `skeletons` for bone hierarchy display
- Play/Pause/Stop controls engine animation playback

### PhysicsDebugView
- Syncs with `physicsBodies` for collider visualization
- Creates colliders from `static_shapes` as static bodies
- Simulation controls affect engine physics settings
- Add Body button creates new physics body in engine

### PluginManagerView
- Shows `subsystems` as plugins with state and memory info
- Refresh button re-syncs with engine subsystems
- State colors match subsystem state

### UVView
- Shows UV data for selected primitive type
- Generates UV layouts for Cube, Sphere, other primitives
- Shows UV statistics with bounds calculation

### CameraSequencerView
- Playback controls engine camera position and rotation
- Interpolates between keyframes with smooth/bezier curves
- First keyframe syncs with current camera position
