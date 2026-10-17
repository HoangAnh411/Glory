#include "glory/game/collision.hpp"

#include <glm/geometric.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>

namespace {
int failures = 0;
void expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}
bool nearlyEqual(float left, float right, float epsilon = 2.0e-2F) {
    return std::abs(left - right) <= epsilon;
}
glory::game::Capsule capsuleAt(glm::vec3 base) { return {base, 0.4F, 1.8F}; }

void testWallDepenetrationAndSliding() {
    using namespace glory::game;
    CollisionWorld wallWorld;
    wallWorld.addObstacle({{1.0F, 0.0F, -1.0F}, {1.5F, 2.0F, 1.0F}, false});
    const CollisionResult stopped = wallWorld.resolve(capsuleAt({0.0F, 0.0F, 0.0F}),
        {2.0F, 0.0F, 0.0F}, {2.0F, 0.0F, 0.0F});
    expect(stopped.position.x <= 0.61F && stopped.position.x >= 0.57F,
        "capsule is pushed out and stopped at the wall radius");
    expect(stopped.velocity.x < 1.0e-3F && stopped.contactCount > 0,
        "wall contact removes inward velocity and reports a contact");

    const CollisionResult depenetrated = wallWorld.resolve(capsuleAt({1.2F, 0.0F, 0.0F}),
        {1.2F, 0.0F, 0.0F}, {});
    expect(depenetrated.position.x < 0.61F || depenetrated.position.x > 1.9F,
        "initial capsule overlap is depenetrated to a non-overlapping side");

    const CollisionResult slid = wallWorld.resolve(capsuleAt({0.0F, 0.0F, -0.5F}),
        {2.0F, 0.0F, 0.5F}, {1.0F, 0.0F, 1.0F});
    expect(slid.position.x <= 0.61F && slid.position.z > 0.2F,
        "movement projects along the wall tangent while blocking the normal");
}

void testWalkableAndSteepSlopes() {
    using namespace glory::game;
    CollisionWorld walkableWorld;
    walkableWorld.addRamp({1.0F, 3.0F, -1.0F, 1.0F, 0.0F, 0.8F});
    const CollisionResult uphill = walkableWorld.resolve(capsuleAt({0.0F, 0.0F, 0.0F}),
        {2.8F, 0.0F, 0.0F}, {2.0F, 0.0F, 0.0F});
    expect(uphill.position.x > 2.5F && nearlyEqual(uphill.position.y, 0.72F),
        "walkable slope raises the capsule along the ramp plane");
    expect(uphill.isGrounded && walkableWorld.ramps().front().normal().y >= 0.707F,
        "slope below 45 degrees provides walkable ground contact");

    CollisionWorld steepWorld;
    steepWorld.addRamp({1.0F, 3.0F, -1.0F, 1.0F, 0.0F, 2.0F});
    const CollisionResult blocked = steepWorld.resolve(capsuleAt({0.0F, 0.0F, 0.0F}),
        {2.0F, 0.0F, 0.0F}, {2.0F, 0.0F, 0.0F});
    expect(blocked.position.x < 0.7F && blocked.contactCount > 0,
        "slope at or above 45 degrees blocks uphill movement");
}

void testStepUpAndSampleTerrain() {
    using namespace glory::game;
    CollisionWorld stepWorld;
    stepWorld.addStep({{1.0F, 0.0F, -1.0F}, {1.5F, 0.2F, 1.0F}, false});
    const CollisionResult stepped = stepWorld.resolve(capsuleAt({0.0F, 0.0F, 0.0F}),
        {1.3F, 0.0F, 0.0F}, {2.0F, 0.0F, 0.0F}, 0.25F);
    expect(stepped.position.x > 1.0F && nearlyEqual(stepped.position.y, 0.2F),
        "capsule steps over a low obstacle within the step height");
    expect(stepped.isGrounded, "capsule remains grounded on the raised step surface");

    const CollisionWorld sample = CollisionWorld::makeSampleWorld();
    expect(sample.obstacles().size() == 2 && sample.ramps().size() == 2,
        "sample terrain includes a wall, two slopes, and a low step");
}

void testWorldColliderData() {
    using namespace glory::game;
    const CollisionWorld world = CollisionWorld::loadFromFiles(
        std::filesystem::path(GLORY_ASSET_DIR) / "data" / "world_map.json",
        std::filesystem::path(GLORY_ASSET_DIR) / "data" / "world_colliders.json");
    expect(nearlyEqual(world.arenaRadius(), 34.0F), "arena collider data loads the authored boundary radius");
    expect(world.obstacles().size() >= 60, "world map local bounds load into the game collision world");
    const bool containsWell = std::any_of(world.obstacles().begin(), world.obstacles().end(), [](const Aabb& box) {
        return box.minimum.x < 4.0F && box.maximum.x > 6.0F && box.minimum.z < 5.0F && box.maximum.z > 7.0F;
    });
    expect(containsWell, "world prop collider is transformed from local bounds into world space");
}
} // namespace

int main() {
    testWallDepenetrationAndSliding();
    testWalkableAndSteepSlopes();
    testStepUpAndSampleTerrain();
    testWorldColliderData();
    if (failures != 0) {
        std::cerr << failures << " collision test(s) failed\n";
        return 1;
    }
    std::cout << "All collision tests passed\n";
    return 0;
}
