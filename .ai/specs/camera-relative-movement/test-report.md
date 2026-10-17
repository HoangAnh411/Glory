# Movement Test Report

## Commands executed

- Baseline harness before controller changes: `build/glory_client.exe --move-test`
- Build: `cmake --build build`
- Unit and integration tests: `ctest --test-dir build --output-on-failure`
- Runtime movement validation: `build/glory_client.exe --move-test`
- Rendering smoke test: `build/glory_client.exe --smoke-test`
- Image inspection: `captures/move-test/move-test-30.png`, `move-test-60.png`, and `move-test-144.png` were opened and reviewed.

## Baseline evidence

The newly added harness failed before the controller fix:

```text
--move-test 30 FPS: FAIL ... scenario=W firstSpeed=4.500000 releaseSpeed=4.499989 ...
```

This recorded the old immediate acceleration and immediate stop behavior.

## Final results

- `cmake --build build`: PASS.
- `ctest --test-dir build --output-on-failure`: PASS, 37/37 tests.
- `--move-test`: PASS at 30, 60, and 144 FPS.
- `--smoke-test`: PASS; OpenGL 4.5 context, HDR framebuffer resize checks, and 120 fixed ticks completed.
- Runtime cases cover camera yaw 0, 90, 180, 270, and 37.5 degrees; W/A/S/D and W+A; measured position velocity; S facing; 720 degrees/second turn cap; diagonal wall sliding; and equal fixed-step distance across render rates.

## Generated traces

- `captures/move-test/move-test-30.json`
- `captures/move-test/move-test-30.png`
- `captures/move-test/move-test-60.json`
- `captures/move-test/move-test-60.png`
- `captures/move-test/move-test-144.json`
- `captures/move-test/move-test-144.png`

## Notes

- The harness uses the production `fixedUpdate` callback and the same `ActionMap` input conversion, with a simulated render accumulator around the 1/60 fixed timestep.
- The PNG traces are CPU generated top-down diagnostic images, so they do not replace an in-game visual QA pass with the final art and animation assets.
- `PLAN.md` was not ticked because this movement refinement is validated as a follow-up to the already completed character controller milestone.
