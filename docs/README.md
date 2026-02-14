# Yalaz Engine Docs

This folder contains editor/UI-focused documentation for the current view system.

## Documents

1. `docs/UI_PANELS_DETAILED.md`
Detailed description of each editor panel (purpose, data source, common actions, related files).

2. `docs/UI_PANELS_QUICK_REFERENCE.md`
Fast lookup table for all registered views.

3. `docs/UI_VIEW_ARCHITECTURE.md`
How panels are registered, instantiated, opened/closed, updated, and rendered.

## Source Of Truth

Panel registration and names are defined in:

- `src/ui/views/Views.h`
- `src/ui/views/ViewManager.cpp`
- `src/ui/views/ViewManager.h`

Panel implementations are in:

- `src/ui/views/*View.cpp`
- `src/ui/views/*View.h`

