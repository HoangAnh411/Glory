#include "glory/game/collision.hpp"

#include <glm/geometric.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <random>

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

    CollisionWorld tallStepWorld;
    tallStepWorld.addObstacle({{1.0F, 0.0F, -1.0F}, {1.5F, 0.6F, 1.0F}, true});
    const CollisionResult blockedByTallStep = tallStepWorld.resolve(capsuleAt({0.0F, 0.0F, 0.0F}),
        {2.0F, 0.0F, 0.0F}, {5.5F, 0.0F, 0.0F}, 0.25F);
    expect(blockedByTallStep.position.x <= 0.61F,
        "step offset blocks an authored step taller than 0.25 m");

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

void testFrameRateIndependentWallBlocking() {
    using namespace glory::game;
    CollisionWorld world;
    world.addObstacle({{1.0F, 0.0F, -4.0F}, {1.5F, 3.0F, 4.0F}, false});
    for (const int fps : {30, 60, 144}) {
        for (const float speed : {5.5F, 12.0F}) {
            glm::vec3 position{0.0F};
            glm::vec3 velocity{speed, 0.0F, 0.0F};
            for (int tick = 0; tick < fps * 2; ++tick) {
                const auto result = world.resolve(capsuleAt(position), position + velocity / static_cast<float>(fps), velocity);
                position = result.position;
                velocity = result.velocity;
            }
            expect(position.x <= 0.61F && position.x >= 0.57F,
                "walking and dodge-speed movement cannot cross an AABB at 30/60/144 Hz");
        }
    }
}

void testCornerStabilityAndArenaBoundary() {
    using namespace glory::game;
    CollisionWorld corner;
    corner.addObstacle({{0.9F, 0.0F, -3.0F}, {1.2F, 3.0F, 3.0F}, false});
    corner.addObstacle({{-3.0F, 0.0F, 0.9F}, {3.0F, 3.0F, 1.2F}, false});
    glm::vec3 position{0.0F};
    glm::vec3 velocity{8.0F, 0.0F, 8.0F};
    for (int tick = 0; tick < 1000; ++tick) {
        const auto result = corner.resolve(capsuleAt(position), position + velocity / 60.0F, velocity);
        position = result.position;
        velocity = result.velocity;
    }
    expect(position.x <= 0.51F && position.z <= 0.51F && position.x >= 0.48F && position.z >= 0.48F,
        "repeated contact with a 90-degree corner settles without penetration or jitter");

    CollisionWorld arena;
    arena.setArenaBoundary({2.0F, -3.0F}, 34.0F);
    std::mt19937 random(4519U);
    std::uniform_real_distribution<float> delta(-3.0F, 3.0F);
    position = {2.0F, 0.0F, -3.0F};
    for (int tick = 0; tick < 10000; ++tick) {
        const glm::vec3 desired = position + glm::vec3{delta(random), delta(random) * 0.2F, delta(random)};
        position = arena.resolve(capsuleAt(position), desired, desired - position).position;
        const float dx = position.x - 2.0F;
        const float dz = position.z + 3.0F;
        expect(dx * dx + dz * dz <= (34.0F - 0.4F) * (34.0F - 0.4F) + 1.0e-3F,
            "random movement stays inside the radius-inset arena boundary");
        expect(position.y >= 0.0F, "random movement remains supported by the ground plane");
    }
}
} // namespace

int main() {
    testWallDepenetrationAndSliding();
    testWalkableAndSteepSlopes();
    testStepUpAndSampleTerrain();
    testWorldColliderData();
    testFrameRateIndependentWallBlocking();
    testCornerStabilityAndArenaBoundary();
    if (failures != 0) {
        std::cerr << failures << " collision test(s) failed\n";
        return 1;
    }
    std::cout << "All collision tests passed\n";
    return 0;
}
