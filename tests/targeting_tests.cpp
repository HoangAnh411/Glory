#include "glory/game/targeting.hpp"

#include <cmath>
#include <iostream>

namespace {
int failures = 0;
void expect(bool condition, const char* message) {
    if (!condition) { std::cerr << "FAIL: " << message << '\n'; ++failures; }
}
}

int main() {
    using namespace glory::game;
    CollisionWorld world;
    const glm::vec3 player{0.0F};
    const glm::vec3 target{3.0F, 0.0F, 0.0F};
    expect(!chooseAttackTarget(world, player, 0.0F, target, true,
        AttackAimMode::Manual, 4.0F).valid, "manual mode leaves attack direction on camera");
    const auto assist = chooseAttackTarget(world, player, 0.0F, target, true,
        AttackAimMode::Assist, 4.0F);
    expect(assist.valid && std::abs(assist.yawDegrees) < 0.01F, "assist selects a target in the cone");
    expect(chooseAttackTarget(world, player, 180.0F, target, true,
        AttackAimMode::Auto, 4.0F).valid, "auto mode ignores camera angle");
    world.addObstacle({{1.0F, 0.0F, -1.0F}, {2.0F, 3.0F, 1.0F}, false, "wall", true});
    expect(!hasLineOfSight(world, player, target), "blocking AABB hides a target");
    expect(!chooseLockTarget(world, player, 0.0F, target, true, 25.0F, 30.0F).valid,
        "lock selection rejects a target behind a wall");
    if (failures != 0) return 1;
    std::cout << "All targeting tests passed\n";
    return 0;
}
