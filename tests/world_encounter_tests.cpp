#include "glory/game/world_encounter.hpp"
#include "glory/game/character_controller.hpp"

#include <glm/geometric.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
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
        const auto sourcePath = std::filesystem::path(GLORY_ASSET_DIR) / "data" / "world_spawns.json";
        const auto tempPath = std::filesystem::temp_directory_path() / "glory_world_spawns_test.json";
        WorldEncounter world = WorldEncounter::loadFromFile(sourcePath);
        require(world.spawnPoints().size() == 3 && world.triggerZones().size() == 4,
            "world JSON should load all spawn points and zones");
        world.saveToFile(tempPath);
        WorldEncounter roundTrip = WorldEncounter::loadFromFile(tempPath);
        require(roundTrip.findSpawn(SpawnEntityType::Boss) != nullptr &&
            std::abs(roundTrip.findSpawn(SpawnEntityType::Boss)->position.x + 3.8F) < 1.0e-5F,
            "world serialization should preserve spawn positions");

        const auto box = std::find_if(roundTrip.triggerZones().begin(), roundTrip.triggerZones().end(),
            [](const TriggerZone& zone) { return zone.shape == TriggerShape::Box; });
        const auto sphere = std::find_if(roundTrip.triggerZones().begin(), roundTrip.triggerZones().end(),
            [](const TriggerZone& zone) { return zone.shape == TriggerShape::Sphere; });
        require(box != roundTrip.triggerZones().end() && WorldEncounter::intersects(box->center, *box),
            "box trigger should contain its center");
        require(sphere != roundTrip.triggerZones().end() &&
            WorldEncounter::intersects(sphere->center + glm::vec3(sphere->radius, 0.0F, 0.0F), *sphere),
            "sphere trigger should include its surface");
        require(!WorldEncounter::intersects(sphere->center + glm::vec3(sphere->radius + 0.1F, 0.0F, 0.0F), *sphere),
            "sphere trigger should exclude points outside its radius");

        TriggerZone testZone{"test_zone", "Test Zone", TriggerShape::Sphere, glm::vec3(0.0F), glm::vec3(1.0F), 2.0F,
            TriggerType::BossAggro};
        WorldEncounter transitions = WorldEncounter::fromData(
            std::vector<SpawnPoint>{SpawnPoint{"player", "Player", SpawnEntityType::Player, glm::vec3(0.0F), 0.0F}},
            {testZone});
        require(transitions.updateTriggers({3.0F, 0.0F, 0.0F}).empty(), "outside position should not trigger");
        auto events = transitions.updateTriggers({0.0F, 0.0F, 0.0F});
        require(events.size() == 1 && events[0].transition == TriggerTransition::Enter,
            "entering a trigger should produce Enter");
        events = transitions.updateTriggers({0.5F, 0.0F, 0.0F});
        require(events.size() == 1 && events[0].transition == TriggerTransition::Inside,
            "remaining in a trigger should produce Inside");
        events = transitions.updateTriggers({3.0F, 0.0F, 0.0F});
        require(events.size() == 1 && events[0].transition == TriggerTransition::Exit &&
            transitions.activeTriggerIds().empty(), "leaving a trigger should produce Exit and clear active state");

        CharacterController controller;
        const SpawnPoint* playerSpawn = roundTrip.findSpawn(SpawnEntityType::Player);
        require(playerSpawn != nullptr, "player spawn point should be available");
        controller.teleport(playerSpawn->position, playerSpawn->yaw);
        require(controller.state().position == playerSpawn->position &&
            std::abs(controller.state().facingYaw - playerSpawn->yaw) < 1.0e-5F &&
            glm::length(controller.state().velocity) == 0.0F,
            "applying a spawn should place and reset player movement state");

        bool rejectedInvalidExtents = false;
        testZone.shape = TriggerShape::Box;
        testZone.extents.x = 0.0F;
        try {
            (void)WorldEncounter::fromData(
                std::vector<SpawnPoint>{SpawnPoint{"player", "Player", SpawnEntityType::Player, glm::vec3(0.0F), 0.0F}},
                {testZone});
        }
        catch (const std::runtime_error&) { rejectedInvalidExtents = true; }
        require(rejectedInvalidExtents, "box trigger with zero extent should fail validation");
        {
            std::ofstream invalidFile(tempPath, std::ios::trunc);
            invalidFile << R"({"spawnPoints":[],"triggerZones":[{"id":"bad","name":"Bad","shape":"Capsule","center":[0,0,0],"triggerType":"BossAggro"}]})";
        }
        bool rejectedInvalidShape = false;
        try { (void)WorldEncounter::loadFromFile(tempPath); }
        catch (const std::runtime_error&) { rejectedInvalidShape = true; }
        require(rejectedInvalidShape, "unsupported trigger shape should fail loading");
        require(world.findSpawn(SpawnEntityType::Boss) != nullptr,
            "failed candidate reload must leave current world data usable");

        std::filesystem::remove(tempPath);
        std::cout << "World spawn, trigger validation, transitions, and placement tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
