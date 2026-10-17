# Review: incident capture and camera telemetry

## Must fix
- None for the instrumentation slice. Capture is opt-in and does not advance the game simulation.

## Recommended
- The captured actor anchor NDC disagrees with the visible model placement in the PNGs. Step 2 must measure the projected visible model bounds (or equivalent pixels) before using NDC to tune the camera.
- Captures show a black ground in all six scenes; screenshots establish the symptom but do not identify the lighting pass responsible.

## Suggestions
- Add a deterministic pixel classification/readback for the actor mask and ground regions for future regression coverage.

## Result
- Build, CTest, smoke, and capture all pass. Capture PNGs were visually reviewed.
