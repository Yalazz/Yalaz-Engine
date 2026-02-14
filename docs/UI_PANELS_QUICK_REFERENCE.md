# UI Panels Quick Reference

Registered in `src/ui/views/Views.h`.

| Type Id | Display Name | Category | Default Open | Main Files |
|---|---|---|---|---|
| `SceneView` | Scene | Core | Yes | `src/ui/views/SceneView.cpp` |
| `GameView` | Game | Core | Yes | `src/ui/views/GameView.cpp` |
| `HierarchyView` | Hierarchy | Core | Yes | `src/ui/views/HierarchyView.cpp` |
| `InspectorView` | Inspector | Core | Yes | `src/ui/views/ObjectInspectorView.cpp` |
| `AssetBrowserView` | Asset Browser | Assets | Yes | `src/ui/views/AssetBrowserView.cpp` |
| `ConsoleView` | Console | Debug | Yes | `src/ui/views/ConsoleView.cpp` |
| `ProfilerView` | Profiler | Debug | Yes | `src/ui/views/ProfilerView.cpp` |
| `MaterialView` | Material Editor | Graphics | No | `src/ui/views/MaterialView.cpp` |
| `TextureView` | Texture Inspector | Graphics | No | `src/ui/views/TextureView.cpp` |
| `UVView` | UV Editor | Graphics | No | `src/ui/views/UVView.cpp` |
| `ShaderDebugView` | Shader Debug | Debug | Yes | `src/ui/views/ShaderDebugView.cpp` |
| `GPUDebugView` | GPU Debug | Debug | Yes | `src/ui/views/GPUDebugView.cpp` |
| `LightingDebugView` | Lighting Debug | Graphics | Yes | `src/ui/views/LightingDebugView.cpp` |
| `AnimationView` | Animation | Animation | Yes | `src/ui/views/AnimationView.cpp` |
| `SettingsView` | Settings | System | Yes | `src/ui/views/SettingsView.cpp` |
| `PluginManagerView` | Plugins | System | Yes | `src/ui/views/PluginManagerView.cpp` |
| `PhysicsDebugView` | Physics Debug | Debug | Yes | `src/ui/views/PhysicsDebugView.cpp` |
| `CameraSequencerView` | Camera Sequencer | Animation | Yes | `src/ui/views/CameraSequencerView.cpp` |
| `RenderSettingsView` | Render Settings | Rendering | Yes | `src/ui/views/RenderSettingsView.cpp` |

## Notes

- `Views.h` includes a legacy `ViewSystemInfo` count that does not include `RenderSettingsView`.
- Effective active view set is based on `RegisterAllViewTypes()`.

