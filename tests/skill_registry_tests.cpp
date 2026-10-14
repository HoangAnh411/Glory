#include "glory/game/combat.hpp"

#include <cmath>
#include <filesystem>
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
        const auto sourcePath = std::filesystem::path(GLORY_ASSET_DIR) / "data" / "skills.json";
        const auto tempPath = std::filesystem::temp_directory_path() / "glory_skill_registry_test.json";
        SkillRegistry original = SkillRegistry::loadFromFile(sourcePath);
        original.saveToFile(tempPath);
        SkillRegistry roundTrip = SkillRegistry::loadFromFile(tempPath);
        require(roundTrip.skills().size() == original.skills().size(), "serialize round-trip should preserve skill count");
        require(roundTrip.find("slash") && roundTrip.find("slash")->animationEvents.size() == 2,
            "serialize round-trip should preserve animation events");
        require(roundTrip.find("heavy_strike")->onHitEffects.front().kind == StatusEffectKind::Stun,
            "serialize round-trip should preserve status effect types");

        SkillDefinition invalid = *roundTrip.find("slash");
        invalid.cooldown = -1.0F;
        bool rejectedNegative = false;
        try { (void)SkillRegistry::fromSkills({invalid}); }
        catch (const std::runtime_error&) { rejectedNegative = true; }
        require(rejectedNegative, "negative skill values should fail validation");
        invalid = *roundTrip.find("slash");
        invalid.damage = std::nanf("");
        bool rejectedNan = false;
        try { (void)SkillRegistry::fromSkills({invalid}); }
        catch (const std::runtime_error&) { rejectedNan = true; }
        require(rejectedNan, "NaN skill values should fail validation");
        invalid = *roundTrip.find("slash");
        invalid.activeEnd = invalid.activeStart;
        bool rejectedWindow = false;
        try { (void)SkillRegistry::fromSkills({invalid}); }
        catch (const std::runtime_error&) { rejectedWindow = true; }
        require(rejectedWindow, "invalid hitbox timing should fail validation");

        CombatSystem combat(roundTrip, CombatTarget{9, {{-1.15F, 0.0F, 0.0F}, 0.4F, 1.8F}, 100.0F, 100.0F});
        const glm::vec3 originalTargetPosition = combat.dummy().hurtbox.base;
        combat.dummy().currentHp = 73.0F;
        SkillDefinition changed = *roundTrip.find("combo_1");
        changed.damage = 37.0F;
        changed.id = "combo_1";
        std::vector<SkillDefinition> updatedSkills = roundTrip.skills();
        for (auto& skill : updatedSkills) if (skill.id == changed.id) skill = changed;
        SkillRegistry updated = SkillRegistry::fromSkills(std::move(updatedSkills));
        updated.saveToFile(tempPath);
        combat.replaceRegistry(SkillRegistry::loadFromFile(tempPath));
        require(combat.dummy().currentHp == 73.0F && combat.dummy().hurtbox.base == originalTargetPosition,
            "hot reload must preserve encounter health and position");
        combat.requestSkill("combo_1");
        combat.update(0.05F, glm::vec3(0.0F), -90.0F, false, "Attack");
        require(std::abs(combat.dummy().currentHp - 36.0F) < 1.0e-4F,
            "hot reload should apply the replacement skill definition");

        std::filesystem::remove(tempPath);
        std::cout << "Skill registry serialization, validation, and hot-reload tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
