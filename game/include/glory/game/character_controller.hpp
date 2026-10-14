#pragma once

#include "glory/game/action_map.hpp"
#include "glory/game/collision.hpp"

#include <glm/vec3.hpp>

namespace glory::game {

struct CharacterControllerConfig {
    float moveSpeed = 5.5F;
    float jumpVelocity = 7.0F;
    float gravity = 18.0F;
    float rotationSpeedDegrees = 540.0F;
    float dodgeDuration = 0.35F;
    float dodgeSpeed = 12.0F;
    float dodgeCooldown = 0.8F;
    float invulnerabilityStart = 0.05F;
    float invulnerabilityEnd = 0.28F;
    float capsuleRadius = 0.4F;
    float capsuleHeight = 1.8F;
    float maximumStepHeight = 0.25F;
};

struct CharacterControllerState {
    glm::vec3 position{0.0F};
    glm::vec3 velocity{0.0F};
    float facingYaw = -90.0F;
    bool isGrounded = true;
    bool attackRequested = false;
    bool isDodging = false;
    bool isInvulnerable = false;
    float dodgeElapsed = 0.0F;
    float dodgeCooldownRemaining = 0.0F;
    glm::vec3 dodgeDirection{0.0F};
    glm::vec3 externalKnockbackVelocity{0.0F};
    float knockbackRemaining = 0.0F;
    unsigned int collisionCount = 0;
};

class CharacterController {
public:
    explicit CharacterController(const CharacterControllerConfig& config = {});
    void update(const ActionMap& actions, float cameraYawDegrees, double fixedDeltaSeconds,
        bool allowDodge = true, float speedMultiplier = 1.0F, bool stunned = false) noexcept;
    void applyKnockback(const glm::vec3& velocity) noexcept;
    [[nodiscard]] bool setClassMovement(float moveSpeed, float dodgeCooldown) noexcept;
    void teleport(const glm::vec3& position, float yawDegrees) noexcept;
    void restoreNetworkState(const CharacterControllerState& state) noexcept;
    void reset() noexcept;
    [[nodiscard]] const CharacterControllerState& state() const noexcept { return state_; }

private:
    CharacterControllerConfig config_;
    CharacterControllerState state_;
    CollisionWorld collisionWorld_ = CollisionWorld::makeSampleWorld();
};

} // namespace glory::game
