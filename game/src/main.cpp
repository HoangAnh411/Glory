#include "glory/engine/application.hpp"
#include "glory/game/simulation.hpp"

#include <cstring>
#include <iostream>

int main(int argc, char* argv[]) {
    glory::engine::ApplicationConfig config;
    if (argc == 2 && std::strcmp(argv[1], "--smoke-test") == 0) {
        config.smokeTestTickLimit = 120;
    } else if (argc > 1) {
        std::cerr << "Usage: glory_client [--smoke-test]\n";
        return 2;
    }

    glory::game::Simulation simulation;
    const int result = glory::engine::runApplication(config, [&](double fixedDeltaSeconds) {
        simulation.fixedUpdate(fixedDeltaSeconds);
    });

    if (result == 0 && config.smokeTestTickLimit != 0) {
        if (simulation.tickCount() != config.smokeTestTickLimit) {
            std::cerr << "Smoke test stopped at tick " << simulation.tickCount() << '\n';
            return 1;
        }
        std::cout << "SDL3/OpenGL 4.5 context and 120 fixed ticks passed\n";
    }

    return result;
}
