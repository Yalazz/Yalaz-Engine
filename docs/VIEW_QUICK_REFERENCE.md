# Yalaz Engine - View System Quick Reference Card

## View Shortcuts

| View | Category | Shortcut | File |
|------|----------|----------|------|
| Scene | Core | S | SceneView.cpp |
| Game | Core | G | GameView.cpp |
| Hierarchy | Core | H | HierarchyView.cpp |
| Inspector | Core | I | ObjectInspectorView.cpp |
| Asset Browser | Assets | A | AssetBrowserView.cpp |
| Console | Debug | C | ConsoleView.cpp |
| Profiler | Debug | P | ProfilerView.cpp |
| Material Editor | Graphics | M | MaterialView.cpp |
| Texture Inspector | Graphics | T | TextureView.cpp |
| UV Editor | Graphics | U | UVView.cpp |
| Shader Debug | Debug | D | ShaderDebugView.cpp |
| GPU Debug | Debug | G | GPUDebugView.cpp |
| Lighting Debug | Debug | L | LightingDebugView.cpp |
| Animation | Animation | N | AnimationView.cpp |
| Settings | System | O | SettingsView.cpp |
| Plugin Manager | System | K | PluginManagerView.cpp |
| Physics Debug | Debug | P | PhysicsDebugView.cpp |
| Camera Sequencer | Animation | Q | CameraSequencerView.cpp |

## Scene View Controls

| Action | Control |
|--------|---------|
| Look Around | Right Mouse + Drag |
| Move | WASD keys |
| Move Up/Down | Q/E keys |
| Sprint | Shift |
| Focus Object | F key |
| Pan | Middle Mouse + Drag |
| Zoom | Scroll Wheel |

## Scene View - View Modes

- **Lit** - Full lighting and materials
- **Unlit** - No lighting, base colors only
- **Wireframe** - Mesh wireframe display
- **Normals** - Surface normal visualization
- **UVs** - UV coordinate visualization
- **Depth** - Depth buffer visualization
- **Overdraw** - Pixel overdraw heatmap

## Scene View - Grid Presets

| Preset | Size | Subdivisions | Fade |
|--------|------|--------------|------|
| Default | 20 | 4 | 20 |
| Blender | 10 | 10 | 50 |
| Unity | 15 | 5 | 30 |
| Unreal | 100 | 10 | 100 |
| Fine | 5 | 10 | 10 |
| Coarse | 50 | 2 | 50 |

## Inspector - Light Presets

### Basic
- Neutral White (1.0, 1.0, 1.0)
- Warm White (1.0, 0.95, 0.85)
- Cool White (0.9, 0.95, 1.0)

### Natural
- Daylight (1.0, 0.98, 0.92)
- Sunset (1.0, 0.5, 0.2)
- Golden Hour (1.0, 0.85, 0.6)
- Blue Hour (0.4, 0.5, 0.8)
- Moonlight (0.7, 0.8, 1.0)
- Overcast (0.85, 0.87, 0.9)

### Artificial
- Tungsten (1.0, 0.8, 0.5)
- Fluorescent (0.95, 1.0, 0.95)
- LED White (1.0, 0.98, 0.96)
- Halogen (1.0, 0.9, 0.7)
- Candlelight (1.0, 0.6, 0.2)
- Neon (1.0, 0.2, 0.8)
- Sodium Vapor (1.0, 0.7, 0.2)

### Creative
- Dramatic Warm (1.0, 0.4, 0.1)
- Dramatic Cool (0.2, 0.4, 1.0)

## Inspector - Primitive Face Colors

| Primitive | Faces |
|-----------|-------|
| Cube | Front, Back, Top, Bottom, Left, Right (6) |
| Sphere | Top, Upper, Lower, Bottom (4) |
| Cylinder | Top, Side, Bottom (3) |
| Cone | Base, Side (2) |
| Torus | Outer, Inner (2) |
| Plane | Single face (1) |

## Camera Sequencer - Shake Presets

| Preset | Intensity | Frequency | Duration | Decay |
|--------|-----------|-----------|----------|-------|
| None | 0 | 0 | 0 | No |
| Subtle | 0.1 | 15 Hz | Continuous | No |
| Handheld | 0.3 | 8 Hz | Continuous | No |
| Impact | 0.8 | 20 Hz | 0.3s | Yes |
| Explosion | 1.5 | 25 Hz | 1.0s | Yes |
| Earthquake | 2.0 | 5 Hz | 3.0s | No |

## Physics Debug - Collider Colors

| Type | Color |
|------|-------|
| Static | Green |
| Dynamic | Blue |
| Kinematic | Yellow |
| Trigger | Magenta |

## Console - Log Levels

| Level | Color | Icon |
|-------|-------|------|
| Info | White | [i] |
| Warning | Yellow | [!] |
| Error | Red | [X] |
| Verbose | Gray | [.] |

## Menu Navigation

```
View Menu
├── Core
│   ├── Scene View
│   ├── Game View
│   ├── Hierarchy
│   └── Inspector
├── Assets
│   └── Asset Browser
├── Graphics
│   ├── Material Editor
│   ├── Texture Inspector
│   └── UV Editor
├── Debug
│   ├── Console
│   ├── Profiler
│   ├── Shader Debug
│   ├── GPU Debug
│   ├── Lighting Debug
│   └── Physics Debug
├── Animation
│   ├── Animation
│   └── Camera Sequencer
├── System
│   ├── Settings
│   └── Plugin Manager
├── ─────────────
├── Show All
├── Hide All
└── Reset Layout
```

## File Locations

All view files are located in:
```
src/ui/views/
```

Documentation:
```
docs/VIEW_SYSTEM_DOCUMENTATION.md  # Full documentation
docs/VIEW_QUICK_REFERENCE.md       # This file
```

---
*Yalaz Engine View System v2.1.0*
