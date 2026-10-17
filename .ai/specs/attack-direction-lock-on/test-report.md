# Bao cao TESTER

## Lenh da chay

- Baseline truoc khi sua camera lock: `build/glory_client.exe --combat-test` -> FAIL.
- Build: `cmake --build build` -> PASS.
- Unit/integration: `ctest --test-dir build --output-on-failure` -> 38/38 PASS.
- Smoke: `build/glory_client.exe --smoke-test` -> PASS, OpenGL 4.5, 120 fixed ticks, HDR framebuffer complete.
- Runtime combat: `build/glory_client.exe --combat-test` -> PASS.

## Baseline FAIL

Log baseline duoc luu trong `combat-test-baseline.log`. Lock phase khi do co `targetLocked=1` nhung boss ra ngoai khung, voi NDC x toi `7.24` va y toi `8.00`; command ket thuc `--combat-test FAIL`.

## Runtime PASS

- `manual_attack`: PASS, facing `90` do khi camera yaw la `90`.
- `assist_attack`: PASS, facing `0` khi boss nam trong cone.
- `auto_attack`: PASS, facing `0` khi camera quay nguoc boss.
- `lock_frame`: PASS, player va boss co marker magenta doc lai duoc, boss bounds trong NDC, camera khong nam trong collider.
- `lock_cancel`: PASS, lock huy khi boss vuot qua range.
- `lock_hold`: PASS, lock tiep tuc giu va camera frame on dinh.
- Ba phase attack cung so sanh damage, cooldown va hitbox radius runtime; khong co sai lech.

## Anh da xem

Sau anh da duoc tao va xem lai:

- `captures/combat-test/manual_attack_combat-test.png`
- `captures/combat-test/assist_attack_combat-test.png`
- `captures/combat-test/auto_attack_combat-test.png`
- `captures/combat-test/lock_frame_combat-test.png`
- `captures/combat-test/lock_cancel_combat-test.png`
- `captures/combat-test/lock_hold_combat-test.png`

JSON telemetry tuong ung nam cung thu muc, gom NDC, arm distance, collider blocker va ket qua readback cua ca hai marker.
