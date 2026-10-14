#include "glory/engine/particle_system.hpp"

#include <algorithm>
#include <cmath>
#include <random>

#include <glm/geometric.hpp>

namespace glory::engine {

std::size_t ParticleSystem::spawnBurst(const glm::vec3& position, std::size_t count,
    const ParticleConfig& config) {
    constexpr std::size_t maximumBurst = 10000;
    constexpr std::size_t maximumActive = 50000;
    if (count == 0 || !std::isfinite(config.lifetime) || config.lifetime <= 0.0F ||
        !std::isfinite(config.minimumSpeed) || !std::isfinite(config.maximumSpeed) ||
        !std::isfinite(config.coneAngleRadians) || particles_.size() >= maximumActive) return 0;
    count = std::min({count, maximumBurst, maximumActive - particles_.size()});

    glm::vec3 axis = config.direction;
    const float axisLength = glm::length(axis);
    if (!std::isfinite(axisLength) || axisLength <= 1.0e-6F) axis = {0.0F, 1.0F, 0.0F};
    else axis /= axisLength;
    const glm::vec3 reference = std::abs(axis.y) < 0.99F ? glm::vec3(0.0F, 1.0F, 0.0F) : glm::vec3(1.0F, 0.0F, 0.0F);
    const glm::vec3 tangent = glm::normalize(glm::cross(reference, axis));
    const glm::vec3 bitangent = glm::cross(axis, tangent);
    const float minimumSpeed = std::max(0.0F, std::min(config.minimumSpeed, config.maximumSpeed));
    const float maximumSpeed = std::max(0.0F, std::max(config.minimumSpeed, config.maximumSpeed));
    const float minimumSize = std::max(0.0F, std::min(config.minimumSize, config.maximumSize));
    const float maximumSize = std::max(0.0F, std::max(config.minimumSize, config.maximumSize));
    const float coneCosine = std::cos(std::clamp(config.coneAngleRadians, 0.0F, 3.14159265F));
    std::mt19937 random(config.randomSeed);
    std::uniform_real_distribution<float> unit(0.0F, 1.0F);

    particles_.reserve(particles_.size() + count);
    for (std::size_t index = 0; index < count; ++index) {
        const float cosine = 1.0F - unit(random) * (1.0F - coneCosine);
        const float sine = std::sqrt(std::max(0.0F, 1.0F - cosine * cosine));
        const float angle = unit(random) * 6.28318530718F;
        const glm::vec3 direction = glm::normalize(axis * cosine + tangent * (sine * std::cos(angle)) +
            bitangent * (sine * std::sin(angle)));
        Particle particle;
        particle.position = position;
        particle.velocity = direction * (minimumSpeed + unit(random) * (maximumSpeed - minimumSpeed));
        particle.acceleration = config.gravity;
        particle.color = config.color;
        particle.size = minimumSize + unit(random) * (maximumSize - minimumSize);
        particle.lifetime = config.lifetime;
        particle.maxLifetime = config.lifetime;
        particle.initialAlpha = config.color.a;
        particles_.push_back(particle);
    }
    return count;
}

void ParticleSystem::update(float deltaSeconds) noexcept {
    if (!std::isfinite(deltaSeconds) || deltaSeconds <= 0.0F) return;
    for (Particle& particle : particles_) {
        particle.velocity += particle.acceleration * deltaSeconds;
        particle.position += particle.velocity * deltaSeconds;
        particle.lifetime -= deltaSeconds;
        const float remaining = particle.maxLifetime > 0.0F
            ? std::clamp(particle.lifetime / particle.maxLifetime, 0.0F, 1.0F) : 0.0F;
        particle.color.a = particle.initialAlpha * remaining;
    }
    std::erase_if(particles_, [](const Particle& particle) {
        return particle.lifetime <= 0.0F;
    });
}

} // namespace glory::engine
