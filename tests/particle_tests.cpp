#include "glory/engine/particle_system.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>

namespace {
void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}
}

int main() {
    using namespace glory::engine;
    ParticleSystem system;
    ParticleConfig config;
    config.direction = {0.0F, 1.0F, 0.0F};
    config.gravity = {0.0F, -10.0F, 0.0F};
    config.coneAngleRadians = 0.0F;
    config.minimumSpeed = config.maximumSpeed = 2.0F;
    config.minimumSize = config.maximumSize = 0.1F;
    config.lifetime = 0.2F;
    require(system.spawnBurst({1.0F, 2.0F, 3.0F}, 3, config) == 3 && system.activeCount() == 3,
        "Burst must create the requested particle count");
    system.update(0.1F);
    const Particle& particle = system.particles().front();
    require(std::abs(particle.velocity.y - 1.0F) < 1.0e-5F, "Gravity must update particle velocity");
    require(std::abs(particle.position.y - 2.1F) < 1.0e-5F, "Updated velocity must move the particle");
    require(std::abs(particle.color.a - 0.5F) < 1.0e-5F, "Particle alpha must fade with remaining lifetime");
    system.update(0.11F);
    require(system.activeCount() == 0, "Expired particles must be removed");
    return 0;
}
