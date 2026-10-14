#include "glory/game/boss.hpp"

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace glory::game {
namespace {

std::unique_ptr<Sequence> makeSequence(std::string name,
    std::unique_ptr<BehaviorNode> condition, std::unique_ptr<BehaviorNode> action) {
    auto sequence = std::make_unique<Sequence>(std::move(name));
    sequence->addChild(std::move(condition));
    sequence->addChild(std::move(action));
    return sequence;
}

glm::vec3 horizontalDirection(const glm::vec3& from, const glm::vec3& to) noexcept {
    glm::vec3 direction{to.x - from.x, 0.0F, to.z - from.z};
    const float length = glm::length(direction);
    return length > 1.0e-5F ? direction / length : glm::vec3{0.0F, 0.0F, 1.0F};
}

} // namespace

Boss::Boss(BossConfig config) : config_(config), position_(config.spawnPosition),
    yawDegrees_(config.spawnYaw), currentHp_(config.maxHp) {
    if (!std::isfinite(config_.maxHp) || config_.maxHp <= 0.0F ||
        !std::isfinite(config_.chaseSpeed) || config_.chaseSpeed < 0.0F ||
        !std::isfinite(config_.chargeSpeed) || config_.chargeSpeed < 0.0F ||
        !std::isfinite(config_.meleeRange) || config_.meleeRange <= 0.0F ||
        !std::isfinite(config_.spawnPosition.x) || !std::isfinite(config_.spawnPosition.y) ||
        !std::isfinite(config_.spawnPosition.z) || !std::isfinite(config_.spawnYaw)) {
        throw std::invalid_argument("Invalid boss configuration");
    }
    hurtbox_ = {position_, 0.55F, 2.2F};
    buildBehaviorTree();
}

void Boss::buildBehaviorTree() {
    auto root = std::make_unique<Selector>("BossRoot");
    root->addChild(makeSequence("ChargeBranch",
        std::make_unique<ConditionNode>("CanCharge", [](const AiContext& context) {
            return context.phase == 2 && (context.chargeActive ||
                (context.distanceToPlayer > 5.0F && context.chargeCooldown <= 0.0F));
        }),
        std::make_unique<ActionNode>("BossCharge", [](AiContext& context) {
            context.command = "BossCharge";
            return NodeStatus::Running;
        })));
    root->addChild(makeSequence("SlamBranch",
        std::make_unique<ConditionNode>("CanSlam", [](const AiContext& context) {
            return context.phase == 2 && context.distanceToPlayer <= 3.2F && context.slamCooldown <= 0.0F;
        }),
        std::make_unique<ActionNode>("BossGroundSlam", [](AiContext& context) {
            context.command = "BossGroundSlam";
            return NodeStatus::Success;
        })));
    root->addChild(makeSequence("SlashBranch",
        std::make_unique<ConditionNode>("CanSlash", [](const AiContext& context) {
            return context.phase == 1 && context.distanceToPlayer <= 2.8F && context.slashCooldown <= 0.0F;
        }),
        std::make_unique<ActionNode>("BossSlash", [](AiContext& context) {
            context.command = "BossSlash";
            return NodeStatus::Success;
        })));
    root->addChild(std::make_unique<ActionNode>("ChasePlayer", [](AiContext& context) {
        context.command = "ChasePlayer";
        return NodeStatus::Running;
    }));
    behaviorTree_ = std::move(root);
}

void Boss::attackPlayer(BossUpdateResult& result, const glm::vec3& playerPosition,
    bool playerInvulnerable, const char* attackName, float damage, float knockback) {
    result.attackName = attackName;
    const glm::vec3 direction = horizontalDirection(position_, playerPosition);
    const glm::vec3 center = std::string(attackName) == "BossGroundSlam" ? position_ :
        position_ + direction * (std::string(attackName) == "BossSlash" ? 1.4F : 0.75F);
    const float radius = std::string(attackName) == "BossGroundSlam" ? 3.0F :
        (std::string(attackName) == "BossSlash" ? 1.5F : 0.75F);
    attackHitbox_ = {center, radius};
    const Hurtbox playerHurtbox{playerPosition, 0.4F, 1.8F};
    if (!playerInvulnerable && intersects(attackHitbox_, playerHurtbox)) {
        result.playerHit = true;
        result.playerDamage = damage * statusEffects_.calculateDamageDealtMultiplier();
        result.playerKnockback = direction * knockback + glm::vec3(0.0F, 1.0F, 0.0F);
    }
}

BossUpdateResult Boss::update(float deltaSeconds, const glm::vec3& playerPosition, bool playerInvulnerable) {
    BossUpdateResult result;
    if (isDead() || !std::isfinite(deltaSeconds) || deltaSeconds <= 0.0F ||
        !std::isfinite(playerPosition.x) || !std::isfinite(playerPosition.y) || !std::isfinite(playerPosition.z)) {
        result.hitStopActive = hitStopRemaining_ > 0.0F;
        return result;
    }
    attackHitbox_ = {};

    slashCooldown_ = std::max(0.0F, slashCooldown_ - deltaSeconds);
    chargeCooldown_ = std::max(0.0F, chargeCooldown_ - deltaSeconds);
    slamCooldown_ = std::max(0.0F, slamCooldown_ - deltaSeconds);
    float health = currentHp_;
    statusEffects_.update(deltaSeconds, health, config_.maxHp);
    currentHp_ = health;
    if (!enraged_ && currentHp_ < config_.maxHp * 0.5F) {
        enraged_ = true;
        phase_ = BossPhase::Phase2;
        statusEffects_.apply({"boss_attack_boost", "Enrage: Attack Boost", StatusEffectKind::AttackBoost,
            60.0F, 60.0F, 0.0F, 0.0F, 0.3F, 1, 1});
        statusEffects_.apply({"boss_speed_boost", "Enrage: Speed Boost", StatusEffectKind::SpeedBoost,
            60.0F, 60.0F, 0.0F, 0.0F, 0.25F, 1, 1});
        hitStopRemaining_ = 0.2F;
        activeNode_ = "Enrage";
        result.enrageTriggered = true;
        result.hitStopActive = true;
        return result;
    }
    if (hitStopRemaining_ > 0.0F) {
        hitStopRemaining_ = std::max(0.0F, hitStopRemaining_ - deltaSeconds);
        activeNode_ = "HitStop";
        result.hitStopActive = hitStopRemaining_ > 0.0F;
        return result;
    }

    AiContext context;
    context.bossPosition = position_;
    context.playerPosition = playerPosition;
    context.distanceToPlayer = glm::distance(position_, playerPosition);
    context.bossHp = currentHp_;
    context.bossMaxHp = config_.maxHp;
    context.deltaSeconds = deltaSeconds;
    context.slashCooldown = slashCooldown_;
    context.chargeCooldown = chargeCooldown_;
    context.slamCooldown = slamCooldown_;
    context.phase = phase_ == BossPhase::Phase2 ? 2U : 1U;
    context.chargeActive = chargeActive_;
    (void)behaviorTree_->tick(context);
    activeNode_ = context.activeNode.empty() ? "Idle" : context.activeNode;

    if (context.command == "ChasePlayer") {
        const glm::vec3 direction = horizontalDirection(position_, playerPosition);
        const float stopDistance = phase_ == BossPhase::Phase1 ? 2.4F : 2.9F;
        const float distance = glm::distance(glm::vec2(position_.x, position_.z),
            glm::vec2(playerPosition.x, playerPosition.z));
        const float travel = std::max(0.0F, distance - stopDistance);
        const float speed = config_.chaseSpeed * statusEffects_.calculateSpeedMultiplier();
        const glm::vec3 desired = position_ + direction * std::min(travel, speed * deltaSeconds);
        const CollisionResult collision = collisionWorld_.resolve(
            {position_, hurtbox_.radius, hurtbox_.height}, desired, direction * speed, 0.25F);
        position_ = collision.position;
        hurtbox_.base = position_;
    } else if (context.command == "BossSlash") {
        slashCooldown_ = phase_ == BossPhase::Phase1 ? 1.35F : 0.9F;
        attackPlayer(result, playerPosition, playerInvulnerable, "BossSlash", 20.0F, 3.5F);
    } else if (context.command == "BossGroundSlam") {
        slamCooldown_ = 2.2F;
        if (glm::distance(position_, playerPosition) <= 3.0F)
            attackPlayer(result, playerPosition, playerInvulnerable, "BossGroundSlam", 45.0F, 6.0F);
        else result.attackName = "BossGroundSlam";
    } else if (context.command == "BossCharge") {
        if (!chargeActive_) {
            chargeActive_ = true;
            chargeHit_ = false;
            activeChargeTime_ = 0.0F;
            chargeCooldown_ = 3.2F;
        }
        activeChargeTime_ += deltaSeconds;
        const glm::vec3 direction = horizontalDirection(position_, playerPosition);
        const float distance = glm::distance(glm::vec2(position_.x, position_.z),
            glm::vec2(playerPosition.x, playerPosition.z));
        const glm::vec3 desired = position_ + direction * std::min(distance, config_.chargeSpeed * deltaSeconds);
        const CollisionResult collision = collisionWorld_.resolve(
            {position_, hurtbox_.radius, hurtbox_.height}, desired, direction * config_.chargeSpeed, 0.25F);
        position_ = collision.position;
        hurtbox_.base = position_;
        result.attackName = "BossCharge";
        const float remainingDistance = glm::distance(position_, playerPosition);
        if (!chargeHit_ && remainingDistance <= 1.5F) {
            chargeHit_ = true;
            attackPlayer(result, playerPosition, playerInvulnerable, "BossCharge", 35.0F, 8.0F);
        }
        if (chargeHit_ || activeChargeTime_ >= 2.0F || remainingDistance <= 1.0F) {
            chargeActive_ = false;
        }
    }
    result.hitStopActive = hitStopRemaining_ > 0.0F;
    return result;
}

void Boss::setHealth(float health) noexcept {
    if (!std::isfinite(health)) return;
    currentHp_ = std::clamp(health, 0.0F, config_.maxHp);
    if (currentHp_ <= 0.0F) {
        phase_ = BossPhase::Dead;
        activeNode_ = "Dead";
        chargeActive_ = false;
        attackHitbox_ = {};
    }
}

void Boss::setSpawnTransform(const glm::vec3& position, float yawDegrees) noexcept {
    if (!std::isfinite(position.x) || !std::isfinite(position.y) || !std::isfinite(position.z) ||
        !std::isfinite(yawDegrees)) return;
    config_.spawnPosition = position;
    config_.spawnYaw = yawDegrees;
    position_ = position;
    yawDegrees_ = yawDegrees;
    hurtbox_.base = position;
}

void Boss::stagger(float duration) noexcept {
    if (std::isfinite(duration) && duration > 0.0F && !isDead())
        hitStopRemaining_ = std::max(hitStopRemaining_, duration);
}

void Boss::reset() noexcept {
    currentHp_ = config_.maxHp;
    phase_ = BossPhase::Phase1;
    position_ = config_.spawnPosition;
    yawDegrees_ = config_.spawnYaw;
    hurtbox_.base = position_;
    slashCooldown_ = 0.0F;
    chargeCooldown_ = 0.0F;
    slamCooldown_ = 0.0F;
    hitStopRemaining_ = 0.0F;
    activeChargeTime_ = 0.0F;
    chargeActive_ = false;
    attackHitbox_ = {};
    chargeHit_ = false;
    enraged_ = false;
    activeNode_ = "Idle";
    statusEffects_.clear();
}

} // namespace glory::game
