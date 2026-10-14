#include "glory/game/status_effect.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace glory::game {

void StatusEffectManager::apply(StatusEffect effect) {
    if (effect.id.empty() || effect.name.empty() || !std::isfinite(effect.duration) || effect.duration <= 0.0F ||
        !std::isfinite(effect.magnitude) || effect.magnitude < 0.0F ||
        !std::isfinite(effect.tickInterval) || effect.tickInterval < 0.0F ||
        (effect.tickInterval > 0.0F && (effect.kind == StatusEffectKind::Stun ||
            effect.kind == StatusEffectKind::Slow || effect.kind == StatusEffectKind::SpeedBoost ||
            effect.kind == StatusEffectKind::AttackBoost || effect.kind == StatusEffectKind::ArmorBreak)) ||
        ((effect.kind == StatusEffectKind::Regen || effect.kind == StatusEffectKind::Bleed ||
            effect.kind == StatusEffectKind::Burn) && effect.tickInterval <= 0.0F)) {
        throw std::invalid_argument("Invalid status effect definition");
    }
    effect.remainingTime = effect.duration;
    effect.tickTimer = effect.tickInterval;
    effect.stackCount = std::clamp(effect.stackCount, 1U, std::max(1U, effect.maxStacks));
    effect.maxStacks = std::max(1U, effect.maxStacks);
    const auto existing = std::find_if(effects_.begin(), effects_.end(), [&](const StatusEffect& active) {
        return active.id == effect.id;
    });
    if (existing == effects_.end()) {
        effects_.push_back(std::move(effect));
    } else {
        existing->remainingTime = effect.duration;
        existing->duration = effect.duration;
        existing->stackCount = std::min(existing->maxStacks, existing->stackCount + 1U);
        existing->magnitude = effect.magnitude;
        existing->tickInterval = effect.tickInterval;
        existing->tickTimer = effect.tickInterval;
    }
}

void StatusEffectManager::update(float deltaSeconds, float& health, float maximumHealth) {
    if (!std::isfinite(deltaSeconds) || deltaSeconds <= 0.0F || !std::isfinite(health) ||
        !std::isfinite(maximumHealth) || maximumHealth < 0.0F) return;
    for (StatusEffect& effect : effects_) {
        float elapsed = std::min(deltaSeconds, effect.remainingTime);
        effect.remainingTime -= elapsed;
        if (effect.tickInterval <= 0.0F) continue;
        effect.tickTimer -= elapsed;
        while (effect.tickTimer <= 1.0e-6F && elapsed > 0.0F) {
            const float amount = effect.magnitude * static_cast<float>(effect.stackCount);
            if (effect.kind == StatusEffectKind::Regen) health = std::min(maximumHealth, health + amount);
            else health = std::max(0.0F, health - amount);
            effect.tickTimer += effect.tickInterval;
        }
    }
    std::erase_if(effects_, [](const StatusEffect& effect) { return effect.remainingTime <= 1.0e-6F; });
}

void StatusEffectManager::clear() noexcept { effects_.clear(); }

float StatusEffectManager::calculateSpeedMultiplier() const noexcept {
    float multiplier = 1.0F;
    for (const StatusEffect& effect : effects_) {
        if (effect.kind == StatusEffectKind::SpeedBoost) multiplier *= 1.0F + effect.magnitude * effect.stackCount;
        if (effect.kind == StatusEffectKind::Slow) multiplier *= std::max(0.0F, 1.0F - effect.magnitude * effect.stackCount);
    }
    return std::clamp(multiplier, 0.0F, 4.0F);
}

float StatusEffectManager::calculateDamageDealtMultiplier() const noexcept {
    float multiplier = 1.0F;
    for (const StatusEffect& effect : effects_)
        if (effect.kind == StatusEffectKind::AttackBoost) multiplier *= 1.0F + effect.magnitude * effect.stackCount;
    return std::clamp(multiplier, 0.0F, 10.0F);
}

float StatusEffectManager::calculateDamageTakenMultiplier() const noexcept {
    float multiplier = 1.0F;
    for (const StatusEffect& effect : effects_)
        if (effect.kind == StatusEffectKind::ArmorBreak) multiplier *= 1.0F + effect.magnitude * effect.stackCount;
    return std::clamp(multiplier, 0.0F, 10.0F);
}

bool StatusEffectManager::isStunned() const noexcept {
    return std::any_of(effects_.begin(), effects_.end(), [](const StatusEffect& effect) {
        return effect.kind == StatusEffectKind::Stun && effect.remainingTime > 0.0F;
    });
}

} // namespace glory::game
