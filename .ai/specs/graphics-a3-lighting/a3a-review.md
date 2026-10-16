# A3a Review

### 1. Phải sửa
- Cascaded maps, stable per-cascade projection, PCF quality levels, cascade transitions/debug, and shared actor/world shadowing remain unimplemented. Do not mark A3 complete until these are connected to runtime.

### 2. Nên sửa
- Move actor shadow rendering into a primary-owned scene pass and time shadow work independently; current actor depth draws are accounted inside the particle/overlay timer.

### 3. Gợi ý
- Add visual regression captures for player, boss, and remote actors against a caster on the ground.
