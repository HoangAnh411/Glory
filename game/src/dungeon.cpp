#include "glory/game/dungeon.hpp"

#include <algorithm>
#include <cmath>
#include <unordered_set>

namespace glory::game {

DungeonManager::DungeonManager() : definitions_{{}} {}

std::optional<std::uint64_t> DungeonManager::createInstance(std::uint32_t dungeonId,
    const std::vector<DungeonMember>& members) {
    const auto definition = std::find_if(definitions_.begin(), definitions_.end(),
        [dungeonId](const DungeonDefinition& value) { return value.dungeonId == dungeonId; });
    if (definition == definitions_.end() || members.empty() || members.size() > 5 ||
        nextInstanceId_ == 0) return std::nullopt;
    std::unordered_set<std::int64_t> uniqueIds;
    for (const auto& member : members) {
        if (member.characterId <= 0 || !uniqueIds.insert(member.characterId).second ||
            characterInstances_.contains(member.characterId) || !std::isfinite(member.currentHp) ||
            !std::isfinite(member.maxHp) || member.maxHp <= 0.0F || member.currentHp < 0.0F ||
            member.currentHp > member.maxHp || !std::isfinite(member.originPosition.x) ||
            !std::isfinite(member.originPosition.y) || !std::isfinite(member.originPosition.z)) return std::nullopt;
    }
    const std::uint64_t id = nextInstanceId_++;
    DungeonInstance instance;
    instance.instanceId = id;
    instance.definition = *definition;
    instance.timerRemaining = definition->durationSeconds;
    instance.bossHealth = definition->bossMaxHp;
    instance.playerSpawn = definition->playerSpawn + glm::vec3(static_cast<float>(id) * 1000.0F, 0.0F, 0.0F);
    instance.bossSpawn = definition->bossSpawn + glm::vec3(static_cast<float>(id) * 1000.0F, 0.0F, 0.0F);
    instance.members = members;
    for (const auto& member : members) characterInstances_.emplace(member.characterId, id);
    instances_.emplace(id, std::move(instance));
    return id;
}

bool DungeonManager::applyBossDamage(std::uint64_t instanceId, std::int64_t characterId, float damage) {
    auto* instance = find(instanceId);
    if (!instance || instance->state != DungeonState::Active || !std::isfinite(damage) || damage <= 0.0F ||
        std::none_of(instance->members.begin(), instance->members.end(), [characterId](const DungeonMember& member) {
            return member.characterId == characterId && member.currentHp > 0.0F;
        })) return false;
    instance->bossHealth = std::max(0.0F, instance->bossHealth - damage);
    if (instance->bossHealth <= 0.0F) instance->state = DungeonState::Completed;
    return true;
}

bool DungeonManager::updateMember(std::uint64_t instanceId, std::int64_t characterId, float currentHp,
    const glm::vec3& position) {
    auto* instance = find(instanceId);
    if (!instance || instance->state != DungeonState::Active || !std::isfinite(currentHp) ||
        !std::isfinite(position.x) || !std::isfinite(position.y) || !std::isfinite(position.z)) return false;
    const auto member = std::find_if(instance->members.begin(), instance->members.end(),
        [characterId](const DungeonMember& value) { return value.characterId == characterId; });
    if (member == instance->members.end() || currentHp < 0.0F || currentHp > member->maxHp) return false;
    member->currentHp = currentHp;
    return true;
}

void DungeonManager::tick(float deltaSeconds) {
    if (!std::isfinite(deltaSeconds) || deltaSeconds <= 0.0F) return;
    for (auto& [id, instance] : instances_) {
        (void)id;
        if (instance.state != DungeonState::Active) continue;
        instance.timerRemaining = std::max(0.0F, instance.timerRemaining - deltaSeconds);
        const bool wiped = std::all_of(instance.members.begin(), instance.members.end(),
            [](const DungeonMember& member) { return member.currentHp <= 0.0F; });
        if (instance.timerRemaining <= 0.0F || wiped) instance.state = DungeonState::Failed;
    }
}

std::optional<DungeonMember> DungeonManager::removeMember(std::uint64_t instanceId, std::int64_t characterId) {
    auto* instance = find(instanceId);
    if (!instance) return std::nullopt;
    const auto member = std::find_if(instance->members.begin(), instance->members.end(),
        [characterId](const DungeonMember& value) { return value.characterId == characterId; });
    if (member == instance->members.end()) return std::nullopt;
    DungeonMember removed = *member;
    instance->members.erase(member);
    characterInstances_.erase(characterId);
    if (instance->members.empty()) instance->state = DungeonState::Closed;
    return removed;
}

bool DungeonManager::closeInstance(std::uint64_t instanceId) {
    const auto found = instances_.find(instanceId);
    if (found == instances_.end() || !found->second.members.empty()) return false;
    for (const auto& member : found->second.members) characterInstances_.erase(member.characterId);
    instances_.erase(found);
    return true;
}

const DungeonInstance* DungeonManager::find(std::uint64_t instanceId) const noexcept {
    const auto found = instances_.find(instanceId);
    return found == instances_.end() ? nullptr : &found->second;
}

DungeonInstance* DungeonManager::find(std::uint64_t instanceId) noexcept {
    const auto found = instances_.find(instanceId);
    return found == instances_.end() ? nullptr : &found->second;
}

const DungeonInstance* DungeonManager::activeInstanceForCharacter(std::int64_t characterId) const noexcept {
    const auto membership = characterInstances_.find(characterId);
    if (membership == characterInstances_.end()) return nullptr;
    const auto* instance = find(membership->second);
    return instance && instance->state == DungeonState::Active ? instance : nullptr;
}

} // namespace glory::game
