#include "glory/game/spatial_grid.hpp"
#include "glory/network/network.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace {

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void testCellCoordinates() {
    glory::game::SpatialGrid grid(32.0F);
    const auto negative = grid.cellFor({-0.1F, 0.0F, -32.1F});
    const auto positive = grid.cellFor({32.0F, 0.0F, 63.9F});
    require(negative.x == -1 && negative.z == -2, "negative cell coordinates must use floor");
    require(positive.x == 1 && positive.z == 1, "positive cell coordinates should map to 32m cells");
}

void testAoiMovementAndZones() {
    using namespace glory::game;
    SpatialGrid grid;
    require(grid.upsert({1, ZoneId::Zone1Plains, SpatialEntityType::Player, {0.0F, 0.0F, 0.0F}}),
        "register observer");
    require(grid.upsert({2, ZoneId::Zone1Plains, SpatialEntityType::Player, {10.0F, 0.0F, 0.0F}}),
        "register nearby player");
    require(grid.upsert({3, ZoneId::Zone1Plains, SpatialEntityType::Monster, {100.0F, 0.0F, 0.0F}}),
        "register distant target");
    auto visible = grid.query(ZoneId::Zone1Plains, glm::vec3(0.0F), 40.0F);
    require(visible.size() == 2 && visible[0].id == 1 && visible[1].id == 2,
        "AOI should include nearby entities and exclude distant entity");
    require(grid.upsert({4, ZoneId::Zone1Plains, SpatialEntityType::Loot, {5.0F, 100.0F, 0.0F}}),
        "register entity above the ground plane");
    visible = grid.query(ZoneId::Zone1Plains, glm::vec3(0.0F), 40.0F);
    require(visible.size() == 3, "AOI radius should be measured on XZ plane");
    require(grid.remove(4), "remove vertical distance test entity");

    require(grid.upsert({3, ZoneId::Zone1Plains, SpatialEntityType::Monster, {39.0F, 0.0F, 0.0F}}),
        "move target into range");
    visible = grid.query(ZoneId::Zone1Plains, glm::vec3(0.0F), 40.0F);
    require(visible.size() == 3, "moved entity should enter AOI");
    require(grid.upsert({2, ZoneId::Zone1Plains, SpatialEntityType::Player, {80.0F, 0.0F, 0.0F}}),
        "move nearby player out of range");
    visible = grid.query(ZoneId::Zone1Plains, glm::vec3(0.0F), 40.0F);
    require(visible.size() == 2 && visible[0].id == 1 && visible[1].id == 3,
        "entity moving away should disappear from observer AOI");

    require(grid.upsert({2, ZoneId::Zone2Arena, SpatialEntityType::Player, {256.0F, 0.0F, 256.0F}}),
        "transition entity to another zone");
    require(grid.query(ZoneId::Zone1Plains, glm::vec3(0.0F), 500.0F).size() == 2,
        "old zone query should not include the transitioned entity");
    const auto newZone = grid.query(ZoneId::Zone2Arena, {256.0F, 0.0F, 256.0F}, 40.0F);
    require(newZone.size() == 1 && newZone[0].id == 2,
        "new zone query should contain only entities in the destination zone");
    require(grid.remove(3) && grid.find(3) == nullptr && grid.entityCount() == 2,
        "removal should erase entity from grid");
}

void testZoneProtocolAndAoiSnapshot() {
    using namespace glory::network;
    ServerSnapshotPacket snapshot;
    snapshot.serverTick = 42;
    snapshot.currentZone = ZoneId::Zone2Arena;
    snapshot.totalEntityCount = 12;
    snapshot.visibleEntityCount = 3;
    snapshot.players.push_back({7});
    ServerSnapshotPacket decodedSnapshot;
    require(decodeServerSnapshot(encodeServerSnapshot(snapshot), decodedSnapshot) &&
        decodedSnapshot.currentZone == ZoneId::Zone2Arena && decodedSnapshot.totalEntityCount == 12 &&
        decodedSnapshot.visibleEntityCount == 3 && decodedSnapshot.players.size() == 1,
        "AOI snapshot fields should round-trip");

    ZoneChangeRequestPacket request{"session-token", 91, ZoneId::Zone2Arena}, decodedRequest;
    require(decodeZoneChangeRequest(encodeZoneChangeRequest(request), decodedRequest) &&
        decodedRequest.characterId == 91 && decodedRequest.targetZone == ZoneId::Zone2Arena,
        "zone change request should round-trip");
    ZoneChangeNotificationPacket notification{ZoneId::Zone2Arena, 256.0F, 0.0F, 256.0F};
    ZoneChangeNotificationPacket decodedNotification;
    require(decodeZoneChangeNotification(encodeZoneChangeNotification(notification), decodedNotification) &&
        decodedNotification.currentZone == ZoneId::Zone2Arena && decodedNotification.spawnX == 256.0F,
        "zone transition notification should round-trip");
    const auto malformed = encodeZoneChangeNotification(notification);
    require(!decodeZoneChangeNotification({malformed.data(), malformed.size() - 1}, decodedNotification),
        "truncated zone notification should be rejected");
}

} // namespace

int main() {
    try {
        testCellCoordinates();
        testAoiMovementAndZones();
        testZoneProtocolAndAoiSnapshot();
        std::cout << "Spatial grid, AOI, zone transition and protocol tests passed\n";
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
