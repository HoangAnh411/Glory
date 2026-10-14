#include "glory/game/world_history.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

glory::game::SkillRegistry makeTestSkills() {
    glory::game::SkillDefinition slash;
    slash.id = "combo_1";
    slash.name = "Test Slash";
    slash.damage = 25.0F;
    slash.animationState = "Attack";
    slash.hitboxRadius = 0.2F;
    slash.hitboxOffset = {2.0F, 0.85F, 0.0F};
    slash.activeStart = 0.0F;
    slash.activeEnd = 1.0F;
    slash.comboWindow = 0.3F;
    slash.cancelWindowStart = 1.1F;
    return glory::game::SkillRegistry::fromSkills({slash});
}

} // namespace

int main() {
    using namespace glory::game;
    try {
        LagCompensator lagCompensator;
        for (std::uint64_t tick = 0; tick <= 24; ++tick) {
            const float x = static_cast<float>(tick) / 6.0F;
            lagCompensator.record(tick, {{7, {{x, 0.0F, 0.0F}, 0.4F, 1.8F}, 100.0F, 100.0F}});
        }

        const auto exact = lagCompensator.history().sample(12.0, 7);
        require(exact && std::abs(exact->hurtbox.base.x - 2.0F) < 0.001F,
            "history query at an exact tick returned the wrong position");
        const auto interpolated = lagCompensator.history().sample(11.5, 7);
        require(interpolated && std::abs(interpolated->hurtbox.base.x - (11.5F / 6.0F)) < 0.001F,
            "history query did not interpolate the hurtbox position between ticks");

        const auto current = lagCompensator.history().sample(24.0, 7);
        require(current && std::abs(current->hurtbox.base.x - 4.0F) < 0.001F,
            "current target position should be X=4");
        const auto rewound = lagCompensator.rewindHurtbox(7, 24, 12.0);
        require(rewound && std::abs(rewound->entity.hurtbox.base.x - 2.0F) < 0.001F,
            "lag compensation did not rewind the target to the rendered tick");

        CombatTarget currentTarget{7, current->hurtbox, 100.0F, 100.0F, 0};
        CombatSystem uncompensated(makeTestSkills(), currentTarget);
        uncompensated.update(1.0F / 60.0F, glm::vec3(0.0F), 0.0F, true, "Attack");
        require(uncompensated.hitEvents().empty(),
            "attack should miss the current target at X=4 without rewind");

        CombatSystem compensated(makeTestSkills(), currentTarget);
        compensated.update(1.0F / 60.0F, glm::vec3(0.0F), 0.0F, true, "Attack", false, false,
            1.0F, false, &rewound->entity.hurtbox);
        require(compensated.hitEvents().size() == 1 && compensated.dummy().currentHp == 75.0F,
            "attack against historical target position did not apply damage");
        require(std::abs(compensated.dummy().hurtbox.base.x - 4.0F) < 0.001F,
            "rewind changed the target's authoritative current position");

        const auto clampedPast = lagCompensator.rewindHurtbox(7, 24, 0.0);
        require(clampedPast && clampedPast->sampledTick == 3.0 &&
            std::abs(clampedPast->entity.hurtbox.base.x - 0.5F) < 0.001F,
            "rewind older than 0.35 seconds was not clamped to 21 ticks");
        const auto clampedFuture = lagCompensator.rewindHurtbox(7, 24, 100.0);
        require(clampedFuture && clampedFuture->sampledTick == 24.0 &&
            std::abs(clampedFuture->entity.hurtbox.base.x - 4.0F) < 0.001F,
            "future target tick was not clamped to the current server tick");

        WorldHistoryBuffer oneSecondHistory(61);
        for (std::uint64_t tick = 0; tick <= 70; ++tick)
            oneSecondHistory.record(tick, {{7, {{static_cast<float>(tick), 0.0F, 0.0F},
                0.4F, 1.8F}, 100.0F, 100.0F}});
        require(oneSecondHistory.size() == 61 && oneSecondHistory.oldestTick() == 10 &&
            !oneSecondHistory.sample(9.0, 7), "world history did not retain exactly its bounded one-second window");

        std::cout << "Lag compensation passed: history interpolation, rewound hit, current-state restore, clamp\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
