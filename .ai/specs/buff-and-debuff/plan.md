# Kế hoạch hệ thống Buff và Debuff

## 1. Mục tiêu
Thêm bộ quản lý hiệu ứng trạng thái chạy độc lập trên CPU trong `glory_game`; tích hợp chỉ số và hồi/trừ máu định kỳ cho người chơi và hình nộm; cho phép skill áp dụng hiệu ứng từ JSON; hiển thị trạng thái và nút thử nghiệm trong DebugPanel.

## 2. Công việc
1. Implement validated status effect definitions, refresh/stack behavior, fixed-delta ticking, expiration, and aggregate movement/damage/stun modifiers.
2. Parse optional status effects in skill JSON and apply them on confirmed hits; scale outgoing and incoming damage and tick the dummy's effects.
3. Apply player speed and stun modifiers to CharacterController input and expose player/dummy effect state through frame telemetry.
4. Add DebugPanel controls for speed/attack buffs, dummy burn/stun and clearing effects; list active effects with remaining time and stacks.
5. Add independent CTest coverage for periodic health, modifiers, stun, expiration/stacking; build, run CTest and smoke test; record results and review.

## 3. File cần sửa hoặc tạo
- `game/include/glory/game/status_effect.hpp`, `game/src/status_effect.cpp`
- `game/include/glory/game/combat.hpp`, `game/src/combat.cpp`
- `game/include/glory/game/character_controller.hpp`, `game/src/character_controller.cpp`
- `game/src/main.cpp`, `engine/include/glory/engine/application.hpp`, `engine/src/application.cpp`, `engine/include/glory/engine/debug_panel.hpp`, `engine/src/debug_panel.cpp`
- `assets/data/skills.json`, `tests/status_effect_tests.cpp`, `CMakeLists.txt`
- `.ai/specs/buff-and-debuff/plan.md`, `test-report.md`, `review.md`, `PLAN.md`

## 4. Rủi ro và cách giảm thiểu
- Fixed-step boundary drift can cause missed or duplicate periodic ticks; consume tick timers in a loop while time remains and cap periodic events to effect duration.
- Stacking can inflate modifiers unexpectedly; use explicit per-effect stack limits and multiply additive percentage contributions per stack, refreshing duration on reapplication.
- JSON data may be malformed; validate effect names, magnitudes, durations and tick intervals at load time with contextual errors.
- Rendering/UI code must not enter game logic; telemetry is copied as plain strings/numbers and requests are sent back as simple commands.

## 5. Tiêu chí nghiệm thu
- Status effects have stable IDs/types, duration, remaining time, tick interval/timer, magnitude and stack count.
- Regen/DoT applies exact periodic HP deltas, effects expire, and reapplication refreshes duration and increments bounded stacks.
- Speed, outgoing/incoming damage multipliers and stun state are queryable and integrated with player movement and combat.
- Skill JSON can attach effects and confirmed hits apply them to the dummy.
- DebugPanel lists player/dummy effects and exposes buff, burn, stun and clear controls.
- New CPU tests pass; existing CTest suite remains green; 120-tick client smoke test exits successfully.
