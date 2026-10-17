#pragma once

#include "glory/game/collision.hpp"
#include "glory/game/status_effect.hpp"

#include <glm/vec3.hpp>

#include <filesystem>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace glory::game {

struct SkillDefinition {
    std::string id;
    std::string name;
    float damage = 0.0F;
    float cooldown = 0.0F;
    float castTime = 0.0F;
    std::string animationState;
    float hitboxRadius = 0.0F;
    glm::vec3 hitboxOffset{0.0F};
    float activeStart = 0.0F;
    float activeEnd = 0.0F;
    std::string comboNext;
    float comboWindow = 0.3F;
    float inputBuffer = 0.15F;
    float cancelWindowStart = 0.3F;
    float hitStopDuration = 0.06F;
    float knockbackSpeed = 3.5F;
    float knockbackDuration = 0.25F;
    std::vector<StatusEffect> onHitEffects;
    struct AnimationEvent {
        float time = 0.0F;
        std::string type;
        std::string name;
    };
    std::vector<AnimationEvent> animationEvents;
};

class SkillRegistry {
public:
    static SkillRegistry loadFromFile(const std::filesystem::path& path);
    static SkillRegistry fromSkills(std::vector<SkillDefinition> skills);
    void saveToFile(const std::filesystem::path& path) const;
    [[nodiscard]] const SkillDefinition* find(const std::string& id) const noexcept;
    [[nodiscard]] const std::vector<SkillDefinition>& skills() const noexcept { return skills_; }

private:
    std::vector<SkillDefinition> skills_;
};

struct Hurtbox {
    glm::vec3 base{0.0F};
    float radius = 0.4F;
    float height = 1.8F;
};

struct Hitbox {
    glm::vec3 center{0.0F};
    float radius = 0.0F;
};

struct CombatTarget {
    unsigned int entityId = 0;
    Hurtbox hurtbox;
    float currentHp = 100.0F;
    float maxHp = 100.0F;
    unsigned int hitCount = 0;
    glm::vec3 knockbackVelocity{0.0F};
    StatusEffectManager statusEffects;
    float defense = 0.0F;
};

struct CombatHitEvent {
    unsigned int targetId = 0;
    glm::vec3 position{0.0F};
    float damage = 0.0F;
};

struct SkillStatus {
    std::string id;
    std::string name;
    float cooldownRemaining = 0.0F;
    float cooldownDuration = 0.0F;
};

[[nodiscard]] bool intersects(const Hitbox& hitbox, const Hurtbox& hurtbox) noexcept;

class CombatSystem {
public:
    explicit CombatSystem(SkillRegistry registry, CombatTarget dummy = {},
        CollisionWorld collisionWorld = CollisionWorld::makeSampleWorld());

    void update(float deltaSeconds, const glm::vec3& ownerPosition, float facingYawDegrees,
        bool attackPressed, const std::string& animationState, bool dodgePressed = false,
        bool ownerDodging = false, float damageDealtMultiplier = 1.0F, bool ownerStunned = false,
        const Hurtbox* hitTestHurtbox = nullptr);
    void requestSkill(const std::string& skillId);
    void replaceRegistry(SkillRegistry registry);
    void resetDummy() noexcept;
    void resetEncounter() noexcept;

    [[nodiscard]] const std::vector<CombatHitEvent>& hitEvents() const noexcept { return hitEvents_; }
    [[nodiscard]] CombatTarget& dummy() noexcept { return dummy_; }
    [[nodiscard]] const CombatTarget& dummy() const noexcept { return dummy_; }
    [[nodiscard]] std::vector<SkillStatus> skillStatuses() const;
    [[nodiscard]] float cooldownRemaining(const std::string& skillId) const noexcept;
    [[nodiscard]] const std::string& animationState() const noexcept { return animationState_; }
    [[nodiscard]] const std::string& activeSkillId() const noexcept { return activeSkillId_; }
    [[nodiscard]] float activeSkillElapsed() const noexcept { return activeSkillElapsed_; }
    [[nodiscard]] unsigned int comboStep() const noexcept { return comboStep_; }
    [[nodiscard]] bool comboWindowOpen() const noexcept { return comboWindowOpen_; }
    [[nodiscard]] bool cancelWindowAvailable() const noexcept { return cancelWindowAvailable_; }
    [[nodiscard]] bool dodgeAllowed() const noexcept { return !attacking_ || cancelWindowAvailable_; }
    [[nodiscard]] bool isHitStopped() const noexcept { return hitStopRemaining_ > 0.0F; }
    [[nodiscard]] float hitStopRemaining() const noexcept { return hitStopRemaining_; }
    [[nodiscard]] bool attackAnimationRequested() const noexcept { return attackAnimationRequested_; }
    [[nodiscard]] bool attackCanceled() const noexcept { return attackCanceled_; }
    [[nodiscard]] float maximumAttackRange() const noexcept;

private:
    [[nodiscard]] bool startSkill(const std::string& skillId, bool comboTransition);
    void updateDummy(float deltaSeconds) noexcept;

    SkillRegistry registry_;
    CombatTarget dummy_;
    CollisionWorld collisionWorld_;
    std::unordered_map<std::string, float> cooldowns_;
    std::unordered_set<unsigned int> hitTargets_;
    std::vector<CombatHitEvent> hitEvents_;
    std::string activeSkillId_;
    std::string pendingSkillId_;
    std::string animationState_ = "Idle";
    float castElapsed_ = 0.0F;
    float activeSkillElapsed_ = 0.0F;
    float comboBufferRemaining_ = 0.0F;
    float hitStopRemaining_ = 0.0F;
    float knockbackRemaining_ = 0.0F;
    unsigned int comboStep_ = 1;
    bool casting_ = false;
    bool attacking_ = false;
    bool comboWindowOpen_ = false;
    bool cancelWindowAvailable_ = false;
    bool attackAnimationRequested_ = false;
    bool attackCanceled_ = false;
};

} // namespace glory::game
