#pragma once

#include <glm/vec3.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace glory::game {

enum class DungeonState : std::uint8_t { Active, Completed, Failed, Closed };

struct DungeonDefinition {
    std::uint32_t dungeonId = 1;
    std::string name = "The Shadow Crypt";
    float durationSeconds = 900.0F;
    glm::vec3 playerSpawn{0.0F, 0.0F, 0.0F};
    glm::vec3 bossSpawn{0.0F, 0.0F, -12.0F};
    float bossMaxHp = 1000.0F;
    std::uint32_t rewardExp = 250;
    std::string rewardItemId = "training_blade";
};

struct DungeonMember {
    std::int64_t characterId = 0;
    std::uint8_t originZone = 0;
    glm::vec3 originPosition{0.0F};
    float currentHp = 100.0F;
    float maxHp = 100.0F;
};

struct DungeonInstance {
    std::uint64_t instanceId = 0;
    DungeonDefinition definition;
    DungeonState state = DungeonState::Active;
    float timerRemaining = 0.0F;
    float bossHealth = 0.0F;
    glm::vec3 playerSpawn{0.0F};
    glm::vec3 bossSpawn{0.0F};
    std::vector<DungeonMember> members;
};

class DungeonManager {
public:
    DungeonManager();

    [[nodiscard]] std::optional<std::uint64_t> createInstance(std::uint32_t dungeonId,
        const std::vector<DungeonMember>& members);
    [[nodiscard]] bool applyBossDamage(std::uint64_t instanceId, std::int64_t characterId, float damage);
    [[nodiscard]] bool updateMember(std::uint64_t instanceId, std::int64_t characterId, float currentHp,
        const glm::vec3& position);
    void tick(float deltaSeconds);
    [[nodiscard]] std::optional<DungeonMember> removeMember(std::uint64_t instanceId,
        std::int64_t characterId);
    [[nodiscard]] bool closeInstance(std::uint64_t instanceId);
    [[nodiscard]] const DungeonInstance* find(std::uint64_t instanceId) const noexcept;
    [[nodiscard]] DungeonInstance* find(std::uint64_t instanceId) noexcept;
    [[nodiscard]] const DungeonInstance* activeInstanceForCharacter(std::int64_t characterId) const noexcept;
    [[nodiscard]] const std::vector<DungeonDefinition>& definitions() const noexcept { return definitions_; }
    [[nodiscard]] const std::unordered_map<std::uint64_t, DungeonInstance>& instances() const noexcept {
        return instances_;
    }

private:
    std::vector<DungeonDefinition> definitions_;
    std::unordered_map<std::uint64_t, DungeonInstance> instances_;
    std::unordered_map<std::int64_t, std::uint64_t> characterInstances_;
    std::uint64_t nextInstanceId_ = 1;
};

} // namespace glory::game
