#include "glory/game/boss.hpp"

#include <glm/geometric.hpp>

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
        Boss chaseBoss({300.0F, 3.5F, 9.0F, 2.8F, {0.0F, 0.0F, 8.0F}});
        const float initialDistance = glm::distance(chaseBoss.position(), glm::vec3(0.0F));
        const BossUpdateResult chase = chaseBoss.update(0.1F, glm::vec3(0.0F), false);
        require(chaseBoss.activeNode() == "ChasePlayer" &&
            glm::distance(chaseBoss.position(), glm::vec3(0.0F)) < initialDistance,
            "phase one should chase the player when outside melee range");

        Boss slashBoss({300.0F, 3.5F, 9.0F, 2.8F, {0.0F, 0.0F, 2.0F}});
        const BossUpdateResult slash = slashBoss.update(0.016F, glm::vec3(0.0F), false);
        require(slash.attackName == "BossSlash" && std::abs(slash.playerDamage - 20.0F) < 1.0e-5F &&
            glm::length(slash.playerKnockback) > 3.5F && slashBoss.attackHitbox().radius > 0.0F,
            "phase one should perform a damaging slash with a hitbox in range");
        const BossUpdateResult slashCooldown = slashBoss.update(0.1F, glm::vec3(0.0F), false);
        require(slashCooldown.attackName.empty() && slashCooldown.playerDamage == 0.0F,
            "melee attack cooldown should prevent repeated hits");

        Boss dodgeBoss({300.0F, 3.5F, 9.0F, 2.8F, {0.0F, 0.0F, 2.0F}});
        const BossUpdateResult dodged = dodgeBoss.update(0.016F, glm::vec3(0.0F), true);
        require(dodged.attackName == "BossSlash" && dodged.playerDamage == 0.0F &&
            glm::length(dodged.playerKnockback) == 0.0F,
            "player I-frames should avoid boss damage and knockback");

        Boss chargeBoss({300.0F, 3.5F, 9.0F, 2.8F, {0.0F, 0.0F, 8.0F}});
        chargeBoss.setHealth(149.0F);
        const BossUpdateResult enrage = chargeBoss.update(0.01F, glm::vec3(0.0F), false);
        require(enrage.enrageTriggered && chargeBoss.phase() == BossPhase::Phase2 &&
            std::abs(chargeBoss.statusEffects().calculateDamageDealtMultiplier() - 1.3F) < 1.0e-5F &&
            std::abs(chargeBoss.statusEffects().calculateSpeedMultiplier() - 1.25F) < 1.0e-5F &&
            chargeBoss.hitStopRemaining() > 0.19F,
            "crossing half health should enrage once, buff the boss and start hit-stop");
        (void)chargeBoss.update(0.21F, glm::vec3(0.0F), false);
        const BossUpdateResult charge = chargeBoss.update(0.1F, glm::vec3(0.0F), false);
        require(charge.attackName == "BossCharge" && chargeBoss.activeNode() == "BossCharge" &&
            chargeBoss.position().z < 8.0F,
            "enraged boss should unlock and execute its ranged charge");

        Boss slamBoss({300.0F, 3.5F, 9.0F, 2.8F, {0.0F, 0.0F, 2.0F}});
        slamBoss.setHealth(149.0F);
        (void)slamBoss.update(0.01F, glm::vec3(0.0F), false);
        (void)slamBoss.update(0.21F, glm::vec3(0.0F), false);
        const BossUpdateResult slam = slamBoss.update(0.01F, glm::vec3(0.0F), false);
        require(slam.attackName == "BossGroundSlam" &&
            std::abs(slam.playerDamage - 58.5F) < 1.0e-4F,
            "enraged boss should unlock an empowered ground slam at close range");

        slamBoss.setHealth(0.0F);
        const BossUpdateResult dead = slamBoss.update(0.1F, glm::vec3(0.0F), false);
        require(slamBoss.isDead() && slamBoss.activeNode() == "Dead" && dead.attackName.empty(),
            "boss at zero HP should stop its behavior tree");
        slamBoss.reset();
        require(slamBoss.phase() == BossPhase::Phase1 && slamBoss.currentHp() == 300.0F,
            "reset should restore the boss encounter");

        std::cout << "Boss encounter tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
