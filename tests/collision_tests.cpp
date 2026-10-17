#include "glory/game/collision.hpp"

#include <glm/geometric.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
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
    expect(nearlyEqual(world.arenaRadius(), 90.0F), "world safety radius covers the authored north/south zones");
    expect(world.obstacles().size() >= 60, "world map local bounds load into the game collision world");
    const bool containsWell = std::any_of(world.obstacles().begin(), world.obstacles().end(), [](const Aabb& box) {
        return box.minimum.x < 4.0F && box.maximum.x > 6.0F && box.minimum.z < 5.0F && box.maximum.z > 7.0F;
    });
    expect(containsWell, "world prop collider is transformed from local bounds into world space");

    const glm::vec3 bossCornerPosition{8.25F, 0.0F, 5.565706F};
    const Capsule bossCapsule{bossCornerPosition, 0.55F, 2.2F};
    const CollisionResult depenetrated = world.resolve(bossCapsule, bossCornerPosition, {});
    bool overlapsObstacle = false;
    for (const Aabb& box : world.obstacles()) {
        if (!box.blocksMovement) continue;
        if (depenetrated.position.y >= box.maximum.y ||
            depenetrated.position.y + bossCapsule.height <= box.minimum.y) continue;
        const float closestX = std::clamp(depenetrated.position.x, box.minimum.x, box.maximum.x);
        const float closestZ = std::clamp(depenetrated.position.z, box.minimum.z, box.maximum.z);
        const float dx = depenetrated.position.x - closestX;
        const float dz = depenetrated.position.z - closestZ;
        overlapsObstacle |= dx * dx + dz * dz < bossCapsule.radius * bossCapsule.radius - 1.0e-4F;
    }
    expect(!overlapsObstacle,
        "boss capsule is depenetrated from consecutive world boxes at the live walk-test failure point");

    const auto hasType = [&](const std::string& prefix, bool blocksMovement) {
        return std::any_of(world.obstacles().begin(), world.obstacles().end(), [&](const Aabb& box) {
            return box.name.starts_with(prefix) && box.blocksMovement == blocksMovement;
        });
    };
    expect(hasType("bush_flower_cluster#", false), "bush bounds are loaded for debug but do not block movement");
    expect(hasType("terrain_hill_green#", false), "large decorative hills do not block movement");
    expect(hasType("terrain_cliff_rock#71", false), "distant backdrop cliffs remain non-blocking");
    expect(hasType("terrain_cliff_rock#60", true), "near-arena cliff walls have physical collision");
    expect(hasType("bench_wooden#", true) && hasType("fence_farm_wood#", true),
        "solid benches and fences remain movement blockers");
    const auto tree = std::find_if(world.obstacles().begin(), world.obstacles().end(), [](const Aabb& box) {
        return box.name.starts_with("tree_village_oak#");
    });
    expect(tree != world.obstacles().end() && tree->blocksMovement &&
        tree->maximum.x - tree->minimum.x <= 1.5F && tree->maximum.z - tree->minimum.z <= 1.5F,
        "tree collision uses a narrow trunk footprint instead of canopy bounds");

    for (const int gateIndex : {35, 51}) {
        const std::string gateName = "village_gate_tower#" + std::to_string(gateIndex);
        const auto gateParts = std::count_if(world.obstacles().begin(), world.obstacles().end(),
            [&](const Aabb& box) { return box.name == gateName && box.blocksMovement; });
        expect(gateParts == 4, "each gate uses four support colliders, not a solid bounding box");

        const float gateZ = gateIndex == 35 ? 24.0F : -24.0F;
        const CollisionResult edgeSupport = world.resolve(capsuleAt({1.5F, 0.0F, gateZ}),
            {7.0F, 0.0F, gateZ}, {5.5F, 0.0F, 0.0F});
        expect(edgeSupport.position.x <= 2.9F && edgeSupport.contactCount > 0,
            "each gate support blocks traversal beside the opening");
    }
}

void testAuthoredGateOpeningsAndCliffWall() {
    using namespace glory::game;
    const CollisionWorld world = CollisionWorld::loadFromFiles(
        std::filesystem::path(GLORY_ASSET_DIR) / "data" / "world_map.json",
        std::filesystem::path(GLORY_ASSET_DIR) / "data" / "world_colliders.json");

    const CollisionResult northGate = world.resolve(capsuleAt({0.0F, 0.0F, 18.0F}),
        {0.0F, 0.0F, 30.0F}, {0.0F, 0.0F, 12.0F});
    expect(nearlyEqual(northGate.position.z, 30.0F) && northGate.contactCount == 0,
        "player capsule passes through the north gate opening");
    const CollisionResult southGate = world.resolve(capsuleAt({0.0F, 0.0F, -18.0F}),
        {0.0F, 0.0F, -30.0F}, {0.0F, 0.0F, -12.0F});
    expect(nearlyEqual(southGate.position.z, -30.0F) && southGate.contactCount == 0,
        "player capsule passes through the south gate opening");
    const CollisionResult northRoad = world.resolve(capsuleAt({0.0F, 0.0F, 28.0F}),
        {0.0F, 0.0F, 40.0F}, {0.0F, 0.0F, 12.0F});
    expect(nearlyEqual(northRoad.position.z, 40.0F) && northRoad.contactCount == 0,
        "open north road remains traversable beyond the village gate and former radius-34 clamp");
    const CollisionResult southRoad = world.resolve(capsuleAt({0.0F, 0.0F, -28.0F}),
        {0.0F, 0.0F, -40.0F}, {0.0F, 0.0F, -12.0F});
    expect(nearlyEqual(southRoad.position.z, -40.0F) && southRoad.contactCount == 0,
        "open south road remains traversable beyond the village gate and former radius-34 clamp");
    const CollisionResult bossAtNorthGate = world.resolve(Capsule{{0.0F, 0.0F, 18.0F}, 0.55F, 2.2F},
        {0.0F, 0.0F, 30.0F}, {0.0F, 0.0F, 12.0F});
    expect(nearlyEqual(bossAtNorthGate.position.z, 30.0F) && bossAtNorthGate.contactCount == 0,
        "boss capsule also passes through the gate opening");
    const CollisionResult gateSupport = world.resolve(capsuleAt({1.5F, 0.0F, 24.0F}),
        {7.0F, 0.0F, 24.0F}, {2.8F, 0.0F, 0.0F});
    expect(gateSupport.position.x <= 2.9F && gateSupport.contactCount > 0,
        "gate side support remains solid beside the opening");

    for (const int cliffIndex : {60, 61, 62, 65, 66, 67}) {
        const std::string cliffName = "terrain_cliff_rock#" + std::to_string(cliffIndex);
        const auto cliff = std::find_if(world.obstacles().begin(), world.obstacles().end(),
            [&](const Aabb& box) { return box.name == cliffName && box.blocksMovement; });
        expect(cliff != world.obstacles().end(), "every near-arena cliff has a blocking authored collider");
        if (cliff == world.obstacles().end()) continue;
        CollisionWorld isolatedWall;
        isolatedWall.setArenaBoundary(world.arenaCenter(), 1000.0F);
        isolatedWall.addObstacle(*cliff);
        const float z = 0.5F * (cliff->minimum.z + cliff->maximum.z);
        const CollisionResult wall = isolatedWall.resolve(capsuleAt({cliff->minimum.x - 2.0F, 0.0F, z}),
            {cliff->maximum.x + 3.0F, 0.0F, z},
            {cliff->maximum.x - cliff->minimum.x + 5.0F, 0.0F, 0.0F});
        expect(wall.position.x <= cliff->minimum.x - 0.35F && wall.contactCount > 0,
            "each near-arena cliff wall blocks without relying on the arena radius");
    }
}

void testInstanceShapeOverridesMapCompoundCollision() {
    using namespace glory::game;
    const auto uniqueId = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto fixtureDirectory = std::filesystem::temp_directory_path() /
        ("glory-collision-override-" + std::to_string(uniqueId));
    std::filesystem::create_directories(fixtureDirectory);
    const auto mapPath = fixtureDirectory / "map.json";
    const auto colliderPath = fixtureDirectory / "colliders.json";
    {
        std::ofstream mapFile(mapPath);
        mapFile << R"({"props":[{"type":"gate","position":[0,0,0],"yaw":0,"scale":1,
            "multi_collision":[[-5,0,-1,-2,4,1]]}]})";
        std::ofstream colliderFile(colliderPath);
        colliderFile << R"({"arena":{"center":[0,0],"radius":90},"instanceOverrides":{
            "gate#0":{"blocksMovement":true,"parts":[[-5,0,-1,-4,4,1],[4,0,-1,5,4,1]]}}})";
    }
    const CollisionWorld world = CollisionWorld::loadFromFiles(mapPath, colliderPath);
    std::filesystem::remove_all(fixtureDirectory);
    expect(world.obstacles().size() == 2 &&
        std::all_of(world.obstacles().begin(), world.obstacles().end(), [](const Aabb& box) {
            return box.name == "gate#0" && box.blocksMovement;
        }), "instance compound shape replaces generated multi-collision instead of adding a stale blocker");
}

void testNonBlockingDecorationAndBlockingTrunk() {
    using namespace glory::game;
    CollisionWorld world;
    world.addObstacle({{1.0F, 0.0F, -1.0F}, {3.0F, 5.0F, 1.0F}, false,
        "decorative canopy", false});
    world.addObstacle({{4.0F, 0.0F, -0.35F}, {4.7F, 3.5F, 0.35F}, false,
        "tree trunk", true});

    const CollisionResult throughDecoration = world.resolve(capsuleAt({0.0F, 0.0F, 0.0F}),
        {3.5F, 0.0F, 0.0F}, {3.5F, 0.0F, 0.0F});
    expect(nearlyEqual(throughDecoration.position.x, 3.5F) && throughDecoration.contactCount == 0,
        "capsule passes through a non-blocking decorative bound");
    const CollisionResult atTrunk = world.resolve(capsuleAt({3.0F, 0.0F, 0.0F}),
        {5.0F, 0.0F, 0.0F}, {2.0F, 0.0F, 0.0F});
    expect(atTrunk.position.x < 4.0F && atTrunk.contactCount > 0,
        "capsule remains blocked by the narrow tree trunk collider");
}

void testCapsuleActorSeparation() {
    using namespace glory::game;
    const CapsuleSeparation separated = separateCapsules(capsuleAt({0.0F, 0.0F, 0.0F}),
        capsuleAt({0.5F, 0.0F, 0.0F}));
    expect(separated.first.x < 0.0F && separated.second.x > 0.0F,
        "overlapping actor capsules receive opposite horizontal corrections");
    const glm::vec3 first = glm::vec3(0.0F) + separated.first;
    const glm::vec3 second{0.5F + separated.second.x, 0.0F, separated.second.z};
    expect(glm::distance(glm::vec2(first.x, first.z), glm::vec2(second.x, second.z)) >= 0.81F,
        "separation clears the capsule radii plus skin");
    expect(glm::length(separateCapsules(capsuleAt({0.0F, 0.0F, 0.0F}),
        capsuleAt({2.0F, 0.0F, 0.0F})).first) == 0.0F,
        "non-overlapping actor capsules do not move");
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
    const CollisionResult throughOpening = arena.resolve(capsuleAt({2.0F, 0.0F, 30.0F}),
        {2.0F, 0.0F, 31.2F}, {0.0F, 0.0F, 1.2F});
    expect(nearlyEqual(throughOpening.position.z, 31.2F),
        "arena radius does not block a passable gap before the safety margin");
    const CollisionResult recovered = arena.resolve(capsuleAt(throughOpening.position),
        {2.0F, 0.0F, 32.0F}, {0.0F, 0.0F, 0.8F});
    expect(recovered.position.z + 3.0F <= 34.0F + 1.0e-3F &&
        recovered.position.z + 3.0F >= 34.0F - 0.4F - 1.0e-3F,
        "arena safety boundary recovers actors only after leaving the safety margin");
    std::mt19937 random(4519U);
    std::uniform_real_distribution<float> delta(-3.0F, 3.0F);
    position = {2.0F, 0.0F, -3.0F};
    for (int tick = 0; tick < 10000; ++tick) {
        const glm::vec3 desired = position + glm::vec3{delta(random), delta(random) * 0.2F, delta(random)};
        position = arena.resolve(capsuleAt(position), desired, desired - position).position;
        const float dx = position.x - 2.0F;
        const float dz = position.z + 3.0F;
        expect(dx * dx + dz * dz <= (34.0F + 0.5F) * (34.0F + 0.5F) + 1.0e-3F,
            "random movement remains within the arena safety recovery margin");
        expect(position.y >= 0.0F, "random movement remains supported by the ground plane");
    }
}
} // namespace

int main() {
    testWallDepenetrationAndSliding();
    testWalkableAndSteepSlopes();
    testStepUpAndSampleTerrain();
    testWorldColliderData();
    testAuthoredGateOpeningsAndCliffWall();
    testInstanceShapeOverridesMapCompoundCollision();
    testNonBlockingDecorationAndBlockingTrunk();
    testCapsuleActorSeparation();
    testFrameRateIndependentWallBlocking();
    testCornerStabilityAndArenaBoundary();
    if (failures != 0) {
        std::cerr << failures << " collision test(s) failed\n";
        return 1;
    }
    std::cout << "All collision tests passed\n";
    return 0;
}
