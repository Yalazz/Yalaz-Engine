# Yalaz Engine Docs

This folder contains editor/UI-focused documentation and engine system documentation.

## Documents

1. `docs/UI_PANELS_DETAILED.md`
Detailed description of each editor panel (purpose, data source, common actions, related files).
Also covers engine systems: save/load, animation, multi-format import pipeline.

2. `docs/UI_PANELS_QUICK_REFERENCE.md`
Fast lookup table for all 19 registered views.

## Key Source Files

Panel registration and names are defined in:

- `src/ui/views/Views.h`
- `src/ui/views/ViewManager.cpp`
- `src/ui/views/ViewManager.h`

Panel implementations are in:

- `src/ui/views/*View.cpp`
- `src/ui/views/*View.h`

Engine systems:

- `src/engine_state.cpp/h` -- Save/Load (JSON serialization of full scene state)
- `src/vk_engine.cpp/h` -- Core engine, rendering, animation, pipelines
- `src/vk_loader.cpp/h` -- Model loading (GLTF/GLB/FBX/DAE/OBJ)
- `src/renderer/` -- Post-processing, path tracer, environment maps
