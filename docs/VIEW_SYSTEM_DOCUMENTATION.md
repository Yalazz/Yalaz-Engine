# Yalaz Engine - View System Documentation

## Overview

The Yalaz Engine View System is a professional, modular UI architecture inspired by industry-standard game engines like Unreal Engine, Unity, and Blender. It provides 18 customizable editor views organized into three phases, offering comprehensive tools for scene editing, asset management, debugging, and production workflows.

**Version:** 3.0.0
**Total Views:** 18
**Architecture:** Component-based EditorView system with ViewManager singleton

---

## Table of Contents

1. [Architecture Overview](#architecture-overview)
2. [Dynamic Features](#dynamic-features)
3. [View Categories](#view-categories)
4. [Phase 1: Core Editor Views](#phase-1-core-editor-views)
5. [Phase 2: Graphics Tools](#phase-2-graphics-tools)
6. [Phase 3: Advanced Debug & Production](#phase-3-advanced-debug--production)
7. [Workspace System](#workspace-system)
8. [How to Use](#how-to-use)
9. [File Locations](#file-locations)
10. [Extending the System](#extending-the-system)

---

## Architecture Overview

### Core Components

| Component | File Location | Description |
|-----------|---------------|-------------|
| `EditorView` | `src/ui/views/EditorView.h/cpp` | Base class for all views |
| `ViewManager` | `src/ui/views/ViewManager.h/cpp` | Singleton that manages all view instances |
| `Views.h` | `src/ui/views/Views.h` | Master header with registration functions |
| `EditorUI` | `src/ui/EditorUI.h/cpp` | Main editor controller with workspace system |

### EditorView Base Class

Every view inherits from `EditorView` and provides:

```cpp
class EditorView {
public:
    virtual void OnInit(VulkanEngine* engine);   // Called once on creation
    virtual void OnShutdown();                   // Called on destruction
    virtual void OnUpdate(float deltaTime);     // Called every frame
    virtual void OnRender();                    // Render ImGui content

    // View metadata
    std::string GetName();      // Display name
    std::string GetIcon();      // Icon character
    ViewCategory GetCategory(); // Category for menu organization
    ViewFlags GetFlags();       // Capabilities (docking, toolbar, etc.)
};
```

---

## Dynamic Features

All features in the view system are **dynamic** - they update in real-time and support live editing.

### Dynamic Scene Loading

**Asset Browser** supports:
- Click on `.gltf` or `.glb` files to load scenes dynamically
- Multiple scenes can be loaded simultaneously
- Each scene gets a unique name (auto-numbered if duplicates)
- Scenes panel shows all loaded scenes with Focus/Unload buttons
- Unload scenes without restarting the engine

```
Asset Browser Features:
├── Dynamic GLTF/GLB scene loading
├── Multiple simultaneous scenes
├── Loaded scenes management panel
├── Focus camera on scene
├── Unload individual scenes
└── Scene node count display
```

### Dynamic Texture Loading

**Material Editor** supports:
- Drag-and-drop textures from Asset Browser
- 5 PBR texture slots: Albedo, Normal, Metallic/Roughness, AO, Emission
- Click slot to remove assigned texture
- Visual feedback for loaded/empty slots
- All changes apply in real-time

```
Material Editor Texture Slots:
├── Albedo (Base Color)
├── Normal Map
├── Metallic/Roughness
├── Ambient Occlusion
└── Emission
```

### Dynamic Material Presets

**18 Material Presets** organized by category:
- **Metals:** Gold, Silver, Copper, Iron, Aluminum, Chrome
- **Plastics:** Red, Blue, Green, White, Black
- **Natural:** Wood, Stone, Marble, Skin
- **Special:** Glass, Rubber, Emissive

Click any preset to instantly apply PBR properties.

### Dynamic Inspector

**Object Inspector** features:
- Real-time transform editing (Position, Rotation, Scale)
- Dynamic face color editing per primitive type
- 18 light presets that apply instantly
- Statistics that update as objects change

---

## View Categories

### Quick Reference

| Phase | Views | Count |
|-------|-------|-------|
| Phase 1 - Core | Scene, Game, Hierarchy, Inspector, Asset Browser, Console | 6 |
| Phase 2 - Graphics | Profiler, Material Editor, Texture Inspector, UV Editor, Shader Debug | 5 |
| Phase 3 - Advanced | GPU Debug, Lighting Debug, Animation, Settings, Plugin Manager, Physics Debug, Camera Sequencer | 7 |
| **Total** | | **18** |

---

## Phase 1: Core Editor Views

### 1. Scene View (`SceneView`)
**File:** `src/ui/views/SceneView.h/cpp`
**Category:** Core | **Shortcut:** S

The main 3D viewport for scene editing and navigation.

**Dynamic Features:**
- **View Modes:** Lit, Unlit, Wireframe, Normals, UVs, Depth, Overdraw (change instantly)
- **Display Toggles:** Grid, Outlines, Gizmos (real-time toggle)
- **Background Effects:** Live color editing with 6 presets (Dark, Light, Blue, Sunset, Green, Purple)
- **Grid Settings:** 6 dynamic presets + full customization
  - Default, Blender, Unity, Unreal, Fine, Coarse
  - Live editing: size, multiplier, colors, opacity, fade distance
- **Camera:** Real-time position/rotation editing, quick view buttons
- **Snap Settings:** Dynamic position, rotation, scale snap values

**Grid Presets:**

| Preset | Base Size | Multiplier | Style |
|--------|-----------|------------|-------|
| Default | 1.0 | 10 | Standard |
| Blender | 1.0 | 10 | Dark lines |
| Unity | 1.0 | 10 | Medium contrast |
| Unreal | 10.0 | 10 | Large scale |
| Fine | 0.1 | 10 | Detailed |
| Coarse | 5.0 | 4 | Large grid |

---

### 2. Game View (`GameView`)
**File:** `src/ui/views/GameView.h/cpp`
**Category:** Core | **Shortcut:** G

Preview the game as players will see it.

**Dynamic Features:**
- Play/Pause/Stop with instant state changes
- Resolution presets change viewport dynamically
- Frame stepping for debugging

---

### 3. Hierarchy View (`HierarchyView`)
**File:** `src/ui/views/HierarchyView.h/cpp`
**Category:** Core | **Shortcut:** H

**Dynamic Features:**
- Real-time object list updates as scenes load/unload
- Instant search filtering
- Dynamic visibility toggles per object
- Context menu with immediate actions

---

### 4. Object Inspector View (`ObjectInspectorView`)
**File:** `src/ui/views/ObjectInspectorView.h/cpp`
**Category:** Core | **Shortcut:** I

**Dynamic Features:**
- **Transform Section:** Live editing with colored axis indicators (X=Red, Y=Green, Z=Blue)
- **Dynamic Face Colors:** Per-face color editing based on primitive type:
  - Cube: 6 faces (Front, Back, Top, Bottom, Left, Right)
  - Sphere: 4 parts (Top, Upper, Lower, Bottom)
  - Cylinder: 3 parts (Top, Side, Bottom)
  - Cone: 2 parts (Base, Side)
  - Torus: 2 parts (Outer, Inner)
  - Plane: 1 face
- **18 Light Presets:** Click to apply instantly
  - Basic: Neutral White, Warm White, Cool White
  - Natural: Daylight, Sunset, Golden Hour, Blue Hour, Moonlight, Overcast
  - Artificial: Tungsten, Fluorescent, LED, Halogen, Candlelight, Neon, Sodium Vapor
  - Creative: Dramatic Warm, Dramatic Cool
- **Statistics:** Live vertex/face/triangle counts

---

### 5. Asset Browser View (`AssetBrowserView`)
**File:** `src/ui/views/AssetBrowserView.h/cpp`
**Category:** Assets | **Shortcut:** A

**Dynamic Features:**
- **Multiple Scene Loading:**
  - Click `.gltf`/`.glb` to load dynamically
  - Auto-naming for duplicate scenes (model, model_1, model_2...)
  - Loaded Scenes panel with management
- **Texture Drag-Drop:**
  - Drag textures to Material Editor slots
  - Visual drag feedback
- **Asset Type Color Coding:**
  - Folders: Blue
  - Models: Green
  - Textures: Orange
  - Shaders: Purple
  - Scenes: Blue-gray
- **Search:** Real-time filtering as you type
- **Grid/List:** Toggle view modes instantly
- **Thumbnail Size:** Dynamic slider adjustment

**Context Menu Actions:**
- Load Scene / Load as New Instance
- Set as Albedo / Normal (for textures)
- Show in Explorer
- Copy Path

---

### 6. Console View (`ConsoleView`)
**File:** `src/ui/views/ConsoleView.h/cpp`
**Category:** Debug | **Shortcut:** C

**Dynamic Features:**
- Real-time log updates
- Dynamic filtering by level (Info, Warning, Error, Verbose)
- Live search filtering
- Auto-scroll option

---

## Phase 2: Graphics Tools

### 7. Profiler View (`ProfilerView`)
**File:** `src/ui/views/ProfilerView.h/cpp`
**Category:** Debug | **Shortcut:** P

**Dynamic Features:**
- Real-time FPS graph with 60-frame history
- Live CPU/GPU timing
- Dynamic memory usage tracking

---

### 8. Material View (`MaterialView`)
**File:** `src/ui/views/MaterialView.h/cpp`
**Category:** Graphics | **Shortcut:** M

**Dynamic Features:**
- **PBR Properties:** All sliders update preview in real-time
  - Base Color with alpha
  - Metallic (0-100%)
  - Roughness (0-100%)
  - Ambient Occlusion
  - Normal Strength
  - Emission with strength
- **Texture Slots (Drag-Drop):**
  - Albedo (Base Color)
  - Normal Map
  - Metallic/Roughness
  - Ambient Occlusion
  - Emission
- **18 Material Presets:** Click to apply instantly
  - Metals: Gold, Silver, Copper, Iron, Aluminum, Chrome
  - Plastics: Red, Blue, Green, White, Black
  - Natural: Wood, Stone, Marble, Skin
  - Special: Glass, Rubber, Emissive
- **Quick Buttons:** Shiny, Matte, Metal, Plastic
- **Emission Presets:** Fire, Neon, Glow
- **Preview:** Live preview with Sphere/Cube/Plane/Cylinder/Torus options

---

### 9. Texture View (`TextureView`)
**File:** `src/ui/views/TextureView.h/cpp`
**Category:** Graphics | **Shortcut:** T

**Dynamic Features:**
- Real-time channel visualization (R, G, B, A, All)
- Dynamic zoom/pan
- Mip level selection
- Live histogram updates

---

### 10. UV View (`UVView`)
**File:** `src/ui/views/UVView.h/cpp`
**Category:** Graphics | **Shortcut:** U

**Dynamic Features:**
- Real-time UV visualization
- Dynamic texture overlay

---

### 11. Shader Debug View (`ShaderDebugView`)
**File:** `src/ui/views/ShaderDebugView.h/cpp`
**Category:** Debug | **Shortcut:** D

**Dynamic Features:**
- Live shader source viewing
- Real-time compile status
- Dynamic uniform inspection

---

## Phase 3: Advanced Debug & Production

### 12. GPU Debug View (`GPUDebugView`)
**File:** `src/ui/views/GPUDebugView.h/cpp`
**Category:** Debug | **Shortcut:** G

**Dynamic Features:**
- Real-time Vulkan resource tracking
- Live memory allocation view
- Dynamic validation messages

---

### 13. Lighting Debug View (`LightingDebugView`)
**File:** `src/ui/views/LightingDebugView.h/cpp`
**Category:** Debug | **Shortcut:** L

**Dynamic Features:**
- Real-time light list updates
- Live shadow map preview
- Dynamic light bounds visualization

---

### 14. Animation View (`AnimationView`)
**File:** `src/ui/views/AnimationView.h/cpp`
**Category:** Animation | **Shortcut:** N

**Dynamic Features:**
- Real-time timeline scrubbing
- Live keyframe editing
- Dynamic curve preview

---

### 15. Settings View (`SettingsView`)
**File:** `src/ui/views/SettingsView.h/cpp`
**Category:** System | **Shortcut:** O

**Dynamic Features:**
- All settings apply in real-time
- Live theme switching

---

### 16. Plugin Manager View (`PluginManagerView`)
**File:** `src/ui/views/PluginManagerView.h/cpp`
**Category:** System | **Shortcut:** K

**Dynamic Features:**
- Real-time plugin enable/disable
- Live configuration updates

---

### 17. Physics Debug View (`PhysicsDebugView`)
**File:** `src/ui/views/PhysicsDebugView.h/cpp`
**Category:** Debug | **Shortcut:** P

**Dynamic Features:**
- Real-time collider visualization
- Live simulation controls (play/pause/step)
- Dynamic time scale (0.1x to 2.0x)
- Real-time contact point display
- Live raycast visualization

**Collider Color Coding:**
| Type | Color |
|------|-------|
| Static | Green |
| Dynamic | Blue |
| Kinematic | Yellow |
| Trigger | Magenta |

---

### 18. Camera Sequencer View (`CameraSequencerView`)
**File:** `src/ui/views/CameraSequencerView.h/cpp`
**Category:** Animation | **Shortcut:** Q

**Dynamic Features:**
- Real-time timeline playback
- Live keyframe editing
- Dynamic shake effect preview

**Shake Presets:**
| Preset | Intensity | Frequency | Duration |
|--------|-----------|-----------|----------|
| None | 0 | 0 | 0 |
| Subtle | 0.1 | 15 Hz | Continuous |
| Handheld | 0.3 | 8 Hz | Continuous |
| Impact | 0.8 | 20 Hz | 0.3s |
| Explosion | 1.5 | 25 Hz | 1.0s |
| Earthquake | 2.0 | 5 Hz | 3.0s |

---

## Workspace System

The editor includes a powerful workspace system for saving and loading UI configurations.

### Built-in Workspaces

| Workspace | Icon | Description | Main Views |
|-----------|------|-------------|------------|
| Default | \|\|\| | Standard layout | Scene, Hierarchy, Inspector, Console, Asset Browser |
| Modeling | [M] | Large viewport | Scene, Hierarchy, Inspector, UV Editor |
| Animation | [A] | Timeline focus | Scene, Animation, Camera Sequencer, Hierarchy |
| Debug | [D] | Profiling tools | Scene, Console, Profiler, GPU Debug |
| Lighting | [L] | Light setup | Scene, Lighting Debug, Inspector, Hierarchy |
| Material | [T] | Texture editing | Scene, Material Editor, Texture Inspector, Asset Browser |
| Fullscreen | [F] | Minimal UI | Scene only |

### Saving Custom Workspaces

1. Arrange views as desired
2. Go to **Workspace > Save Current Workspace...**
3. Enter a name and description
4. Click Save

Custom workspaces are saved to `workspaces.json` and persist between sessions.

### Managing Workspaces

- **Workspace > Manage Workspaces...** opens the workspace manager
- Double-click a workspace to apply it
- Custom workspaces can be deleted (built-in cannot)
- Current workspace shown in menu bar

---

## How to Use

### Dynamic Scene Loading

1. Open **Asset Browser** (View > Assets > Asset Browser)
2. Navigate to folder with `.gltf` or `.glb` files
3. Click on a model file to load it
4. Scene appears in **Loaded Scenes** panel (toggle with "Scenes" checkbox)
5. Click **Focus** to center camera on scene
6. Click **Unload** to remove scene from memory
7. Load multiple scenes by clicking on different files

### Dynamic Texture Assignment

1. Open **Material Editor** (View > Graphics > Material Editor)
2. Open **Asset Browser** side by side
3. Navigate to textures folder in Asset Browser
4. Drag a texture (`.png`, `.jpg`, etc.) to a texture slot in Material Editor
5. Slot turns green when texture is loaded
6. Click the slot to remove the texture
7. All 5 PBR slots support drag-drop

### Dynamic Material Editing

1. Open **Material Editor**
2. Adjust PBR properties with sliders (changes apply instantly)
3. Use Quick buttons (Shiny, Matte, Metal, Plastic)
4. Click material presets to apply full material settings
5. Preview updates in real-time with selected mesh type

### Opening Views

**Method 1: Menu Bar**
1. Click "View" in the main menu bar
2. Navigate to category (Core, Assets, Graphics, Debug, etc.)
3. Click on the view name to toggle visibility

**Method 2: Workspace Presets**
1. Click "Workspace" in the menu bar
2. Select a preset to instantly configure all views

**Method 3: Quick Actions**
- Use "View > Show All" to open all views
- Use "View > Hide All" to close all views
- Use "View > Reset Layout" to restore default

---

## File Locations

### Core Files

```
src/ui/
├── EditorUI.h/cpp           # Main editor controller + workspace system
├── EditorTheme.h            # Theme configuration
├── EditorSelection.h        # Selection state
│
└── views/
    ├── EditorView.h/cpp     # Base class
    ├── ViewManager.h/cpp    # View management singleton
    ├── Views.h              # Master header
    ├── ViewSystemIntegration.h
    │
    ├── # Phase 1 - Core Editor
    ├── SceneView.h/cpp
    ├── GameView.h/cpp
    ├── HierarchyView.h/cpp
    ├── ObjectInspectorView.h/cpp
    ├── AssetBrowserView.h/cpp
    ├── ConsoleView.h/cpp
    │
    ├── # Phase 2 - Graphics Tools
    ├── ProfilerView.h/cpp
    ├── MaterialView.h/cpp
    ├── TextureView.h/cpp
    ├── UVView.h/cpp
    ├── ShaderDebugView.h/cpp
    │
    └── # Phase 3 - Advanced Debug & Production
        ├── GPUDebugView.h/cpp
        ├── LightingDebugView.h/cpp
        ├── AnimationView.h/cpp
        ├── SettingsView.h/cpp
        ├── PluginManagerView.h/cpp
        ├── PhysicsDebugView.h/cpp
        └── CameraSequencerView.h/cpp
```

### Documentation

```
docs/
├── VIEW_SYSTEM_DOCUMENTATION.md  # This file (full documentation)
└── VIEW_QUICK_REFERENCE.md       # Quick reference card
```

### Configuration Files

```
workspaces.json  # Custom workspace configurations (auto-generated)
scene.json       # Scene save/load file
```

---

## Extending the System

### Creating a New View

1. **Create header file** (`MyView.h`):

```cpp
#pragma once
#include "EditorView.h"

namespace Yalaz::UI {

class MyView : public EditorView {
public:
    MyView() : EditorView("My View", "[M]", ViewCategory::Core) {}

    void OnInit(VulkanEngine* engine) override;
    void OnRender() override;

private:
    // Your member variables
};

} // namespace Yalaz::UI
```

2. **Create implementation file** (`MyView.cpp`)

3. **Register in `Views.h`:**
```cpp
vm.RegisterViewType<MyView>("MyView", "My View", "M", true);
```

4. **Add to `ViewManager.cpp`:**
```cpp
m_Views.push_back(std::make_unique<MyView>());
```

5. **Add to `CMakeLists.txt`**

---

## Version History

| Version | Changes |
|---------|---------|
| 3.0.0 | Removed old panel system, added workspace save/load, dynamic scene loading, dynamic texture slots, material presets |
| 2.1.0 | Added PhysicsDebugView and CameraSequencerView |
| 2.0.0 | Complete view system with 16 views |
| 1.0.0 | Initial view system architecture |

---

## Credits

Yalaz Engine View System v3.0.0
Professional game development UI architecture
All features are dynamic and update in real-time

---

*Last Updated: January 2026*
