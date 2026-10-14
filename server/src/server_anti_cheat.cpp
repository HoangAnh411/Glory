#include "glory/server/server_anti_cheat.hpp"

#include "glory/game/collision.hpp"

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>

#include <algorithm>
#include <cmath>

namespace glory::server {
namespace {

constexpr float teleportDistance = 8.0F;
constexpr float collisionEpsilon = 0.05F;

} // namespace

bool ServerAntiCheat::validateMovement(const glm::vec3& previousPosition,
    const glm::vec3& nextPosition, float deltaSeconds, float allowedSpeed,
    bool allowDodgeSpeed) noexcept {
    if (!std::isfinite(previousPosition.x) || !std::isfinite(previousPosition.y) ||
        !std::isfinite(previousPosition.z) || !std::isfinite(nextPosition.x) ||
        !std::isfinite(nextPosition.y) || !std::isfinite(nextPosition.z) ||
        !std::isfinite(deltaSeconds) || deltaSeconds <= 0.0F ||
        !std::isfinite(allowedSpeed) || allowedSpeed <= 0.0F) {
        recordViolation(AntiCheatViolation::Teleport);
        return false;
    }

    const glm::vec3 displacement = nextPosition - previousPosition;
    const float distance = glm::length(displacement);
    if (!std::isfinite(distance) || distance > teleportDistance) {
        recordViolation(AntiCheatViolation::Teleport);
        return false;
    }

    const float horizontalDistance = glm::length(glm::vec2(displacement.x, displacement.z));
    telemetry_.currentSpeed = horizontalDistance / deltaSeconds;
    telemetry_.maximumSpeed = allowedSpeed * (1.0F + defaultTolerance);
    if (allowDodgeSpeed) telemetry_.maximumSpeed = std::max(telemetry_.maximumSpeed, 12.0F * (1.0F + defaultTolerance));
    const float verticalSpeed = std::abs(displacement.y) / deltaSeconds;
    const float maximumVerticalSpeed = std::max(7.0F, allowedSpeed) * (1.0F + defaultTolerance);
    if (telemetry_.currentSpeed > telemetry_.maximumSpeed || verticalSpeed > maximumVerticalSpeed) {
        recordViolation(AntiCheatViolation::Speed);
        return false;
    }

    const auto resolved = collisionWorld_.resolve({previousPosition, 0.4F, 1.8F}, nextPosition,
        displacement / deltaSeconds);
    if (glm::distance(resolved.position, nextPosition) > collisionEpsilon) {
        recordViolation(AntiCheatViolation::Collision);
        return false;
    }

    return true;
}

bool ServerAntiCheat::validateCooldown(float cooldownRemaining) noexcept {
    if (!std::isfinite(cooldownRemaining) || cooldownRemaining < 0.0F || cooldownRemaining > 0.0F) {
        recordViolation(AntiCheatViolation::Cooldown);
        return false;
    }
    return true;
}

void ServerAntiCheat::recordViolation(AntiCheatViolation violation) noexcept {
    switch (violation) {
    case AntiCheatViolation::Speed: ++telemetry_.speedViolations; break;
    case AntiCheatViolation::Teleport: ++telemetry_.teleportViolations; break;
    case AntiCheatViolation::Collision: ++telemetry_.collisionViolations; break;
    case AntiCheatViolation::Cooldown: ++telemetry_.cooldownViolations; break;
    }
    ++telemetry_.violationStreak;
    telemetry_.kickRequested = telemetry_.violationStreak >= maximumViolationStreak;
}

void ServerAntiCheat::resetMovementBaseline() noexcept {
    telemetry_.currentSpeed = 0.0F;
}

} // namespace glory::server
