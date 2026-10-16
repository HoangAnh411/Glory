# A3c Review

### 1. Phải sửa
- None found in the reviewed implementation; runtime cascades, shared shadow atlas, all tests, smoke, and the measured smoke GPU budget pass.

### 2. Nên sửa
- Visual confirmation of shadow acne, peter-panning and cascade seams needs fixed-camera captures/RenderDoc inspection; CPU tests cannot verify these image-space artifacts.
- Actor shadow timing aggregates all actor draws; it does not report boss and each remote as separate values.

### 3. Gợi ý
- Measure on a representative combat scene before treating the smoke-scene timings as a gameplay performance guarantee.
