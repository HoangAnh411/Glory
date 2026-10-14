# Kế hoạch: Chat, party, guild

## 1. Mục tiêu

Thêm chat nhiều kênh, quản lý party có leader/vitals, guild bền vững trong SQLite, packet giới hạn an toàn và Social panel. Game rules không phụ thuộc renderer; server quyết định routing và membership.

## 2. Tasks

1. Tạo `game/social.hpp/.cpp`: enum kênh chat, validation/routing theo world/zone/party/guild/whisper/system và `PartyManager` giới hạn 5 thành viên, invite/accept/leave/kick, cập nhật vitals và chuyển leader.
2. Mở rộng `glory_network`: codec chat có session/character, party request/sync, guild request/sync; giới hạn số lượng/string/payload và từ chối channel/action sai.
3. Mở rộng `AccountDatabase`: tạo schema `guilds`, `guild_members` có foreign key/unique; API tạo/query guild, chấp nhận invite, rời guild (transfer leader hoặc xóa guild trống), set rank; các thao tác kiểm tra session/ownership và transaction.
4. Tích hợp server: lưu social roster theo character đã vào world; chat routing authoritative, party actions và per-party sync, guild invites/accept/persist/rank, gửi sync khi chọn nhân vật và thay đổi; xóa state khi disconnect.
5. Thêm SocialPanelState/callback và cửa sổ Social: lịch sử chat cuộn, chọn kênh/whisper target, party members/HP/actions, guild info/member/rank/actions.
6. Viết `tests/social_system_tests.cpp`: channel routing, party lifecycle/leader/vitals, guild SQLite lifecycle/reopen/load, codec round-trip/invalid payload; đăng ký CTest thứ 29.
7. Build, chạy 29 CTest và smoke test server/client; review độc lập và chỉ khi đủ PASS mới tick roadmap.

## 3. File dự kiến

- `CMakeLists.txt`
- `game/include/glory/game/social.hpp`, `game/src/social.cpp`
- `network/include/glory/network/network.hpp`, `network/src/network.cpp`
- `server/include/glory/server/account_database.hpp`, `server/src/account_database.cpp`, `server/src/main.cpp`
- `engine/include/glory/engine/application.hpp`, `engine/include/glory/engine/debug_panel.hpp`, `engine/src/application.cpp`, `engine/src/debug_panel.cpp`
- `game/src/main.cpp`
- `tests/social_system_tests.cpp`
- `.ai/specs/chat-party-guild/{plan.md,test-report.md,review.md}`
- `PLAN.md`

## 4. Rủi ro và xử lý

- Client không được tự khai sender, zone, party hoặc guild để định tuyến; server dựng identity từ session/character đang hoạt động.
- Chat giới hạn 1..256 ký tự, giới hạn byte UTF-8 theo MTU ENet và loại `System` chỉ do server phát.
- Party invitations chỉ có hiệu lực cho nhân vật online; membership/vitals sync chỉ gửi đúng thành viên.
- Guild invite là lời mời tạm trong runtime; accept/create/leave/rank ghi SQLite trước khi sync. SQLite transaction và unique character membership tránh guild trùng.
- Leader rời guild chuyển quyền cho thành viên còn lại đầu tiên; guild rỗng sẽ bị xóa.
- Guild sync giới hạn danh sách member trên wire để không vượt packet cap; database vẫn giữ toàn bộ thành viên.

## 5. Tiêu chí chấp nhận

- Routing World/Zone/Party/Guild/Whisper/System trả đúng tập recipient, bỏ sender không hợp lệ, từ chối tin nhắn rỗng hoặc quá giới hạn.
- Party max 5, chỉ leader kick/invite, acceptance đúng invite, vitals sync và leader chuyển khi rời.
- Guild schema có FK/unique; create tên trùng bị từ chối; invite/accept/leave/set rank được xác thực và bền vững qua mở lại database.
- Codec chat/party/guild round-trip và từ chối enum, string, list, payload vượt giới hạn.
- Social UI có lịch sử theo màu/kênh, gửi chat, party list/HP/nút thao tác, guild list/rank/nút thao tác.
- Build PASS, 29/29 CTest PASS, server/client smoke PASS; review không có mục phải sửa.
