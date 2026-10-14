#pragma once

#include <glm/vec3.hpp>

#include <filesystem>
#include <string>
#include <vector>

namespace glory::game {

enum class SpawnEntityType { Player, PracticeDummy, Boss };
enum class TriggerShape { Box, Sphere };
enum class TriggerType { EncounterBoundary, BossAggro, HealArea, HazardArea };
enum class TriggerTransition { Enter, Inside, Exit };

struct SpawnPoint {
    std::string id;
    std::string name;
    SpawnEntityType entityType = SpawnEntityType::Player;
    glm::vec3 position{0.0F};
    float yaw = 0.0F;
};

struct TriggerZone {
    std::string id;
    std::string name;
    TriggerShape shape = TriggerShape::Box;
    glm::vec3 center{0.0F};
    glm::vec3 extents{1.0F};
    float radius = 1.0F;
    TriggerType triggerType = TriggerType::EncounterBoundary;
};

struct WorldTriggerEvent {
    std::string id;
    std::string name;
    TriggerType type = TriggerType::EncounterBoundary;
    TriggerTransition transition = TriggerTransition::Enter;
};

class WorldEncounter {
public:
    static WorldEncounter loadFromFile(const std::filesystem::path& path);
    static WorldEncounter fromData(std::vector<SpawnPoint> spawnPoints,
        std::vector<TriggerZone> triggerZones);

    void saveToFile(const std::filesystem::path& path) const;
    [[nodiscard]] const std::vector<SpawnPoint>& spawnPoints() const noexcept { return spawnPoints_; }
    [[nodiscard]] const std::vector<TriggerZone>& triggerZones() const noexcept { return triggerZones_; }
    [[nodiscard]] const SpawnPoint* findSpawn(SpawnEntityType entityType) const noexcept;
    [[nodiscard]] const SpawnPoint* findSpawn(const std::string& id) const noexcept;
    [[nodiscard]] std::vector<WorldTriggerEvent> updateTriggers(const glm::vec3& playerPosition);
    [[nodiscard]] std::vector<std::string> activeTriggerIds() const;

    [[nodiscard]] static bool intersects(const glm::vec3& point, const TriggerZone& zone) noexcept;

private:
    std::vector<SpawnPoint> spawnPoints_;
    std::vector<TriggerZone> triggerZones_;
    std::vector<std::string> activeTriggerIds_;
};

} // namespace glory::game
