#pragma once

#include "glory/game/behavior_tree.hpp"
#include "glory/game/combat.hpp"
#include "glory/game/collision.hpp"
#include "glory/game/status_effect.hpp"

#include <memory>
#include <string>

namespace glory::game {

enum class BossPhase { Phase1, Phase2, Dead };

struct BossConfig {
    float maxHp = 300.0F;
    float chaseSpeed = 3.5F;
    float chargeSpeed = 9.0F;
    float meleeRange = 2.8F;
    glm::vec3 spawnPosition{-3.8F, 0.0F, 0.0F};
    float spawnYaw = 0.0F;
};

struct BossUpdateResult {
    float playerDamage = 0.0F;
    glm::vec3 playerKnockback{0.0F};
    std::string attackName;
    bool playerHit = false;
    bool enrageTriggered = false;
    bool hitStopActive = false;
};

class Boss {
public:
    explicit Boss(BossConfig config = {});
    [[nodiscard]] BossUpdateResult update(float deltaSeconds, const glm::vec3& playerPosition,
        bool playerInvulnerable);
    void setHealth(float health) noexcept;
    void stagger(float duration) noexcept;
    void reset() noexcept;

    [[nodiscard]] const std::string& name() const noexcept { return name_; }
    [[nodiscard]] const glm::vec3& position() const noexcept { return position_; }
    void setPosition(const glm::vec3& position) noexcept;
    void setPosition(const glm::vec3& position, float yawDegrees) noexcept {
        setSpawnTransform(position, yawDegrees);
    }
    void setSpawnTransform(const glm::vec3& position, float yawDegrees) noexcept;
    void setCollisionWorld(CollisionWorld world);
    void resolveWorldDisplacement(const glm::vec3& displacement) noexcept;
    [[nodiscard]] float yawDegrees() const noexcept { return yawDegrees_; }
    [[nodiscard]] float currentHp() const noexcept { return currentHp_; }
    [[nodiscard]] float maxHp() const noexcept { return config_.maxHp; }
    [[nodiscard]] BossPhase phase() const noexcept { return phase_; }
    [[nodiscard]] bool isDead() const noexcept { return phase_ == BossPhase::Dead; }
    [[nodiscard]] const std::string& activeNode() const noexcept { return activeNode_; }
    [[nodiscard]] const Hurtbox& hurtbox() const noexcept { return hurtbox_; }
    [[nodiscard]] const Hitbox& attackHitbox() const noexcept { return attackHitbox_; }
    [[nodiscard]] const StatusEffectManager& statusEffects() const noexcept { return statusEffects_; }
    [[nodiscard]] float hitStopRemaining() const noexcept { return hitStopRemaining_; }

private:
    void buildBehaviorTree();
    void attackPlayer(BossUpdateResult& result, const glm::vec3& playerPosition,
        bool playerInvulnerable, const char* attackName, float damage, float knockback);

    BossConfig config_;
    std::string name_ = "Gorgon, the Iron Warden";
    glm::vec3 position_{0.0F};
    float yawDegrees_ = 0.0F;
    Hurtbox hurtbox_;
    Hitbox attackHitbox_;
    float currentHp_ = 0.0F;
    float slashCooldown_ = 0.0F;
    float chargeCooldown_ = 0.0F;
    float slamCooldown_ = 0.0F;
    float hitStopRemaining_ = 0.0F;
    float activeChargeTime_ = 0.0F;
    bool chargeActive_ = false;
    bool chargeHit_ = false;
    bool enraged_ = false;
    BossPhase phase_ = BossPhase::Phase1;
    std::string activeNode_ = "Idle";
    StatusEffectManager statusEffects_;
    std::unique_ptr<BehaviorNode> behaviorTree_;
    CollisionWorld collisionWorld_ = CollisionWorld::makeSampleWorld();
};

} // namespace glory::game
