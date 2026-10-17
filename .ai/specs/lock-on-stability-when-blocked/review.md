# Review

## 1. Must Fix

None.

## 2. Should Fix

None for this focused fix.

## 3. Suggestions

- A future combat runtime scenario could deliberately place the boss against a blocking AABB and verify lock persistence through the full client callback, in addition to the focused targeting regression.

## Verification

- `chooseLockTarget` still performs the initial line-of-sight check.
- `shouldMaintainLock` is independent from line of sight and enforces only alive/range validity.
- The fixed update now uses the maintenance predicate, so a temporary collider obstruction cannot clear an existing lock.
- The existing combat camera and collision paths remain unchanged.
