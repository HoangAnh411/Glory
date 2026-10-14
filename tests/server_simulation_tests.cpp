#include "glory/server/server_simulation.hpp"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace {

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

std::filesystem::path dataPath(const char* fileName) {
    return std::filesystem::path(GLORY_ASSET_DIR) / "data" / fileName;
}

} // namespace

int main() {
    try {
        using namespace glory;
        server::ServerSimulation simulation(dataPath("world_spawns.json"), dataPath("skills.json"));
        require(std::abs(server::ServerSimulation::fixedDeltaSeconds - 1.0 / 60.0) < 1.0e-12,
            "server fixed timestep should be 60 Hz");
        for (int index = 0; index < 120; ++index) simulation.tick();
        const server::ServerSnapshot timedState = simulation.snapshot();
        require(timedState.tick == 120 && std::abs(timedState.elapsedSeconds - 2.0) < 1.0e-9,
            "server simulation should accumulate exactly 120 fixed ticks / 2 seconds");

        server::ServerSimulation collisionSimulation(dataPath("world_spawns.json"), dataPath("skills.json"));
        collisionSimulation.player().teleport({19.0F, 0.0F, 0.0F}, 0.0F);
        input::InputFrame moveRight;
        moveRight.held.push_back(input::Control::KeyW);
        bool collisionObserved = false;
        for (int index = 0; index < 24; ++index) {
            collisionSimulation.tick(moveRight, 0.0F);
            collisionObserved = collisionObserved || collisionSimulation.player().state().collisionCount > 0;
        }
        require(collisionObserved && collisionSimulation.player().state().position.x < 20.1F,
            "headless server character movement should resolve sample world collision");

        server::ServerSimulation combatSimulation(dataPath("world_spawns.json"), dataPath("skills.json"));
        combatSimulation.boss().setSpawnTransform({-1.15F, 0.0F, 0.0F}, 0.0F);
        input::InputFrame attack;
        attack.pressed.push_back(input::Control::MouseLeft);
        attack.held.push_back(input::Control::MouseLeft);
        combatSimulation.tick(attack, -90.0F);
        input::InputFrame neutral;
        combatSimulation.tick(neutral, -90.0F);
        combatSimulation.tick(neutral, -90.0F);
        combatSimulation.tick(neutral, -90.0F);
        require(combatSimulation.combat().dummy().hitCount == 1 &&
            combatSimulation.combat().dummy().currentHp < 300.0F,
            "headless server combat should apply skill damage without render context");

        std::cout << "Headless server tick, timing, collision, and combat tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
