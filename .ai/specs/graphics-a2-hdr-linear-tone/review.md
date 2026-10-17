# A2 Review

## Must Fix
- None found in the reviewed A2 diff.

## Should Fix
- Current glTF loader supports base-color textures and material emissive factors; it does not yet load emissive, normal, or ORM texture slots. Texture upload now supports an explicit linear-data mode for future normal/ORM maps.
- The 1080p timing scene is intentionally small and does not represent a full arena fight or establish performance on GTX 1060/RX 580.

## Suggestions
- Capture an in-game before/after screenshot with the HUD hidden or unchanged, then tune the sun and ambient balance in A3.
- Add representative emissive and ORM test assets when those slots are implemented in the asset pipeline.

## Review Result
- The renderer sends opaque scene, sky, particles, boss, and remote-player overlays through the shared scene target. ImGui draws after tone mapping to the default framebuffer.
- ACES output performs the only HDR-to-sRGB encoding. The legacy direct path encodes linear shader output once and bypasses the ACES pass.
- Timer queries are asynchronous; per-pass elapsed queries are not nested, and total-frame time comes from timestamp endpoints.
- The blue arena boundary line is an ImGui background polyline in `engine/src/debug_panel.cpp`; it is now off by default with an F3 toggle.

## HDR Framing Regression Review (2026-10-17)
- Must fix: none. The fullscreen triangle UV now spans the full HDR image; the capture regression validates the projected marker after tonemapping and would fail for the recorded pre-fix image.
- DPI handling: render target, viewport and projection aspect use `SDL_GetWindowSizeInPixels`; diagnostics compare them to point dimensions. Actual 150% Windows scaling could not be exercised because the host reported 1x even with `SDL_WINDOW_HIGH_PIXEL_DENSITY`.
- Pass state: scene, sky, particle/overlay, and tonemap explicitly set their drawable viewport and disable stale scissor state. Logged viewports agree with target dimensions.
- Scope: no camera/gameplay change and no camera checklist update. Keep the separate camera framing issue pending until its own evidence is reviewed.
