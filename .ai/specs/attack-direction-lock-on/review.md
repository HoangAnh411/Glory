# Bao cao REVIEWER

## 1. Phai sua

- Khong co. Code da build duoc, unit suite 38/38 PASS, smoke PASS va combat-test runtime PASS.

## 2. Nen sua

- Targeting hien dang di qua mot boss offline duy nhat; khi game co nhieu enemy can truyen danh sach ung vien thay vi mot `targetPosition`.
- `chooseLockTarget` dung nua mat phang phia truoc cua camera de xap xi tam man hinh; he thong nhieu muc tieu nen dung projection man hinh va diem tam that.
- Capture hien bat collision wireframe de de audit marker; bo loc nay nen co mot preset anh QA rieng neu can anh gameplay sach.

## 3. Goi y

- Them danh sach target co entity id de vẩy chuot doi muc tieu that su khi co nhieu enemy.
- Tach `TargetingResult` thanh gameplay service doc lap voi camera projection neu server can tai su dung quyet dinh lock-on.
- Them regression cho settings `attack_aim_mode` khi F3 thay doi va khoi dong lai client.
