#include "glory/game/classes.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <stdexcept>
#include <unordered_set>

namespace glory::game {
namespace {

using Json = nlohmann::json;

float readPositive(const Json& object, const char* key, const std::string& context) {
    if (!object.contains(key) || !object.at(key).is_number())
        throw std::runtime_error(context + ": missing numeric field '" + key + "'");
    const float value = object.at(key).get<float>();
    if (!std::isfinite(value) || value <= 0.0F)
        throw std::runtime_error(context + ": field '" + key + "' must be positive and finite");
    return value;
}

} // namespace

ClassCatalog ClassCatalog::loadFromFile(const std::filesystem::path& path,
    const SkillRegistry& skills) {
    std::ifstream input(path);
    if (!input) throw std::runtime_error("Unable to open classes JSON: " + path.string());
    Json root;
    try {
        input >> root;
    } catch (const Json::exception& error) {
        throw std::runtime_error("Invalid classes JSON: " + std::string(error.what()));
    }
    if (!root.is_object() || !root.contains("classes") || !root.at("classes").is_array() ||
        root.at("classes").empty())
        throw std::runtime_error("Classes JSON must contain a non-empty classes array");

    ClassCatalog catalog;
    std::unordered_set<std::string> names;
    for (const auto& entry : root.at("classes")) {
        if (!entry.is_object()) throw std::runtime_error("Class entry must be an object");
        CharacterClassDefinition definition;
        if (!entry.contains("name") || !entry.at("name").is_string() ||
            !entry.contains("description") || !entry.at("description").is_string())
            throw std::runtime_error("Class name and description are required strings");
        definition.name = entry.at("name").get<std::string>();
        definition.description = entry.at("description").get<std::string>();
        if (definition.name.empty() || definition.description.empty() || !names.insert(definition.name).second)
            throw std::runtime_error("Class names/descriptions must be non-empty and names unique");
        definition.baseHp = readPositive(entry, "baseHp", definition.name);
        definition.baseAttack = readPositive(entry, "baseAttack", definition.name);
        if (!entry.contains("baseDefense") || !entry.at("baseDefense").is_number())
            throw std::runtime_error(definition.name + ": missing numeric field 'baseDefense'");
        definition.baseDefense = entry.at("baseDefense").get<float>();
        if (!std::isfinite(definition.baseDefense) || definition.baseDefense < 0.0F)
            throw std::runtime_error(definition.name + ": defense must be finite and non-negative");
        definition.moveSpeed = readPositive(entry, "moveSpeed", definition.name);
        definition.dodgeCooldown = entry.contains("dodgeCooldown")
            ? readPositive(entry, "dodgeCooldown", definition.name) : 0.8F;
        if (!entry.contains("skillIds") || !entry.at("skillIds").is_array() ||
            entry.at("skillIds").empty())
            throw std::runtime_error(definition.name + ": skillIds must be a non-empty array");
        std::unordered_set<std::string> classSkills;
        for (const auto& skillId : entry.at("skillIds")) {
            if (!skillId.is_string()) throw std::runtime_error(definition.name + ": skill id must be a string");
            const auto id = skillId.get<std::string>();
            if (id.empty() || !classSkills.insert(id).second || !skills.find(id))
                throw std::runtime_error(definition.name + ": duplicate or unknown skill id '" + id + "'");
            definition.skillIds.push_back(id);
        }
        catalog.classes_.push_back(std::move(definition));
    }
    return catalog;
}

const CharacterClassDefinition* ClassCatalog::find(const std::string& name) const noexcept {
    for (const auto& definition : classes_)
        if (definition.name == name) return &definition;
    return nullptr;
}

SkillRegistry ClassCatalog::skillsForClass(const std::string& name,
    const SkillRegistry& allSkills) const {
    const auto* definition = find(name);
    if (!definition) throw std::invalid_argument("Unknown character class: " + name);
    std::vector<SkillDefinition> selected;
    selected.reserve(definition->skillIds.size());
    for (const auto& id : definition->skillIds) {
        const auto* skill = allSkills.find(id);
        if (!skill) throw std::runtime_error("Class skill is missing from registry: " + id);
        selected.push_back(*skill);
    }
    return SkillRegistry::fromSkills(std::move(selected));
}

float calculateDamageTaken(float damage, float defense) noexcept {
    if (!std::isfinite(damage) || damage <= 0.0F) return 1.0F;
    if (!std::isfinite(defense)) defense = 0.0F;
    defense = std::max(0.0F, defense);
    return std::max(1.0F, damage * (100.0F / (100.0F + defense)));
}

} // namespace glory::game
