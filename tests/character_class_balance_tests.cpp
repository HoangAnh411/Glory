#include "glory/game/classes.hpp"

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
glory::game::SkillRegistry loadSkills() {
    return glory::game::SkillRegistry::loadFromFile(
        std::filesystem::path(GLORY_ASSET_DIR) / "data" / "skills.json");
}

void testClassesAndValidation() {
    using namespace glory::game;
    const auto skills = loadSkills();
    const auto catalog = ClassCatalog::loadFromFile(
        std::filesystem::path(GLORY_ASSET_DIR) / "data" / "classes.json", skills);
    require(catalog.classes().size() == 3, "catalog loads all three classes");
    const auto* warrior = catalog.find("Warrior");
    const auto* assassin = catalog.find("Assassin");
    const auto* mage = catalog.find("Mage");
    require(warrior && warrior->baseHp == 150.0F && warrior->baseAttack == 25.0F &&
        warrior->baseDefense == 15.0F && warrior->moveSpeed == 4.5F, "Warrior balance values");
    require(assassin && assassin->baseHp == 90.0F && assassin->baseAttack == 32.0F &&
        assassin->baseDefense == 5.0F && assassin->moveSpeed == 6.0F &&
        assassin->dodgeCooldown == 0.5F, "Assassin balance values");
    require(mage && mage->baseHp == 80.0F && mage->baseAttack == 40.0F &&
        mage->baseDefense == 2.0F && mage->moveSpeed == 4.0F, "Mage balance values");
    require(!catalog.find("Unknown"), "unknown classes are rejected");
    const auto warriorSkills = catalog.skillsForClass("Warrior", skills);
    const auto assassinSkills = catalog.skillsForClass("Assassin", skills);
    const auto mageSkills = catalog.skillsForClass("Mage", skills);
    require(warriorSkills.skills().size() == 2 && warriorSkills.find("shield_bash") &&
        warriorSkills.find("shield_bash")->onHitEffects.front().kind == StatusEffectKind::Stun,
        "Warrior receives dedicated stun skills");
    require(assassinSkills.skills().size() == 2 && assassinSkills.find("twin_daggers") &&
        assassinSkills.find("twin_daggers")->onHitEffects.front().kind == StatusEffectKind::Bleed,
        "Assassin receives dedicated bleed skills");
    require(mageSkills.skills().size() == 2 && mageSkills.find("fireball") &&
        mageSkills.find("fireball")->hitboxRadius > 3.0F &&
        mageSkills.find("fireball")->onHitEffects.front().kind == StatusEffectKind::Burn &&
        mageSkills.find("frost_nova")->onHitEffects.front().kind == StatusEffectKind::Slow,
        "Mage receives ranged area and control skills");
    bool rejected = false;
    try { (void)catalog.skillsForClass("Unknown", skills); }
    catch (const std::invalid_argument&) { rejected = true; }
    require(rejected, "unknown class skill selection throws");
}

void testDefenseMitigationAndCombat() {
    using namespace glory::game;
    require(std::abs(calculateDamageTaken(100.0F, 0.0F) - 100.0F) < 1.0e-5F,
        "zero defense preserves incoming damage");
    require(std::abs(calculateDamageTaken(100.0F, 100.0F) - 50.0F) < 1.0e-5F,
        "mitigation follows the specified formula");
    require(calculateDamageTaken(100.0F, 100.0F) < calculateDamageTaken(100.0F, 0.0F),
        "higher defense receives less damage");
    require(calculateDamageTaken(0.1F, 10000.0F) == 1.0F,
        "incoming damage has a minimum of one");

    const auto allSkills = loadSkills();
    const auto catalog = ClassCatalog::loadFromFile(
        std::filesystem::path(GLORY_ASSET_DIR) / "data" / "classes.json", allSkills);
    const Hurtbox target{{-1.1F, 0.0F, 0.0F}, 0.5F, 2.0F};
    const auto applySkill = [&](const char* className, const char* skillId) {
        CombatSystem combat(catalog.skillsForClass(className, allSkills),
            CombatTarget{7, target, 100.0F, 100.0F, 0});
        combat.requestSkill(skillId);
        for (int tick = 0; tick < 16 && combat.hitEvents().empty(); ++tick)
            combat.update(0.05F, glm::vec3(0.0F), -90.0F, false, "Attack");
        return combat;
    };
    auto warrior = applySkill("Warrior", "shield_bash");
    auto assassin = applySkill("Assassin", "twin_daggers");
    require(warrior.dummy().hitCount == 1 && warrior.dummy().statusEffects.isStunned(),
        "Warrior shield bash applies stun in combat");
    require(assassin.dummy().hitCount == 1 && !assassin.dummy().statusEffects.effects().empty() &&
        assassin.dummy().statusEffects.effects().front().kind == StatusEffectKind::Bleed,
        "Assassin daggers apply bleed in combat");
    CombatSystem mage(catalog.skillsForClass("Mage", allSkills),
        CombatTarget{7, {{-5.8F, 0.0F, 0.0F}, 0.6F, 2.0F}, 100.0F, 100.0F, 0});
    mage.requestSkill("fireball");
    for (int tick = 0; tick < 20 && mage.hitEvents().empty(); ++tick)
        mage.update(0.05F, glm::vec3(0.0F), -90.0F, false, "Attack");
    require(mage.dummy().hitCount == 1 && !mage.dummy().statusEffects.effects().empty() &&
        mage.dummy().statusEffects.effects().front().kind == StatusEffectKind::Burn,
        "Mage ranged fireball applies burn to a distant target");
    CombatSystem armored(allSkills, CombatTarget{7, target, 200.0F, 200.0F, 0});
    armored.dummy().defense = 100.0F;
    armored.requestSkill("combo_1");
    armored.update(0.05F, glm::vec3(0.0F), -90.0F, false, "Attack");
    require(armored.dummy().currentHp > 180.0F,
        "combat damage is mitigated by target armor");
}
}

int main() {
    try {
        testClassesAndValidation();
        testDefenseMitigationAndCombat();
        std::cout << "Character class balance tests passed\n";
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
