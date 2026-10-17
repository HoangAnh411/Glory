#include "glory/game/character_controller.hpp"

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace glory::game {
namespace {

constexpr float degreesToRadians = 0.01745329251994329577F;

float wrapDegrees(float degrees) noexcept { return std::remainder(degrees, 360.0F); }

} // namespace

CharacterController::CharacterController(const CharacterControllerConfig& config) : config_(config) {
    if (!std::isfinite(config_.moveSpeed) || config_.moveSpeed < 0.0F ||
        !std::isfinite(config_.jumpVelocity) || config_.jumpVelocity < 0.0F ||
        !std::isfinite(config_.gravity) || config_.gravity < 0.0F ||
        !std::isfinite(config_.rotationSpeedDegrees) || config_.rotationSpeedDegrees < 0.0F ||
        !std::isfinite(config_.dodgeDuration) || config_.dodgeDuration <= 0.0F ||
        !std::isfinite(config_.dodgeSpeed) || config_.dodgeSpeed < 0.0F ||
        !std::isfinite(config_.dodgeCooldown) || config_.dodgeCooldown < 0.0F ||
        !std::isfinite(config_.invulnerabilityStart) || config_.invulnerabilityStart < 0.0F ||
        !std::isfinite(config_.invulnerabilityEnd) || config_.invulnerabilityEnd < config_.invulnerabilityStart ||
        config_.invulnerabilityEnd > config_.dodgeDuration ||
        !std::isfinite(config_.capsuleRadius) || config_.capsuleRadius <= 0.0F ||
        !std::isfinite(config_.capsuleHeight) || config_.capsuleHeight < config_.capsuleRadius * 2.0F ||
        !std::isfinite(config_.maximumStepHeight) || config_.maximumStepHeight < 0.0F) {
        throw std::invalid_argument("Invalid character controller configuration");
    }
}

void CharacterController::update(const ActionMap& actions, float cameraYawDegrees,
    double fixedDeltaSeconds, bool allowDodge, float speedMultiplier, bool stunned) noexcept {
    state_.attackRequested = false;
    if (!std::isfinite(fixedDeltaSeconds) || fixedDeltaSeconds <= 0.0 ||
        !std::isfinite(cameraYawDegrees)) return;

    const float delta = static_cast<float>(fixedDeltaSeconds);
    if (!std::isfinite(delta)) return;
    state_.attackRequested = !stunned && actions.state(Action::PrimaryAttack).pressed;
    state_.collisionCount = 0;
    state_.dodgeCooldownRemaining = std::max(0.0F, state_.dodgeCooldownRemaining - delta);

    const float yaw = cameraYawDegrees * degreesToRadians;
    const glm::vec3 forward{std::cos(yaw), 0.0F, std::sin(yaw)};
    const glm::vec3 right{-std::sin(yaw), 0.0F, std::cos(yaw)};
    glm::vec3 moveDirection{0.0F};
    if (actions.state(Action::MoveForward).held) moveDirection += forward;
    if (actions.state(Action::MoveBackward).held) moveDirection -= forward;
    if (actions.state(Action::MoveRight).held) moveDirection += right;
    if (actions.state(Action::MoveLeft).held) moveDirection -= right;

    if (stunned) moveDirection = glm::vec3(0.0F);
    const float moveLength = glm::length(moveDirection);
    const bool hasMoveInput = moveLength > 1.0e-6F;
    if (moveLength > 1.0e-6F) {
        moveDirection /= moveLength;
        const float effectiveSpeed = config_.moveSpeed * (std::isfinite(speedMultiplier)
            ? std::clamp(speedMultiplier, 0.0F, 4.0F) : 1.0F);
        state_.velocity.x = moveDirection.x * effectiveSpeed;
        state_.velocity.z = moveDirection.z * effectiveSpeed;
        const float desiredYaw = std::atan2(moveDirection.z, moveDirection.x) / degreesToRadians;
        const float yawDifference = wrapDegrees(desiredYaw - state_.facingYaw);
        const float maxTurn = config_.rotationSpeedDegrees * delta;
        state_.facingYaw = wrapDegrees(state_.facingYaw + std::clamp(yawDifference, -maxTurn, maxTurn));
    } else {
        state_.velocity.x = 0.0F;
        state_.velocity.z = 0.0F;
    }

    if (allowDodge && !stunned && actions.state(Action::Dodge).pressed &&
        state_.dodgeCooldownRemaining <= 0.0F && !state_.isDodging) {
        if (hasMoveInput) state_.dodgeDirection = moveDirection;
        else {
            const float facingRadians = state_.facingYaw * degreesToRadians;
            state_.dodgeDirection = {std::cos(facingRadians), 0.0F, std::sin(facingRadians)};
        }
        state_.isDodging = true;
        state_.dodgeElapsed = 0.0F;
        state_.dodgeCooldownRemaining = config_.dodgeCooldown;
    }

    const float previousDodgeElapsed = state_.dodgeElapsed;
    const bool dodgeActiveThisTick = state_.isDodging;
    if (dodgeActiveThisTick) {
        state_.dodgeElapsed = std::min(config_.dodgeDuration, state_.dodgeElapsed + delta);
        state_.isDodging = state_.dodgeElapsed < config_.dodgeDuration;
        state_.isInvulnerable = state_.dodgeElapsed >= config_.invulnerabilityStart &&
            state_.dodgeElapsed < config_.invulnerabilityEnd;
        state_.velocity.x = state_.dodgeDirection.x * config_.dodgeSpeed;
        state_.velocity.z = state_.dodgeDirection.z * config_.dodgeSpeed;
    } else {
        state_.isInvulnerable = false;
    }

    if (!stunned && actions.state(Action::Jump).pressed && state_.isGrounded && !state_.isDodging) {
        state_.velocity.y = config_.jumpVelocity;
        state_.isGrounded = false;
    }
    if (!state_.isGrounded) state_.velocity.y -= config_.gravity * delta;

    const float activeDodgeDelta = dodgeActiveThisTick
        ? std::max(0.0F, std::min(delta, config_.dodgeDuration - previousDodgeElapsed)) : 0.0F;
    const float knockbackStep = std::min(delta, state_.knockbackRemaining);
    const float knockbackSpeed = glm::length(state_.externalKnockbackVelocity);
    glm::vec3 knockbackDisplacement{0.0F};
    if (knockbackStep > 0.0F && knockbackSpeed > 1.0e-5F) {
        const float nextSpeed = state_.knockbackRemaining > 0.0F
            ? knockbackSpeed * std::max(0.0F,
                (state_.knockbackRemaining - knockbackStep) / state_.knockbackRemaining) : 0.0F;
        const glm::vec3 direction = state_.externalKnockbackVelocity / knockbackSpeed;
        knockbackDisplacement = direction * ((knockbackSpeed + nextSpeed) * 0.5F * knockbackStep);
        state_.externalKnockbackVelocity = direction * nextSpeed;
        state_.knockbackRemaining = std::max(0.0F, state_.knockbackRemaining - knockbackStep);
    }
    glm::vec3 desiredPosition = state_.position + state_.velocity * delta + knockbackDisplacement;
    if (dodgeActiveThisTick) {
        desiredPosition.x = state_.position.x + state_.dodgeDirection.x * config_.dodgeSpeed * activeDodgeDelta;
        desiredPosition.z = state_.position.z + state_.dodgeDirection.z * config_.dodgeSpeed * activeDodgeDelta;
        desiredPosition += knockbackDisplacement;
    }
    const Capsule capsule{state_.position, config_.capsuleRadius, config_.capsuleHeight};
    const CollisionResult collision = collisionEnabled_
        ? collisionWorld_.resolve(capsule, desiredPosition, state_.velocity, config_.maximumStepHeight)
        : CollisionResult{desiredPosition, state_.velocity, desiredPosition.y <= 0.0F, 0};
    state_.position = collision.position;
    state_.velocity = collision.velocity;
    state_.isGrounded = collision.isGrounded;
    state_.collisionCount = collision.contactCount;

}

void CharacterController::applyKnockback(const glm::vec3& velocity) noexcept {
    if (!std::isfinite(velocity.x) || !std::isfinite(velocity.y) || !std::isfinite(velocity.z)) return;
    state_.externalKnockbackVelocity = velocity;
    state_.knockbackRemaining = 0.25F;
}

void CharacterController::resolveWorldDisplacement(const glm::vec3& displacement) noexcept {
    if (!collisionEnabled_ || !std::isfinite(displacement.x) || !std::isfinite(displacement.y) ||
        !std::isfinite(displacement.z)) return;
    const CollisionResult result = collisionWorld_.resolve(
        {state_.position, config_.capsuleRadius, config_.capsuleHeight},
        state_.position + displacement, state_.velocity, config_.maximumStepHeight);
    state_.position = result.position;
    state_.velocity = result.velocity;
    state_.isGrounded = result.isGrounded;
    state_.collisionCount += result.contactCount;
}

bool CharacterController::setClassMovement(float moveSpeed, float dodgeCooldown) noexcept {
    if (!std::isfinite(moveSpeed) || moveSpeed <= 0.0F ||
        !std::isfinite(dodgeCooldown) || dodgeCooldown < 0.0F) return false;
    config_.moveSpeed = moveSpeed;
    config_.dodgeCooldown = dodgeCooldown;
    return true;
}

void CharacterController::reset() noexcept {
    state_ = {};
    state_.facingYaw = -90.0F;
    state_.dodgeDirection = glm::vec3(0.0F);
    state_.externalKnockbackVelocity = glm::vec3(0.0F);
    state_.knockbackRemaining = 0.0F;
}

void CharacterController::teleport(const glm::vec3& position, float yawDegrees) noexcept {
    state_.position = position;
    state_.velocity = glm::vec3(0.0F);
    state_.facingYaw = yawDegrees;
    state_.isGrounded = true;
    state_.isDodging = false;
    state_.isInvulnerable = false;
    state_.dodgeElapsed = 0.0F;
    state_.dodgeCooldownRemaining = 0.0F;
    state_.attackRequested = false;
    state_.dodgeDirection = glm::vec3(0.0F);
    state_.externalKnockbackVelocity = glm::vec3(0.0F);
    state_.knockbackRemaining = 0.0F;
}

void CharacterController::restoreNetworkState(const CharacterControllerState& state) noexcept {
    if (!std::isfinite(state.position.x) || !std::isfinite(state.position.y) ||
        !std::isfinite(state.position.z) || !std::isfinite(state.velocity.x) ||
        !std::isfinite(state.velocity.y) || !std::isfinite(state.velocity.z) ||
        !std::isfinite(state.facingYaw) || !std::isfinite(state.dodgeElapsed) ||
        !std::isfinite(state.dodgeCooldownRemaining) || !std::isfinite(state.dodgeDirection.x) ||
        !std::isfinite(state.dodgeDirection.y) || !std::isfinite(state.dodgeDirection.z) ||
        !std::isfinite(state.externalKnockbackVelocity.x) ||
        !std::isfinite(state.externalKnockbackVelocity.y) ||
        !std::isfinite(state.externalKnockbackVelocity.z) || !std::isfinite(state.knockbackRemaining)) return;
    state_ = state;
    state_.dodgeElapsed = std::max(0.0F, state_.dodgeElapsed);
    state_.dodgeCooldownRemaining = std::max(0.0F, state_.dodgeCooldownRemaining);
    const glm::vec2 horizontalVelocity{state_.velocity.x, state_.velocity.z};
    const float horizontalSpeed = glm::length(horizontalVelocity);
    state_.dodgeDirection = state.dodgeDirection;
    if (state_.isDodging && glm::length(state_.dodgeDirection) <= 1.0e-5F && horizontalSpeed > 1.0e-5F)
        state_.dodgeDirection = glm::vec3(horizontalVelocity.x / horizontalSpeed, 0.0F,
            horizontalVelocity.y / horizontalSpeed);
    state_.externalKnockbackVelocity = glm::vec3(state.externalKnockbackVelocity);
    state_.knockbackRemaining = std::max(0.0F, state.knockbackRemaining);
}

} // namespace glory::game
