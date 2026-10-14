#include "glory/game/combat.hpp"

#include <glm/geometric.hpp>

#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <utility>

namespace {

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

glory::game::SkillRegistry loadSkills() {
    return glory::game::SkillRegistry::loadFromFile(
        std::filesystem::path(GLORY_ASSET_DIR) / "data" / "skills.json");
}

} // namespace

int main() {
    using namespace glory::game;
    try {
        SkillRegistry registry = loadSkills();
        require(registry.skills().size() == 11, "JSON should load base, combo, and class skills");
        const SkillDefinition* slash = registry.find("slash");
        const SkillDefinition* heavy = registry.find("heavy_strike");
        const SkillDefinition* combo1 = registry.find("combo_1");
        const SkillDefinition* combo2 = registry.find("combo_2");
        const SkillDefinition* combo3 = registry.find("combo_3");
        require(slash && std::abs(slash->damage - 25.0F) < 1.0e-5F &&
            std::abs(slash->cooldown - 0.5F) < 1.0e-5F, "slash data should match JSON");
        require(heavy && std::abs(heavy->damage - 60.0F) < 1.0e-5F &&
            std::abs(heavy->castTime - 0.35F) < 1.0e-5F, "heavy strike data should match JSON");
        require(heavy->onHitEffects.size() == 1 &&
            heavy->onHitEffects.front().kind == StatusEffectKind::Stun,
            "heavy strike should load its configured stun effect");
        require(combo1 && combo2 && combo3 && combo1->damage < combo2->damage &&
            combo2->damage < combo3->damage, "combo damage should increase by step");
        require(combo3->onHitEffects.size() == 1 &&
            combo3->onHitEffects.front().kind == StatusEffectKind::Bleed,
            "combo finisher should load its configured bleed effect");

        const Hurtbox target{{-1.15F, 0.0F, 0.0F}, 0.4F, 1.8F};
        require(intersects({{-1.15F, 0.85F, 0.0F}, 0.9F}, target), "overlapping sphere and capsule should hit");
        require(!intersects({{3.0F, 0.85F, 0.0F}, 0.3F}, target), "separated sphere and capsule should miss");

        CombatTarget distantTarget{7, {{4.0F, 0.0F, 0.0F}, 0.4F, 1.8F}, 100.0F, 100.0F, 0};
        CombatSystem comboCombat(loadSkills(), distantTarget);
        comboCombat.update(0.05F, glm::vec3(0.0F), -90.0F, true, "Attack");
        require(comboCombat.comboStep() == 1, "primary attack should start combo step one");
        comboCombat.update(0.05F, glm::vec3(0.0F), -90.0F, true, "Attack");
        comboCombat.update(0.04F, glm::vec3(0.0F), -90.0F, false, "Attack");
        require(comboCombat.comboStep() == 2 && comboCombat.attackAnimationRequested(),
            "buffered input should chain to combo step two when the window opens");
        comboCombat.update(0.05F, glm::vec3(0.0F), -90.0F, true, "Attack");
        comboCombat.update(0.10F, glm::vec3(0.0F), -90.0F, false, "Attack");
        require(comboCombat.comboStep() == 3, "buffered input should chain to combo step three");

        CombatSystem timeoutCombat(loadSkills(), distantTarget);
        timeoutCombat.update(0.05F, glm::vec3(0.0F), -90.0F, true, "Attack");
        timeoutCombat.update(0.40F, glm::vec3(0.0F), -90.0F, false, "Attack");
        require(timeoutCombat.comboStep() == 1 && !timeoutCombat.comboWindowOpen(),
            "combo should reset to step one after its window expires");

        CombatSystem expiredBufferCombat(loadSkills(), distantTarget);
        expiredBufferCombat.update(0.05F, glm::vec3(0.0F), -90.0F, true, "Attack");
        expiredBufferCombat.update(0.01F, glm::vec3(0.0F), -90.0F, true, "Attack");
        expiredBufferCombat.update(0.20F, glm::vec3(0.0F), -90.0F, false, "Attack");
        require(expiredBufferCombat.comboStep() == 1 && expiredBufferCombat.comboWindowOpen(),
            "expired input buffer must not advance the combo when the window opens");

        CombatSystem cancelCombat(loadSkills(), distantTarget);
        cancelCombat.update(0.05F, glm::vec3(0.0F), -90.0F, true, "Attack");
        require(!cancelCombat.dodgeAllowed(), "dodge must be locked during the active attack phase");
        cancelCombat.update(0.05F, glm::vec3(0.0F), -90.0F, false, "Attack", true, true);
        require(!cancelCombat.attackCanceled(), "dodge cannot cancel before active frames finish");
        cancelCombat.update(0.13F, glm::vec3(0.0F), -90.0F, false, "Attack");
        require(cancelCombat.cancelWindowAvailable() && cancelCombat.dodgeAllowed(),
            "dodge cancel should become available during recovery");
        cancelCombat.update(0.01F, glm::vec3(0.0F), -90.0F, false, "Attack", true, true);
        require(cancelCombat.attackCanceled() && cancelCombat.comboStep() == 1,
            "successful dodge cancel should end the attack and reset the chain");

        CombatSystem impactCombat(loadSkills(), {7, target, 100.0F, 100.0F, 0});
        impactCombat.update(0.05F, glm::vec3(0.0F), -90.0F, true, "Attack");
        require(impactCombat.hitEvents().size() == 1 && impactCombat.dummy().hitCount == 1,
            "combo strike should damage the dummy once");
        require(std::abs(impactCombat.dummy().currentHp - 80.0F) < 1.0e-5F,
            "combo one should apply configured damage");
        require(impactCombat.isHitStopped() &&
            std::abs(impactCombat.hitStopRemaining() - 0.06F) < 1.0e-5F,
            "light hit should start configured hit-stop");
        require(impactCombat.dummy().knockbackVelocity.x < -3.4F &&
            std::abs(impactCombat.dummy().knockbackVelocity.z) < 1.0e-4F,
            "knockback should launch away from the attacker");
        const float frozenElapsed = impactCombat.activeSkillElapsed();
        const glm::vec3 frozenPosition = impactCombat.dummy().hurtbox.base;
        impactCombat.update(0.02F, glm::vec3(0.0F), -90.0F, false, "Attack");
        impactCombat.update(0.02F, glm::vec3(0.0F), -90.0F, false, "Attack");
        impactCombat.update(0.02F, glm::vec3(0.0F), -90.0F, false, "Attack");
        require(!impactCombat.isHitStopped() &&
            std::abs(impactCombat.activeSkillElapsed() - frozenElapsed) < 1.0e-5F &&
            glm::length(impactCombat.dummy().hurtbox.base - frozenPosition) < 1.0e-5F,
            "hit-stop should freeze attack time and target movement for its duration");
        impactCombat.update(0.125F, glm::vec3(0.0F), -90.0F, false, "Attack");
        require(impactCombat.dummy().hurtbox.base.x < frozenPosition.x &&
            impactCombat.dummy().knockbackVelocity.x < 0.0F &&
            std::abs(glm::length(impactCombat.dummy().knockbackVelocity) - 1.75F) < 0.05F,
            "knockback should move in the correct direction and decelerate linearly");
        impactCombat.update(0.2F, glm::vec3(0.0F), -90.0F, false, "Attack");
        require(glm::length(impactCombat.dummy().knockbackVelocity) < 1.0e-5F,
            "knockback should stop after its configured duration");
        require(impactCombat.dummy().hurtbox.base.y == 0.0F,
            "collision resolution should keep the target grounded");

        CombatSystem effectCombat(loadSkills(), {7, target, 100.0F, 100.0F, 0});
        effectCombat.requestSkill("combo_3");
        effectCombat.update(0.05F, glm::vec3(0.0F), -90.0F, false, "Attack", false, false, 1.5F);
        require(std::abs(effectCombat.dummy().currentHp - 25.0F) < 1.0e-5F &&
            effectCombat.dummy().statusEffects.effects().size() == 1 &&
            effectCombat.dummy().statusEffects.effects().front().kind == StatusEffectKind::Bleed,
            "confirmed hit should scale damage and apply skill status effects");

        CollisionWorld wallWorld;
        wallWorld.addObstacle({{-2.2F, 0.0F, -1.0F}, {-1.7F, 2.0F, 1.0F}, false});
        CombatSystem wallCombat(loadSkills(), {7, target, 100.0F, 100.0F, 0}, wallWorld);
        wallCombat.update(0.05F, glm::vec3(0.0F), -90.0F, true, "Attack");
        wallCombat.update(0.06F, glm::vec3(0.0F), -90.0F, false, "Attack");
        wallCombat.update(0.2F, glm::vec3(0.0F), -90.0F, false, "Attack");
        require(wallCombat.dummy().hurtbox.base.x > -1.7F,
            "collision world should prevent knockback from passing through a wall");

        std::cout << "Combo, cancel, hit-stop, and knockback tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

