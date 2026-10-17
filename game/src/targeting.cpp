#include "glory/game/targeting.hpp"

#include <glm/geometric.hpp>

#include <algorithm>
#include <cmath>

namespace glory::game {
namespace {
constexpr float radiansToDegrees = 57.295779513082320876F;
constexpr float degreesToRadians = 0.01745329251994329577F;

bool finite(const glm::vec3& value) noexcept {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

bool segmentIntersectsAabb(const glm::vec3& origin, const glm::vec3& target, const Aabb& box) noexcept {
    const glm::vec3 direction = target - origin;
    float minimumT = 0.0F;
    float maximumT = 1.0F;
    for (int axis = 0; axis < 3; ++axis) {
        const float start = origin[axis];
        const float delta = direction[axis];
        if (std::abs(delta) < 1.0e-6F) {
            if (start < box.minimum[axis] || start > box.maximum[axis]) return false;
            continue;
        }
        float t0 = (box.minimum[axis] - start) / delta;
        float t1 = (box.maximum[axis] - start) / delta;
        if (t0 > t1) std::swap(t0, t1);
        minimumT = std::max(minimumT, t0);
        maximumT = std::min(maximumT, t1);
        if (minimumT > maximumT) return false;
    }
    return maximumT >= 0.0F && minimumT <= 1.0F;
}

TargetingResult makeResult(const glm::vec3& playerPosition, const glm::vec3& targetPosition) noexcept {
    TargetingResult result;
    result.position = targetPosition;
    const glm::vec3 delta{targetPosition.x - playerPosition.x, 0.0F, targetPosition.z - playerPosition.z};
    result.distance = glm::length(delta);
    result.yawDegrees = horizontalYawDegrees(playerPosition, targetPosition);
    result.valid = result.distance > 1.0e-4F;
    return result;
}
}

float horizontalYawDegrees(const glm::vec3& from, const glm::vec3& to) noexcept {
    const glm::vec3 delta{to.x - from.x, 0.0F, to.z - from.z};
    if (!finite(from) || !finite(to) || glm::length(delta) <= 1.0e-4F) return 0.0F;
    return std::atan2(delta.z, delta.x) * radiansToDegrees;
}

bool hasLineOfSight(const CollisionWorld& world, const glm::vec3& origin, const glm::vec3& target) noexcept {
    if (!finite(origin) || !finite(target)) return false;
    const glm::vec3 start = origin + glm::vec3(0.0F, 1.2F, 0.0F);
    const glm::vec3 end = target + glm::vec3(0.0F, 1.2F, 0.0F);
    for (const Aabb& obstacle : world.obstacles()) {
        if (obstacle.blocksMovement && segmentIntersectsAabb(start, end, obstacle)) return false;
    }
    return true;
}

TargetingResult chooseAttackTarget(const CollisionWorld& world, const glm::vec3& playerPosition,
    float cameraYawDegrees, const glm::vec3& targetPosition, bool targetAlive,
    AttackAimMode mode, float attackRange) noexcept {
    TargetingResult result = makeResult(playerPosition, targetPosition);
    if (!targetAlive || !std::isfinite(cameraYawDegrees) || !std::isfinite(attackRange) ||
        attackRange <= 0.0F || !result.valid || result.distance > attackRange ||
        !hasLineOfSight(world, playerPosition, targetPosition)) {
        result.valid = false;
        return result;
    }
    if (mode == AttackAimMode::Manual) {
        result.valid = false;
        return result;
    }
    if (mode == AttackAimMode::Assist) {
        const float cameraRadians = cameraYawDegrees * degreesToRadians;
        const glm::vec3 cameraForward{std::cos(cameraRadians), 0.0F, std::sin(cameraRadians)};
        const glm::vec3 targetDirection = (targetPosition - playerPosition) / result.distance;
        if (glm::dot(cameraForward, targetDirection) < std::cos(35.0F * degreesToRadians))
            result.valid = false;
    }
    return result;
}

TargetingResult chooseLockTarget(const CollisionWorld& world, const glm::vec3& playerPosition,
    float cameraYawDegrees, const glm::vec3& targetPosition, bool targetAlive,
    float selectionRange, float lockRange) noexcept {
    TargetingResult result = makeResult(playerPosition, targetPosition);
    if (!targetAlive || !std::isfinite(cameraYawDegrees) || !std::isfinite(selectionRange) ||
        !std::isfinite(lockRange) || selectionRange <= 0.0F || lockRange <= 0.0F || !result.valid ||
        result.distance > selectionRange || !hasLineOfSight(world, playerPosition, targetPosition)) {
        result.valid = false;
        return result;
    }
    const float cameraRadians = cameraYawDegrees * degreesToRadians;
    const glm::vec3 cameraForward{std::cos(cameraRadians), 0.0F, std::sin(cameraRadians)};
    const glm::vec3 targetDirection = (targetPosition - playerPosition) / result.distance;
    if (glm::dot(cameraForward, targetDirection) <= 0.0F) result.valid = false;
    return result;
}

} // namespace glory::game
