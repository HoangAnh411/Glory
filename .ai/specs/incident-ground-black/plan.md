# Incident fix: black ground

## Root cause
The scene fragment shader multiplies material color by the vertex color (`vColor.rgb`). The ground VAO supplied only position, normal, and UV attributes, so the enabled shader input at location 5 received OpenGL's default generic value `(0,0,0,1)`. This multiplied every ground pixel to black.

## Change
Add an explicit white vertex color to each ground vertex and bind it at attribute location 5. Do not change ambient or direct-light intensity.

## Acceptance
- Ground shader receives white vertex color on the ground mesh.
- Existing F3 debug views remain selectable.
- Build, CTest, 1080p smoke, and capture pass; inspect the generated ground pixels.
