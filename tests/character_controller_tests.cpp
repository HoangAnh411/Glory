#include "glory/game/character_controller.hpp"

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>

#include <cmath>
#include <algorithm>
#include <iostream>
#include <utility>
#include <vector>

namespace {
int failures = 0;

void expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

bool nearlyEqual(float left, float right, float epsilon = 1.0e-3F) {
    return std::abs(left - right) <= epsilon;
}

void applyInput(glory::game::ActionMap& actions, std::vector<glory::input::Control> held,
    std::vector<glory::input::Control> pressed = {}) {
    glory::input::InputFrame frame;
    frame.held = std::move(held);
    frame.pressed = std::move(pressed);
    actions.update(frame);
}

void testCameraRelativeMovement() {
    using glory::game::Action;
    using glory::game::ActionMap;
    using glory::game::CharacterController;
    using glory::input::Control;
    ActionMap actions;
    CharacterController controller;
    applyInput(actions, {Control::KeyW});
    controller.update(actions, -90.0F, 1.0);
    expect(nearlyEqual(controller.state().position.z, -5.5F), "forward follows camera yaw");

    applyInput(actions, {Control::KeyS});
    controller.update(actions, -90.0F, 1.0);
    expect(nearlyEqual(controller.state().position.z, 0.0F), "backward reverses forward movement");

    applyInput(actions, {Control::KeyD});
    controller.update(actions, -90.0F, 1.0);
    expect(nearlyEqual(controller.state().position.x, 5.5F), "right strafe follows camera right vector");

    applyInput(actions, {Control::KeyW, Control::KeyD});
    const glm::vec3 before = controller.state().position;
    controller.update(actions, 0.0F, 1.0);
    expect(nearlyEqual(glm::length(controller.state().position - before), 5.5F),
        "diagonal movement is normalized to run speed");
}

void testSmoothMovementAndTurnRate() {
    using namespace glory::game;
    using glory::input::Control;
    ActionMap actions;
    CharacterController controller;
    CollisionWorld openWorld;
    controller.setCollisionWorld(openWorld);
    applyInput(actions, {Control::KeyW});
    controller.update(actions, 0.0F, 1.0 / 60.0);
    const float firstSpeed = glm::length(glm::vec2(controller.state().velocity.x, controller.state().velocity.z));
    expect(firstSpeed > 0.0F && firstSpeed < 5.5F * 0.95F,
        "movement accelerates instead of applying full speed on the first tick");
    for (int tick = 0; tick < 10; ++tick) controller.update(actions, 0.0F, 1.0 / 60.0);
    expect(glm::length(glm::vec2(controller.state().velocity.x, controller.state().velocity.z)) > 5.4F,
        "movement reaches the configured run speed smoothly");
    applyInput(actions, {});
    controller.update(actions, 0.0F, 1.0 / 60.0);
    const float releaseSpeed = glm::length(glm::vec2(controller.state().velocity.x, controller.state().velocity.z));
    expect(releaseSpeed > 0.0F && releaseSpeed < 5.5F,
        "movement decelerates instead of stopping instantaneously");

    controller.teleport({}, -90.0F);
    applyInput(actions, {Control::KeyW});
    controller.update(actions, 90.0F, 1.0 / 60.0);
    expect(std::abs(std::remainder(controller.state().facingYaw - (-78.0F), 360.0F)) < 0.01F,
        "facing rotation is capped at 720 degrees per second");
    const float yawWhileMoving = controller.state().facingYaw;
    applyInput(actions, {});
    controller.update(actions, 90.0F, 1.0 / 60.0);
    expect(nearlyEqual(controller.state().facingYaw, yawWhileMoving), "idle input does not rotate the character");
}

void testDiagonalWallSlideUsesSolver() {
    using namespace glory::game;
    using glory::input::Control;
    ActionMap actions;
    CharacterController controller;
    CollisionWorld world;
    world.addObstacle({{1.0F, -1.0F, -20.0F}, {1.5F, 3.0F, 20.0F}, false, "test_wall", true});
    controller.setCollisionWorld(world);
    controller.teleport({}, 0.0F);
    applyInput(actions, {Control::KeyW, Control::KeyA});
    for (int tick = 0; tick < 90; ++tick) controller.update(actions, 0.0F, 1.0 / 60.0);
    expect(controller.state().position.x <= 0.61F && controller.state().position.z < -1.0F,
        "diagonal movement stops at the wall and slides along it through the solver");
}

void testJumpGravityAndGrounding() {
    using namespace glory::game;
    ActionMap actions;
    CharacterController controller;
    applyInput(actions, {glory::input::Control::KeySpace}, {glory::input::Control::KeySpace});
    controller.update(actions, -90.0F, 1.0 / 120.0);
    expect(!controller.state().isGrounded && controller.state().velocity.y > 0.0F,
        "jump starts only from the grounded state");
    float highestY = controller.state().position.y;
    applyInput(actions, {});
    for (int tick = 0; tick < 1200; ++tick) {
        controller.update(actions, -90.0F, 1.0 / 120.0);
        highestY = std::max(highestY, controller.state().position.y);
    }
    expect(highestY > 1.0F, "jump reaches a positive apex");
    expect(nearlyEqual(controller.state().position.y, 0.0F), "falling character lands on the ground plane");
    expect(nearlyEqual(controller.state().velocity.y, 0.0F) && controller.state().isGrounded,
        "landing resets vertical velocity and grounded state");
}

void testFacingRotationAndAttackEdge() {
    using namespace glory::game;
    ActionMap actions;
    CharacterController controller;
    applyInput(actions, {glory::input::Control::KeyW});
    controller.update(actions, 0.0F, 0.1);
    expect(controller.state().facingYaw > -90.0F && controller.state().facingYaw < 0.0F,
        "facing turns smoothly toward the movement direction");
    for (int tick = 0; tick < 10; ++tick) controller.update(actions, 0.0F, 0.1);
    expect(nearlyEqual(controller.state().facingYaw, 0.0F), "facing converges to movement direction");

    applyInput(actions, {glory::input::Control::MouseLeft}, {glory::input::Control::MouseLeft});
    controller.update(actions, 0.0F, 1.0 / 60.0);
    expect(controller.state().attackRequested, "primary attack emits a one-tick request");
    applyInput(actions, {glory::input::Control::MouseLeft});
    controller.update(actions, 0.0F, 1.0 / 60.0);
    expect(!controller.state().attackRequested, "held attack does not retrigger each tick");
}

void testDodgeSpeedIFramesAndCooldown() {
    using namespace glory::game;
    ActionMap actions;
    CharacterController controller;
    glory::input::InputFrame frame;
    frame.held = {glory::input::Control::KeyLeftShift};
    frame.pressed = frame.held;
    actions.update(frame);
    controller.update(actions, -90.0F, 0.05);
    expect(controller.state().isDodging, "dodge starts on the dodge action press");
    expect(controller.state().isInvulnerable, "i-frame activates at the configured window start");
    expect(nearlyEqual(glm::length(glm::vec2(controller.state().velocity.x, controller.state().velocity.z)), 12.0F),
        "dodge uses the configured dash speed");
    expect(controller.state().position.z > 0.5F, "stationary dodge moves backward from the facing direction");

    frame.pressed.clear();
    actions.update(frame);
    controller.update(actions, -90.0F, 0.23);
    expect(!controller.state().isInvulnerable, "i-frame expires at the configured window end");
    controller.update(actions, -90.0F, 0.07);
    expect(!controller.state().isDodging, "dash ends at dodge duration");

    frame.pressed = {glory::input::Control::KeyLeftShift};
    actions.update(frame);
    controller.update(actions, -90.0F, 0.01);
    expect(!controller.state().isDodging, "cooldown prevents an immediate second dash");

    frame.pressed.clear();
    frame.held.clear();
    actions.update(frame);
    controller.update(actions, -90.0F, 0.49);
    frame.held = {glory::input::Control::KeyLeftShift};
    frame.pressed = frame.held;
    actions.update(frame);
    controller.update(actions, -90.0F, 0.01);
    expect(controller.state().isDodging, "dodge can be used again after its cooldown expires");
}

void testLockOnDodgePrefersExplicitMovement() {
    using glory::game::ActionMap;
    using glory::game::CharacterController;
    using glory::input::Control;

    ActionMap movingActions;
    CharacterController movingController;
    applyInput(movingActions, {Control::KeyW, Control::KeyLeftShift}, {Control::KeyLeftShift});
    movingController.update(movingActions, 0.0F, 1.0 / 60.0, true, 1.0F, false, true,
        {0.0F, 0.0F, 10.0F});
    expect(nearlyEqual(movingController.state().dodgeDirection.x, 1.0F) &&
        nearlyEqual(movingController.state().dodgeDirection.z, 0.0F),
        "lock-on dodge follows explicit camera-relative movement input");

    ActionMap targetOnlyActions;
    CharacterController targetOnlyController;
    applyInput(targetOnlyActions, {Control::KeyLeftShift}, {Control::KeyLeftShift});
    targetOnlyController.update(targetOnlyActions, 0.0F, 1.0 / 60.0, true, 1.0F, false, true,
        {0.0F, 0.0F, 10.0F});
    expect(nearlyEqual(targetOnlyController.state().dodgeDirection.x, 0.0F) &&
        nearlyEqual(targetOnlyController.state().dodgeDirection.z, 1.0F),
        "lock-on dodge without movement input follows the target");
}
} // namespace

int main() {
    testCameraRelativeMovement();
    testSmoothMovementAndTurnRate();
    testDiagonalWallSlideUsesSolver();
    testJumpGravityAndGrounding();
    testFacingRotationAndAttackEdge();
    testDodgeSpeedIFramesAndCooldown();
    testLockOnDodgePrefersExplicitMovement();
    if (failures != 0) {
        std::cerr << failures << " character controller test(s) failed\n";
        return 1;
    }
    std::cout << "All character controller tests passed\n";
    return 0;
}
