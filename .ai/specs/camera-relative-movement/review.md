# Movement Review

## 1. Must fix

None found.

The implementation keeps horizontal movement, dodge displacement, jump gravity,
knockback displacement, and collision resolution inside `CharacterController::update`.
No hitbox dimensions, combo rules, or damage values were changed.

## 2. Recommended

- Consider moving the scripted movement scenario definitions into a small reusable
  test utility if more runtime movement modes are added.
- Consider exposing movement acceleration tuning through class data only if design
  later requires per-class acceleration.

## 3. Suggestions

- Add a future animation-specific assertion that records the selected locomotion
  state beside the measured post-solver speed.
- Add a future capture that renders the actual character while following one of the
  top-down traces for visual review of turn and run animation blending.

## Review conclusion

The code matches the movement plan and acceptance criteria. Runtime and CTest
evidence is recorded in `test-report.md`; no required follow-up blocks completion.
