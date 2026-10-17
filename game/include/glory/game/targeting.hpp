#pragma once

#include "glory/game/collision.hpp"

#include <glm/vec3.hpp>

namespace glory::game {

enum class AttackAimMode { Manual = 0, Assist = 1, Auto = 2 };

struct TargetingResult {
    bool valid = false;
    glm::vec3 position{0.0F};
    float distance = 0.0F;
    float yawDegrees = 0.0F;
};

[[nodiscard]] float horizontalYawDegrees(const glm::vec3& from, const glm::vec3& to) noexcept;
[[nodiscard]] bool hasLineOfSight(const CollisionWorld& world, const glm::vec3& origin,
    const glm::vec3& target) noexcept;
[[nodiscard]] TargetingResult chooseAttackTarget(const CollisionWorld& world,
    const glm::vec3& playerPosition, float cameraYawDegrees, const glm::vec3& targetPosition,
    bool targetAlive, AttackAimMode mode, float attackRange) noexcept;
[[nodiscard]] TargetingResult chooseLockTarget(const CollisionWorld& world,
    const glm::vec3& playerPosition, float cameraYawDegrees, const glm::vec3& targetPosition,
    bool targetAlive, float selectionRange, float lockRange) noexcept;

} // namespace glory::game
