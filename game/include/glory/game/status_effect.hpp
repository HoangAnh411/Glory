#pragma once

#include <string>
#include <vector>

namespace glory::game {

enum class StatusEffectKind { AttackBoost, SpeedBoost, Regen, Bleed, Burn, Slow, Stun, ArmorBreak };

struct StatusEffect {
    std::string id;
    std::string name;
    StatusEffectKind kind = StatusEffectKind::AttackBoost;
    float duration = 0.0F;
    float remainingTime = 0.0F;
    float tickInterval = 0.0F;
    float tickTimer = 0.0F;
    float magnitude = 0.0F;
    unsigned int stackCount = 1;
    unsigned int maxStacks = 1;
};

class StatusEffectManager {
public:
    void apply(StatusEffect effect);
    void update(float deltaSeconds, float& health, float maximumHealth);
    void clear() noexcept;

    [[nodiscard]] float calculateSpeedMultiplier() const noexcept;
    [[nodiscard]] float calculateDamageDealtMultiplier() const noexcept;
    [[nodiscard]] float calculateDamageTakenMultiplier() const noexcept;
    [[nodiscard]] bool isStunned() const noexcept;
    [[nodiscard]] const std::vector<StatusEffect>& effects() const noexcept { return effects_; }

private:
    std::vector<StatusEffect> effects_;
};

} // namespace glory::game
