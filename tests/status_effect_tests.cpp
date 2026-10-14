#include "glory/game/status_effect.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

} // namespace

int main() {
    using namespace glory::game;
    try {
        StatusEffectManager effects;
        float hp = 100.0F;
        effects.apply({"burn", "Burn", StatusEffectKind::Burn, 3.0F, 3.0F, 0.5F, 0.5F, 5.0F, 1, 1});
        effects.update(0.49F, hp, 100.0F);
        require(std::abs(hp - 100.0F) < 1.0e-5F, "DoT must wait for its first tick");
        effects.update(0.01F, hp, 100.0F);
        require(std::abs(hp - 95.0F) < 1.0e-5F, "DoT should apply at each configured interval");

        effects.clear();
        hp = 80.0F;
        effects.apply({"regen", "Regen", StatusEffectKind::Regen, 2.0F, 2.0F, 1.0F, 1.0F, 5.0F, 1, 1});
        effects.update(2.0F, hp, 100.0F);
        require(std::abs(hp - 90.0F) < 1.0e-5F, "HoT should tick twice over two seconds");

        effects.apply({"speed", "Speed Boost", StatusEffectKind::SpeedBoost, 5.0F, 5.0F, 0.0F, 0.0F,
            0.3F, 1, 2});
        effects.apply({"slow", "Slow", StatusEffectKind::Slow, 5.0F, 5.0F, 0.0F, 0.0F, 0.4F, 1, 1});
        effects.apply({"attack", "Attack Boost", StatusEffectKind::AttackBoost, 5.0F, 5.0F, 0.0F, 0.0F,
            0.3F, 1, 1});
        effects.apply({"armor", "Armor Break", StatusEffectKind::ArmorBreak, 5.0F, 5.0F, 0.0F, 0.0F,
            0.25F, 1, 1});
        require(std::abs(effects.calculateSpeedMultiplier() - 0.78F) < 1.0e-5F,
            "speed boost and slow should compose multiplicatively");
        require(std::abs(effects.calculateDamageDealtMultiplier() - 1.3F) < 1.0e-5F &&
            std::abs(effects.calculateDamageTakenMultiplier() - 1.25F) < 1.0e-5F,
            "damage modifiers should match configured percentages");

        effects.apply({"stun", "Stun", StatusEffectKind::Stun, 1.0F, 1.0F, 0.0F, 0.0F, 1.0F, 1, 1});
        effects.update(0.6F, hp, 100.0F);
        require(effects.isStunned(), "stun must remain active until its duration elapses");
        effects.update(0.41F, hp, 100.0F);
        require(!effects.isStunned(), "stun must expire at its duration");

        effects.clear();
        effects.apply({"stack", "Stack", StatusEffectKind::SpeedBoost, 2.0F, 2.0F, 0.0F, 0.0F, 0.2F, 1, 2});
        effects.update(1.0F, hp, 100.0F);
        effects.apply({"stack", "Stack", StatusEffectKind::SpeedBoost, 2.0F, 2.0F, 0.0F, 0.0F, 0.2F, 1, 2});
        require(effects.effects().back().stackCount == 2 &&
            std::abs(effects.effects().back().remainingTime - 2.0F) < 1.0e-5F,
            "reapplication should add a bounded stack and refresh duration");
        effects.update(2.01F, hp, 100.0F);
        require(effects.effects().empty(), "expired effects should be removed automatically");

        std::cout << "Status effect tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
