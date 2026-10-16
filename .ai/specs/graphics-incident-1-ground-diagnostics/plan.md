# Graphics incident 1: ground diagnostics

## Objective
Add renderer-side diagnostic views for ground lighting and guard shader inputs that can produce undefined or non-finite values. Preserve the normal image unless a debug view is selected.

## Tasks
1. Add F3-selectable albedo, world-normal, ambient, direct-light, shadow-factor, fog-factor, depth, and invalid-value views.
2. Guard normal normalization, light-space projection, and zero-length fog direction; keep lighting/fog outputs finite and nonnegative.
3. Build, run targeted and full CTest, and smoke at 1080p. Record what code inspection and test establish about the black ground.

## Files
- `engine/include/glory/engine/model_renderer.hpp`
- `engine/src/model_renderer.cpp`
- `engine/src/debug_panel.cpp`
- `tests/shadow_tests.cpp`
- `PLAN.md`
- `docs/perf.md`
- `.ai/specs/graphics-incident-1-ground-diagnostics/`

## Risks
- GLSL 4.5 debug classification must compile on the project's OpenGL target.
- CPU tests cannot prove rendered pixel colors; report visual checks as unavailable unless an actual screenshot is captured.

## Acceptance
- All requested views are selectable in F3 and the invalid view marks non-finite/negative lighting values magenta.
- Invalid shadow projection and zero-length fog vectors do not propagate invalid values.
- Build, CTest, and 1080p smoke pass; no gameplay modules change.
