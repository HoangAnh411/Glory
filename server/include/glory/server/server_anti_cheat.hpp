#pragma once

#include "glory/game/collision.hpp"

#include <glm/vec3.hpp>

#include <cstdint>

namespace glory::server {

enum class AntiCheatViolation : std::uint8_t { Speed, Teleport, Collision, Cooldown };

struct AntiCheatTelemetry {
    float currentSpeed = 0.0F;
    float maximumSpeed = 0.0F;
    std::uint32_t speedViolations = 0;
    std::uint32_t teleportViolations = 0;
    std::uint32_t collisionViolations = 0;
    std::uint32_t cooldownViolations = 0;
    std::uint32_t violationStreak = 0;
    bool kickRequested = false;
};

class ServerAntiCheat {
public:
    static constexpr float defaultTolerance = 0.15F;
    static constexpr std::uint32_t maximumViolationStreak = 8;

    [[nodiscard]] bool validateMovement(const glm::vec3& previousPosition,
        const glm::vec3& nextPosition, float deltaSeconds, float allowedSpeed,
        bool allowDodgeSpeed = false) noexcept;
    [[nodiscard]] bool validateCooldown(float cooldownRemaining) noexcept;
    void recordViolation(AntiCheatViolation violation) noexcept;
    void resetMovementBaseline() noexcept;
    [[nodiscard]] const AntiCheatTelemetry& telemetry() const noexcept { return telemetry_; }

private:
    AntiCheatTelemetry telemetry_;
    glory::game::CollisionWorld collisionWorld_ = glory::game::CollisionWorld::makeSampleWorld();
};

} // namespace glory::server
