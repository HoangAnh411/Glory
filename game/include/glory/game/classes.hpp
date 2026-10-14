#pragma once

#include "glory/game/combat.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace glory::game {

struct CharacterClassDefinition {
    std::string name;
    std::string description;
    float baseHp = 100.0F;
    float baseAttack = 10.0F;
    float baseDefense = 0.0F;
    float moveSpeed = 5.5F;
    float dodgeCooldown = 0.8F;
    std::vector<std::string> skillIds;
};

class ClassCatalog {
public:
    static ClassCatalog loadFromFile(const std::filesystem::path& path,
        const SkillRegistry& skills);
    [[nodiscard]] const CharacterClassDefinition* find(const std::string& name) const noexcept;
    [[nodiscard]] const std::vector<CharacterClassDefinition>& classes() const noexcept { return classes_; }
    [[nodiscard]] SkillRegistry skillsForClass(const std::string& name,
        const SkillRegistry& allSkills) const;

private:
    std::vector<CharacterClassDefinition> classes_;
};

[[nodiscard]] float calculateDamageTaken(float damage, float defense) noexcept;

} // namespace glory::game
